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
#include <stdint.h>

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
/// @summary Define the maximum number of active keys at any given time.
#ifndef LLGUI_MAX_ACTIVE_KEYS
#define LLGUI_MAX_ACTIVE_KEYS    8U
#endif

/*/////////////////
//   Data Types  //
/////////////////*/
/// @summary Defines the data associated with a single glyph in a bitmap font.
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

/// @summary Defines the data associated with a bitmap font.
struct bitmap_font_t
{
    size_t          GlyphCount; /// The number of glyphs defined in the font.
    size_t          BucketCount;/// The number of buckets in the glyph table.
    uint32_t      **GTable;     /// Table mapping Unicode codepoint -> index in Glyphs.
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

/// @summary Defines the data necessary to allocate storage for a bitmap font.
struct bitmap_font_info_t
{
    size_t          GlyphCount; /// The number of glyphs defined in the font.
    size_t          KernCount;  /// The number of kerning table entries.
    size_t          BitDepth;   /// The bits-per-pixel of the image data, either 8 or 32.
    size_t          PageWidth;  /// The width of a single texture page, in pixels.
    size_t          PageHeight; /// The height of a single texture page, in pixels.
    size_t          PageCount;  /// The number of texture pages containing glyphs.
    char const     *FontName;   /// An optional font name, may be NULL.
    size_t          PointSize;  /// The font size, in points.
    size_t          LineHeight; /// The number of vertical pixels between two lines.
    size_t          Baseline;   /// The number of pixels from the top of a line to the common base.
};

/// @summary Encapsulates the current state of a single key in the input buffer.
struct key_state_t
{
    uint16_t        KeyCode;    /// The raw key code.
    float           DownTime;   /// The time value at which the key was pressed.
    float           Delay;      /// The amount of time remaining before a key repeat.
};

/// @summary Stores the state associated with currently pressed keys. This is 
/// used internally by the IMGUI context, and typically not by the application.
/// The IMGUI context accesses the fields of this type directly.
struct key_buffer_t
{
    size_t          Count;                           /// The number of active keys.
    uint16_t        KeyCode [LLGUI_MAX_ACTIVE_KEYS]; /// The raw key codes.
    float           DownTime[LLGUI_MAX_ACTIVE_KEYS]; /// The key press timestamp.
    float           Delay   [LLGUI_MAX_ACTIVE_KEYS]; /// Time remaining before key repeat.
};

/*////////////////
//   Functions  //
////////////////*/
/// @summary Initializes the fields of a bitmap_font_info_t structure to their defaults.
/// @param info The structure to initialize.
LLGUI_PUBLIC void init_bitmap_font_info(gui::bitmap_font_info_t *info);

/// @summary Allocates storage for a bitmap font.
/// @param font The bitmap font to initialize.
/// @param info Information about the font provided by the application.
/// @return true if font storage was initialized.
LLGUI_PUBLIC bool create_bitmap_font(gui::bitmap_font_t *font, gui::bitmap_font_info_t const *info);

/// @summary Releases storage allocated for a bitmap font.
/// @param font The font to delete.
LLGUI_PUBLIC void delete_bitmap_font(gui::bitmap_font_t *font);

/// @summary Defines a single glyph within a bitmap font.
/// @param font The font to update.
/// @param glyph The glyph definition.
/// @param i The zero-based index of the glyph, in [0, font->GlyphCount).
/// @return true if the glyph definition was stored within the font.
LLGUI_PUBLIC bool define_glyph(gui::bitmap_font_t *font, gui::bitmap_glyph_t const *glyph, size_t i);

/// @summary Defines a single kerning entry within a bitmap font.
/// @param font The font to update.
/// @param codepoint_a The codepoint of the first glyph.
/// @param codepoint_b The codepoint of the second glyph.
/// @param advance_x The amount to advance the cursor on the x-axis after drawing
/// glyph A, when glyph B immediately follows it.
/// @param i The zero-based index of the kerning entry, in [0, font->KernCount).
/// @return true if the kerning definition was stored within the font.
LLGUI_PUBLIC bool define_kerning(gui::bitmap_font_t *font, uint32_t codepoint_a, uint32_t codepoint_b, int32_t advance_x, size_t i);

/// @summary Copies the image data for a glyph page.
/// @param font The font to update.
/// @param src The image data for the page.
/// @param src_size The size of the image data, in bytes. 
/// @param i The zero-based index of the page to update, in [0, font->PageCount).
/// @param flip_y Specify true to flip the page data.
/// @return true if the image data was copied to the font.
LLGUI_PUBLIC bool define_page(gui::bitmap_font_t *font, void const *src, size_t src_size, size_t i, bool flip_y);

/// @summary Retrieves a pointer to the image data for a glyph page. The page is
/// font->PageBytes bytes in length, and will be either R8 or ARGB8 data.
/// @param font The font to query.
/// @param i The zero-based index of the page to retrieve, in [0, font->PageCount).
/// @return A pointer to the page image data, or NULL.
LLGUI_PUBLIC void* glyph_page(gui::bitmap_font_t const *font, size_t i);

/// @summary Calculates the dimensions of a string when rendered with a given font.
/// @param font The font definition to use.
/// @param str A NULL-terminated UTF-8 string.
/// @param out_w On return, this location stores the width of the string, in pixels.
/// @param out_h On return, this location stores the height of the string, in pixels.
LLGUI_PUBLIC void measure_string(gui::bitmap_font_t const *font, char const *str, size_t *out_w, size_t *out_h);

/// @summary Initializes a key buffer to empty (no keys active).
/// @param buffer The buffer to initialize.
LLGUI_PUBLIC void init_key_buffer(gui::key_buffer_t *buffer);

/// @summary Resets a key buffer to empty (no keys active).
/// @param buffer The buffer to flush.
LLGUI_PUBLIC void key_buffer_flush(gui::key_buffer_t *buffer);

/// @summary Buffers a key press event, marking a key as active.
/// @param buffer The active key buffer.
/// @param key Information about the key that was pressed.
LLGUI_PUBLIC void key_buffer_press(gui::key_buffer_t *buffer, gui::key_state_t const *key);

/// @summary Indicates that a key was released, removing it from the active buffer.
/// @param buffer The active key buffer.
/// @param key_code The key code of the key that was released.
LLGUI_PUBLIC void key_buffer_release(gui::key_buffer_t *buffer, uint16_t key_code);

/// @summary Locates a key within the active buffer.
/// @param buffer The buffer to query.
/// @param key_code The key code of the key being queried.
/// @param out_index On return, stores the zero-based index of the key.
/// @return true if the key was found in the active buffer.
LLGUI_PUBLIC bool key_index(gui::key_buffer_t *buffer, uint16_t key_code, size_t *out_index);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace gui */

#endif /* !defined(LLGUI_HPP_INCLUDED) */

