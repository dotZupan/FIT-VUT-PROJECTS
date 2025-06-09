using System.Security;

using ipk25chat_client.Enums;

namespace ipk25chat_client;

public class FSM
{
    private FSMEnum currState = FSMEnum.start;
    public FSMEnum State => currState;
    private readonly object stateLock = new();


    public void ChageStateSend(MessageType msg)
    {
        lock (stateLock)
        {
            FSMEnum newState = currState;
            if (msg == MessageType.BYE)
            {
                newState = FSMEnum.end;
            }

            switch (msg)
            {
                case MessageType.AUTH:
                    //Console.Error.WriteLine("DEBUG:" + currState);
                    if (currState == FSMEnum.start)
                    {
                        newState = FSMEnum.auth;
                    }
                    else if (currState == FSMEnum.auth)
                    {
                        newState = FSMEnum.auth;
                    }
                    else
                    {
                        throw new ArgumentException("Invalid state");
                    }

                    
                    break;

                case MessageType.JOIN:
                    // Console.Error.WriteLine("DEBUG:" + currState);
                    if (currState == FSMEnum.open)
                    {
                        newState = FSMEnum.join;
                    }
                    else
                    {
                        throw new ArgumentException("Invalid state");
                    }

                    // Console.WriteLine(newState);
                    break;

            }
            // Console.Error.WriteLine(newState);
            currState = newState;
        }
    }
    
    
    // changes states based on the recieved messages. It takes control over whole fsm so no other task can interfere with the states
    public void changeState(string? prefix)
    {
        lock (stateLock)
        {
            FSMEnum newState = currState;

            if (prefix == "ERR" || prefix == "BYE")
            {
                currState = FSMEnum.end;
                return;
            }

            switch (currState)
            {
                case FSMEnum.start:
                    if (prefix == "REPLY OK")
                    {
                        newState = FSMEnum.open; 
                    }
                    else if (prefix == "ERR" || prefix == "BYE")
                    {
                        newState = FSMEnum.end;
                    }
                    break;

                case FSMEnum.auth:
                    if (prefix == "REPLY OK")
                    {
                        newState = FSMEnum.open;
                    }
                    else if (prefix == "MSG")
                    {
                        newState = FSMEnum.end;
                    }

                    // becuase at the start new state is curr_state then reply nok case is not needed, this behavior is replicated in similliar cases
                    break;

                case FSMEnum.open:
                    if (prefix == "REPLY OK" || prefix == "REPLY NOK")
                    {
                        newState = FSMEnum.end;
                    }

                    
                    break;

                case FSMEnum.join:
                    
                    if (prefix == "REPLY OK" || prefix == "REPLY NOK")
                    {
                        newState = FSMEnum.open;
                    }

                    break;

                default:
                    newState = FSMEnum.end;
                    break;
            }
            // Console.Error.WriteLine(newState);
            currState = newState;
        }
    }

    // Is message given by user valid in this state?
    public bool IsCmdValid(MessageType msg)
    {
        if (msg == MessageType.BYE || msg == MessageType.ERR)
        {
            return true;
        }
        return currState switch
        {
            FSMEnum.start => msg == MessageType.AUTH,
            FSMEnum.auth  => msg == MessageType.AUTH,
            FSMEnum.open  => msg == MessageType.JOIN || msg == MessageType.MSG,
            FSMEnum.join  => false,
            FSMEnum.end   => false,
            _             => false
        };
    }
    
}