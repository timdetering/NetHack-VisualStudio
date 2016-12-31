#pragma once

#include <list>

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

        void PushBack(const Event & inEvent)
        {
            m_lock.AcquireExclusive();
            m_list.push_back(inEvent);
            m_lock.ReleaseExclusive();
            m_conditionVariable.Wake();
        }

#if 0
        void PushFront(const Event & inEvent)
        {
            m_lock.AcquireExclusive();
            m_list.push_front(inEvent);
            m_lock.ReleaseExclusive();
            m_conditionVariable.Wake();
        }
#endif

        Event PopFront()
        {
            m_lock.AcquireExclusive();
            while (m_list.empty())
                m_conditionVariable.Sleep(m_lock);
            Event e = m_list.front();
            m_list.pop_front();
            m_lock.ReleaseExclusive();

            return e;
        }

        bool Empty()
        {
            m_lock.AcquireShared();
            bool empty = m_list.empty();
            m_lock.ReleaseShared();
            return empty;
        }

    private:
        
        std::list<Event>    m_list;
        Lock                m_lock;
        ConditionVariable   m_conditionVariable;

    };
    
}