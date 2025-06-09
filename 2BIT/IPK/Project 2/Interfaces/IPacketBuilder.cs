namespace ipk25chat_client.Interfaces;

public interface IPacketBuilder {
    IChatPacket buildAuth(string username, string secret, string displayName);
    IChatPacket buildJoin(string channel, string displayName);
    IChatPacket buildMsg(string displayName, string msg);
    IChatPacket buildBye(string displayName);
    IChatPacket buildErr(string displayName, string err);
}