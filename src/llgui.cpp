/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements functions for bitmap font management and immediate-mode
/// user interface controls.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "llgui.hpp"

#ifdef _MSC_VER
#define STRDUP    _strdup
#else
#define STRDUP    strdup
#endif

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary Define the number of glyph indices we'd like to store per-bucket.
/// Glyph indices are stored as 32-bit unsigned integers. The actual size of 
/// the bucket is two more than this, and stores the number of entries used.
static size_t const FONT_BUCKET_SIZE      = 14;

/// @summary Define the minimum number of buckets we want in the glyph table.
static size_t const FONT_MIN_BUCKETS      = 16;

/// @summary The zero-based index of the current size of a bucket in a glyph table.
static size_t const FONT_SIZE_INDEX       = 0;

/// @summary The zero-based index of the capacity of a bucket in a glyph table.
static size_t const FONT_CAPACITY_INDEX   = 1;

/// @summary The zero-based index of the first codepoint in a glyph table bucket.
static size_t const FONT_CODEPOINT_OFFSET = 2;

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Calculate a power-of-two value greater than or equal to a given value.
/// @param The input value, which may or may not be a power of two.
/// @param min The minimum power-of-two value, which must also be non-zero.
/// @return A power-of-two that is greater than or equal to value.
static inline size_t pow2_ge(size_t value, size_t min)
{
    assert(min > 0);
    assert(min & (min - 1) == 0); 
    size_t x = min;
    while (x < value)
        x  <<= 1;
    return x;
}

/// @summary Mixes the bits in a 32-bit unsigned integer value.
/// @param c The input value.
/// @return A value based on the input.
static inline uint32_t uint32_hash(uint32_t c)
{
    // Bob Jenkins' mix function.
    uint32_t a = 0x9E3779B9;
    uint32_t b = 0x9E3779B9;
    a -= b; a -= c; a ^= (c >> 13);
    b -= c; b -= a; b ^= (a <<  8);
    c -= a; c -= b; c ^= (b >> 13);
    a -= b; a -= c; a ^= (c >> 12);
    b -= c; b -= a; b ^= (a << 16);
    c -= a; c -= b; c ^= (b >>  5);
    a -= b; a -= c; a ^= (c >>  3);
    b -= c; b -= a; b ^= (a << 10);
    c -= a; c -= b; c ^= (b >> 15);
    return  c;
}

