/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements a basic 2D renderer for sprites and GUI controls.
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
static size_t const ATLAS_NAMES_PER_BUCKET = 16;

/// @summary The default capacity of an image cache, in images.
static size_t const ATLAS_DEFAULT_CAPACITY = 1024;

/// @summary The zero-based index of the first name within a bucket.
static size_t const ATLAS_FIRST_NAME       = 2;

/// @summary Define the minimum number of name buckets allocated per-image cache.
static size_t const ATLAS_MIN_BUCKET_COUNT = ATLAS_DEFAULT_CAPACITY / ATLAS_NAMES_PER_BUCKET;

/// @summary Define the default capacity of the TexturePages texture ID storage.
static size_t const ATLAS_PAGE_CAPACITY    = 4;

/*///////////////
//   Globals   //
///////////////*/

/*//////////////////
//   Data Types   //
//////////////////*/
/// @summary Defines the data required to represent a priority queue of rectangles ordered by
/// some criteria, such as minimum or maximum extent, minimum or maximum area, and so on.
struct rectq_t
{
    size_t  Count;     /// The number of items currently in the queue.
    size_t *Index;     /// An array of rectangle index values.
    size_t *Width;     /// An array of rectangle width values.
    size_t *Height;    /// An array of rectangle height values.
};

/// @summary A functor type used for ordering rectangles into ascending order by width.
struct rectq_minw_t
{
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param a The zero-based index of the first rectangle.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the width of rectangle a is smaller than that of rectangle b.
    ///         +1 if the width of rectangle a is larger than that of rectangle b.
    ///          0 if the rectangles have the same width.
    inline int operator()(rectq_t const *pq, size_t const a, size_t const b) const
    {
        size_t const * V = pq->Width;
        return (V[a] < V[b]) ? -1 : ((V[a] > V[b]) ? +1 : 0);
    }
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param w The width of the rectangle being inserted.
    /// @param h The height of the rectangle being inserted.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the width of rectangle a is smaller than that of rectangle b.
    ///         +1 if the width of rectangle a is larger than that of rectangle b.
    ///          0 if the rectangles have the same width.
    inline int operator()(rectq_t const *pq, size_t const w, size_t const h, size_t const b) const
    {
        size_t const * V = pq->Width;
        return (w < V[b]) ? -1 : ((w > V[b]) ? +1 : 0);
    }
};

/// @summary A functor type used for ordering rectangles into descending order by width.
struct rectq_maxw_t
{
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param a The zero-based index of the first rectangle.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the width of rectangle a is larger than that of rectangle b.
    ///         +1 if the width of rectangle a is smaller than that of rectangle b.
    ///          0 if the rectangles have the same width.
    inline int operator()(rectq_t const *pq, size_t const a, size_t const b) const
    {
        size_t const * V = pq->Width;
        return (V[a] < V[b]) ? +1 : ((V[a] > V[b]) ? -1 : 0);
    }
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param w The width of the rectangle being inserted.
    /// @param h The height of the rectangle being inserted.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the width of rectangle a is larger than that of rectangle b.
    ///         +1 if the width of rectangle a is smaller than that of rectangle b.
    ///          0 if the rectangles have the same width.
    inline int operator()(rectq_t const *pq, size_t const w, size_t const h, size_t const b) const
    {
        size_t const * V = pq->Width;
        return (w < V[b]) ? +1 : ((w > V[b]) ? -1 : 0);
    }
};

/// @summary A functor type used for ordering rectangles into ascending order by height.
struct rectq_minh_t
{
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param a The zero-based index of the first rectangle.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the height of rectangle a is smaller than that of rectangle b.
    ///         +1 if the height of rectangle a is larger than that of rectangle b.
    ///          0 if the rectangles have the same height.
    inline int operator()(rectq_t const *pq, size_t const a, size_t const b) const
    {
        size_t const * V = pq->Height;
        return (V[a] < V[b]) ? -1 : ((V[a] > V[b]) ? +1 : 0);
    }
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param w The width of the rectangle being inserted.
    /// @param h The height of the rectangle being inserted.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the height of rectangle a is smaller than that of rectangle b.
    ///         +1 if the height of rectangle a is larger than that of rectangle b.
    ///          0 if the rectangles have the same height.
    inline int operator()(rectq_t const *pq, size_t const w, size_t const h, size_t const b) const
    {
        size_t const * V = pq->Height;
        return (h < V[b]) ? -1 : ((h > V[b]) ? +1 : 0);
    }
};

