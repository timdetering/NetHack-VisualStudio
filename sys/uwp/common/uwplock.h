/* NetHack 3.6	uwplock.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
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
