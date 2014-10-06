/*/////////////////////////////////////////////////////////////////////////////
/// @summary
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include "gldraw2d.hpp"
#include "llgui.hpp"

/*/////////////////
//   Constants   //
/////////////////*/
#define SETP(x, v)  if ((x) != NULL) *(x) = (v)

/*///////////////
//   Globals   //
///////////////*/


/*///////////////////////
//   Local Functions   //
///////////////////////*/


/*///////////////////////
//  Public Functions   //
///////////////////////*/
bool r2d::dxgi_format_to_gl(uint32_t dxgi, GLenum *out_internalformat, GLenum *out_format, GLenum *out_datatype)
{
    switch (dxgi)
    {
        case data::DXGI_FORMAT_UNKNOWN:
        case data::DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case data::DXGI_FORMAT_R32G32B32_TYPELESS:
        case data::DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case data::DXGI_FORMAT_R32G32_TYPELESS:
        case data::DXGI_FORMAT_R32G8X24_TYPELESS:
        case data::DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case data::DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case data::DXGI_FORMAT_R16G16_TYPELESS:
        case data::DXGI_FORMAT_R32_TYPELESS:
        case data::DXGI_FORMAT_R24G8_TYPELESS:
        case data::DXGI_FORMAT_R8G8_TYPELESS:
        case data::DXGI_FORMAT_R16_TYPELESS:
        case data::DXGI_FORMAT_R8_TYPELESS:
        case data::DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case data::DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case data::DXGI_FORMAT_R1_UNORM:
        case data::DXGI_FORMAT_R8G8_B8G8_UNORM:
        case data::DXGI_FORMAT_G8R8_G8B8_UNORM:
        case data::DXGI_FORMAT_BC1_TYPELESS:
        case data::DXGI_FORMAT_BC2_TYPELESS:
        case data::DXGI_FORMAT_BC2_UNORM:
        case data::DXGI_FORMAT_BC2_UNORM_SRGB:
        case data::DXGI_FORMAT_BC3_TYPELESS:
        case data::DXGI_FORMAT_BC4_TYPELESS:
        case data::DXGI_FORMAT_BC4_UNORM:
        case data::DXGI_FORMAT_BC4_SNORM:
        case data::DXGI_FORMAT_BC5_TYPELESS:
        case data::DXGI_FORMAT_BC5_SNORM:
        case data::DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case data::DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case data::DXGI_FORMAT_BC6H_TYPELESS:
        case data::DXGI_FORMAT_BC7_TYPELESS:
        case data::DXGI_FORMAT_AYUV:
        case data::DXGI_FORMAT_Y410:
        case data::DXGI_FORMAT_Y416:
        case data::DXGI_FORMAT_NV12:
        case data::DXGI_FORMAT_P010:
        case data::DXGI_FORMAT_P016:
        case data::DXGI_FORMAT_420_OPAQUE:
        case data::DXGI_FORMAT_YUY2:
        case data::DXGI_FORMAT_Y210:
        case data::DXGI_FORMAT_Y216:
        case data::DXGI_FORMAT_NV11:
        case data::DXGI_FORMAT_AI44:
        case data::DXGI_FORMAT_IA44:
            break;
        case data::DXGI_FORMAT_R32G32B32A32_FLOAT:
            SETP(out_internalformat, GL_RGBA32F);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_FLOAT);
            return true;
        case data::DXGI_FORMAT_R32G32B32A32_UINT:
            SETP(out_internalformat, GL_RGBA32UI);
            SETP(out_format        , GL_BGRA_INTEGER);
            SETP(out_datatype      , GL_UNSIGNED_INT);
            return true;
        case data::DXGI_FORMAT_R32G32B32A32_SINT:
            SETP(out_internalformat, GL_RGBA32I);
            SETP(out_format        , GL_BGRA_INTEGER);
            SETP(out_datatype      , GL_INT);
            return true;
        case data::DXGI_FORMAT_R32G32B32_FLOAT:
            SETP(out_internalformat, GL_RGB32F);
            SETP(out_format        , GL_BGR);
            SETP(out_datatype      , GL_FLOAT);
            return true;
        case data::DXGI_FORMAT_R32G32B32_UINT:
            SETP(out_internalformat, GL_RGB32UI);
            SETP(out_format        , GL_BGR_INTEGER);
            SETP(out_datatype      , GL_UNSIGNED_INT);
            return true;
        case data::DXGI_FORMAT_R32G32B32_SINT:
            SETP(out_internalformat, GL_RGB32I);
            SETP(out_format        , GL_BGR_INTEGER);
            SETP(out_datatype      , GL_INT);
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_FLOAT:
            SETP(out_internalformat, GL_RGBA16F);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_HALF_FLOAT);
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_UNORM:
            SETP(out_internalformat, GL_RGBA16);
            SETP(out_format        , GL_BGRA_INTEGER);
            SETP(out_datatype      , GL_UNSIGNED_SHORT);
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_UINT:
            SETP(out_internalformat, GL_RGBA16UI);
            SETP(out_format        , GL_BGRA_INTEGER);
            SETP(out_datatype      , GL_UNSIGNED_SHORT);
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_SNORM:
            SETP(out_internalformat, GL_RGBA16_SNORM);
            SETP(out_format        , GL_BGRA_INTEGER);
            SETP(out_datatype      , GL_SHORT);
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_SINT:
            SETP(out_internalformat, GL_RGBA16I);
            SETP(out_format        , GL_BGRA_INTEGER);
            SETP(out_datatype      , GL_SHORT);
            return true;
        case data::DXGI_FORMAT_R32G32_FLOAT:
            SETP(out_internalformat, GL_RG32F);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_FLOAT);
            return true;
        case data::DXGI_FORMAT_R32G32_UINT:
            SETP(out_internalformat, GL_RG32UI);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_UNSIGNED_INT);
            return true;
        case data::DXGI_FORMAT_R32G32_SINT:
            SETP(out_internalformat, GL_RG32I);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_INT);
            return true;
        case data::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            SETP(out_internalformat, GL_DEPTH_STENCIL);
            SETP(out_format        , GL_DEPTH_STENCIL);
            SETP(out_datatype      , GL_FLOAT); // ???
            return true;
        case data::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            SETP(out_internalformat, GL_RG32F);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_FLOAT);
            return true;
        case data::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return true;
        case data::DXGI_FORMAT_R10G10B10A2_UNORM:
            SETP(out_internalformat, GL_RGB10_A2);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_2_10_10_10_REV);
            return true;
        case data::DXGI_FORMAT_R10G10B10A2_UINT:
            SETP(out_internalformat, GL_RGB10_A2UI);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_2_10_10_10_REV);
            return true;
        case data::DXGI_FORMAT_R11G11B10_FLOAT:
            SETP(out_internalformat, GL_R11F_G11F_B10F);
            SETP(out_format        , GL_BGR);
            SETP(out_datatype      , GL_FLOAT); // ???
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_UNORM:
            SETP(out_internalformat, GL_RGBA8);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_8_8_8_8_REV);
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            SETP(out_internalformat, GL_SRGB8_ALPHA8);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_8_8_8_8_REV);
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_UINT:
            SETP(out_internalformat, GL_RGBA8UI);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_8_8_8_8_REV);
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_SNORM:
            SETP(out_internalformat, GL_RGBA8_SNORM);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_8_8_8_8_REV);
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_SINT:
            SETP(out_internalformat, GL_RGBA8I);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_BYTE);
            return true;
        case data::DXGI_FORMAT_R16G16_FLOAT:
            SETP(out_internalformat, GL_RG16F);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_HALF_FLOAT);
            return true;
        case data::DXGI_FORMAT_R16G16_UNORM:
            SETP(out_internalformat, GL_RG16);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_UNSIGNED_SHORT);
            return true;
        case data::DXGI_FORMAT_R16G16_UINT:
            SETP(out_internalformat, GL_RG16UI);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_UNSIGNED_SHORT);
            return true;
        case data::DXGI_FORMAT_R16G16_SNORM:
            SETP(out_internalformat, GL_RG16_SNORM);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_SHORT);
            return true;
        case data::DXGI_FORMAT_R16G16_SINT:
            SETP(out_internalformat, GL_RG16I);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_SHORT);
            return true;
        case data::DXGI_FORMAT_D32_FLOAT:
            SETP(out_internalformat, GL_DEPTH_COMPONENT);
            SETP(out_format        , GL_DEPTH_COMPONENT);
            SETP(out_datatype      , GL_FLOAT);
            return true;
        case data::DXGI_FORMAT_R32_FLOAT:
            SETP(out_internalformat, GL_R32F);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_FLOAT);
            return true;
        case data::DXGI_FORMAT_R32_UINT:
            SETP(out_internalformat, GL_R32UI);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_UNSIGNED_INT);
            return true;
        case data::DXGI_FORMAT_R32_SINT:
            SETP(out_internalformat, GL_R32I);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_INT);
            return true;
        case data::DXGI_FORMAT_D24_UNORM_S8_UINT:
            SETP(out_internalformat, GL_DEPTH_STENCIL);
            SETP(out_format        , GL_DEPTH_STENCIL);
            SETP(out_datatype      , GL_UNSIGNED_INT);
            return true;
        case data::DXGI_FORMAT_R8G8_UNORM:
            SETP(out_internalformat, GL_RG8);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_R8G8_UINT:
            SETP(out_internalformat, GL_RG8UI);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_R8G8_SNORM:
            SETP(out_internalformat, GL_RG8_SNORM);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_BYTE);
            return true;
        case data::DXGI_FORMAT_R8G8_SINT:
            SETP(out_internalformat, GL_RG8I);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_BYTE);
            return true;
        case data::DXGI_FORMAT_R16_FLOAT:
            SETP(out_internalformat, GL_R16F);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_HALF_FLOAT);
            return true;
        case data::DXGI_FORMAT_D16_UNORM:
            SETP(out_internalformat, GL_DEPTH_COMPONENT);
            SETP(out_format        , GL_DEPTH_COMPONENT);
            SETP(out_datatype      , GL_UNSIGNED_SHORT);
            return true;
        case data::DXGI_FORMAT_R16_UNORM:
            SETP(out_internalformat, GL_R16);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_UNSIGNED_SHORT);
            return true;
        case data::DXGI_FORMAT_R16_UINT:
            SETP(out_internalformat, GL_R16UI);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_UNSIGNED_SHORT);
            return true;
        case data::DXGI_FORMAT_R16_SNORM:
            SETP(out_internalformat, GL_R16_SNORM);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_SHORT);
            return true;
        case data::DXGI_FORMAT_R16_SINT:
            SETP(out_internalformat, GL_R16I);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_SHORT);
            return true;
        case data::DXGI_FORMAT_R8_UNORM:
            SETP(out_internalformat, GL_R8);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_R8_UINT:
            SETP(out_internalformat, GL_R8UI);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_R8_SNORM:
            SETP(out_internalformat, GL_R8_SNORM);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_BYTE);
            return true;
        case data::DXGI_FORMAT_R8_SINT:
            SETP(out_internalformat, GL_R8I);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_BYTE);
            return true;
        case data::DXGI_FORMAT_A8_UNORM:
            SETP(out_internalformat, GL_R8);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            SETP(out_internalformat, GL_RGB9_E5);
            SETP(out_format        , GL_RGB);
            SETP(out_datatype      , GL_UNSIGNED_INT);
            return true;
        case data::DXGI_FORMAT_BC1_UNORM:
            SETP(out_internalformat, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
            SETP(out_format        , GL_RGBA);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_BC1_UNORM_SRGB:
            SETP(out_internalformat, 0x8C4D);
            SETP(out_format        , GL_RGBA);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_BC3_UNORM:
            SETP(out_internalformat, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
            SETP(out_format        , GL_RGBA);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_BC3_UNORM_SRGB:
            SETP(out_internalformat, 0x8C4E);
            SETP(out_format        , GL_RGBA);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_BC5_UNORM:
            SETP(out_internalformat, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
            SETP(out_format        , GL_RGBA);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_B5G6R5_UNORM:
            SETP(out_internalformat, GL_RGB);
            SETP(out_format        , GL_BGR);
            SETP(out_datatype      , GL_UNSIGNED_SHORT_5_6_5_REV);
            return true;
        case data::DXGI_FORMAT_B5G5R5A1_UNORM:
            SETP(out_internalformat, GL_RGBA);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_SHORT_1_5_5_5_REV);
            return true;
        case data::DXGI_FORMAT_B8G8R8A8_UNORM:
            SETP(out_internalformat, GL_RGBA8);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_8_8_8_8_REV);
            return true;
        case data::DXGI_FORMAT_B8G8R8X8_UNORM:
            SETP(out_internalformat, GL_RGBA8);
            SETP(out_format        , GL_BGR);
            SETP(out_datatype      , GL_UNSIGNED_INT_8_8_8_8_REV);
            return true;
        case data::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            SETP(out_internalformat, GL_RGB10_A2);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_2_10_10_10_REV);
            return true;
        case data::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            SETP(out_internalformat, GL_SRGB8_ALPHA8);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_INT_8_8_8_8_REV);
            return true;
        case data::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            SETP(out_internalformat, GL_SRGB8_ALPHA8);
            SETP(out_format        , GL_BGR);
            SETP(out_datatype      , GL_UNSIGNED_INT_8_8_8_8_REV);
            return true;
        case data::DXGI_FORMAT_BC6H_UF16:
            SETP(out_internalformat, 0x8E8F);
            SETP(out_format        , GL_RGB);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_BC6H_SF16:
            SETP(out_internalformat, 0x8E8E);
            SETP(out_format        , GL_RGB);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_BC7_UNORM:
            SETP(out_internalformat, 0x8E8C);
            SETP(out_format        , GL_RGBA);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_BC7_UNORM_SRGB:
            SETP(out_internalformat, 0x8E8D);
            SETP(out_format        , GL_RGBA);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_P8:
            SETP(out_internalformat, GL_R8);
            SETP(out_format        , GL_RED);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_A8P8:
            SETP(out_internalformat, GL_RG8);
            SETP(out_format        , GL_RG);
            SETP(out_datatype      , GL_UNSIGNED_BYTE);
            return true;
        case data::DXGI_FORMAT_B4G4R4A4_UNORM:
            SETP(out_internalformat, GL_RGBA4);
            SETP(out_format        , GL_BGRA);
            SETP(out_datatype      , GL_UNSIGNED_SHORT_4_4_4_4_REV);
            return true;
        default:
            break;
    }
    return false;
}
