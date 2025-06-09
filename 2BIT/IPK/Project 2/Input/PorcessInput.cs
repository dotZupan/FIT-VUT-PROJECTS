using ipk25chat_client.Interfaces;
using ipk25chat_client.Enums;

namespace ipk25chat_client.Input;

public static class ProcessInput
{
    private static string _currentDisplayName = "Unknown";
    public static string CurrentDisplay => _currentDisplayName;

    private static IChatPacket? ProcessInputByUser(string input)
    {
        // when no message was given skip
        if (string.IsNullOrWhiteSpace(input))
        {
            // Console.Error.WriteLine("[INPUT] Empty input received.");
            return null;
        }

        // Console.Error.WriteLine($"[INPUT] Received: {input} in state {Program.Fsm.State}");

        var split = input.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        var builder = Program.PacketBuilder;

        if (split.Length == 0) return null;

        
        switch (split[0])
        {
            case "/auth":
                if (split.Length != 4)
                {
                    MessagePrinter.PrintLocalError("Usage: /auth <username> <secret> <displayName>");
                    return null;
                }
                
                if (!Program.Fsm.IsCmdValid(MessageType.AUTH))
                {
                    MessagePrinter.PrintLocalError("Cannot authenticate: Already authenticated.");
                    return null;
                }

                var username = InputValidator.TruncateUsername(split[1]);
                var secret = InputValidator.TruncateSecret(split[2]);
                var displayName = InputValidator.TruncateDisplayName(split[3]);
                
                if (username.Length < split[1].Length)
                    MessagePrinter.PrintLocalError("Username truncated to 20 characters.");
                if (secret.Length < split[2].Length)
                    MessagePrinter.PrintLocalError("Secret truncated to 128 characters.");
                if (displayName.Length < split[3].Length)
                    MessagePrinter.PrintLocalError("Display name truncated to 20 characters.");


                if (!InputValidator.ValidateUsername(username))
                {
                    // Console.Error.WriteLine("[VALIDATION] Invalid username");
                    return null;
                }

                if (!InputValidator.ValidateSecret(secret))
                {
                    // Console.Error.WriteLine("[VALIDATION] Invalid secret");
                    return null;
                }

                if (!InputValidator.ValidateDisplayName(displayName))
                {
                    // Console.Error.WriteLine("[VALIDATION] Invalid displayName");
                    return null;
                }

                _currentDisplayName = displayName;
                // Console.Error.WriteLine($"[BUILD] Building AUTH packet for {username} as {_currentDisplayName}");
                return builder!.buildAuth(username, secret, displayName);

            case "/join":

                if (split.Length != 2)
                {
                    MessagePrinter.PrintLocalError("Usage: /join <channel>");
                    return null;
                }

                if (!Program.Fsm.IsCmdValid(MessageType.JOIN))
                {
                    MessagePrinter.PrintLocalError("JOIN not allowed in current state.");
                    return null;
                }
                
                var channel = InputValidator.TruncateChannelId(split[1]);
                if (channel.Length < split[1].Length)
                    MessagePrinter.PrintLocalError("Channel ID truncated to 20 characters.");
                
                if (!InputValidator.ValidateChannelId(channel))
                {
                    MessagePrinter.PrintLocalError("Invalid channel ID.");
                    return null;
                }

                // Console.Error.WriteLine($"[BUILD] Building JOIN packet for channel {channel} as {_currentDisplayName}");
                return builder!.buildJoin(channel, _currentDisplayName);

            case "/rename":
                // Console.Error.WriteLine("[INPUT] Processing /rename command");

                if (split.Length != 2)
                {
                    MessagePrinter.PrintLocalError("Usage: /rename <newDisplayName>");
                    return null;
                }

                var newDisplayName = InputValidator.TruncateDisplayName(split[1]);
                if (newDisplayName.Length < split[1].Length)
                    MessagePrinter.PrintLocalError("New username truncated to 20 characters.");
                
                if (!InputValidator.ValidateDisplayName(newDisplayName))
                {
                    MessagePrinter.PrintLocalError("Invalid display name.");
                    return null;
                }

                _currentDisplayName = newDisplayName;
                // Console.Error.WriteLine($"[RENAME] Display name changed to {_currentDisplayName}");
                return null;

            case "/help":
                ShowHelp();
                return null;

            default:
                // in case of invalid message skip
                if (split[0].StartsWith("/"))
                {
                    MessagePrinter.PrintLocalError("Unknown command.");
                    return null;
                }

                // Console.Error.WriteLine($"[INPUT] Treating as message input (FSM state: {Program.Fsm.State})");

                if (!Program.Fsm.IsCmdValid(MessageType.MSG))
                {
                    MessagePrinter.PrintLocalError("Cannot send message in current state.");
                    return null;
                }
                
                var cleanMsg = InputValidator.TruncateMessage(input);
                if (cleanMsg.Length < input.Length)
                    MessagePrinter.PrintLocalError("Message content truncated to 60000 characters.");

                
                if (!InputValidator.ValidateMessage(cleanMsg))
                {
                    MessagePrinter.PrintLocalError("Invalid message: max 60000 printable characters.");
                    return null;
                }

                // Console.Error.WriteLine($"[BUILD] Building MSG packet from {_currentDisplayName}: {input}");
                return builder!.buildMsg(_currentDisplayName, cleanMsg);
        }
    }

    public static async Task InputReader()
    {
        while (true)
        {
            var input = Console.ReadLine();
            // Console.Error.WriteLine("[STDIN] Console.ReadLine() returned: " + input);

            Program.InputSemaphore.WaitOne();

            if (input == null)
            {
                // Console.Error.WriteLine("[INPUT] Input stream closed.");

                if (Program.TryGetClient(out var client))
                {
                    var byePacket = Program.PacketBuilder!.buildBye(CurrentDisplay);
                    client.EnqueuePacket(byePacket);
                    Program.Fsm.ChageStateSend(MessageType.BYE);

                    await client.WaitForSendQueueEmptyAsync();
                }

                break;
            }

            var packet = ProcessInputByUser(input);
            if (packet != null)
            {
                // Console.Error.WriteLine($"[ENQUEUE] Enqueueing packet of type {packet.Type()}");
                if (Program.TryGetClient(out var clientToSend))
                {
                    clientToSend.EnqueuePacket(packet);
                }
            }
            else
            {
                // Console.Error.WriteLine("[DEBUG] Packet is NULL – nebude odoslaný");
            }

            Program.InputSemaphore.Release();

            if (Program.Fsm.State == FSMEnum.end)
            {
                // Console.Error.WriteLine("[FSM] State is END, exiting input loop.");
                break;
            }
        }
    }

    private static void ShowHelp()
    {
        Console.WriteLine("Available commands:");
        Console.WriteLine("  /auth <username> <secret> <displayName>");
        Console.WriteLine("  /join <channel>");
        Console.WriteLine("  /rename <newDisplayName>");
        Console.WriteLine("  /help");
        Console.WriteLine("  <message> (just type it to send)");
    }
}