/// @summary A functor type used for ordering rectangles into descending order by height.
struct rectq_maxh_t
{
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param a The zero-based index of the first rectangle.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the height of rectangle a is larger than that of rectangle b.
    ///         +1 if the height of rectangle a is smaller than that of rectangle b.
    ///          0 if the rectangles have the same width.
    inline int operator()(rectq_t const *pq, size_t const a, size_t const b) const
    {
        size_t const * V = pq->Height;
        return (V[a] < V[b]) ? +1 : ((V[a] > V[b]) ? -1 : 0);
    }
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param w The width of the rectangle being inserted.
    /// @param h The height of the rectangle being inserted.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the height of rectangle a is larger than that of rectangle b.
    ///         +1 if the height of rectangle a is smaller than that of rectangle b.
    ///          0 if the rectangles have the same width.
    inline int operator()(rectq_t const *pq, size_t const w, size_t const h, size_t const b) const
    {
        size_t const * V = pq->Height;
        return (h < V[b]) ? +1 : ((h > V[b]) ? -1 : 0);
    }
};

/// @summary A functor type used for ordering rectangles into ascending order by area.
struct rectq_mina_t
{
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param a The zero-based index of the first rectangle.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the area of rectangle a is smaller than that of rectangle b.
    ///         +1 if the area of rectangle a is larger than that of rectangle b.
    ///          0 if the rectangles have the same area.
    inline int operator()(rectq_t const *pq, size_t const a, size_t const b) const
    {
        size_t const A = pq->Width[a]  * pq->Height[a];
        size_t const B = pq->Width[b]  * pq->Height[b];
        return (A < B) ? -1 : ((A > B) ? +1 : 0);
    }
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param a The zero-based index of the first rectangle.
    /// @param b The zero-based index of the second rectangle.
    /// @param w The width of the rectangle being inserted.
    /// @param h The height of the rectangle being inserted.
    /// @return -1 if the area of rectangle a is smaller than that of rectangle b.
    ///         +1 if the area of rectangle a is larger than that of rectangle b.
    ///          0 if the rectangles have the same area.
    inline int operator()(rectq_t const *pq, size_t const w, size_t const h, size_t const b) const
    {
        size_t const A = w * h;
        size_t const B = pq->Width[b]  * pq->Height[b];
        return (A < B) ? -1 : ((A > B) ? +1 : 0);
    }
};

/// @summary A functor type used for ordering rectangles into descending order by area.
struct rectq_maxa_t
{
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param a The zero-based index of the first rectangle.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the area of rectangle a is larger than that of rectangle b.
    ///         +1 if the area of rectangle a is smaller than that of rectangle b.
    ///          0 if the rectangles have the same width.
    inline int operator()(rectq_t const *pq, size_t const a, size_t const b) const
    {
        size_t const A = pq->Width[a]  * pq->Height[a];
        size_t const B = pq->Width[b]  * pq->Height[b];
        return (A < B) ? +1 : ((A > B) ? -1 : 0);
    }
    /// @summary The function operator. Compares two rectangle values.
    /// @param pq The rectangle priority queue being heapified.
    /// @param w The width of the rectangle being inserted.
    /// @param h The height of the rectangle being inserted.
    /// @param b The zero-based index of the second rectangle.
    /// @return -1 if the area of rectangle a is larger than that of rectangle b.
    ///         +1 if the area of rectangle a is smaller than that of rectangle b.
    ///          0 if the rectangles have the same width.
    inline int operator()(rectq_t const *pq, size_t const w, size_t const h, size_t const b) const
    {
        size_t const A = w * h;
        size_t const B = pq->Width[b]  * pq->Height[b];
        return (A < B) ? +1 : ((A > B) ? -1 : 0);
    }
};

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Mixes the bits of a 32-bit unsigned integer.
/// @param value The input value.
/// @return The output value.
static inline uint32_t hash_u32(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x85EBCA6B;
    value ^= value >> 13;
    value *= 0xC2B2AE35;
    value ^= value >> 16;
    return value;
}

