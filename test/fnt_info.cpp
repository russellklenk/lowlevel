/*/////////////////////////////////////////////////////////////////////////////
/// @summary Loads and displays information about DDS files.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <stdio.h>
#include <stdlib.h>
#include "lldatain.hpp"

/*/////////////////
//   Constants   //
/////////////////*/
static const size_t CHANNEL_COUNT = 5;
static const data::bmfont_channel_e CHANNEL_FLAGS[CHANNEL_COUNT] = 
{
    data::BMFONT_CHANNEL_NONE, 
    data::BMFONT_CHANNEL_BLUE, 
    data::BMFONT_CHANNEL_GREEN, 
    data::BMFONT_CHANNEL_RED, 
    data::BMFONT_CHANNEL_ALPHA
};

static const size_t ATTRIBUTE_COUNT = 6;
static const data::bmfont_attributes_e ATTRIBUTE_FLAGS[ATTRIBUTE_COUNT] =
{
    data::BMFONT_ATTRIBUTE_NONE, 
    data::BMFONT_ATTRIBUTE_SMOOTH, 
    data::BMFONT_ATTRIBUTE_UNICODE, 
    data::BMFONT_ATTRIBUTE_ITALIC, 
    data::BMFONT_ATTRIBUTE_BOLD, 
    data::BMFONT_ATTRIBUTE_FIXED
};

static const size_t CONTENT_COUNT = 5;
static const data::bmfont_content_e CONTENT_VALUES[CONTENT_COUNT] =
{
    data::BMFONT_CONTENT_GLYPH, 
    data::BMFONT_CONTENT_OUTLINE, 
    data::BMFONT_CONTENT_COMBINED, 
    data::BMFONT_CONTENT_ZERO, 
    data::BMFONT_CONTENT_ONE
};

