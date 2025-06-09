using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

using ipk25chat_client.Interfaces;
using ipk25chat_client.Input;
using ipk25chat_client.Enums;

namespace ipk25chat_client.TCP;

public class TcpChatClient : IChatClient
{
    private readonly TcpClient _client;
    private NetworkStream? stream;
    private readonly FSM fsm;
    private readonly ConcurrentQueue<IChatPacket> _sendQueue = new();
    private readonly SemaphoreSlim replyWait = new(0, 1);
    private bool expectingReply = false;
    private readonly SemaphoreSlim _sendSignal = new(0);
    private readonly CancellationTokenSource _cts = new();
    public bool SendQueueEmpty => _sendQueue.IsEmpty;
    
    private TaskCompletionSource<bool> _sendQueueEmptyTcs = new(TaskCreationOptions.RunContinuationsAsynchronously);

    public TcpChatClient()
    {
        fsm = Program.Fsm;
        _client = new TcpClient();
    }
    
    
    
    public void Connect(string host, int port)
    {
        IPAddress? IPv4 = null;
        try
        {
            IPAddress[] hostAddresses = Dns.GetHostAddresses(host);
            IPv4 = Array.Find(hostAddresses, ip => ip.AddressFamily == AddressFamily.InterNetwork);

            if (IPv4 == null)
            {
                MessagePrinter.PrintLocalError("No IPv4 address found for the given host.");
                Environment.Exit(1);
            }
        }
        catch (Exception)
        {
            MessagePrinter.PrintLocalError("Failed to resolve hostname.");
            Environment.Exit(1);
        }

        try
        {
            _client.Connect(IPv4, port);
            stream = _client.GetStream();
        }
        catch (Exception)
        {
            MessagePrinter.PrintLocalError("Unable to connect to the server.");
            Environment.Exit(1);
        }
    }


    // add packet  into a queue
    public void EnqueuePacket(IChatPacket packet)
    {
        _sendQueue.Enqueue(packet);

        
        // When empty queue is signalised, refresh TCS
        if (_sendQueueEmptyTcs.Task.IsCompleted)
        {
            _sendQueueEmptyTcs = new(TaskCreationOptions.RunContinuationsAsynchronously);
        }

        _sendSignal.Release();
    }

    public void SendInternalError(string reason)
    {
        if (_client.Connected && stream != null)
        {
            var errPacket = new TcpPacketBuilder().buildErr(ProcessInput.CurrentDisplay, reason);
            EnqueuePacket(errPacket);
        }
    }

    public Task WaitForSendQueueEmptyAsync() => _sendQueueEmptyTcs.Task;
    
    public void GracefulShutdown(int exitCode = 0)
    {
        _cts.Cancel(); 
        stream?.Close();
        _client?.Close();
        Environment.Exit(exitCode);
    }

    public async Task SendAsync()
    {
        if (stream == null)
        {
            MessagePrinter.PrintLocalError("Stream not initialized.");
            return;
        }

        while (!_cts.IsCancellationRequested && (fsm.State != FSMEnum.end || !_sendQueue.IsEmpty))
        {
            try
            {
                await _sendSignal.WaitAsync(_cts.Token);

                if (!_sendQueue.TryDequeue(out var packet))
                    continue;
                
                if (_sendQueue.IsEmpty)
                {
                    _sendQueueEmptyTcs.TrySetResult(true);
                }

                byte[] bytes = packet.ToBytes();
                await stream.WriteAsync(bytes, 0, bytes.Length, _cts.Token);

                fsm.ChageStateSend(packet.Type());

                // in case when REPLY is needed wait for 5 seconds
                if (packet.Type() is MessageType.AUTH or MessageType.JOIN)
                {
                    expectingReply = true;
                    var replyReceived = await replyWait.WaitAsync(TimeSpan.FromSeconds(5), _cts.Token);

                    if (!replyReceived)
                    {
                        MessagePrinter.PrintLocalError("No REPLY received within 5 seconds.");
                        await SendErr("REPLY Timeout", shutdown: true, awaitSend: false);
                    }
                }
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception e)
            {
                Console.WriteLine($"ERROR: {e.Message}");
                await SendErr("Send fail: " + e.Message, shutdown: true, awaitSend: false);
            }
        }
    }

    public async Task ReceiveAsync()
    {
        if (stream == null)
        {
            MessagePrinter.PrintLocalError("StreamReader not initialized.");
            return;
        }

        byte[] buffer = new byte[2048];
        string messageBuffer = "";
        
        while (fsm.State != FSMEnum.end)
        {
            int read;
            try
            {
                read = await stream.ReadAsync(buffer, 0, buffer.Length);
            }
            catch (Exception e)
            {
                MessagePrinter.PrintLocalError($"Receive error: {e.Message}");
                await SendErr("Receive error: " + e.Message, shutdown: true, awaitSend: false);
                break;
            }

            if (read == 0)
            {
                // Console.Error.WriteLine("[Server closed connection]");
                fsm.changeState("BYE");
                GracefulShutdown(0);
                break;
            }

            messageBuffer += Encoding.ASCII.GetString(buffer, 0, read);

            // if not whole message was recieved, don't evaluate but continue
            if (!messageBuffer.Contains("\r\n"))
                continue;
            
            // more than one message can be sent in one packet
            string[] splitMessages = messageBuffer.Split("\r\n", StringSplitOptions.None);
            
            // if packet contains not full message add it to the buffer
            if (!messageBuffer.EndsWith("\r\n"))
            {
                messageBuffer = splitMessages[^1];
                splitMessages = splitMessages[..^1];
            }
            else
            {
                messageBuffer = "";
            }

            foreach (var rawMsg in splitMessages)
            {
                if (string.IsNullOrWhiteSpace(rawMsg))
                    continue;
    
                // get rid of "\r\n"
                string trimmed = rawMsg.Trim();
                await ProcessServerMessage(trimmed);
            }
        }
    }