/// @summary Resets a rectangle priority queue to empty without allocating or freeing storage.
/// @param pq The rectangle priority queue to clear.
static inline void rectq_clear(rectq_t *pq)
{
    pq->Count = 0;
}

/// @summary Allocates storage for a rectangle priority queue and initializes the queue to empty.
/// @param pq The priority queue structure to initialize.
/// @param capacity The maximum number of items to be inserted into the queue.
/// @return true if the queue was initialized successfully.
static bool create_rectq(rectq_t *pq, size_t capacity)
{
    if (pq != NULL)
    {
        pq->Count  = 0;
        pq->Index  = NULL;
        pq->Width  = NULL;
        pq->Height = NULL;
        if (capacity > 0)
        {   // allocate storage as one large contiguous block.
            size_t n = capacity * 3;
            void  *p = malloc(n * sizeof(size_t));
            if (p != NULL)
            {
                pq->Index  = ((size_t*) p + (0));
                pq->Width  = ((size_t*) p + (capacity));
                pq->Height = ((size_t*) p + (capacity * 2));
                return true;
            }
            else return false;
        }
        else return true;
    }
    else return false;
}

/// @summary Frees storage associated with a rectangle priority queue.
/// @param pq The rectangle priority queue to delete.
static void delete_rectq(rectq_t *pq)
{
    if (pq != NULL)
    {
        pq->Count = 0;
        if (pq->Index != NULL)
        {   // all storage was allocated as a single contiguous block.
            // the base pointer is stored in pq->Index.
            free(pq->Index);
        }
        pq->Index  = NULL;
        pq->Width  = NULL;
        pq->Height = NULL;
    }
}

/// @summary Inserts an item into a priority queue.
/// @param pq The priority queue to update.
/// @param w The horizontal extent of the rectangle.
/// @param h The vertical extent of the rectangle.
/// @param i The unique identifier of the rectangle.
/// @param cmp Provides int operator()(rectq_t *pq, size_t const w, size_t const h, size_t const b) const
/// and returns -1:0:+1 to order items within the queue.
template <typename Comp>
static inline void rectq_insert(rectq_t *pq, size_t w, size_t h, size_t i, Comp const &cmp)
{
    size_t pos = pq->Count++;
    size_t idx =(pos - 1) / 2;
    while (pos > 0 && cmp(pq, w, h, idx) < 0)
    {
        pq->Index [pos] = pq->Index [idx];
        pq->Width [pos] = pq->Width [idx];
        pq->Height[pos] = pq->Height[idx];
        pos = idx;
        idx =(idx - 1) / 2;
    }
    pq->Index [pos] = i;
    pq->Width [pos] = w;
    pq->Height[pos] = h;
}

/// @summary Removes the highest priority item from the queue.
/// @param pq The priority queue to update.
/// @param w On return, stores the horizontal extent of the removed rectangle.
/// @param h On return, stores the vertical extent of the removed rectangle.
/// @param i On return, stores the unique identifier of the removed rectangle.
/// @param cmp Provides int operator()(rectq_t *pq, size_t const a, size_t const b) const
/// and returns -1:0:+1 to order items within the queue.
template <typename Comp>
static inline void rectq_remove(rectq_t *pq, size_t &w, size_t &h, size_t &i, Comp const &cmp)
{   // the highest-priority item is located at index 0.
    i = pq->Index [0];
    w = pq->Width [0];
    h = pq->Height[0];

    // swap the last item into the position vacated by the first item.
    size_t n      = pq->Count - 1;
    pq->Index [0] = pq->Index [n];
    pq->Width [0] = pq->Width [n];
    pq->Height[0] = pq->Height[n];
    pq->Count     = n;

    // now restore the heap ordering constraint.
    size_t pos = 0;
    for ( ; ; )
    {
        size_t l = (2 * pos) + 1; // left child.
        size_t r = (2 * pos) + 1; // right child.
        size_t m;                 // child with highest priority.

        // determine the child node with the highest priority.
        if  (l >= n) break;       // node at pos has no children.
        if  (r >= n) m = l;       // node at pos has no right child.
        else m  = (cmp(pq, l, r) < 0) ? l : r;

        // now compare the node at pos with the highest priority child, m.
        if (cmp(pq, pos, m) < 0)
        {   // children have lower priority than parent; order restored.
            break;
        }

        // swap the parent with the largest child.
        size_t temp_i   = pq->Index [pos];
        size_t temp_w   = pq->Width [pos];
        size_t temp_h   = pq->Height[pos];
        pq->Index [pos] = pq->Index [m];
        pq->Width [pos] = pq->Width [m];
        pq->Height[pos] = pq->Height[m];
        pq->Index [m]   = temp_i;
        pq->Width [m]   = temp_w;
        pq->Height[m]   = temp_h;
        pos = m;
    }
}

