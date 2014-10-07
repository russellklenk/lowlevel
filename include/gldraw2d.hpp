/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements a basic 2D rendering system (including GUI support) on
/// top of the various low-levele libraries (llopengl, llgui, etc.) This is
/// just enough to take care of the basics that need to be dealt with in any
/// non-trivial application.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef GLDRAW2D_HPP_INCLUDED
#define GLDRAW2D_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <stddef.h>
#include <stdint.h>
#include "lldatain.hpp"
#include "llopengl.hpp"

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Abstract away Windows 32-bit calling conventions and visibility.
#if defined(_WIN32) && defined(_MSC_VER)
    #define  GLDRAW2D_CALL_C    __cdecl
    #if   defined(_MSC_VER)
    #define  GLDRAW2D_IMPORT    __declspec(dllimport)
    #define  GLDRAW2D_EXPORT    __declspec(dllexport)
    #elif defined(__GNUC__)
    #define  GLDRAW2D_IMPORT    __attribute__((dllimport))
    #define  GLDRAW2D_EXPORT    __attribute__((dllexport))
    #else
    #define  GLDRAW2D_IMPORT
    #define  GLDRAW2D_EXPORT
    #endif
#else
    #define  GLDRAW2D_CALL_C
    #if __GNUC__ >= 4
    #define  GLDRAW2D_IMPORT    __attribute__((visibility("default")))
    #define  GLDRAW2D_EXPORT    __attribute__((visibility("default")))
    #endif
#endif

/// @summary Define import/export based on whether we're being used as a DLL.
#if defined(GLDRAW2D_SHARED)
    #ifdef  GLDRAW2D_EXPORTS
    #define GLDRAW2D_PUBLIC     GLDRAW2D_EXPORT
    #else
    #define GLDRAW2D_PUBLIC     GLDRAW2D_IMPORT
    #endif
