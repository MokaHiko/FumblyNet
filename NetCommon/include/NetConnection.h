#pragma once

#include <memory>

#include "TSQueue.h"
#include "NetMessage.h"

namespace Fumbly {
    // Forward declare server interface
    template<typename T>
    class ServerInterface;

    template<typename T>
    class Connection: public std::enable_shared_from_this<Connection<T>>
    {
    public:
        enum class Owner
        {
            Unkown,
            Server, 
            Client
        };

        Connection(Owner owner, asio::io_context& asioContext, asio::ip::tcp::socket socket, TSQueue<OwnedMessage<T>>& qIn)
            :m_AsioContext(asioContext), m_Socket(std::move(socket)), m_QMessagesIn(qIn)
        {
            m_OwnerType = owner;

            // Validation paths
            if(m_OwnerType == Owner::Server)
            {
                // Store sent message
                m_HandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

                // Store precalculate solution
                m_HandshakeCheck = EncryptData(m_HandshakeOut);
            }
            else if (m_OwnerType == Owner::Client)
            {
                m_HandshakeIn = 0;
                m_HandshakeOut = 0;
            }
        }
        ~Connection(){}

    public:
        // Connection provides unique ID to client and prime context for messages
        void ConnectToClient(ServerInterface<T>* server, uint32_t uid = 0)
        {
            if (m_OwnerType == Owner::Server)
            {
                if (m_Socket.is_open())
                {
                    m_ID = uid;

                    // Send validation data to client trying to connect
                    WriteValidation();

                    // Wait async for client to respond
                    ReadValidation(server);
                }
            }
        }

        void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
        {
            // Give asio async task
            if(m_OwnerType == Owner::Client)
            {
                asio::async_connect(m_Socket, endpoints,
                [this](std::error_code ec, asio::ip::tcp::endpoint endpoint){
                    if(!ec)
                    {
                        ReadValidation();
                    }
                    else
                    {
                        std::cout << "[" << m_ID << "] Connect to Server Fail.\n";
                        m_Socket.close();
                    }
                });
            }
        }

        // Can be called by client or server
        void Disconnect()
        {
            if(IsConnected())
            {
                // Prime Asio to disconnect
                asio::post(m_AsioContext, [this]() {m_Socket.close(); });
            }
        }
        bool IsConnected() const
        {
            return m_Socket.is_open();
        }

        uint32_t GetID() {return m_ID;}

    public:
        void Send(const Message<T>& msg)
        {
            asio::post(m_AsioContext, [this, msg]() {
                bool isWritingMessages = !m_QMessagesOut.Empty();
                m_QMessagesOut.PushBack(msg);

                // Check if asio context has already begun the loop of writing messages
                if (!isWritingMessages)
                {
                    WriteHeader();
                }
                });
        }

    private:
        // ASYNC - Prime Context ready to read a message header
        void ReadHeader()
        {
            asio::async_read(m_Socket, asio::buffer(&m_MsgTemporaryIn.Header, sizeof(MessageHeader<T>)), 
            [this](std::error_code ec, std::size_t length) {
                if(!ec)
                {
                    // Check if message has body
                    if(m_MsgTemporaryIn.Header.Size > 0)
                    {
                        // Allocate enough space on temporary msg buffer for msg
                        m_MsgTemporaryIn.Body.resize(m_MsgTemporaryIn.Header.Size);
                        ReadBody();
                    }
                    else
                    {
                        AddToIncomingMessageQueue();
                    }
                } 
                else            
                {
                    std::cout << "[" << m_ID << "] Read Header Fail.\n";
                    m_Socket.close();
                }
            });
        }

        // ASYNC - Prime Context ready to read a message body
        void ReadBody()
        {
            asio::async_read(m_Socket, asio::buffer(m_MsgTemporaryIn.Body.data(), m_MsgTemporaryIn.Header.Size),
            [this](std::error_code ec, std::size_t length) {
                if(!ec)
                {
                    AddToIncomingMessageQueue();
                }
                else
                {
                    std::cout << "[" << m_ID << "] Read Body Fail.\n";
                    m_Socket.close();
                }
            });
        }

