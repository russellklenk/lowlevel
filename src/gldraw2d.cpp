/*/////////////////////////////////////////////////////////////////////////////
/// @summary
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <assert.h>
#include <string.h>
#include "gldraw2d.hpp"
#include "llgui.hpp"

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary The desired number of uint32_t names per-bucket.
static size_t const IMAGE_CACHE_NAMES_PER_BUCKET = 16;

/// @summary The default capacity of an image cache, in images.
static size_t const IMAGE_CACHE_DEFAULT_CAPACITY = 1024;

/// @summary The zero-based index of the first name within a bucket.
static size_t const IMAGE_CACHE_FIRST_NAME       = 2;

/// @summary Define the minimum number of name buckets allocated per-image cache.
static size_t const IMAGE_CACHE_MIN_BUCKET_COUNT = IMAGE_CACHE_DEFAULT_CAPACITY / IMAGE_CACHE_NAMES_PER_BUCKET;

/// @summary Define the default capacity of the TexturePages texture ID storage.
static size_t const IMAGE_CACHE_PAGE_CAPACITY    = 4;

/*///////////////
//   Globals   //
///////////////*/

/*//////////////////
//   Data Types   //
//////////////////*/

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Attempt to find a node that can contain the given area within the
/// rectangle packer binary tree, performing any necessary spatial subdivision.
/// @param p The rectangle packer being updated.
/// @param n The zero-based index of the node at which the insertion is being attempted.
/// @param w The width of the rectangle being inserted.
/// @param h The height of the rectangle being inserted.
/// @return The node at which the rectangle should be inserted, or NULL if the
/// rectangle cannot fit within the remaining area of the master rectangle.
static r2d::pknode_t* node_insert(r2d::packer_t *p, uint32_t n, size_t w, size_t h)
{
    r2d::pknode_t *node  = &p->Nodes[n];
    if (node->Child[0]  != 0 && node->Child[1] != 0)
    {
        // this node is not a leaf node, so attempt the insert in the subtree.
        r2d::pknode_t *t = node_insert(p, node->Child[0], w, h);
        if (t != NULL)   return t;
        else return node_insert(p, node->Child[1], w, h);
    }
    else
    {
        if (node->Flags & r2d::PACKER_FLAGS_USED)
            return NULL;

        // if the sub-rect won't fit, don't continue down this path.
        uint32_t rect_width  = node->Bound[2] - node->Bound[0];
        uint32_t rect_height = node->Bound[3] - node->Bound[1];
        if (w  > rect_width || h  > rect_height)
            return NULL;

        // if the sub-rect fits exactly, we'll store it at 'node'.
        if (w == rect_width && h == rect_height)
            return node;

        // otherwise, we'll split the space at this node into a used
        // portion, stored in node->Child[0], and an unsed portion in
        // stored in node->Child[1].
        r2d::pknode_t  a;
        a.Flags      = r2d::PACKER_FLAGS_NONE;
        a.Index      = 0xFFFFFFFFU;
        a.Child[0]   = 0;
        a.Child[1]   = 0;

        r2d::pknode_t  b;
        b.Flags      = r2d::PACKER_FLAGS_NONE;
        b.Index      = 0xFFFFFFFFU;
        b.Child[0]   = 0;
        b.Child[1]   = 0;

        uint32_t dw  = rect_width  - w;
        uint32_t dh  = rect_height - h;
        if (dw > dh)
        {
            a.Bound[0] = node->Bound[0];
            a.Bound[1] = node->Bound[1];
            a.Bound[2] = node->Bound[0] + uint32_t(w);
            a.Bound[3] = node->Bound[3];

            b.Bound[0] = node->Bound[0] + uint32_t(w);
            b.Bound[1] = node->Bound[1];
            b.Bound[2] = node->Bound[2];
            b.Bound[3] = node->Bound[3];
        }
        else
        {
            a.Bound[0] = node->Bound[0];
            a.Bound[1] = node->Bound[1];
            a.Bound[2] = node->Bound[2];
            a.Bound[3] = node->Bound[1] + uint32_t(h);

            b.Bound[0] = node->Bound[0];
            b.Bound[1] = node->Bound[1] + uint32_t(h);
            b.Bound[2] = node->Bound[2];
            b.Bound[3] = node->Bound[3];
        }

        if (p->Count == p->Capacity)
        {
            size_t        nc =  p->Capacity  >  2048 ? (p->Capacity + 2048) : p->Capacity    * 2;
            r2d::pknode_t *N = (r2d::pknode_t*) realloc(p->Nodes, nc * sizeof(r2d::pknode_t) * 3);
            r2d::pkrect_t *R = (r2d::pkrect_t*) realloc(p->Rects, nc * sizeof(r2d::pkrect_t));
            if (N != NULL)   p->Nodes = N;
            if (R != NULL)   p->Rects = R;
            if (N != NULL && R != NULL) p->Capacity = nc;
            else return NULL;
        }

        size_t index_a =(p->Count * 3) + 1;
        size_t index_b =(p->Count * 3) + 2;
        node->Child[0] = index_a;
        node->Child[1] = index_b;
        p->Nodes[index_a] = a;
        p->Nodes[index_b] = b;
        return node_insert(p, node->Child[0], w, h);
    }
}

