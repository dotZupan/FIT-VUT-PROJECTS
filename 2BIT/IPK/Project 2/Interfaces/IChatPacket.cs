using ipk25chat_client.Enums;

namespace ipk25chat_client.Interfaces;

public interface IChatPacket
{
    MessageType Type();
    byte[] ToBytes();
}