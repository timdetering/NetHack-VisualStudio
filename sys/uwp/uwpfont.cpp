#include "uwputil.h"
#include "uwpfont.h"

#include "common\DirectXHelper.h"
#include <string>
#include <vector> 

namespace Nethack
{


    Font::Font(const Microsoft::WRL::ComPtr<IDWriteFont3> & font, const std::string & name)
    {
        m_font = font;
        m_name = name;

        m_monospaced = (font->IsMonospacedFont() != 0);

        m_font->GetMetrics(&m_metrics);

        int designUnitsWidth = m_metrics.glyphBoxRight - m_metrics.glyphBoxLeft;
        int designUnitsHeight = m_metrics.glyphBoxTop - m_metrics.glyphBoxBottom;

        m_emSize.m_x = (float)m_metrics.designUnitsPerEm / (float)designUnitsWidth;
        m_emSize.m_y = (float)m_metrics.designUnitsPerEm / (float)designUnitsHeight;

        m_widthToHeight = m_emSize.m_y / m_emSize.m_x;
    }

    FontFamily::FontFamily(const Microsoft::WRL::ComPtr<IDWriteFontFamily1> & fontFamily, const std::string & name)
    {
        m_fontFamily = fontFamily;
        m_name = name;

        int fontCount = fontFamily->GetFontCount();

        for (int fontIndex = 0; fontIndex < fontCount; fontIndex++)
        {
            Microsoft::WRL::ComPtr<IDWriteFont3>	font;
            DX::ThrowIfFailed(fontFamily->GetFont(fontIndex, &font));

            Microsoft::WRL::ComPtr<IDWriteLocalizedStrings>	names;
            DX::ThrowIfFailed(font->GetFaceNames(&names));

            UINT32 nameIndex;
            BOOL exists;
            DX::ThrowIfFailed(names->FindLocaleName(L"en-us", &nameIndex, &exists));

            if (exists)
            {
                UINT32 length;
                DX::ThrowIfFailed(names->GetStringLength(nameIndex, &length));

                wchar_t * wname = new wchar_t[length + 1];
                DX::ThrowIfFailed(names->GetString(nameIndex, wname, length + 1));
                
                std::string name = Nethack::to_string(std::wstring(wname));
                m_fonts[name] = Font(font, name);
            }
        }
    }

    FontCollection::FontCollection(void)
    {

        // Initialize the DirectWrite Factory.
        DX::ThrowIfFailed(
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory3),
                &m_dwriteFactory
            )
        );

        DX::ThrowIfFailed(m_dwriteFactory->GetSystemFontCollection(false, &m_systemFontCollection));

        int familyCount = m_systemFontCollection->GetFontFamilyCount();
        for (int familyIndex = 0; familyIndex < familyCount; familyIndex++)
        {
            Microsoft::WRL::ComPtr<IDWriteFontFamily1>	fontFamily;
            DX::ThrowIfFailed(m_systemFontCollection->GetFontFamily(familyIndex, &fontFamily));

            Microsoft::WRL::ComPtr<IDWriteLocalizedStrings>	familyNames;
            DX::ThrowIfFailed(fontFamily->GetFamilyNames(&familyNames));

            UINT32 nameIndex;
            BOOL exists;
            DX::ThrowIfFailed(familyNames->FindLocaleName(L"en-us", &nameIndex, &exists));

            if (exists)
            {
                UINT32 familyNameLength;
                DX::ThrowIfFailed(familyNames->GetStringLength(nameIndex, &familyNameLength));

                wchar_t * wfamilyName = new wchar_t[familyNameLength + 1];
                DX::ThrowIfFailed(familyNames->GetString(nameIndex, wfamilyName, familyNameLength + 1));

                std::string familyName = Nethack::to_string(std::wstring(wfamilyName));
                m_fontFamilies[familyName] = FontFamily(fontFamily, familyName);
            }
        }
    }
}