/*///////////////////////
//  Public Functions   //
///////////////////////*/
bool r2d::dxgi_format_to_gl(uint32_t dxgi, GLenum &out_internalformat, GLenum &out_format, GLenum &out_datatype)
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
            out_internalformat = GL_RGBA32F;
            out_format         = GL_BGRA;
            out_datatype       = GL_FLOAT;
            return true;
        case data::DXGI_FORMAT_R32G32B32A32_UINT:
            out_internalformat = GL_RGBA32UI;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case data::DXGI_FORMAT_R32G32B32A32_SINT:
            out_internalformat = GL_RGBA32I;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_INT;
            return true;
        case data::DXGI_FORMAT_R32G32B32_FLOAT:
            out_internalformat = GL_RGB32F;
            out_format         = GL_BGR;
            out_datatype       = GL_FLOAT;
            return true;
        case data::DXGI_FORMAT_R32G32B32_UINT:
            out_internalformat = GL_RGB32UI;
            out_format         = GL_BGR_INTEGER;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case data::DXGI_FORMAT_R32G32B32_SINT:
            out_internalformat = GL_RGB32I;
            out_format         = GL_BGR_INTEGER;
            out_datatype       = GL_INT;
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_FLOAT:
            out_internalformat = GL_RGBA16F;
            out_format         = GL_BGRA;
            out_datatype       = GL_HALF_FLOAT;
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_UNORM:
            out_internalformat = GL_RGBA16;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_UINT:
            out_internalformat = GL_RGBA16UI;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_SNORM:
            out_internalformat = GL_RGBA16_SNORM;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_SHORT;
            return true;
        case data::DXGI_FORMAT_R16G16B16A16_SINT:
            out_internalformat = GL_RGBA16I;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_SHORT;
            return true;
        case data::DXGI_FORMAT_R32G32_FLOAT:
            out_internalformat = GL_RG32F;
            out_format         = GL_RG;
            out_datatype       = GL_FLOAT;
            return true;
        case data::DXGI_FORMAT_R32G32_UINT:
            out_internalformat = GL_RG32UI;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case data::DXGI_FORMAT_R32G32_SINT:
            out_internalformat = GL_RG32I;
            out_format         = GL_RG;
            out_datatype       = GL_INT;
            return true;
        case data::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            out_internalformat = GL_DEPTH_STENCIL;
            out_format         = GL_DEPTH_STENCIL;
            out_datatype       = GL_FLOAT; // ???
            return true;
        case data::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            out_internalformat = GL_RG32F;
            out_format         = GL_RG;
            out_datatype       = GL_FLOAT;
            return true;
        case data::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return true;
        case data::DXGI_FORMAT_R10G10B10A2_UNORM:
            out_internalformat = GL_RGB10_A2;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_2_10_10_10_REV;
            return true;
        case data::DXGI_FORMAT_R10G10B10A2_UINT:
            out_internalformat = GL_RGB10_A2UI;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_2_10_10_10_REV;
            return true;
        case data::DXGI_FORMAT_R11G11B10_FLOAT:
            out_internalformat = GL_R11F_G11F_B10F;
            out_format         = GL_BGR;
            out_datatype       = GL_FLOAT; // ???
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_UNORM:
            out_internalformat = GL_RGBA8;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            out_internalformat = GL_SRGB8_ALPHA8;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_UINT:
            out_internalformat = GL_RGBA8UI;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_SNORM:
            out_internalformat = GL_RGBA8_SNORM;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case data::DXGI_FORMAT_R8G8B8A8_SINT:
            out_internalformat = GL_RGBA8I;
            out_format         = GL_BGRA;
            out_datatype       = GL_BYTE;
            return true;
        case data::DXGI_FORMAT_R16G16_FLOAT:
            out_internalformat = GL_RG16F;
            out_format         = GL_RG;
            out_datatype       = GL_HALF_FLOAT;
            return true;
        case data::DXGI_FORMAT_R16G16_UNORM:
            out_internalformat = GL_RG16;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case data::DXGI_FORMAT_R16G16_UINT:
            out_internalformat = GL_RG16UI;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case data::DXGI_FORMAT_R16G16_SNORM:
            out_internalformat = GL_RG16_SNORM;
            out_format         = GL_RG;
            out_datatype       = GL_SHORT;
            return true;
        case data::DXGI_FORMAT_R16G16_SINT:
            out_internalformat = GL_RG16I;
            out_format         = GL_RG;
            out_datatype       = GL_SHORT;
            return true;
        case data::DXGI_FORMAT_D32_FLOAT:
            out_internalformat = GL_DEPTH_COMPONENT;
            out_format         = GL_DEPTH_COMPONENT;
            out_datatype       = GL_FLOAT;
            return true;
        case data::DXGI_FORMAT_R32_FLOAT:
            out_internalformat = GL_R32F;
            out_format         = GL_RED;
            out_datatype       = GL_FLOAT;
            return true;
        case data::DXGI_FORMAT_R32_UINT:
            out_internalformat = GL_R32UI;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case data::DXGI_FORMAT_R32_SINT:
            out_internalformat = GL_R32I;
            out_format         = GL_RED;
            out_datatype       = GL_INT;
            return true;
        case data::DXGI_FORMAT_D24_UNORM_S8_UINT:
            out_internalformat = GL_DEPTH_STENCIL;
            out_format         = GL_DEPTH_STENCIL;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case data::DXGI_FORMAT_R8G8_UNORM:
            out_internalformat = GL_RG8;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_R8G8_UINT:
            out_internalformat = GL_RG8UI;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_R8G8_SNORM:
            out_internalformat = GL_RG8_SNORM;
            out_format         = GL_RG;
            out_datatype       = GL_BYTE;
            return true;
        case data::DXGI_FORMAT_R8G8_SINT:
            out_internalformat = GL_RG8I;
            out_format         = GL_RG;
            out_datatype       = GL_BYTE;
            return true;
        case data::DXGI_FORMAT_R16_FLOAT:
            out_internalformat = GL_R16F;
            out_format         = GL_RED;
            out_datatype       = GL_HALF_FLOAT;
            return true;
        case data::DXGI_FORMAT_D16_UNORM:
            out_internalformat = GL_DEPTH_COMPONENT;
            out_format         = GL_DEPTH_COMPONENT;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case data::DXGI_FORMAT_R16_UNORM:
            out_internalformat = GL_R16;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case data::DXGI_FORMAT_R16_UINT:
            out_internalformat = GL_R16UI;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case data::DXGI_FORMAT_R16_SNORM:
            out_internalformat = GL_R16_SNORM;
            out_format         = GL_RED;
            out_datatype       = GL_SHORT;
            return true;
        case data::DXGI_FORMAT_R16_SINT:
            out_internalformat = GL_R16I;
            out_format         = GL_RED;
            out_datatype       = GL_SHORT;
            return true;
        case data::DXGI_FORMAT_R8_UNORM:
            out_internalformat = GL_R8;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_R8_UINT:
            out_internalformat = GL_R8UI;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_R8_SNORM:
            out_internalformat = GL_R8_SNORM;
            out_format         = GL_RED;
            out_datatype       = GL_BYTE;
            return true;
        case data::DXGI_FORMAT_R8_SINT:
            out_internalformat = GL_R8I;
            out_format         = GL_RED;
            out_datatype       = GL_BYTE;
            return true;
        case data::DXGI_FORMAT_A8_UNORM:
            out_internalformat = GL_R8;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            out_internalformat = GL_RGB9_E5;
            out_format         = GL_RGB;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case data::DXGI_FORMAT_BC1_UNORM:
            out_internalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_BC1_UNORM_SRGB:
            out_internalformat = 0x8C4D;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_BC3_UNORM:
            out_internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_BC3_UNORM_SRGB:
            out_internalformat = 0x8C4E;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_BC5_UNORM:
            out_internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_B5G6R5_UNORM:
            out_internalformat = GL_RGB;
            out_format         = GL_BGR;
            out_datatype       = GL_UNSIGNED_SHORT_5_6_5_REV;
            return true;
        case data::DXGI_FORMAT_B5G5R5A1_UNORM:
            out_internalformat = GL_RGBA;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            return true;
        case data::DXGI_FORMAT_B8G8R8A8_UNORM:
            out_internalformat = GL_RGBA8;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case data::DXGI_FORMAT_B8G8R8X8_UNORM:
            out_internalformat = GL_RGBA8;
            out_format         = GL_BGR;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case data::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            out_internalformat = GL_RGB10_A2;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_2_10_10_10_REV;
            return true;
        case data::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            out_internalformat = GL_SRGB8_ALPHA8;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case data::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            out_internalformat = GL_SRGB8_ALPHA8;
            out_format         = GL_BGR;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case data::DXGI_FORMAT_BC6H_UF16:
            out_internalformat = 0x8E8F;
            out_format         = GL_RGB;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_BC6H_SF16:
            out_internalformat = 0x8E8E;
            out_format         = GL_RGB;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_BC7_UNORM:
            out_internalformat = 0x8E8C;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_BC7_UNORM_SRGB:
            out_internalformat = 0x8E8D;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_P8:
            out_internalformat = GL_R8;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_A8P8:
            out_internalformat = GL_RG8;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case data::DXGI_FORMAT_B4G4R4A4_UNORM:
            out_internalformat = GL_RGBA4;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            return true;
        default:
            break;
    }
    return false;
}

