using System.Text.RegularExpressions;

namespace ipk25chat_client.Input;


public static class InputValidator
{
    private static readonly Regex IdRegex = new(@"^[a-zA-Z0-9_.-]{1,20}$");
    private static readonly Regex SecretRegex = new(@"^[a-zA-Z0-9_-]{1,128}$");
    private static readonly Regex DisplayNameRegex = new(@"^[\x21-\x7E]{1,20}$"); // printable ASCII
    private static readonly Regex MessageRegex = new(@"^[\x20-\x7E\x0A]{1,60000}$"); // space, printable, newline

    public static bool ValidateUsername(string username)
        => IdRegex.IsMatch(username);

    public static bool ValidateChannelId(string channel)
        => IdRegex.IsMatch(channel);

    public static bool ValidateSecret(string secret)
        => SecretRegex.IsMatch(secret);

    public static bool ValidateDisplayName(string name)
        => DisplayNameRegex.IsMatch(name);

    public static bool ValidateMessage(string msg)
        => MessageRegex.IsMatch(msg);
    
    
    
    
    
    // if input was longer than allowed, cut it
    public static string TruncateDisplayName(string input)
    {
        if (input.Length > 20)
        {
            MessagePrinter.PrintLocalError("Too long display name - Truncated");
        }
        return new string(input
            .Where(c => c >= 0x21 && c <= 0x7E) // printable ASCII
            .Take(20) 
            .ToArray());
    }

    public static string TruncateUsername(string input)
    {
        return new string(input
            .Where(c => char.IsLetterOrDigit(c) || c == '-' || c == '_' || c == '.')
            .Take(20)
            .ToArray());
    }

    public static string TruncateSecret(string input)
    {
        return new string(input
            .Where(c => char.IsLetterOrDigit(c) || c == '-' || c == '_')
            .Take(128)
            .ToArray());
    }

    public static string TruncateChannelId(string input)
    {
        return TruncateUsername(input); // same as Username
    }

    public static string TruncateMessage(string input)
    {
        return new string(input
            .Where(c => (c >= 0x20 && c <= 0x7E) || c == '\n')
            .Take(60000)
            .ToArray());
    }

    
}

