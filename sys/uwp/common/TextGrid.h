/* NetHack 3.6	TextGrid.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "StepTimer.h"
#include "DeviceResources.h"
#include "MyMath.h"
#include "uwplock.h"
#include "..\content\ShaderStructures.h"

#include <memory>
#include <vector>

namespace Nethack
{

    enum class TextAttribute
    {
        None,
        Bold,
        Dim,
        Underline,
        Blink,
        Inverse
    };

    enum class TextColor
    {
        Black,
        Red,
        Green,
        Brown,
        Blue,
        Magenta,
        Cyan,
        Gray,
        Bright,
        Orange,
        BrightGreen,
        Yellow,
        BrightBlue,
        BrightMagenta,
        BrightCyan,
        White,
        Count
    };

    class TextCell
    {
    public:

        TextCell() : m_char(' '), m_attribute(TextAttribute::None), m_color(TextColor::Gray) {}
        TextCell(TextColor color, TextAttribute attribute, unsigned char c) :
            m_color(color), m_attribute(attribute), m_char(c) {}

        unsigned char   m_char;
        TextAttribute   m_attribute;
        TextColor       m_color;
    };

    //
    // TextGrid
    //
    // Class used to render text to te screen.
    //

    class TextGrid
    {
    public:

        TextGrid(const Int2D & inGridDimensions);
        ~TextGrid(void);

        void SetDeviceResources();

//        const TextCell & GetCell(int inX, int inY) const;
//        TextCell & EditCell(int inX, int inY);

        void Render();
        void Update(DX::StepTimer const& timer);

        void CreateWindowSizeDependentResources(void);
        void CreateDeviceDependentResources(void);
        void ReleaseDeviceDependentResources(void);

        void SetCenterPosition(const Float2D  &inScreenPosition);
        Float2D GetCenterPosition(void) const { return m_gridScreenCenter;  }

        void SetWidth(float inScreenWidth);
        void SetTopLeft(const Float2D & inScreenTopLeft);

        void SetGridOffset(const Int2D & inGridOffset); // In glyphs size
        void SetGridScreenPixelOffset(const Int2D & inGridOffset); // In screen pixels

        void SetScale(float inScale);
        float GetScale(void) const { return m_scale;  }

        Float2D GetPixelScreenDimensions(void) const { return m_pixelScreenDimensions; }

        bool HitTest(const Float2D & inScreenPosition);
        bool HitTest(const Float2D & inScreenPixelPosition, Int2D & outGridPosition);

        FloatRect GetScreenRect(void) const { return m_screenRect; }

        void Clear(void);
        void Scroll(int amount);

        void Put(TextColor color, TextAttribute attribute, char c);
        void Put(int x, int y, const TextCell & textCell, int len = 1);

        void Putstr(TextColor color, TextAttribute attribute, const char * text);
        void Putstr(int x, int y, TextColor color, TextAttribute attribute, const char * text);

        const Int2D & GetDimensions() { return m_gridDimensions; }

        void ScaleAndCenter(const Nethack::IntRect & inRect);

        void SetCursor(Int2D & cursor);


    private:

        void UpdateVertcies(void);
        void CalculateScreenValues(void);

        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        Int2D m_gridDimensions;

        int m_cellCount;

        Lock                  m_cellsLock;
        std::vector<TextCell> m_cells;

        static const int kCursorTicks = 550;
        Int2D   m_cursor;
        bool    m_cursorBlink;
        int64   m_cursorBlinkTicks;

        bool m_dirty;
        int m_vertexCount;
        VertexPositionColor * m_vertices;
        Int2D m_screenSize;
        FloatRect m_screenRect;
        Float2D m_cellScreenDimensions;
        Float2D m_gridScreenCenter;

        static const int kCursorVertexCount = 6;
        VertexPositionColor m_cursorVertices[kCursorVertexCount];

        float m_scale;
        Int2D m_gridPixelDimensions;
        Float2D m_pixelScreenDimensions;
        Float2D m_gridScreenDimensions;

        Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>		m_cursorVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_solidPixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11BlendState>	m_invertDstBlendState;

        bool m_loadingComplete;

    };
}