using System;
using System.ComponentModel.Design.Serialization;

using ipk25chat_client.Enums;
using ipk25chat_client.Input;

namespace ipk25chat_client.Args;

public record ArgPars
{
    // when not set = true, when set = false
    private static bool prot = true;
    private static bool serv = true;
    private static bool retr = true;
    private static bool por  = true;
    private static bool time = true;
    
    private static AppArgs _instance = new ();
    public static AppArgs Instance => _instance;
    
    private const string help = """
                                Usage:
                                  -t <tcp|udp>        Protocol type (required)
                                  -s <hostname>       Server IP address or hostname (required)
                                  -p <port>           Server port (optional, default: 4567)
                                  -d <timeout>        UDP confirmation timeout in milliseconds (optional, default: 250)
                                  -r <retries>        Number of UDP retransmissions (optional, default: 3)
                                  -h                  Display this help message and exit
                                """;
    
    
    public static void ParseArgs(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine(help);
            Environment.Exit(0);
        }
        for (int i = 0; i < args.Length; i++)
        {
            try
            {
                switch (args[i])
                {
                    case "-h":
                        Console.WriteLine(help);
                        Environment.Exit(0);
                        break;

                    case "-t":
                        Protocol? protocol = new Protocol();
                        if (i + 1 < args.Length && prot)
                        {
                            if (args[i + 1].ToLower() == "tcp")
                            {
                                protocol = Protocol.tcp;
                            }
                            else if (args[i + 1].ToLower() == "udp")
                            {
                                protocol = Protocol.udp;
                            }
                            else
                            {
                                protocol = null;
                            }
                            prot = false;
                            i++;
                        }
                        else
                        {
                            throw new ArgumentException("Unknown protocol");
                        }

                        _instance = _instance with { ProtocolType = protocol };

                        break;
                    case "-s":
                        string? host = null;
                        if (i + 1 < args.Length && serv)
                        {
                            host = args[i + 1];
                            serv = false;
                            i++;
                        }
                        else
                        {
                            throw new ArgumentException("Unknown host");
                        }

                        _instance = _instance with { ServerIp = host };
                        break;

                    case "-p":

                        if (i + 1 < args.Length && UInt16.TryParse(args[i + 1], out UInt16 port) && por)
                        {
                            _instance = _instance with { ServerPort = port };
                            por = false;
                            i++;
                        }
                        else
                        {
                            throw new ArgumentException("Unknown port");
                        }


                        break;

                    case "-d":

                        if (i + 1 < args.Length && UInt16.TryParse(args[i + 1], out UInt16 timeout) && time)
                        {
                            _instance = _instance with { UdpTimeout = timeout };
                            time = false;
                            i++;
                        }
                        else
                        {
                            throw new ArgumentException("Unknown timeout");
                        }

                        break;

                    case "-r":
                        if (i + 1 < args.Length && byte.TryParse(args[i + 1], out byte uRetrans) && retr)
                        {
                            _instance = _instance with { UdpRetransmission = uRetrans };
                            retr = false;
                            i++;
                        }
                        else
                        {
                            throw new ArgumentException("Unknown retransmission");
                        }

                        break;
                    
                    default:
                        throw new ArgumentException("Unknown argument");
                    
                }
            }
            catch(ArgumentException ex)
            {
                MessagePrinter.PrintLocalError($"Wrong argument: {ex.Message}");
                Environment.Exit(1);
            }
        }

        if (_instance.ProtocolType == null || _instance.ServerIp == null)
        {
            MessagePrinter.PrintLocalError("Protocol type or IP address not specified");
            Environment.Exit(1);
        }
        
        
        
    }
    
}