/*///////////////////////
//   Local Functions   //
///////////////////////*/
static char const* channel_str(uint8_t channel)
{
    switch (channel)
    {
        case data::BMFONT_CHANNEL_NONE:
            return "BMFONT_CHANNEL_NONE";
        case data::BMFONT_CHANNEL_BLUE:
            return "BMFONT_CHANNEL_BLUE";
        case data::BMFONT_CHANNEL_GREEN:
            return "BMFONT_CHANNEL_GREEN";
        case data::BMFONT_CHANNEL_RED:
            return "BMFONT_CHANNEL_RED";
        case data::BMFONT_CHANNEL_ALPHA:
            return "BMFONT_CHANNEL_ALPHA";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* attrib_str(uint8_t attribs)
{
    switch (attribs)
    {
        case data::BMFONT_ATTRIBUTE_NONE:
            return "BMFONT_ATTRIBUTE_NONE";
        case data::BMFONT_ATTRIBUTE_SMOOTH:
            return "BMFONT_ATTRIBUTE_SMOOTH";
        case data::BMFONT_ATTRIBUTE_UNICODE:
            return "BMFONT_ATTRIBUTE_UNICODE";
        case data::BMFONT_ATTRIBUTE_ITALIC:
            return "BMFONT_ATTRIBUTE_ITALIC";
        case data::BMFONT_ATTRIBUTE_BOLD:
            return "BMFONT_ATTRIBUTE_BOLD";
        case data::BMFONT_ATTRIBUTE_FIXED:
            return "BMFONT_ATTRIBUTE_FIXED";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* content_str(uint8_t channel)
{
    switch (channel)
    {
        case data::BMFONT_CONTENT_GLYPH:
            return "BMFONT_CONTENT_GLYPH";
        case data::BMFONT_CONTENT_OUTLINE:
            return "BMFONT_CONTENT_OUTLINE";
        case data::BMFONT_CONTENT_COMBINED:
            return "BMFONT_CONTENT_COMBINED";
        case data::BMFONT_CONTENT_ZERO:
            return "BMFONT_CONTENT_ZERO";
        case data::BMFONT_CONTENT_ONE:
            return "BMFONT_CONTENT_ONE";
        default:
            break;
    }
    return "UNKNOWN";
}

static void print_info_block(FILE *fp, data::bmfont_info_block_t const *bk)
{
    if (bk == NULL)
    {
        fprintf(fp, "BMFONT_INFO_BLOCK:\n");
        fprintf(fp, "  Not present.\n");
        fprintf(fp, "\n");
        return;
    }

    fprintf(fp, "BMFONT_INFO_BLOCK:\n");
    fprintf(fp, "  Font Name:      %s\n", bk->FontName);
    fprintf(fp, "  Font Size:      %d\n", signed(bk->FontSize));
    fprintf(fp, "  Charset:        %c\n", bk->CharSet);
    fprintf(fp, "  Attributes:     ");
    for (size_t i = 0, n = 0; i < ATTRIBUTE_COUNT; ++i)
    {
        if ((n == 0 && bk->Attributes == data::BMFONT_ATTRIBUTE_NONE) || 
            (bk->Attributes & ATTRIBUTE_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", attrib_str(ATTRIBUTE_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "  Outline:        %u\n", unsigned(bk->Outline));
    fprintf(fp, "  Padding T:      %u\n", unsigned(bk->PaddingTop));
    fprintf(fp, "  Padding L:      %u\n", unsigned(bk->PaddingLeft));
    fprintf(fp, "  Padding R:      %u\n", unsigned(bk->PaddingRight));
    fprintf(fp, "  Padding B:      %u\n", unsigned(bk->PaddingBottom));
    fprintf(fp, "  Spacing X:      %u\n", unsigned(bk->SpacingX));
    fprintf(fp, "  Spacing Y:      %u\n", unsigned(bk->SpacingY));
    fprintf(fp, "  Stretch Height: %u\n", unsigned(bk->StretchH));
    fprintf(fp, "  AA Level:       %u\n", unsigned(bk->AA));
    fprintf(fp, "\n");
}

static void print_common_block(FILE *fp, data::bmfont_common_block_t const *bk)
{
    if (bk == NULL)
    {
        fprintf(fp, "BMFONT_COMMON_BLOCK:\n");
        fprintf(fp, "  Not present.\n");
        fprintf(fp, "\n");
        return;
    }

    fprintf(fp, "BMFONT_COMMON_BLOCK:\n");
    fprintf(fp, "  Line Height:    %u\n", unsigned(bk->LineHeight));
    fprintf(fp, "  Base Line:      %u\n", unsigned(bk->BaseLine));
    fprintf(fp, "  Scale Width:    %u\n", unsigned(bk->ScaleWidth));
    fprintf(fp, "  Scale Height:   %u\n", unsigned(bk->ScaleHeight));
    fprintf(fp, "  Page Count:     %u\n", unsigned(bk->PageCount));
    fprintf(fp, "  Attributes:     ");
    for (size_t i = 0, n = 0; i < ATTRIBUTE_COUNT; ++i)
    {
        if ((n == 0 && bk->Attributes == data::BMFONT_ATTRIBUTE_NONE) || 
            (bk->Attributes & ATTRIBUTE_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", attrib_str(ATTRIBUTE_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "  Alpha Channel:  %s\n", content_str(bk->AlphaChannel));
    fprintf(fp, "  Red Channel:    %s\n", content_str(bk->RedChannel));
    fprintf(fp, "  Green Channel:  %s\n", content_str(bk->GreenChannel));
    fprintf(fp, "  Blue Channel:   %s\n", content_str(bk->BlueChannel));
    fprintf(fp, "\n");
}

static void print_pages_block(FILE *fp, data::bmfont_pages_block_t const *bk, size_t npages, size_t pagelen)
{
    if (bk == NULL)
    {
        fprintf(fp, "BMFONT_PAGES_BLOCK:\n");
        fprintf(fp, "  Not present.\n");
        fprintf(fp, "\n");
        return;
    }

    char const *page = bk->PageNames;
    fprintf(fp, "BMFONT_PAGES_BLOCK:\n");
    for (size_t i = 0; i < npages; ++i, page += pagelen)
    {
        fprintf(fp, "  %s\n", page);
    }
    fprintf(fp, "\n");
}

static void print_char(FILE *fp, data::bmfont_char_t const *ch)
{
    fprintf(fp, "BMFONT_CHAR:\n");
    fprintf(fp, "  Codepoint:    %u\n", unsigned(ch->Codepoint));
    fprintf(fp, "  Texture X:    %u\n", unsigned(ch->TextureX));
    fprintf(fp, "  Texture Y:    %u\n", unsigned(ch->TextureY));
    fprintf(fp, "  Width:        %u\n", unsigned(ch->Width));
    fprintf(fp, "  Height:       %u\n", unsigned(ch->Height));
    fprintf(fp, "  Offset X:     %u\n", unsigned(ch->OffsetX));
    fprintf(fp, "  Offset Y:     %u\n", unsigned(ch->OffsetY));
    fprintf(fp, "  Advance X:    %u\n", unsigned(ch->AdvanceX));
    fprintf(fp, "  Page Index:   %u\n", unsigned(ch->PageIndex));
    fprintf(fp, "  Channel:      ");
    for (size_t i = 0, n = 0; i < CHANNEL_COUNT; ++i)
    {
        if ((n == 0 && ch->Channel == data::BMFONT_CHANNEL_NONE) || (ch->Channel & CHANNEL_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", channel_str(CHANNEL_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
}

static void print_chars_block(FILE *fp, data::bmfont_chars_block_t const *bk, size_t nchars)
{
    if (bk == NULL)
    {
        fprintf(fp, "BMFONT_CHARS_BLOCK:\n");
        fprintf(fp, "  Not present.\n");
        fprintf(fp, "\n");
        return;
    }

    fprintf(fp, "BMFONT_CHARS_BLOCK:\n");
    for (size_t i = 0; i < nchars; ++i)
    {
        print_char(fp, &bk->Char[i]);
    }
    fprintf(fp, "\n");
}

static void print_kerning(FILE *fp, data::bmfont_kerning_t const *kern)
{
    fprintf(fp, "BMFONT_KERNING_PAIR:\n");
    fprintf(fp, "  A:            %u\n", kern->A);
    fprintf(fp, "  B:            %u\n", kern->B);
    fprintf(fp, "  Advance:      %d\n", signed(kern->AdvanceX));
    fprintf(fp, "\n");
}

static void print_kerning_block(FILE *fp, data::bmfont_kerning_block_t const *bk, size_t nkern)
{
    if (bk == NULL)
    {
        fprintf(fp, "BMFONT_KERNING_BLOCK:\n");
        fprintf(fp, "  Not present.\n");
        fprintf(fp, "\n");
        return;
    }

    fprintf(fp, "BMFONT_KERNING_BLOCK:\n");
    for (size_t i = 0; i < nkern; ++i)
    {
        print_kerning(fp, &bk->Pair[i]);
    }
    fprintf(fp, "\n");
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Implements the entry point of the application.
/// @param argc The number of arguments passed on the command line.
/// @param argv An array of strings specifying command line arguments.
/// @return EXIT_SUCCESS or EXIT_FAILURE.
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("ERROR: Not enough command-line arguments.\n");
        printf("USAGE: fntinfo path/to/file.fnt\n");
        exit(EXIT_FAILURE);
    }

    size_t fnt_size = 0;
    void  *fnt_data = data::load_binary(argv[1], &fnt_size);
    if (fnt_data == NULL)
    {
        printf("ERROR: Input file \'%s\' not found.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("INFO: Loaded \'%s\', %u bytes.\n", argv[1], uint32_t(fnt_size));
    }

    data::bmfont_desc_t font;
    if (!data::bmfont_describe(fnt_data, fnt_size, &font))
    {
        printf("ERROR: Unexpected data in BMfont.\n");
        free(fnt_data);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("INFO: Successfully parsed BMfont.\n");
    }

    print_info_block(stdout, font.Info);
    print_common_block(stdout, font.Common);
    print_pages_block(stdout, font.Pages, font.NumPages, font.PageLength);
    print_chars_block(stdout, font.Chars, font.NumGlyphs);
    print_kerning_block(stdout, font.Kerning, font.NumKerning);

    free(fnt_data);
    exit(EXIT_SUCCESS);
}
