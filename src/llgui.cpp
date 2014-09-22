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
/// the bucket is one more than this, and stores the number of entries used.
static size_t const FONT_BUCKET_SIZE = 15;

/// @summary Define the minimum number of buckets we want in the glyph table.
static size_t const FONT_MIN_BUCKETS = 16;

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Calculate a power-of-two value greater than or equal to a given value.
/// @param The input value.
/// @param min The minimum power-of-two value, which must also be non-zero.
/// @return A power-of-two that is greater than or equal to value.
static size_t pow2_ge(size_t value, size_t min)
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
static uint32_t uint32_hash(uint32_t c)
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
            font->GTable[i]    = (uint32_t*) malloc(FONT_BUCKET_SIZE + 1);
            font->GTable[i][0] = 0; // the first item in each bucket is the size.
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

