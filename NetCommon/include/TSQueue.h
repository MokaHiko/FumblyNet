#pragma once
#include <thread>
#include <mutex>
#include <deque>

namespace Fumbly {
    template<typename T>
    class TSQueue
    {
    public:
        TSQueue(){}
        ~TSQueue() {Clear();}

        TSQueue(const TSQueue& other) = delete;
        void operator=(const TSQueue& other) = delete;
    public:
        const T& Front()
        {
            std::scoped_lock lock(muxQueue);
            return deqQue.front();
        }

        const T& Back()
        {
            std::scoped_lock lock(muxQueue);
            return deqQue.back();
        }

        const T& Count()
        {
            std::scoped_lock lock(muxQueue);
            return deqQue.size()
        }

        void Clear()
        {
            std::scoped_lock lock(muxQueue);
            return deqQue.clear();
        } 

        void PushFront(const T& item)
        {
            std::scoped_lock(muxQueue);
            deqQue.emplace_front(std::move(item));
        }

        T PopFront()
        {
            std::scoped_lock lock(muxQueue);
            auto t = std::move(deqQue.front());
            deqQue.pop_front();
            return t;
        }

        T PopBack()
        {
            std::scoped_lock lock(muxQueue);
            auto t = std::move(deqQue.back());
            deqQue.pop_back();
            return t;
        }

        void PushBack(const T& item)
        {
            std::scoped_lock(muxQueue);
            deqQue.emplace_back(std::move(item));

        }

        bool Empty()
        {
            std::scoped_lock(muxQueue);
            return deqQue.empty();
        }

    private:
        std::mutex muxQueue;
        std::deque<T> deqQue;
    };
}