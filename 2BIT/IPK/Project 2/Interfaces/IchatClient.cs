using ipk25chat_client.Interfaces;

public interface IChatClient
{
    void EnqueuePacket(IChatPacket packet);
    Task SendAsync();
    Task ReceiveAsync();
    bool SendQueueEmpty { get; }
    void GracefulShutdown(int code = 0);
    public void Connect(string host, int port);
    Task WaitForSendQueueEmptyAsync();
}