#include <FumblyNet.h>

enum class CustomMsgTypes : uint32_t
{
	PingServer,
	ServerMessage,
	ServerAccept,
	MessageAll
};

class CustomServer: public Fumbly::ServerInterface<CustomMsgTypes>
{
public:
	CustomServer(uint16_t nPort ) : Fumbly::ServerInterface<CustomMsgTypes>(nPort)
	{

	}
protected:
    virtual bool OnClientConnect(std::shared_ptr<Fumbly::Connection<CustomMsgTypes>> client)
    {
        Fumbly::Message<CustomMsgTypes> msg; 
        msg.Header.Id = CustomMsgTypes::ServerAccept;
        client->Send(msg);

        return true;
    }

    virtual void OnClientDisconnect(std::shared_ptr<Fumbly::Connection<CustomMsgTypes>> client)
    {

    }

    // Main message callback
    virtual void OnMessage(std::shared_ptr<Fumbly::Connection<CustomMsgTypes>> client, Fumbly::Message<CustomMsgTypes> msg)
    {
        switch(msg.Header.Id)
        {
            case(CustomMsgTypes::PingServer):
            {
                std::cout << "[" << client->GetID() << "]: Pinged the Server!\n";
                client->Send(msg);
            } break;
            case(CustomMsgTypes::MessageAll):
            {
                Fumbly::Message<CustomMsgTypes> msgToAll;
                msgToAll.Header.Id = CustomMsgTypes::ServerMessage;
                msgToAll << client->GetID();

                MessageAllClients(msgToAll, client);
            } break;
        }
    }
};

int main()
{
    const uint16_t PORT = 60000;
    CustomServer server(PORT);

    server.Start();

    while (true)
    {
        server.Update(-1, true);
    }
	return 0;
}