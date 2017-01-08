#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>

#include <agile.h>
#include <concrt.h>
#include <math.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <dwrite_2.h>
#include <wincodec.h>
#include <DirectXMath.h>
#include <agile.h>
#include <d3d11_3.h>
#include <dxgi1_4.h>
#include <d2d1_3.h>
#include <wrl.h>
#include <wrl/client.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>

#include "uwpasciitexture.h"

#include "common\DirectXHelper.h"
#include "common\DeviceResources.h"

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;

namespace Nethack
{
        
   void AsciiTexture::Create(
       const std::wstring & fontName, DWRITE_FONT_WEIGHT weight)
    {
        static const int glyphRowCount = 16;
        static const int glyphColumnCount = 16;

        static const int gutter = 1;

        // glyphs have a one pixel wide gutter

        const Int2D & glyphPixels = m_deviceResources->GetGlyphPixelDimensions();
        ID3D11Device3 * d3dDevice = m_deviceResources->GetD3DDevice();
                    
        int glyphWidth = glyphPixels.m_x + (2 * gutter);
        int glyphHeight = glyphPixels.m_y + (2 * gutter);

        int textureWidth = glyphWidth * glyphColumnCount;
        int textureHeight = glyphHeight * glyphRowCount;

        CD3D11_TEXTURE2D_DESC textureDesc(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            textureWidth,        // Width
            textureHeight,        // Height
            1,          // MipLevels
            1,          // ArraySize
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
        );

        DX::ThrowIfFailed(
            d3dDevice->CreateTexture2D(
                &textureDesc,
                nullptr,
                &m_asciiTexture
            )
        );

        CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc(
            m_asciiTexture.Get(),
            D3D11_SRV_DIMENSION_TEXTURE2D
        );

        DX::ThrowIfFailed(
            d3dDevice->CreateShaderResourceView(
                m_asciiTexture.Get(),
                &shaderResourceViewDesc,
                &m_asciiTextureShaderResourceView
            )
        );

        const float dxgiDpi = 96.0f;

        ID2D1DeviceContext2 * d2dContext = m_deviceResources->GetD2DDeviceContext();

        d2dContext->SetDpi(dxgiDpi, dxgiDpi);

        D2D1_BITMAP_PROPERTIES1 bitmapProperties =
            D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                dxgiDpi,
                dxgiDpi
            );

        ComPtr<ID2D1Bitmap1> cubeTextureTarget;
        ComPtr<IDXGISurface> cubeTextureSurface;
        DX::ThrowIfFailed(
            m_asciiTexture.As(&cubeTextureSurface)
        );

        DX::ThrowIfFailed(
            d2dContext->CreateBitmapFromDxgiSurface(
                cubeTextureSurface.Get(),
                &bitmapProperties,
                &cubeTextureTarget
            )
        );

        d2dContext->SetTarget(cubeTextureTarget.Get());

        ComPtr<ID2D1SolidColorBrush> whiteBrush;
        DX::ThrowIfFailed(
            d2dContext->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                &whiteBrush
            )
        );

        D2D1_SIZE_F renderTargetSize = d2dContext->GetSize();

        d2dContext->BeginDraw();

        d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());

        d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        ComPtr<IDWriteTextFormat> textFormat;
        IDWriteFactory3 * dwriteFactory = m_deviceResources->GetDWriteFactory();

        DX::ThrowIfFailed(
            dwriteFactory->CreateTextFormat(
                fontName.c_str(),
                nullptr,
                weight,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                72,
                L"en-US", // locale
                &textFormat
            )
        );

        HRESULT hr;

        // Center the text horizontally.
        DX::ThrowIfFailed(textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));

        // Center the text vertically.
        DX::ThrowIfFailed(
            textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
        );

        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                char c = (y * 16) + x;
                wchar_t wc;

                int result = MultiByteToWideChar(437, 0, &c, 1, &wc, 1);

                d2dContext->DrawText(
                    &wc,
                    1,
                    textFormat.Get(),
                    D2D1::RectF((float)((x * glyphWidth) + gutter), (float)((y * glyphHeight) + gutter),
                    (float)(((x + 1) * glyphWidth) - gutter), (float)(((y + 1) * glyphHeight) - gutter)),
                    whiteBrush.Get()
                );
            }
        }

        // We ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
        // is lost. It will be handled during the next call to Present.
        hr = d2dContext->EndDraw();
        if (hr != D2DERR_RECREATE_TARGET)
        {
            DX::ThrowIfFailed(hr);
        }

        D3D_FEATURE_LEVEL featureLevel = m_deviceResources->GetDeviceFeatureLevel();

        // create the sampler
        D3D11_SAMPLER_DESC samplerDescription;
        ZeroMemory(&samplerDescription, sizeof(D3D11_SAMPLER_DESC));
        samplerDescription.Filter = D3D11_FILTER_ANISOTROPIC;
        //    samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDescription.MipLODBias = 0.0f;
        samplerDescription.MaxAnisotropy = featureLevel > D3D_FEATURE_LEVEL_9_1 ? 4 : 2;
        samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDescription.BorderColor[0] = 0.0f;
        samplerDescription.BorderColor[1] = 0.0f;
        samplerDescription.BorderColor[2] = 0.0f;
        samplerDescription.BorderColor[3] = 0.0f;
        // allow use of all mip levels
        samplerDescription.MinLOD = 0;
        samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;

        DX::ThrowIfFailed(
            d3dDevice->CreateSamplerState(
                &samplerDescription,
                &m_asciiTextureSampler
            )
        );
    }

}