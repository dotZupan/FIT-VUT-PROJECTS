using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Text;

using ipk25chat_client.Interfaces;
using ipk25chat_client.Input;
using ipk25chat_client.Enums;
using ipk25chat_client.Args;

namespace ipk25chat_client.UDP;

public class UdpChatClient : IChatClient
{
    private readonly ConcurrentDictionary<ushort, TaskCompletionSource<bool>> _confirmations = new();

    private UdpClient? _client;
    private IPEndPoint? _serverEndPoint;
    private readonly ConcurrentQueue<UdpPacket> _sendQueue = new();
    private readonly SemaphoreSlim _sendSignal = new(0);
    private readonly HashSet<ushort> _processedIds = new();
    private readonly CancellationTokenSource _cts = new();
    private readonly UdpPacketBuilder _packetBuilder = new();
    
    private TaskCompletionSource<bool> _sendQueueEmptyTcs = new(TaskCreationOptions.RunContinuationsAsynchronously);

    public bool SendQueueEmpty => _sendQueue.IsEmpty;
    
    public void Connect(string host, int port)
    {
        _client = new UdpClient();
        _client.Client.Bind(new IPEndPoint(IPAddress.Any, 0));

        if (IPAddress.TryParse(host, out var ip))
        {
            _serverEndPoint = new IPEndPoint(ip, port);
        }
        else
        {
            try
            {
                IPAddress[] addresses = Dns.GetHostAddresses(host);
                _serverEndPoint = addresses.FirstOrDefault(ip => ip.AddressFamily == AddressFamily.InterNetwork)
                    is { } ip4
                    ? new IPEndPoint(ip4, port)
                    : throw new Exception("No IPv4 address found.");
            }
            catch (Exception e)
            {
                MessagePrinter.PrintLocalError($"Failed to resolve hostname '{host}': {e.Message}");
                Environment.Exit(1);
            }
        }
    }

    // Add packets into the send queue 
    public void EnqueuePacket(IChatPacket packet)
    {
        if (packet is not UdpPacket udpPacket)
        {
            MessagePrinter.PrintLocalError("Invalid packet type for UDP client.");
            return;
        }

        _sendQueue.Enqueue(udpPacket);

        if (_sendQueueEmptyTcs.Task.IsCompleted)
        {
            _sendQueueEmptyTcs = new(TaskCreationOptions.RunContinuationsAsynchronously);
        }

        _sendSignal.Release();
    }
    
    
    public Task WaitForSendQueueEmptyAsync() => _sendQueueEmptyTcs.Task;