/// @summary Calculates the total area used by a set of rectangles.
/// @param n The number of rectangles and dimension of the width and height arrays.
/// @param w An array of n rectangle width values.
/// @param h An array of n rectangle height values.
/// @param hpad The padding along the horizontal edges of each rectangle.
/// @param vpad The padding along the vertical edges of each rectangle.
/// @return The total area required for all rectangles.
static size_t total_area(size_t n, size_t const *w, size_t const *h, size_t hpad, size_t vpad)
{
    size_t area   = 0;
    for (size_t i = 0; i < n; ++i)
    {
        size_t const width  = w[i] + (2 * hpad);
        size_t const height = h[i] + (2 * vpad);
        area += (width * height);
    }
    return area;
}

/// @summary Determine the largest value in an array of values.
/// @param n The number of values in the array to search.
/// @param v The array of values to search.
/// @return The largest value in the search array.
static size_t max_value(size_t n, size_t const *v)
{
    size_t x =  0;
    for (size_t i = 0; i < n; ++i)
    {
        if (x < v[i])
            x = v[i];
    }
    return x;
}

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

bool r2d::create_atlas(r2d::atlas_t *atlas, r2d::atlas_config_t const &config)
{
    if (atlas != NULL)
    {
        GLint   nalign         = 4;
        GLsizei nbytes         = 0;
        atlas->PageWidth       = config.PageWidth;
        atlas->PageHeight      = config.PageHeight;
        atlas->HorizontalPad   = config.HorizontalPad;
        atlas->VerticalPad     = config.VerticalPad;
        atlas->EntryCapacity   = 0;
        atlas->EntryCount      = 0;
        atlas->EntryList       = NULL;
        atlas->PageCapacity    = 0;
        atlas->PageCount       = 0;
        atlas->TexturePages    = NULL;
        atlas->BucketCount     = config.ExpectedCount / ATLAS_NAMES_PER_BUCKET;
        atlas->BucketList      = NULL;
        atlas->EntryNames      = NULL;
        atlas->EntryIndices    = NULL;
        atlas->PageLayout      = config.Layout;
        atlas->PageFormat      = config.Format;
        atlas->PageDataType    = config.DataType;
        atlas->TransferBuffer  = 0;
        atlas->TransferBytes   = 0;
        atlas->BufferOffset    = 0;
        if (atlas->BucketCount < ATLAS_MIN_BUCKET_COUNT)
            atlas->BucketCount = ATLAS_MIN_BUCKET_COUNT;

        // pre-allocate storage for the entry lookup-by-name table.
        atlas->BucketList        = (r2d::bucket_t*) malloc(atlas->BucketCount * sizeof(r2d::bucket_t));
        atlas->EntryNames        = (uint32_t    **) malloc(atlas->BucketCount * sizeof(uint32_t*));
        atlas->EntryIndices      = (uint32_t    **) malloc(atlas->BucketCount * sizeof(uint32_t*));
        if (atlas->BucketList   == NULL) goto error_cleanup;
        if (atlas->EntryNames   == NULL) goto error_cleanup;
        if (atlas->EntryIndices == NULL) goto error_cleanup;
        for (size_t i = 0; i < atlas->BucketCount; ++i)
        {
            atlas->BucketList[i].Capacity = ATLAS_NAMES_PER_BUCKET;
            atlas->BucketList[i].Count    = 0;
            atlas->EntryNames[i]          = (uint32_t*) malloc(ATLAS_NAMES_PER_BUCKET * sizeof(uint32_t));
            atlas->EntryIndices[i]        = (uint32_t*) malloc(ATLAS_NAMES_PER_BUCKET * sizeof(uint32_t));
            if (atlas->EntryNames[i]   == NULL) goto error_cleanup;
            if (atlas->EntryIndices[i] == NULL) goto error_cleanup;
        }

        // pre-allocate storage for the entry list.
        if (config.ExpectedCount > 0)
        {
            atlas->EntryCapacity = config.ExpectedCount;
            atlas->EntryCount    = 0;
            atlas->EntryList     = (r2d::atlas_entry_t*) malloc(config.ExpectedCount * sizeof(r2d::atlas_entry_t));
            if (atlas->EntryList == NULL) goto error_cleanup;
        }

        // pre-allocate storage for texture page packers and page texture IDs.
        atlas->PagePackers  = (r2d::packer_t*) malloc(ATLAS_PAGE_CAPACITY * sizeof(r2d::packer_t));
        if (atlas->PagePackers == NULL)  goto error_cleanup;
        atlas->TexturePages = (GLuint*)  malloc(ATLAS_PAGE_CAPACITY * sizeof(GLuint));
        if (atlas->TexturePages == NULL) goto error_cleanup;
        atlas->PageCapacity = ATLAS_PAGE_CAPACITY;
        atlas->PageCount    = 0;

        // generate the PBO used to stream data to the GPU.
        glGenBuffers(1, &atlas->TransferBuffer);
        if (atlas->TransferBuffer == 0) goto error_cleanup;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, atlas->TransferBuffer);
        glGetIntegerv(GL_UNPACK_ALIGNMENT , &nalign);
        nbytes = gl::bytes_per_slice(config.Format, config.DataType, config.PageWidth, config.PageHeight, size_t(nalign));
        glBufferData(GL_PIXEL_UNPACK_BUFFER, nbytes, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        atlas->TransferBytes = size_t(nbytes);
        atlas->BufferOffset  = 0;
        return true;

error_cleanup:
        if (atlas->TransferBuffer != 0)
        {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            glDeleteBuffers(1, &atlas->TransferBuffer);
        }
        if (atlas->TexturePages != NULL)  free(atlas->TexturePages);
        if (atlas->PagePackers != NULL)   free(atlas->PagePackers);
        if (atlas->EntryList != NULL)     free(atlas->EntryList);
        if (atlas->EntryIndices != NULL)
        {
            for (size_t i = 0; i < atlas->BucketCount; ++i)
            {
                if (atlas->EntryIndices[i] != NULL)
                {
                    free(atlas->EntryIndices[i]);
                    atlas->EntryIndices[i]  = NULL;
                }
            }
            free(atlas->EntryIndices);
        }
        if (atlas->EntryNames != NULL)
        {
            for (size_t i = 0; i < atlas->BucketCount; ++i)
            {
                if (atlas->EntryNames[i] != NULL)
                {
                    free(atlas->EntryNames[i]);
                    atlas->EntryNames[i]  = NULL;
                }
            }
            free(atlas->EntryNames);
        }
        if (atlas->BucketList != NULL)
        {
            free(atlas->BucketList);
        }
        atlas->EntryCapacity  = 0;
        atlas->EntryCount     = 0;
        atlas->EntryList      = NULL;
        atlas->PageCapacity   = 0;
        atlas->PageCount      = 0;
        atlas->PagePackers    = NULL;
        atlas->TexturePages   = NULL;
        atlas->BucketCount    = 0;
        atlas->BucketList     = NULL;
        atlas->EntryNames     = NULL;
        atlas->EntryIndices   = NULL;
        atlas->TransferBuffer = 0;
        atlas->TransferBytes  = 0;
        atlas->BufferOffset   = 0;
        return false;
    }
    else return false;
}

