using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

public class UdpScanner
{
    private readonly string _interfaceName;
    private readonly int _timeout;

    public UdpScanner(string interfaceName, int timeout)
    {
        _interfaceName = interfaceName;
        _timeout = timeout;
    }

    public async Task ScanAsync(IPAddress address, int port, CancellationToken ct)
    {
        try
        {
            // Get the appropriate local IP address for binding based on address family (IPv4 or IPv6)
            var localIp = address.AddressFamily == AddressFamily.InterNetwork
                ? NetworkInterfaceManager.GetInterfaceIPv4Address(_interfaceName)
                : NetworkInterfaceManager.GetInterfaceIPv6Address(_interfaceName);

            if (localIp == null)
            {
                Console.Error.WriteLine($"No suitable IP address found on interface {_interfaceName}");
                return;
            }

            // Create UDP socket for sending
            using var udpSocket = new Socket(address.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
            udpSocket.Bind(new IPEndPoint(localIp, 0));

            // Create ICMP listener socket for receiving
            using var icmpSocket = new Socket(address.AddressFamily, SocketType.Raw,
                address.AddressFamily == AddressFamily.InterNetwork ? ProtocolType.Icmp : ProtocolType.IcmpV6);
            icmpSocket.Bind(new IPEndPoint(localIp, 0));
            icmpSocket.ReceiveTimeout = _timeout;
            
            
            // Create message to send
            var payload = new byte[1] { 0x00 };
            var endpoint = new IPEndPoint(address, port);
            await udpSocket.SendToAsync(new ArraySegment<byte>(payload), SocketFlags.None, endpoint);

            // Check for response
            var buffer = new byte[1024];
            var receiveTask = Task.Run(() =>
            {
                EndPoint remoteEp = new IPEndPoint(address.AddressFamily == AddressFamily.InterNetwork ? IPAddress.Any : IPAddress.IPv6Any, 0);
                int bytesRead = icmpSocket.ReceiveFrom(buffer, ref remoteEp);
                return (bytesRead, buffer);
            }, ct);

            var completedTask = await Task.WhenAny(receiveTask, Task.Delay(_timeout, ct));

            if (completedTask == receiveTask)
            {
                // check if it's a "Port Unreachable" message
                var (bytesRead, recvBuffer) = await receiveTask;
                if (IsPortUnreachable(recvBuffer, bytesRead, address.AddressFamily))
                {
                    Console.WriteLine($"{address} {port} udp closed");
                }
                else
                {
                    Console.WriteLine($"{address} {port} udp open");
                }
            }
            else
            {
                Console.WriteLine($"{address} {port} udp open");    // No response received within the timeout, assume the port is open 
            }
        }
        catch (SocketException ex)
        {
            Console.Error.WriteLine($"ERROR: {ex.Message}");
        }
        catch (TimeoutException)
        {
            Console.WriteLine($"{address} {port} udp open");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Error scanning {address}:{port}: {ex.Message}");
        }
    }

    private bool IsPortUnreachable(byte[] buffer, int length, AddressFamily family)
    {
        if (family == AddressFamily.InterNetwork && length >= 28)
        {
            // IPv4 ICMP: Type 3 = Destination Unreachable, Code 3 = Port Unreachable
            return buffer[20] == 3 && buffer[21] == 3;
        }
        else if (family == AddressFamily.InterNetworkV6 && length >= 8)
        {
            // IPv6 ICMPv6: Type 1 = Destination Unreachable, Code 4 = Port Unreachable
            return buffer[0] == 1 && buffer[1] == 4;
        }
        return false;
    }
}
