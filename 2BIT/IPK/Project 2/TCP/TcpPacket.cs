using System.Text;

using ipk25chat_client.Interfaces;
using ipk25chat_client.Enums;

namespace ipk25chat_client.TCP;

public class TcpPacket : IChatPacket
{
    private MessageType type;
    private string? data;

    public TcpPacket(MessageType type, string? data = null)
    {
        this.type = type;
        this.data = data;
    }

    public MessageType Type() => type;
    public string? Data() => data;
    
    public byte[] ToBytes()
    {
        if (data != null) return Encoding.ASCII.GetBytes(data);
        
        return Array.Empty<byte>();
    }
}


public class TcpPacketBuilder : IPacketBuilder
{
    private const string endTag = "\r\n";

    public IChatPacket buildAuth(string username, string secret, string displayName)
    {
        var type = MessageType.AUTH;
        var data = $"AUTH {username} AS {displayName} USING {secret}{endTag}";
        return new TcpPacket(type, data);
    }

    public IChatPacket buildJoin(string channel, string displayName)
    {
        var type = MessageType.JOIN;
        var data = $"JOIN {channel} AS {displayName}{endTag}";
        return new TcpPacket(type, data);
    }

    public IChatPacket buildMsg(string displayName, string msg)
    {
        var type = MessageType.MSG;
        var data = $"MSG FROM {displayName} IS {msg}{endTag}";
        return new TcpPacket(type, data);
        
    }

    public IChatPacket buildBye(string displayName)
    {
        var type = MessageType.BYE;
        var data = $"BYE FROM {displayName}{endTag}";
        return new TcpPacket(type, data);
    }

    public IChatPacket buildErr(string displayName, string err)
    {
        var type = MessageType.ERR;
        var data = $"ERR FROM {displayName} IS {err}{endTag}";
        return new TcpPacket(type, data);
    }
}