void r2d::delete_atlas(r2d::atlas_t *atlas)
{
    if (atlas != NULL)
    {
        if (atlas->TransferBuffer != 0)
        {   // if the buffer is in use, it will be
            // deleted when the GPU is finished with it.
            glDeleteBuffers(1, &atlas->TransferBuffer);
        }
        if (atlas->TexturePages != NULL)
        {
            if (atlas->PageCount > 0)
            {   // any textures in use will be deleted when the GPU is done with them.
                glDeleteTextures(GLsizei(atlas->PageCount), atlas->TexturePages);
            }
            free(atlas->TexturePages);
        }
        if (atlas->PagePackers != NULL)
        {
            for (size_t i = 0; i < atlas->PageCount; ++i)
            {
                r2d::delete_packer(&atlas->PagePackers[i]);
            }
            free(atlas->PagePackers);
        }
        if (atlas->EntryList != NULL)
        {
            free(atlas->EntryList);
        }
        if (atlas->EntryIndices != NULL)
        {
            for (size_t i = 0; i < atlas->BucketCount; ++i)
            {
                if (atlas->EntryIndices[i] != NULL)
                {
                    free(atlas->EntryIndices[i]);
                    atlas->EntryIndices[i]  = NULL;
                }
            }
            free(atlas->EntryIndices);
        }
        if (atlas->EntryNames != NULL)
        {
            for (size_t i = 0; i < atlas->BucketCount; ++i)
            {
                if (atlas->EntryNames[i] != NULL)
                {
                    free(atlas->EntryNames[i]);
                    atlas->EntryNames[i]  = NULL;
                }
            }
            free(atlas->EntryNames);
        }
        if (atlas->BucketList != NULL)
        {
            free(atlas->BucketList);
        }
        atlas->EntryCapacity     = 0;
        atlas->EntryCount        = 0;
        atlas->EntryList         = NULL;
        atlas->PageCapacity      = 0;
        atlas->PageCount         = 0;
        atlas->PagePackers       = NULL;
        atlas->TexturePages      = NULL;
        atlas->BucketCount       = 0;
        atlas->BucketList        = NULL;
        atlas->EntryNames        = NULL;
        atlas->EntryIndices      = NULL;
        atlas->TransferBuffer    = 0;
        atlas->TransferBytes     = 0;
        atlas->BufferOffset      = 0;
    }
}

