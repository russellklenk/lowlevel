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
/// @summary The FourCC 'DDS ' using little-endian byte ordering.
#ifndef LLDATAIN_DDS_MAGIC_LE
#define LLDATAIN_DDS_MAGIC_LE    0x20534444U
#endif

/*/////////////////
//   Data Types  //
/////////////////*/
/// @summary Defines the different text encodings that can be detected by
/// inspecting the first four bytes of a text document for a byte order marker.
enum text_encoding_e
{
    /// Indicates that the blob::determine_text_encoding() function is unsure
    /// of the encoding. This typically indicates that no BOM is present.
    TEXT_ENCODING_UNSURE        = 0,
    /// Indicates that the text is encoded using single-byte ASCII.
    TEXT_ENCODING_ASCII         = 1,
    /// Indicates that the text is encoded using UTF-8.
    TEXT_ENCODING_UTF8          = 2,
    /// Indicates that the text is encoded using big-endian UTF-16.
    TEXT_ENCODING_UTF16_BE      = 3,
    /// Indicates that the text is encoded using little-endian UTF-16.
    TEXT_ENCODING_UTF16_LE      = 4,
    /// Indicates that the text is encoded using big-endian UTF-32.
    TEXT_ENCODING_UTF32_BE      = 5,
    /// Indicates that the text is encoded using little-endian UTF-32.
    TEXT_ENCODING_UTF32_LE      = 6,
    /// This type value is unused and serves only to force a minimum of 32-bits
    /// of storage space for values of this enumeration type.
    TEXT_ENCODING_FORCE_32BIT   = 0x7FFFFFFFL
};

/// @summary Bitflags for dds_pixelformat_t::Flags. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943984(v=vs.85).aspx
/// for the DDS_PIXELFORMAT structure.
enum dds_pixelformat_flags_e
{
    DDPF_NONE                   = 0x00000000U,
    DDPF_ALPHAPIXELS            = 0x00000001U,
    DDPF_ALPHA                  = 0x00000002U,
    DDPF_FOURCC                 = 0x00000004U,
    DDPF_RGB                    = 0x00000040U,
    DDPF_YUV                    = 0x00000200U,
    DDPF_LUMINANCE              = 0x00020000U
};

/// @summary Bitflags for dds_header_t::Flags. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx
/// for the DDS_HEADER structure.
enum dds_header_flags_e
{
    DDSD_NONE                   = 0x00000000U,
    DDSD_CAPS                   = 0x00000001U,
    DDSD_HEIGHT                 = 0x00000002U,
    DDSD_WIDTH                  = 0x00000004U,
    DDSD_PITCH                  = 0x00000008U,
    DDSD_PIXELFORMAT            = 0x00001000U,
    DDSD_MIPMAPCOUNT            = 0x00020000U,
    DDSD_LINEARSIZE             = 0x00080000U,
    DDSD_DEPTH                  = 0x00800000U,
    DDS_HEADER_FLAGS_TEXTURE    = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT,
    DDS_HEADER_FLAGS_MIPMAP     = DDSD_MIPMAPCOUNT,
    DDS_HEADER_FLAGS_VOLUME     = DDSD_DEPTH,
    DDS_HEADER_FLAGS_PITCH      = DDSD_PITCH,
    DDS_HEADER_FLAGS_LINEARSIZE = DDSD_LINEARSIZE
};

/// @summary Bitflags for dds_header_t::Caps. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx
/// for the DDS_HEADER structure.
enum dds_caps_e
{
    DDSCAPS_NONE                = 0x00000000U,
    DDSCAPS_COMPLEX             = 0x00000008U,
    DDSCAPS_TEXTURE             = 0x00001000U,
    DDSCAPS_MIPMAP              = 0x00400000U,
    DDS_SURFACE_FLAGS_MIPMAP    = DDSCAPS_COMPLEX | DDSCAPS_MIPMAP,
    DDS_SURFACE_FLAGS_TEXTURE   = DDSCAPS_TEXTURE,
    DDS_SURFACE_FLAGS_CUBEMAP   = DDSCAPS_COMPLEX
};

