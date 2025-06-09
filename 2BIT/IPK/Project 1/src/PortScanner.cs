using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

public class PortScanner
{
    private readonly string _interfaceName;
    private readonly int _timeout;

    public PortScanner(string interfaceName, int timeout)
    {
        _interfaceName = interfaceName;
        _timeout = timeout;
    }

    public async Task ScanAsync(IPAddress address, List<int>? tcpPorts, List<int>? udpPorts, CancellationToken ct)
    {
        // Handle IPv6 link-local addresses by adding scope ID if missing
        if (address.AddressFamily == AddressFamily.InterNetworkV6 && 
            address.IsIPv6LinkLocal && 
            !address.ToString().Contains('%'))
        {
            address = IPAddress.Parse($"{address}%{_interfaceName}");
        }

        var tasks = new List<Task>();

        // Launch TCP scanning tasks if any TCP ports are specified
        if (tcpPorts?.Count > 0)
        {
            var tcpScanner = new TcpScanner(_interfaceName, _timeout);
            foreach (var port in tcpPorts)
            {
                tasks.Add(tcpScanner.ScanAsync(address, port, ct));
            }
        }

        // Launch UDP scanning tasks if any UDP ports are specified
        if (udpPorts?.Count > 0)
        {
            var udpScanner = new UdpScanner(_interfaceName, _timeout);
            foreach (var port in udpPorts)
            {
                tasks.Add(udpScanner.ScanAsync(address, port, ct));
            }
        }

        // Await all scanning tasks to finish
        await Task.WhenAll(tasks);
    }
}