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
        void Create(const std::wstring & fontName, DWRITE_FONT_WEIGHT weight);

        Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_asciiTexture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_asciiTextureShaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>          m_asciiTextureSampler;

    private:

        DX::DeviceResources * m_deviceResources;

    };

}