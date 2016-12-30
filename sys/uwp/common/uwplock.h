#pragma once

#include <windows.h>
namespace Nethack
{

    class ConditionVariable;

    class Lock
    {
    public:

        Lock(void);
        ~Lock(void);

        void AcquireExclusive(void);
        void AcquireShared(void);
        void ReleaseExclusive(void);
        void ReleaseShared(void);

    private:

        friend class ConditionVariable;

        SRWLOCK m_lock;
    };

}
