#include "Skin.h"
#include "BinaryData.h"

namespace ui
{
const Palette& palette (SkinId s)
{
    static const Palette toxic { juce::Colour (0xff09090d), juce::Colour (0xe0121219), juce::Colour (0xff1c1c26), juce::Colour (0xff2e2e3c),
                                 juce::Colour (0xff39ff14), juce::Colour (0xff9b4dff), juce::Colour (0xffeeeef5), juce::Colour (0xff9494a8),
                                 juce::Colour (0xff17171e), juce::Colour (0xff2b2b37) };
    static const Palette noir { juce::Colour (0xff121110), juce::Colour (0xff1f1d1a), juce::Colour (0xff2a2722), juce::Colour (0xff3d3830),
                                juce::Colour (0xffe9a23b), juce::Colour (0xffc75b39), juce::Colour (0xffefe6d2), juce::Colour (0xffa69c86),
                                juce::Colour (0xff141312), juce::Colour (0xff3a352d) };
    static const Palette schem { juce::Colour (0xff070b18), juce::Colour (0xff0b1226), juce::Colour (0xff101a34), juce::Colour (0xff22345e),
                                 juce::Colour (0xff3de0ff), juce::Colour (0xffff3da6), juce::Colour (0xffd6e6ff), juce::Colour (0xff7d93c4),
                                 juce::Colour (0xff070b18), juce::Colour (0xff1a2848) };
    switch (s)
    {
        case SkinId::StudioNoir: return noir;
        case SkinId::Schematic: return schem;
        case SkinId::ToxicLab: break;
    }
    return toxic;
}

static juce::Typeface::Ptr load (const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
}

juce::Font plainFont (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font skinFont (SkinId s, FontRole role, float height)
{
    static auto orbitron = load (BinaryData::OrbitronExtraBold_ttf, BinaryData::OrbitronExtraBold_ttfSize);
    static auto rajSemi = load (BinaryData::RajdhaniSemiBold_ttf, BinaryData::RajdhaniSemiBold_ttfSize);
    static auto rajBold = load (BinaryData::RajdhaniBold_ttf, BinaryData::RajdhaniBold_ttfSize);
    static auto barlow = load (BinaryData::BarlowCondensedMedium_ttf, BinaryData::BarlowCondensedMedium_ttfSize);
    static auto cormorant = load (BinaryData::CormorantSemiBoldItalic_ttf, BinaryData::CormorantSemiBoldItalic_ttfSize);
    static auto mono = load (BinaryData::JetBrainsMonoMedium_ttf, BinaryData::JetBrainsMonoMedium_ttfSize);
    static auto grotesk = load (BinaryData::SpaceGroteskBold_ttf, BinaryData::SpaceGroteskBold_ttfSize);

    juce::Typeface::Ptr tf;
    switch (s)
    {
        case SkinId::ToxicLab:
            tf = role == FontRole::Title ? orbitron : (role == FontRole::Mono ? mono : (role == FontRole::Label ? rajBold : rajSemi));
            break;
        case SkinId::StudioNoir:
            tf = (role == FontRole::Title || role == FontRole::Serif) ? cormorant : (role == FontRole::Mono ? mono : barlow);
            break;
        case SkinId::Schematic:
            tf = role == FontRole::Title ? grotesk : mono;
            break;
    }
    if (tf == nullptr) return plainFont (height);
    return juce::Font (juce::FontOptions (tf).withHeight (height));
}

juce::PropertiesFile& settings()
{
    static juce::PropertiesFile file = []
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "ToxicRig";
        o.folderName = "ToxicZone";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        return juce::PropertiesFile (o);
    }();
    return file;
}
} // namespace ui
