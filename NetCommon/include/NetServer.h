#ifndef NETSERVER_H
#define NETSERVER_H

#pragma once
#include "NetCommon.h"
#include "TSQueue.h"
#include "NetMessage.h"
#include "NetConnection.h"

namespace Fumbly {
    template<typename T>
    class ServerInterface
    {
    public:
        ServerInterface(uint16_t port) 
            :m_AsioAcceptor(m_AsioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
        {

        }

        ~ServerInterface()
        {
            Stop();
        }

        bool Start() 
        {
            try {
                // Prime work for server
                WaitForClientConnection();

                m_ThreadContext = std::thread([this](){m_AsioContext.run();});
            } catch (std::exception& e) {
                std::cerr << "[SERVER] Exeption: " << e.what() << "\n";
                return false;
            }

            std::cout << "[SERVER] Started!\n";
            return true;
        }

        bool Stop()
        {
            // Tell Asio to Stop
            m_AsioContext.stop();

            // Make sure all asio work is finished before exiting by waiting with join()
            if(m_ThreadContext.joinable())
                m_ThreadContext.join();

            std::cout << "[SERVER] Stopped!\n";
            return true;
        }

        // ASYNC - Instruct asio to wait for connection
        void WaitForClientConnection()
        {
            m_AsioAcceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket){
                if(!ec) {
                    std::cout << "[SERVER] New Connection: "  << socket.remote_endpoint() << "\n";

                    // Create connection between client and server
                     std::shared_ptr<Connection<T>> newconn = std::make_shared<Connection<T>>(
                         Connection<T>::Owner::Server, 
                         m_AsioContext,
                         std::move(socket),
                         m_QMessagesIn
                     );

                    // Give user ability to deny connection
                     if(OnClientConnect(newconn)) {
                         m_DeqConnections.push_back(std::move(newconn));
                         m_DeqConnections.back()->ConnectToClient(this, m_IDCounter++);
                         std::cout << "[SERVER] client: " << m_DeqConnections.back()->GetID() << " Connection Approved\n";

                     } else {
                         std::cout << "[SERVER] remote endpoint: " << socket.remote_endpoint() << " Denied Connection \n";
                     }
                } else {
                    std::cout << "[SERVER] New Connection Error: "  << ec.message() << "\n";
                }

                //  Make sure server is always primed to listen for connections
                WaitForClientConnection();
            });
        }

        void MessageClient(std::shared_ptr<Connection<T>> client, const Message<T>& msg) 
        {
            if(client && client->IsConnected()) {
                client->Send(msg);
            } else {
                OnClientDisconnect(client);
                client.reset(); 
                m_DeqConnections.erase(std::remove(m_DeqConnections.begin(), m_DeqConnections.end(), cient), m_DeqConnections.end());
            }
        }

        void MessageAllClients(const Message<T>& msg, std::shared_ptr<Connection<T>> pIgnoreClient)
        {
            bool bInvalidClientsExist = false;
            for(auto& client : m_DeqConnections)
            {
                // check if client is connected
                if(client && client->IsConnected())
                {
                    if(client != pIgnoreClient)
                        client->Send(msg);
                } else {
                    OnClientDisconnect(client);
                    client.reset();
                    bInvalidClientsExist = true;
                }
            }

            // erase-remove all invalid connections
            if(bInvalidClientsExist) {
                m_DeqConnections.erase(
                    std::remove(m_DeqConnections.begin(), m_DeqConnections.end(), nullptr), 
                    m_DeqConnections.end());
            }
        }

        void Update(size_t nmaxMessages = -1, bool waitForMessage = false) 
        {
            // We don't need the server to occupy 100% of a cpu core
            if(waitForMessage) m_QMessagesIn.Wait();
            size_t nMessageCount = 0;

            while(nMessageCount < nmaxMessages && !m_QMessagesIn.Empty())
            {
                // Grab the front message
                auto msg = m_QMessagesIn.PopFront();

                // Pass to message handler
                OnMessage(msg.Remote, msg.Msg);

                nMessageCount++;
            }
        }

    public:
        // Called after client is validated
        virtual void OnClientValidated(std::shared_ptr<Connection<T>> client)
        {

        }
    protected:
        // Return if valid client
        virtual bool OnClientConnect(std::shared_ptr<Connection<T>> client)
        {
            return false;
        }

        virtual void OnClientDisconnect(std::shared_ptr<Connection<T>> client)
        {

        }

        // Main message callback
        virtual void OnMessage(std::shared_ptr<Connection<T>> client, Message<T> msg)
        {

        }
    
    protected:
        // Thread safe queue of incoming messages
        TSQueue<OwnedMessage<T>> m_QMessagesIn;

        // Container of all active validated connections
        std::deque<std::shared_ptr<Connection<T>>> m_DeqConnections;

        // Server ASIO context
        asio::io_context m_AsioContext;
        std::thread m_ThreadContext;

        // Asio acceptor that handles client sockets
        asio::ip::tcp::acceptor m_AsioAcceptor;

        // Clients will have a global ID;
        uint32_t m_IDCounter = 10000;
    };
}

#endif