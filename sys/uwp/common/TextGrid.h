#pragma once

#include "StepTimer.h"
#include "DeviceResources.h"
#include "MyMath.h"

#include <memory>
#include <vector>

namespace Nethack
{
    struct VertexPositionColor;

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
        White
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

        const TextCell & GetCell(int inX, int inY) const;
        TextCell & EditCell(int inX, int inY);

        void Render();
        void Update(DX::StepTimer const& timer);

        void CreateWindowSizeDependentResources(void);
        void CreateDeviceDependentResources(void);
        void ReleaseDeviceDependentResources(void);

        void SetCenterPosition(const Float2D  &inScreenPosition);
        Float2D GetCenterPosition(void) const { return m_gridScreenCenter;  }

        void SetWidth(float inScreenWidth);
        void SetTopLeft(const Float2D & inScreenTopLeft);
        void SetGridOffset(const Int2D & inGridOffset);

        void SetScale(float inScale);
        float GetScale(void) const { return m_scale;  }

        Float2D GetPixelScreenDimensions(void) const { return m_pixelScreenDimensions; }

        bool HitTest(const Float2D & inScreenPosition);
        bool HitTest(const Float2D & inScreenPixelPosition, Int2D & outGridPosition);

        FloatRect GetScreenRect(void) const { return m_screenRect; }

        void Clear(void);
        void Scroll(int amount);
        void Put(int x, int y, const TextCell & textCell, int len = 1);
        void Putstr(int x, int y, TextColor color, TextAttribute attribute, const char * text);

    private:

        void UpdateVertcies(void);
        void CalculateScreenValues(void);

        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        Int2D m_gridDimensions;

        int m_cellCount;

        std::vector<TextCell> m_cells;

        bool m_dirty;
        int m_vertexCount;
        VertexPositionColor * m_vertices;
        Int2D m_screenSize;
        FloatRect m_screenRect;
        Float2D m_cellScreenDimensions;
        Float2D m_gridScreenCenter;

        float m_scale;
        Int2D m_gridPixelDimensions;
        Float2D m_pixelScreenDimensions;
        Float2D m_gridScreenDimensions;

        Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;

        bool m_loadingComplete;

    };
}