/// @summary Bitflags for dds_header_t::Caps2. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx
/// for the DDS_HEADER structure.
enum dds_caps2_e
{
    DDSCAPS2_NONE               = 0x00000000U,
    DDSCAPS2_CUBEMAP            = 0x00000200U,
    DDSCAPS2_CUBEMAP_POSITIVEX  = 0x00000400U,
    DDSCAPS2_CUBEMAP_NEGATIVEX  = 0x00000800U,
    DDSCAPS2_CUBEMAP_POSITIVEY  = 0x00001000U,
    DDSCAPS2_CUBEMAP_NEGATIVEY  = 0x00002000U,
    DDSCAPS2_CUBEMAP_POSITIVEZ  = 0x00004000U,
    DDSCAPS2_CUBEMAP_NEGATIVEZ  = 0x00008000U,
    DDSCAPS2_VOLUME             = 0x00200000U,
    DDS_FLAG_VOLUME             = DDSCAPS2_VOLUME,
    DDS_CUBEMAP_POSITIVEX       = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX,
    DDS_CUBEMAP_NEGATIVEX       = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX,
    DDS_CUBEMAP_POSITIVEY       = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY,
    DDS_CUBEMAP_NEGATIVEY       = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY,
    DDS_CUBEMAP_POSITIVEZ       = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ,
    DDS_CUBEMAP_NEGATIVEZ       = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ,
    DDS_CUBEMAP_ALLFACES        = DDSCAPS2_CUBEMAP |
                                  DDSCAPS2_CUBEMAP_POSITIVEX |
                                  DDSCAPS2_CUBEMAP_NEGATIVEX |
                                  DDSCAPS2_CUBEMAP_POSITIVEY |
                                  DDSCAPS2_CUBEMAP_NEGATIVEY |
                                  DDSCAPS2_CUBEMAP_POSITIVEZ |
                                  DDSCAPS2_CUBEMAP_NEGATIVEZ
};

/// @summary Bitflags for dds_header_t::Caps3. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx
/// for the DDS_HEADER structure.
enum dds_caps3_e
{
    DDSCAPS3_NONE = 0x00000000U
};

/// @summary Bitflags for dds_header_t::Caps4. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx
/// for the DDS_HEADER structure.
enum dds_caps4_e
{
    DDSCAPS4_NONE = 0x00000000U
};

