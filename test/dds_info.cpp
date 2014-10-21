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
static const size_t DDPF_COUNT = 7;
static const data::dds_pixelformat_flags_e DDPF_FLAGS[DDPF_COUNT] =
{
    data::DDPF_NONE,
    data::DDPF_ALPHAPIXELS,
    data::DDPF_ALPHA,
    data::DDPF_FOURCC,
    data::DDPF_RGB,
    data::DDPF_YUV,
    data::DDPF_LUMINANCE
};

static const size_t DDSD_COUNT = 9;
static const data::dds_header_flags_e DDSD_FLAGS[DDSD_COUNT] =
{
    data::DDSD_NONE,
    data::DDSD_CAPS,
    data::DDSD_HEIGHT,
    data::DDSD_WIDTH,
    data::DDSD_PITCH,
    data::DDSD_PIXELFORMAT,
    data::DDSD_MIPMAPCOUNT,
    data::DDSD_LINEARSIZE,
    data::DDSD_DEPTH
};

static const size_t DDSCAPS_COUNT = 4;
static const data::dds_caps_e DDSCAPS_FLAGS[DDSCAPS_COUNT] =
{
    data::DDSCAPS_NONE,
    data::DDSCAPS_COMPLEX,
    data::DDSCAPS_TEXTURE,
    data::DDSCAPS_MIPMAP
};

static const size_t DDSCAPS2_COUNT = 9;
static const data::dds_caps2_e DDSCAPS2_FLAGS[DDSCAPS2_COUNT] =
{
    data::DDSCAPS2_NONE,
    data::DDSCAPS2_CUBEMAP,
    data::DDSCAPS2_CUBEMAP_POSITIVEX,
    data::DDSCAPS2_CUBEMAP_NEGATIVEX,
    data::DDSCAPS2_CUBEMAP_POSITIVEY,
    data::DDSCAPS2_CUBEMAP_NEGATIVEY,
    data::DDSCAPS2_CUBEMAP_POSITIVEZ,
    data::DDSCAPS2_CUBEMAP_NEGATIVEZ,
    data::DDSCAPS2_VOLUME
};

static const size_t DDSCAPS3_COUNT = 1;
static const data::dds_caps3_e DDSCAPS3_FLAGS[DDSCAPS3_COUNT] =
{
    data::DDSCAPS3_NONE
};

static const size_t DDSCAPS4_COUNT = 1;
static const data::dds_caps4_e DDSCAPS4_FLAGS[DDSCAPS4_COUNT] =
{
    data::DDSCAPS4_NONE
};

