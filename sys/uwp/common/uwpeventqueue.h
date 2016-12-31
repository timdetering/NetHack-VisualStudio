#pragma once

#include <list>

#include "uwplock.h"
#include "uwpconditionvariable.h"
#include "MyMath.h"

namespace Nethack
{
        
    class Event
    {
    public:

        enum class Type { Undefined, Char, Mouse };
        enum class Tap { Undefined, Left, Right };

        Event()
        {
            m_type = Type::Undefined;
            m_tap = Tap::Undefined;
            m_char = 0;
        }

        Event(char inChar)
        {
            m_type = Type::Char;
            m_tap = Tap::Undefined;
            m_char = inChar;
        }

        Event(const Int2D & pos, Tap tap = Tap::Left)
        {
            m_type = Type::Mouse;
            m_tap = tap;
            m_pos = pos;
            m_char = 0;
        }

        Type m_type;
        Tap m_tap;
        Int2D m_pos;
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