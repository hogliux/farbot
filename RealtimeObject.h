#pragma once

#include <atomic>
#include <memory>
#include <mutex>

//==============================================================================
/** Useful class to synchronise access to an object from multiple threads with the additional feature that one
 * designated thread will never wait to get access to the object (i.e. the realtime thread). */
template <typename T> class RealtimeObject
{
public:
    /** Creates a default constructed T */
    RealtimeObject();

    /** Creates a copy of T */
    explicit RealtimeObject (const T & obj);

    /** Moves T into this realtime wrapper */
    explicit RealtimeObject (T && obj);

    ~RealtimeObject();

    /** Create T by calling T's constructor which takes args */
    template <typename... Args>
    static RealtimeObject create(Args... args);

    /** Returns a pointer to T. If this thread is not the realtime thread
     *  then this operation will block until no other thread is using T.
     *  If the realtime thread calls this method, then T (or a copy) will
     *  be returned without blocking.
     */
    T* acquire (bool isRealTimeThread);

    /** Releases the lock on T previously acquired by acquire */
    void release (T* obj);

    //==============================================================================
    /** Instead of calling acquire and release manually, you can also use this RAII
     *  version which calls acquire automatically on construction and release when
     *  destructed.
     */
    class ScopedAccess
    {
    public:
        explicit ScopedAccess (RealtimeObject& parent, bool isRealtimeThread = false);
        ~ScopedAccess();

        // Various ways to get access to the underlying object
        T* get() noexcept;
        const T* get() const noexcept;
        T &operator *() noexcept;
        const T &operator *() const noexcept;
        T* operator->() noexcept;
        const T* operator->() const noexcept;
    private:
        RealtimeObject& p;
        T* currentValue;

    };
private:
    explicit RealtimeObject(std::unique_ptr<T> && u);

    std::unique_ptr<T> storage;
    std::atomic<T*> pointer;

    std::mutex nonRealtimeLock;
    std::unique_ptr<T> copy;
};

//==============================================================================
#include "RealtimeObject.tcc"