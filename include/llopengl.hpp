/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines some utility functions for working with the OpenGL API.
/// Note that this module does not handle setting up an OpenGL context, which
/// is best left to a third-party library like GLFW.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LLOPENGL_HPP_INCLUDED
#define LLOPENGL_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <stddef.h>
#include <stdint.h>
#include <algorithm>

// if it is detected that GLEW is being used, #define LLOPENGL_EXTERNAL_GL so
// that platform GL headers are not #included in the block of code below.
#if !defined(__glew_h__) && !defined(__GLEW_H__)
    #ifdef LLOPENGL_USE_GLEW
        #include "glew.h"
        #ifndef  LLOPENGL_EXTERNAL_GL
        #define  LLOPENGL_EXTERNAL_GL
        #endif
    #endif
#else
    #ifndef LLOPENGL_EXTERNAL_GL
    #define LLOPENGL_EXTERNAL_GL
    #endif
#endif

// the user may be employing a third-party extension library, like GLEW,
// that may forbid direct inclusion of gl.h. in that case, #define LLOPENGL_EXTERNAL_GL
// to 1, and the replacement headers should be included prior to this header.
// for Windows and Linux platforms, the user should make sure that they have
// the glext.h header (available from the opengl.org website) corresponding to
// the OpenGL version they want to use.
#ifndef LLOPENGL_EXTERNAL_GL
    #if defined(__APPLE__)
        #include <OpenGL/OpenGL.h>
        #include <OpenGL/gl3.h>
        #include <OpenGL/gl3ext.h>
    #elif defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
        #define  GL_GLEXT_PROTOTYPES
        #include <windows.h>
        #include <GL/gl.h>
        #include "GL/glext.h"
    #else
        #define  GL_GLEXT_PROTOTYPES
        #include <GL/gl.h>
        #include "GL/glext.h"
    #endif
#endif

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Abstract away Windows 32-bit calling conventions.
#if defined(_WIN32) && defined(_MSC_VER)
    #define  LLOPENGL_CALL_C    __cdecl
#else
    #define  LLOPENGL_CALL_C
#endif

/// @summary Abstract away Windows DLL import/export.
#if defined(_MSC_VER)
    #define  LLOPENGL_IMPORT    __declspec(dllimport)
    #define  LLOPENGL_EXPORT    __declspec(dllexport)
#else
    #define  LLOPENGL_IMPORT
    #define  LLOPENGL_EXPORT
#endif

/// @summary Define import/export based on whether we're being used as a DLL.
#if defined(LLOPENGL_SHARED)
    #ifdef  LLOPENGL_EXPORTS
    #define LLOPENGL_PUBLIC     LLOPENGL_EXPORT
    #else
    #define LLOPENGL_PUBLIC     LLOPENGL_IMPORT
    #endif
