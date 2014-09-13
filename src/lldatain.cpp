/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements some functions for parsing a limited set of data
/// formats so you can quickly get some data into your application. Currently
/// supported formats are DDS (for image data), WAV (for sound data) and JSON.
/// The data should be loaded into memory and passed to the parsing routines.
/// The data is typically parsed in-place, and may be modified.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lldatain.hpp"

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary Abstract platform differences for standard buffered file I/O.
#ifdef _MSC_VER
    #define STAT64_STRUCT struct __stat64
    #define STAT64_FUNC   _stat64
    #define FTELLO_FUNC   _ftelli64
    #define FSEEKO_FUNC   _fseeki64
#else
    #define STAT64_STRUCT struct stat
    #define STAT64_FUNC   stat
    #define FTELLO_FUNC   ftello
    #define FSEEKO_FUNC   fseeko
#endif /* defined(_MSC_VER) */

/// @summary A string defining the valid characters in a base-64 encoding.
/// This table is used when encoding binary data to base-64.
static char const        Base64_Chars[]   =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/// @summary A lookup table mapping the 256 possible values of a byte to a
/// value in [0, 63] (or -1 for invalid values in a base-64 encoding.) This
/// table is used when decoding base64-encoded binary data.
static signed char const Base64_Indices[] =
{

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 62, -1, -1, -1, 63,  /* ... , '+', ... '/' */
    52, 53, 54, 55, 56, 57, 58, 59,  /* '0' - '7'          */
    60, 61, -1, -1, -1, -1, -1, -1,  /* '8', '9', ...      */

    -1, 0,  1,  2,  3,  4,  5,  6,   /* ..., 'A' - 'G'     */
     7, 8,  9,  10, 11, 12, 13, 14,  /* 'H' - 'O'          */
    15, 16, 17, 18, 19, 20, 21, 22,  /* 'P' - 'W'          */
    23, 24, 25, -1, -1, -1, -1, -1,  /* 'X', 'Y', 'Z', ... */

    -1, 26, 27, 28, 29, 30, 31, 32,  /* ..., 'a' - 'g'     */
    33, 34, 35, 36, 37, 38, 39, 40,  /* 'h' - 'o'          */
    41, 42, 43, 44, 45, 46, 47, 48,  /* 'p' - 'w'          */
    49, 50, 51, -1, -1, -1, -1, -1,  /* 'x', 'y', 'z', ... */

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1
};