static const size_t DXGI_FORMAT_COUNT = 116;
static const data::dxgi_format_e DXGI_FORMATS[DXGI_FORMAT_COUNT] =
{
    data::DXGI_FORMAT_UNKNOWN,
    data::DXGI_FORMAT_R32G32B32A32_TYPELESS,
    data::DXGI_FORMAT_R32G32B32A32_FLOAT,
    data::DXGI_FORMAT_R32G32B32A32_UINT,
    data::DXGI_FORMAT_R32G32B32A32_SINT,
    data::DXGI_FORMAT_R32G32B32_TYPELESS,
    data::DXGI_FORMAT_R32G32B32_FLOAT,
    data::DXGI_FORMAT_R32G32B32_UINT,
    data::DXGI_FORMAT_R32G32B32_SINT,
    data::DXGI_FORMAT_R16G16B16A16_TYPELESS,
    data::DXGI_FORMAT_R16G16B16A16_FLOAT,
    data::DXGI_FORMAT_R16G16B16A16_UNORM,
    data::DXGI_FORMAT_R16G16B16A16_UINT,
    data::DXGI_FORMAT_R16G16B16A16_SNORM,
    data::DXGI_FORMAT_R16G16B16A16_SINT,
    data::DXGI_FORMAT_R32G32_TYPELESS,
    data::DXGI_FORMAT_R32G32_FLOAT,
    data::DXGI_FORMAT_R32G32_UINT,
    data::DXGI_FORMAT_R32G32_SINT,
    data::DXGI_FORMAT_R32G8X24_TYPELESS,
    data::DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    data::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
    data::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
    data::DXGI_FORMAT_R10G10B10A2_TYPELESS,
    data::DXGI_FORMAT_R10G10B10A2_UNORM,
    data::DXGI_FORMAT_R10G10B10A2_UINT,
    data::DXGI_FORMAT_R11G11B10_FLOAT,
    data::DXGI_FORMAT_R8G8B8A8_TYPELESS,
    data::DXGI_FORMAT_R8G8B8A8_UNORM,
    data::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    data::DXGI_FORMAT_R8G8B8A8_UINT,
    data::DXGI_FORMAT_R8G8B8A8_SNORM,
    data::DXGI_FORMAT_R8G8B8A8_SINT,
    data::DXGI_FORMAT_R16G16_TYPELESS,
    data::DXGI_FORMAT_R16G16_FLOAT,
    data::DXGI_FORMAT_R16G16_UNORM,
    data::DXGI_FORMAT_R16G16_UINT,
    data::DXGI_FORMAT_R16G16_SNORM,
    data::DXGI_FORMAT_R16G16_SINT,
    data::DXGI_FORMAT_R32_TYPELESS,
    data::DXGI_FORMAT_D32_FLOAT,
    data::DXGI_FORMAT_R32_FLOAT,
    data::DXGI_FORMAT_R32_UINT,
    data::DXGI_FORMAT_R32_SINT,
    data::DXGI_FORMAT_R24G8_TYPELESS,
    data::DXGI_FORMAT_D24_UNORM_S8_UINT,
    data::DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
    data::DXGI_FORMAT_X24_TYPELESS_G8_UINT,
    data::DXGI_FORMAT_R8G8_TYPELESS,
    data::DXGI_FORMAT_R8G8_UNORM,
    data::DXGI_FORMAT_R8G8_UINT,
    data::DXGI_FORMAT_R8G8_SNORM,
    data::DXGI_FORMAT_R8G8_SINT,
    data::DXGI_FORMAT_R16_TYPELESS,
    data::DXGI_FORMAT_R16_FLOAT,
    data::DXGI_FORMAT_D16_UNORM,
    data::DXGI_FORMAT_R16_UNORM,
    data::DXGI_FORMAT_R16_UINT,
    data::DXGI_FORMAT_R16_SNORM,
    data::DXGI_FORMAT_R16_SINT,
    data::DXGI_FORMAT_R8_TYPELESS,
    data::DXGI_FORMAT_R8_UNORM,
    data::DXGI_FORMAT_R8_UINT,
    data::DXGI_FORMAT_R8_SNORM,
    data::DXGI_FORMAT_R8_SINT,
    data::DXGI_FORMAT_A8_UNORM,
    data::DXGI_FORMAT_R1_UNORM,
    data::DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
    data::DXGI_FORMAT_R8G8_B8G8_UNORM,
    data::DXGI_FORMAT_G8R8_G8B8_UNORM,
    data::DXGI_FORMAT_BC1_TYPELESS,
    data::DXGI_FORMAT_BC1_UNORM,
    data::DXGI_FORMAT_BC1_UNORM_SRGB,
    data::DXGI_FORMAT_BC2_TYPELESS,
    data::DXGI_FORMAT_BC2_UNORM,
    data::DXGI_FORMAT_BC2_UNORM_SRGB,
    data::DXGI_FORMAT_BC3_TYPELESS,
    data::DXGI_FORMAT_BC3_UNORM,
    data::DXGI_FORMAT_BC3_UNORM_SRGB,
    data::DXGI_FORMAT_BC4_TYPELESS,
    data::DXGI_FORMAT_BC4_UNORM,
    data::DXGI_FORMAT_BC4_SNORM,
    data::DXGI_FORMAT_BC5_TYPELESS,
    data::DXGI_FORMAT_BC5_UNORM,
    data::DXGI_FORMAT_BC5_SNORM,
    data::DXGI_FORMAT_B5G6R5_UNORM,
    data::DXGI_FORMAT_B5G5R5A1_UNORM,
    data::DXGI_FORMAT_B8G8R8A8_UNORM,
    data::DXGI_FORMAT_B8G8R8X8_UNORM,
    data::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    data::DXGI_FORMAT_B8G8R8A8_TYPELESS,
    data::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    data::DXGI_FORMAT_B8G8R8X8_TYPELESS,
    data::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
    data::DXGI_FORMAT_BC6H_TYPELESS,
    data::DXGI_FORMAT_BC6H_UF16,
    data::DXGI_FORMAT_BC6H_SF16,
    data::DXGI_FORMAT_BC7_TYPELESS,
    data::DXGI_FORMAT_BC7_UNORM,
    data::DXGI_FORMAT_BC7_UNORM_SRGB,
    data::DXGI_FORMAT_AYUV,
    data::DXGI_FORMAT_Y410,
    data::DXGI_FORMAT_Y416,
    data::DXGI_FORMAT_NV12,
    data::DXGI_FORMAT_P010,
    data::DXGI_FORMAT_P016,
    data::DXGI_FORMAT_420_OPAQUE,
    data::DXGI_FORMAT_YUY2,
    data::DXGI_FORMAT_Y210,
    data::DXGI_FORMAT_Y216,
    data::DXGI_FORMAT_NV11,
    data::DXGI_FORMAT_AI44,
    data::DXGI_FORMAT_IA44,
    data::DXGI_FORMAT_P8,
    data::DXGI_FORMAT_A8P8,
    data::DXGI_FORMAT_B4G4R4A4_UNORM
};

