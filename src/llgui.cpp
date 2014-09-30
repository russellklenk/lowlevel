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
    assert((min > 0));
    assert((min & (min - 1)) == 0);
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

/// @summary Initializes a control list.
/// @param list The list to initialize.
/// @param capacity The initial list capacity.
template <typename T>
static inline void clist_init(gui::control_list_t<T> *list, size_t capacity)
{
    list->Capacity  = capacity;
    list->Count     = 0;
    list->Ids       = NULL;
    list->State     = NULL;
    if (capacity)
    {
        list->Ids   = (uint32_t*) malloc(capacity * sizeof(uint32_t));
        list->State = (T       *) malloc(capacity * sizeof(T));
    }
}

/// @summary Releases resources associated with a control list.
/// @param list The control list to delete.
template <typename T>
static inline void clist_free(gui::control_list_t<T> *list)
{
    if (list->State != NULL) free(list->State);
    if (list->Ids   != NULL) free(list->Ids);
    list->Capacity   = 0;
    list->Count      = 0;
    list->Ids        = NULL;
    list->State      = NULL;
}

/// @summary Flushes a control list, returning it to empty.
/// @param list The list to flush.
template <typename T>
static inline void clist_flush(gui::control_list_t<T> *list)
{
    list->Count = 0;
}

/// @summary Searches a control list for a given control ID.
/// @param list The list to search.
/// @param id The unique identifier of the control.
/// @param out_index On return, stores the zero-based index of the control state.
/// @return true if the control was found.
template <typename T>
static inline bool clist_find(gui::control_list_t<T> *list, uint32_t id, size_t *out_index)
{
    for (size_t i = 0; i < list->Count; ++i)
    {
        if (list->Ids[i] == id)
        {
            *out_index = i;
            return true;
        }
    }
    return false;
}

/// @summary Appends a control to the end of the list.
/// @param list The list to update.
/// @param id The unique identifier of the control being added.
/// @param state The state of the control being added.
/// @return The zero-based index of the control.
template <typename T>
static inline size_t clist_append(gui::control_list_t<T> *list, uint32_t id, T const &state)
{
    if (list->Count == list->Capacity)
    {
        size_t    new_max   = list->Capacity < 64 ? list->Capacity * 2 : list->Capacity + 64;
        uint32_t *new_ids   = (uint32_t*) realloc(list->Ids  , new_max * sizeof(uint32_t));
        T        *new_state = (T       *) realloc(list->State, new_max * sizeof(T));
        if (new_ids   != NULL) list->Ids    = new_ids;
        if (new_state != NULL) list->State  = new_state;
        if (new_ids   != NULL && new_state != NULL) list->Capacity = new_max;
    }
    assert(list->Count < list->Capacity);
    list->Ids[list->Count]   = id;
    list->State[list->Count] = state;
    return list->Count++;
}

