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
/// @summary Abstract away Windows 32-bit calling conventions.
#if defined(_WIN32) && defined(_MSC_VER)
    #define  LLGUI_CALL_C    __cdecl
#else
    #define  LLGUI_CALL_C
#endif

/// @summary Abstract away Windows DLL import/export.
#if defined(_MSC_VER)
    #define  LLGUI_IMPORT    __declspec(dllimport)
    #define  LLGUI_EXPORT    __declspec(dllexport)
#else
    #define  LLGUI_IMPORT
    #define  LLGUI_EXPORT
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
/// @summary Defines the fields present on the main file header.
#pragma pack(push, 1)
struct bmfont_header_t
{
    char Magic[3];            /// The characters 'BMF'.
    char Version;             /// The file format version. Currently, 3.
};
#pragma pack(pop)

/// @summary Stores metadata relating to a single block within a BMFont binary file.
#pragma pack(push, 1)
struct bmfont_block_header_t
{
    char     Id;              /// The block type identifier.
    uint32_t DataSize;        /// The size of the block data, in bytes.
};
#pragma pack(pop)

/// @summary The data associated with an INFO block in a BMFont file. Most of
/// this information can be safely ignored and is used at design-time.
#pragma pack(push, 1)
struct bmfont_info_block_t
{
    int16_t  FontSize;        /// The font size, in points.
    uint8_t  Attributes;      /// Bit0 = smooth, Bit1=Unicode, Bit2=Italic, Bit3=Bold, Bit4=Fixed Height
    uint8_t  CharSet;         /// The name of the OEM charset (when not Unicode).
    uint16_t StretchH;        /// The font height stretch percentage; 100 = none.
    uint8_t  AA;              /// The supersampling level used, 1 = none.
    uint8_t  PaddingTop;      /// The padding for each character, top side.
    uint8_t  PaddingRight;    /// The padding for each character, right side.
    uint8_t  PaddingBottom;   /// The padding for each character, bottom side.
    uint8_t  PaddingLeft;     /// The padding for each character, left side.
    uint8_t  SpacingX;        /// The horizontal spacing for each character.
    uint8_t  SpacingY;        /// The vertical spacing for each character.
    uint8_t  Outline;         /// The outline thickness for the glyphs.
    char     FontName[1];     /// NULL-terminated ASCII font name string, variable.
};
#pragma pack(pop)

/// @summary The data associated with a COMMON block in a BMFont file.
#pragma pack(push, 1)
struct bmfont_common_block_t
{
    uint16_t LineHeight;      /// The distance between each line of text, in pixels.
    uint16_t BaseLine;        /// # of pixels from absolute top of line to glyph base.
    uint16_t ScaleWidth;      /// Width of a texture page, in pixels.
    uint16_t ScaleHeight;     /// Height of a texture page, in pixels.
    uint16_t PageCount;       /// The number of texture pages in the font.
    uint8_t  Attributes;      /// 
    uint8_t  AlphaChannel;    /// 0 = glyph, 1 = outline, 2 = glyph+outline, 3 = zero, 4 = one
    uint8_t  RedChannel;      /// 0 = glyph, 1 = outline, 2 = glyph+outline, 3 = zero, 4 = one
    uint8_t  GreenChannel;    /// 0 = glyph, 1 = outline, 2 = glyph+outline, 3 = zero, 4 = one
    uint8_t  BlueChannel;     /// 0 = glyph, 1 = outline, 2 = glyph+outline, 3 = zero, 4 = one
};
#pragma pack(pop)

/// @summary The data associated with a PAGES block in a BMFont file. This 
/// block is essentially just a blob of character data. There is one NULL-
/// terminated string for each page, and bmfont_common_block_t::PageCount pages.
/// Each string is the same length.
#pragma pack(push, 1)
struct bmfont_pages_block_t
{
    char     PageNames[1];    /// Array of same-length NULL-terminated strings.
};
#pragma pack(pop)

/// @summary Describes a single glyph within a texture page.
#pragma pack(push, 1)
struct bmfont_char_t
{
    uint32_t Codepoint;       /// The Unicode codepoint associated with the glyph.
    uint16_t TextureX;        /// X-coordinate of the upper-left corner of the glyph.
    uint16_t TextureY;        /// Y-coordinate of the upper-left corner of the glyph.
    uint16_t Width;           /// Width of the glyph on the texture, in pixels.
    uint16_t Height;          /// Height of the glyph on the texture, in pixels.
    uint16_t OffsetX;         /// Horizontal offset when copying the glyph to the screen.
    uint16_t OffsetY;         /// Vertical offset when copying the glyph to the screen.
    uint16_t AdvanceX;        /// How much to advance the current position.
    uint8_t  PageIndex;       /// The index of the page containing the glyph data.
    uint8_t  Channel;         /// The texture channel where the glyph data is found.
};
#pragma pack(pop)

/// @summary The data associated with a CHARS block in a BMFont file. This 
/// block is just a blob of bmfont_char_t instances, tightly packed. The number
/// of characters is bmfont_block_header_t::DataSize / sizeof(bmfont_char_t).
#pragma pack(push, 1)
struct bmfont_chars_block_t
{
    bmfont_char_t Char[1];    /// Array of bmfont_char_t.
};
#pragma pack(pop)

/// @summary Describes a kerning pair, which controls the spacing between a 
/// specific pair of glyphs. Not every glyph pair will have custom kerning.
#pragma pack(push, 1)
struct bmfont_kerning_t
{
    uint32_t A;               /// The codepoint of the first glyph.
    uint32_t B;               /// The codepoint of the second glyph.
    int32_t  AdvanceX;        /// The amount to advance the current position when drawing the glyph pair.
};
#pragma pack(pop)

/// @summary The data associated with a KERNING block in a BMFont file. This 
/// block is just a blob of bmfont_kerning_t instances, tightly packed. The 
/// number of pairs is bmfont_block_header_t::DataSize / sizeof(bmfont_kerning_t).
#pragma pack(push, 1)
struct bmfont_kerning_block_t
{
    bmfont_kerning_t Pair[1]; /// Array of bmfont_kerning_t.
};
#pragma pack(pop)

/*////////////////
//   Functions  //
////////////////*/

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace gui */

#endif /* !defined(LLGUI_HPP_INCLUDED) */