static const size_t RESOURCE_DIMENSION_COUNT = 5;
static const data::d3d11_resource_dimension_e RESOURCE_DIMENSION[RESOURCE_DIMENSION_COUNT] =
{
    data::D3D11_RESOURCE_DIMENSION_UNKNOWN,
    data::D3D11_RESOURCE_DIMENSION_BUFFER,
    data::D3D11_RESOURCE_DIMENSION_TEXTURE1D,
    data::D3D11_RESOURCE_DIMENSION_TEXTURE2D,
    data::D3D11_RESOURCE_DIMENSION_TEXTURE3D
};

static const size_t RESOURCE_MISC_FLAG_COUNT = 1;
static const data::d3d11_resource_misc_flag_e RESOURCE_MISC_FLAG[RESOURCE_MISC_FLAG_COUNT] =
{
    data::D3D11_RESOURCE_MISC_TEXTURECUBE
};

static const size_t ALPHA_MODE_COUNT = 5;
static const data::dds_alpha_mode_e ALPHA_MODE[ALPHA_MODE_COUNT] =
{
    data::DDS_ALPHA_MODE_UNKNOWN,
    data::DDS_ALPHA_MODE_STRAIGHT,
    data::DDS_ALPHA_MODE_PREMULTIPLIED,
    data::DDS_ALPHA_MODE_OPAQUE,
    data::DDS_ALPHA_MODE_CUSTOM
};

/*///////////////////////
//   Local Functions   //
///////////////////////*/
static char const* fourcc_le_str(uint32_t fourcc)
{
    static char Buffer[5];
    char *chars = (char*) &fourcc;
    Buffer[0]   = chars[0];
    Buffer[1]   = chars[1];
    Buffer[2]   = chars[2];
    Buffer[3]   = chars[3];
    Buffer[4]   = '\0';
    return Buffer; // obviously not re-entrant or thread safe.
}

