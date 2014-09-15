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
/// @summary Determines the size of a file. Supports file sizes larger than 4GB.
/// @param file The file pointer to query.
/// @return The size of the file, in bytes.
static uint64_t file_size(FILE *file)
{
    int64_t curr  = FTELLO_FUNC(file);
    FSEEKO_FUNC(file, 0,    SEEK_END);
    int64_t endp  = FTELLO_FUNC(file);
    FSEEKO_FUNC(file, curr, SEEK_SET);
    return uint64_t(endp);
}

/// @summary Utility function to return a pointer to the data at a given byte
/// offset from the start of a buffer, cast to the desired type.
/// @param buf Pointer to the buffer.
/// @param offset The byte offset relative to the buffer pointer.
/// @return A pointer to the data at the specified byte offset.
template <typename T>
static T const* data_at(void const *buf, ptrdiff_t offset)
{
    uint8_t const *base_ptr = (uint8_t const*) buf;
    uint8_t const *data_ptr = base_ptr + offset;
    return (T const*) data_ptr;
}

/// @summary Returns the minimum of two values.
/// @param a...b The input values.
/// @return The larger of the input values.
template<class T>
static const T& min2(const T& a, const T& b)
{
    return (a < b) ? a : b;
}

/// @summary Returns the maximum of two values.
/// @param a...b The input values.
/// @return The larger of the input values.
template<class T>
static const T& max2(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

/// @summary Calculates the dimension of a mipmap level. This function may be
/// used to calculate the width, height or depth dimensions.
/// @param dimension The dimension of the highest-resolution level (level 0.)
/// @param level_index The zero-based index of the miplevel to compute.
/// @return The corresponding dimension of the specified mipmap level.
static inline size_t level_dimension(size_t dimension, size_t level_index)
{
    size_t  l_dimension  = dimension >> level_index;
    return (l_dimension == 0) ? 1 : l_dimension;
}

/// @summary Calculates the dimension of an image, in pixels, and accounting
/// for block compression. Note that only width and height should be calculated
/// using this logic as block compression is 2D-only.
/// @param format One of data::dxgi_format_e.
/// @param dimension The width or height of an image.
/// @return The width or height, in pixels.
static inline size_t image_dimension(uint32_t format, size_t dimension)
{
    if (data::dds_block_compressed(format))
    {
        // all BC formats encode 4x4 blocks.
        return max2<size_t>(1, ((dimension + 3) / 4) * 4);
    }
    else return max2<size_t>(1, dimension);
}

/// @summary Locate a specific RIFF chunk within the file data.
/// @param start The starting point of the search. This address should be the
/// start of a RIFF chunk header.
/// @param end The pointer representing the end of the file or search space.
/// @param id The chunk identifier to search for.
/// @return A pointer to the start of the chunk header, or NULL.
static uint8_t const* find_chunk(void const *start, void const *end, uint32_t id)
{
    size_t const  hsize = sizeof(data::riff_chunk_header_t);
    uint8_t const *iter = (uint8_t const*) start;
    while (iter < end)
    {
        data::riff_chunk_header_t const *head = (data::riff_chunk_header_t const*) iter;
        if (head->ChunkId == id)
            return iter;

        iter += hsize + head->DataSize; // move to the next chunk start
        if (size_t(iter) & 1) iter++;   // chunks start on an even address
    }
    return NULL;
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

bool data::dds_header(void const *data, size_t data_size, data::dds_header_t *out_header)
{
    size_t const offset   = sizeof(uint32_t);
    size_t const min_size = offset + sizeof(data::dds_header_t);

    if (data == NULL)
    {
        // no input data specified.
        return false;
    }
    if (data_size < min_size)
    {
        // not enough input data specified.
        return false;
    }

    uint32_t magic = *((uint32_t const*) data);
    if (magic != LLDATAIN_DDS_MAGIC_LE)
    {
        // invalid magic bytes, we expect 'DDS '.
        return false;
    }

    if (out_header)
    {
        *out_header = *(data_at<dds_header_t>(data, offset));
    }
    return true;
}

bool data::dds_header_dxt10(void const *data, size_t data_size, data::dds_header_dxt10_t *out_header)
{
    data::dds_header_t header;
    if (data::dds_header(data, data_size, &header))
    {
        size_t const    offset   = sizeof(uint32_t) + sizeof(data::dds_header_t);
        size_t const    min_size = offset + sizeof(data::dds_header_dxt10_t);
        if (data_size < min_size)
        {
            // not enough input data specified.
            return false;
        }

        if ((header.Format.Flags & data::DDPF_FOURCC) == 0)
        {
            // no fourCC present on this file.
            return false;
        }

        if (header.Format.FourCC != data::fourcc_le('D','X','1','0'))
        {
            // the expected DX10 fourCC isn't present, so there's no header.
            return false;
        }

        if (out_header)
        {
            *out_header = *(data_at<data::dds_header_dxt10_t>(data, offset));
        }
        return true;
    }
    else return false;
}

uint32_t data::dds_format(data::dds_header_t const *header, data::dds_header_dxt10_t const *header_ex)
{
    if (header_ex != NULL)
    {
        return header_ex->Format;
    }
    if (header == NULL)
    {
        return data::DXGI_FORMAT_UNKNOWN;
    }

    data::dds_pixelformat_t const &pf = header->Format;
    #define ISBITMASK(r, g, b, a) (pf.BitMaskR == (r) && pf.BitMaskG == (g) && pf.BitMaskB == (b) && pf.BitMaskA == (a))

    if (pf.Flags & data::DDPF_FOURCC)
    {
        if (pf.FourCC == data::fourcc_le('D','X','T','1'))
        {
            return data::DXGI_FORMAT_BC1_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('D','X','T','2'))
        {
            return data::DXGI_FORMAT_BC2_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('D','X','T','3'))
        {
            return data::DXGI_FORMAT_BC2_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('D','X','T','4'))
        {
            return data::DXGI_FORMAT_BC3_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('D','X','T','5'))
        {
            return data::DXGI_FORMAT_BC3_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('A','T','I','1'))
        {
            return data::DXGI_FORMAT_BC4_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('A','T','I','2'))
        {
            return data::DXGI_FORMAT_BC5_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('B','C','4','U'))
        {
            return data::DXGI_FORMAT_BC4_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('B','C','4','S'))
        {
            return data::DXGI_FORMAT_BC4_SNORM;
        }
        if (pf.FourCC == data::fourcc_le('B','C','5','U'))
        {
            return data::DXGI_FORMAT_BC5_UNORM;
        }
        if (pf.FourCC == data::fourcc_le('B','C','5','S'))
        {
            return data::DXGI_FORMAT_BC5_SNORM;
        }
        switch (pf.FourCC)
        {
            case 36: // D3DFMT_A16B16G16R16
                return data::DXGI_FORMAT_R16G16B16A16_UNORM;

            case 110: // D3DFMT_Q16W16V16U16
                return data::DXGI_FORMAT_R16G16B16A16_SNORM;

            case 111: // D3DFMT_R16F
                return data::DXGI_FORMAT_R16_FLOAT;

            case 112: // D3DFMT_G16R16F
                return data::DXGI_FORMAT_R16G16_FLOAT;

            case 113: // D3DFMT_A16B16G16R16F
                return data::DXGI_FORMAT_R16G16B16A16_FLOAT;

            case 114: // D3DFMT_R32F
                return data::DXGI_FORMAT_R32_FLOAT;

            case 115: // D3DFMT_G32R32F
                return data::DXGI_FORMAT_R32G32_FLOAT;

            case 116: // D3DFMT_A32B32G32R32F
                return data::DXGI_FORMAT_R32G32B32A32_FLOAT;

            default:
                break;
        }
        return data::DXGI_FORMAT_UNKNOWN;
    }
    if (pf.Flags & data::DDPF_RGB)
    {
        switch (pf.RGBBitCount)
        {
            case 32:
                {
                    if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                    {
                        return data::DXGI_FORMAT_R8G8B8A8_UNORM;
                    }
                    if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
                    {
                        return data::DXGI_FORMAT_B8G8R8A8_UNORM;
                    }
                    if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
                    {
                        return data::DXGI_FORMAT_B8G8R8X8_UNORM;
                    }

                    // No DXGI format maps to ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000) aka D3DFMT_X8B8G8R8
                    // Note that many common DDS reader/writers (including D3DX) swap the the RED/BLUE masks for 10:10:10:2
                    // formats. We assumme below that the 'backwards' header mask is being used since it is most likely
                    // written by D3DX. The more robust solution is to use the 'DX10' header extension and specify the
                    // DXGI_FORMAT_R10G10B10A2_UNORM format directly.

                    // For 'correct' writers, this should be 0x000003ff, 0x000ffc00, 0x3ff00000 for RGB data.
                    if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
                    {
                        return data::DXGI_FORMAT_R10G10B10A2_UNORM;
                    }
                    // No DXGI format maps to ISBITMASK(0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000) aka D3DFMT_A2R10G10B10.
                    if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
                    {
                        return data::DXGI_FORMAT_R16G16_UNORM;
                    }
                    if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
                    {
                        // Only 32-bit color channel format in D3D9 was R32F
                        return data::DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114.
                    }
                }
                break;

            case 24:
                // No 24bpp DXGI formats aka D3DFMT_R8G8B8
                break;

            case 16:
                {
                    if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
                    {
                        return data::DXGI_FORMAT_B5G5R5A1_UNORM;
                    }
                    if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
                    {
                        return data::DXGI_FORMAT_B5G6R5_UNORM;
                    }
                    // No DXGI format maps to ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x0000) aka D3DFMT_X1R5G5B5.
                    if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
                    {
                        return data::DXGI_FORMAT_B4G4R4A4_UNORM;
                    }
                    // No DXGI format maps to ISBITMASK(0x0f00, 0x00f0, 0x000f, 0x0000) aka D3DFMT_X4R4G4B4.
                    // No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
                }
                break;
        }
    }
    if (pf.Flags & data::DDPF_ALPHA)
    {
        if (pf.RGBBitCount == 8)
        {
            return data::DXGI_FORMAT_A8_UNORM;
        }
    }
    if (pf.Flags & data::DDPF_LUMINANCE)
    {
        if (pf.RGBBitCount == 8)
        {
            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
            {
                // D3DX10/11 writes this out as DX10 extension.
                return data::DXGI_FORMAT_R8_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x0f, 0x00, 0x00, 0xf0) aka D3DFMT_A4L4
        }
        if (pf.RGBBitCount == 16)
        {
            if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
            {
                // D3DX10/11 writes this out as DX10 extension.
                return data::DXGI_FORMAT_R16_UNORM;
            }
            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
            {
                // D3DX10/11 writes this out as DX10 extension
                return data::DXGI_FORMAT_R8G8_UNORM;
            }
        }
    }
    #undef ISBITMASK
    return data::DXGI_FORMAT_UNKNOWN;
}

size_t data::dds_pitch(uint32_t format, uint32_t width)
{
    if (data::dds_block_compressed(format))
    {
        size_t bw = max2<size_t>(1, (width + 3) / 4);
        return bw * data::dds_bytes_per_block(format);
    }
    if (data::dds_packed(format))
    {
        return ((width + 1) >> 1) * 4;
    }
    return (width * data::dds_bits_per_pixel(format) + 7) / 8;
}

bool data::dds_block_compressed(uint32_t format)
{
    switch (format)
    {
        case data::DXGI_FORMAT_BC1_TYPELESS:
        case data::DXGI_FORMAT_BC1_UNORM:
        case data::DXGI_FORMAT_BC1_UNORM_SRGB:
        case data::DXGI_FORMAT_BC4_TYPELESS:
        case data::DXGI_FORMAT_BC4_UNORM:
        case data::DXGI_FORMAT_BC4_SNORM:
        case data::DXGI_FORMAT_BC2_TYPELESS:
        case data::DXGI_FORMAT_BC2_UNORM:
        case data::DXGI_FORMAT_BC2_UNORM_SRGB:
        case data::DXGI_FORMAT_BC3_TYPELESS:
        case data::DXGI_FORMAT_BC3_UNORM:
        case data::DXGI_FORMAT_BC3_UNORM_SRGB:
        case data::DXGI_FORMAT_BC5_TYPELESS:
        case data::DXGI_FORMAT_BC5_UNORM:
        case data::DXGI_FORMAT_BC5_SNORM:
        case data::DXGI_FORMAT_BC6H_TYPELESS:
        case data::DXGI_FORMAT_BC6H_UF16:
        case data::DXGI_FORMAT_BC6H_SF16:
        case data::DXGI_FORMAT_BC7_TYPELESS:
        case data::DXGI_FORMAT_BC7_UNORM:
        case data::DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;

        default:
            break;
    }
    return false;
}

bool data::dds_packed(uint32_t format)
{
    if (format == data::DXGI_FORMAT_R8G8_B8G8_UNORM ||
        format == data::DXGI_FORMAT_G8R8_G8B8_UNORM)
    {
        return true;
    }
    else return false;
}

bool data::dds_cubemap(data::dds_header_t const *header, data::dds_header_dxt10_t const *header_ex)
{
    if (header_ex)
    {
        if (header_ex->Dimension == data::D3D11_RESOURCE_DIMENSION_TEXTURE2D &&
            header_ex->Flags     == data::D3D11_RESOURCE_MISC_TEXTURECUBE)
        {
            return true;
        }
        // else fall through and look at the dds_header_t.
    }
    if (header)
    {
        if ((header->Caps  & data::DDSCAPS_COMPLEX) == 0)
            return false;
        if ((header->Caps2 & data::DDSCAPS2_CUBEMAP) == 0)
            return false;
        if ((header->Caps2 & data::DDSCAPS2_CUBEMAP_POSITIVEX) ||
            (header->Caps2 & data::DDSCAPS2_CUBEMAP_NEGATIVEX) ||
            (header->Caps2 & data::DDSCAPS2_CUBEMAP_POSITIVEY) ||
            (header->Caps2 & data::DDSCAPS2_CUBEMAP_NEGATIVEY) ||
            (header->Caps2 & data::DDSCAPS2_CUBEMAP_POSITIVEZ) ||
            (header->Caps2 & data::DDSCAPS2_CUBEMAP_NEGATIVEZ))
            return true;
    }
    return false;
}

bool data::dds_volume(data::dds_header_t const *header, data::dds_header_dxt10_t const *header_ex)
{
    if (header_ex)
    {
        if (header_ex->ArraySize != 1)
        {
            // arrays of volumes are not supported.
            return false;
        }
    }
    if (header)
    {
        if ((header->Caps  & data::DDSCAPS_COMPLEX) == 0)
            return false;
        if ((header->Caps2 & data::DDSCAPS2_VOLUME) == 0)
            return false;
        if ((header->Flags & data::DDSD_DEPTH) == 0)
            return false;
        return header->Depth > 1;
    }
    return false;
}

bool data::dds_array(data::dds_header_t const *header, data::dds_header_dxt10_t const *header_ex)
{
    if (header && header_ex)
    {
        return header_ex->ArraySize > 1;
    }
    return false;
}

bool data::dds_mipmap(data::dds_header_t const *header, data::dds_header_dxt10_t const *header_ex)
{
    if (header_ex)
    {
        if (header_ex->Dimension != data::D3D11_RESOURCE_DIMENSION_TEXTURE1D &&
            header_ex->Dimension != data::D3D11_RESOURCE_DIMENSION_TEXTURE2D &&
            header_ex->Dimension != data::D3D11_RESOURCE_DIMENSION_TEXTURE3D)
            return false;
    }
    if (header)
    {
        if ((header->Caps  & DDSCAPS_COMPLEX) == 0)
            return false;
        if ((header->Caps  & DDSCAPS_MIPMAP) == 0)
            return false;
        if ((header->Flags & DDSD_MIPMAPCOUNT) == 0)
            return false;

        return header->Levels > 0;
    }
    return false;
}

size_t data::dds_bits_per_pixel(uint32_t format)
{
    switch (format)
    {
        case data::DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case data::DXGI_FORMAT_R32G32B32A32_FLOAT:
        case data::DXGI_FORMAT_R32G32B32A32_UINT:
        case data::DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;

        case data::DXGI_FORMAT_R32G32B32_TYPELESS:
        case data::DXGI_FORMAT_R32G32B32_FLOAT:
        case data::DXGI_FORMAT_R32G32B32_UINT:
        case data::DXGI_FORMAT_R32G32B32_SINT:
            return 96;

        case data::DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case data::DXGI_FORMAT_R16G16B16A16_FLOAT:
        case data::DXGI_FORMAT_R16G16B16A16_UNORM:
        case data::DXGI_FORMAT_R16G16B16A16_UINT:
        case data::DXGI_FORMAT_R16G16B16A16_SNORM:
        case data::DXGI_FORMAT_R16G16B16A16_SINT:
        case data::DXGI_FORMAT_R32G32_TYPELESS:
        case data::DXGI_FORMAT_R32G32_FLOAT:
        case data::DXGI_FORMAT_R32G32_UINT:
        case data::DXGI_FORMAT_R32G32_SINT:
        case data::DXGI_FORMAT_R32G8X24_TYPELESS:
        case data::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case data::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case data::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return 64;

        case data::DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case data::DXGI_FORMAT_R10G10B10A2_UNORM:
        case data::DXGI_FORMAT_R10G10B10A2_UINT:
        case data::DXGI_FORMAT_R11G11B10_FLOAT:
        case data::DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case data::DXGI_FORMAT_R8G8B8A8_UNORM:
        case data::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case data::DXGI_FORMAT_R8G8B8A8_UINT:
        case data::DXGI_FORMAT_R8G8B8A8_SNORM:
        case data::DXGI_FORMAT_R8G8B8A8_SINT:
        case data::DXGI_FORMAT_R16G16_TYPELESS:
        case data::DXGI_FORMAT_R16G16_FLOAT:
        case data::DXGI_FORMAT_R16G16_UNORM:
        case data::DXGI_FORMAT_R16G16_UINT:
        case data::DXGI_FORMAT_R16G16_SNORM:
        case data::DXGI_FORMAT_R16G16_SINT:
        case data::DXGI_FORMAT_R32_TYPELESS:
        case data::DXGI_FORMAT_D32_FLOAT:
        case data::DXGI_FORMAT_R32_FLOAT:
        case data::DXGI_FORMAT_R32_UINT:
        case data::DXGI_FORMAT_R32_SINT:
        case data::DXGI_FORMAT_R24G8_TYPELESS:
        case data::DXGI_FORMAT_D24_UNORM_S8_UINT:
        case data::DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case data::DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case data::DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case data::DXGI_FORMAT_R8G8_B8G8_UNORM:
        case data::DXGI_FORMAT_G8R8_G8B8_UNORM:
        case data::DXGI_FORMAT_B8G8R8A8_UNORM:
        case data::DXGI_FORMAT_B8G8R8X8_UNORM:
        case data::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case data::DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case data::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case data::DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case data::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return 32;

        case data::DXGI_FORMAT_R8G8_TYPELESS:
        case data::DXGI_FORMAT_R8G8_UNORM:
        case data::DXGI_FORMAT_R8G8_UINT:
        case data::DXGI_FORMAT_R8G8_SNORM:
        case data::DXGI_FORMAT_R8G8_SINT:
        case data::DXGI_FORMAT_R16_TYPELESS:
        case data::DXGI_FORMAT_R16_FLOAT:
        case data::DXGI_FORMAT_D16_UNORM:
        case data::DXGI_FORMAT_R16_UNORM:
        case data::DXGI_FORMAT_R16_UINT:
        case data::DXGI_FORMAT_R16_SNORM:
        case data::DXGI_FORMAT_R16_SINT:
        case data::DXGI_FORMAT_B5G6R5_UNORM:
        case data::DXGI_FORMAT_B5G5R5A1_UNORM:
        case data::DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;

        case data::DXGI_FORMAT_R8_TYPELESS:
        case data::DXGI_FORMAT_R8_UNORM:
        case data::DXGI_FORMAT_R8_UINT:
        case data::DXGI_FORMAT_R8_SNORM:
        case data::DXGI_FORMAT_R8_SINT:
        case data::DXGI_FORMAT_A8_UNORM:
            return 8;

        case data::DXGI_FORMAT_R1_UNORM:
            return 1;

        case data::DXGI_FORMAT_BC1_TYPELESS:
        case data::DXGI_FORMAT_BC1_UNORM:
        case data::DXGI_FORMAT_BC1_UNORM_SRGB:
        case data::DXGI_FORMAT_BC4_TYPELESS:
        case data::DXGI_FORMAT_BC4_UNORM:
        case data::DXGI_FORMAT_BC4_SNORM:
            return 4;

        case data::DXGI_FORMAT_BC2_TYPELESS:
        case data::DXGI_FORMAT_BC2_UNORM:
        case data::DXGI_FORMAT_BC2_UNORM_SRGB:
        case data::DXGI_FORMAT_BC3_TYPELESS:
        case data::DXGI_FORMAT_BC3_UNORM:
        case data::DXGI_FORMAT_BC3_UNORM_SRGB:
        case data::DXGI_FORMAT_BC5_TYPELESS:
        case data::DXGI_FORMAT_BC5_UNORM:
        case data::DXGI_FORMAT_BC5_SNORM:
        case data::DXGI_FORMAT_BC6H_TYPELESS:
        case data::DXGI_FORMAT_BC6H_UF16:
        case data::DXGI_FORMAT_BC6H_SF16:
        case data::DXGI_FORMAT_BC7_TYPELESS:
        case data::DXGI_FORMAT_BC7_UNORM:
        case data::DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;

        default:
            return 0;
    }
}

size_t data::dds_bytes_per_block(uint32_t format)
{
    switch (format)
    {
        case data::DXGI_FORMAT_BC1_TYPELESS:
        case data::DXGI_FORMAT_BC1_UNORM:
        case data::DXGI_FORMAT_BC1_UNORM_SRGB:
        case data::DXGI_FORMAT_BC4_TYPELESS:
        case data::DXGI_FORMAT_BC4_UNORM:
        case data::DXGI_FORMAT_BC4_SNORM:
            return 8;

        case data::DXGI_FORMAT_BC2_TYPELESS:
        case data::DXGI_FORMAT_BC2_UNORM:
        case data::DXGI_FORMAT_BC2_UNORM_SRGB:
        case data::DXGI_FORMAT_BC3_TYPELESS:
        case data::DXGI_FORMAT_BC3_UNORM:
        case data::DXGI_FORMAT_BC3_UNORM_SRGB:
        case data::DXGI_FORMAT_BC5_TYPELESS:
        case data::DXGI_FORMAT_BC5_UNORM:
        case data::DXGI_FORMAT_BC5_SNORM:
        case data::DXGI_FORMAT_BC6H_TYPELESS:
        case data::DXGI_FORMAT_BC6H_UF16:
        case data::DXGI_FORMAT_BC6H_SF16:
        case data::DXGI_FORMAT_BC7_TYPELESS:
        case data::DXGI_FORMAT_BC7_UNORM:
        case data::DXGI_FORMAT_BC7_UNORM_SRGB:
            return 16;

        default:
            break;
    }
    return 0;
}

size_t data::dds_array_count(data::dds_header_t const *header, data::dds_header_dxt10_t const *header_ex)
{
    if (header && header_ex)
    {
        // DX10 extended header is required for surface arrays.
        return size_t(header_ex->ArraySize);
    }
    else if (header) return 1;
    else return 0;
}

size_t data::dds_level_count(data::dds_header_t const *header, data::dds_header_dxt10_t const *header_ex)
{
    if (data::dds_mipmap(header, header_ex))
    {
        return header->Levels + 1;
    }
    else if (header) return 1;
    else return 0;
}

size_t data::dds_describe(
    void                     const *data,
    size_t                          data_size,
    data::dds_header_t       const *header,
    data::dds_header_dxt10_t const *header_ex,
    data::dds_level_desc_t         *out_levels,
    size_t                          max_levels)
{
    uint32_t  format = data::DXGI_FORMAT_UNKNOWN;
    uint8_t const *p = (uint8_t const*) data;
    size_t offset    = 0;
    size_t basew     = 0;
    size_t baseh     = 0;
    size_t based     = 0;
    size_t bitspp    = 0;
    size_t blocksz   = 0;
    size_t nitems    = data::dds_array_count(header, header_ex);
    size_t nlevels   = data::dds_level_count(header, header_ex);
    size_t dst_i     = 0;
    bool   bcn       = false;

    if (header == NULL)
    {
        // the base DDS header is required.
        return false;
    }

    // get some basic data about the DDS.
    format  = data::dds_format(header, header_ex);
    bitspp  = data::dds_bits_per_pixel(format);
    blocksz = data::dds_bytes_per_block(format);
    basew   = (header->Flags & data::DDSD_WIDTH)  ? header->Width  : 0;
    baseh   = (header->Flags & data::DDSD_HEIGHT) ? header->Height : 0;
    based   = data::dds_volume(header, header_ex) ? header->Depth  : 1;
    bcn     = (blocksz > 0);

    // now update the byte offset and data pointer, and write the output.
    offset += sizeof(uint32_t);
    offset += sizeof(data::dds_header_t);
    if (header_ex) offset += sizeof(data::dds_header_dxt10_t);
    for (size_t i = 0; i < nitems && dst_i < max_levels; ++i)
    {
        for (size_t j = 0; j < nlevels && dst_i < max_levels && offset < data_size; ++j)
        {
            data::dds_level_desc_t &dst = out_levels[dst_i++];
            size_t levelw        = level_dimension(basew , j);
            size_t levelh        = level_dimension(baseh , j);
            size_t leveld        = level_dimension(based , j);
            size_t levelp        = data::dds_pitch(format, levelw);
            size_t blockh        = max2<size_t>(1, (levelh + 3) / 4);
            dst.Index            = j;
            dst.Width            = image_dimension(format, levelw);
            dst.Height           = image_dimension(format, levelh);
            dst.Slices           = leveld;
            dst.BytesPerElement  = bcn ? blocksz : (bitspp / 8); // DXGI_FORMAT_R1_UNORM...?
            dst.BytesPerRow      = levelp;
            dst.BytesPerSlice    = bcn ? levelp * blockh : levelp * levelh;
            dst.DataSize         = dst.BytesPerSlice  * leveld;
            dst.LevelData        = (void*) (p + offset);
            dst.Format           = format;
            offset              += dst.DataSize;
        }
    }
    return dst_i;
}

size_t data::wav_describe(
    void const          *data,
    size_t               data_size,
    data::wave_format_t *out_desc,
    data::wave_data_t   *out_clips,
    size_t               max_clips)
{
    data::riff_chunk_header_t   *dc = NULL;
    data::wave_format_t *fmt        = NULL;
    data::riff_header_t *riff       = NULL;
    uint8_t const       *format_ptr = NULL;
    uint8_t const       *search_ptr = NULL;
    uint8_t const       *data_ptr   = NULL;
    uint8_t const       *base_ptr   = (uint8_t const*) data;
    uint8_t const       *end_ptr    = (uint8_t const*) data + data_size;
    size_t               min_size   = 0;
    size_t               clip_index = 0;
    float                duration   = 0.0f;

    min_size  = sizeof(data::riff_header_t);
    min_size += sizeof(data::riff_chunk_header_t) * 2;
    min_size += sizeof(data::wave_format_t);
    if (data == NULL || data_size < min_size)
        goto wave_error;

    riff = (data::riff_header_t*) base_ptr;
    if (riff->ChunkId  != data::fourcc_le('R','I','F','F'))
        goto wave_error;
    if (riff->RiffType != data::fourcc_le('W','A','V','E'))
        goto wave_error;

    search_ptr = base_ptr + sizeof(data::riff_header_t);
    format_ptr = find_chunk(search_ptr, end_ptr, data::fourcc_le('f','m','t',' '));
    if (format_ptr == NULL)
        goto wave_error;

    fmt = (data::wave_format_t*)  (format_ptr + sizeof(data::riff_chunk_header_t));
    if (fmt->CompressionType != data::WAVE_COMPRESSION_PCM)
        goto wave_unsupported;

    if (max_clips == 0)
    {
        // the caller didn't request any clip information.
        if (out_desc) *out_desc = *fmt;
        return 0;
    }

    // return information about any data clips we find, up to max_clips or EOF.
    data_ptr = format_ptr;
    while (data_ptr != NULL && data_ptr < end_ptr && clip_index < max_clips)
    {
        data_ptr = find_chunk(data_ptr, end_ptr, data::fourcc_le('d','a','t','a'));
        if (data_ptr)
        {
            data::riff_header_t *data_hdr = (data::riff_header_t*) data_ptr;
            data::wave_data_t   &clip     = out_clips[clip_index++];
            clip.DataSize     =  data_hdr->DataSize;
            clip.SampleCount  =  data_hdr->DataSize / (fmt->ChannelCount * (fmt->BitsPerSample / 8));
            clip.SampleData   = (void*)(data_ptr + sizeof(data::riff_header_t));
            clip.Duration     =  float (data_hdr->DataSize) / float(fmt->ChannelCount * (fmt->BitsPerSample / 8) * fmt->SampleRate);
        }
    }
    if (out_desc) *out_desc = *fmt;
    return clip_index;

wave_error:
    if (out_desc)
    {
        out_desc->CompressionType = data::WAVE_COMPRESSION_UNKNOWN;
        out_desc->ChannelCount    = 0;
        out_desc->SampleRate      = 0;
        out_desc->BytesPerSecond  = 0;
        out_desc->BlockAlignment  = 0;
        out_desc->BitsPerSample   = 0;
        out_desc->FormatDataSize  = 0;
        out_desc->FormatData[0]   = 0;
    }
    return 0;

wave_unsupported:
    if (out_desc) *out_desc = *fmt;
    return 0;
}
