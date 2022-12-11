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

            // Signal condition variable to wake up
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.notify_one();
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

            // Signal condition variable to wake up
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.notify_one();
        }

        bool Empty()
        {
            std::scoped_lock(muxQueue);
            return deqQue.empty();
        }

        void Wait()
        {
            // Run while deqQue is empty
            while(Empty())
            {
                std::unique_lock<std::mutex> ul(muxBlocking);

                // wait until condition has been signalled by push or pop functions
                cvBlocking.wait(ul);
            }
        }

    private:
        std::mutex muxQueue;
        std::deque<T> deqQue;

        std::condition_variable cvBlocking;
        std::mutex muxBlocking;
    };
}