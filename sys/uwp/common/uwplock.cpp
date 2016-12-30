#include "uwplock.h"

namespace Nethack
{

    Lock::Lock(void)
    {
        InitializeSRWLock(&m_lock);
    }

    Lock::~Lock(void)
    {
    }

    void Lock::AcquireExclusive(void)
    {
        AcquireSRWLockExclusive(&m_lock);
    }

    void Lock::AcquireShared(void)
    {
        AcquireSRWLockShared(&m_lock);
    }

    void Lock::ReleaseExclusive(void)
    {
        ReleaseSRWLockExclusive(&m_lock);
    }

    void Lock::ReleaseShared(void)
    {
        ReleaseSRWLockShared(&m_lock);
    }

}
