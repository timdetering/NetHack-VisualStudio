#pragma once

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

#include <vector>
#include <map>

#include "uwpmath.h"

namespace Nethack
{
    class Font
    {
    public:

        Font() {}
        Font(const Microsoft::WRL::ComPtr<IDWriteFont3> & font, const std::string & name);

        bool                     m_monospaced;
        std::string              m_name;
        DWRITE_FONT_METRICS1     m_metrics;

        float                    m_lineSpacingEm;
        float                    m_underlineThicknessEm;
        float                    m_underlinePositionEm; // relative to top ov alignment box

        Float2D                  m_boxSizeEm;

        float                    m_maxBoxTopEm;
        float                    m_minBoxBottomEm;
        float                    m_maxBoxWidthEm;

        int                      m_maxBoxTop;       // design units relative to baseline
        int                      m_minBoxBottom;    // design units relative to bsaeline
        int                      m_maxBoxWidth;     // design units

        static const int         kGlyphCount = 3;

        UINT16                   m_glyphIndices[kGlyphCount];
        DWRITE_GLYPH_METRICS     m_glyphMetrics[kGlyphCount];

    private:

        Microsoft::WRL::ComPtr<IDWriteFont3>    m_font;
        Microsoft::WRL::ComPtr<IDWriteFontFace> m_fontFace;

        void CalculateMetrics();

    };

    struct IgnoreCase {
        bool operator() (const std::string& lhs, const std::string& rhs) const {
            return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };

    class FontFamily
    {
    public:

        FontFamily() {}
        FontFamily(const Microsoft::WRL::ComPtr<IDWriteFontFamily1> & fontFamily, const std::string & name);

        std::map<std::string, Font> m_fonts;
        std::string                                 m_name;

    private:

        Microsoft::WRL::ComPtr<IDWriteFontFamily1>	m_fontFamily;

    };

    class FontCollection
    {
    public:

        FontCollection();

        std::map<std::string, FontFamily, IgnoreCase> m_fontFamilies;

    private:

        Microsoft::WRL::ComPtr<IDWriteFactory3>		    m_dwriteFactory;
        Microsoft::WRL::ComPtr<IDWriteFontCollection1>	m_systemFontCollection;

    };

}
