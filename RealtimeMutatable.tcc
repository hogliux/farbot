#pragma once

#include "RealtimeMutatable.hpp"
#include <cassert>

namespace farbot
{
template <typename T>
RealtimeMutatable<T>::RealtimeMutatable() {}

template <typename T>
RealtimeMutatable<T>::RealtimeMutatable (const T & obj) : data ({obj, obj}), realtimeCopy (obj) {}

template <typename T>
RealtimeMutatable<T>::~RealtimeMutatable()
{
    assert ((control.load() & BUSY_BIT) == 0); // <- never delete this object while the realtime thread is still using it

    // Spin!
    while ((control.load() & BUSY_BIT) == 1);

    auto accquired = nonRealtimeLock.try_lock();

    ((void)(accquired));
    assert (accquired);  // <- you didn't call release on one of the non-realtime threads before deleting this object

    nonRealtimeLock.unlock();
}

template <typename T>
template <typename... Args>
RealtimeMutatable<T> RealtimeMutatable<T>::create(Args && ... args)
{
    return RealtimeObject (false, std::forward<Args>(args)...);
}

template <typename T>
T& RealtimeMutatable<T>::realtimeAcquire() noexcept
{
    return realtimeCopy;
}

template <typename T>
void RealtimeMutatable<T>::realtimeRelease() noexcept
{
    auto idx = control.fetch_or (BUSY_BIT, std::memory_order_acquire) & INDEX_BIT;
    data[idx] = realtimeCopy;
    control.store ((idx & INDEX_BIT) | NEWDATA_BIT, std::memory_order_release);
}

template <typename T>
const T& RealtimeMutatable<T>::nonRealtimeAcquire()
{
    nonRealtimeLock.lock();
    auto current = control.load(std::memory_order_acquire);

    // there is new data so flip the indices around atomically ensuring we are not inside realtimeAssign
    if ((current & NEWDATA_BIT) != 0)
    {
        int newValue;

        do
        {
            // expect the realtime thread not to be inside the realtime-assign
            current &= ~BUSY_BIT;

            // realtime thread should flip index value and clear the newdata bit
            newValue = (current ^ INDEX_BIT) & INDEX_BIT;
        } while (! control.compare_exchange_weak (current, newValue, std::memory_order_acq_rel));

        current = newValue;
    }

    // flip the index bit as we always use the index that the realtime thread is currently NOT using
    auto nonRealtimeIndex = (current & INDEX_BIT) ^ 1;

    return data[nonRealtimeIndex];
}

template <typename T>
void RealtimeMutatable<T>::nonRealtimeRelease()
{
    nonRealtimeLock.unlock();
}

template <typename T>
template <typename... Args>
RealtimeMutatable<T>::RealtimeMutatable(bool, Args &&... args) 
   : data ({T (std::forward (args)...), T (std::forward (args)...)}), realtimeCopy (std::forward (args)...)
{}

//==============================================================================
namespace detail
{
template <typename T, bool>
class RMScopedAccessImpl
{
protected:
    RMScopedAccessImpl (RealtimeMutatable<T>& parent)
        : p (parent), currentValue (&p.realtimeAcquire()) {}
    ~RMScopedAccessImpl() { p.realtimeRelease(); }
public:
    T* get() noexcept                       { return currentValue;  }
    const T* get() const noexcept           { return currentValue;  }
    T &operator *() noexcept                { return *currentValue; }
    const T &operator *() const noexcept    { return *currentValue; }
    T* operator->() noexcept                { return currentValue; }
    const T* operator->() const noexcept    { return currentValue; }
private:
    RealtimeMutatable<T>& p;
    T* currentValue;
};

template <typename T>
class RMScopedAccessImpl<T, false>
{
protected:
    RMScopedAccessImpl (RealtimeMutatable<T>& parent) noexcept
        : p (parent), currentValue (&p.nonRealtimeAcquire()) {}
    ~RMScopedAccessImpl() noexcept { p.nonRealtimeRelease(); }
public:
    const T* get() const noexcept           { return currentValue;  }
    const T &operator *() const noexcept    { return *currentValue; }
    const T* operator->() const noexcept    { return currentValue; }
private:
    RealtimeMutatable<T>& p;
    const T* currentValue;
};
}

template <typename T>
template <bool isRealtimeThread>
RealtimeMutatable<T>::ScopedAccess<isRealtimeThread>::ScopedAccess (RealtimeMutatable<T>& parent)
    : detail::RMScopedAccessImpl<T, isRealtimeThread> (parent) {}
}