bool r2d::create_packer(r2d::packer_t *packer, size_t width, size_t height, size_t capacity)
{
    if (packer != NULL)
    {
        if (capacity == 0) capacity = 1; // need at least one node for the root.
        packer->Width    = width;
        packer->Height   = height;
        packer->Free     = width * height;
        packer->Used     = 0;
        packer->Capacity = capacity;
        packer->Count    = 0;
        packer->Nodes    = NULL;
        packer->Rects    = NULL;

        r2d::pknode_t    root;
        packer->Nodes    = (r2d::pknode_t*) malloc(capacity * sizeof(r2d::pknode_t) * 3);
        packer->Rects    = (r2d::pkrect_t*) malloc(capacity * sizeof(r2d::pkrect_t));
        root.Flags       = r2d::PACKER_FLAGS_NONE;
        root.Index       = 0xFFFFFFFFU;
        root.Child[0]    = 0;
        root.Child[1]    = 0;
        root.Bound[0]    = 0;
        root.Bound[1]    = 0;
        root.Bound[2]    = uint32_t(width);
        root.Bound[3]    = uint32_t(height);
        packer->Nodes[0] = root;
        return true;
    }
    else return false;
}

void r2d::delete_packer(r2d::packer_t *packer)
{
    if (packer != NULL)
    {
        if (packer->Rects != NULL) free(packer->Rects);
        if (packer->Nodes != NULL) free(packer->Nodes);
        packer->Width      = 0;
        packer->Height     = 0;
        packer->Free       = 0;
        packer->Used       = 0;
        packer->Capacity   = 0;
        packer->Count      = 0;
        packer->Nodes      = NULL;
        packer->Rects      = NULL;
    }
}

