#pragma once
#include <vector>
#include <atomic>
#include <cassert>

#include "clock.hpp"

namespace farbot
{
// single consumer, single producer
#if 0
template <typename T>
class fifo
{
public:
    fifo (int capacity) : slots (capacity + 1) {}

    // single producer
    bool push (T && arg)
    {
        auto pos  = writepos.load(std::memory_order_relaxed);
        auto next = (pos + 1) % static_cast<int> (slots.size());

        if (next == readpos.load(std::memory_order_acquire))
            return false;

        slots[pos] = std::move (arg);
        writepos.store (next, std::memory_order_release);

        return true;
    }

    // single consumer
    bool pop(T& result)
    {
        auto pos = readpos.load(std::memory_order_relaxed);
        
        if (pos == writepos.load(std::memory_order_acquire))
            return false;

        result = std::move (slots[pos]);
        readpos.store ((pos + 1) % static_cast<int> (slots.size()), std::memory_order_release);

        return true;
    }

private:
    std::vector<T> slots = {};
    std::atomic<int> readpos = 0, writepos = 0;
};
#endif
#pragma once
#include <vector>
#include <atomic>
#include <cassert>

#include "clock.hpp"

namespace farbot
{
// single consumer, single producer
template <typename T>
class fifo
{
public:
    fifo (std::uint32_t capacity) 
        : slots (capacity), 
          slot_mask (capacity - 1),
          storage (capacity << 1),
          freelist (capacity << 1),
          storage_mask ((capacity << 1) - 1)
    {
        // capacity must be power of two
        assert ((capacity & slot_mask) == 0);

        for (auto& it : slots)
            it.store (nullptr, std::memory_order_relaxed);

        auto* ptr = storage.data();
        for (auto& it : freelist)
            it = ptr++;
    }

    bool push(T* value)
    {
        for (std::size_t i = 0; i < slots.size(); ++i)
        {
            auto pos = writepos.fetch_add (1, std::memory_order_relaxed) & slot_mask;

            T* tmp = nullptr;
            if (slots[pos].compare_exchange_strong (tmp, value, std::memory_order_acq_rel))
                return true;
        }

        return false;
    }

    bool pop (T*& result)
    {
        for (std::size_t i = 0; i < slots.size(); ++i)
        {
            auto pos = readpos.fetch_add (1, std::memory_order_release) & slot_mask;

            if (result = slots[pos].exchange (nullptr, std::memory_order_acq_rel))
                return true;
        }

        return false;
    }

private:
    std::vector<std::atomic<T*>> slots;
    const std::uint32_t slot_mask;
    std::atomic<std::uint32_t> readpos = {0}, writepos = {0};
};
}
}