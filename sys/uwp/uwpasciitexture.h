#pragma once

#include "uwpfont.h"

namespace DX
{
    class DeviceResources;
}

namespace Nethack
{

    class AsciiTexture
    {
    public:

        AsciiTexture(DX::DeviceResources * deviceResources) : m_deviceResources(deviceResources) { }
        void Create(const std::string & fontName, DWRITE_FONT_WEIGHT weight);

        void GetGlyphRect(unsigned char c, Nethack::FloatRect & outRect) const;

        std::string                                         m_fontFamilyName;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_asciiTexture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_asciiTextureShaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>          m_asciiTextureSampler;
        Int2D                                               m_glyphPixels;
        float                                               m_underlinePosition;  // relative to top of alignment box in pixels
        float                                               m_underlineThickness; // in pixels

    private:

        DX::DeviceResources * m_deviceResources;

    };

}