void r2d::reset_packer(r2d::packer_t *packer)
{
    if (packer->Capacity > 0)
    {
        r2d::pknode_t root;
        root.Flags    = r2d::PACKER_FLAGS_NONE;
        root.Index    = 0xFFFFFFFFU;
        root.Child[0] = 0;
        root.Child[1] = 0;
        root.Bound[0] = 0;
        root.Bound[1] = 0;
        root.Bound[2] = uint32_t(packer->Width);
        root.Bound[3] = uint32_t(packer->Height);
        packer->Count = 0;
        packer->Nodes[0] = root;
    }
    packer->Free  = packer->Width * packer->Height;
    packer->Used  = 0;
}

bool r2d::packer_insert(r2d::packer_t *packer, size_t width, size_t height, size_t hpad, size_t vpad, uint32_t id, r2d::pkrect_t *rect)
{
    size_t w = width  + (hpad * 2);
    size_t h = height + (vpad * 2);
    size_t a = w * h;

    // if there isn't enough space available, don't
    // bother searching for a place to put this rectangle.
    if (a > packer->Free)
        return false;

    r2d::pknode_t *n  = node_insert(packer, 0, w, h);
    if (n != NULL)
    {
        r2d::pkrect_t r;
        r.X        = n->Bound[0] + hpad;
        r.Y        = n->Bound[1] + vpad;
        r.Width    = width;
        r.Height   = height;
        r.Content  = id;
        r.Flags    = n->Flags;
        if (rect) *rect = r;

        n->Flags  |= r2d::PACKER_FLAGS_USED;
        n->Index   = uint32_t(packer->Count);
        packer->Rects[packer->Count++] = r;
        packer->Free -= a;
        packer->Used += a;
        return true;
    }
    else return false;
}

