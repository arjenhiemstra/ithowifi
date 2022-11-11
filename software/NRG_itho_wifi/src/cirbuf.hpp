#pragma once

#include <array>
#include <mutex>

template <class T, size_t TElemCount>
class circular_buffer
{
public:
    explicit circular_buffer() = default;

    int put(T item) noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        int pos = head_;

        buf_[head_] = item;

        if (full_)
        {
            tail_ = (tail_ + 1) % TElemCount;
        }

        head_ = (head_ + 1) % TElemCount;

        full_ = head_ == tail_;

        return pos;
    }

    T pop() const noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        if (empty())
        {
            return T();
        }

        // Read data and advance the tail (we now have a free space)
        auto val = buf_[tail_];
        full_ = false;
        tail_ = (tail_ + 1) % TElemCount;

        return val;
    }

    T &peek(size_t pos) const noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        if (empty())
        {
            return buf_[0];
        }

        pos = pos % TElemCount;

        return buf_[pos];
    }

    T &peek_first() const noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        if (empty())
        {
            return buf_[0];
        }

        return buf_[tail_];
    }

    T &peek_back() const noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        if (empty())
        {
            return buf_[0];
        }

        int pos = head_ - 1;
        if (pos < 0)
            pos = TElemCount - 1;

        return buf_[pos];
    }

    size_t get_back_pos() const noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        if (empty())
        {
            return 0;
        }

        int pos = head_ - 1;
        if (pos < 0)
            pos = TElemCount - 1;

        return pos;
    }

    size_t get_first_pos() const noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        if (empty())
        {
            return 0;
        }

        return tail_;
    }

    void reset() noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        head_ = tail_;
        full_ = false;
    }

    bool empty() const noexcept
    {
        // Can have a race condition in a multi-threaded application
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        // if head and tail are equal, we are empty
        return (!full_ && (head_ == tail_));
    }

    bool full() const noexcept
    {
        // If tail is ahead the head by 1, we are full
        return full_;
    }

    size_t capacity() const noexcept
    {
        return TElemCount;
    }

    size_t size() const noexcept
    {
        // A lock is needed in size ot prevent a race condition, because head_, tail_, and full_
        // can be updated between executing lines within this function in a multi-threaded
        // application
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        size_t size = TElemCount;

        if (!full_)
        {
            if (head_ >= tail_)
            {
                size = head_ - tail_;
            }
            else
            {
                size = TElemCount + head_ - tail_;
            }
        }

        return size;
    }

private:
    mutable std::recursive_mutex mutex_;
    mutable std::array<T, TElemCount> buf_;
    mutable size_t head_ = 0;
    mutable size_t tail_ = 0;
    mutable bool full_ = 0;
};