/// @summary Values for dds_header_dxt10_t::Format. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb173059(v=vs.85).aspx
enum dxgi_format_e
{
    DXGI_FORMAT_UNKNOWN                     = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
    DXGI_FORMAT_R32G32B32A32_UINT           = 3,
    DXGI_FORMAT_R32G32B32A32_SINT           = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
    DXGI_FORMAT_R32G32B32_FLOAT             = 6,
    DXGI_FORMAT_R32G32B32_UINT              = 7,
    DXGI_FORMAT_R32G32B32_SINT              = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
    DXGI_FORMAT_R16G16B16A16_UINT           = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
    DXGI_FORMAT_R16G16B16A16_SINT           = 14,
    DXGI_FORMAT_R32G32_TYPELESS             = 15,
    DXGI_FORMAT_R32G32_FLOAT                = 16,
    DXGI_FORMAT_R32G32_UINT                 = 17,
    DXGI_FORMAT_R32G32_SINT                 = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
    DXGI_FORMAT_R10G10B10A2_UINT            = 25,
    DXGI_FORMAT_R11G11B10_FLOAT             = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
    DXGI_FORMAT_R8G8B8A8_UINT               = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
    DXGI_FORMAT_R8G8B8A8_SINT               = 32,
    DXGI_FORMAT_R16G16_TYPELESS             = 33,
    DXGI_FORMAT_R16G16_FLOAT                = 34,
    DXGI_FORMAT_R16G16_UNORM                = 35,
    DXGI_FORMAT_R16G16_UINT                 = 36,
    DXGI_FORMAT_R16G16_SNORM                = 37,
    DXGI_FORMAT_R16G16_SINT                 = 38,
    DXGI_FORMAT_R32_TYPELESS                = 39,
    DXGI_FORMAT_D32_FLOAT                   = 40,
    DXGI_FORMAT_R32_FLOAT                   = 41,
    DXGI_FORMAT_R32_UINT                    = 42,
    DXGI_FORMAT_R32_SINT                    = 43,
    DXGI_FORMAT_R24G8_TYPELESS              = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
    DXGI_FORMAT_R8G8_TYPELESS               = 48,
    DXGI_FORMAT_R8G8_UNORM                  = 49,
    DXGI_FORMAT_R8G8_UINT                   = 50,
    DXGI_FORMAT_R8G8_SNORM                  = 51,
    DXGI_FORMAT_R8G8_SINT                   = 52,
    DXGI_FORMAT_R16_TYPELESS                = 53,
    DXGI_FORMAT_R16_FLOAT                   = 54,
    DXGI_FORMAT_D16_UNORM                   = 55,
    DXGI_FORMAT_R16_UNORM                   = 56,
    DXGI_FORMAT_R16_UINT                    = 57,
    DXGI_FORMAT_R16_SNORM                   = 58,
    DXGI_FORMAT_R16_SINT                    = 59,
    DXGI_FORMAT_R8_TYPELESS                 = 60,
    DXGI_FORMAT_R8_UNORM                    = 61,
    DXGI_FORMAT_R8_UINT                     = 62,
    DXGI_FORMAT_R8_SNORM                    = 63,
    DXGI_FORMAT_R8_SINT                     = 64,
    DXGI_FORMAT_A8_UNORM                    = 65,
    DXGI_FORMAT_R1_UNORM                    = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
    DXGI_FORMAT_BC1_TYPELESS                = 70,
    DXGI_FORMAT_BC1_UNORM                   = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
    DXGI_FORMAT_BC2_TYPELESS                = 73,
    DXGI_FORMAT_BC2_UNORM                   = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
    DXGI_FORMAT_BC3_TYPELESS                = 76,
    DXGI_FORMAT_BC3_UNORM                   = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
    DXGI_FORMAT_BC4_TYPELESS                = 79,
    DXGI_FORMAT_BC4_UNORM                   = 80,
    DXGI_FORMAT_BC4_SNORM                   = 81,
    DXGI_FORMAT_BC5_TYPELESS                = 82,
    DXGI_FORMAT_BC5_UNORM                   = 83,
    DXGI_FORMAT_BC5_SNORM                   = 84,
    DXGI_FORMAT_B5G6R5_UNORM                = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
    DXGI_FORMAT_BC6H_TYPELESS               = 94,
    DXGI_FORMAT_BC6H_UF16                   = 95,
    DXGI_FORMAT_BC6H_SF16                   = 96,
    DXGI_FORMAT_BC7_TYPELESS                = 97,
    DXGI_FORMAT_BC7_UNORM                   = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
    DXGI_FORMAT_AYUV                        = 100,
    DXGI_FORMAT_Y410                        = 101,
    DXGI_FORMAT_Y416                        = 102,
    DXGI_FORMAT_NV12                        = 103,
    DXGI_FORMAT_P010                        = 104,
    DXGI_FORMAT_P016                        = 105,
    DXGI_FORMAT_420_OPAQUE                  = 106,
    DXGI_FORMAT_YUY2                        = 107,
    DXGI_FORMAT_Y210                        = 108,
    DXGI_FORMAT_Y216                        = 109,
    DXGI_FORMAT_NV11                        = 110,
    DXGI_FORMAT_AI44                        = 111,
    DXGI_FORMAT_IA44                        = 112,
    DXGI_FORMAT_P8                          = 113,
    DXGI_FORMAT_A8P8                        = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM              = 115,
    DXGI_FORMAT_FORCE_UINT                  = 0xFFFFFFFFU
};