    public async Task SendAsync()
    {
        while (!_cts.IsCancellationRequested)
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

                ushort msgId = packet.MessageId;
                var type = packet.Type();
                var tcs = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
                _confirmations[msgId] = tcs;

                // Console.Error.WriteLine($"[SEND] Sending packet: {type} #{msgId}");
                var data = packet.ToBytes();
                
                
                if (type == MessageType.JOIN || type == MessageType.AUTH)
                {
                    Program.Fsm.ChageStateSend(type);
                }

                for (int attempt = 0; attempt <= ArgPars.Instance.UdpRetransmission; attempt++)
                {
                    // Console.Error.WriteLine($"[SEND] Attempt {attempt} for #{msgId}");
                    await _client!.SendAsync(data, data.Length, _serverEndPoint);

                    var completed = await Task.WhenAny(tcs.Task, Task.Delay(ArgPars.Instance.UdpTimeout));
                    if (completed == tcs.Task && tcs.Task.Result)
                    {
                        // Console.Error.WriteLine($"[SEND] Confirmed #{msgId}");
                        break;
                    }

                    // Console.Error.WriteLine($"[SEND] Timeout on attempt {attempt} for #{msgId}");
                }

                _confirmations.TryRemove(msgId, out _);

                if (!tcs.Task.IsCompleted || !tcs.Task.Result)
                {
                    MessagePrinter.PrintLocalError($"Message #{msgId} not confirmed");
                    Program.Fsm.ChageStateSend(MessageType.ERR);
                    GracefulShutdown(1);
                }
            }
            catch (Exception)
            {
                // Console.Error.WriteLine($"[SEND] Exception: {e.Message}");
                GracefulShutdown(1);
            }
        }
    }

    public async Task ReceiveAsync()
    {
        while (!_cts.IsCancellationRequested)
        {
            try
            {
                var result = await _client!.ReceiveAsync(_cts.Token);
                var data = result.Buffer;
                // Console.Error.WriteLine($"[RECV] Raw UDP packet: {BitConverter.ToString(data)}");

                var packet = UdpPacketParser.Parse(data);
                var msgId = packet.MessageId;
                var type = packet.Type();

                // Accept Confirm and notify sendAsync
                if (type == MessageType.CONFIRM)
                {
                    // Console.Error.WriteLine($"[RECV] Received CONFIRM for #{msgId}");
                    if (_confirmations.TryRemove(msgId, out var tcs))
                    {
                        tcs.TrySetResult(true);
                    }
                    continue;
                }

                // BYE packet is handled separatly
                if (type != MessageType.BYE)
                {
                    var confirm = _packetBuilder.buildConfirm(msgId);
                    await _client.SendAsync(confirm.ToBytes(), confirm.ToBytes().Length, result.RemoteEndPoint);
                }

                if (_processedIds.Contains(msgId))
                {
                    continue;
                }
                
                // after first message beside of CONFIRM set Endpoint
                if (_serverEndPoint == null || !_serverEndPoint.Equals(result.RemoteEndPoint))
                {
                    // Console.Error.WriteLine($"[RECV] Learned dynamic server endpoint: {result.RemoteEndPoint}");
                    _serverEndPoint = result.RemoteEndPoint;
                }

                _processedIds.Add(msgId);

                // When not supported packet is recieved send ERR packet to server and disconnect
                if (type == MessageType.UNKNOWN)
                {
                    var err = _packetBuilder.buildErr(ProcessInput.CurrentDisplay, "Unknown message");
                    _sendQueue.Enqueue((UdpPacket)err);
                    _sendSignal.Release();

                    MessagePrinter.PrintLocalError("Unknown message");
                    Program.Fsm.changeState("ERR");
                    GracefulShutdown(1);
                    continue;
                }

                switch (type)
                {
                    case MessageType.PING:
                        break;

                    case MessageType.REPLY:
                        // Console.Error.WriteLine($"[RECV] Received REPLY #{msgId}");
                        var prefix = data[3] != 0 ? "REPLY OK" : "REPLY NOK";
                        if (!ValidateAndHandleFsm(prefix, "Unexpected REPLY in current state."))
                            return;

                        string replyText = Encoding.ASCII.GetString(data[6..^1]);
                        MessagePrinter.PrintReply(data[3] != 0 ? "OK" : "NOK", replyText);
                        break;

                    case MessageType.MSG:
                        if (!ValidateAndHandleFsm("MSG", "Unexpected MSG in current state."))
                            return;

                        string[] parts = Encoding.ASCII.GetString(data[3..^1]).Split('\0');
                        MessagePrinter.PrintMessage(parts[0], parts[1]);
                        break;

                    case MessageType.ERR:
                        Program.Fsm.changeState("ERR");
                        string[] eparts = Encoding.ASCII.GetString(data[3..^1]).Split('\0');
                        MessagePrinter.PrintError(eparts[0], eparts[1]);
                        GracefulShutdown(1);
                        break;

                    case MessageType.BYE:
                        // send confirm and wait one retransmission period to see if server recieved confirm
                        var confirm = _packetBuilder.buildConfirm(msgId);
                        await _client.SendAsync(confirm.ToBytes(), confirm.ToBytes().Length, result.RemoteEndPoint);

                        int byeRetries = 0;
                        int maxRetries = ArgPars.Instance.UdpRetransmission;
                        int timeout = ArgPars.Instance.UdpTimeout;

                        // retransmit as many times as program can retransmit by deafault
                        while (byeRetries < maxRetries)
                        {
                            try
                            {
                                using var timeoutCts = new CancellationTokenSource(timeout);
                                var retryResult = await _client.ReceiveAsync(timeoutCts.Token);
                                var retryData = retryResult.Buffer;
                                var retryPacket = UdpPacketParser.Parse(retryData);

                                if (retryPacket.Type() == MessageType.BYE && retryPacket.MessageId == msgId)
                                {
                                    byeRetries++;
                                    var retryConfirm = _packetBuilder.buildConfirm(msgId);
                                    await _client.SendAsync(retryConfirm.ToBytes(), retryConfirm.ToBytes().Length, retryResult.RemoteEndPoint);
                                }
                            }
                            catch (OperationCanceledException)
                            {
                                break;
                            }
                        }

                        GracefulShutdown(0);
                        return;
                }
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception e)
            {
                MessagePrinter.PrintLocalError("Receive failed: " + e.Message);
                GracefulShutdown(1);
                break;
            }
        }
    }

    // When recieved message from server was not expected send error to server and disconnect
    private bool ValidateAndHandleFsm(string prefix, string reason)
    {
        Program.Fsm.changeState(prefix);
        if (Program.Fsm.State == FSMEnum.end)
        {
            MessagePrinter.PrintLocalError(reason);
            var err = _packetBuilder.buildErr(ProcessInput.CurrentDisplay, reason);
            _sendQueue.Enqueue((UdpPacket)err);
            _sendSignal.Release();
            GracefulShutdown(1);
            return false;
        }

        return true;
    }

    
    // When disconnect by user is raised create bye packet
    // sent it to the server wait for replay and possible retransmissions 
    public async Task SendByeAndWaitForConfirm(string displayName)
    {
        var packet = (UdpPacket)new UdpPacketBuilder().buildBye(displayName);
        var msgId = packet.MessageId;

        var tcs = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
        _confirmations[msgId] = tcs;

        // Console.Error.WriteLine($"[SHUTDOWN] Sending BYE #{msgId}");
        await _client!.SendAsync(packet.ToBytes(), packet.ToBytes().Length, _serverEndPoint);

        for (int attempt = 1; attempt <= ArgPars.Instance.UdpRetransmission; attempt++)
        {
            var completed = await Task.WhenAny(tcs.Task, Task.Delay(ArgPars.Instance.UdpTimeout));
            if (completed == tcs.Task && tcs.Task.Result)
            {
                // Console.Error.WriteLine($"[SHUTDOWN] Confirmed BYE #{msgId}");
                _confirmations.TryRemove(msgId, out _);
                Program.Fsm.ChageStateSend(MessageType.BYE);
                return;
            }

            // Console.Error.WriteLine($"[SHUTDOWN] Retry {attempt} for BYE #{msgId}");
            await _client.SendAsync(packet.ToBytes(), packet.ToBytes().Length, _serverEndPoint);
        }

        // Console.Error.WriteLine($"[SHUTDOWN] Failed to confirm BYE #{msgId}");
        _confirmations.TryRemove(msgId, out _);
        Program.Fsm.ChageStateSend(MessageType.ERR);
        // confirme was not recieved, invalid connection, so there is no need to send any more packets to server
        GracefulShutdown(1);
    }

    public void GracefulShutdown(int code = 0)
    {
        _cts.Cancel();
        _client!.Close();
        Environment.Exit(code);
    }
}