bool r2d::create_atlas_entry(r2d::atlas_entry_t *ent, uint32_t name, size_t frame_count)
{
    if (ent != NULL)
    {
        if (frame_count == 0) frame_count = 1;
        ent->Name        = name;
        ent->Flags       = r2d::ATLAS_ENTRY_NORMAL;
        ent->FrameCount  = frame_count;
        ent->MaxWidth    = 0;
        ent->MaxHeight   = 0;
        ent->Page0       = 0;
        ent->Frame0      = {
            0, 0, 0, 0
        };
        if (frame_count == 1)
        {   // this is the typical case - only one frame.
            // don't allocate additional storage, use the pre-allocated stuff.
            ent->PageIds = &ent->Page0;
            ent->Frames  = &ent->Frame0;
        }
        else
        {   // multi-frame entries allocate separate array storage.
            ent->Flags  |= r2d::ATLAS_ENTRY_MULTIFRAME;
            ent->PageIds = (size_t*)            malloc(frame_count * sizeof(size_t));
            ent->Frames  = (r2d::atlas_frame_t*)malloc(frame_count * sizeof(r2d::atlas_frame_t));
        }
        return true;
    }
    else return false;
}

void r2d::delete_atlas_entry(r2d::atlas_entry_t *ent)
{
    if (ent != NULL)
    {
        if (ent->Flags & r2d::ATLAS_ENTRY_MULTIFRAME)
        {
            if (ent->Frames  != NULL) free(ent->Frames);
            if (ent->PageIds != NULL) free(ent->PageIds);
        }
        ent->FrameCount = 0;
        ent->PageIds    = NULL;
        ent->Frames     = NULL;
    }
}