/// @summary Values for dds_header_dxt10_t::Dimension. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943983(v=vs.85).aspx
/// for the DDS_HEADER_DXT10 structure.
enum d3d11_resource_dimension_e
{
    D3D10_RESOURCE_DIMENSION_UNKNOWN        = 0,
    D3D10_RESOURCE_DIMENSION_BUFFER         = 1,
    D3D10_RESOURCE_DIMENSION_TEXTURE1D      = 2,
    D3D10_RESOURCE_DIMENSION_TEXTURE2D      = 3,
    D3D10_RESOURCE_DIMENSION_TEXTURE3D      = 4
};

/// @summary Values for dds_header_dxt10_t::Flags. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943983(v=vs.85).aspx
/// for the DDS_HEADER_DXT10 structure.
enum d3d11_resource_misc_flag_e
{
    D3D11_RESOURCE_MISC_TEXTURECUBE         = 0x00000004U,
};

/// @summary Values for dds_header_dxt10_t::Flags2. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943983(v=vs.85).aspx
/// for the DDS_HEADER_DXT10 structure.
enum dds_alpha_mode_e
{
    DDS_ALPHA_MODE_UNKNOWN                  = 0x00000000U,
    DDS_ALPHA_MODE_STRAIGHT                 = 0x00000001U,
    DDS_ALPHA_MODE_PREMULTIPLIED            = 0x00000002U,
    DDS_ALPHA_MODE_OPAQUE                   = 0x00000003U,
    DDS_ALPHA_MODE_CUSTOM                   = 0x00000004U
};

/// @summary The equivalent of the DDS_PIXELFORMAT structure. See MSDN at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943984(v=vs.85).aspx
#pragma pack(push, 1)
struct dds_pixelformat_t
{
    uint32_t Size;            /// The size of the structure (32 bytes).
    uint32_t Flags;           /// Combination of dds_pixelformat_flags_e.
    uint32_t FourCC;          /// DXT1, DXT2, DXT3, DXT4, DXT5 or DX10. See MSDN.
    uint32_t RGBBitCount;     /// The number of bits per-pixel.
    uint32_t BitMaskR;        /// Mask for reading red/luminance/Y data.
    uint32_t BitMaskG;        /// Mask for reading green/U data.
    uint32_t BitMaskB;        /// Mask for reading blue/V data.
    uint32_t BitMaskA;        /// Mask for reading alpha channel data.
};
#pragma pack(pop)

/// @summary The equivalent of the DDS_HEADER structure. See MSDN at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx
#pragma pack(push, 1)
struct dds_header_t
{
    uint32_t Size;            /// The size of the structure (124 bytes).
    uint32_t Flags;           /// Combination of dds_header_flags_e.
    uint32_t Height;          /// The surface height, in pixels.
    uint32_t Width;           /// The surface width, in pixels.
    uint32_t Pitch;           /// Bytes per-scanline, or bytes in top-level (compressed).
    uint32_t Depth;           /// The surface depth, in slices. For non-volume surfaces, 0.
    uint32_t Levels;          /// The number of mipmap levels, or 0 if there are no mipmaps.
    uint32_t Reserved1[11];   /// Reserved for future use.
    dds_pixelformat_t Format; /// Pixel format descriptor.
    uint32_t Caps;            /// Combination of dds_caps_e.
    uint32_t Caps2;           /// Combination of dds_caps2_e.
    uint32_t Caps3;           /// Combination of dds_caps3_e.
    uint32_t Caps4;           /// Combination of dds_caps4_e.
    uint32_t Reserved2;       /// Reserved for future use.
};
#pragma pack(pop)

/// @summary The equivalent of the DDS_HEADER_DXT10 structure. See MSDN at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943983(v=vs.85).aspx
#pragma pack(push, 1)
struct dds_header_dxt10_t
{
    uint32_t Format;          /// One of dxgi_format_e.
    uint32_t Dimension;       /// One of d3d11_resource_dimension_e.
    uint32_t Flags;           /// Combination of d3d11_resource_misc_flag_e.
    uint32_t ArraySize;       /// The number of of items in an array texture.
    uint32_t Flags2;          /// One of dds_alpha_mode_e.
};
#pragma pack(pop)

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