        // ASYNC - Prime Context ready to write a message header
        void WriteHeader()
        {
            asio::async_write(m_Socket, asio::buffer(&m_QMessagesOut.Front().Header, sizeof(MessageHeader<T>)),
            [this](std::error_code ec, std::size_t length){
                if(!ec)
                {
                    // Check if message at the front of the queue has a body to send
                    if(m_QMessagesOut.Front().Body.size() > 0)
                    {
                        WriteBody();
                    }
                    else
                    {
                        m_QMessagesOut.PopFront();
                        if(!m_QMessagesOut.Empty())
                        {
                            WriteHeader();
                        }
                    }
                }
                else
                {
                    std::cout << "[" << m_ID << "] Write Header Fail.\n";
                    m_Socket.close();
                }
            });
        }

        // ASYNC - Prime Context ready to write a message body
        void WriteBody()
        {
            asio::async_write(m_Socket, asio::buffer(m_QMessagesOut.Front().Body.data(), m_QMessagesOut.Front().Header.Size), 
            [this](std::error_code ec, std::size_t length){
                if(!ec)
                {
                    m_QMessagesOut.PopFront();
                    if(!m_QMessagesOut.Empty())
                    {
                        WriteHeader();
                    }
                }
                else
                {
                    std::cout << "[" << m_ID << "] Write Body Fail.\n";
                    m_Socket.close();
                }
            });
        }

        void AddToIncomingMessageQueue()
        {
            if(m_OwnerType == Owner::Server)
            {
                // Push message into QueueMessagesIn as owned message with shared pointer to connection
                m_QMessagesIn.PushBack({this->shared_from_this(), m_MsgTemporaryIn});
            }
            else
            {
                // It is assumed the message comes from the server
                m_QMessagesIn.PushBack({nullptr, m_MsgTemporaryIn});
            }
            // Give asio another async task (reading more messages)
            ReadHeader();
        }

        // "Encrypt data"
        uint64_t EncryptData(uint64_t input)
        {
            uint64_t output = input ^ 0xDEABCFFA;
            return output;
        }

        // ASYNC - Used by both server and client to write validation packet
        void WriteValidation()
        {
            asio::async_write(m_Socket, asio::buffer(&m_HandshakeOut, sizeof(uint64_t)), 
            [this](asio::error_code ec, std::size_t length){
                if(!ec)
                {
                    if(m_OwnerType == Owner::Client)
                        ReadHeader();
                }
                else
                {
                    std::cout << "Failed to sent validation message!\n";
                    m_Socket.close();
                }
            });
        }

        // Async - Used by both server and client to read validation messages sent
        void ReadValidation(ServerInterface<T>* server = nullptr)
        {
            asio::async_read(m_Socket, asio::buffer(&m_HandshakeIn, sizeof(uint64_t)),
                [this, server](asio::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        if (m_OwnerType == Owner::Server)
                        {
                            if (m_HandshakeIn == m_HandshakeCheck)
                            {
                                // Call client validation callback
                                std::cout << "Client Validated" << std::endl;
                                server->OnClientValidated(this->shared_from_this());

                                // Begin Async Read Loop
                                ReadHeader();
                            }
                            else
                            {
                                std::cout << "Client Disconected (Failed to validate message!)\n";
                                m_Socket.close();
                            }
                        }
                        else if (m_OwnerType == Owner::Client)
                        {
                            // Solve received message
                            m_HandshakeOut = EncryptData(m_HandshakeIn);

                            // Send solution
                            WriteValidation();
                        }
                    }
                    else
                    {
                        std::cout << "Client Disconected (Failed to read validation message!)\n";
                        m_Socket.close();
                    }
                });
        }
    protected:
        // Unique socket to a remote held by connection
        asio::ip::tcp::socket m_Socket;

        // Client/Server will provide the local application context
        asio::io_context& m_AsioContext;

        // Queue holds all messages to be sent to the remote side of this connection
        TSQueue<Message<T>> m_QMessagesOut;

        // Client/Server provides queue queue holds all messages that have been recieved from the remote side of this connection. 
        // It is a reference as the owner of this connection is expected to provide a queue
        TSQueue<OwnedMessage<T>>& m_QMessagesIn;
        Message<T> m_MsgTemporaryIn;
        Message<T> m_MsgTemporaryOut;

        Owner m_OwnerType = Owner::Unkown;

        uint32_t m_ID = 0;

        // Handshake validation
        uint64_t m_HandshakeOut = 0;
        uint64_t m_HandshakeIn = 0;
        uint64_t m_HandshakeCheck = 0;
    };
}
