#pragma once

#include "NetCommon.h"

namespace Fumbly {
    template <typename T>
    struct MessageHeader
    {
        T Id{};
        uint32_t Size = 0;
    };
    
    template <typename T>
    struct Message
    {
        MessageHeader<T> Header{};
        std::vector<uint8_t> Body{};

        // Returns of size of entire message packet in bytes
        size_t Size() const {return Body.size();}

        // Ostream overload for debugging
        friend std::ostream& operator<<(std::ostream& os, const Message<T>& msg)
        {
            os << "ID: " << (int)(msg.Header.Id) << " Size: "  << msg.Header.size();
            return os;
        }

        // Pushing data to body of message
        template<typename DataType>
        friend Message<T>& operator<<(Message<T>& msg, const DataType& data)
        {
            // check if data type is serializable
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be transmitted in Message\n");

            // cache current size of vector for data insert
            size_t i = msg.Body.size();

            // resize the vector by the size of the data being pushed
            msg.Body.resize(msg.Body.size() + sizeof(DataType));

            // physicall copy the data into the newly allocated vector space
            memcpy(msg.Body.data() + i, &data, sizeof(DataType)); 

            // Recalculate the message size;
            msg.Header.Size = static_cast<uint32_t>(msg.Size());

            return msg;
        }

        template<typename DataType> 
        friend Message<T>& operator>>(Message<T>& msg, DataType& data)
        {
            // check if data is serializable
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be processed!");

            // cache locatiion where to start pulling the data
            size_t i = msg.Body.size() - sizeof(DataType);

            // physically copy data from the vector to user defined reference
            memcpy(&data, msg.Body.data() + i, sizeof(DataType));

            // resize vector after removed bytes
            msg.Body.resize(i);

            // recalculate header size
            msg.Header.Size = static_cast<uint32_t>(msg.Size());

            return msg;
        }
    };

	template<typename T>
	class Connection;

	template<typename T>
	struct OwnedMessage
	{
        // Reference to connection in order to identify message
		std::shared_ptr<Connection<T>> Remote = nullptr;
		Message<T> Msg;

		friend std::ostream& operator<<(std::ostream& os, const OwnedMessage& msg)
		{
			os << msg.Msg;
			return os;
		}
	};
}