void r2d::set_atlas_entry_frame(r2d::atlas_entry_t *ent, size_t frame_index, size_t page_id, r2d::atlas_frame_t const &frame)
{
    assert(frame_index < ent->FrameCount);

    if (frame.Width  > ent->MaxWidth)
        ent->MaxWidth  = frame.Width;

    if (frame.Height > ent->MaxHeight)
        ent->MaxHeight = frame.Height;

    if (frame_index > 0 && ent->PageIds[frame_index] != page_id)
        ent->Flags    |= r2d::ATLAS_ENTRY_MULTIPAGE;

    ent->PageIds[frame_index] = page_id;
    ent->Frames [frame_index] = frame;
}

bool r2d::create_image_cache(r2d::image_cache_t *cache, r2d::image_cache_config_t const &config)
{
    if (cache != NULL)
    {
        GLint   nalign         = 4;
        GLsizei nbytes         = 0;
        cache->PageWidth       = config.PageWidth;
        cache->PageHeight      = config.PageHeight;
        cache->HorizontalPad   = config.HorizontalPad;
        cache->VerticalPad     = config.VerticalPad;
        cache->EntryCapacity   = 0;
        cache->EntryCount      = 0;
        cache->EntryList       = NULL;
        cache->PageCapacity    = 0;
        cache->PageCount       = 0;
        cache->TexturePages    = NULL;
        cache->BucketCount     = config.ExpectedCount / IMAGE_CACHE_NAMES_PER_BUCKET;
        cache->EntryNames      = NULL;
        cache->PageLayout      = config.Layout;
        cache->PageFormat      = config.Format;
        cache->PageDataType    = config.DataType;
        cache->TransferBuffer  = 0;
        cache->TransferBytes   = 0;
        cache->BufferOffset    = 0;
        if (cache->BucketCount < IMAGE_CACHE_MIN_BUCKET_COUNT)
            cache->BucketCount = IMAGE_CACHE_MIN_BUCKET_COUNT;

        // pre-allocate storage for the entry lookup-by-name table.
        cache->EntryNames      = (uint32_t**) malloc(cache->BucketCount * sizeof(uint32_t*));
        if (cache->EntryNames == NULL) goto error_cleanup;
        for (size_t i = 0; i < cache->BucketCount; ++i)
        {
            cache->EntryNames[i]     = (uint32_t*) malloc((IMAGE_CACHE_NAMES_PER_BUCKET + 2) * sizeof(uint32_t));
            if (cache->EntryNames[i] == NULL)
                goto error_cleanup;
            cache->EntryNames[i][0]  = IMAGE_CACHE_NAMES_PER_BUCKET; // capacity
            cache->EntryNames[i][1]  = 0;                            // count
        }

        // pre-allocate storage for the entry list.
        if (config.ExpectedCount > 0)
        {
            cache->EntryCapacity = config.ExpectedCount;
            cache->EntryCount    = 0;
            cache->EntryList     = (r2d::atlas_entry_t*) malloc(config.ExpectedCount * sizeof(r2d::atlas_entry_t));
            if (cache->EntryList == NULL) goto error_cleanup;
        }

        // pre-allocate storage for page texture IDs.
        cache->TexturePages = (GLuint*) malloc(IMAGE_CACHE_PAGE_CAPACITY * sizeof(GLuint));
        if (cache->TexturePages == NULL) goto error_cleanup;
        cache->PageCapacity = IMAGE_CACHE_PAGE_CAPACITY;
        cache->PageCount    = 0;

        // generate the PBO used to stream data to the GPU.
        glGenBuffers(1, &cache->TransferBuffer);
        if (cache->TransferBuffer == 0) goto error_cleanup;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, cache->TransferBuffer);
        glGetIntegerv(GL_UNPACK_ALIGNMENT , &nalign);
        nbytes = gl::bytes_per_slice(config.Format, config.DataType, config.PageWidth, config.PageHeight, size_t(nalign));
        glBufferData(GL_PIXEL_UNPACK_BUFFER, nbytes, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        cache->TransferBytes = size_t(nbytes);
        cache->BufferOffset  = 0;
        return true;

error_cleanup:
        if (cache->TransferBuffer != 0)
        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            glDeleteBuffers(1, &cache->TransferBuffer);
        }
        if (cache->TexturePages != NULL)
        {
            free(cache->TexturePages);
        }
        if (cache->EntryNames != NULL)
        {
            for (size_t i = 0; i < cache->BucketCount; ++i)
            {
                if (cache->EntryNames[i] != NULL)
                {
                    free(cache->EntryNames[i]);
                    cache->EntryNames[i]  = NULL;
                }
            }
            free(cache->EntryNames);
        }
        if (cache->EntryList != NULL)
        {
            free(cache->EntryList);
        }
        cache->EntryCapacity  = 0;
        cache->EntryCount     = 0;
        cache->EntryList      = NULL;
        cache->PageCapacity   = 0;
        cache->PageCount      = 0;
        cache->TexturePages   = NULL;
        cache->BucketCount    = 0;
        cache->EntryNames     = NULL;
        cache->TransferBuffer = 0;
        cache->TransferBytes  = 0;
        cache->BufferOffset   = 0;
        return false;
    }
    else return false;
}

