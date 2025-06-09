using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

// author - Matus Fignar
class Program
{
    static async Task Main(string[] args)
    {
        var cts = new CancellationTokenSource();

        // Handle Ctrl+C (SIGINT) event
        Console.CancelKeyPress += (s, e) =>
        {
            e.Cancel = true; // Prevent the application from terminating immediately
            cts.Cancel();
            Console.Error.WriteLine("\nScan cancelled. Exiting gracefully...");

            // Force exit if the program doesn't exit in 3 seconds
            Task.Run(async () =>
            {
                await Task.Delay(3000);
                Console.Error.WriteLine("Forcefully exiting.");
                Environment.Exit(1);
            });
        };

        try
        {
            var parser = new ArgumentParser(args);

            if (parser.ShowHelp)
            {
                parser.DisplayHelp();
                return;
            }

            if (parser.ListInterfaces || args.Length == 0)
            {
                NetworkInterfaceManager.ListActiveInterfaces();
                return;
            }

            if (!parser.IsValid || string.IsNullOrEmpty(parser.Target))
            {
                Console.Error.WriteLine("Error: Invalid arguments");
                parser.DisplayHelp();
                Environment.Exit(1);
            }

            // Resolve hostname to get both IPv4 and IPv6 addresses
            var hostEntry = await Dns.GetHostEntryAsync(parser.Target);
            
            if (hostEntry.AddressList.Length == 0)
            {
                Console.Error.WriteLine("Error: No IP addresses found for the target");
                Environment.Exit(1);
            }

            var scanner = new PortScanner(parser.Interface ?? throw new InvalidOperationException("Interface must be specified"), 
                parser.Timeout);
            
            // Launch scanning tasks for all resolved addresses
            var scanTasks = hostEntry.AddressList
                .Select(address => scanner.ScanAsync(
                    address, 
                    parser.TcpPorts ?? new List<int>(), 
                    parser.UdpPorts ?? new List<int>(), 
                    cts.Token))
                .ToList();

            // Wait for all scan tasks to complete
            try
            {
                await Task.WhenAll(scanTasks);
            }
            catch (OperationCanceledException)
            {
                Console.Error.WriteLine("Scanning was cancelled by the user.");
            }
        }
        catch (SocketException ex) when (ex.SocketErrorCode == SocketError.AccessDenied)
        {
            Console.Error.WriteLine("Error: Need root/administrator privileges for raw sockets");
            Environment.Exit(1);
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Fatal error: {ex.Message}");
            Environment.Exit(1);
        }
    }
}