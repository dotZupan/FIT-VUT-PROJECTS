using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

public class TcpScanner
{
    private readonly string _interfaceName;
    private readonly int _timeout;
    private readonly Random _random = new();

    public TcpScanner(string interfaceName, int timeout)
    {
        _interfaceName = interfaceName;
        _timeout = timeout;
    }

    public async Task ScanAsync(IPAddress address, int port, CancellationToken ct)
    {
        try
        {
            if (address.AddressFamily == AddressFamily.InterNetwork)    // Check if IPv4 or IPv6 adress
                await RawIPv4ScanAsync(address, port, ct);
            else
                await RawIPv6ScanAsync(address, port, ct);
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error scanning {address}:{port}: {ex.Message}");
        }
    }

    private async Task RawIPv4ScanAsync(IPAddress dstIp, int dstPort, CancellationToken ct)
{
    try
    {
        var sourceIp = NetworkInterfaceManager.GetInterfaceIPv4Address(_interfaceName);
        if (sourceIp == null)
        {
            Console.Error.WriteLine($"No IPv4 address found on {_interfaceName}");
            return;
        }

        
        var protocolTypes = new[] { ProtocolType.Tcp, (ProtocolType)255, ProtocolType.Raw };
        
        foreach (var protocolType in protocolTypes)
        {
            try
            {
                using var sendSocket = new Socket(AddressFamily.InterNetwork, SocketType.Raw, ProtocolType.Raw);
                sendSocket.SetSocketOption(SocketOptionLevel.IP, SocketOptionName.HeaderIncluded, true);
                sendSocket.Bind(new IPEndPoint(sourceIp, 0));

                using var recvSocket = new Socket(AddressFamily.InterNetwork, SocketType.Raw, ProtocolType.Tcp);
                recvSocket.Bind(new IPEndPoint(sourceIp, 0));
                recvSocket.ReceiveTimeout = _timeout;

                var srcPort = (ushort)_random.Next(1024, 65535);
                var packet = BuildIPv4SynPacket(sourceIp, dstIp, (ushort)dstPort, srcPort);
                var endpoint = new IPEndPoint(dstIp, dstPort);

                // Attempt sending and receiving packets twice before reporting filtered status
                for (int attempt = 0; attempt < 2; attempt++)
                {
                    try
                    {
                        await sendSocket.SendToAsync(new ArraySegment<byte>(packet), SocketFlags.None, endpoint);

                        var buffer = new byte[1024];
                        var receiveTask = recvSocket.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None, ct).AsTask();

                        if (await Task.WhenAny(receiveTask, Task.Delay(_timeout, ct)) == receiveTask)
                        {
                            var bytesRead = await receiveTask;
                            if (IsTcpResponse(buffer, bytesRead, srcPort, (ushort)dstPort, out string status))
                            {
                                Console.WriteLine($"{dstIp} {dstPort} tcp {status}");
                                return;
                            }
                        }

                        if (attempt == 0)
                        {
                            await Task.Delay(1000, ct);
                        }
                    }
                    catch (SocketException ex)
                    {
                        Console.Error.WriteLine($"Attempt {attempt + 1} failed: {ex.SocketErrorCode}");
                    }
                }

                Console.WriteLine($"{dstIp} {dstPort} tcp filtered");
                return;
            }
            catch (SocketException ex) when (ex.SocketErrorCode == SocketError.ProtocolNotSupported)
            {
                continue; // Continue to the next protocol type if not supported
            }
        }

        Console.Error.WriteLine($"ERROR: Could not establish raw socket for IPv4 scanning");
    }
    catch (Exception ex)
    {
        Console.Error.WriteLine($"ERROR scanning {dstIp}:{dstPort}: {ex.Message}");
    }
}
    
    
    
    private async Task RawIPv6ScanAsync(IPAddress dstIp, int dstPort, CancellationToken ct)
    {
        try
        {
            var sourceIp = NetworkInterfaceManager.GetInterfaceIPv6Address(_interfaceName);
            if (sourceIp == null)
            {
                Console.Error.WriteLine($"No IPv6 address found on {_interfaceName}");
                return;
            }

            // Fallback to TCP Connect scan if raw sockets not supported
            if (!Socket.OSSupportsIPv6)
            {
                Console.Error.WriteLine("IPv6 not supported on this system");
                return;
            }

            try
            {
                // Try standard TCP Connect scan as fallback
                using var sendsocket = new Socket(AddressFamily.InterNetworkV6, SocketType.Raw, ProtocolType.Raw);
                sendsocket.Bind(new IPEndPoint(sourceIp, 0));
                sendsocket.ReceiveTimeout = _timeout;
                sendsocket.SendTimeout = _timeout;

                int localPort;

                if (sendsocket.LocalEndPoint != null)
                {
                    localPort = ((IPEndPoint)sendsocket.LocalEndPoint).Port;
                }
                else
                {
                    Console.Error.WriteLine($"The socket ({sendsocket.ToString()}) is not bound.");
                    return;
                }


                using var recvSocket = new Socket(AddressFamily.InterNetworkV6, SocketType.Raw, ProtocolType.Tcp);
                recvSocket.Bind(new IPEndPoint(sourceIp, localPort));
                recvSocket.ReceiveTimeout = _timeout;
                var endpoint = new IPEndPoint(dstIp, 0);

                for (int attempt = 0; attempt < 2; attempt++)
                {
                    try
                    {
                        var packet = BuildIPv6SynPacket(sourceIp, dstIp, (ushort)dstPort, (ushort)localPort);
                        await sendsocket.SendToAsync(packet, endpoint);
                        
                        
                        
                        var buffer = new byte[1024];
                        var receiveTask = recvSocket.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None, ct)
                            .AsTask();

                        if (await Task.WhenAny(receiveTask, Task.Delay(_timeout, ct)) == receiveTask)
                        {
                            var bytesRead = await receiveTask;
                            if (IsTcpResponsev6(buffer, bytesRead, (ushort)localPort, (ushort)dstPort, out string status))
                            {
                                Console.WriteLine($"{dstIp} {dstPort} tcp {status}");
                                return;
                            }
                        }

                        if (attempt == 1) Console.WriteLine($"{dstIp} {dstPort} tcp filtered");
                        else await Task.Delay(1000, ct);
                    }
                    catch (SocketException ex)
                    {
                        Console.Error.WriteLine($"Raw IPv6 scan attempt {attempt + 1} failed: {ex.SocketErrorCode}");
                        if (attempt == 1) Console.WriteLine($"{dstIp} {dstPort} tcp filtered");
                    }
                }

                return; // Exit if raw scan worked

            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"ERROR scanning {dstIp}:{dstPort}: {ex.Message}");
            }
        }
        catch
        {
        }
    }

    private bool IsTcpResponse(byte[] buffer, int length, ushort srcPort, ushort dstPort, out string status)
    {
        status = "filtered";
        
        if (length < 40) return false;

        
        int ipHeaderLength = (buffer[0] & 0x0F) * 4; 
        int tcpOffset = ipHeaderLength;

        if (length < tcpOffset + 20) return false;

        // Compare source and destination ports
        ushort responseSrcPort = (ushort)((buffer[tcpOffset] << 8) | buffer[tcpOffset + 1]);
        ushort responseDstPort = (ushort)((buffer[tcpOffset + 2] << 8) | buffer[tcpOffset + 3]);
        
        if (responseSrcPort != dstPort || responseDstPort != srcPort)
            return false;

        byte flags = buffer[tcpOffset + 13];

        if ((flags & 0x12) == 0x12) // SYN-ACK
        {
            status = "open";
            return true;
        }

        if ((flags & 0x04) == 0x04) // RST
        {
            status = "closed";
            return true;
        }

        return false;
    }
    
    private bool IsTcpResponsev6(byte[] buffer, int length, ushort srcPort, ushort dstPort, out string status)
    {
        status = "filtered";
        
        if (length < 20) return false;


        int tcpOffset = 0;
        
        
        ushort responseSrcPort = (ushort)((buffer[tcpOffset] << 8) | buffer[tcpOffset + 1]);
        ushort responseDstPort = (ushort)((buffer[tcpOffset + 2] << 8) | buffer[tcpOffset + 3]);
        
        if (responseSrcPort != dstPort || responseDstPort != srcPort)
            return false;

        byte flags = buffer[tcpOffset + 13];

        if ((flags & 0x12) == 0x12) // SYN-ACK
        {
            status = "open";
            return true;
        }

        if ((flags & 0x04) == 0x04) // RST
        {
            status = "closed";
            return true;
        }

        return false;
    }
    

    private byte[] BuildIPv4SynPacket(IPAddress srcIp, IPAddress dstIp, ushort dstPort, ushort srcPort)
    {
        var packet = new byte[40]; // 20 bytes IP header + 20 bytes TCP header

        // IPv4 Header
        packet[0] = 0x45; // Version 4, IHL = 5
        packet[1] = 0x00; // DSCP, ECN
        packet[2] = 0x00; // Total Length (will be set later)
        packet[3] = 40;   // Total Length = 40 bytes
        packet[6] = 0x40; // Flags (DF)
        packet[8] = 0x40; // TTL = 64
        packet[9] = 0x06; // Protocol = TCP

        srcIp.GetAddressBytes().CopyTo(packet, 12); // Source IP
        dstIp.GetAddressBytes().CopyTo(packet, 16); // Destination IP

        // Compute IP Header Checksum
        var ipChecksum = ComputeChecksum(packet, 0, 20);
        packet[10] = (byte)(ipChecksum >> 8);
        packet[11] = (byte)(ipChecksum & 0xFF);

        // TCP Header
        packet[20] = (byte)(srcPort >> 8); // Source Port
        packet[21] = (byte)(srcPort & 0xFF);
        packet[22] = (byte)(dstPort >> 8); // Destination Port
        packet[23] = (byte)(dstPort & 0xFF);

        // Sequence Number (random)
        _random.NextBytes(packet[24..28]);

        packet[32] = 0x50; // Data Offset (5) << 4
        packet[33] = 0x02; // Flags: SYN
        packet[34] = 0xFF; // Window Size
        packet[35] = 0xFF;

        // TCP Checksum (0 for calculation)
        packet[36] = 0x00;
        packet[37] = 0x00;

        // Compute TCP Checksum with Pseudo Header
        var tcpChecksum = ComputeTcpChecksum(packet, 20, 20, srcIp, dstIp);
        packet[36] = (byte)(tcpChecksum >> 8);
        packet[37] = (byte)(tcpChecksum & 0xFF);

        return packet;
    }

    private byte[] BuildIPv6SynPacket(IPAddress srcIp, IPAddress dstIp, ushort dstPort, ushort srcPort)
    {
        // IPv6 packet: 40 bytes header + 24 bytes TCP = 64 bytes total
        var packet = new byte[64];
        var seqNum = (uint)_random.Next();

        // IPv6 Header (40 bytes)
        packet[0] = 0x60; // Version 6 (0110) + Traffic Class (0000)
        packet[1] = 0x00; // Traffic Class (continued) + Flow Label (0000)
        packet[2] = 0x00; // Flow Label (continued)
        packet[3] = 0x00; // Flow Label (continued)
        packet[4] = 0x00; // Payload Length MSB
        packet[5] = 24;   // Payload Length LSB (20 bytes TCP) //TODO
        packet[6] = 0x06; // Next Header = TCP
        packet[7] = 0x40; // Hop Limit = 64

        srcIp.GetAddressBytes().CopyTo(packet, 8);  // Source Address (16 bytes)
        dstIp.GetAddressBytes().CopyTo(packet, 24); // Destination Address (16 bytes)

        // TCP Header (20 bytes, starts at offset 40)
        packet[40] = (byte)(srcPort >> 8); // Source Port
        packet[41] = (byte)(srcPort & 0xFF);
        packet[42] = (byte)(dstPort >> 8); // Destination Port
        packet[43] = (byte)(dstPort & 0xFF);

        // Sequence Number (random)
        //  BitConverter.GetBytes(seqNum).CopyTo(packet, 44);

        packet[52] = 0x60; // Data Offset (5) << 4
        packet[53] = 0x02; // Flags: SYN
        packet[54] = 0x00; // Window Size
        packet[55] = 0x04;

        // TCP Checksum (0 for calculation)
        packet[56] = 0x00;
        packet[57] = 0x00;
        
        packet[60] = 0x02; // Maximum Segment size
        packet[61] = 0x04; // Length
        packet[62] = 0x05; // MSS
        packet[63] = 0xb4;

        // Compute TCP Checksum with IPv6 Pseudo Header
        var tcpChecksum = ComputeTcpChecksum(packet, 40, 24, srcIp, dstIp);
        packet[56] = (byte)(tcpChecksum >> 8);
        packet[57] = (byte)(tcpChecksum & 0xFF);

        return packet;
    }

    private ushort ComputeChecksum(byte[] buffer, int offset, int length)
    {
        uint sum = 0;
        for (int i = offset; i < offset + length; i += 2)
        {
            ushort word = (ushort)((buffer[i] << 8) + (i + 1 < offset + length ? buffer[i + 1] : 0));
            sum += word;
        }
        while ((sum >> 16) != 0)
            sum = (sum & 0xFFFF) + (sum >> 16);
        return (ushort)~sum;
    }

    private ushort ComputeTcpChecksum(byte[] tcpSegment, int tcpOffset, int tcpLength, IPAddress srcIp, IPAddress dstIp)
    {
        byte[] pseudoHeader;

        if (srcIp.AddressFamily == AddressFamily.InterNetwork)
        {
            // IPv4 pseudo header 
            pseudoHeader = new byte[12];
            srcIp.GetAddressBytes().CopyTo(pseudoHeader, 0);
            dstIp.GetAddressBytes().CopyTo(pseudoHeader, 4);
            pseudoHeader[9] = 0x06; // Protocol = TCP
            pseudoHeader[10] = (byte)(tcpLength >> 8);
            pseudoHeader[11] = (byte)(tcpLength & 0xFF);
        }
        else
        {
            // IPv6 pseudo header 
            pseudoHeader = new byte[40];
            srcIp.GetAddressBytes().CopyTo(pseudoHeader, 0);
            dstIp.GetAddressBytes().CopyTo(pseudoHeader, 16);
            pseudoHeader[32] = (byte)(tcpLength >> 24);
            pseudoHeader[33] = (byte)(tcpLength >> 16);
            pseudoHeader[34] = (byte)(tcpLength >> 8);
            pseudoHeader[35] = (byte)(tcpLength & 0xFF);
            pseudoHeader[39] = 0x06; // Next Header = TCP
        }

        // Combine pseudo header + TCP segment + pad if odd length
        int totalLength = pseudoHeader.Length + tcpLength;
        if (totalLength % 2 != 0) totalLength++;
        
        byte[] checksumBuffer = new byte[totalLength];
        Buffer.BlockCopy(pseudoHeader, 0, checksumBuffer, 0, pseudoHeader.Length);
        Buffer.BlockCopy(tcpSegment, tcpOffset, checksumBuffer, pseudoHeader.Length, tcpLength);

        return ComputeChecksum(checksumBuffer, 0, checksumBuffer.Length);
    }
}