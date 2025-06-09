namespace ipk25chat_client.Input;

public static class MessagePrinter
{
    public static void PrintReply(string status, string content)
    {
        if (status == "OK")
            Console.WriteLine($"Action Success: {content}");
        else if (status == "NOK")
            Console.WriteLine($"Action Failure: {content}");
    }

    public static void PrintError(string from, string content)
    {
        Console.WriteLine($"ERROR FROM {from}: {content}");
    }

    public static void PrintMessage(string from, string content)
    {
        Console.WriteLine($"{from}: {content}");
    }

    public static void PrintLocalError(string content)
    {
        Console.WriteLine($"ERROR: {content}");
    }
}