/// @summary Reads the surface header present in all DDS files.
/// @param data The buffer from which the header data should be read.
/// @param data_size The maximum number of bytes to read from the input buffer.
/// @param out_header Pointer to the structure to populate.
/// @return true if the file appears to be a valid DDS file.
LLDATAIN_PUBLIC bool dds_header(void const *data, size_t data_size, dds_header_t *out_header);

/// @summary Reads the extended surface header from a DDS buffer, if present.
/// @param data The buffer from which the header data should be read.
/// @param data_size The maximum number of bytes to read from the input buffer.
/// @param out_header Pointer to the structure to populate.
/// @return true if the file contains an extended surface header.
LLDATAIN_PUBLIC bool dds_header_dxt10(void const *data, size_t data_size, dds_header_dxt10_t *out_header);

/// @summary Determines the DXGI_FORMAT value based on data in DDS headers.
/// @param header The base surface header of the DDS.
/// @param header_ex The extended surface header of the DDS, or NULL.
/// @retur  One of the values of the dxgi_format_e enumeration.
LLDATAIN_PUBLIC uint32_t dds_format(dds_header_t const *header, dds_header_dxt10_t const *header_ex);

/// @summary Calculates the correct pitch value for a scanline, based on the
/// data format and width of the surface. This is necessary because many DDS
/// writers do not correctly compute the pitch value. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
/// @param format One of the values of the dxgi_format_e enumeration.
LLDATAIN_PUBLIC size_t dds_pitch(uint32_t format, uint32_t width);

/// @summary Determines if a DXGI format value is a block-compressed format.
/// @param format One of dxgi_format_e.
/// @return true if format is one of DXGI_FORMAT_BCn.
LLDATAIN_PUBLIC bool dds_block_compressed(uint32_t format);

/// @summary Determines if a DXGI fomrat value specifies a packed format.
/// @param format One of dxgi_format_e.
/// @return true if format is one of DXGI_FORMAT_R8G8_B8G8_UNORM or DXGI_FORMAT_G8R8_G8B8_UNORM.
LLDATAIN_PUBLIC bool dds_packed(uint32_t format);

/// @summary Calculate the number of bits-per-pixel for a given format. Block-
/// compressed formats are supported as well.
/// @param format One of dxgi_format_e.
/// @return The number of bits per-pixel.
LLDATAIN_PUBLIC size_t dds_bits_per_pixel(uint32_t format);

/// @summary Calculate the number of bytes per 4x4-pixel block.
/// @param format One of dxgi_format_e.
/// @return The number of bytes in a 4x4 pixel block, or 0 for non-block-compressed formats.
LLDATAIN_PUBLIC size_t dds_bytes_per_block(uint32_t format);

/// @summary Generates a little-endian FOURCC.
/// @param a...d The four characters comprising the code.
/// @return The packed four-cc value, in little-endian format.
static inline uint32_t fourcc_le(char a, char b, char c, char d)
{
    uint32_t A = (uint32_t) a;
    uint32_t B = (uint32_t) b;
    uint32_t C = (uint32_t) c;
    uint32_t D = (uint32_t) d;
    return ((D << 24) | (C << 16) | (B << 8) | (A << 0));
}

/// @summary Generates a big-endian FOURCC.
/// @param a...d The four characters comprising the code.
/// @return The packed four-cc value, in big-endian format.
static inline uint32_t fourcc_be(char a, char b, char c, char d)
{
    uint32_t A = (uint32_t) a;
    uint32_t B = (uint32_t) b;
    uint32_t C = (uint32_t) c;
    uint32_t D = (uint32_t) d;
    return ((A << 24) | (B << 16) | (C << 8) | (D << 0));
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace data */

#endif /* !defined(LLDATAIN_HPP_INCLUDED) */