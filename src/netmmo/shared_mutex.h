#ifndef SHARED_MUTEX_H
#define SHARED_MUTEX_H

#include <mutex>
#include <atomic>
#include <condition_variable>

#include "spinlock.h"

namespace std
{
#define BLOCKING_OBJECT_IMPROVED
    
#ifdef LOCK_FREE
    class shared_mutex
    {
    public:
        shared_mutex() : nreaders(0),
                         iswait(false)
        {}
        
        void lock()
        {
            sp_wr.lock();
            mut.lock();
        }
        
        bool try_lock()
        {
            return true;
        }
        
        void unlock()
        {
            if( iswait.exchange(false) )
                cond_var.notify_all();
            sp_wr.unlock();
            mut.unlock();
        }
        
        void lock_shared()
        {
            if( sp_wr.lock() )
            {
                nreaders++;
                if( nreaders == 1 )
                    mut.lock();
                sp_wr.unlock();
            }
            else
            {
                iswait.store(true);
                std::unique_lock<std::mutex> lk( notify );
                cond_var.wait(lk);
            }
        }
        
        bool try_lock_shared()
        {
            return true;
        }
        
        void unlock_shared()
        {
            if( sp_wr.lock() )
            {
                nreaders--;
                sp_wr.unlock();
                mut.unlock();
            }
        }
        
    private:
        std::mutex mut;
        std::mutex notify;
        lf::spinlock sp_wr;
        std::condition_variable cond_var;
        int nreaders;
        std::atomic<bool> iswait;
    };
#endif
    
    
    
#ifdef BLOCKING_OBJECT
    class shared_mutex
    {
    public:
        shared_mutex(): nreaders(0)
        {}
        
        void lock()
        {
            no_writers.lock();
            no_readers.lock();
            no_writers.unlock();
        }
        
        bool try_lock()
        {
            bool result = false;
            if( no_writers.try_lock() )
            {
                result = no_readers.try_lock();
                no_writers.unlock();
            }
            
            return result;
        }
        
        void unlock()
        {
            no_readers.unlock();
        }
        
        void lock_shared()
        {
            no_writers.lock();
            counter_mutex.lock();
            int prev = nreaders; // with local variable caching it works with best performance; Why?
            nreaders++;
            if( prev == 0 )
                no_readers.lock();
            counter_mutex.unlock();
            no_writers.unlock();
        }
        
        bool try_lock_shared()
        {
            bool result = false;
            
            if( no_writers.try_lock() )
            {
                if( counter_mutex.try_lock() )
                {
                    int prev = nreaders; // with local variable caching it works with best performance; Why?
                    nreaders++;
                    if( prev == 0 )
                        result = no_readers.try_lock();
                    else
                        result = true;
                    counter_mutex.unlock();
                }
                no_writers.unlock();
            }
            
            return result;
        }
        
        void unlock_shared()
        {
            counter_mutex.lock();
            nreaders--;
            int current = nreaders; // with local variable cache it works with best performance; How?
            if(current == 0)
                no_readers.unlock();
            counter_mutex.unlock();
        }
    private:
        std::mutex no_writers;      // if writer come in, no one new reader allowed, long reading non-actual data is absurd
        std::mutex no_readers;      // common mutex
        std::mutex counter_mutex;   // mutex for counter and counter check
        
        int nreaders;
    };
#endif
    
    
    
#ifdef BLOCKING_OBJECT_IMPROVED
    class shared_mutex
    {
    public:
        shared_mutex(): iswrite(false),
                        nreaders(0)
        {}
        
        void lock()
        {
            no_writers.lock();
            iswrite.store(true);
            no_readers.lock();
            no_writers.unlock();
        }
        
        bool try_lock()
        {
            bool result = false;
            if( no_writers.try_lock() )
            {
                result = no_readers.try_lock();
                no_writers.unlock();
            }
            
            return result;
        }
        
        void unlock()
        {
            no_readers.unlock();
            iswrite.store(false);
        }
        
        void lock_shared()
        {
            if( iswrite.load() )
            {
                no_writers.lock();
                counter_mutex.lock();
                int prev = nreaders; // with local variable caching it works with best performance; Why?
                nreaders++;
                if( prev == 0 )
                    no_readers.lock();
                counter_mutex.unlock();
                no_writers.unlock();
            }
            else
            {
                counter_mutex.lock();
                int prev = nreaders; // with local variable caching it works with best performance; Why?
                nreaders++;
                if( prev == 0 )
                    no_readers.lock();
                counter_mutex.unlock();
            }
        }
        
        bool try_lock_shared()
        {
            bool result = false;
            
            if( no_writers.try_lock() )
            {
                if( counter_mutex.try_lock() )
                {
                    int prev = nreaders; // with local variable caching it works with best performance; Why?
                    nreaders++;
                    if( prev == 0 )
                        result = no_readers.try_lock();
                    else
                        result = true;
                    counter_mutex.unlock();
                }
                no_writers.unlock();
            }
            
            return result;
        }
        
        void unlock_shared()
        {
            counter_mutex.lock();
            nreaders--;
            int current = nreaders; // with local variable cache it works with best performance; How?
            if(current == 0)
                no_readers.unlock();
            counter_mutex.unlock();
        }
    private:
        std::mutex no_writers;      // if writer come in, no one new reader allowed, long reading non-actual data is absurd
        std::mutex no_readers;      // common mutex
        std::mutex counter_mutex;   // mutex for counter and counter check
        std::atomic<bool> iswrite;
        
        int nreaders;
    };
#endif
    
}

#endif