/* NetHack 3.6	uwplock.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#include "uwplock.h"

#include <assert.h>

namespace Nethack
{

    Lock::Lock(void)
    {
        InitializeSRWLock(&m_lock);
        m_ownerThreadId = 0;
    }

    Lock::~Lock(void)
    {
    }

    void Lock::AcquireExclusive(void)
    {
        assert(m_ownerThreadId != GetCurrentThreadId());
        AcquireSRWLockExclusive(&m_lock);
        m_ownerThreadId = GetCurrentThreadId();
    }

#if 0
    void Lock::AcquireShared(void)
    {
        AcquireSRWLockShared(&m_lock);
    }
#endif

    void Lock::ReleaseExclusive(void)
    {
        assert(m_ownerThreadId == GetCurrentThreadId());
        m_ownerThreadId = 0;
        ReleaseSRWLockExclusive(&m_lock);
    }

#if 0
    void Lock::ReleaseShared(void)
    {
        ReleaseSRWLockShared(&m_lock);
    }
#endif

    bool Lock::HasExclusive(void)
    {
        return (m_ownerThreadId == GetCurrentThreadId());
    }

}
