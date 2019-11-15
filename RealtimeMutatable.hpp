#pragma once

#include <atomic>
#include <memory>
#include <mutex>

namespace farbot
{
//==============================================================================
namespace detail { template <typename, bool> class RMScopedAccessImpl; }

//==============================================================================
/** Useful class to synchronise access to an object from multiple threads with the additional feature that one
 * designated thread will never wait to get access to the object (i.e. the realtime thread). */
template <typename T> class RealtimeMutatable
{
public:
    /** Creates a default constructed T */
    RealtimeMutatable();

    /** Creates a copy of T */
    explicit RealtimeMutatable (const T & obj);

    ~RealtimeMutatable();

    /** Create T by calling T's constructor which takes args */
    template <typename... Args>
    static RealtimeMutatable create(Args && ... args);

    //==============================================================================
    /** Returns a reference to T. Use this method on the real-time thread.
     *  It must be matched by realtimeRelease when you are finished using the 
     *  object. Alternatively, use the ScopedAccess helper class below.
     * 
     *  Only a single real-time thread can acquire this object at once!
     * 
     *  This method is wait- and lock-free.
     */
    T& realtimeAcquire() noexcept;

    /** Releases the lock on T previously acquired by realtimeAcquire.
     * 
     *  This method is wait- and lock-free.
     */
    void realtimeRelease() noexcept;

    //==============================================================================
    /** Returns a reference to T. Use this method on the non real-time thread.
     *  It must be matched by nonRealtimeRelease when you are finished using
     *  the object. Alternatively, use the ScopedAccess helper class below.
     * 
     *  Multiple non-realtime threads can acquire an object at the same time.
     * 
     *  This method uses a lock should not be used on a realtime thread.
     */ 
    const T& nonRealtimeAcquire();

    /** Releases the lock on T previously acquired by nonRealtimeAcquire.
     * 
     *  This method uses both a lock and a spin loop and should not be used
     *  on a realtime thread.
     */
    void nonRealtimeRelease();

    //==============================================================================
    /** Instead of calling acquire and release manually, you can also use this RAII
     *  version which calls acquire automatically on construction and release when
     *  destructed.
     */
    template <bool isRealtimeThread>
    class ScopedAccess    : public detail::RMScopedAccessImpl<T, isRealtimeThread>
    {
    public:
        explicit ScopedAccess (RealtimeMutatable& parent);

       #if DOXYGEN
        // Various ways to get access to the underlying object.
        // Non-const method are only supported on the realtime thread
        T* get() noexcept;
        const T* get() const noexcept;
        T &operator *() noexcept;
        const T &operator *() const noexcept;
        T* operator->() noexcept;
        const T* operator->() const noexcept;
       #endif

        //==============================================================================
        ScopedAccess(const ScopedAccess&) = delete;
        ScopedAccess(ScopedAccess &&) = delete;
        ScopedAccess& operator=(const ScopedAccess&) = delete;
        ScopedAccess& operator=(ScopedAccess&&) = delete;
    };
private:
    template <typename... Args>
    explicit RealtimeMutatable(bool, Args &&...);

    enum
    {
        INDEX_BIT = (1 << 0),
        BUSY_BIT = (1 << 1),
        NEWDATA_BIT = (1 << 2)
    };

    std::atomic<int> control = {0};

    std::array<T, 2> data;
    T realtimeCopy;

    std::mutex nonRealtimeLock;
};
}

//==============================================================================
#include "RealtimeMutatable.tcc"