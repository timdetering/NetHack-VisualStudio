/* NetHack 3.6	uwputil.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

namespace Nethack
{
    inline std::string to_string(const std::wstring & inString)
    {
        std::string outString;
        outString.assign(inString.begin(), inString.end());
        return outString;
    }

    inline void to_wstring(const std::string & inString, std::wstring & outString)
    {
        outString.assign(inString.begin(), inString.end());
    }

    inline std::wstring to_wstring(const std::string & inString)
    {
        std::wstring outString;
        outString.assign(inString.begin(), inString.end());
        return outString;
    }

    inline void to_string(const std::wstring & inString, std::string & outString)
    {
        outString.assign(inString.begin(), inString.end());
    }

    inline Platform::String ^ to_platform_string(const std::string & inString)
    {
        std::wstring string;
        string.assign(inString.begin(), inString.end());
        return ref new Platform::String(string.c_str());
    }

    inline Platform::String ^ to_platform_string(const std::wstring & inString)
    {
        return ref new Platform::String(inString.c_str());
    }

    inline std::string to_string(Platform::String ^ inString)
    {
        std::wstring string(inString->Data());
        std::string outString;
        outString.assign(string.begin(), string.end());
        return outString;
    }

    inline std::wstring to_wstring(Platform::String ^ inString)
    {
        return std::wstring(inString->Data());
    }

    class Semaphore
    {
    public:

        Semaphore()
        {
            m_semaphore = ::CreateSemaphore(NULL, 0, 1, NULL);
            assert(m_semaphore != NULL);
        }

        ~Semaphore()
        {
            CloseHandle(m_semaphore);
        }

        void Signal()
        {
            ReleaseSemaphore(m_semaphore, 1, NULL);
        }

        void Wait()
        {
            DWORD hr = WaitForSingleObject(m_semaphore, INFINITE);
            assert(hr == WAIT_OBJECT_0);
        }

    private:

        HANDLE m_semaphore;

    };

    class ConditionVariable;

    class Lock
    {
    public:

        Lock(void)
        {
            InitializeSRWLock(&m_lock);
            m_ownerThreadId = 0;
        }
        ~Lock(void)
        {
            // do nothing
        }

        void AcquireExclusive(void)
        {
            assert(m_ownerThreadId != GetCurrentThreadId());
            AcquireSRWLockExclusive(&m_lock);
            m_ownerThreadId = GetCurrentThreadId();
        }

        void ReleaseExclusive(void)
        {
            assert(m_ownerThreadId == GetCurrentThreadId());
            m_ownerThreadId = 0;
            ReleaseSRWLockExclusive(&m_lock);
        }

        bool HasExclusive(void)
        {
            return (m_ownerThreadId == GetCurrentThreadId());
        }

    private:

        friend class ConditionVariable;

        SRWLOCK m_lock;
        DWORD m_ownerThreadId;
    };

    class ConditionVariable
    {
    public:

        ConditionVariable()
        {
            InitializeConditionVariable(&m_conditionVariable);
        }

        void Sleep(Lock & inLock)
        {
            inLock.m_ownerThreadId = 0;
            BOOL success = SleepConditionVariableSRW(&m_conditionVariable, &inLock.m_lock, INFINITE, 0);
            inLock.m_ownerThreadId = GetCurrentThreadId();
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

