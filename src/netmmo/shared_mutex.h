#ifndef SHARED_MUTEX_H
#define SHARED_MUTEX_H

#include <mutex>
#include <atomic>
#include <condition_variable>

namespace std
{
#define BLOCKING_OBJECT
    
#ifdef LOCK_FREE
    class shared_mutex
    {
    public:
        shared_mutex() : m_shared_number(0),
                         m_is_lock(false)
        {}
        
        void lock()
        {
            m_is_lock.store( true );
            if( m_shared_number.load() > 0 )
            {
                std::unique_lock<std::mutex> lk(mut);
                cond_var.wait(lk);
               
            }
            mut.lock();
        }
        
        bool try_lock()
        {
            return true;
        }
        
        void unlock()
        {
            m_is_lock.store( false );
            cond_var.notify_all();
            mut.unlock();
        }
        
        void lock_shared()
        {
            if( m_is_lock )
            {
                std::unique_lock<std::mutex> lk(mut);
                cond_var.wait(lk);
            }
            else
            {
                m_shared_number++;
                mut.try_lock();
            }
        }
        
        bool try_lock_shared()
        {
            return true;
        }
        
        void unlock_shared()
        {
            m_shared_number--;
            if( m_shared_number.load() < 1 )
            {
                cond_var.notify_all();
                mut.unlock();
            }
        }
        
    private:
        // Cколько раз вызван lock_shared()
        std::atomic<int> m_shared_number;
        
        // вызван ли lock(), если true, - блокировать вызовы lock_shared() 
        std::atomic<bool> m_is_lock;
        
        std::condition_variable cond_var;
        std::mutex mut;
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
            //no_writers.lock();
            no_readers.lock();
            //no_writers.unlock();
        }
        
        bool try_lock()
        {
            return true;
        }
        
        void unlock()
        {
            no_readers.unlock();
        }
        
        void lock_shared()
        {
            //no_writers.lock();
            counter_mutex.lock();
            int prev = nreaders;
            nreaders++;
            if( prev == 0 )
                no_readers.lock();
            counter_mutex.unlock();
            //no_writers.unlock();
        }
        
        bool try_lock_shared()
        {
            return true;
        }
        
        void unlock_shared()
        {
            counter_mutex.lock();
            nreaders--;
            int current = nreaders;
            if(current == 0)
                no_readers.unlock();
            counter_mutex.unlock();
        }
    private:
        std::mutex no_writers;
        std::mutex no_readers;
        std::mutex counter_mutex;
        
        int nreaders;
    };
#endif
    
}

#endif