using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;

public class ArgumentParser
{
    public bool IsValid { get; private set; } = true;
    public bool ShowHelp { get; private set; } = false;
    public string? Interface { get; private set; } = null;
    public string? Target { get; private set; } = null;
    public int Timeout { get; private set; } = 5000;
    public List<int>? TcpPorts { get; private set; } = null;
    public List<int>? UdpPorts { get; private set; } = null;
    public bool ListInterfaces { get; private set; } = false;   // Flag to list available interfaces

    public ArgumentParser(string[] args)
    {
        TcpPorts = new List<int>();
        UdpPorts = new List<int>();
        try
        {
            for (int i = 0; i < args.Length; i++)
            {
                switch (args[i])
                {
                    case "-h":
                    case "--help":
                        ShowHelp = true;
                        return;

                    case "-i":
                    case "--interface":
                        if (i + 1 < args.Length && !args[i + 1].StartsWith("-"))
                            Interface = args[++i];  // Assign network interface name
                        else
                            ListInterfaces = true;
                        break;

                    case "-t":
                    case "--pt":
                        if (i + 1 < args.Length)
                            TcpPorts = ParsePorts(args[++i]);
                        else
                            IsValid = false;
                        break;

                    case "-u":
                    case "--pu":
                        if (i + 1 < args.Length)
                            UdpPorts = ParsePorts(args[++i]);
                        else
                            IsValid = false;
                        break;

                    case "-w":
                    case "--wait":
                        if (i + 1 < args.Length && int.TryParse(args[++i], out int timeout))
                            Timeout = timeout;  // Assign custom timeout value
                        else
                            IsValid = false;
                        break;

                    default:
                        if (Target == null && !args[i].StartsWith("-"))
                            Target = args[i];   // Assign target address/hostname
                        else
                            IsValid = false;
                        break;
                }
            }

            if (!string.IsNullOrEmpty(Target) && TcpPorts == null && UdpPorts == null)
                IsValid = false;
        }
        catch
        {
            IsValid = false;
        }
    }

    public void DisplayHelp()
    {
        Console.WriteLine("Usage: ipk-l4-scan [options] [target]");
        Console.WriteLine("Options:");
        Console.WriteLine("  -i, --interface <iface>  Network interface (use -i alone to list)");
        Console.WriteLine("  -t, --pt <ports>        TCP ports (e.g., 80 or 1-100 or 22,80,443)");
        Console.WriteLine("  -u, --pu <ports>        UDP ports");
        Console.WriteLine("  -w, --wait <ms>         Timeout in milliseconds (default: 5000)");
        Console.WriteLine("  -h, --help              Show this help");
        
    }

    private List<int>? ParsePorts(string input)
    {
        var ports = new List<int>();
        foreach (var part in input.Split(','))
        {
            if (part.Contains('-'))
            {
                var range = part.Split('-');
                if (range.Length == 2 && int.TryParse(range[0], out int start) && int.TryParse(range[1], out int end))
                {
                    if (start > 0 && end <= 65535 && start <= end)
                        ports.AddRange(Enumerable.Range(start, end - start + 1));
                    else
                        IsValid = false;
                }
                else
                {
                    IsValid = false;
                }
            }
            else if (int.TryParse(part, out int port))
            {
                if (port > 0 && port <= 65535)
                    ports.Add(port);
                else
                    IsValid = false;
            }
            else
            {
                IsValid = false;
            }
        }
        return ports.Distinct().OrderBy(p => p).ToList();   // Return sorted list of unique ports
    }
    
}