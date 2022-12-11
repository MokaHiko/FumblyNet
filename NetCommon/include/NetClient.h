#ifndef NETCLIENT_H
#define NETCLIENT_H

#pragma once

#include "NetCommon.h"
#include "NetMessage.h"
#include "TSQueue.h"
#include "NetConnection.h"

namespace Fumbly {
    template<typename T>
    class ClientInterface
    {
    public:
        ClientInterface() : m_Socket(m_AsioContext)
        {
        }

        virtual ~ClientInterface()
        {
            Disconnect();
        }
    public:
        bool Connect(const std::string& host, const uint16_t port)
        {
            try
            {
                // Resolve hostname/ip-address into a tanable physical address
                asio::ip::tcp::resolver resolver(m_AsioContext);
                asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

                // Create Connection Instance and instantiate client socket
                m_Connection = std::make_unique<Connection<T>>(
                    Connection<T>::Owner::Client,
                    m_AsioContext,
                    asio::ip::tcp::socket(m_AsioContext),
                    m_QMessagesIn
                    );

                // Tell the connection object to connect to server and prime it for listening to messages
                m_Connection->ConnectToServer(endpoints);

                // Start Context Thread
                m_ThrContext = std::thread([this](){m_AsioContext.run();});
            }
            catch (const std::exception& e) {
                std::cerr << "Client Exception: " << e.what() << "\n";
                return false;
            }

            return true;
        }

        void Disconnect()
        {
            if(IsConnected()) {
                m_Connection->Disconnect();
            }

            m_AsioContext.stop();

            // Stop asio thread
            if(m_ThrContext.joinable()) {
                m_ThrContext.join();
            }

            m_Connection.release();
        }

        bool IsConnected()
        {
            if (m_Connection) {
                return m_Connection->IsConnected();
            }
            else {
                return false;
            }
        }

        TSQueue<OwnedMessage<T>>& Incoming()
        {
            return m_QMessagesIn;
        }

        void Send(const Message<T>& msg)
        {
            if(IsConnected())
                m_Connection->Send(msg);
        }

    protected:

        // Asio context that handles the data transfer...
        asio::io_context m_AsioContext;

        // ... but needs a thread of its wont o execute its work commands
        std::thread m_ThrContext;

        // This is the hardware socket that is connected to the server
        asio::ip::tcp::socket m_Socket;

        // Client has single instance of connection which handles data transfer
        std::unique_ptr<Connection<T>> m_Connection;

    private:
        // Thread safe queue of incoming messages from server to be used by connection
        TSQueue<OwnedMessage<T>> m_QMessagesIn;
    };
}

#endif