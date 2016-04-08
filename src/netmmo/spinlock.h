#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>

namespace lf
{
    class spinlock
    {
    public:
        spinlock(): m_bit(true)
        {}
        
        bool lock()
        {
            return m_bit.exchange(false);
        }
        
        void unlock()
        {
            m_bit.store(true);
        }
        
        bool is_locked() const
        {
            return m_bit.load();
        }
    private:
        std::atomic<bool> m_bit;
    };
}

#endif