void r2d::freeze_atlas(r2d::atlas_t *atlas)
{
    if (atlas->TransferBuffer != 0)
    {
        glDeleteBuffers(1, &atlas->TransferBuffer);
        atlas->TransferBuffer  = 0;
        atlas->TransferBytes   = 0;
        atlas->BufferOffset    = 0;
    }
    if (atlas->PagePackers != NULL)
    {
        for (size_t i = 0; i < atlas->PageCount; ++i)
        {
            r2d::delete_packer(&atlas->PagePackers[i]);
        }
        free(atlas->PagePackers);
        atlas->PagePackers = NULL;
    }
}

r2d::atlas_entry_t* r2d::find_atlas_entry(r2d::atlas_t *atlas, uint32_t name)
{
    size_t   const  bucket_index = hash_u32(name)  % atlas->BucketCount;
    size_t   const  bucket_size  = atlas->BucketList[bucket_index].Count;
    uint32_t const *bucket_names = atlas->EntryNames[bucket_index];
    for (size_t  i = 0; i < bucket_size; ++i)
    {
        if (name == bucket_names[i])
        {
            uint32_t value_index = atlas->EntryIndices[bucket_index][i];
            return &atlas->EntryList[value_index];
        }
    }
    return NULL;
}

r2d::atlas_entry_t* r2d::get_atlas_entry(r2d::atlas_t *atlas, size_t index)
{
    return &atlas->EntryList[index];
}

