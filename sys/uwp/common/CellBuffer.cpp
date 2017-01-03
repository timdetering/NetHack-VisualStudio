/* NetHack 3.6	CellBuffer.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

#include "CellBuffer.h"

#include <Windows.h>

CellAttribute::CellAttribute(void)
{
    Set(ConsoleColor::Gray, ConsoleAttribute::None);
}

CellAttribute::CellAttribute(ConsoleColor cellColor, ConsoleAttribute cellAttribute)
{
    Set(cellColor, cellAttribute);
}

void CellAttribute::Set(ConsoleColor cellColor, ConsoleAttribute cellAttribute)
{
    m_color = (char) cellColor;
    m_attribute = (char) cellAttribute;
}

CellBuffer::CellBuffer(int width, int height) : m_width(width), m_height(height), m_area(width * height)
{
    m_cellAttributes.resize(m_area, CellAttribute());
    m_text.resize(m_area, ' ');
}

CellBuffer::~CellBuffer()
{
    // do nothing
}

void CellBuffer::Clear(void)
{
    m_cellAttributes.assign(m_area, CellAttribute());
    m_text.assign(m_area, ' ');
}

void CellBuffer::Scroll(int amount)
{
    for (int y = 0; y < m_height - amount; y++)
    {
        ::memcpy(&m_text[y * m_width], &m_text[(y + amount) * m_width], m_width);
        ::memcpy(&m_cellAttributes[y * m_width], &m_cellAttributes[(y + amount) * m_width], m_width * sizeof(wchar_t));
    }

    ::memset(&m_text[(m_height - amount) * m_width], ' ', amount * m_width);
    ::wmemset((wchar_t *) &m_cellAttributes[(m_height - amount) * m_width], CellAttribute(), amount * m_width);
}

char * CellBuffer::GetText(int x, int y)
{
    assert(x < m_width);
    assert(y < m_height);

    return &m_text[(y * m_width) + x];
}

char CellBuffer::GetChar(int x, int y)
{
    assert(x < m_width);
    assert(y < m_height);

    return m_text[(y * m_width) + x];
}

CellAttribute * CellBuffer::GetCellAttributes(int x, int y)
{
    assert(x < m_width);
    assert(y < m_height);

    return &m_cellAttributes[(y * m_width) + x];
}

CellAttribute CellBuffer::GetCellAttribute(int x, int y)
{
    return m_cellAttributes[(y * m_width) + x];
}

void CellBuffer::Putstr(int x, int y, CellAttribute cellAttribute, const char * text)
{
    int len = strlen(text);
    assert(len + x <= m_width);

    ::memcpy(GetText(x, y), text, len);
    ::wmemset((wchar_t *) GetCellAttributes(x, y), cellAttribute, len);
}

void CellBuffer::Put(int x, int y, CellAttribute cell, char c, int len)
{
    assert(x < m_width);
    assert(y < m_height);

    memset(&m_text[(y * m_width) + x], c, len);
    wmemset((wchar_t *) &m_cellAttributes[(y * m_width) + x], cell, len);
}


void CellBuffer::Write(CellBuffer * cellBuffer, int xOffset, int yOffset, int width, int height)
{
    assert(width + xOffset <= m_width);
    assert(height + yOffset <= m_height);

    for (int y = 0; y < height; y++)
    {
        ::memcpy(GetText(xOffset, y + yOffset), cellBuffer->GetText(0, y), width);
        ::memcpy(GetCellAttributes(xOffset, y + yOffset), cellBuffer->GetCellAttributes(0, y), width * sizeof(wchar_t));
    }
}
