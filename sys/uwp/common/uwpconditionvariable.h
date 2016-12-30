#pragma once

#include "uwplock.h"
#include <assert.h>

namespace Nethack
{

    class ConditionVariable
    {
    public:

        ConditionVariable()
        {
            InitializeConditionVariable(&m_conditionVariable);
        }

        void Sleep(Lock & inLock)
        {
            BOOL success = SleepConditionVariableSRW(&m_conditionVariable, &inLock.m_lock, INFINITE, 0);
            assert(success);
        }

        void Wake()
        {
            WakeConditionVariable(&m_conditionVariable);
        }

    private:

        CONDITION_VARIABLE m_conditionVariable;
    };

}
