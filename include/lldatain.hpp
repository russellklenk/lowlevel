/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define some functions and types for parsing a limited set of data
/// formats so you can quickly get some data into your application. Currently
/// supported formats are DDS (for image data), WAV (for sound data) and JSON.
/// The data should be loaded into memory and passed to the parsing routines.
/// The data is typically parsed in-place, and may be modified.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LLDATAIN_HPP_INCLUDED
#define LLDATAIN_HPP_INCLUDED

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
    #define  LLDATAIN_CALL_C    __cdecl
    #if   defined(_MSC_VER)
    #define  LLDATAIN_IMPORT    __declspec(dllimport)
    #define  LLDATAIN_EXPORT    __declspec(dllexport)
    #elif defined(__GNUC__)
    #define  LLDATAIN_IMPORT    __attribute__((dllimport))
    #define  LLDATAIN_EXPORT    __attribute__((dllexport))
    #else
    #define  LLDATAIN_IMPORT
    #define  LLDATAIN_EXPORT
    #endif
#else
    #define  LLDATAIN_CALL_C
    #if __GNUC__ >= 4
    #define  LLDATAIN_IMPORT    __attribute__((visibility("default")))
    #define  LLDATAIN_EXPORT    __attribute__((visibility("default")))
    #endif
#endif

/// @summary Define import/export based on whether we're being used as a DLL.
#if defined(LLDATAIN_SHARED)
    #ifdef  LLDATAIN_EXPORTS
    #define LLDATAIN_PUBLIC     LLDATAIN_EXPORT
    #else
    #define LLDATAIN_PUBLIC     LLDATAIN_IMPORT
    #endif
