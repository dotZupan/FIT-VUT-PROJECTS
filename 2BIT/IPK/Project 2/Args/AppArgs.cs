using ipk25chat_client.Enums;

namespace ipk25chat_client.Args;

public record AppArgs
{
    // configuration of the program
    public Protocol? ProtocolType { get; init; } = null;
    public string? ServerIp { get; init; } = null;
    public UInt16 ServerPort { get; init; } = 4567;
    public UInt16 UdpTimeout { get; init; } = 250;
    public byte UdpRetransmission { get; init; } = 3;
}