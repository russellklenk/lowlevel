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

/// @summary Defines metadata associated with a hash bucket.
struct bucket_t
{
    size_t              Capacity;      /// The bucket capacity, in items.
    size_t              Count;         /// The number of items currently in-use.
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

/// @summary Defines the configuration data for an image cache, used to dynamically
/// build texture atlases (without mipmaps) for 2D content such as GUIs and sprites.
struct atlas_config_t
{
    size_t              PageWidth;     /// The width of the texture page(s), in pixels.
    size_t              PageHeight;    /// The height of the texture page(s), in pixels.
    size_t              HorizontalPad; /// The horizontal padding amount, in pixels.
    size_t              VerticalPad;   /// The vertical padding amount, in pixels.
    size_t              ExpectedCount; /// The number of images expected to be cached, or 0.
    GLenum              Layout;        /// The OpenGL pixel layout of the texture page(s), ex. GL_BGRA.
    GLenum              Format;        /// The OpenGL internal format of the texture page(s), ex. GL_RGBA8.
    GLenum              DataType;      /// The OpenGL data type of the texture page(s), ex. GL_UNSIGNED_INT_8_8_8_8_REV.
};

/// @summary Dynamically builds texture atlases (without mipmaps) for 2D content
/// such as GUIs and sprites. Each texture object is referred to as a page.
/// Updates are streamed to the texture object using a pixel buffer object.
struct atlas_t
{
    size_t              PageWidth;     /// The width of a texture page, in pixels.
    size_t              PageHeight;    /// The height of a texture page, in pixels.
    size_t              HorizontalPad; /// The horizontal padding between sub-images, in pixels.
    size_t              VerticalPad;   /// The vertical padding between sub-images, in pixels.
    size_t              EntryCapacity; /// The number of entries that can be stored.
    size_t              EntryCount;    /// The number of entries currently valid.
    r2d::atlas_entry_t *EntryList;     /// The set of entries across all pages.
    size_t              PageCapacity;  /// The number of texture page IDs that can be stored.
    size_t              PageCount;     /// The number of texture pages used.
    r2d::packer_t      *PagePackers;   /// The list of rectangle packers used to place sub-images on the page.
    GLuint             *TexturePages;  /// The set of OpenGL texture object IDs.
    size_t              BucketCount;   /// The number of buckets defined in the name->index table.
    r2d::bucket_t      *BucketList;    /// The list of bucket metadata; BucketCount items.
    uint32_t          **EntryNames;    /// A table mapping entry name->index in EntryList.
    uint32_t          **EntryIndices;  /// A table mapping entry name->index in EntryList.
    GLenum              PageLayout;    /// The OpenGL pixel layout of the texture page(s), ex. GL_BGRA.
    GLenum              PageFormat;    /// The OpenGL internal format of the texture page(s),  ex. GL_RGBA8.
    GLenum              PageDataType;  /// The OpenGL data type of the texture page(s), ex. GL_UNSIGNED_INT_8_8_8_8_REV.
    GLuint              TransferBuffer;/// The OpenGL pixel buffer object ID.
    size_t              TransferBytes; /// The size of the transfer buffer, in bytes.
    size_t              BufferOffset;  /// The current byte offset into the transfer buffer.
};

/*/////////////////
//   Functions   //
/////////////////*/
/// @summary Given a value from the DXGI_FORMAT enumeration, determine the
/// appropriate OpenGL format, base format and data type values. This is useful
/// when loading texture data from a DDS container.
/// @param dxgi A value of the DXGI_FORMAT enumeration (data::dxgi_format_e).
/// @param out_internalformat On return, stores the corresponding OpenGL internal format.
/// @param out_baseformat On return, stores the corresponding OpenGL base format (layout).
/// @param out_datatype On return, stores the corresponding OpenGL data type.
/// @return true if the input format could be mapped to OpenGL.
GLDRAW2D_PUBLIC bool dxgi_format_to_gl(uint32_t dxgi, GLenum &out_internalformat, GLenum &out_format, GLenum &out_datatype);

/// @summary Given a chunk of data representing a DDS file, parse the data and
/// upload it into a new OpenGL texture object.
/// @param data A blob of data representing a DDS-formatted image.
/// @param data_size The maximum number of bytes to read from the DDS data.
/// @param out_levels On return, this address points to an array of descriptions
/// of the levels in the mipmap chain described in the DDS file. This array is
/// allocated using the standard C library malloc() function. The caller is
/// responsible for freeing the returned array using the free() function.
/// @param out_count On return, stores the number if items in the levels array.
/// @param out_texId On return, stores the OpenGL texture object ID.
/// @return true if the DDS was successfully loaded into the OpenGL texture.
GLDRAW2D_PUBLIC bool load_dds(void const *dds_data, size_t dds_size, gl::level_desc_t **out_levels, size_t *out_count, GLuint *out_texId);

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

/// @summary Allocates internal storage and GPU resources for an image atlas.
/// @param atlas The image atlas to initialize.
/// @param config Image atlas configuration parameters.
/// @return true if the image atlas was successfully initialized.
GLDRAW2D_PUBLIC bool create_atlas(r2d::atlas_t *atlas, r2d::atlas_config_t const &config);

/// @summary Frees all storage and GPU resources associated with an image atlas.
/// @param atlas The image atlas to delete.
GLDRAW2D_PUBLIC void delete_atlas(r2d::atlas_t *atlas);

/// @summary Indicates that no more images will be uploaded to the specified
/// atlas, and deletes the transfer object associated with the atlas.
/// @param atlas The image atlas to freeze.
GLDRAW2D_PUBLIC void freeze_atlas(r2d::atlas_t *atlas);

/// @summary Locates the metadata for an item within an image atlas given its name.
/// @param atlas The image atlas to search.
/// @param name The 32-bit unique identifier of the item specified when the item was added.
/// @return The associated metadata, or NULL.
GLDRAW2D_PUBLIC r2d::atlas_entry_t* find_atlas_entry(r2d::atlas_t *atlas, uint32_t name);

/// @summary Locates the metadata for an item within an image atlas given its index.
/// @param atlas The image atlas to search.
/// @param index The zero-based index of the entry to retrieve.
/// @return The associated metadata, or NULL.
GLDRAW2D_PUBLIC r2d::atlas_entry_t* get_atlas_entry(r2d::atlas_t *atlas, size_t index);

/// @summary Creates a logical entry on the texture atlas. This only allocates the necessary
/// structures; it does not place any images or upload any data to texture pages.
/// @param atlas The image atlas to update.
/// @param name A unique 32-bit integer identifier for the image within the atlas.
/// @param frame_count The number of frames in the animation sequence.
/// @param out_index On return, this location stores the index of the associated atlas entry
/// within the image atlas, which can be used to look up placement information.
/// @return The record representing the entry on the image atlas.
GLDRAW2D_PUBLIC r2d::atlas_entry_t* atlas_create_entry(r2d::atlas_t *atlas, uint32_t name, size_t frame_count, size_t *out_index);

/// @summary Creates a new entry on the image atlas and places one or more rectangles.
/// @param atlas The image atlas to update.
/// @param name A unique 32-bit integer identifier for the image within the atlas.
/// @param frame_count The number of frames in the animation sequence.
/// @param frame_widths An array of values specifying the horizontal extent of each frame.
/// @param frame_heights An array of values specifying the vertical extent of each frame.
/// @param out_index On return, this location stores the index of the associated atlas entry
/// within the image atlas, which can be used to look up placement information.
/// @return The record representing the entry on the image atlas.
GLDRAW2D_PUBLIC r2d::atlas_entry_t* atlas_create_entry(r2d::atlas_t *atlas, uint32_t name, size_t frame_count, size_t const *frame_widths, size_t const *frame_heights, size_t *out_index);

/// @summary Creates a new entry on the image atlas and places one or more rectangles.
/// @param atlas The image atlas to update.
/// @param name A unique 32-bit integer identifier for the image within the atlas.
/// @param frame_count The number of frames in the animation sequence.
/// @param frame_widths An array of values specifying the horizontal extent of each frame.
/// @param frame_heights An array of values specifying the vertical extent of each frame.
/// @param hpad The amount of padding along the horizontal edges of the image, in pixels.
/// @param vpad The amount of padding along the vertical edges of the image, in pixels.
/// @param out_index On return, this location stores the index of the associated atlas entry
/// within the image atlas, which can be used to look up placement information.
/// @return The record representing the entry on the image atlas.
GLDRAW2D_PUBLIC r2d::atlas_entry_t* atlas_create_entry(r2d::atlas_t *atlas, uint32_t name, size_t frame_count, size_t const *frame_widths, size_t const *frame_heights, size_t hpad, size_t vpad, size_t *out_index);

/// @summary Places an image within the texture atlas. This only reserves the space for the
/// image; it does not upload any data to texture pages.
/// @param atlas The image atlas to update.
/// @param entry The logical entry within the atlas to modify.
/// @param frame The zero-based index of the frame being placed.
/// @param width The width of the frame, in pixels.
/// @param height The height of the frame, in pixels.
/// @return true if the image was successfully placed on the atlas.
GLDRAW2D_PUBLIC bool atlas_place_frame(r2d::atlas_t *atlas, r2d::atlas_entry_t *entry, size_t frame, size_t width, size_t height);

/// @summary Places an image within the texture atlas. This only reserves the space for the
/// image; it does not upload any data to texture pages.
/// @param atlas The image atlas to update.
/// @param entry The logical entry within the atlas to modify.
/// @param frame The zero-based index of the frame being placed.
/// @param width The width of the frame, in pixels.
/// @param height The height of the frame, in pixels.
/// @param hpad The amount of padding along the horizontal edges of the image, in pixels.
/// @param vpad The amount of padding along the vertical edges of the image, in pixels.
/// @return true if the image was successfully placed on the atlas.
GLDRAW2D_PUBLIC bool atlas_place_frame(r2d::atlas_t *atlas, r2d::atlas_entry_t *entry, size_t frame, size_t width, size_t height, size_t hpad, size_t vpad);

/// @summary Transfers pixel data for an image or frame to the associated texture page.
/// @param atlas The atlas on which the entry has been placed.
/// @param entry The metadata of the entry to update, retrieved with [find|get]_atlas_entry().
/// @param frame The zero-based index of the frame whose data is being transferred.
/// @param pixels The pixel data to transfer. This data is copied to a pixel buffer object
/// and then queued for asynchronous transfer to the GPU.
/// @return true if the pixel data transfer was scheduled.
GLDRAW2D_PUBLIC bool atlas_transfer_frame(r2d::atlas_t *atlas, r2d::atlas_entry_t *entry, size_t frame, void const *pixels);

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
