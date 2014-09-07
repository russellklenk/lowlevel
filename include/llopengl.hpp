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
/// @summary Defines the maximum number of shader stages. In OpenGL 3.3 Core,
/// we have stages GL_VERTEX_SHADER, GL_GEOMETRY_SHADER and GL_FRAGMENT_SHADER,
/// with extensions adding GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER.
#ifndef GL_MAX_SHADER_STAGES
#define GL_MAX_SHADER_STAGES  (5U)
#endif

/// @summary Macro to convert a byte offset into a pointer.
/// @param x The byte offset value.
/// @return The offset value, as a pointer.
#ifndef GL_BUFFER_OFFSET
#define GL_BUFFER_OFFSET(x)   ((GLvoid*)(((uint8_t*)NULL)+(x)))
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
LLOPENGL_PUBLIC bool build_shader(gl::shader_source_t *source, gl:shader_desc_t *shader, GLuint *out_program);

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

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace gl */

#endif /* LLOPENGL_HPP_INCLUDED */