#else
    #define GLDRAW2D_PUBLIC
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace r2d
{

/*//////////////////
//   Data Types   //
//////////////////*/
/// @summary Flags used with the rectangle packer.
enum packer_flags_e
{
    PACKER_FLAGS_NONE      =  0,
    PACKER_FLAGS_USED      = (1 << 0)
};

/// @summary Flags used with a texture atlas entry.
enum atlas_entry_flags_e
{
    ATLAS_ENTRY_NORMAL     =  0,
    ATLAS_ENTRY_MULTIFRAME = (1 << 0),
    ATLAS_ENTRY_MULTIPAGE  = (1 << 1)
};

/// @summary Represents a single node in a binary tree used for packing
/// rectangles within a single, larger master rectangle.
struct pknode_t
{
    uint32_t            Flags;         /// Combination of packer_flags_e.
    uint32_t            Index;         /// Index of the associated pkrect_t.
    uint32_t            Child[2];      /// Index of the child nodes, 0 if no child.
    uint32_t            Bound[4];      /// Left-Top-Right-Bottom bounding rectangle.
};

/// @summary Represents a single sub-rectangle within a larger image. This
/// data is stored separately from nodes for more cache-friendly behavior.
struct pkrect_t
{
    size_t              X;             /// X-coordinate of upper-left corner of the content.
    size_t              Y;             /// Y-coordinate of upper-left corner of the content.
    size_t              Width;         /// Width of the rectangle content.
    size_t              Height;        /// Height of the rectangle content.
    uint32_t            Content;       /// Application-defined content identifier.
    uint32_t            Flags;         /// Flag bits, copied from the node.
};

/// @summary Stores the data necessary for maintaining the set of sub-rectangles
/// packed together within a single larger master rectangle.
struct packer_t
{
    size_t              Width;         /// Width of primary image, in pixels.
    size_t              Height;        /// Height of primary image, in pixels.
    size_t              Free;          /// Total area currently unused.
    size_t              Used;          /// Total area currently used.
    size_t              Capacity;      /// The capacity of the node and rectangle storage.
    size_t              Count;         /// The number of sub-rectangles currently defined.
    r2d::pknode_t      *Nodes;         /// Storage for node instances.
    r2d::pkrect_t      *Rects;         /// Storage for rectangle data.
};

/// @summary Describes a single frame within a logical texture atlas entry. Items in
/// the texture atlas may be animations, in which case multiple frames are present
/// on the same logical entry.
struct atlas_frame_t
{
    size_t              X;             /// The upper-left corner of the frame on the texture.
    size_t              Y;             /// The upper-left corner of the frame on the texture.
    size_t              Width;         /// The frame width, in pixels.
    size_t              Height;        /// The frame height, in pixels.
};

/// @summary Represents a single logical entry in a texture atlas. The entry
/// may have several frames (if it is an animation, for example.)
struct atlas_entry_t
{
    uint32_t            Name;          /// The hash of the name of the entry.
    uint32_t            Flags;         /// Combination of atlas_entry_flags_e.
    size_t              FrameCount;    /// The number of frames in the entry.
    size_t              MaxWidth;      /// The maximum width of any frame, in pixels.
    size_t              MaxHeight;     /// The maximum height of any frame, in pixels.
    size_t             *PageIds;       /// An array of FrameCount page indices or IDs.
    size_t              Page0;         /// The ID or index of the page for the first frame.
    r2d::atlas_frame_t *Frames;        /// An array of FrameCount frame descriptions.
    r2d::atlas_frame_t  Frame0;        /// Statically allocated storage for the first frame.
};

/// @summary Dynamically builds texture atlases (without mipmaps) for 2D content
/// such as GUIs and sprites. Each texture object is referred to as a page.
/// Updates are streamed to the texture object using a pixel buffer object.
struct image_cache_t
{
    size_t              PageWidth;     /// The width of a texture page, in pixels.
    size_t              PageHeight;    /// The height of a texture page, in pixels.
    size_t              HorizontalPad; /// The horizontal padding between sub-images, in pixels.
    size_t              VerticalPad;   /// The vertical padding between sub-images, in pixels.
    size_t              EntryCapacity; /// The number of entries that can be stored.
    size_t              EntryCount;    /// The number of entries currently valid.
    r2d::atlas_entry_t *PageEntries;   /// The set of entries across all pages.
    GLuint             *TexturePages;  /// The set of OpenGL texture object IDs.
    size_t              BucketCount;   /// The number of buckets defined in the name->index table.
    uint32_t          **EntryNames;    /// A table mapping entry name->index in PageEntries.
    GLuint              TransferBuffer;/// The OpenGL pixel buffer object ID.
};

/*/////////////////
//   Functions   //
/////////////////*/
/// @summary Given a value from the DXGI_FORMAT enumeration, determine the
/// appropriate OpenGL format, base format and data type values. This is useful
/// when loading texture data from a DDS container.
/// @param dxgi A value of the DXGI_FORMAT enumeration (data::dxgi_format_e).
/// @param out_internalformat On return, stores the corresponding OpenGL internal format.
/// @param out_baseformat On return, stores the corresponding OpenGL base format.
/// @param out_datatype On return, stores the corresponding OpenGL data type.
/// @return true if the input format could be mapped to OpenGL.
GLDRAW2D_PUBLIC bool dxgi_format_to_gl(uint32_t dxgi, GLenum *out_internalformat, GLenum *out_format, GLenum *out_datatype);

/// @summary Initializes an packer for dynamically packing several rectangles
/// representing images onto a single, larger rectangle.
/// @param packer The rectangle packer to initialize.
/// @param width The width of the master rectangle.
/// @param height The height of the master rectangle.
/// @param capacity The expected number of sub-rectangles.
GLDRAW2D_PUBLIC bool create_packer(r2d::packer_t *packer, size_t width, size_t height, size_t capacity);

/// @summary Frees resources associated with a rectangle packer.
/// @param packer The rectangle packer to delete.
GLDRAW2D_PUBLIC void delete_packer(r2d::packer_t *packer);

/// @summary Resets a packer to its initial empty state, without freeing the
/// underlying storage resources or modifying the target rectangle size.
/// @param packer The rectangle packer to reset.
GLDRAW2D_PUBLIC void reset_packer(r2d::packer_t *packer);

/// @summary Attempts to position a sub-rectangle within the master rectangle.
/// @param packer The packer used to position sub-rectangles.
/// @param width The un-padded width of the sub-rectangle.
/// @param height The un-padded height of the sub-rectangle.
/// @param hpad The amount of horizontal padding to use.
/// @param vpad The amount of vertical padding to use.
/// @param id The application-defined identifier for the sub-rectangle.
/// @param rect On return, this address is filled with information about the
/// sub-rectangle within the master image. If the sub-rectangle will not fit
/// within the master rectangle, this value is not modified.
/// @return true if the sub-rectangle was positioned on the master rectangle.
GLDRAW2D_PUBLIC bool packer_insert(r2d::packer_t *packer, size_t width, size_t height, size_t hpad, size_t vpad, uint32_t id, r2d::pkrect_t *rect);

/// @summary Allocates storage for and initializes an atlas entry with the specified attributes.
/// @param ent The entry description to initialize.
/// @param name The name of the entry, used during lookup-by-name within the atlas.
/// @param frame_count The number of frames in the entry. This is typically 1.
/// @return true if the entry was created successfully.
GLDRAW2D_PUBLIC bool create_atlas_entry(r2d::atlas_entry_t *ent, uint32_t name, size_t frame_count);

/// @summary Frees storage associated with a texture atlas entry.
/// @param ent The entry description to delete.
GLDRAW2D_PUBLIC void delete_atlas_entry(r2d::atlas_entry_t *ent);

/// @summary Sets the description of a single frame of animation within the atlas entry.
/// @param ent The entry description to update.
/// @param frame_index The zero-based index of the frame to set.
/// @param page_id The identifier or index of the page containing the frame data.
/// @param frame A description of the frame bounds within the texture page.
GLDRAW2D_PUBLIC void set_atlas_entry_frame(r2d::atlas_entry_t *ent, size_t frame_index, size_t page_id, r2d::atlas_frame_t const &frame);

    // we need:
    // a sprite batch for solid-colored quads
    // a sprite batch for textured quads
    // a system to manage the creation and deletion of sprites
    // all-in-one functions for logic and rendering of text & gui controls

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace r2d */

#endif /* GLDRAW2D_HPP_INCLUDED */