/*///////////////////////
//   Local Functions   //
///////////////////////*/
static uint64_t file_size(FILE *file)
{
    int64_t curr  = FTELLO_FUNC(file);
    FSEEKO_FUNC(file, 0,    SEEK_END);
    int64_t endp  = FTELLO_FUNC(file);
    FSEEKO_FUNC(file, curr, SEEK_SET);
    return uint64_t(endp);
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
size_t data::bom(int32_t encoding, uint8_t out_BOM[4])
{
    size_t  bom_size = 0;
    switch (encoding)
    {
        case data::TEXT_ENCODING_UTF8:
            {
                bom_size   = 3;
                out_BOM[0] = 0xEF;
                out_BOM[1] = 0xBB;
                out_BOM[2] = 0xBF;
                out_BOM[3] = 0x00;
            }
            break;

        case data::TEXT_ENCODING_UTF16_BE:
            {
                bom_size   = 2;
                out_BOM[0] = 0xFE;
                out_BOM[1] = 0xFF;
                out_BOM[2] = 0x00;
                out_BOM[3] = 0x00;
            }
            break;

        case data::TEXT_ENCODING_UTF16_LE:
            {
                bom_size   = 2;
                out_BOM[0] = 0xFF;
                out_BOM[1] = 0xFE;
                out_BOM[2] = 0x00;
                out_BOM[3] = 0x00;
            }
            break;

        case data::TEXT_ENCODING_UTF32_BE:
            {
                bom_size   = 4;
                out_BOM[0] = 0x00;
                out_BOM[1] = 0x00;
                out_BOM[2] = 0xFE;
                out_BOM[3] = 0xFF;
            }
            break;

        case data::TEXT_ENCODING_UTF32_LE:
            {
                bom_size   = 4;
                out_BOM[0] = 0xFF;
                out_BOM[1] = 0xFE;
                out_BOM[2] = 0x00;
                out_BOM[3] = 0x00;
            }
            break;

        default:
            {
                // no byte order marker.
                bom_size   = 0;
                out_BOM[0] = 0;
                out_BOM[1] = 0;
                out_BOM[2] = 0;
                out_BOM[3] = 0;
            }
            break;
    }
    return bom_size;
}

int32_t data::encoding(uint8_t BOM[4], size_t *out_size)
{
    size_t  bom_size = 0;
    int32_t text_enc = data::TEXT_ENCODING_UNSURE;

    if (0 == BOM[0])
    {
        if (0 == BOM[1] && 0xFE == BOM[2] && 0xFF == BOM[3])
        {
            // UTF32 big-endian.
            bom_size = 4;
            text_enc = data::TEXT_ENCODING_UTF32_BE;
        }
        else
        {
            // no BOM (or unrecognized).
            bom_size = 0;
            text_enc = data::TEXT_ENCODING_UNSURE;
        }
    }
    else if (0xFF == BOM[0])
    {
        if (0xFE == BOM[1])
        {
            if (0 == BOM[2] && 0 == BOM[3])
            {
                // UTF32 little-endian.
                bom_size = 4;
                text_enc = data::TEXT_ENCODING_UTF32_LE;
            }
            else
            {
                // UTF16 little-endian.
                bom_size = 2;
                text_enc = data::TEXT_ENCODING_UTF16_LE;
            }
        }
        else
        {
            // no BOM (or unrecognized).
            bom_size = 0;
            text_enc = data::TEXT_ENCODING_UNSURE;
        }
    }
    else if (0xFE == BOM[0] && 0xFF == BOM[1])
    {
        // UTF16 big-endian.
        bom_size = 2;
        text_enc = data::TEXT_ENCODING_UTF16_BE;
    }
    else if (0xEF == BOM[0] && 0xBB == BOM[1] && 0xBF == BOM[2])
    {
        // UTF-8.
        bom_size = 3;
        text_enc = data::TEXT_ENCODING_UTF8;
    }
    else
    {
        // no BOM (or unrecognized).
        bom_size = 0;
        text_enc = data::TEXT_ENCODING_UNSURE;
    }

    if (out_size != NULL)
    {
        // store the BOM size in bytes.
        *out_size = bom_size;
    }
    return text_enc;
}

size_t data::base64_size(size_t binary_size, size_t *out_pad_size)
{
    // base64 transforms 3 input bytes into 4 output bytes.
    // pad the binary size so it is evenly divisible by 3.
    size_t rem = binary_size % 3;
    size_t adj = (rem != 0)  ? 3 - rem : 0;
    if (out_pad_size  != 0)  *out_pad_size = adj;
    return ((binary_size + adj) / 3) * 4 + 1; // +1 for NULL
}

size_t data::binary_size(size_t base64_size, size_t pad_size)
{
    return (((3 * base64_size) / 4) - pad_size);
}

size_t data::binary_size(char const *base64_source, size_t base64_length)
{
    if (NULL == base64_source || 0 == base64_length)
    {
        // zero-length input - zero-length output.
        return 0;
    }

    // end points at the last character in the base64 source data.
    char const *end = base64_source + base64_length - 1;
    size_t      pad = 0;
    if (base64_length >= 1 && '=' == *end--)  ++pad;
    if (base64_length >= 2 && '=' == *end--)  ++pad;
    return data::binary_size(base64_length, pad);
}

size_t data::base64_encode(char *dst, size_t dst_size, void const *src, size_t src_size)
{
    size_t         pad    = 0;
    size_t         req    = data::base64_size(src_size, &pad);
    size_t         ins    = src_size;
    uint8_t const *inp    = (uint8_t const*) src;
    char          *outp   = dst;
    uint8_t        buf[4] = {0};

    if (dst_size < req)
    {
        // insufficient space in buffer.
        return 0;
    }

    // process input three bytes at a time.
    while (ins >= 3)
    {
        // buf[0] = left  6 bits of inp[0].
        // buf[1] = right 2 bits of inp[0], left 4 bits of inp[1].
        // buf[2] = right 4 bits of inp[1], left 2 bits of inp[2].
        // buf[3] = right 6 bits of inp[2].
        buf[0]  = (uint8_t)  ((inp[0] & 0xFC) >> 2);
        buf[1]  = (uint8_t) (((inp[0] & 0x03) << 4) + ((inp[1] & 0xF0) >> 4));
        buf[2]  = (uint8_t) (((inp[1] & 0x0F) << 2) + ((inp[2] & 0xC0) >> 6));
        buf[3]  = (uint8_t)   (inp[2] & 0x3F);
        // produce four bytes of output from three bytes of input.
        *outp++ = Base64_Chars[buf[0]];
        *outp++ = Base64_Chars[buf[1]];
        *outp++ = Base64_Chars[buf[2]];
        *outp++ = Base64_Chars[buf[3]];
        // we've consumed and processed three bytes of input.
        inp    += 3;
        ins    -= 3;
    }
    // pad any remaining input (either 1 or 2 bytes) up to three bytes; encode.
    if (ins > 0)
    {
        uint8_t src[3];
        size_t  i  = 0;

        // copy remaining real bytes from input; pad with nulls.
        for (i = 0; i  < ins; ++i) src[i] = *inp++;
        for (     ; i != 3;   ++i) src[i] = 0;
        // buf[0] = left  6 bits of inp[0].
        // buf[1] = right 2 bits of inp[0], left 4 bits of inp[1].
        // buf[2] = right 4 bits of inp[1], left 2 bits of inp[2].
        // buf[3] = right 6 bits of inp[2].
        buf[0]  = (uint8_t)  ((src[0] & 0xFC) >> 2);
        buf[1]  = (uint8_t) (((src[0] & 0x03) << 4) + ((src[1] & 0xF0) >> 4));
        buf[2]  = (uint8_t) (((src[1] & 0x0F) << 2) + ((src[2] & 0xC0) >> 6));
        buf[3]  = (uint8_t)   (src[2] & 0x3F);
        // produce four bytes of output from three bytes of input.
        *(outp+0) = Base64_Chars[buf[0]];
        *(outp+1) = Base64_Chars[buf[1]];
        *(outp+2) = Base64_Chars[buf[2]];
        *(outp+3) = Base64_Chars[buf[3]];
        // overwrite the junk characters with '=' characters.
        for (outp += 1 + ins; ins++ != 3;)  *outp++ = '=';
    }
    // always append the trailing null.
    *outp++ = '\0';
    // return the number of bytes written.
    return ((size_t)(outp - dst));
}

size_t data::base64_decode(void *dst, size_t dst_size, char const *src, size_t src_size)
{
    char const *inp    = src;
    char const *end    = src + src_size;
    signed char idx[4] = {0};
    uint8_t    *outp   = (uint8_t*) dst;
    size_t      curr   =  0;
    size_t      pad    =  0;
    size_t      req    = data::binary_size(src_size, 0);

    if (dst_size < (req - 2))
    {
        // insufficient space in buffer.
        return 0;
    }

    while (inp != end)
    {
        char ch = *inp++;
        if (ch != '=')
        {
            signed char chi = Base64_Indices[(unsigned char)ch];
            if (chi != -1)
            {
                // valid character, buffer it.
                idx[curr++] = chi;
                pad         = 0;
            }
            else continue; // unknown character, skip it.
        }
        else
        {
            // this is a padding character.
            idx[curr++] = 0;
            ++pad;
        }

        if (4 == curr)
        {
            // we've read three bytes of data; generate output.
            curr     = 0;
            *outp++  = (uint8_t) ((idx[0] << 2) + ((idx[1] & 0x30) >> 4));
            if (pad != 2)
            {
                *outp++  = (uint8_t) (((idx[1] & 0xF) << 4) + ((idx[2] & 0x3C) >> 2));
                if (pad != 1)
                {
                    *outp++ = (uint8_t) (((idx[2] & 0x3) << 6) + idx[3]);
                }
            }
            if (pad != 0) break;
        }
    }
    // return the number of bytes written.
    return ((size_t)(outp - ((uint8_t*)dst)));
}

char* data::load_text(char const *path, size_t *out_buffer_size, data::text_encoding_e *out_encoding)
{
    FILE *fp = fopen(path, "rb");
    if (fp != NULL)
    {
        size_t  size   = (size_t) file_size(fp);
        uint8_t bom[4] = {0};
        if (size == 0)
        {
            // the file exists, but is empty. return an empty string.
            if (out_buffer_size) *out_buffer_size = 0;
            if (out_encoding) *out_encoding = data::TEXT_ENCODING_UNSURE;
            char *buffer = (char*) malloc(sizeof(uint32_t));
            *((uint32_t*)buffer) = 0;
            fclose(fp);
            return buffer;
        }

        // determine the file encoding. this is a small read.
        size_t offset = 0;
        size_t nbom   = size >= 4 ? 4 : size;
        size_t nread  = fread(bom, 1, nbom, fp);
        if (nread != nbom)
        {
            // there was a problem reading the file; fail.
            if (out_buffer_size) *out_buffer_size = 0;
            if (out_encoding) *out_encoding = data::TEXT_ENCODING_UNSURE;
            fclose(fp);
            return NULL;
        }
        int32_t enc = data::encoding(bom, &nbom);
        if (out_encoding) *out_encoding = (data::text_encoding_e) enc;
        size -= nbom;

        // allocate the output buffer, with an extra zero word.
        char *buffer = (char*) malloc(size + sizeof(uint32_t));
        if (buffer == NULL)
        {
            if (out_buffer_size) *out_buffer_size = 0;
            fclose(fp);
            return NULL;
        }
        if (out_buffer_size) *out_buffer_size = size;

        // fill the output buffer with data from the file.
        if (nbom < nread)
        {
            // we read a portion of the data into the bom buffer.
            // copy the data from bom to the output buffer.
            for (size_t i = nbom; i < nread; ++i)
            {
                buffer[offset++] = bom[i];
            }
        }
        while (offset < size)
        {
            nread   = fread(&buffer[offset], 1, size - offset, fp);
            offset += nread;
            if (nread == 0)
            {
                if (feof(fp))
                {
                    // end of file reached unexpectedly. not an error.
                    if (out_buffer_size) *out_buffer_size = offset;
                    break;
                }
                else
                {
                    // there was an error reading the file, so fail.
                    if (out_buffer_size) *out_buffer_size = 0;
                    free(buffer);
                    fclose(fp);
                    return NULL;
                }
            }
        }
        *((uint32_t*)&buffer[offset]) = 0;
        fclose(fp);
        return buffer;
    }
    else
    {
        if (out_buffer_size) *out_buffer_size = 0;
        if (out_encoding) *out_encoding = data::TEXT_ENCODING_UNSURE;
        return NULL; // unable to open the file for reading.
    }
}

void* data::load_binary(char const *path, size_t *out_buffer_size)
{
    FILE *fp = fopen(path, "rb");
    if (fp != NULL)
    {
        size_t offset = 0;
        size_t nread  = 0;
        size_t size   = (size_t) file_size(fp);
        char *buffer  = (char *) malloc(size);
        if (buffer == NULL)
        {
            if (out_buffer_size) *out_buffer_size = 0;
            fclose(fp);
            return NULL;
        }
        if (out_buffer_size) *out_buffer_size = size;

        while (offset < size)
        {
            nread   = fread(&buffer[offset], 1, size - offset, fp);
            offset += nread;
            if (nread == 0)
            {
                if (feof(fp))
                {
                    // end of file reached unexpectedly.
                    if (out_buffer_size) *out_buffer_size = offset;
                    break;
                }
                else
                {
                    // there was an error reading the file, so fail.
                    if (out_buffer_size) *out_buffer_size = 0;
                    free(buffer);
                    fclose(fp);
                    return NULL;
                }
            }
        }
        fclose(fp);
        return buffer;
    }
    else
    {
        if (out_buffer_size) *out_buffer_size = 0;
        return NULL;
    }
}