/// @summary Updates an existing control, or appends it if it doesn't exist.
/// @param list The list to update.
/// @param id The unique identifier of the control.
/// @param state The current state of the control.
/// @return The zero-based index of the control.
template <typename T>
static inline size_t clist_update(gui::control_list_t<T> *list, uint32_t id, T const &state)
{
    size_t index = 0;
    if (clist_find(list, id, &index))
    {
        list->State[index] = state;
        return index;
    }
    else return clist_append(list, id, state);
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

void gui::init_key_buffer(gui::key_buffer_t *buffer)
{
    if (buffer)
    {
        buffer->Count = 0;
    }
}

void gui::key_buffer_flush(gui::key_buffer_t *buffer)
{
    buffer->Count = 0;
}

void gui::key_buffer_press(gui::key_buffer_t *buffer, gui::key_state_t const *key)
{
    if (buffer->Count < LLGUI_MAX_ACTIVE_KEYS)
    {
        size_t      n = buffer->Count;
        uint16_t   *k = buffer->KeyCode;
        uint16_t    v = key->KeyCode;
        for (size_t i = 0; i < n; ++i)
        {
            if (k[i] == v)
            {   // this key is already active, update state.
                buffer->DownTime[i] = key->DownTime;
                buffer->Delay[i]    = key->Delay;
                return;
            }
        }
        buffer->KeyCode[n]  = v;
        buffer->DownTime[n] = key->DownTime;
        buffer->Delay[n]    = key->Delay;
        buffer->Count++;
    }
    // else, the key press is dropped.
}

void gui::key_buffer_release(gui::key_buffer_t *buffer, uint16_t key_code)
{
    size_t      n = buffer->Count;
    uint16_t   *k = buffer->KeyCode;
    for (size_t i = 0; i < n; ++i)
    {
        if (k[i] == key_code)
        {   // swap the last active key into this slot.
            buffer->KeyCode[i]  = buffer->KeyCode [n-1];
            buffer->DownTime[i] = buffer->DownTime[n-1];
            buffer->Delay[i]    = buffer->Delay   [n-1];
            buffer->Count--;
            return;
        }
    }
}

bool gui::key_index(gui::key_buffer_t *buffer, uint16_t key_code, size_t *out_index)
{
    size_t      n = buffer->Count;
    uint16_t   *k = buffer->KeyCode;
    for (size_t i = 0; i < n; ++i)
    {
        if (k[i] == key_code)
        {
            if (out_index) *out_index = i;
            return true;
        }
    }
    return false;
}

bool gui::create_context(gui::context_t *ui)
{
    if (ui)
    {
        ui->HotItem     = LLGUI_INVALID_ID;
        ui->ActiveItem  = LLGUI_INVALID_ID;
        ui->PointerX    = 0.0f;
        ui->PointerY    = 0.0f;
        ui->InteractX   = 0.0f;
        ui->InteractY   = 0.0f;
        ui->Interaction = gui::INTERACTION_OFF;
        ui->KeyCount    = 0;
        ui->CapsLockOn  = false;
        ui->ShiftDown   = false;
        ui->UpdateTime  = 0.0f;
        ui->DeltaTime   = 0.0f;
        ui->RepeatRate  = 10.0f; // 10 characters per-second
        ui->BlinkRate   = 2.0f;  // blink 2 times per-second
        ui->CaretAlpha  = 1.0f;  // fully opaque
        gui::init_key_buffer(&ui->KeyHistory);
        clist_init(&ui->Buttons, 32);
        clist_init(&ui->Toggles, 32);
        // more clist_init()
        // ...
        return true;
    }
    else return false;
}

void gui::delete_context(gui::context_t *ui)
{
    if (ui)
    {
        ui->HotItem    = LLGUI_INVALID_ID;
        ui->ActiveItem = LLGUI_INVALID_ID;
        clist_free(&ui->Toggles);
        clist_free(&ui->Buttons);
        // more clist_free()
        // ...
    }
}

void gui::flush_context(gui::context_t *ui)
{
    ui->HotItem     = LLGUI_INVALID_ID;
    ui->ActiveItem  = LLGUI_INVALID_ID;
    ui->Interaction = gui::INTERACTION_OFF;
    ui->KeyCount    = 0;
    clist_flush(&ui->Buttons);
    clist_flush(&ui->Toggles);
    // more clist_flush()
    // ...
}

bool gui::hit_test(size_t x, size_t y, size_t w, size_t h, size_t test_x, size_t test_y)
{
    return (test_x >= x && test_x < (x + w) && 
            test_y >= y && test_y < (y + h));
}

bool gui::pointer_over(gui::context_t *ui, size_t x, size_t y, size_t w, size_t h)
{
    return (ui->PointerX >= x && ui->PointerX < (x + w) &&
            ui->PointerY >= y && ui->PointerY < (y + h));
}

void gui::pointer_move(gui::context_t *ui, float x, float y)
{
    ui->PointerX = x;
    ui->PointerY = y;
}

void gui::interaction_begin(gui::context_t *ui, float x, float y, uint32_t modifiers)
{
    ui->PointerX    = x;
    ui->PointerY    = y;
    ui->InteractX   = x;
    ui->InteractY   = y;
    modifiers      &= gui::INTERACTION_ALT | gui::INTERACTION_CTRL  | gui::INTERACTION_SHIFT;
    ui->Interaction = gui::INTERACTION_ON  | gui::INTERACTION_BEGIN | modifiers;
}

void gui::interaction_end(gui::context_t *ui, float x, float y, uint32_t modifiers)
{
    ui->PointerX    = x;
    ui->PointerY    = y;
    modifiers      &= gui::INTERACTION_ALT | gui::INTERACTION_CTRL | gui::INTERACTION_SHIFT;
    ui->Interaction = gui::INTERACTION_ON  | gui::INTERACTION_END  | modifiers;
}

void gui::key_press(gui::context_t *ui, float x, float y, uint16_t key_code, uint32_t modifiers)
{
    gui::key_state_t ks;
    ks.KeyCode     = key_code;
    ks.DownTime    = ui->UpdateTime;
    ks.Delay       = 1.0f; // @todo: have ui->RepeatDelay
    gui::key_buffer_press(&ui->KeyHistory, &ks);
    ui->PointerX   = x;
    ui->PointerY   = y;
    if ((modifiers & gui::INTERACTION_CAPS) != 0)
    {
        ui->CapsLockOn = ! ui->CapsLockOn;
    }
    if ((modifiers & gui::INTERACTION_SHIFT) != 0)
    {
        ui->ShiftDown = true;
    }
}

void gui::key_repeat(gui::context_t *ui, float x, float y, uint16_t key_code)
{
    size_t index = 0;
    if (gui::key_index(&ui->KeyHistory, key_code, &index))
    {
        gui::key_buffer_t &kb = ui->KeyHistory;
        float          repeat = 1.0f / ui->RepeatRate;

        if (kb.Delay[index]  - ui->DeltaTime > 0.0f)
        {
            // still waiting for the initial delay period to expire.
            kb.Delay[index] -= ui->DeltaTime;
        }
        else
        {
            // the key is known to be pressed. has enough time passed 
            // that we need to regenerate the key press event?
            if (ui->UpdateTime - kb.DownTime[index] > repeat)
            {
                // generate a key repeat by updating the timestamp.
                kb.DownTime[index] = ui->UpdateTime;
            }
        }
    }
    ui->PointerX = x;
    ui->PointerY = y;
}

void gui::key_release(gui::context_t *ui, float x, float y, uint16_t key_code, uint32_t modifiers)
{
    gui::key_buffer_release(&ui->KeyHistory, key_code);
    if ((modifiers & gui::INTERACTION_SHIFT) == 0)
    {
        ui->ShiftDown = false;
    }
}

bool gui::make_hot(gui::context_t *ui, uint32_t id)
{
    if (ui->ActiveItem == LLGUI_INVALID_ID || ui->ActiveItem == id)
    {
        ui->HotItem = id;
        return true;
    }
    else return false; // a different item is currently active.
}

void gui::make_active(gui::context_t *ui, uint32_t id)
{
    ui->ActiveItem = id;
}

void gui::make_not_hot(gui::context_t *ui, uint32_t id)
{
    if (ui->HotItem == id)
    {
        ui->HotItem  = LLGUI_INVALID_ID;
    }
}

void gui::make_not_active(gui::context_t *ui, uint32_t id)
{
    if (ui->ActiveItem == id)
    {
        ui->ActiveItem  = LLGUI_INVALID_ID;
    }
}

bool gui::interaction_starting(gui::context_t *ui)
{
    return ((ui->Interaction & gui::INTERACTION_ON) != 0 && (ui->Interaction & gui::INTERACTION_BEGIN) != 0);
}

bool gui::interaction_active(gui::context_t *ui)
{
    return ((ui->Interaction & gui::INTERACTION_ON) != 0);
}

bool gui::interaction_ending(gui::context_t *ui)
{
    return ((ui->Interaction & gui::INTERACTION_ON) != 0 && (ui->Interaction & gui::INTERACTION_END) != 0);
}

void gui::begin_update(gui::context_t *ui, float current_time, float elapsed_time)
{
    ui->UpdateTime  = current_time;
    ui->DeltaTime   = elapsed_time;

    float seconds_per_blink = 1.0f / ui->BlinkRate;
    float multiples = current_time * ui->BlinkRate;
    float whole     = floorf(multiples);
    float frac      = multiples - whole;

    if (int(whole) & 1) ui->CaretAlpha = 1.0f - frac;
    else ui->CaretAlpha = frac;
}

void gui::end_input(gui::context_t *ui)
{
    gui::key_buffer_t const &kb = ui->KeyHistory;
    size_t                   nk = 0;

    ui->KeyCount  = 0;
    for (size_t i = 0; i < kb.Count; ++i)
    {
        if (kb.DownTime[i] == ui->UpdateTime && nk < LLGUI_MAX_ACTIVE_KEYS)
        {
            ui->ActiveKeys[nk++] = kb.KeyCode[i];
        }
    }
    ui->KeyCount  = nk;
}

void gui::end_update(gui::context_t *ui)
{
    if ((ui->Interaction & gui::INTERACTION_BEGIN) != 0)
    {
        ui->Interaction ^= gui::INTERACTION_BEGIN;
    }
    if ((ui->Interaction & gui::INTERACTION_END) != 0)
    {
        ui->Interaction  = gui::INTERACTION_OFF;
    }
    ui->KeyCount = 0;
}

gui::button_t* gui::button(gui::context_t *ui, uint32_t id, size_t x, size_t y, size_t w, size_t h, bool click, bool active)
{
    gui::button_t *control = NULL;
    size_t         index   = 0;
    bool           isHot   = false;
    bool           clicked = false;

    // create the control if it doesn't already exist.
    if (!clist_find(&ui->Buttons, id, &index))
    {
        gui::button_t    state;
        state.XYWH[0]    = x;
        state.XYWH[1]    = y;
        state.XYWH[2]    = w;
        state.XYWH[3]    = h;
        state.State      = uint32_t(index);
        state.IsHot      = false;
        state.IsActive   = false;
        state.WasClicked = false;
        index = clist_append(&ui->Buttons, id, state);
    }

    control = &ui->Buttons.State[index];
    control->XYWH[0] = x;
    control->XYWH[1] = y;
    control->XYWH[2] = w;
    control->XYWH[3] = h;
    isHot  = gui::pointer_over(ui, x, y, w, h);

    if (click)
    {
        control->IsHot      = isHot;
        control->WasClicked = true;
        return control;
    }
    if (isHot && active)
    {
        gui::make_hot(ui, id);
    }
    else
    {
        gui::make_not_hot(ui, id);
        gui::make_not_active(ui, id);
    }
    if (ui->ActiveItem == id)
    {
        if (gui::interaction_ending(ui))
        {
            if (ui->HotItem == id)
            {
                clicked = true;
            }
            gui::make_not_active(ui, id);
        }
    }
    else if (isHot && active)
    {
        if (gui::interaction_starting(ui))
        {
            gui::make_active(ui, id);
        }
    }

    control->IsHot      =(id == ui->HotItem);
    control->IsActive   =(id == ui->ActiveItem);
    control->WasClicked = clicked;
    return control;
}

gui::toggle_t* gui::toggle(gui::context_t *ui, uint32_t id, size_t x, size_t y, size_t w, size_t h, bool default_set, bool click, bool active)
{
    gui::toggle_t *control = NULL;
    size_t         index   = 0;
    bool           isHot   = false;
    bool           clicked = false;

    // create the control if it doesn't already exist.
    if (!clist_find(&ui->Toggles, id, &index))
    {
        gui::toggle_t    state;
        state.XYWH[0]    = x;
        state.XYWH[1]    = y;
        state.XYWH[2]    = w;
        state.XYWH[3]    = h;
        state.State      = uint32_t(index);
        state.IsHot      = false;
        state.IsActive   = false;
        state.WasClicked = false;
        state.IsOn       = default_set;
        index = clist_append(&ui->Toggles, id, state);
    }

    control = &ui->Toggles.State[index];
    control->XYWH[0] = x;
    control->XYWH[1] = y;
    control->XYWH[2] = w;
    control->XYWH[3] = h;
    isHot  = gui::pointer_over(ui, x, y, w, h);

    if (click)
    {
        control->IsHot      = isHot;
        control->WasClicked = true;
        control->IsOn       =!control->IsOn;
        return control;
    }
    if (isHot && active)
    {
        gui::make_hot(ui, id);
    }
    else
    {
        gui::make_not_hot(ui, id);
        gui::make_not_active(ui, id);
    }
    if (ui->ActiveItem == id)
    {
        if (gui::interaction_ending(ui))
        {
            if (ui->HotItem == id)
            {
                clicked = true;
            }
            gui::make_not_active(ui, id);
        }
    }
    else if (isHot && active)
    {
        if (gui::interaction_starting(ui))
        {
            gui::make_active(ui, id);
        }
    }

    control->IsHot      =(id == ui->HotItem);
    control->IsActive   =(id == ui->ActiveItem);
    control->WasClicked = clicked;
    control->IsOn       = clicked ? !control->IsOn : control->IsOn;
    return control;
}

