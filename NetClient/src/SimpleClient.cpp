#include <FumblyNet.h>

enum class CustomMsgTypes : uint32_t
{
	PingServer,
	ServerMessage,
	ServerAccept,
	MessageAll
};

class CustomClient : public Fumbly::ClientInterface<CustomMsgTypes>
{
public:
	void PingServer()
	{
		Fumbly::Message<CustomMsgTypes> msg = {};
		msg.Header.Id = CustomMsgTypes::PingServer;

		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		msg << timeNow;

		Send(msg);
	}

	void MessageAll()
	{
		Fumbly::Message<CustomMsgTypes> msg = {};
		msg.Header.Id = CustomMsgTypes::MessageAll;
		Send(msg);
	}
};

bool key[3] = {false, false, false};
bool old_key[3] = {false, false, false};

int main()
{
	CustomClient client;
	client.Connect("127.0.0.1", 60000);

	bool isRunning = true;

	while(isRunning)
	{
		if(GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}
		if(key[0] && !old_key[0]) client.PingServer();
		if(key[1] && !old_key[1]) client.MessageAll();
		if(key[2] && !old_key[2]) isRunning = false;

		for(int i = 0; i < 3; i++) old_key[i] = key[i];
			
		if(client.IsConnected())
		{
			if(!client.Incoming().Empty())	
			{
				auto msg = client.Incoming().PopFront().Msg;
				switch(msg.Header.Id)
				{
					case (CustomMsgTypes::ServerAccept):
					{
						std::cout << "Connected to Server!" << "\n";
					} break;
					case (CustomMsgTypes::PingServer):
					{
						std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point timeThen;
						msg >> timeThen;
						std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
					} break;
					case (CustomMsgTypes::ServerMessage):
					{
						uint32_t clientID; 
						msg >> clientID;
						std::cout << "Hello From: " << clientID << "\n";
					} break;
				}
			}
		}
		else
		{
			isRunning = false;
		}
	}
	return 0;
}