   // Handles messages from the server and detects any invalid or malformed messages
    private async Task ProcessServerMessage(string serverMsg)
    {
        string[] parts = serverMsg.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        string content;
        string from;
        string? prefix = parts[0].ToUpperInvariant();
        
        switch (prefix)
        {
            case "MSG":
                // required structure is MSG FROM {display_name} IS {content}
                if (parts.Length < 5 || !parts[1].Equals("FROM", StringComparison.OrdinalIgnoreCase) || !parts[3].Equals("IS", StringComparison.OrdinalIgnoreCase))
                {
                    MessagePrinter.PrintLocalError($"Malformed MSG");
                    await SendErr("Malformed MSG", shutdown: true, awaitSend: true);
                    return;
                }
                fsm.changeState(prefix);
                // MSG was not expected in current state
                if (fsm.State == FSMEnum.end)
                {
                    MessagePrinter.PrintLocalError($"Recieved MSG message in wrong state.");
                    await SendErr("Recieved MSG message in wrong state.", shutdown: true, awaitSend: true);
                    return;
                }
                from = parts[2];
                content = string.Join(" ", parts[4..]);
                MessagePrinter.PrintMessage(from, content);
                return;
            
            case "REPLY":
                
                // REPLY {"OK"|"NOK"} IS {MessageContent}
                if (parts.Length < 3 ||
                    (!parts[1].Equals("NOK", StringComparison.OrdinalIgnoreCase) &&
                     !parts[1].Equals("OK", StringComparison.OrdinalIgnoreCase)) ||
                     !parts[2].Equals("IS", StringComparison.OrdinalIgnoreCase))
                {
                    MessagePrinter.PrintLocalError($"Malformed REPLY");
                    await SendErr("Malformed REPLY", shutdown: true, awaitSend: true);
                    return;
                }
                
                string fullPrefix = $"{prefix} {parts[1].ToUpperInvariant()}";
                fsm.changeState(fullPrefix);
                // REPLY was not expected in current state
                if (fsm.State == FSMEnum.end)
                {
                    MessagePrinter.PrintLocalError($"Recieved REPLY message in wrong state.");
                    await SendErr("Recieved REPLY message in wrong state.", shutdown: true, awaitSend: true);
                    return;
                }

                var status = parts[1].ToUpperInvariant();
                content = string.Join(" ", parts[3..]);
                MessagePrinter.PrintReply(status, content);

                if (expectingReply)
                {
                    expectingReply = false;
                    replyWait.Release();
                }
                else
                {
                    // Console.Error.WriteLine("Recieved REPLY in invalid State");
                }
                return;
            
            case "BYE":
                // required format is BYE FROM {DisplayName}
                if (parts.Length < 3 || !parts[1].Equals("FROM", StringComparison.OrdinalIgnoreCase))
                {
                    MessagePrinter.PrintLocalError($"Malformed BYE");
                    await SendErr("Malformed BYE", shutdown: true, awaitSend: true);

                    return;
                }
                
                from = parts[2];
                fsm.changeState("BYE");
                _ = SendByeAndShutdown();
                
                return;
            
            case "ERR":
                // required format is ERR FROM {DisplayName} IS {MessageContent}
                if (parts.Length < 5 ||
                    !parts[1].Equals("FROM", StringComparison.OrdinalIgnoreCase) ||
                    !parts[3].Equals("IS", StringComparison.OrdinalIgnoreCase))
                {
                    MessagePrinter.PrintLocalError($"Malformed ERR");
                    await SendErr("Malformed ERR", shutdown: true, awaitSend: true);
                    return;
                }
                fsm.changeState(prefix);
                from = parts[2];
                content = string.Join(" ", parts[4..]);

                MessagePrinter.PrintError(from, content);

                if (expectingReply)
                {
                    expectingReply = false;
                    replyWait.Release();
                }

                _ = SendByeAndShutdown(); 
                return;
            
            
            default:
                fsm.changeState("ERR");
                MessagePrinter.PrintLocalError($"Unknown Message type");
                await SendErr("Unknown Message", shutdown: true, awaitSend: true);
                return;
                
        }
    }

    
    private async Task SendByeAndShutdown()
    {
        if (Program.TryGetClient(out var client))
        {
            client.EnqueuePacket(new TcpPacketBuilder().buildBye(ProcessInput.CurrentDisplay));
            Program.Fsm.ChageStateSend(MessageType.BYE);

            while (!client.SendQueueEmpty)
            {
                await Task.Delay(20);
            }

            client.GracefulShutdown(1);
        }
    }

    // Incase of malformed (or invalid) messages it sends err packet to the server and gracefuly disconnect
    private async Task SendErr(string reason, bool shutdown = true, bool awaitSend = true)
    {
        if (Program.TryGetClient(out var client))
        {
            client.EnqueuePacket(new TcpPacketBuilder().buildErr(ProcessInput.CurrentDisplay, reason));
            Program.Fsm.ChageStateSend(MessageType.ERR);

            if (awaitSend)
            {
                while (!client.SendQueueEmpty)
                {
                    await Task.Delay(20);
                }
            }

            if (shutdown)
            {
                client.GracefulShutdown(1);
            }
        }
    }

}
