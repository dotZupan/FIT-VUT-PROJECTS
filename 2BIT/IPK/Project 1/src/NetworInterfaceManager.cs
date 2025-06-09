using System;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;

public static class NetworkInterfaceManager
{
    public static void ListActiveInterfaces()
    {
        Console.WriteLine("Available network interfaces:");
        foreach (var ni in NetworkInterface.GetAllNetworkInterfaces().Where(n => n.OperationalStatus == OperationalStatus.Up))
        {
            Console.WriteLine($"\n{ni.Name} - {ni.Description}");
            Console.WriteLine($"  MAC: {ni.GetPhysicalAddress()}");
            Console.WriteLine($"  IP Addresses:");
            
            foreach (var addr in ni.GetIPProperties().UnicastAddresses.OrderBy(a => a.Address.AddressFamily))
            {
                Console.WriteLine($"    {addr.Address} ({addr.Address.AddressFamily})");
            }
        }
    }



    public static IPAddress? GetInterfaceIPv4Address(string interfaceName)
    {
        if (string.IsNullOrEmpty(interfaceName))
        {
            return IPAddress.Any;
        }

        var nic = GetNetworkInterface(interfaceName);
        if (nic == null)
        {
            throw new ArgumentException($"Interface {interfaceName} not found");
        }

        return nic.GetIPProperties().UnicastAddresses
            .FirstOrDefault(addr => addr.Address.AddressFamily == AddressFamily.InterNetwork &&
                                    !IPAddress.IsLoopback(addr.Address))?.Address;
    }

    public static IPAddress? GetInterfaceIPv6Address(string interfaceName)
    {
        if (string.IsNullOrEmpty(interfaceName))
            return IPAddress.IPv6Any;

        var nic = GetNetworkInterface(interfaceName);
        if (nic == null) return null;

        
        return nic.GetIPProperties().UnicastAddresses
            .Where(addr => addr.Address.AddressFamily == AddressFamily.InterNetworkV6)
            .OrderByDescending(addr => 
                !addr.Address.IsIPv6LinkLocal &&  // Prefer global addresses, skip link-local
                !addr.Address.IsIPv6SiteLocal  
                )    
            .Select(addr => addr.Address)
            .FirstOrDefault();
    }
    

    private static NetworkInterface GetNetworkInterface(string interfaceName)
    {
        return NetworkInterface.GetAllNetworkInterfaces()
                   .FirstOrDefault(ni => ni.Name.Equals(interfaceName, StringComparison.OrdinalIgnoreCase)) 
               ?? throw new ArgumentException($"Network interface '{interfaceName}' not found");
    }
    
}