void r2d::delete_image_cache(r2d::image_cache_t *cache)
{
    if (cache != NULL)
    {
        if (cache->TransferBuffer != 0)
        {   // if the buffer is in use, it will be 
            // deleted when the GPU is finished with it.
            glDeleteBuffers(1, &cache->TransferBuffer);
        }
        if (cache->TexturePages != NULL)
        {
            if (cache->PageCount > 0)
            {   // any textures in use will be deleted when the GPU is done with them.
                glDeleteTextures(GLsizei(cache->PageCount), cache->TexturePages);
            }
            free(cache->TexturePages);
        }
        if (cache->EntryNames != NULL)
        {
            for (size_t i = 0; i < cache->BucketCount; ++i)
            {
                if (cache->EntryNames[i] != NULL)
                {
                    free(cache->EntryNames[i]);
                    cache->EntryNames[i]  = NULL;
                }
            }
            free(cache->EntryNames);
        }
        if (cache->EntryList != NULL)
        {
            free(cache->EntryList);
        }
        cache->EntryCapacity  = 0;
        cache->EntryCount     = 0;
        cache->EntryList      = NULL;
        cache->PageCapacity   = 0;
        cache->PageCount      = 0;
        cache->TexturePages   = NULL;
        cache->BucketCount    = 0;
        cache->EntryNames     = NULL;
        cache->TransferBuffer = 0;
        cache->TransferBytes  = 0;
        cache->BufferOffset   = 0;
    }
}

void r2d::freeze_image_cache(r2d::image_cache_t *cache)
{
    if (cache->TransferBuffer != 0)
    {
        glDeleteBuffers(1, &cache->TransferBuffer);
        cache->TransferBuffer  = 0;
        cache->TransferBytes   = 0;
        cache->BufferOffset    = 0;
    }
}

// use glMapBufferRange at cache->BufferOffset, for gl::bytes_per_slice(image_to_copy),
// memcpy the entire source data to the PBO, glUnmapBuffer, and then glTexSubImage2D 
// (via the gl::pixel_transfer_h2d() function) to queue the async upload. if not 
// enough buffer space, map with discard.