#else
    #define LLOPENGL_PUBLIC
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace gl {

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary Defines the maximum number of shader stages. Currently, we have
/// stages GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER,
/// GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER and GL_COMPUTE_SHADER.
#ifndef GL_MAX_SHADER_STAGES
#define GL_MAX_SHADER_STAGES          (6U)
#endif

/// @summary Macro to convert a byte offset into a pointer.
/// @param x The byte offset value.
/// @return The offset value, as a pointer.
#ifndef GL_BUFFER_OFFSET
#define GL_BUFFER_OFFSET(x)           ((GLvoid*)(((uint8_t*)NULL)+(x)))
#endif

/// @summary Defines the location of the position and texture attributes within
/// the position-texture-color vertex. These attributes are encoded as a vec4.
#ifndef GL_SPRITE_PTC_LOCATION_PTX
#define GL_SPRITE_PTC_LOCATION_PTX    (0)
#endif

/// @summary Defines the location of the tint color attribute within the vertex.
/// This attribute is encoded as a packed 32-bit RGBA color value.
#ifndef GL_SPRITE_PTC_LOCATION_CLR
#define GL_SPRITE_PTC_LOCATION_CLR    (1)
#endif

/*//////////////////
//   Data Types   //
//////////////////*/
    /// @summary Describes an active GLSL attribute.
struct attribute_desc_t
{
    GLenum  DataType;     /// The data type, ex. GL_FLOAT
    GLint   Location;     /// The assigned location within the program
    size_t  DataSize;     /// The size of the attribute data, in bytes
    size_t  DataOffset;   /// The offset of the attribute data, in bytes
    size_t  Dimension;    /// The data dimension, for array types
};

/// @summary Describes an active GLSL sampler.
struct sampler_desc_t
{
    GLenum  SamplerType;  /// The sampler type, ex. GL_SAMPLER_2D
    GLenum  BindTarget;   /// The texture bind target, ex. GL_TEXTURE_2D
    GLint   Location;     /// The assigned location within the program
    GLint   ImageUnit;    /// The assigned texture image unit, ex. GL_TEXTURE0
};

/// @summary Describes an active GLSL uniform.
struct uniform_desc_t
{
    GLenum  DataType;     /// The data type, ex. GL_FLOAT
    GLint   Location;     /// The assigned location within the program
    size_t  DataSize;     /// The size of the uniform data, in bytes
    size_t  DataOffset;   /// The offset of the uniform data, in bytes
    size_t  Dimension;    /// The data dimension, for array types
};

/// @summary Describes a successfully compiled and linked GLSL shader program.
struct shader_desc_t
{
    size_t            UniformCount;   /// Number of active uniforms
    uint32_t         *UniformNames;   /// Hashed names of active uniforms
    uniform_desc_t   *Uniforms;       /// Information about active uniforms
    size_t            AttributeCount; /// Number of active inputs
    uint32_t         *AttributeNames; /// Hashed names of active inputs
    attribute_desc_t *Attributes;     /// Information about active inputs
    size_t            SamplerCount;   /// Number of active samplers
    uint32_t         *SamplerNames;   /// Hashed names of samplers
    sampler_desc_t   *Samplers;       /// Information about active samplers
    void             *Metadata;       /// Pointer to the raw memory
};

/// @summary Describes the source code input to the shader compiler and linker.
/// This is provided as a convenience and allows building a shader in one step.
struct shader_source_t
{
    size_t  StageCount;                        /// Number of valid stages
    GLenum  StageNames [GL_MAX_SHADER_STAGES]; /// GL_VERTEX_SHADER, etc.
    GLsizei StringCount[GL_MAX_SHADER_STAGES]; /// Number of strings per-stage
    char  **SourceCode [GL_MAX_SHADER_STAGES]; /// NULL-terminated ASCII strings
};

/// @summary Describes a single level of an image in a mipmap chain.
struct level_desc_t
{
    size_t Index;           /// The mipmap level index (0=highest resolution)
    size_t Width;           /// The width of the level, in pixels
    size_t Height;          /// The height of the level, in pixels
    size_t Slices;          /// The number of slices in the level
    size_t BytesPerElement; /// The number of bytes per-channel or pixel
    size_t BytesPerRow;     /// The number of bytes per-row
    size_t BytesPerSlice;   /// The number of bytes per-slice
    GLenum Format;          /// The OpenGL internal format
    GLenum DataType;        /// The OpenGL element data type
    GLenum BaseFormat;      /// The OpenGL base format
};

/// @summary Describes a transfer of pixel data from the device (GPU) to the
/// host (CPU), using glReadPixels, glGetTexImage or glGetCompressedTexImage.
/// The Target[X/Y/Z] fields can be specified to have the call calculate the
/// offset from the TransferBuffer pointer, or can be set to 0 if the
/// TransferBuffer is pointing to the start of the target data. Note that for
/// texture images, the entire mip level is transferred, and so the TransferX,
/// TransferY, TransferWidth and TransferHeight fields are ignored. The fields
/// Target[Width/Height] describe dimensions of the target image, which may be
/// different than the dimensions of the region being transferred. Note that
/// the Format and DataType describe the target image.
struct pixel_transfer_d2h_t
{
    GLenum Target;          /// The image source, ex. GL_READ_FRAMEBUFFER or GL_TEXTURE_2D
    GLenum Format;          /// The desired pixel data format, ex. GL_BGRA
    GLenum DataType;        /// The desired pixel data type, ex. GL_UNSIGNED_INT_8_8_8_8_REV
    GLuint PackBuffer;      /// The PBO to use as the pack target, or 0
    size_t SourceIndex;     /// The source mip level or array index, 0 for framebuffer
    size_t TargetX;         /// The upper-left-front corner on the target image
    size_t TargetY;         /// The upper-left-front corner on the target image
    size_t TargetZ;         /// The upper-left-front corner on the target image
    size_t TargetWidth;     /// The width of the full target image, in pixels
    size_t TargetHeight;    /// The height of the full target image, in pixels
    size_t TransferX;       /// The upper-left corner of the region to transfer
    size_t TransferY;       /// The upper-left corner of the region to transfer
    size_t TransferWidth;   /// The width of the region to transfer, in pixels
    size_t TransferHeight;  /// The height of the region to transfer, in pixels
    void  *TransferBuffer;  /// Pointer to target image data, or PBO byte offset
};

/// @summary Describes a transfer of pixel data from the host (CPU) to the
/// device (GPU), using glTexSubImage or glCompressedTexSubImage. The fields
/// Target[X/Y/Z] indicate where to place the data on the target texture. The
/// Source[X/Y/Z] fields can be specified to have the call calculate the offset
/// from the TransferBuffer pointer, or can be set to 0 if TransferBuffer is
/// pointing to the start of the source data. The Source[Width/Height] describe
/// the dimensions of the entire source image, which may be different than the
/// dimensions of the region being transferred. The source of the transfer may
/// be either a Pixel Buffer Object, in which case the UnpackBuffer should be
/// set to the handle of the PBO, and TransferBuffer is an offset, or an
/// application memory buffer, in which case UnpackBuffer should be set to 0
/// and TransferBuffer should point to the source image data. A host to device
/// transfer can be used to upload a source image, or a portion thereof, to a
/// texture object on the GPU. Note that the Format and DataType describe the
/// source image, and not the texture object.
struct pixel_transfer_h2d_t
{
    GLenum Target;          /// The image target, ex. GL_TEXTURE_2D
    GLenum Format;          /// The format of your pixel data, ex. GL_BGRA
    GLenum DataType;        /// The layout of your pixel data, ex. GL_UNSIGNED_INT_8_8_8_8_REV
    GLuint UnpackBuffer;    /// The PBO to use as the unpack source, or 0
    size_t TargetIndex;     /// The target mip level or array index
    size_t TargetX;         /// The upper-left-front corner on the target texture
    size_t TargetY;         /// The upper-left-front corner on the target texture
    size_t TargetZ;         /// The upper-left-front corner on the target texture
    size_t SourceX;         /// The upper-left-front corner on the source image
    size_t SourceY;         /// The upper-left-front corner on the source image
    size_t SourceZ;         /// The upper-left-front corner on the source image
    size_t SourceWidth;     /// The width of the full source image, in pixels
    size_t SourceHeight;    /// The height of the full source image, in pixels
    size_t TransferWidth;   /// The width of the region to transfer, in pixels
    size_t TransferHeight;  /// The height of the region to transfer, in pixels
    size_t TransferSlices;  /// The number of slices to transfer
    size_t TransferSize;    /// The total number of bytes to transfer
    void  *TransferBuffer;  /// Pointer to source image data, or PBO byte offset
};

/// @summary A structure representing a single interleaved sprite vertex in
/// the vertex buffer. The vertex encodes 2D screen space position, texture
/// coordinate, and packed ABGR color values into 20 bytes per-vertex. The
/// GPU expands the vertex into 32 bytes. Tint color is constant per-sprite.
#pragma pack(push, 1)
struct sprite_vertex_ptc_t
{
    float     XYUV[4];          /// Position.x, Position.y (screen space), 2d texcoord.
    uint32_t  TintColor;        /// The ABGR tint color.
};
#pragma pack(pop)

/// @summary A structure representing the data required to describe a single
/// sprite within the application. Each sprite is translated into four vertices
/// and six indices. The application fills out a sprite descriptor and pushes
/// it to the batch for later processing.
struct sprite_t
{
    float     ScreenX;          /// Screen-space X coordinate of the origin.
    float     ScreenY;          /// Screen-space Y coordinate of the origin.
    float     OriginX;          /// Origin X-offset from the upper-left corner.
    float     OriginY;          /// Origin Y-offset from the upper-left corner.
    float     ScaleX;           /// The horizontal scale factor.
    float     ScaleY;           /// The vertical scale factor.
    float     Orientation;      /// The angle of orientation, in radians.
    uint32_t  TintColor;        /// The ABGR tint color.
    uint32_t  ImageX;           /// Y-offset of the upper-left corner of the source image.
    uint32_t  ImageY;           /// Y-offset of the upper-left corner of the source image.
    uint32_t  ImageWidth;       /// The width of the source image, in pixels.
    uint32_t  ImageHeight;      /// The height of the source image, in pixels.
    uint32_t  TextureWidth;     /// The width of the texture defining the source image.
    uint32_t  TextureHeight;    /// The height of the texture defining the source image.
    uint32_t  LayerDepth;       /// The layer depth of the sprite, increasing into the background.
    uint32_t  RenderState;      /// An application-defined render state identifier.
};

/// @summary A structure storing the data required to represent a sprite within
/// the sprite batch. Sprites are transformed to quads when they are pushed to
/// the sprite batch. Each quad definition is 64 bytes.
struct sprite_quad_t
{
    float     Source[4];        /// The XYWH rectangle on the source texture.
    float     Target[4];        /// The XYWH rectangle on the screen.
    float     Origin[2];        /// The XY origin point of rotation.
    float     Scale[2];         /// Texture coordinate scale factors.
    float     Orientation;      /// The angle of orientation, in radians.
    uint32_t  TintColor;        /// The ABGR tint color.
};

/// @summary Data used for sorting buffered quads. Grouped together to improve
/// cache usage by avoiding loading all of the data for a quad_t.
struct sprite_sort_data_t
{
    uint32_t  LayerDepth;       /// The layer depth of the sprite, increasing into the background.
    uint32_t  RenderState;      /// The render state associated with the sprite.
};

/// @summary A structure storing all of the data required to render sprites
/// using a particular effect. All of the shader state is maintained externally.
struct sprite_effect_t
{
    size_t    VertexCapacity;   /// The maximum number of vertices we can buffer.
    size_t    VertexOffset;     /// Current offset (in vertices) in buffer.
    size_t    VertexSize;       /// Size of one vertex, in bytes.
    size_t    IndexCapacity;    /// The maximum number of indices we can buffer.
    size_t    IndexOffset;      /// Current offset (in indices) in buffer.
    size_t    IndexSize;        /// Size of one index, in bytes.
    uint32_t  CurrentState;     /// The active render state identifier.
    GLuint    VertexArray;      /// The VAO describing the vertex layout.
    GLuint    VertexBuffer;     /// Buffer object for dynamic vertex data.
    GLuint    IndexBuffer;      /// Buffer object for dynamic index data.
    GLboolean BlendEnabled;     /// GL_TRUE if blending is enabled.
    GLenum    BlendSourceColor; /// The source color blend factor.
    GLenum    BlendSourceAlpha; /// The source alpha blend factor.
    GLenum    BlendTargetColor; /// The destination color blend factor.
    GLenum    BlendTargetAlpha; /// The destination alpha blend factor.
    GLenum    BlendFuncColor;   /// The color channel blend function.
    GLenum    BlendFuncAlpha;   /// The alpha channel blend function.
    GLfloat   BlendColor[4];    /// RGBA constant blend color.
    float     Projection[16];   /// Projection matrix for current viewport
};

/// @summary Signature for a function used to apply render state for an effect
/// prior to rendering any quads. The function should perform operations like
/// setting the active program, calling gl::sprite_effect_bind_buffers() and
/// gl::sprite_effect_apply_blendstate(), and so on.
/// @param effect The effect being used for rendering.
/// @param context Opaque data passed by the application.
typedef void (*sprite_effect_setup_fn)(gl::sprite_effect_t *effect, void *context);

/// @summary Signature for a function used to apply render state for a quad primitive.
/// The function should perform operations like setting up samplers, uniforms, and so on.
/// @param effect The effect being used for rendering.
/// @param render_state The application render state identifier.
/// @param context Opaque data passed by the application.
typedef void (*sprite_effect_apply_fn)(gl::sprite_effect_t *effect, uint32_t render_state, void *context);

/// @summary Wraps a set of function pointers used to apply effect-specific state.
struct sprite_effect_apply_t
{
    gl::sprite_effect_setup_fn SetupEffect; /// Callback used to perform initial setup.
    gl::sprite_effect_apply_fn ApplyState;  /// Callback used to perform state changes.
};

/// @summary A structure for buffering data associated with a set of sprites.
struct sprite_batch_t
{
    size_t                     Count;       /// The number of buffered sprites.
    size_t                     Capacity;    /// The capacity of the various buffers.
    gl::sprite_quad_t         *Quads;       /// Buffer for transformed quad data.
    gl::sprite_sort_data_t    *State;       /// Render state identifiers for each quad.
    uint32_t                  *Order;       /// Insertion order values for each quad.
};

/// @summary Maintains the state associated with a default sprite shader
/// accepting position, texture and color attributes as vertex input.
struct sprite_shader_ptc_clr_t
{
    GLuint                     Program;     /// The OpenGL program object ID.
    gl::shader_desc_t          ShaderDesc;  /// Metadata about the shader program.
    gl::attribute_desc_t      *AttribPTX;   /// Information about the Position-Texture attribute.
    gl::attribute_desc_t      *AttribCLR;   /// Information about the ARGB color attribute.
    gl::uniform_desc_t        *UniformMSS;  /// Information about the screenspace -> clipspace matrix.
};

/// @summary Maintains the state associated with a default sprite shader
/// accepting position, texture and color attributes as vertex input.
struct sprite_shader_ptc_tex_t
{
    GLuint                     Program;     /// The OpenGL program object ID.
    gl::shader_desc_t          ShaderDesc;  /// Metadata about the shader program.
    gl::attribute_desc_t      *AttribPTX;   /// Information about the Position-Texture attribute.
    gl::attribute_desc_t      *AttribCLR;   /// Information about the ARGB color attribute.
    gl::sampler_desc_t        *SamplerTEX;  /// Information about the texture sampler.
    gl::uniform_desc_t        *UniformMSS;  /// Information about the screenspace -> clipspace matrix.
};

/*//////////////
//  Functors  //
//////////////*/
/// @summary Functor used for sorting sprites into back-to-front order.
struct back_to_front
{
    gl::sprite_batch_t *batch;

    inline back_to_front(gl::sprite_batch_t *sprite_batch)
        :
        batch(sprite_batch)
    { /* empty */ }

    inline bool operator()(uint32_t ia, uint32_t ib)
    {
        gl::sprite_sort_data_t const &sdata_a = batch->State[ia];
        gl::sprite_sort_data_t const &sdata_b = batch->State[ib];
        if (sdata_a.LayerDepth  > sdata_b.LayerDepth)  return true;
        if (sdata_a.LayerDepth  < sdata_b.LayerDepth)  return false;
        if (sdata_a.RenderState < sdata_b.RenderState) return true;
        if (sdata_a.RenderState > sdata_b.RenderState) return false;
        return  (ia < ib);
    }
};

/// @summary Functor used for sorting sprites into front-to-back order.
struct front_to_back
{
    gl::sprite_batch_t *batch;

    inline front_to_back(gl::sprite_batch_t *sprite_batch)
        :
        batch(sprite_batch)
    { /* empty */ }

    inline bool operator()(uint32_t ia, uint32_t ib)
    {
        gl::sprite_sort_data_t const &sdata_a = batch->State[ia];
        gl::sprite_sort_data_t const &sdata_b = batch->State[ib];
        if (sdata_a.LayerDepth  < sdata_b.LayerDepth)  return true;
        if (sdata_a.LayerDepth  > sdata_b.LayerDepth)  return false;
        if (sdata_a.RenderState < sdata_b.RenderState) return true;
        if (sdata_a.RenderState > sdata_b.RenderState) return false;
        return  (ia > ib);
    }
};

/// @summary Functor used for sorting sprites by render state.
struct by_render_state
{
    gl::sprite_batch_t *batch;

    inline by_render_state(gl::sprite_batch_t *sprite_batch)
        :
        batch(sprite_batch)
    { /* empty */ }

    inline bool operator()(uint32_t ia, uint32_t ib)
    {
        gl::sprite_sort_data_t const &sdata_a = batch->State[ia];
        gl::sprite_sort_data_t const &sdata_b = batch->State[ib];
        if (sdata_a.RenderState < sdata_b.RenderState) return true;
        if (sdata_a.RenderState > sdata_b.RenderState) return false;
        return  (ia < ib);
    }
};

/*/////////////////
//   Functions   //
/////////////////*/
/// @summary Given an OpenGL data type value, calculates the corresponding size.
/// @param data_type The OpenGL data type value, for example, GL_UNSIGNED_BYTE.
/// @return The size of a single element of the specified type, in bytes.
LLOPENGL_PUBLIC size_t data_size(GLenum data_type);

/// @summary Given an ASCII string name, calculates a 32-bit hash value. This
/// function is used for generating names for shader attributes, uniforms and
/// samplers, allowing for more efficient look-up by name.
/// @param name A NULL-terminated ASCII string identifier.
/// @return A 32-bit unsigned integer hash of the name.
LLOPENGL_PUBLIC uint32_t shader_name(char const *name);

/// @summary Determines whether an identifier would be considered a GLSL built-
/// in value; that is, whether the identifier starts with 'gl_'.
/// @param name A NULL-terminated ASCII string identifier.
/// @return true if @a name starts with 'gl_'.
LLOPENGL_PUBLIC bool builtin(char const *name);

/// @summary Creates an OpenGL shader object and compiles shader source code.
/// @param shader_type The shader type, for example GL_VERTEX_SHADER.
/// @param shader_source An array of NULL-terminated ASCII strings representing
/// the source code of the shader program.
/// @param string_count The number of strings in the @a shader_source array.
/// @param out_shader On return, this address stores the OpenGL shader object.
/// @param out_log_size On return, this address stores the number of bytes in
/// the shader compile log. Retrieve log content with glsprite_compile_log().
/// @return true if shader compilation was successful.
LLOPENGL_PUBLIC bool compile_shader(GLenum shader_type, char **shader_source, size_t string_count, GLuint *out_shader, size_t *out_log_size);

/// @summary Retrieves the log for the most recent shader compilation.
/// @param shader The OpenGL shader object.
/// @param buffer The destination buffer.
/// @param buffer_size The maximum number of bytes to write to @a buffer.
LLOPENGL_PUBLIC void copy_compile_log(GLuint shader, char *buffer, size_t buffer_size);

/// @summary Creates an OpenGL program object and attaches, but does not link,
/// associated shader fragments. Prior to linking, vertex attribute and
/// fragment output bindings should be specified by the application using
/// assign_vertex_attributes() and assign_fragment_outputs().
/// @param shader_list The list of shader object to attach.
/// @param shader_count The number of shader objects in the shader list.
/// @param out_program On return, this address stores the program object.
/// @return true if the shader objects were attached successfully.
LLOPENGL_PUBLIC bool attach_shaders(GLuint *shader_list, size_t shader_count, GLuint *out_program);

/// @summary Sets the mapping of vertex attribute names to zero-based indices
/// of the vertex attributes within the vertex format definition. See the
/// glEnableVertexAttribArray and glVertexAttribPointer documentation. The
/// vertex attribute bindings should be set before linking the shader program.
/// @param program The OpenGL program object being modified.
/// @param attrib_names An array of NULL-terminated ASCII strings specifying
/// the vertex attribute names.
/// @param attrib_locations An array of zero-based integers specifying the
/// vertex attribute location assignments, such that attrib_names[i] is bound
/// to attrib_locations[i].
/// @param attrib_count The number of elements in the name and location arrays.
/// @return true if the bindings were assigned without error.
LLOPENGL_PUBLIC bool assign_vertex_attributes(GLuint program, char const **attrib_names, GLuint  *attrib_locations, size_t attrib_count);

/// @summary Sets the mapping of fragment shader outputs to draw buffer
/// indices. See the documentation for glBindFragDataLocation and glDrawBuffers
/// for more information. The fragment shader output bindings should be set
/// before linking the shader program.
/// @param program The OpenGL program object being modified.
/// @param output_names An array of NULL-terminated ASCII strings specifying
/// the fragment shader output names.
/// @param output_locations An array of zero-based integers specifying the
/// draw buffer index assignments, such that output_names[i] is bound
/// to draw buffer output_locations[i].
/// @param output_count The number of elements in the name and location arrays.
/// @return true if the bindings were assigned without error.
LLOPENGL_PUBLIC bool assign_fragment_outputs(GLuint program, char const **output_names, GLuint *output_locations, size_t output_count);

/// @summary Links and validates shader fragments and assigns any automatic
/// vertex attribute, fragment shader output and uniform locations.
/// @param program The OpenGL program object to link.
/// @param out_max_name On return, this address is updated with number of bytes
/// required to store the longest name string of a vertex attribute, uniform,
/// texture sampler or fragment shader output, including the terminating NULL.
/// @param out_log_size On return, this address is updated with the number of
/// bytes in the shader linker log. Retrieve log content with the function
/// glsprite_linker_log().
/// @return true if shader linking was successful.
LLOPENGL_PUBLIC bool link_program(GLuint program, size_t *out_max_name, size_t *out_log_size);

/// @summary Retrieves the log for the most recent shader program linking.
/// @param program The OpenGL program object.
/// @param buffer The destination buffer.
/// @param buffer_size The maximum number of bytes to write to @a buffer.
LLOPENGL_PUBLIC void copy_linker_log(GLuint program, char *buffer, size_t buffer_size);

/// @summary Allocates memory for a shader_desc_t structure using the standard
/// C library malloc() function. The various counts can be obtained from the
/// values returned by reflect_program_counts().
/// @param desc The shader description to initialize.
/// @param num_attribs The number of active attributes for the program.
/// @param num_samplers The number of active texture samplers for the program.
/// @param num_uniforms The number of active uniforms for the program.
/// @return true if memory was allocated successfully.
LLOPENGL_PUBLIC bool shader_desc_alloc(gl::shader_desc_t *desc, size_t num_attribs, size_t num_samplers, size_t num_uniforms);

/// @summary Releases memory for a shader_desc_t structure using the standard
/// C library free() function.
/// @param desc The shader description to release.
LLOPENGL_PUBLIC void shader_desc_free(gl::shader_desc_t *desc);

/// @summary Counts the number of active vertex attribues, texture samplers
/// and uniform values defined in a shader program.
/// @param program The OpenGL program object to query.
/// @param buffer A temporary buffer used to hold attribute and uniform names.
/// @param buffer_size The maximum number of bytes that can be written to the
/// temporary name buffer.
/// @param include_builtins Specify true to include GLSL builtin values in the
/// returned vertex attribute count.
/// @param out_num_attribs On return, this address is updated with the number
/// of active vertex attribute values.
/// @param out_num_samplers On return, this address is updated with the number
/// of active texture sampler values.
/// @param out_num_uniforms On return, this address is updated with the number
/// of active uniform values.
LLOPENGL_PUBLIC void reflect_program_counts(
    GLuint  program,
    char   *buffer,
    size_t  buffer_size,
    bool    include_builtins,
    size_t *out_num_attribs,
    size_t *out_num_samplers,
    size_t *out_num_uniforms);

/// @summary Retrieves descriptions of active vertex attributes, texture
/// samplers and uniform values defined in a shader program.
/// @param program The OpenGL program object to query.
/// @param buffer A temporary buffer used to hold attribute and uniform names.
/// @param buffer_size The maximum number of bytes that can be written to the
/// temporary name buffer.
/// @param include_builtins Specify true to include GLSL builtin values in the
/// returned vertex attribute count.
/// @param attrib_names Pointer to an array to be filled with the 32-bit hash
/// values of the active vertex attribute names.
/// @param attrib_info Pointer to an array to be filled with vertex attribute
/// descriptions.
/// @param sampler_names Pointer to an array to be filled with the 32-bit hash
/// values of the active texture sampler names.
/// @param sampler_info Pointer to an array to be filled with texture sampler
/// descriptions.
/// @param uniform_names Pointer to an array to be filled with the 32-bit hash
/// values of the active uniform names.
/// @param uniform_info Pointer to an array to be filled with uniform
/// descriptions.
LLOPENGL_PUBLIC void reflect_program_details(
    GLuint                program,
    char                 *buffer,
    size_t                buffer_size,
    bool                  include_builtins,
    uint32_t             *attrib_names,
    gl::attribute_desc_t *attrib_info,
    uint32_t             *sampler_names,
    gl::sampler_desc_t   *sampler_info,
    uint32_t             *uniform_names,
    gl::uniform_desc_t   *uniform_info);

/// @summary Binds a texture object to a texture sampler for the currently bound shader program.
/// @param sampler The description of the sampler to set.
/// @param texture The OpenGL texture object to bind to the sampler.
LLOPENGL_PUBLIC void set_sampler(gl::sampler_desc_t *sampler, GLuint texture);

/// @summary Sets a uniform value for the currently bound shader program.
/// @param uniform The description of the uniform to set.
/// @param value The data to copy to the uniform.
/// @param transpose For matrix values, specify true to transpose the matrix
/// elements before passing them to the shader program.
LLOPENGL_PUBLIC void set_uniform(gl::uniform_desc_t *uniform, void const *value, bool transpose);

/// @summary Initializes a shader source code buffer to empty.
/// @param source The source code buffer to clear.
LLOPENGL_PUBLIC void shader_source_init(gl::shader_source_t *source);

/// @summary Adds source code for a shader stage to a shader source buffer.
/// @param source The source code buffer to modify.
/// @param shader_stage The shader stage, for example, GL_VERTEX_SHADER.
/// @param source_code An array of NULL-terminated ASCII strings specifying the
/// source code fragments for the specified shader stage.
/// @param string_count The number of strings in the source_code array.
LLOPENGL_PUBLIC void shader_source_add(gl::shader_source_t *source, GLenum shader_stage, char **source_code, size_t string_count);

/// @summary Compiles, links and reflects a shader program.
/// @param source The shader source code buffer.
/// @param shader The shader program object to initialize.
/// @param out_program On return, this address is set to the identifier of the
/// OpenGL shader program object. If an error occurs, this value will be 0.
/// @return true if the build process was successful.
LLOPENGL_PUBLIC bool build_shader(gl::shader_source_t *source, gl::shader_desc_t *shader, GLuint *out_program);

/// @summary Given an OpenGL block-compressed internal format identifier,
/// determine the size of each compressed block, in pixels. For non block-
/// compressed formats, the block size is defined to be 1.
/// @param internal_format The OpenGL internal format value.
/// @return The dimension of a block, in pixels.
LLOPENGL_PUBLIC size_t block_dimension(GLenum internal_format);

/// @summary Given an OpenGL block-compressed internal format identifier,
/// determine the size of each compressed block of pixels.
/// @param internal_format The OpenGL internal format value. RGB/RGBA/SRGB and
/// SRGBA S3TC/DXT format identifiers are the only values currently accepted.
/// @return The number of bytes per compressed block of pixels, or zero.
LLOPENGL_PUBLIC size_t bytes_per_block(GLenum internal_format);

/// @summary Given an OpenGL internal format value, calculates the number of
/// bytes per-element (or per-block, for block-compressed formats).
/// @param internal_format The OpenGL internal format, for example, GL_RGBA.
/// @param data_type The OpenGL data type, for example, GL_UNSIGNED_BYTE.
/// @return The number of bytes per element (pixel or block), or zero.
LLOPENGL_PUBLIC size_t bytes_per_element(GLenum internal_format, GLenum data_type);

/// @summary Given an OpenGL internal_format value and a width, calculates the
/// number of bytes between rows in a 2D image slice.
/// @param internal_format The OpenGL internal format, for example, GL_RGBA.
/// @param data_type The OpenGL data type, for example, GL_UNSIGNED_BYTE.
/// @param width The row width, in pixels.
/// @param alignment The alignment requirement of the OpenGL implementation,
/// corresponding to the pname of GL_PACK_ALIGNMENT or GL_UNPACK_ALIGNMENT for
/// the glPixelStorei function. The specification default is 4.
/// @return The number of bytes per-row, or zero.
LLOPENGL_PUBLIC size_t bytes_per_row(GLenum internal_format, GLenum data_type, size_t width, size_t alignment);

/// @summary Calculates the size of the buffer required to store an image
/// with the specified attributes.
/// @param internal_format The OpenGL internal format value, for example,
/// GL_RGBA. The most common compressed formats are supported (DXT/S3TC).
/// @param data_type The data type identifier, for example, GL_UNSIGNED_BYTE.
/// @param width The width of the image, in pixels.
/// @param height The height of the image, in pixels.
/// @param alignment The alignment requirement of the OpenGL implementation,
/// corresponding to the pname of GL_PACK_ALIGNMENT or GL_UNPACK_ALIGNMENT for
/// the glPixelStorei function. The specification default is 4.
/// @return The number of bytes required to store the image data.
LLOPENGL_PUBLIC size_t bytes_per_slice(GLenum internal_format, GLenum data_type, size_t width, size_t height, size_t alignment);

/// @summary Calculates the dimension of an image (width, height, etc.) rounded
/// up to the next alignment boundary based on the internal format.
/// @summary internal_format The OpenGL internal format value, for example,
/// GL_RGBA. The most common compressed formats are supported (DXT/S3TC).
/// @param dimension The dimension value (width, height, etc.), in pixels.
/// @return The dimension value padded up to the next alignment boundary. The
/// returned value is always specified in pixels.
LLOPENGL_PUBLIC size_t image_dimension(GLenum internal_format, size_t dimension);

/// @summary Given an OpenGL internal format type value, determines the
/// corresponding base format value.
/// @param internal_format The OpenGL internal format value. See the
/// documentation for glTexImage2D(), internalFormat argument.
/// @return The OpenGL base internal format values. See the documentation for
/// glTexImage2D(), format argument.
LLOPENGL_PUBLIC GLenum base_format(GLenum internal_format);

/// @summary Given an OpenGL sampler type value, determines the corresponding
/// texture bind target identifier.
/// @param sampler_type The OpenGL sampler type, for example, GL_SAMPLER_2D.
/// @return The corresponding bind target, for example, GL_TEXTURE_2D.
LLOPENGL_PUBLIC GLenum texture_target(GLenum sampler_type);

/// @summary Computes the number of levels in a mipmap chain given the
/// dimensions of the highest resolution level.
/// @param width The width of the highest resolution level, in pixels.
/// @param height The height of the highest resolution level, in pixels.
/// @param slice_count The number of slices of the highest resolution level.
/// For everything except a 3D image, this value should be specified as 1.
/// @param max_levels The maximum number of levels in the mipmap chain. If
/// there is no limit, this value should be specified as 0.
LLOPENGL_PUBLIC size_t level_count(size_t width, size_t height, size_t slice_count, size_t max_levels);

/// @summary Computes the dimension (width, height or number of slices) of a
/// particular level in a mipmap chain given the dimension for the highest
/// resolution image.
/// @param dimension The dimension in the highest resolution image.
/// @param level_index The index of the target mip-level, with index 0
/// representing the highest resolution image.
/// @return The dimension in the specified mip level.
LLOPENGL_PUBLIC size_t level_dimension(size_t dimension, size_t level_index);

/// @summary Given basic image attributes, builds a complete description of
/// the levels in a mipmap chain.
/// @param internal_format The OpenGL internal format, for example GL_RGBA.
/// See the documentation for glTexImage2D(), internalFormat argument.
/// @param data_type The OpenGL data type, for example, GL_UNSIGNED_BYTE.
/// @param width The width of the highest resolution image, in pixels.
/// @param height The height of the highest resolution image, in pixels.
/// @param slice_count The number of slices in the highest resolution image.
/// For all image types other than 3D images, specify 1 for this value.
/// @param alignment The alignment requirement of the OpenGL implementation,
/// corresponding to the pname of GL_PACK_ALIGNMENT or GL_UNPACK_ALIGNMENT for
/// the glPixelStorei function. The specification default is 4.
/// @param max_levels The maximum number of levels in the mipmap chain. To
/// describe all possible levels, specify 0 for this value.
/// @param level_desc The array of level descriptors to populate.
LLOPENGL_PUBLIC void describe_mipmaps(
    GLenum            internal_format,
    GLenum            data_type,
    size_t            width,
    size_t            height,
    size_t            slice_count,
    size_t            alignment,
    size_t            max_levels,
    gl::level_desc_t *level_desc);

/// @summary Fills a memory buffer with a checkerboard pattern. This is useful
/// for indicating uninitialized textures and for testing. The image internal
/// format is expected to be GL_RGBA, data type GL_UNSIGNED_INT_8_8_8_8_REV,
/// and the data is written using the native system byte ordering (GL_BGRA).
/// @param width The image width, in pixels.
/// @param height The image height, in pixels.
/// @param alpha The value to write to the alpha channel, in [0, 1].
/// @param buffer The buffer to which image data will be written.
LLOPENGL_PUBLIC void checker_image(size_t width, size_t height, float alpha, void *buffer);

/// @summary Given basic texture attributes, allocates storage for all levels
/// of a texture, such that the texture is said to be complete. This should
/// only be performed once per-texture. After calling this function, the
/// texture object attributes should be considered immutable. Transfer data to
/// the texture using the transfer_pixels_h2d() function. The wrapping modes
/// are always set to GL_CLAMP_TO_EDGE. The caller is responsible for creating
/// and binding the texture object prior to calling this function.
/// @param target The OpenGL texture target, defining the texture type.
/// @param internal_format The OpenGL internal format, for example GL_RGBA.
/// @param data_type The OpenGL data type, for example, GL_UNSIGNED_BYTE.
/// @param min_filter The minification filter to use.
/// @param mag_filter The magnification filter to use.
/// @param width The width of the highest resolution image, in pixels.
/// @param height The height of the highest resolution image, in pixels.
/// @param slice_count The number of slices in the highest resolution image. If
/// the @a target argument specifies an array target, this represents the
/// number of items in the texture array. For 3D textures, it represents the
/// number of slices in the image. For all other types, this value must be 1.
/// @param max_levels The maximum number of levels in the mipmap chain. To
/// define all possible levels, specify 0 for this value.
LLOPENGL_PUBLIC void texture_storage(
    GLenum target,
    GLenum internal_format,
    GLenum data_type,
    GLenum min_filter,
    GLenum mag_filter,
    size_t width,
    size_t height,
    size_t slice_count,
    size_t max_levels);

/// @summary Copies pixel data from the device (GPU) to the host (CPU). The
/// pixel data consists of the framebuffer contents, or the contents of a
/// single mip-level of a texture image.
/// @param transfer An object describing the transfer operation to execute.
LLOPENGL_PUBLIC void transfer_pixels_d2h(gl::pixel_transfer_d2h_t *transfer);

/// @summary Copies pixel data from the host (CPU) to the device (GPU). The
/// pixel data is copied to a single mip-level of a texture image.
/// @param transfer An object describing the transfer operation to execute.
LLOPENGL_PUBLIC void transfer_pixels_h2d(gl::pixel_transfer_h2d_t *transfer);

/// @summary Initializes a sprite batch with the specified capacity.
/// @param batch The sprite batch.
/// @param capacity The initial capacity of the batch, in quads.
LLOPENGL_PUBLIC void create_sprite_batch(gl::sprite_batch_t *batch, size_t capacity);

/// @summary Frees the memory associated with a sprite batch.
/// @param batch The sprite batch to free.
LLOPENGL_PUBLIC void delete_sprite_batch(gl::sprite_batch_t *batch);

/// @summary Ensures that the sprite batch has at least the specified capacity.
/// @param batch The sprite batch.
LLOPENGL_PUBLIC void ensure_sprite_batch(gl::sprite_batch_t *batch, size_t capacity);

/// @summary Discards data buffered by a sprite batch.
/// @param batch The sprite batch to flush.
LLOPENGL_PUBLIC void flush_sprite_batch(gl::sprite_batch_t *batch);

/// @summary Transforms a set of sprite definitions into a series of quad definitions.
/// @param quads The buffer of quads to write to.
/// @param sdata The buffer of state data to write to.
/// @param indices The buffer of order indices to write to.
/// @param quad_offset The zero-based index of the first quad to write.
/// @param sprites The buffer of sprite definitions to read from.
/// @param sprite_offset The zero-based index of the first sprite to read.
/// @param count The number of sprite definitions to read.
LLOPENGL_PUBLIC void generate_quads(
    gl::sprite_quad_t      *quads,
    gl::sprite_sort_data_t *sdata,
    uint32_t               *indices,
    size_t                  quad_offset,
    gl::sprite_t const     *sprites,
    size_t                  sprite_offset,
    size_t                  sprite_count);

/// @summary Generates transformed position-texture-color vertex data for a set, or subset, of quads.
/// @param buffer The buffer to which vertex data will be written.
/// @param buffer_offset The offset into the buffer of the first vertex.
/// @param quads The buffer from which quad attributes will be read.
/// @param indices An array of zero-based indices specifying the order in which to read quads from the quad buffer.
/// @param quad_offset The offset into the quad list of the first quad.
/// @param quad_count The number of quads to generate.
LLOPENGL_PUBLIC void generate_quad_vertices_ptc(
    void                     *buffer,
    size_t                    buffer_offset,
    gl::sprite_quad_t const  *quads,
    uint32_t const           *indices,
    size_t                    quad_offset,
    size_t                    quad_count);

/// @summary Generates index data for a set, or subset, of quads. Triangles are
/// specified using counter-clockwise winding. Indices are 16-bit unsigned int.
/// @param buffer The destination buffer.
/// @param offset The offset into the buffer of the first index.
/// @param base_vertex The zero-based index of the first vertex of the batch.
/// This allows multiple batches to write into the same index buffer.
/// @param quad_count The number of quads being generated.
LLOPENGL_PUBLIC void generate_quad_indices_u16(void *buffer, size_t offset, size_t base_vertex, size_t quad_count);

/// @summary Generates index data for a set, or subset, of quads. Triangles are
/// specified using counter-clockwise winding. Indices are 32-bit unsigned int.
/// @param buffer The destination buffer.
/// @param offset The offset into the buffer of the first index.
/// @param base_vertex The zero-based index of the first vertex of the batch.
/// This allows multiple batches to write into the same index buffer.
/// @param quad_count The number of quads being generated.
LLOPENGL_PUBLIC void generate_quad_indices_u32(void *buffer, size_t offset, size_t base_vertex, size_t quad_count);

/// @summary Creates the GPU resources required to buffer and render quads.
/// @param effect The effect to initialize.
/// @param quad_count The maximum number of quads that can be buffered.
/// @param vertex_size The size of a single vertex, in bytes.
/// @param index_size The size of a single index, in bytes.
LLOPENGL_PUBLIC bool create_sprite_effect(gl::sprite_effect_t *effect, size_t quad_count, size_t vertex_size, size_t index_size);

/// @summary Releases the GPU resources used for buffering and rendering quads.
/// @param effect The effect to destroy.
LLOPENGL_PUBLIC void delete_sprite_effect(gl::sprite_effect_t *effect);

/// @summary Disables alpha blending for an effect. The state changes do not
/// take effect until the effect is (re)bound.
/// @param effect The effect to update.
LLOPENGL_PUBLIC void sprite_effect_blend_none(gl::sprite_effect_t *effect);

/// @summary Enables standard alpha blending (texture transparency) for an
/// effect. The state changes do not take effect until the effect is (re)bound.
/// @param effect The effect to update.
LLOPENGL_PUBLIC void sprite_effect_blend_alpha(gl::sprite_effect_t *effect);

/// @summary Enables additive alpha blending for an effect. The state changes
/// do not take effect until the effect is (re)bound.
/// @param effect The effect to update.
LLOPENGL_PUBLIC void sprite_effect_blend_additive(gl::sprite_effect_t *effect);

/// @summary Enables alpha blending with premultiplied alpha in the source texture.
/// The state changes do not take effect until the effect is (re)bound.
/// @param effect The effect to update.
LLOPENGL_PUBLIC void sprite_effect_blend_premultiplied(gl::sprite_effect_t *effect);

/// @summary Sets up the effect projection matrix for the given viewport.
/// @param effect The effect to update.
/// @param width The viewport width.
/// @param height The viewport height.
LLOPENGL_PUBLIC void sprite_effect_set_viewport(gl::sprite_effect_t *effect, int width, int height);

/// @summary Binds the vertex and index buffers of an effect for use in
/// subsequent rendering commands.
/// @param effect The effect being applied.
LLOPENGL_PUBLIC void sprite_effect_bind_buffers(gl::sprite_effect_t *effect);

/// @summary Applies the alpha blending state specified by the effect.
/// @param effect The effect being applied.
LLOPENGL_PUBLIC void sprite_effect_apply_blendstate(gl::sprite_effect_t *effect);

/// @summary Configures the Vertex Array Object for an effect using the standard
/// Position-TexCoord-Color layout configuration.
LLOPENGL_PUBLIC void sprite_effect_setup_vao_ptc(gl::sprite_effect_t *effect);

/// @summary Generates and uploads vertex and index data for a batch of quads
/// to the vertex and index buffers of an effect. The buffers act as circular
/// buffers. If the end of the buffers is reached, as much data as possible is
/// buffered, and the function returns.
/// @param effect The effect to update.
/// @param quads The source quad definitions.
/// @param indices An array of zero-based indices specifying the order in which to read quads from the quad buffer.
/// @param quad_offset The offset, in quads, of the first quad to read.
/// @param quad_count The number of quads to read.
/// @param base_index On return, this address is updated with the offset, in
/// indices, of the first buffered primitive written to the index buffer.
/// @return The number of quads actually buffered. May be less than @a count.
LLOPENGL_PUBLIC size_t sprite_effect_buffer_data_ptc(
    gl::sprite_effect_t     *effect,
    gl::sprite_quad_t const *quads,
    uint32_t const          *indices,
    size_t                   quad_offset,
    size_t                   quad_count,
    size_t                  *base_index);

/// @summary Renders an entire sprite batch with a given effect.
/// @param effect The effect being applied.
/// @param batch The sprite batch being rendered.
/// @param fxfuncs The effect-specific functions for applying render state.
/// @param context Opaque data defined by the application.
LLOPENGL_PUBLIC void sprite_effect_draw_batch_ptc(
    gl::sprite_effect_t             *effect,
    gl::sprite_batch_t              *batch,
    gl::sprite_effect_apply_t const *fxfuncs,
    void                            *context);

/// @summary Renders a portion of a sprite batch for which the vertex and index
/// data has already been buffered. This function is generally not called by
/// the user directly; it is called internally from sprite_effect_draw_batch_ptc().
/// @param effect The effect being applied.
/// @param batch The sprite batch being rendered.
/// @param quad_offset The index of the first quad in the batch to be rendered.
/// @param quad_count The number of quads to draw.
/// @param base_index The first index to read from the index buffer.
LLOPENGL_PUBLIC void sprite_effect_draw_batch_region_ptc(
    gl::sprite_effect_t             *effect,
    gl::sprite_batch_t              *batch,
    size_t                           quad_offset,
    size_t                           quad_count,
    size_t                           base_index,
    gl::sprite_effect_apply_t const *fxfuncs,
    void                            *context);

/// @summary Creates a shader program consisting of a vertex and fragment shader
/// used for rendering solid-colored 2D sprites in screen space.
/// @param shader The shader state to initialize.
/// @return true if the shader program was created successfully.
LLOPENGL_PUBLIC bool create_sprite_shader_ptc_clr(gl::sprite_shader_ptc_clr_t *shader);

/// @summary Frees the resources associated with a solid-color sprite shader.
/// @param shader The shader program object to free.
LLOPENGL_PUBLIC void delete_sprite_shader_ptc_clr(gl::sprite_shader_ptc_clr_t *shader);

/// @summary Creates a shader program consisting of a vertex and fragment shader
/// used for rendering standard 2D sprites in screen space. The shaders sample
/// image data from a single 2D texture object, and modulate the sample value
/// with the per-sprite tint color.
/// @param shader The shader state to initialize.
/// @return true if the shader program was created successfully.
LLOPENGL_PUBLIC bool create_sprite_shader_ptc_tex(gl::sprite_shader_ptc_tex_t *shader);

/// @summary Frees the resources associated with a textured sprite shader.
/// @param shader The shader program object to free.
LLOPENGL_PUBLIC void delete_sprite_shader_ptc_tex(gl::sprite_shader_ptc_tex_t *shader);

/*////////////////////////
//   Inline Functions   //
////////////////////////*/
/// @summary Searches a list of name-value pairs for a named item.
/// @param name_u32 The 32-bit unsigned integer hash of the search query.
/// @param name_list A list of 32-bit unsigned integer name hashes.
/// @param value_list A list of values, ordered such that name_list[i]
/// corresponds to value_list[i].
/// @param count The number of items in the name and value lists.
template <typename T>
static inline T* kv_find(uint32_t name_u32, uint32_t const *name_list, T *value_list, size_t count)
{
    for (size_t i = 0;  i < count; ++i)
    {
        if (name_list[i] == name_u32)
            return &value_list[i];
    }
    return NULL;
}

/// @summary Searches a list of name-value pairs for a named item.
/// @param name_str A NULL-terminated ASCII string specifying the search query.
/// @param name_list A list of 32-bit unsigned integer name hashes.
/// @param value_list A list of values, ordered such that name_list[i]
/// corresponds to value_list[i].
/// @param count The number of items in the name and value lists.
template <typename T>
static inline T* kv_find(char const *name_str, uint32_t const *name_list, T *value_list, size_t count)
{
    uint32_t name_u32 = gl::shader_name(name_str);
    return gl::kv_find(name_u32, name_list, value_list, count);
}

/// @summary Searches for a vertex attribute definition by name.
/// @param shader The shader program object to query.
/// @param name A NULL-terminated ASCII string vertex attribute identifier.
/// @return The corresponding vertex attribute definition, or NULL.
static inline gl::attribute_desc_t *find_attribute(gl::shader_desc_t *shader, char const *name)
{
    return gl::kv_find(name, shader->AttributeNames, shader->Attributes, shader->AttributeCount);
}

/// @summary Searches for a texture sampler definition by name.
/// @param shader The shader program object to query.
/// @param name A NULL-terminated ASCII string texture sampler identifier.
/// @return The corresponding texture sampler definition, or NULL.
static inline gl::sampler_desc_t* find_sampler(gl::shader_desc_t *shader, char const *name)
{
    return gl::kv_find(name, shader->SamplerNames, shader->Samplers, shader->SamplerCount);
}

/// @summary Searches for a uniform variable definition by name.
/// @param shader The shader program object to query.
/// @param name A NULL-terminated ASCII string uniform variable identifier.
/// @return The corresponding uniform variable definition, or NULL.
static inline gl::uniform_desc_t* find_uniform(gl::shader_desc_t *shader, char const *name)
{
    return gl::kv_find(name, shader->UniformNames, shader->Uniforms, shader->UniformCount);
}

/// @summary Sorts a sprite batch using std::sort(). The sort is indirect; the
/// order array is what gets sorted. The order array can then be used to read
/// quad definitions from the batch in sorted order.
/// @typename TComp A functor type (uint32_t index_a, uint32_t index_b).
/// @param batch The sprite batch to sort.
template <typename TComp>
static inline void sort_sprite_batch(gl::sprite_batch_t *batch)
{
    TComp cmp(batch);
    std::sort(batch->Order, batch->Order + batch->Count, cmp);
}

/// @summary Clamps a floating-point value into the given range.
/// @param x The value to clamp.
/// @param lower The inclusive lower-bound.
/// @param upper The inclusive upper-bound.
/// @return The value X: lower <= X <= upper.
static inline float clampf(float x, float lower, float upper)
{
    return (x < lower) ? lower : ((x > upper) ? upper : x);
}

/// @summary Converts an RGBA tuple into a packed 32-bit ABGR value.
/// @param rgba The RGBA tuple, with each component in [0, 1].
/// @return The color value packed into a 32-bit unsigned integer.
static inline uint32_t abgr32(float const *rgba)
{
    uint32_t r = (uint32_t) gl::clampf(rgba[0] * 255.0f, 0.0f, 255.0f);
    uint32_t g = (uint32_t) gl::clampf(rgba[1] * 255.0f, 0.0f, 255.0f);
    uint32_t b = (uint32_t) gl::clampf(rgba[2] * 255.0f, 0.0f, 255.0f);
    uint32_t a = (uint32_t) gl::clampf(rgba[3] * 255.0f, 0.0f, 255.0f);
    return ((a << 24) | (b << 16) | (g << 8) | r);
}

/// @summary Converts an RGBA value into a packed 32-bit ABGR value.
/// @param R The red channel value, in [0, 1].
/// @param G The green channel value, in [0, 1].
/// @param B The blue channel value, in [0, 1].
/// @param A The alpha channel value, in [0, 1].
/// @return The color value packed into a 32-bit unsigned integer.
static inline uint32_t abgr32(float R, float G, float B, float A)
{
    uint32_t r = (uint32_t) gl::clampf(R * 255.0f, 0.0f, 255.0f);
    uint32_t g = (uint32_t) gl::clampf(G * 255.0f, 0.0f, 255.0f);
    uint32_t b = (uint32_t) gl::clampf(B * 255.0f, 0.0f, 255.0f);
    uint32_t a = (uint32_t) gl::clampf(A * 255.0f, 0.0f, 255.0f);
    return ((a << 24) | (b << 16) | (g << 8) | r);
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace gl */

#endif /* LLOPENGL_HPP_INCLUDED */
