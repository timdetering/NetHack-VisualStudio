/* NetHack 3.6	uwplock.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
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
