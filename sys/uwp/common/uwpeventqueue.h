#pragma once

#include <queue>

#include "uwplock.h"
#include "uwpconditionvariable.h"

namespace Nethack
{
        
    class Event
    {
    public:

        enum class Type { Char };

        Event(char inChar)
        {
            m_type = Type::Char;
            m_char = inChar;
        }

        Type m_type;
        int m_x;
        int m_y;
        char m_char;

    };

    class EventQueue
    {
    public:

        EventQueue()
        {
            // do nothing
        }

        ~EventQueue()
        {
            // do nothing
        }

        void Push(const Event & inEvent)
        {
            m_lock.AcquireExclusive();
            m_queue.push(inEvent);
            m_lock.ReleaseExclusive();
            m_conditionVariable.Wake();
        }

#if 0
        Event Front()
        {
            m_lock.AcquireShared();
            Event e = m_queue.front();
            m_lock.ReleaseShared();
            return e;
        }
#endif

        Event Pop()
        {
            m_lock.AcquireExclusive();
            while (m_queue.empty())
                m_conditionVariable.Sleep(m_lock);
            Event e = m_queue.front();
            m_queue.pop();
            m_lock.ReleaseExclusive();

            return e;
        }

#if 0
        bool Empty()
        {
            m_lock.AcquireShared();
            bool empty = m_queue.empty();
            m_lock.ReleaseShared();
            return empty;
        }
#endif

    private:
        
        std::queue<Event>   m_queue;
        Lock                m_lock;
        ConditionVariable   m_conditionVariable;

    };
    
}