r2d::atlas_entry_t* r2d::atlas_add(r2d::atlas_t *atlas, uint32_t name, size_t frame_count, size_t *out_index)
{
    if (atlas->EntryCount == atlas->EntryCapacity)
    {   // grow storage for the entry definitions.
        size_t newc = atlas->EntryCapacity > 4096 ? atlas->EntryCapacity + 512 : atlas->EntryCapacity * 2;
        void  *newl = realloc(atlas->EntryList, newc * sizeof(r2d::atlas_entry_t));
        if (newl != NULL)
        {
            atlas->EntryCapacity = newc;
            atlas->EntryList     = (r2d::atlas_entry_t*) newl;
        }
        else return NULL;
    }

    size_t const entry_id  = atlas->EntryCount;
    size_t const bucket_id = hash_u32(name) % atlas->BucketCount;
    size_t const bucket_n  = atlas->BucketList[bucket_id].Count;
    if (bucket_n == atlas->BucketList[bucket_id].Capacity)
    {   // grow storage for the entry name lookup table.
        size_t newc = bucket_n + 32;
        void  *newn = realloc(atlas->EntryNames  [bucket_id], newc * sizeof(uint32_t));
        void  *newi = realloc(atlas->EntryIndices[bucket_id], newc * sizeof(uint32_t));
        if (newn != NULL) atlas->EntryNames  [bucket_id]   = (uint32_t*) newn;
        if (newi != NULL) atlas->EntryIndices[bucket_id]   = (uint32_t*) newi;
        if (newn != NULL && newi != NULL) atlas->BucketList[bucket_id].Capacity = newc;
        else return NULL;
    }

    r2d::atlas_entry_t *e = &atlas->EntryList[entry_id];
    if (!r2d::create_atlas_entry(e, name , frame_count))
    {   // failed to allocate storage for frame data for the entry.
        return NULL;
    }
    if (out_index) *out_index = entry_id;

    size_t const  idx = atlas->BucketList[bucket_id].Count;
    atlas->EntryNames  [bucket_id][idx] = name;
    atlas->EntryIndices[bucket_id][idx] = uint32_t(entry_id);
    atlas->BucketList  [bucket_id].Count++;
    atlas->EntryCount++;
    return e;
}

bool r2d::atlas_place_frame(r2d::atlas_t *atlas, r2d::atlas_entry_t *entry, size_t frame, size_t w, size_t h)
{
    return r2d::atlas_place_frame(atlas, entry, frame, w, h, atlas->HorizontalPad, atlas->VerticalPad);
}

bool r2d::atlas_place_frame(r2d::atlas_t *atlas, r2d::atlas_entry_t *entry, size_t frame, size_t w, size_t h, size_t hpad, size_t vpad)
{
    size_t frame_width  = w + (hpad * 2);
    size_t frame_height = h + (vpad * 2);

    if (frame_width > atlas->PageWidth || frame_height > atlas->PageHeight)
    {   // no texture page can be large enough to hold this frame.
        return false;
    }
    if (atlas->PagePackers == NULL)
    {   // the atlas has been frozen, no data can be added.
        return false;
    }

    size_t const nump = atlas->PageCount;
    size_t page_index = nump - 1;
    while (page_index < nump)
    {
        r2d::pkrect_t   rect;
        r2d::packer_t  *pack = &atlas->PagePackers[page_index];
        if (r2d::packer_insert(pack, w, h, hpad, vpad, entry->Name, &rect))
        {   // the frame was successfully placed on this page.
            // note that rect is the unpadded content rectangle.
            r2d::atlas_frame_t  eframe = {
                rect.X,
                rect.Y,
                rect.Width,
                rect.Height
            };
            r2d::set_atlas_entry_frame(entry, frame, page_index, eframe);
            return true;
        }
        // decrement at page_index = 0 will cause wrap-around.
        // page_index in this case will then be > npages.
        page_index--;
    }

    // we were unable to find a page on which to place the frame, so allocate a new page.
    if (atlas->PageCount == atlas->PageCapacity)
    {   // allocate additional storage. we'll initialize as-needed.
        size_t newc = atlas->PageCapacity + ATLAS_PAGE_CAPACITY;
        void  *newp = realloc(atlas->PagePackers , newc * sizeof(r2d::packer_t));
        void  *newt = realloc(atlas->TexturePages, newc * sizeof(GLuint));
        if (newp != NULL) atlas->PagePackers  = (r2d::packer_t*) newp;
        if (newt != NULL) atlas->TexturePages = (GLuint*) newt;
        if (newp != NULL && newt != NULL) atlas->PageCapacity = newc;
    }

    size_t const page_width  = atlas->PageWidth;
    size_t const page_height = atlas->PageHeight;
    size_t const page_id     = atlas->PageCount;

    // create texture storage for the new texture page.
    GLuint tex  = 0;
    glGenTextures(1, &tex);
    if (tex == 0) return false;
    glBindTexture(GL_TEXTURE_2D, tex);
    gl::texture_storage(GL_TEXTURE_2D, atlas->PageFormat, atlas->PageDataType, GL_CLAMP_TO_EDGE , GL_CLAMP_TO_EDGE, page_width, page_height, 1, 1);

    // initialize a new rectangle packer for placing sub-images.
    if (!r2d::create_packer(&atlas->PagePackers[page_id], page_width, page_height, ATLAS_DEFAULT_CAPACITY))
    {   // unable to initialize the packer, so delete the texture page.
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &tex);
        return false;
    }

    r2d::pkrect_t  rect;
    r2d::packer_t *pack = &atlas->PagePackers[page_id];
    r2d::packer_insert(pack, w, h, hpad, vpad, entry->Name, &rect);
    r2d::atlas_frame_t eframe = { rect.X, rect.Y, rect.Width, rect.Height };
    r2d::set_atlas_entry_frame(entry, frame, page_id, eframe);
    atlas->TexturePages[page_id] = tex;
    atlas->PageCount++;
    return true;
}

