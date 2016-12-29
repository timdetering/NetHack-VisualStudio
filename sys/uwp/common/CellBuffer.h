#pragma once

#include <assert.h>
#include <memory.h>
#include <wchar.h>

#include <vector>

enum class ConsoleAttribute
{
    None = 0,
    Bold = 1,
    Dim = 2,
    Underline = 4,
    Blink = 5,
    Inverse = 7
};

enum class ConsoleColor
{
    Black,
    Red,
    Green,
    Brown,
    Blue,
    Magenta,
    Cyan,
    Gray,
    NoColor,
    Orange,
    BrightGreen,
    Yellow,
    BrightBlue,
    BrightMagenta,
    BrightCyan,
    White
};

class CellAttribute
{
public:

    CellAttribute();
    CellAttribute(ConsoleColor color, ConsoleAttribute attribute);

    operator ConsoleColor() { return (ConsoleColor) m_color; }
    operator ConsoleAttribute() { return (ConsoleAttribute) m_attribute; }

    operator wchar_t() { return m_data; }

    bool operator ==(const CellAttribute & a) const { return a.m_data == m_data; }
    bool operator !=(const CellAttribute & a) const { return a.m_data != m_data; }

private:

    void Set(ConsoleColor color, ConsoleAttribute attribute);

    union
    {
        struct
        {
            char m_color;
            char m_attribute;
        };

        wchar_t  m_data;
    };
};

class CellBuffer {
public:

    CellBuffer(int width = 80, int height = 28);
    ~CellBuffer();

    void Write(CellBuffer * cellBuffer, int xOffset, int yOffset, int width, int height);

    void Clear(void);
    void Scroll(int amount);
    void Putstr(int x, int y, CellAttribute cell, const char * text);
    void Put(int x, int y, CellAttribute cell, char c, int len = 1);

    char * GetText(int x, int y);
    char GetChar(int x, int y);

    CellAttribute * GetCellAttributes(int x, int y);
    CellAttribute GetCellAttribute(int x, int y);

    int GetWidth() { return m_width; }
    int GetHeight() { return m_height; }

private:

    int             m_width;
    int             m_height;
    int             m_area;

    std::vector<CellAttribute>  m_cellAttributes;
    std::vector<char>           m_text;

};