/// @summary Retrieves the next UTF-8 codepoint from a string.
/// @param str Pointer to the start of the codepoint.
/// @param cp On return, this value stores the current codepoint.
/// @return A pointer to the start of the next codepoint.
static inline char const* next_codepoint(char const *str, uint32_t &cp)
{
    if ((str[0] & 0x80) == 0)
    {   // cp in [0x00000, 0x0007F]
        cp = str[0];
        return str + 1;
    }
    if ((str[0] & 0xFF) >= 0xC2 &&   (str[0] & 0xFF) <= 0xDF && (str[1] & 0xC0) == 0x80)
    {   // cp in [0x00080, 0x007FF]
        cp = (str[0] & 0x1F) <<  6 | (str[1] & 0x3F);
        return str + 2;
    }
    if ((str[0] & 0xF0) == 0xE0 &&   (str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80)
    {   // cp in [0x00800, 0x0FFFF]
        cp = (str[0] & 0x0F) << 12 | (str[1] & 0x3F) << 6  |    (str[2] & 0x3F);
        return str + 3;
    }
    if ((str[0] & 0xFF) == 0xF0 &&   (str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80 && (str[3] & 0xC0) == 0x80)
    {   // cp in [0x10000, 0x3FFFF]
        cp = (str[1] & 0x3F) << 12 | (str[2] & 0x3F) << 6  |    (str[3] & 0x3F);
        return str + 4;
    }
    // else, invalid UTF-8 codepoint.
    cp = 0xFFFFFFFFU;
    return str + 1;
}

/// @summary Determine the amount to advance the cursor by on the horizontal axis, 
/// taking into account the kerning table of the font.
/// @param f The font to query.
/// @param a The codepoint of the first glyph.
/// @param b The codepoint of the second glyph.
/// @param default_x The advance amount to return if there is no special kerning
/// between codepoints a and b.
static int32_t advance_x(gui::bitmap_font_t const *f, uint32_t a, uint32_t b, int32_t default_x)
{
    size_t   const  n = f->KernCount;
    uint32_t const *A = f->KerningA;
    for (size_t i = 0;  i < n; ++i)
    {
        if (A[i] == a)
        {
            uint32_t const *B = f->KerningB;
            for (size_t j = i;  j < n; ++j)
            {
                if (A[j] != a)
                    break;
                if (B[j] == b)
                    return f->KerningX[j];
            }
        }
    }
    return default_x;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
void gui::init_bitmap_font_info(gui::bitmap_font_info_t *info)
{
    if (info)
    {
        info->GlyphCount = 0;
        info->KernCount  = 0;
        info->BitDepth   = 0;
        info->PageWidth  = 0;
        info->PageHeight = 0;
        info->PageCount  = 0;
        info->FontName   = NULL;
        info->PointSize  = 0;
        info->LineHeight = 0;
        info->Baseline   = 0;
    }
}

bool gui::create_bitmap_font(gui::bitmap_font_t *font, gui::bitmap_font_info_t const *info)
{
    if (font && info)
    {
        size_t nbuckets    = pow2_ge(info->GlyphCount / FONT_BUCKET_SIZE, FONT_MIN_BUCKETS);
        font->GlyphCount   = info->GlyphCount;
        font->BucketCount  = nbuckets;
        font->GTable       = (uint32_t**)           malloc(nbuckets);
        font->Glyphs       = (gui::bitmap_glyph_t*) malloc(info->GlyphCount);
        for (size_t i = 0; i < nbuckets; ++i)
        {
            font->GTable[i]    = (uint32_t*) malloc(FONT_BUCKET_SIZE + 2);
            font->GTable[i][FONT_SIZE_INDEX]      = 0;
            font->GTable[i][FONT_CAPACITY_INDEX]  = FONT_BUCKET_SIZE;
        }

        font->KernCount    = info->KernCount;
        font->KerningA     = (uint32_t*) malloc(info->KernCount * sizeof(uint32_t));
        font->KerningB     = (uint32_t*) malloc(info->KernCount * sizeof(uint32_t));
        font->KerningX     = ( int32_t*) malloc(info->KernCount * sizeof( int32_t));

        font->BitDepth     = info->BitDepth;
        font->PageWidth    = info->PageWidth;
        font->PageHeight   = info->PageHeight;
        font->PageBytes    = info->PageWidth * info->PageHeight * (info->BitDepth / 8);
        font->PageCount    = info->PageCount;
        font->PageData     = malloc(info->PageCount * font->PageBytes);

        font->FontName     = info->FontName ? STRDUP(info->FontName) : NULL;
        font->PointSize    = info->PointSize;
        font->LineHeight   = info->LineHeight;
        font->Baseline     = info->Baseline;
        font->MinWidth     = 0;
        font->MaxWidth     = 0;
        font->AvgWidth     = 0.0f;
        return true;
    }
    else return false;
}

void gui::delete_bitmap_font(gui::bitmap_font_t *font)
{
    if (font)
    {
        if (font->FontName) free(font->FontName);
        if (font->PageData) free(font->PageData);
        if (font->KerningX) free(font->KerningX);
        if (font->KerningB) free(font->KerningB);
        if (font->KerningA) free(font->KerningA);
        if (font->Glyphs)   free(font->Glyphs);
        if (font->GTable)
        {
            for (size_t i = 0; i < font->BucketCount; ++i)
            {
                if (font->GTable[i]) free(font->GTable[i]);
            }
            free(font->GTable);
        }
        font->GlyphCount  = 0;
        font->BucketCount = 0;
        font->GTable      = NULL;
        font->Glyphs      = NULL;
        font->KernCount   = 0;
        font->KerningA    = NULL;
        font->KerningB    = NULL;
        font->KerningX    = NULL;
        font->PageCount   = 0;
        font->PageData    = NULL;
        font->FontName    = NULL;
    }
}

bool gui::define_glyph(gui::bitmap_font_t *font, gui::bitmap_glyph_t const *glyph, size_t i)
{
    uint32_t bucket = uint32_hash(glyph->Codepoint) & (font->BucketCount - 1);
    uint32_t  count = font->GTable[bucket][FONT_SIZE_INDEX];
    uint32_t  nmax  = font->GTable[bucket][FONT_CAPACITY_INDEX];
    if (count == nmax)
    {
        size_t    n = (nmax < 256) ? nmax * 2 : nmax + 256;
        uint32_t *b = (uint32_t *) realloc(font->GTable[bucket], n * sizeof(uint32_t));
        if (b == NULL) return false;
        font->GTable[bucket] = b;
        font->GTable[bucket][FONT_CAPACITY_INDEX] = n;
    }
    font->GTable[bucket][FONT_SIZE_INDEX]++;
    font->GTable[bucket][count] = uint32_t(i);
    font->Glyphs[i] =*glyph;
    return true;
}

bool gui::define_kerning(gui::bitmap_font_t *font, uint32_t A, uint32_t B, int32_t X, size_t i)
{
    font->KerningA[i] = A;
    font->KerningB[i] = B;
    font->KerningX[i] = X;
    return true;
}

bool gui::define_page(gui::bitmap_font_t *font, void const *src, size_t src_size, size_t i, bool flip_y)
{
    if (src == NULL || src_size <= font->PageBytes)
    {
        return false;
    }
    size_t         rowl = (font->BitDepth  / 8) * font->PageWidth;
    size_t         rowo = (flip_y) ?  font->PageHeight : 0;
    size_t         rowi = (flip_y) ? -rowl : rowl;
    uint8_t       *dstp = (uint8_t*)  font->PageData + (i * font->PageBytes) + (rowo * rowl);
    uint8_t const *srcp = (uint8_t const*) src;
    for (size_t r = 0;  r < font->PageHeight; ++r)
    {
        memcpy(dstp, srcp, rowl);
        srcp += rowl; // always increasing
        dstp += rowi; // either increasing or decreasing
    }
    return true;
}

void* gui::glyph_page(gui::bitmap_font_t const *font, size_t i)
{
    if (i < font->PageCount) return (uint8_t*) font->PageData + (i * font->PageBytes);
    else return NULL;
}

void gui::measure_string(gui::bitmap_font_t const *font, char const *str, size_t *out_w, size_t *out_h)
{
    if (str != NULL)
    {
        size_t   l = 0; // length, in characters
        size_t   w = 0; // total width, in pixels
        size_t   h = 0; // total height, in pixels
        uint32_t c = 0; // current  UTF-8 codepoint, 0xFFFFFFFFU = invalid
        uint32_t p = 0; // previous UTF-8 codepoint, 0xFFFFFFFFU = invalid
        uint32_t const  m = font->BucketCount - 1;
        char const     *n = next_codepoint(str, c);
        while (c != 0)
        {
            uint32_t bucket = uint32_hash(c) & m;
            uint32_t const *table = font->GTable[bucket];
            uint32_t const  count = font->GTable[bucket][FONT_SIZE_INDEX];
            for (size_t  i  = FONT_CODEPOINT_OFFSET; i < FONT_CODEPOINT_OFFSET + count; ++i)
            {
                if (table[i] == c)
                {
                    size_t gi = table[i];   // the index in into font->Glyphs.
                    w += advance_x(font, p, c, font->Glyphs[gi].AdvanceX);
                    break;
                }
            }
            if (c == '\n') h += font->LineHeight;
            p = c;
            n = next_codepoint(n, c);
            l++;
        }
        if (l > 0) h += font->LineHeight;
        if (out_w)*out_w = w;
        if (out_h)*out_h = h;
    }
    else
    {
        if (out_w) *out_w = 0;
        if (out_h) *out_h = 0;
    }
}

