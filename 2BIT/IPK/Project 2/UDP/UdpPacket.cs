using System.Text;

using ipk25chat_client.Enums;
using ipk25chat_client.Interfaces;

namespace ipk25chat_client.UDP;

public class UdpPacket : IChatPacket
{
    private readonly MessageType _type;
    private readonly byte[] _data;
    private readonly ushort _messageId;

    public UdpPacket(MessageType type, byte[] data)
    {
        if (data.Length < 3)
            throw new ArgumentException("UDP packet must be at least 3 bytes long");
        
        _type = type;
        _data = data;
        // _messageId = BitConverter.ToUInt16(data[1..3].Reverse().ToArray()); // Big endian
        _messageId = (ushort)((data[1] << 8) | data[2]);
    }

    public MessageType Type() => _type;

    public byte[] ToBytes() => _data;

    public ushort MessageId => _messageId;

    public override string ToString() =>
        $"{_type} (#{_messageId}): {BitConverter.ToString(_data)}";
}

public class UdpPacketBuilder : IPacketBuilder
{
    private static ushort _counter = 0;

    private static ushort GetNextId()
    {
        // var id = _counter++;
        // return (ushort)((id << 8) | (id >> 8)); // swap bytes for big endian
        return _counter++;
    }

    private static void AppendMessageId(List<byte> packet, ushort id)
    {
        packet.Add((byte)(id >> 8));
        packet.Add((byte)(id & 0xFF));
    }

    private static void AppendString(List<byte> packet, string str)
    {
        packet.AddRange(Encoding.ASCII.GetBytes(str));
        packet.Add(0x00); // null terminator
    }

    public IChatPacket buildAuth(string username, string secret, string displayName)
    {
        var msgId = GetNextId();
        var packet = new List<byte> { (byte)MessageType.AUTH };
        AppendMessageId(packet, msgId);
        AppendString(packet, username);
        AppendString(packet, displayName);
        AppendString(packet, secret);
        return new UdpPacket(MessageType.AUTH, packet.ToArray());
    }

    public IChatPacket buildJoin(string channel, string displayName)
    {
        var msgId = GetNextId();
        var packet = new List<byte> { (byte)MessageType.JOIN };
        AppendMessageId(packet, msgId);
        AppendString(packet, channel);
        AppendString(packet, displayName);
        return new UdpPacket(MessageType.JOIN, packet.ToArray());
    }

    public IChatPacket buildMsg(string displayName, string message)
    {
        var msgId = GetNextId();
        var packet = new List<byte> { (byte)MessageType.MSG };
        AppendMessageId(packet, msgId);
        AppendString(packet, displayName);
        AppendString(packet, message);
        return new UdpPacket(MessageType.MSG, packet.ToArray());
    }

    public IChatPacket buildBye(string displayName)
    {
        var msgId = GetNextId();
        var packet = new List<byte> { (byte)MessageType.BYE };
        AppendMessageId(packet, msgId);
        AppendString(packet, displayName);
        return new UdpPacket(MessageType.BYE, packet.ToArray());
    }

    public IChatPacket buildErr(string displayName, string message)
    {
        var msgId = GetNextId();
        var packet = new List<byte> { (byte)MessageType.ERR };
        AppendMessageId(packet, msgId);
        AppendString(packet, displayName);
        AppendString(packet, message);
        return new UdpPacket(MessageType.ERR, packet.ToArray());
    }

    // Extra pre potvrdenie prijatia správ
    public IChatPacket buildConfirm(ushort refMessageId)
    {
        var packet = new List<byte> { (byte)MessageType.CONFIRM };
        AppendMessageId(packet, refMessageId);
        return new UdpPacket(MessageType.CONFIRM, packet.ToArray());
    }
}


public static class UdpPacketParser
{
    public static UdpPacket Parse(byte[] data)
    {
        if (data.Length < 3)
            return new UdpPacket(MessageType.UNKNOWN, data); // Too short for any valid message

        var type = (MessageType)data[0];
        ushort msgId = (ushort)((data[1] << 8) | data[2]);

        try
        {
            switch (type)
            {
                case MessageType.REPLY:
                    // [1B type][2B id][1B result][2B "IS"][...text][0]
                    if (data.Length < 7 || data[^1] != 0)
                        return new UdpPacket(MessageType.UNKNOWN, data);
                    return new UdpPacket(MessageType.REPLY, data);
                
                case MessageType.MSG:
                case MessageType.ERR:
                    // [1B type][2B id][...string1][0][...string2][0]
                    if (!HasTwoNullTerminatedStrings(data[3..]))
                        return new UdpPacket(MessageType.UNKNOWN, data);
                    return new UdpPacket(type, data);

                case MessageType.CONFIRM:
                    if (data.Length != 3)
                        return new UdpPacket(MessageType.UNKNOWN, data);
                    return new UdpPacket(MessageType.CONFIRM, data);
                
                case MessageType.PING:
                    if (data.Length == 3) // iba header + messageId
                        return new UdpPacket(MessageType.PING, data);
                    return new UdpPacket(MessageType.UNKNOWN, data);

                default:
                    return new UdpPacket(MessageType.UNKNOWN, data);
            }
        }
        catch
        {
            return new UdpPacket(MessageType.UNKNOWN, data);
        }
    }

    private static bool HasTwoNullTerminatedStrings(byte[] segment)
    {
        int nulls = 0;
        foreach (var b in segment)
        {
            if (b == 0 && ++nulls == 2)
                return true;
        }
        return false;
    }
    
}
