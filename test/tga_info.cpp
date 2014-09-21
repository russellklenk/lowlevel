/*/////////////////////////////////////////////////////////////////////////////
/// @summary Loads and displays information about TGA files.
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
static const size_t CMAPTYPE_COUNT = 2;
static const data::tga_colormaptype_e CMAP_TYPES[CMAPTYPE_COUNT] =
{
    data::TGA_COLORMAPTYPE_NONE,
    data::TGA_COLORMAPTYPE_INCLUDED
};

static const size_t IMAGETYPE_COUNT = 7;
static const data::tga_imagetype_e IMAGE_TYPES[IMAGETYPE_COUNT] =
{
    data::TGA_IMAGETYPE_NO_IMAGE_DATA,
    data::TGA_IMAGETYPE_UNCOMPRESSED_PAL,
    data::TGA_IMAGETYPE_UNCOMPRESSED_TRUE,
    data::TGA_IMAGETYPE_UNCOMPRESSED_GRAY,
    data::TGA_IMAGETYPE_RLE_PAL,
    data::TGA_IMAGETYPE_RLE_TRUE,
    data::TGA_IMAGETYPE_RLE_GRAY
};

/*///////////////////////
//   Local Functions   //
///////////////////////*/
static char const* cmaptype_str(uint8_t cmap_type)
{
    switch (cmap_type)
    {
        case data::TGA_COLORMAPTYPE_NONE:
            return "TGA_COLORMAPTYPE_NONE";
        case data::TGA_COLORMAPTYPE_INCLUDED:
            return "TGA_COLORMAPTYPE_INCLUDED";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* imagetype_str(uint8_t attribs)
{
    switch (attribs)
    {
        case data::TGA_IMAGETYPE_NO_IMAGE_DATA:
            return "TGA_IMAGETYPE_NO_IMAGE_DATA";
        case data::TGA_IMAGETYPE_UNCOMPRESSED_PAL:
            return "TGA_IMAGETYPE_UNCOMPRESSED_PAL";
        case data::TGA_IMAGETYPE_UNCOMPRESSED_TRUE:
            return "TGA_IMAGETYPE_UNCOMPRESSED_TRUE";
        case data::TGA_IMAGETYPE_UNCOMPRESSED_GRAY:
            return "TGA_IMAGETYPE_UNCOMPRESSED_GRAY";
        case data::TGA_IMAGETYPE_RLE_PAL:
            return "TGA_IMAGETYPE_RLE_PAL";
        case data::TGA_IMAGETYPE_RLE_TRUE:
            return "TGA_IMAGETYPE_RLE_TRUE";
        case data::TGA_IMAGETYPE_RLE_GRAY:
            return "TGA_IMAGETYPE_RLE_GRAY";
        default:
            break;
    }
    return "UNKNOWN";
}

static void print_header(FILE *fp, data::tga_header_t const *head)
{
    if (head == NULL)
    {
        fprintf(fp, "TGA_HEADER:\n");
        fprintf(fp, "  Not present.\n");
        fprintf(fp, "\n");
        return;
    }

    fprintf(fp, "TGA_HEADER:\n");
    fprintf(fp, "  ID Length:      %u\n", unsigned(head->ImageIdLength));
    fprintf(fp, "  Colormap Type:  %s\n", cmaptype_str(head->ColormapType));
    fprintf(fp, "  Image Type:     %s\n", imagetype_str(head->ImageType));
    fprintf(fp, "  CmapFirstEntry: %u\n", unsigned(head->CmapFirstEntry));
    fprintf(fp, "  CmapLength:     %u\n", unsigned(head->CmapLength));
    fprintf(fp, "  CmapEntrySize:  %u\n", unsigned(head->CmapEntrySize));
    fprintf(fp, "  ImageXOrigin:   %u\n", unsigned(head->ImageXOrigin));
    fprintf(fp, "  ImageYOrigin:   %u\n", unsigned(head->ImageYOrigin));
    fprintf(fp, "  ImageWidth:     %u\n", unsigned(head->ImageWidth));
    fprintf(fp, "  ImageHeight:    %u\n", unsigned(head->ImageHeight));
    fprintf(fp, "  ImageBitDepth:  %u\n", unsigned(head->ImageBitDepth));
    fprintf(fp, "  ImageFlags:     ");
    for (int i = 7; i >= 0; --i)
    {
        fprintf(fp, "%c", head->ImageFlags & (1 << i) ? '1' : '0');
    }
    fprintf(fp, "b\n");
    fprintf(fp, "\n");
}

static void print_footer(FILE *fp, data::tga_footer_t const *foot)
{
    if (foot == NULL)
    {
        fprintf(fp, "TGA_FOOTER:\n");
        fprintf(fp, "  Not present.\n");
        fprintf(fp, "\n");
        return;
    }

    fprintf(fp, "TGA_HEADER:\n");
    fprintf(fp, "  ExtOffset:      %u\n", unsigned(foot->ExtOffset));
    fprintf(fp, "  DevOffset:      %u\n", unsigned(foot->DevOffset));
    fprintf(fp, "  Signature:      %s\n", foot->Signature);
    fprintf(fp, "\n");
}

static void print_desc(FILE *fp, data::tga_desc_t const *desc)
{
    if (desc == NULL)
    {
        fprintf(fp, "TGA_DESC:\n");
        fprintf(fp, "  Could not retrieve description.\n");
        fprintf(fp, "\n");
        return;
    }

    fprintf(fp, "TGA_DESC:\n");
    fprintf(fp, "  Colormap Type:  %s\n", cmaptype_str(desc->ColormapType));
    fprintf(fp, "  Image Type:     %s\n", imagetype_str(desc->ImageType));
    fprintf(fp, "  CmapFirstEntry: %u\n", unsigned(desc->CmapFirstEntry));
    fprintf(fp, "  CmapLength:     %u\n", unsigned(desc->CmapLength));
    fprintf(fp, "  CmapEntrySize:  %u\n", unsigned(desc->CmapEntrySize));
    fprintf(fp, "  OriginBottom:   %s\n", desc->OriginBottom ? "true" : "false");
    fprintf(fp, "  ImageWidth:     %u\n", unsigned(desc->ImageWidth));
    fprintf(fp, "  ImageHeight:    %u\n", unsigned(desc->ImageHeight));
    fprintf(fp, "  BitsPerPixel:   %u\n", unsigned(desc->BitsPerPixel));
    fprintf(fp, "  PixelDataSize:  %u\n", unsigned(desc->PixelDataSize));
    fprintf(fp, "  CmapDataSize:   %u\n", unsigned(desc->ColormapDataSize));
    fprintf(fp, "  Colormap Data:  %p\n", desc->ColormapData);
    fprintf(fp, "  Pixel Data:     %p\n", desc->PixelData);
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
        printf("USAGE: tgainfo path/to/file.tga\n");
        exit(EXIT_FAILURE);
    }

    size_t tga_size = 0;
    void  *tga_data = data::load_binary(argv[1], &tga_size);
    if (tga_data == NULL)
    {
        printf("ERROR: Input file \'%s\' not found.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("INFO:  Loaded \'%s\', %u bytes.\n", argv[1], uint32_t(tga_size));
    }

    data::tga_header_t head;
    if (!data::tga_header(tga_data, tga_size, &head))
    {
        free(tga_data);
        printf("ERROR: File does not appear to be a valid TGA.\n");
        exit(EXIT_FAILURE);
    }
    else print_header(stdout, &head);

    data::tga_footer_t foot;
    if (!data::tga_footer(tga_data, tga_size, &foot))
    {
        printf("INFO:  No TGA v2.0 footer found.\n");
    }
    else print_footer(stdout, &foot);

    data::tga_desc_t desc;
    if (!data::tga_describe(tga_data, tga_size, &desc))
    {
        free(tga_data);
        printf("ERROR: Could not retrieve TGA description.\n");
        exit(EXIT_FAILURE);
    }
    else print_desc(stdout, &desc);

    switch (desc.BitsPerPixel)
    {
        case 8:
            {
                uint8_t *pix = (uint8_t*) malloc(desc.PixelDataSize);
                if (!data::tga_decode_r8(pix, desc.PixelDataSize, &desc))
                {
                    printf("ERROR: Could not decode grayscale data.\n");
                }
                else
                {
                    printf("INFO:  Decoded grayscale data. First bytes:\n");
                    printf("%02X %02X %02X %02X\n", unsigned(pix[0]), unsigned(pix[1]), unsigned(pix[2]), unsigned(pix[3]));
                }
                free(pix);
            }
            break;

        case 24:
        case 32:
            {
                uint8_t *pix = (uint8_t*) malloc(desc.PixelDataSize);
                if (!data::tga_decode_argb32(pix, desc.PixelDataSize, &desc))
                {
                    printf("ERROR: Could not decode RGBA data.\n");
                }
                else
                {
                    uint32_t *p = (uint32_t*) pix;
                    printf("INFO:  Decoded RGBA data. First pixels:\n");
                    printf("%08X %08X %08X %08X\n", p[0], p[1], p[2], p[3]);
                }
                free(pix);
            }
            break;

        default:
            printf("INFO:  Unsupported TGA bit depth %u.\n", unsigned(desc.BitsPerPixel));
            break;
    }

    free(tga_data);
    exit(EXIT_SUCCESS);
}
