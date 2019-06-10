#pragma once

#include "RealtimeObject.h"
#include <cassert>

template <typename T>
RealtimeObject<T>::RealtimeObject() : storage (std::make_unique<T>()), pointer (storage.get()) {}

template <typename T>
RealtimeObject<T>::RealtimeObject (const T & obj) : storage (std::make_unique<T> (obj)),        pointer (storage.get()) {}

template <typename T>
RealtimeObject<T>::RealtimeObject (T && obj) : storage (std::make_unique<T> (std::move (obj))), pointer (storage.get()) {}

template <typename T>
RealtimeObject<T>::~RealtimeObject()
{
    assert (pointer.load() != nullptr); // <- never delete this object while the realtime thread is holding the lock

    // Spin!
    while (pointer.load() == nullptr);

    auto accquired = nonRealtimeLock.try_lock();

    ignoreUnused (accquired);
    assert (accquired);  // <- you didn't call release on one of the non-realtime threads before deleting this object

    nonRealtimeLock.unlock();
}

template <typename T>
template <typename... Args>
RealtimeObject<T> RealtimeObject<T>::create(Args... args)
{
    return RealtimeObject (std::make_unique<T> (std::forward(args...)));
}

template <typename T>
T* RealtimeObject<T>::acquire (bool isRealTimeThread)
{
    T* result;

    if (isRealTimeThread)
    {
        assert (pointer.load() != nullptr); // <- You didn't balance your acquire and release calls!
        result = pointer.exchange (nullptr);
    }
    else
    {
        nonRealtimeLock.lock();
        copy.reset (new T (*storage));

        result = copy.get();
    }

    return result;
}

template <typename T>
void RealtimeObject<T>::release (T* obj)
{
    // did this object come from the realtime thread?
    if (obj == storage.get())
    {
        assert (pointer.load() == nullptr); // <- You didn't balance your acquire and release calls!

        // replace back
        pointer.store (obj);
    }
    else if (obj == copy.get()) // this object came from a non realtime thread. it's ok to block!
    {
        T* ptr;

        // block until realtime thread is done using the object
        do {
            ptr = storage.get();
        } while (! pointer.compare_exchange_weak (ptr, copy.get()));

        storage = std::move (copy);
        nonRealtimeLock.unlock();
    }
    else
    {
        // you tried to release an object that did not come from acquire!!!
       assert (false);
    }
}

template <typename T>
RealtimeObject<T>::RealtimeObject(std::unique_ptr<T> && u) : storage (std::move (u)), pointer (storage.get()) {}

template <typename T>
RealtimeObject<T>::ScopedAccess::ScopedAccess (RealtimeObject<T>& parent, bool isRealtimeThread)
    : p (parent),
      currentValue (p.acquire (isRealtimeThread))
{}

template <typename T>
RealtimeObject<T>::ScopedAccess::~ScopedAccess()
{
    p.release (currentValue);
}

template <typename T> T* RealtimeObject<T>::ScopedAccess::get() noexcept                       { return currentValue;  }
template <typename T> const T* RealtimeObject<T>::ScopedAccess::get() const noexcept           { return currentValue;  }
template <typename T> T &RealtimeObject<T>::ScopedAccess::operator *() noexcept                { return *currentValue; }
template <typename T> const T &RealtimeObject<T>::ScopedAccess::operator *() const noexcept    { return *currentValue; }
template <typename T> T* RealtimeObject<T>::ScopedAccess::operator->() noexcept                { return currentValue; }
template <typename T> const T* RealtimeObject<T>::ScopedAccess::operator->() const noexcept    { return currentValue; }
