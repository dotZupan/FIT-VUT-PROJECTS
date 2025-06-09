using ipk25chat_client.Args;
using ipk25chat_client.Enums;
using ipk25chat_client.Input;
using ipk25chat_client.Interfaces;
using ipk25chat_client.TCP;
using ipk25chat_client.UDP;

namespace ipk25chat_client;

internal static class Program
{ 
    public static Semaphore InputSemaphore { get; } = new Semaphore(1, 1);
    private static IChatClient? _client;
    public static IChatClient Client => _client!;
    private static readonly FSM _fsm = new();
    public static FSM Fsm => _fsm;
    public static IPacketBuilder? PacketBuilder { get; private set; }

    public static async Task Main(string[] args)
    {
        Console.CancelKeyPress += async (_, e) =>
        {
            e.Cancel = true;
            // Console.Error.WriteLine("\n[CTRL+C] Graceful shutdown requested...");

            // Enqueue BYE packet in right Client and wait, if no Client is active just leave
            if (TryGetClient(out var client))
            {
                if (client is UdpChatClient udp)
                {
                    await udp.SendByeAndWaitForConfirm(ProcessInput.CurrentDisplay);
                }
                else
                {
                    client.EnqueuePacket(PacketBuilder!.buildBye(ProcessInput.CurrentDisplay));
                    await client.WaitForSendQueueEmptyAsync();
                }

                client.GracefulShutdown(0);
            }
            else
            {
                Environment.Exit(0);
            }
        };

        // Parse CLI arguments and set up client
        ArgPars.ParseArgs(args);
        var config = ArgPars.Instance;

        if (config.ProtocolType == Protocol.tcp)
        {
            _client = new TcpChatClient();
            PacketBuilder = new TcpPacketBuilder();
        }
        else if (config.ProtocolType == Protocol.udp)
        {
            _client = new UdpChatClient();
            PacketBuilder = new UdpPacketBuilder();
        }
        else 
        {
            Environment.Exit(1);
        }
        
        
        /* based on given parameters connect to server and start:
         *       - Receiving messages fom server
         *       - Sending loop
         *       - User command reader
         */
        _client.Connect(config.ServerIp!, config.ServerPort);
        
        var receiveTask = _client.ReceiveAsync(); 
        var sendTask = _client.SendAsync();       
        var inputTask = ProcessInput.InputReader(); 
        
        await Task.WhenAny(receiveTask, sendTask, inputTask);
    }

    
    // Returns true if specific Client is being used
    public static bool TryGetClient(out IChatClient client)
    {
        if (_client == null)
        {
            client = null!;
            return false;
        }

        client = _client;
        return true;
    }
    
    
    
}