bool r2d::atlas_transfer_frame(r2d::atlas_t *atlas, r2d::atlas_entry_t *entry, size_t frame, void const *pixels)
{
    r2d::atlas_frame_t const &bounds = entry->Frames[frame];
    size_t             const  w      = bounds.Width;
    size_t             const  h      = bounds.Height;
    size_t             const  align  = 4;
    GLenum             const  type   = atlas->PageDataType;
    GLenum             const  format = atlas->PageFormat;
    GLsizei            const  size   = gl::bytes_per_slice(format, type, w, h, align);
    GLintptr                  offset = atlas->BufferOffset;
    GLbitfield                flags  = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;

    if (offset + size > atlas->TransferBytes)
    {   // additionally discard (orphan) the buffer.
        flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
        offset = 0;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, atlas->TransferBuffer);
    glBindTexture(GL_TEXTURE_2D, atlas->TexturePages[entry->PageIds[frame]]);
    GLvoid *buffer_ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, offset, size, flags);
    if (buffer_ptr != NULL)
    {   // synchronously copy the data into the PBO; unmap the buffer.
        memcpy(buffer_ptr, pixels, size);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        // transfer the region from the PBO to the texture memory. the
        // transfer should be performed asynchronously from the CPU.
        gl::pixel_transfer_h2d_t x;
        x.Target          = GL_TEXTURE_2D;
        x.Format          = format;
        x.DataType        = type;
        x.TargetIndex     = 0;
        x.TargetX         = bounds.X;
        x.TargetY         = bounds.Y;
        x.TargetZ         = 0;
        x.SourceX         = 0;
        x.SourceY         = 0;
        x.SourceZ         = 0;
        x.SourceWidth     = bounds.Width;
        x.SourceHeight    = bounds.Height;
        x.TransferWidth   = bounds.Width;
        x.TransferHeight  = bounds.Height;
        x.TransferSlices  = 1;
        x.TransferSize    = size;
        x.TransferBuffer  = (void*) offset;
        gl::transfer_pixels_h2d(&x);

        // unbind the transfer buffer and texture page.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // update the current offset within the PBO.
        atlas->BufferOffset = size_t(offset + size);
        return true;
    }
    else return false;
}

/*
bool r2d::atlas_transfer_frame(r2d::atlas_t *atlas, r2d::atlas_entry_t *entry, size_t frame, void const *pixels)
{
    r2d::atlas_frame_t &f = entry->Frames[frame];
    GLintptr            o = atlas->BufferOffset;
    GLbitfield          a = GL_MAP_WRITE_BIT;
    GLsizei             n = gl::bytes_per_slice(
    void               *p = NULL;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, atlas->TransferBuffer);
    p = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, o, n, a);
}

        glGenBuffers(1, &atlas->TransferBuffer);
        if (atlas->TransferBuffer == 0) goto error_cleanup;
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, atlas->TransferBuffer);
        glGetIntegerv(GL_UNPACK_ALIGNMENT , &nalign);
        nbytes = gl::bytes_per_slice(config.Format, config.DataType, config.PageWidth, config.PageHeight, size_t(nalign));
        glBufferData(GL_PIXEL_UNPACK_BUFFER, nbytes, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        atlas->TransferBytes = size_t(nbytes);
        atlas->BufferOffset  = 0;
// use glMapBufferRange at cache->BufferOffset, for gl::bytes_per_slice(image_to_copy),
// memcpy the entire source data to the PBO, glUnmapBuffer, and then glTexSubImage2D
// (via the gl::pixel_transfer_h2d() function) to queue the async upload. if not
// enough buffer space, map with discard.
*/

