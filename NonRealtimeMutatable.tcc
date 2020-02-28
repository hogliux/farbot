#pragma once

#include "NonRealtimeMutatable.hpp"
#include <cassert>

namespace farbot
{
template <typename T>
NonRealtimeMutatable<T>::NonRealtimeMutatable() : storage (std::make_unique<T>()), pointer (storage.get()) {}

template <typename T>
NonRealtimeMutatable<T>::NonRealtimeMutatable (const T & obj) : storage (std::make_unique<T> (obj)),        pointer (storage.get()) {}

template <typename T>
NonRealtimeMutatable<T>::NonRealtimeMutatable (T && obj) : storage (std::make_unique<T> (std::move (obj))), pointer (storage.get()) {}

template <typename T>
NonRealtimeMutatable<T>::~NonRealtimeMutatable()
{
    assert (pointer.load() != nullptr); // <- never delete this object while the realtime thread is holding the lock

    // Spin!
    while (pointer.load() == nullptr);

    auto accquired = nonRealtimeLock.try_lock();

    ((void)(accquired));
    assert (accquired);  // <- you didn't call release on one of the non-realtime threads before deleting this object

    nonRealtimeLock.unlock();
}

template <typename T>
template <typename... Args>
NonRealtimeMutatable<T> NonRealtimeMutatable<T>::create(Args && ... args)
{
    return RealtimeObject (std::make_unique<T> (std::forward<Args>(args)...));
}

template <typename T>
const T& NonRealtimeMutatable<T>::realtimeAcquire() noexcept
{
    assert (pointer.load() != nullptr); // <- You didn't balance your acquire and release calls!
    currentObj = pointer.exchange (nullptr);
    return *currentObj;
}

template <typename T>
void NonRealtimeMutatable<T>::realtimeRelease() noexcept
{
    // You didn't balance your acquire and release calls
    assert (pointer.load() == nullptr); 

    // replace back
    pointer.store (currentObj);
}

template <typename T>
T& NonRealtimeMutatable<T>::nonRealtimeAcquire()
{
    nonRealtimeLock.lock();
    copy.reset (new T (*storage));

    return *copy.get();
}

template <typename T>
void NonRealtimeMutatable<T>::nonRealtimeRelease()
{
    T* ptr;

    // block until realtime thread is done using the object
    do {
        ptr = storage.get();
    } while (! pointer.compare_exchange_weak (ptr, copy.get()));

    storage = std::move (copy);
    nonRealtimeLock.unlock();
}

template <typename T>
template <typename... Args>
void NonRealtimeMutatable<T>::nonRealtimeReplace(Args && ... args)
{
    nonRealtimeLock.lock();
    copy.reset (new T (std::forward<Args>(args)...));

    nonRealtimeRelease();
}

template <typename T>
NonRealtimeMutatable<T>::NonRealtimeMutatable(std::unique_ptr<T> && u) : storage (std::move (u)), pointer (storage.get()) {}

//==============================================================================
namespace detail
{
template <typename T, bool>
class NRMScopedAccessImpl
{
protected:
    NRMScopedAccessImpl (NonRealtimeMutatable<T>& parent)
        : p (parent), currentValue (&p.nonRealtimeAcquire()) {}
    ~NRMScopedAccessImpl() { p.nonRealtimeRelease(); }
public:
    T* get() noexcept                       { return currentValue;  }
    const T* get() const noexcept           { return currentValue;  }
    T &operator *() noexcept                { return *currentValue; }
    const T &operator *() const noexcept    { return *currentValue; }
    T* operator->() noexcept                { return currentValue; }
    const T* operator->() const noexcept    { return currentValue; }
private:
    NonRealtimeMutatable<T>& p;
    T* currentValue;
};

template <typename T>
class NRMScopedAccessImpl<T, true>
{
protected:
    NRMScopedAccessImpl (NonRealtimeMutatable<T>& parent) noexcept
        : p (parent), currentValue (&p.realtimeAcquire()) {}
    ~NRMScopedAccessImpl() noexcept { p.realtimeRelease(); }
public:
    const T* get() const noexcept           { return currentValue;  }
    const T &operator *() const noexcept    { return *currentValue; }
    const T* operator->() const noexcept    { return currentValue; }
private:
    NonRealtimeMutatable<T>& p;
    const T* currentValue;
};
}

template <typename T>
template <bool isRealtimeThread>
NonRealtimeMutatable<T>::ScopedAccess<isRealtimeThread>::ScopedAccess (NonRealtimeMutatable<T>& parent)
    : detail::NRMScopedAccessImpl<T, isRealtimeThread> (parent) {}
}