/* NetHack 3.6	uwputil.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

namespace Nethack
{
    const char kEscape = 27; /* '\033' */
    const char kDelete = 127;
    const char kNewline = '\n';
    const char kBackspace = '\b';
    const char kCarriageReturn = '\r';
    const char kTab = '\t';
    const char kControlC = '\003';
    const char kNull = '\0';
    const char kSpace = ' ';
    const char kControlP = '\020';
    const char kEOF = -1;


    enum class ScanCode
    {
        Unknown, Escape, One, Two, Three, Four, Five, Six, Seven, Eight, Nine, Zero,
        Minus, Equal, Backspace, Tab,
        Q, W, E, R, T, Y, U, I, O, P, LeftBracket, RightBracket,
        Enter, Control,
        A, S, D, F, G, H, J, K, L, SemiColon, Quote,
        BackQuote, LeftShift, BackSlash,
        Z, X, C, V, B, N, M, Comma, Period, ForwardSlash,
        RightShift, NotSure, Alt, Space, Caps,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10,
        Num, Scroll, Home, Up, PageUp, PadMinus, Left, Center, Right, PadPlus,
        End, Down, PageDown, Insert, Delete,
        Scan84, Scan85, Scan86,
        F11, F12, Count

    };

    enum class VirtualKey
    {
        Zero = 0x30, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
        A = 0x41, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
    };

    // Constant buffer used to send MVP matrices to the vertex shader.
    struct ModelViewProjectionConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionColor
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 foregroundColor;
        DirectX::XMFLOAT3 backgroundColor;
        DirectX::XMFLOAT2 coord;
    };

    template<typename T>
    class Vector2D
    {
    public:

        Vector2D() : m_x(0), m_y(0) {}
        Vector2D(const T & inX, const T & inY) : m_x(inX), m_y(inY) {}

        T m_x;
        T m_y;

        Vector2D & operator+=(const Vector2D & a) { m_x += a.m_x; m_y += a.m_y; return *this; }
        Vector2D & operator-=(const Vector2D & a) { m_x -= a.m_x; m_y -= a.m_y; return *this; }
        Vector2D & operator*=(const Vector2D & a) { m_x *= a.m_x; m_y *= a.m_y; return *this; }
        Vector2D & operator/=(const Vector2D & a) { m_x /= a.m_x; m_y /= a.m_y; return *this; }

        Vector2D & operator+=(T a) { m_x += a; m_y += a; return *this; }
        Vector2D & operator-=(T a) { m_x -= a; m_y -= a; return *this; }
        Vector2D & operator*=(T a) { m_x *= a; m_y *= a; return *this; }
        Vector2D & operator/=(T a) { m_x /= a; m_y /= a; return *this; }

    };

    template<typename T>  Vector2D<T> operator+(Vector2D<T> a, const Vector2D<T> & b) { return a += b; }
    template<typename T>  Vector2D<T> operator-(Vector2D<T> a, const Vector2D<T> & b) { return a -= b; }
    template<typename T>  Vector2D<T> operator*(Vector2D<T> a, const Vector2D<T> & b) { return a *= b; }
    template<typename T>  Vector2D<T> operator/(Vector2D<T> a, const Vector2D<T> & b) { return a /= b; }

    template<typename T>  Vector2D<T> operator+(Vector2D<T> a, T b) { return a += b; }
    template<typename T>  Vector2D<T> operator-(Vector2D<T> a, T b) { return a -= b; }
    template<typename T>  Vector2D<T> operator*(Vector2D<T> a, T b) { return a *= b; }
    template<typename T>  Vector2D<T> operator/(Vector2D<T> a, T b) { return a /= b; }

    typedef Vector2D<float> Float2D;
    typedef Vector2D<int> Int2D;

    inline Float2D operator+(const Float2D & a, const Int2D & b) { return Float2D(a.m_x + (float)b.m_x, a.m_y + (float)b.m_y); }
    inline Float2D operator-(const Float2D & a, const Int2D & b) { return Float2D(a.m_x - (float)b.m_x, a.m_y - (float)b.m_y); }
    inline Float2D operator*(const Float2D & a, const Int2D & b) { return Float2D(a.m_x * (float)b.m_x, a.m_y * (float)b.m_y); }
    inline Float2D operator/(const Float2D & a, const Int2D & b) { return Float2D(a.m_x / (float)b.m_x, a.m_y / (float)b.m_y); }

    template<typename T>
    class Rect
    {
    public:

        Rect() {}
        Rect(const T & inTopLeft, const T & inBottomRight)
            : m_topLeft(inTopLeft), m_bottomRight(inBottomRight)
        {

        }

        T m_topLeft;
        T m_bottomRight;
    };

    typedef Rect<Float2D> FloatRect;
    typedef Rect<Int2D> IntRect;

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

