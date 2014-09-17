/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the logic portion of an IMGUI system. Rendering of the
/// controls is left up to the application. Currently, only BMFont binary font
/// data is supported for text rendering.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LLGUI_HPP_INCLUDED
#define LLGUI_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <stddef.h>

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Abstract away Windows 32-bit calling conventions and visibility.
#if defined(_WIN32) && defined(_MSC_VER)
    #define  LLGUI_CALL_C    __cdecl
    #if   defined(_MSC_VER)
    #define  LLGUI_IMPORT    __declspec(dllimport)
    #define  LLGUI_EXPORT    __declspec(dllexport)
    #elif defined(__GNUC__)
    #define  LLGUI_IMPORT    __attribute__((dllimport))
    #define  LLGUI_EXPORT    __attribute__((dllexport))
    #else
    #define  LLGUI_IMPORT
    #define  LLGUI_EXPORT
    #endif
#else
    #define  LLGUI_CALL_C
    #if __GNUC__ >= 4
    #define  LLGUI_IMPORT    __attribute__((visibility("default")))
    #define  LLGUI_EXPORT    __attribute__((visibility("default")))
    #endif
#endif

/// @summary Define import/export based on whether we're being used as a DLL.
#if defined(LLGUI_SHARED)
    #ifdef  LLGUI_EXPORTS
    #define LLGUI_PUBLIC     LLGUI_EXPORT
    #else
    #define LLGUI_PUBLIC     LLGUI_IMPORT
    #endif
#else
    #define LLGUI_PUBLIC
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace gui {

/*/////////////////
//   Constants   //
/////////////////*/

/*/////////////////
//   Data Types  //
/////////////////*/
/// @summary Defines the data associated with a single glyph in a bitmap font.
/// This structure is almost identical to bmfont_char_t, but without the packing restrictions.
struct bitmap_glyph_t
{
    uint32_t        Codepoint;  /// The Unicode codepoint associated with the glyph.
    uint16_t        TextureX;   /// X-coordinate of the upper-left corner of the glyph.
    uint16_t        TextureY;   /// Y-coordinate of the upper-left corner of the glyph.
    uint16_t        Width;      /// Width of the glyph on the texture, in pixels.
    uint16_t        Height;     /// Height of the glyph on the texture, in pixels.
    uint16_t        OffsetX;    /// Horizontal offset when copying the glyph to the screen.
    uint16_t        OffsetY;    /// Vertical offset when copying the glyph to the screen.
    uint16_t        AdvanceX;   /// How much to advance the current position.
    uint8_t         PageIndex;  /// The index of the page containing the glyph data.
};

/// @summary Defines the data associated with a bitmap font. This data is
/// typically loaded from a BMFont binary file.
struct bitmap_font_t
{
    size_t          GlyphCount; /// The number of glyphs defined in the font.
    size_t        **GTable;     /// Table mapping Unicode codepoint -> index in Glyphs.
    bitmap_glyph_t *Glyphs;     /// List of glyph definitions.
    size_t          KernCount;  /// The number of entries in the kerning table.
    uint32_t       *KerningA;   /// Codepoints of the first glyph in a kerning pair.
    uint32_t       *KerningB;   /// Codepoints of the second glyph in a kerning pair.
    int32_t        *KerningX;   /// Horizontal advance for a kerning pair.
    size_t          BitDepth;   /// Number of bits-per-pixel in a page image, either 8 or 32.
    size_t          PageWidth;  /// Width of a single page image, in pixels.
    size_t          PageHeight; /// Height of a single page image, in pixels.
    size_t          PageBytes;  /// The number of bytes in a single page image.
    size_t          PageCount;  /// The number of pages defined in the font.
    void           *PageData;   /// The raw page data in either R8 or ABGR format.
    char           *FontName;   /// The name of the font.
    size_t          PointSize;  /// The font size, in points.
    size_t          LineHeight; /// The number of vertical pixels between two lines.
    size_t          Baseline;   /// The number of pixels from the top of a line to the common base.
    size_t          MinWidth;   /// The smallest width of any glyph in the font.
    size_t          MaxWidth;   /// The largest width of any glyph in the font.
    float           AvgWidth;   /// The average width of a glyph in the font.
};

/*////////////////
//   Functions  //
////////////////*/

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace gui */

#endif /* !defined(LLGUI_HPP_INCLUDED) */