static char const* ddpf_str(uint32_t flags)
{
    switch (flags)
    {
        case data::DDPF_NONE:
            return "DDPF_NONE";
        case data::DDPF_ALPHAPIXELS:
            return "DDPF_ALPHAPIXELS";
        case data::DDPF_ALPHA:
            return "DDPF_ALPHA";
        case data::DDPF_FOURCC:
            return "DDPF_FOURCC";
        case data::DDPF_RGB:
            return "DDPF_RGB";
        case data::DDPF_YUV:
            return "DDPF_YUV";
        case data::DDPF_LUMINANCE:
            return "DDPF_LUMINANCE";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* ddsd_str(uint32_t flags)
{
    switch (flags)
    {
        case data::DDSD_NONE:
            return "DDSD_NONE";
        case data::DDSD_CAPS:
            return "DDSD_CAPS";
        case data::DDSD_HEIGHT:
            return "DDSD_HEIGHT";
        case data::DDSD_WIDTH:
            return "DDSD_WIDTH";
        case data::DDSD_PITCH:
            return "DDSD_PITCH";
        case data::DDSD_PIXELFORMAT:
            return "DDSD_PIXELFORMAT";
        case data::DDSD_MIPMAPCOUNT:
            return "DDSD_MIPMAPCOUNT";
        case data::DDSD_LINEARSIZE:
            return "DDSD_LINEARSIZE";
        case data::DDSD_DEPTH:
            return "DDSD_DEPTH";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* dds_caps_str(uint32_t flags)
{
    switch (flags)
    {
        case data::DDSCAPS_NONE:
            return "DDSCAPS_NONE";
        case data::DDSCAPS_COMPLEX:
            return "DDSCAPS_COMPLEX";
        case data::DDSCAPS_TEXTURE:
            return "DDSCAPS_TEXTURE";
        case data::DDSCAPS_MIPMAP:
            return "DDSCAPS_MIPMAP";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* dds_caps2_str(uint32_t flags)
{
    switch (flags)
    {
        case data::DDSCAPS2_NONE:
            return "DDSCAPS2_NONE";
        case data::DDSCAPS2_CUBEMAP:
            return "DDSCAPS2_CUBEMAP";
        case data::DDSCAPS2_CUBEMAP_POSITIVEX:
            return "DDSCAPS2_CUBEMAP_POSITIVEX";
        case data::DDSCAPS2_CUBEMAP_NEGATIVEX:
            return "DDSCAPS2_CUBEMAP_NEGATIVEX";
        case data::DDSCAPS2_CUBEMAP_POSITIVEY:
            return "DDSCAPS2_CUBEMAP_POSITIVEY";
        case data::DDSCAPS2_CUBEMAP_NEGATIVEY:
            return "DDSCAPS2_CUBEMAP_NEGATIVEY";
        case data::DDSCAPS2_CUBEMAP_POSITIVEZ:
            return "DDSCAPS2_CUBEMAP_POSITIVEZ";
        case data::DDSCAPS2_CUBEMAP_NEGATIVEZ:
            return "DDSCAPS2_CUBEMAP_NEGATIVEZ";
        case data::DDSCAPS2_VOLUME:
            return "DDSCAPS2_VOLUME";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* dds_caps3_str(uint32_t flags)
{
    switch (flags)
    {
        case data::DDSCAPS3_NONE:
            return "DDSCAPS3_NONE";
        default:
            break;
    }
    return "UNKOWN";
}

static char const* dds_caps4_str(uint32_t flags)
{
    switch (flags)
    {
        case data::DDSCAPS4_NONE:
            return "DDSCAPS4_NONE";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* dxgi_format_str(uint32_t format)
{
    switch (format)
    {
        case data::DXGI_FORMAT_UNKNOWN:
            return "DXGI_FORMAT_UNKNOWN";
        case data::DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return "DXGI_FORMAT_R32G32B32A32_TYPELESS";
        case data::DXGI_FORMAT_R32G32B32A32_FLOAT:
            return "DXGI_FORMAT_R32G32B32A32_FLOAT";
        case data::DXGI_FORMAT_R32G32B32A32_UINT:
            return "DXGI_FORMAT_R32G32B32A32_UINT";
        case data::DXGI_FORMAT_R32G32B32A32_SINT:
            return "DXGI_FORMAT_R32G32B32A32_SINT";
        case data::DXGI_FORMAT_R32G32B32_TYPELESS:
            return "DXGI_FORMAT_R32G32B32_TYPELESS";
        case data::DXGI_FORMAT_R32G32B32_FLOAT:
            return "DXGI_FORMAT_R32G32B32_FLOAT";
        case data::DXGI_FORMAT_R32G32B32_UINT:
            return "DXGI_FORMAT_R32G32B32_UINT";
        case data::DXGI_FORMAT_R32G32B32_SINT:
            return "DXGI_FORMAT_R32G32B32_SINT";
        case data::DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return "DXGI_FORMAT_R16G16B16A16_TYPELESS";
        case data::DXGI_FORMAT_R16G16B16A16_FLOAT:
            return "DXGI_FORMAT_R16G16B16A16_FLOAT";
        case data::DXGI_FORMAT_R16G16B16A16_UNORM:
            return "DXGI_FORMAT_R16G16B16A16_UNORM";
        case data::DXGI_FORMAT_R16G16B16A16_UINT:
            return "DXGI_FORMAT_R16G16B16A16_UINT";
        case data::DXGI_FORMAT_R16G16B16A16_SNORM:
            return "DXGI_FORMAT_R16G16B16A16_SNORM";
        case data::DXGI_FORMAT_R16G16B16A16_SINT:
            return "DXGI_FORMAT_R16G16B16A16_SINT";
        case data::DXGI_FORMAT_R32G32_TYPELESS:
            return "DXGI_FORMAT_R32G32_TYPELESS";
        case data::DXGI_FORMAT_R32G32_FLOAT:
            return "DXGI_FORMAT_R32G32_FLOAT";
        case data::DXGI_FORMAT_R32G32_UINT:
            return "DXGI_FORMAT_R32G32_UINT";
        case data::DXGI_FORMAT_R32G32_SINT:
            return "DXGI_FORMAT_R32G32_SINT";
        case data::DXGI_FORMAT_R32G8X24_TYPELESS:
            return "DXGI_FORMAT_R32G8X24_TYPELESS";
        case data::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return "DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
        case data::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
        case data::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";
        case data::DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return "DXGI_FORMAT_R10G10B10A2_TYPELESS";
        case data::DXGI_FORMAT_R10G10B10A2_UNORM:
            return "DXGI_FORMAT_R10G10B10A2_UNORM";
        case data::DXGI_FORMAT_R10G10B10A2_UINT:
            return "DXGI_FORMAT_R10G10B10A2_UINT";
        case data::DXGI_FORMAT_R11G11B10_FLOAT:
            return "DXGI_FORMAT_R11G11B10_FLOAT";
        case data::DXGI_FORMAT_R8G8B8A8_TYPELESS:
            return "DXGI_FORMAT_R8G8B8A8_TYPELESS";
        case data::DXGI_FORMAT_R8G8B8A8_UNORM:
            return "DXGI_FORMAT_R8G8B8A8_UNORM";
        case data::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
        case data::DXGI_FORMAT_R8G8B8A8_UINT:
            return "DXGI_FORMAT_R8G8B8A8_UINT";
        case data::DXGI_FORMAT_R8G8B8A8_SNORM:
            return "DXGI_FORMAT_R8G8B8A8_SNORM";
        case data::DXGI_FORMAT_R8G8B8A8_SINT:
            return "DXGI_FORMAT_R8G8B8A8_SINT";
        case data::DXGI_FORMAT_R16G16_TYPELESS:
            return "DXGI_FORMAT_R16G16_TYPELESS";
        case data::DXGI_FORMAT_R16G16_FLOAT:
            return "DXGI_FORMAT_R16G16_FLOAT";
        case data::DXGI_FORMAT_R16G16_UNORM:
            return "DXGI_FORMAT_R16G16_UNORM";
        case data::DXGI_FORMAT_R16G16_UINT:
            return "DXGI_FORMAT_R16G16_UINT";
        case data::DXGI_FORMAT_R16G16_SNORM:
            return "DXGI_FORMAT_R16G16_SNORM";
        case data::DXGI_FORMAT_R16G16_SINT:
            return "DXGI_FORMAT_R16G16_SINT";
        case data::DXGI_FORMAT_R32_TYPELESS:
            return "DXGI_FORMAT_R32_TYPELESS";
        case data::DXGI_FORMAT_D32_FLOAT:
            return "DXGI_FORMAT_D32_FLOAT";
        case data::DXGI_FORMAT_R32_FLOAT:
            return "DXGI_FORMAT_R32_FLOAT";
        case data::DXGI_FORMAT_R32_UINT:
            return "DXGI_FORMAT_R32_UINT";
        case data::DXGI_FORMAT_R32_SINT:
            return "DXGI_FORMAT_R32_SINT";
        case data::DXGI_FORMAT_R24G8_TYPELESS:
            return "DXGI_FORMAT_R24G8_TYPELESS";
        case data::DXGI_FORMAT_D24_UNORM_S8_UINT:
            return "DXGI_FORMAT_D24_UNORM_S8_UINT";
        case data::DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return "DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
        case data::DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return "DXGI_FORMAT_X24_TYPELESS_G8_UINT";
        case data::DXGI_FORMAT_R8G8_TYPELESS:
            return "DXGI_FORMAT_R8G8_TYPELESS";
        case data::DXGI_FORMAT_R8G8_UNORM:
            return "DXGI_FORMAT_R8G8_UNORM";
        case data::DXGI_FORMAT_R8G8_UINT:
            return "DXGI_FORMAT_R8G8_UINT";
        case data::DXGI_FORMAT_R8G8_SNORM:
            return "DXGI_FORMAT_R8G8_SNORM";
        case data::DXGI_FORMAT_R8G8_SINT:
            return "DXGI_FORMAT_R8G8_SINT";
        case data::DXGI_FORMAT_R16_TYPELESS:
            return "DXGI_FORMAT_R16_TYPELESS";
        case data::DXGI_FORMAT_R16_FLOAT:
            return "DXGI_FORMAT_R16_FLOAT";
        case data::DXGI_FORMAT_D16_UNORM:
            return "DXGI_FORMAT_D16_UNORM";
        case data::DXGI_FORMAT_R16_UNORM:
            return "DXGI_FORMAT_R16_UNORM";
        case data::DXGI_FORMAT_R16_UINT:
            return "DXGI_FORMAT_R16_UINT";
        case data::DXGI_FORMAT_R16_SNORM:
            return "DXGI_FORMAT_R16_SNORM";
        case data::DXGI_FORMAT_R16_SINT:
            return "DXGI_FORMAT_R16_SINT";
        case data::DXGI_FORMAT_R8_TYPELESS:
            return "DXGI_FORMAT_R8_TYPELESS";
        case data::DXGI_FORMAT_R8_UNORM:
            return "DXGI_FORMAT_R8_UNORM";
        case data::DXGI_FORMAT_R8_UINT:
            return "DXGI_FORMAT_R8_UINT";
        case data::DXGI_FORMAT_R8_SNORM:
            return "DXGI_FORMAT_R8_SNORM";
        case data::DXGI_FORMAT_R8_SINT:
            return "DXGI_FORMAT_R8_SINT";
        case data::DXGI_FORMAT_A8_UNORM:
            return "DXGI_FORMAT_A8_UNORM";
        case data::DXGI_FORMAT_R1_UNORM:
            return "DXGI_FORMAT_R1_UNORM";
        case data::DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return "DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
        case data::DXGI_FORMAT_R8G8_B8G8_UNORM:
            return "DXGI_FORMAT_R8G8_B8G8_UNORM";
        case data::DXGI_FORMAT_G8R8_G8B8_UNORM:
            return "DXGI_FORMAT_G8R8_G8B8_UNORM";
        case data::DXGI_FORMAT_BC1_TYPELESS:
            return "DXGI_FORMAT_BC1_TYPELESS";
        case data::DXGI_FORMAT_BC1_UNORM:
            return "DXGI_FORMAT_BC1_UNORM";
        case data::DXGI_FORMAT_BC1_UNORM_SRGB:
            return "DXGI_FORMAT_BC1_UNORM_SRGB";
        case data::DXGI_FORMAT_BC2_TYPELESS:
            return "DXGI_FORMAT_BC2_TYPELESS";
        case data::DXGI_FORMAT_BC2_UNORM:
            return "DXGI_FORMAT_BC2_UNORM";
        case data::DXGI_FORMAT_BC2_UNORM_SRGB:
            return "DXGI_FORMAT_BC2_UNORM_SRGB";
        case data::DXGI_FORMAT_BC3_TYPELESS:
            return "DXGI_FORMAT_BC3_TYPELESS";
        case data::DXGI_FORMAT_BC3_UNORM:
            return "DXGI_FORMAT_BC3_UNORM";
        case data::DXGI_FORMAT_BC3_UNORM_SRGB:
            return "DXGI_FORMAT_BC3_UNORM_SRGB";
        case data::DXGI_FORMAT_BC4_TYPELESS:
            return "DXGI_FORMAT_BC4_TYPELESS";
        case data::DXGI_FORMAT_BC4_UNORM:
            return "DXGI_FORMAT_BC4_UNORM";
        case data::DXGI_FORMAT_BC4_SNORM:
            return "DXGI_FORMAT_BC4_SNORM";
        case data::DXGI_FORMAT_BC5_TYPELESS:
            return "DXGI_FORMAT_BC5_TYPELESS";
        case data::DXGI_FORMAT_BC5_UNORM:
            return "DXGI_FORMAT_BC5_UNORM";
        case data::DXGI_FORMAT_BC5_SNORM:
            return "DXGI_FORMAT_BC5_SNORM";
        case data::DXGI_FORMAT_B5G6R5_UNORM:
            return "DXGI_FORMAT_B5G6R5_UNORM";
        case data::DXGI_FORMAT_B5G5R5A1_UNORM:
            return "DXGI_FORMAT_B5G5R5A1_UNORM";
        case data::DXGI_FORMAT_B8G8R8A8_UNORM:
            return "DXGI_FORMAT_B8G8R8A8_UNORM";
        case data::DXGI_FORMAT_B8G8R8X8_UNORM:
            return "DXGI_FORMAT_B8G8R8X8_UNORM";
        case data::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
        case data::DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return "DXGI_FORMAT_B8G8R8A8_TYPELESS";
        case data::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
        case data::DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return "DXGI_FORMAT_B8G8R8X8_TYPELESS";
        case data::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";
        case data::DXGI_FORMAT_BC6H_TYPELESS:
            return "DXGI_FORMAT_BC6H_TYPELESS";
        case data::DXGI_FORMAT_BC6H_UF16:
            return "DXGI_FORMAT_BC6H_UF16";
        case data::DXGI_FORMAT_BC6H_SF16:
            return "DXGI_FORMAT_BC6H_SF16";
        case data::DXGI_FORMAT_BC7_TYPELESS:
            return "DXGI_FORMAT_BC7_TYPELESS";
        case data::DXGI_FORMAT_BC7_UNORM:
            return "DXGI_FORMAT_BC7_UNORM";
        case data::DXGI_FORMAT_BC7_UNORM_SRGB:
            return "DXGI_FORMAT_BC7_UNORM_SRGB";
        case data::DXGI_FORMAT_AYUV:
            return "DXGI_FORMAT_AYUV";
        case data::DXGI_FORMAT_Y410:
            return "DXGI_FORMAT_Y410";
        case data::DXGI_FORMAT_Y416:
            return "DXGI_FORMAT_Y416";
        case data::DXGI_FORMAT_NV12:
            return "DXGI_FORMAT_NV12";
        case data::DXGI_FORMAT_P010:
            return "DXGI_FORMAT_P010";
        case data::DXGI_FORMAT_P016:
            return "DXGI_FORMAT_P016";
        case data::DXGI_FORMAT_420_OPAQUE:
            return "DXGI_FORMAT_420_OPAQUE";
        case data::DXGI_FORMAT_YUY2:
            return "DXGI_FORMAT_YUY2";
        case data::DXGI_FORMAT_Y210:
            return "DXGI_FORMAT_Y210";
        case data::DXGI_FORMAT_Y216:
            return "DXGI_FORMAT_Y216";
        case data::DXGI_FORMAT_NV11:
            return "DXGI_FORMAT_NV11";
        case data::DXGI_FORMAT_AI44:
            return "DXGI_FORMAT_AI44";
        case data::DXGI_FORMAT_IA44:
            return "DXGI_FORMAT_IA44";
        case data::DXGI_FORMAT_P8:
            return "DXGI_FORMAT_P8";
        case data::DXGI_FORMAT_A8P8:
            return "DXGI_FORMAT_A8P8";
        case data::DXGI_FORMAT_B4G4R4A4_UNORM:
            return "DXGI_FORMAT_B4G4R4A4_UNORM";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* d3d11_resource_dimension_str(uint32_t dim)
{
    switch (dim)
    {
        case data::D3D11_RESOURCE_DIMENSION_UNKNOWN:
            return "D3D11_RESOURCE_DIMENSION_UNKNOWN";
        case data::D3D11_RESOURCE_DIMENSION_BUFFER:
            return "D3D11_RESOURCE_DIMENSION_BUFFER";
        case data::D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            return "D3D11_RESOURCE_DIMENSION_TEXTURE1D";
        case data::D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            return "D3D11_RESOURCE_DIMENSION_TEXTURE2D";
        case data::D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            return "D3D11_RESOURCE_DIMENSION_TEXTURE3D";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* d3d11_resource_misc_flag_str(uint32_t flags)
{
    switch (flags)
    {
        case data::D3D11_RESOURCE_MISC_TEXTURECUBE:
            return "D3D11_RESOURCE_MISC_TEXTURECUBE";
        default:
            break;
    }
    return "UNKNOWN";
}

static char const* alpha_mode_str(uint32_t flags)
{
    switch (flags)
    {
        case data::DDS_ALPHA_MODE_UNKNOWN:
            return "DDS_ALPHA_MODE_UNKNOWN";
        case data::DDS_ALPHA_MODE_STRAIGHT:
            return "DDS_ALPHA_MODE_STRAIGHT";
        case data::DDS_ALPHA_MODE_PREMULTIPLIED:
            return "DDS_ALPHA_MODE_PREMULTIPLIED";
        case data::DDS_ALPHA_MODE_OPAQUE:
            return "DDS_ALPHA_MODE_OPAQUE";
        case data::DDS_ALPHA_MODE_CUSTOM:
            return "DDS_ALPHA_MODE_CUSTOM";
        default:
            break;
    }
    return "UNKNOWN";
}

static void print_ddpf(FILE *fp, data::dds_pixelformat_t const *ddpf)
{
    fprintf(fp, "DDS_PIXELFORMAT:\n");
    fprintf(fp, "  Size:        %u\n", ddpf->Size);
    fprintf(fp, "  Flags:       ");
    for (size_t i = 0, n = 0; i < DDPF_COUNT; ++i)
    {
        if ((n == 0 && ddpf->Flags == data::DDPF_NONE) || (ddpf->Flags & DDPF_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", ddpf_str(DDPF_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "  FourCC:      %s\n", fourcc_le_str(ddpf->FourCC));
    fprintf(fp, "  RGBBitCount: %u\n", ddpf->RGBBitCount);
    fprintf(fp, "  BitMaskR:    0x%08X\n", ddpf->BitMaskR);
    fprintf(fp, "  BitMaskG:    0x%08X\n", ddpf->BitMaskG);
    fprintf(fp, "  BitMaskB:    0x%08X\n", ddpf->BitMaskB);
    fprintf(fp, "  BitMaskA:    0x%08X\n", ddpf->BitMaskA);
    fprintf(fp, "\n");
}

static void print_header(FILE *fp, data::dds_header_t const *head)
{
    fprintf(fp, "DDS_HEADER:\n");
    fprintf(fp, "  Size:        %u\n", head->Size);
    fprintf(fp, "  Flags:       ");
    for (size_t i = 0, n = 0; i < DDSD_COUNT; ++i)
    {
        if ((n == 0 && head->Flags == data::DDSD_NONE) || (head->Flags & DDSD_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", ddsd_str(DDSD_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "  Width:       %u\n", head->Width);
    fprintf(fp, "  Height:      %u\n", head->Height);
    fprintf(fp, "  Depth:       %u\n", head->Depth);
    fprintf(fp, "  Pitch:       %u\n", head->Pitch);
    fprintf(fp, "  Levels:      %u\n", head->Levels);
    print_ddpf(fp, &head->Format);
    fprintf(fp, "  Caps:        ");
    for (size_t i = 0, n = 0; i < DDSCAPS_COUNT; ++i)
    {
        if ((n == 0 && head->Caps == data::DDSCAPS_NONE) || (head->Caps & DDSCAPS_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", dds_caps_str(DDSCAPS_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "  Caps2:       ");
    for (size_t i = 0, n = 0; i < DDSCAPS2_COUNT; ++i)
    {
        if ((n == 0 && head->Caps2 == data::DDSCAPS2_NONE) || (head->Caps2 & DDSCAPS2_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", dds_caps2_str(DDSCAPS2_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "  Caps3:       ");
    for (size_t i = 0, n = 0; i < DDSCAPS3_COUNT; ++i)
    {
        if ((n == 0 && head->Caps3 == data::DDSCAPS3_NONE) || (head->Caps2 & DDSCAPS3_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", dds_caps3_str(DDSCAPS3_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "  Caps4:       ");
    for (size_t i = 0, n = 0; i < DDSCAPS4_COUNT; ++i)
    {
        if ((n == 0 && head->Caps4 == data::DDSCAPS4_NONE) || (head->Caps2 & DDSCAPS4_FLAGS[i]))
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", dds_caps4_str(DDSCAPS4_FLAGS[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "\n");
}

static void print_header_ex(FILE *fp, data::dds_header_dxt10_t const *head)
{
    if (head == NULL)
    {
        fprintf(fp, "D3D_HEADER_DXT10:\n");
        fprintf(fp, "  Not Present.\n");
        fprintf(fp, "\n");
        return;
    }

    fprintf(fp, "D3D_HEADER_DXT10:\n");
    fprintf(fp, "  Format:      %s\n", dxgi_format_str(head->Format));
    fprintf(fp, "  Dimension:   %s\n", d3d11_resource_dimension_str(head->Dimension));
    fprintf(fp, "  Flags:       ");
    for (size_t i = 0, n = 0; i < RESOURCE_MISC_FLAG_COUNT; ++i)
    {
        if (head->Flags & RESOURCE_MISC_FLAG[i])
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", d3d11_resource_misc_flag_str(RESOURCE_MISC_FLAG[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "  Array Size:  %u\n", head->ArraySize);
    fprintf(fp, "  Flags2:      ");
    for (size_t i = 0, n = 0; i < ALPHA_MODE_COUNT; ++i)
    {
        if (head->Flags & ALPHA_MODE[i])
        {
            if ((n != 0))
                fprintf(fp, " | ");
            fprintf(fp, "%s", alpha_mode_str(ALPHA_MODE[i]));
            n++;
        }
    }
    fprintf(fp, "\n");
    fprintf(fp, "\n");
}

static void print_leveldesc(FILE *fp, data::dds_level_desc_t const *desc)
{
    fprintf(fp, "DDS_LEVEL_DESC:\n");
    fprintf(fp, "  Index:       %u\n", uint32_t(desc->Index));
    fprintf(fp, "  Width:       %u\n", uint32_t(desc->Width));
    fprintf(fp, "  Height:      %u\n", uint32_t(desc->Height));
    fprintf(fp, "  Slices:      %u\n", uint32_t(desc->Slices));
    fprintf(fp, "  BytesPerEl:  %u\n", uint32_t(desc->BytesPerElement));
    fprintf(fp, "  BytesPerRow: %u\n", uint32_t(desc->BytesPerRow));
    fprintf(fp, "  BytesPerSlc: %u\n", uint32_t(desc->BytesPerSlice));
    fprintf(fp, "  DataSize:    %u\n", uint32_t(desc->DataSize));
    fprintf(fp, "  LevelData:   %p\n", desc->LevelData);
    fprintf(fp, "  Format:      %s\n", dxgi_format_str(desc->Format));
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
        printf("USAGE: ddsinfo path/to/file.dds\n");
        exit(EXIT_FAILURE);
    }

    size_t dds_size = 0;
    void  *dds_data = data::load_binary(argv[1], &dds_size);
    if (dds_data == NULL)
    {
        printf("ERROR: Input file \'%s\' not found.\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("INFO: Loaded \'%s\', %u bytes.\n", argv[1], uint32_t(dds_size));
    }

    size_t                    count     =  0;
    size_t                   nitems     =  0;
    size_t                   nlevels    =  0;
    data::dds_level_desc_t   *levels    = NULL;
    data::dds_header_dxt10_t *h_ex      = NULL;
    data::dds_header_t        header    = {0};
    data::dds_header_dxt10_t  header_ex = {0};
    if (!data::dds_header(dds_data, dds_size, &header))
    {
        printf("ERROR: File does not appear to be a valid DDS.\n");
        goto cleanup_error;
    }
    // @note: need to change the API so it looks at the FourCC.
    // clients can't be relied upon to create and manage a separate pointer,
    // and doing the logical thing will result in incorrect behavior as-is.
    if (!data::dds_header_dxt10(dds_data, dds_size, &header_ex))
    {
        printf("INFO: No extended header present.\n");
        h_ex = NULL;
    }
    else
    {
        printf("INFO: Found extended header.\n");
        h_ex = &header_ex;
    }

    nitems   = data::dds_array_count(&header, h_ex);
    nlevels  = data::dds_level_count(&header, h_ex);
    if (nitems == 0 && nlevels == 0)
    {
        printf("ERROR: File appears invalid; no items or levels.\n");
        goto cleanup_error;
    }
    else
    {
        printf("INFO: Found %u surface(s), (each) with %u levels.\n", uint32_t(nitems), uint32_t(nlevels));
    }

    levels = (data::dds_level_desc_t*) malloc(nitems * nlevels * sizeof(data::dds_level_desc_t));
    count  =  data::dds_describe(dds_data, dds_size, &header, h_ex, levels, nitems * nlevels);
    if (count == 0)
    {
        printf("ERROR: Failed to describe the surface(s).\n");
        goto cleanup_error;
    }
    else
    {
        printf("INFO: Described %u/%u level(s).\n", uint32_t(count), uint32_t(nlevels * nitems));
    }

    print_header(stdout, &header);
    print_header_ex(stdout, h_ex);
    for (size_t i = 0; i < count; ++i)
    {
        print_leveldesc(stdout, &levels[i]);
    }

    free(levels);
    free(dds_data);
    exit(EXIT_SUCCESS);

cleanup_error:
    if (levels) free(levels);
    if (dds_data) free(dds_data);
    exit(EXIT_FAILURE);
}