#else
    #define LLDATAIN_PUBLIC
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace data {

/*/////////////////
//   Constants   //
/////////////////*/

/*/////////////////
//   Data Types  //
/////////////////*/
/// @summary Defines the different text encodings that can be detected by
/// inspecting the first four bytes of a text document for a byte order marker.
enum text_encoding_e
{
    /// Indicates that the blob::determine_text_encoding() function is unsure
    /// of the encoding. This typically indicates that no BOM is present.
    TEXT_ENCODING_UNSURE      = 0,
    /// Indicates that the text is encoded using single-byte ASCII.
    TEXT_ENCODING_ASCII       = 1,
    /// Indicates that the text is encoded using UTF-8.
    TEXT_ENCODING_UTF8        = 2,
    /// Indicates that the text is encoded using big-endian UTF-16.
    TEXT_ENCODING_UTF16_BE    = 3,
    /// Indicates that the text is encoded using little-endian UTF-16.
    TEXT_ENCODING_UTF16_LE    = 4,
    /// Indicates that the text is encoded using big-endian UTF-32.
    TEXT_ENCODING_UTF32_BE    = 5,
    /// Indicates that the text is encoded using little-endian UTF-32.
    TEXT_ENCODING_UTF32_LE    = 6,
    /// This type value is unused and serves only to force a minimum of 32-bits
    /// of storage space for values of this enumeration type.
    TEXT_ENCODING_FORCE_32BIT = 0x7FFFFFFFL
};

/*////////////////
//   Functions  //
////////////////*/
/// @summary Gets the bytes (up to four) representing the Unicode BOM associated
/// with a specific text encoding.
/// @param encoding One of the text encoding constants, specifying the
/// encoding for which the BOM will be returned.
/// @param out_BOM An array of four bytes that will hold the BOM
/// corresponding to the specified text encoding. Between zero and four
/// bytes will be written to this array.
/// @return The number of bytes written to @a out_BOM.
LLDATAIN_PUBLIC size_t bom(int32_t encoding, uint8_t out_BOM[4]);

/// @summary Given four bytes possibly representing a Unicode byte-order-marker,
/// attempts to determine the text encoding and actual size of the BOM.
/// @param BOM The array of four bytes potentially containing a BOM.
/// @param out_size If this value is non-NULL, on return it is updated
/// with the actual size of the BOM, in bytes.
/// @return One of the text_encoding_e constants, specifying the text encoding
/// indicated by the BOM.
LLDATAIN_PUBLIC int32_t encoding(uint8_t BOM[4], size_t *out_size);

/// @summary Computes the maximum number of bytes required to base64-encode a
/// binary data block. All data is assumed to be output on one line. One byte
/// is included for a trailing NULL.
/// @param binary_size The size of the binary data block, in bytes.
/// @param out_pad_size If non-NULL, on return this value is updated with
/// the number of padding bytes that will be added during encoding.
/// @return The maximum number of bytes required to base64-encode a data
/// block of the specified size.
LLDATAIN_PUBLIC size_t base64_size(size_t binary_size, size_t *out_pad_size);

/// @summary Computes the number of raw bytes required to store a block of
/// binary data when converted back from base64. All source data is assumed to be on one line.
/// @param base64_size The size of the base64-encoded data.
/// @param pad_size The number of bytes of padding data. If not known, specify a value of zero.
/// @return The number of bytes of binary data generated during decoding.
LLDATAIN_PUBLIC size_t binary_size(size_t base64_size, size_t pad_size);

/// @summary Computes the number of raw bytes required to store a block of
/// binary data when converted back from base64. All source data is assumed to
/// be on one line. This version of the function examines the source data
/// directly, and so can provide a precise value.
/// @param base64_source Pointer to the start of the base64-encoded data.
/// @param base64_length The number of ASCII characters in the base64 data
/// string. This value can be computed using the standard C library strlen
/// function if the length is not otherwise available.
/// @return The number of bytes of binary data that will be generated during decoding.
LLDATAIN_PUBLIC size_t binary_size(char const *base64_source, size_t base64_length);

/// @summary Base64-encodes a block of arbitrary data. All data is returned on
/// a single line; no newlines are inserted and no formatting is performed. The
/// output buffer is guaranteed to be NULL-terminated.
/// @param dst Pointer to the start of the output buffer.
/// @param dst_size The maximum number of bytes that can be written to
/// the output buffer. This value must be at least as large as the value
/// returned by the data::base64_size() function.
/// @param src Pointer to the start of the input data.
/// @param src_size The number of bytes of input data to encode.
/// @return The number of bytes written to the output buffer.
LLDATAIN_PUBLIC size_t base64_encode(char *dst, size_t dst_size, void const *src, size_t src_size);

/// @summary Decodes a base64-encoded block of text into the corresponding raw binary representation.
/// @param dst Pointer to the start of the output buffer.
/// @param dst_size The maximum number of bytes that can be written to
/// the output buffer. This value can be up to two bytes less than the
/// value returned by data::binary_size() depending on the amount of
/// padding added during encoding.
/// @param src Pointer to the start of the base64-encoded input data.
/// @param src_size The number of bytes of input data to read and decode.
/// @return The number of bytes written to the output buffer.
LLDATAIN_PUBLIC size_t base64_decode(void *dst, size_t dst_size, char const *src, size_t src_size);

/// @summary Loads the entire contents of a text file into a buffer using the
/// stdio buffered file I/O routines. The buffer is allocated with malloc() and
/// should be freed using the free() function. The buffer is guaranteed to be
/// terminated with a zero byte, and the BOM (if present) is stripped from the data.
/// The file is opened in binary mode, and no translation is performed.
/// @param path The NULL-terminated path of the file to load.
/// @param out_buffer_size On return, this location is updated with the size of
/// the the returned buffer, in bytes, and NOT including the extra zero-byte or BOM.
/// @param out_encoding On return, this location is updated with the encoding
/// of the text data, if it could be determined, or TEXT_ENCODING_UNSURE otherwise.
/// @return A buffer containing the loaded, zero-terminated data, or NULL.
LLDATAIN_PUBLIC char* load_text(char const *path, size_t *out_buffer_size, data::text_encoding_e *out_encoding);

/// @summary Loads the entire contents of a file into a buffer using the stdio
/// buffered file I/O routines. The buffer is allocated with malloc() and should
/// be freed using the free() function.
/// @param path The NULL-terminated path of the file to load.
/// @param out_buffer_size On return, this location is updated with the size of
/// the the returned buffer, in bytes.
/// @return A buffer containing the loaded data, or NULL.
LLDATAIN_PUBLIC void* load_binary(char const *path, size_t *out_buffer_size);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace data */

#endif /* !defined(LLDATAIN_HPP_INCLUDED) */
