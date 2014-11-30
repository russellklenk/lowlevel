/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements some utility functions for working with the OpenGL API.
/// Note that this module does not handle setting up an OpenGL context, which
/// is best left to a third-party library like GLFW.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#ifdef  _MSC_VER
#include <intrin.h>
#endif
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "llopengl.hpp"

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary The vertex shader source code for rendering solid-colored quads
/// specified in screen-space.
static char const *SpriteShaderPTC_CLR_VSS =
    "#version 330\n"
    "uniform mat4 uMSS;\n"
    "layout (location = 0) in vec4 aPTX;\n"
    "layout (location = 1) in vec4 aCLR;\n"
    "out vec4 vCLR;\n"
    "void main() {\n"
    "    vCLR = aCLR;\n"
    "    gl_Position = uMSS * vec4(aPTX.x, aPTX.y, 0, 1);\n"
    "}\n";

/// @summary The fragment shader source code for rendering solid-colored quads
/// specified in screen-space.
static char const *SpriteShaderPTC_CLR_FSS =
    "#version 330\n"
    "in  vec4 vCLR;\n"
    "out vec4 oCLR;\n"
    "void main() {\n"
    "    oCLR = vCLR;\n"
    "}\n";

/// @summary The vertex shader source code for rendering textured and tinted
/// quads specified in screen-space.
static char const *SpriteShaderPTC_TEX_VSS =
    "#version 330\n"
    "uniform mat4 uMSS;\n"
    "layout (location = 0) in vec4 aPTX;\n"
    "layout (location = 1) in vec4 aCLR;\n"
    "out vec4 vCLR;\n"
    "out vec2 vTEX;\n"
    "void main() {\n"
    "    vCLR = aCLR;\n"
    "    vTEX = vec2(aPTX.z, aPTX.w);\n"
    "    gl_Position = uMSS * vec4(aPTX.x, aPTX.y, 0, 1);\n"
    "}\n";

/// @summary The fragment shader source code for rendering textured and tinted
/// quads specified in screen-space.
static char const *SpriteShaderPTC_TEX_FSS =
    "#version 330\n"
    "uniform sampler2D sTEX;\n"
    "in  vec2 vTEX;\n"
    "in  vec4 vCLR;\n"
    "out vec4 oCLR;\n"
    "void main() {\n"
    "    oCLR = texture(sTEX, vTEX) * vCLR;\n"
    "}\n";

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Rotates an unsigned 32-bit integer to the left, with wrapping.
/// @param x The value to rotate.
/// @param r The number of bits to rotate by.
/// @return The rotated value.
static inline uint32_t rotl32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}

/// @summary Rounds a value up to the nearest even multiple of a power of two.
/// @param size The size value to round.
/// @param pow2 The power of two value to round up to.
/// @return The nearest even multiple of the specified power of two.
static inline size_t align_up(size_t size, size_t pow2)
{
    return (size != 0) ? ((size + (pow2 - 1)) & ~(pow2 - 1)) : pow2;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
size_t gl::data_size(GLenum data_type)
{
    switch (data_type)
    {
    case GL_UNSIGNED_BYTE:               return sizeof(GLubyte);
    case GL_FLOAT:                       return sizeof(GLfloat);
    case GL_FLOAT_VEC2:                  return sizeof(GLfloat) *  2;
    case GL_FLOAT_VEC3:                  return sizeof(GLfloat) *  3;
    case GL_FLOAT_VEC4:                  return sizeof(GLfloat) *  4;
    case GL_INT:                         return sizeof(GLint);
    case GL_INT_VEC2:                    return sizeof(GLint)   *  2;
    case GL_INT_VEC3:                    return sizeof(GLint)   *  3;
    case GL_INT_VEC4:                    return sizeof(GLint)   *  4;
    case GL_BOOL:                        return sizeof(GLint);
    case GL_BOOL_VEC2:                   return sizeof(GLint)   *  2;
    case GL_BOOL_VEC3:                   return sizeof(GLint)   *  3;
    case GL_BOOL_VEC4:                   return sizeof(GLint)   *  4;
    case GL_FLOAT_MAT2:                  return sizeof(GLfloat) *  4;
    case GL_FLOAT_MAT3:                  return sizeof(GLfloat) *  9;
    case GL_FLOAT_MAT4:                  return sizeof(GLfloat) * 16;
    case GL_FLOAT_MAT2x3:                return sizeof(GLfloat) *  6;
    case GL_FLOAT_MAT2x4:                return sizeof(GLfloat) *  8;
    case GL_FLOAT_MAT3x2:                return sizeof(GLfloat) *  6;
    case GL_FLOAT_MAT3x4:                return sizeof(GLfloat) * 12;
    case GL_FLOAT_MAT4x2:                return sizeof(GLfloat) *  8;
    case GL_FLOAT_MAT4x3:                return sizeof(GLfloat) * 12;
    case GL_BYTE:                        return sizeof(GLbyte);
    case GL_UNSIGNED_SHORT:              return sizeof(GLushort);
    case GL_SHORT:                       return sizeof(GLshort);
    case GL_UNSIGNED_INT:                return sizeof(GLuint);
    case GL_UNSIGNED_SHORT_5_6_5:        return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_5_6_5_REV:    return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_4_4_4_4:      return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:  return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_5_5_5_1:      return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:  return sizeof(GLushort);
    case GL_UNSIGNED_INT_8_8_8_8:        return sizeof(GLubyte);
    case GL_UNSIGNED_INT_8_8_8_8_REV:    return sizeof(GLubyte);
    case GL_UNSIGNED_INT_10_10_10_2:     return sizeof(GLuint);
    case GL_UNSIGNED_INT_2_10_10_10_REV: return sizeof(GLuint);
    case GL_UNSIGNED_BYTE_3_3_2:         return sizeof(GLubyte);
    case GL_UNSIGNED_BYTE_2_3_3_REV:     return sizeof(GLubyte);
    default:                             break;
    }
    return 0;
}

uint32_t gl::shader_name(char const *name)
{
    #define HAS_NULL_BYTE(x) (((x) - 0x01010101) & (~(x) & 0x80808080))
    #ifdef  _MSC_VER
        #define ROTL32(x, y) _rotl((x), (y))
    #else
        #define ROTL32(x, y) rotl32((x), (y))
    #endif

    uint32_t hash = 0;
    if (name != NULL)
    {
        // hash the majority of the data in 4-byte chunks.
        while (!HAS_NULL_BYTE(*((uint32_t*)name)))
        {
            hash  = ROTL32(hash, 7) + name[0];
            hash  = ROTL32(hash, 7) + name[1];
            hash  = ROTL32(hash, 7) + name[2];
            hash  = ROTL32(hash, 7) + name[3];
            name += 4;
        }
        // hash the remaining 0-3 bytes.
        while (*name) hash = ROTL32(hash, 7) + *name++;
    }
    #undef HAS_NULL_BYTE
    #ifdef _MSC_VER
        #undef ROTL32
    #endif
    return hash;
}

bool gl::builtin(char const *name)
{
    char prefix[4] = {'g','l','_','\0'};
    return (strncmp(name, prefix, 3) == 0);
}

bool gl::compile_shader(
    GLenum   shader_type,
    char   **shader_source,
    size_t   string_count,
    GLuint  *out_shader,
    size_t  *out_log_size)
{
    GLuint  shader   = 0;
    GLsizei log_size = 0;
    GLint   result   = GL_FALSE;

    shader = glCreateShader(shader_type);
    if (0 == shader)
    {
        if (out_shader)   *out_shader   = 0;
        if (out_log_size) *out_log_size = 1;
        return false;
    }
    glShaderSource (shader, GLsizei(string_count), (GLchar const**) shader_source, NULL);
    glCompileShader(shader);
    glGetShaderiv  (shader, GL_COMPILE_STATUS,  &result);
    glGetShaderiv  (shader, GL_INFO_LOG_LENGTH, &log_size);
    glGetError();

    if (out_shader)   *out_shader   = shader;
    if (out_log_size) *out_log_size = log_size + 1;
    return (result == GL_TRUE);
}

void gl::copy_compile_log(GLuint shader, char *buffer, size_t buffer_size)
{
    GLsizei len = 0;
    glGetShaderInfoLog(shader, (GLsizei) buffer_size, &len, buffer);
    buffer[len] = '\0';
}

bool gl::attach_shaders(GLuint *shader_list, size_t shader_count, GLuint *out_program)
{
    GLuint  program = 0;
    program = glCreateProgram();
    if (0  == program)
    {
        if (out_program) *out_program = 0;
        return false;
    }

    for (size_t i = 0; i < shader_count;  ++i)
    {
        glAttachShader(program, shader_list[i]);
        if (glGetError() != GL_NO_ERROR)
        {
            glDeleteProgram(program);
            if (out_program) *out_program = 0;
            return false;
        }
    }
    if (out_program) *out_program = program;
    return true;
}

bool gl::assign_vertex_attributes(GLuint program, char const **attrib_names, GLuint *attrib_locations, size_t attrib_count)
{
    bool result   = true;
    for (size_t i = 0; i < attrib_count; ++i)
    {
        glBindAttribLocation(program, attrib_locations[i], attrib_names[i]);
        if (glGetError() != GL_NO_ERROR)
            result = false;
    }
    return result;
}

bool gl::assign_fragment_outputs(GLuint program, char const **output_names, GLuint *output_locations, size_t output_count)
{
    bool result   = true;
    for (size_t i = 0; i < output_count; ++i)
    {
        glBindFragDataLocation(program, output_locations[i], output_names[i]);
        if (glGetError() != GL_NO_ERROR)
            result = false;
    }
    return result;
}

bool gl::link_program(GLuint program, size_t *out_max_name, size_t *out_log_size)
{
    GLsizei log_size = 0;
    GLint   result   = GL_FALSE;

    glLinkProgram (program);
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);
    if (out_log_size) *out_log_size = log_size;

    if (result == GL_TRUE)
    {
        GLint a_max = 0;
        GLint u_max = 0;
        GLint n_max = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &u_max);
        glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &a_max);
        n_max = u_max > a_max ? u_max : a_max;
        if (out_max_name)  *out_max_name = (size_t)(n_max + 1);
    }
    else if (out_max_name) *out_max_name = 1;
    return  (result == GL_TRUE);
}

void gl::copy_linker_log(GLuint program, char *buffer, size_t buffer_size)
{
    GLsizei len = 0;
    glGetProgramInfoLog(program, (GLsizei) buffer_size, &len, buffer);
    buffer[len] = '\0';
}

bool gl::shader_desc_alloc(gl::shader_desc_t *desc, size_t num_attribs, size_t num_samplers, size_t num_uniforms)
{
    gl::attribute_desc_t *attribs       = NULL;
    gl::sampler_desc_t   *samplers      = NULL;
    gl::uniform_desc_t   *uniforms      = NULL;
    uint32_t             *attrib_names  = NULL;
    uint32_t             *sampler_names = NULL;
    uint32_t             *uniform_names = NULL;
    uint8_t              *memory_block  = NULL;
    uint8_t              *memory_ptr    = NULL;
    size_t                aname_size    = 0;
    size_t                sname_size    = 0;
    size_t                uname_size    = 0;
    size_t                attrib_size   = 0;
    size_t                sampler_size  = 0;
    size_t                uniform_size  = 0;
    size_t                total_size    = 0;

    // calculate the total size of all shader program metadata.
    aname_size    = sizeof(uint32_t)           * num_attribs;
    sname_size    = sizeof(uint32_t)           * num_samplers;
    uname_size    = sizeof(uint32_t)           * num_uniforms;
    attrib_size   = sizeof(attribute_desc_t)   * num_attribs;
    sampler_size  = sizeof(sampler_desc_t)     * num_samplers;
    uniform_size  = sizeof(uniform_desc_t)     * num_uniforms;
    total_size    = aname_size  + sname_size   + uname_size +
                    attrib_size + sampler_size + uniform_size;

    // perform a single large memory allocation for the metadata.
    memory_block  = (uint8_t*) malloc(total_size);
    memory_ptr    = memory_block;
    if (memory_block == NULL)
        return false;

    // assign pointers to various sub-blocks within the larger memory block.
    attrib_names  = (uint32_t*) memory_ptr;
    memory_ptr   +=  aname_size;
    attribs       = (gl::attribute_desc_t*) memory_ptr;
    memory_ptr   +=  attrib_size;

    sampler_names = (uint32_t*) memory_ptr;
    memory_ptr   +=  sname_size;
    samplers      = (gl::sampler_desc_t*) memory_ptr;
    memory_ptr   +=  sampler_size;

    uniform_names = (uint32_t*) memory_ptr;
    memory_ptr   +=  uname_size;
    uniforms      = (gl::uniform_desc_t*) memory_ptr;
    memory_ptr   +=  uniform_size;

    // set all of the fields on the shader_desc_t structure.
    desc->UniformCount   = num_uniforms;
    desc->UniformNames   = uniform_names;
    desc->Uniforms       = uniforms;
    desc->AttributeCount = num_attribs;
    desc->AttributeNames = attrib_names;
    desc->Attributes     = attribs;
    desc->SamplerCount   = num_samplers;
    desc->SamplerNames   = sampler_names;
    desc->Samplers       = samplers;
    desc->Metadata       = memory_block;
    return true;
}

void gl::shader_desc_free(gl::shader_desc_t *desc)
{
    if (desc->Metadata != NULL)
    {
        free(desc->Metadata);
        desc->Metadata       = NULL;
        desc->UniformCount   = 0;
        desc->UniformNames   = NULL;
        desc->Uniforms       = NULL;
        desc->AttributeCount = 0;
        desc->AttributeNames = NULL;
        desc->Attributes     = NULL;
        desc->SamplerCount   = 0;
        desc->SamplerNames   = NULL;
        desc->Samplers       = NULL;
    }
}

void gl::reflect_program_counts(
    GLuint  program,
    char   *buffer,
    size_t  buffer_size,
    bool    include_builtins,
    size_t *out_num_attribs,
    size_t *out_num_samplers,
    size_t *out_num_uniforms)
{
    size_t  num_attribs  = 0;
    GLint   attrib_count = 0;
    GLsizei buf_size     = (GLsizei) buffer_size;

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attrib_count);
    for (GLint i = 0; i < attrib_count; ++i)
    {
        GLenum type = GL_FLOAT;
        GLuint idx  = (GLuint) i;
        GLint  len  = 0;
        GLint  sz   = 0;
        glGetActiveAttrib(program, idx, buf_size, &len, &sz, &type, buffer);
        if (gl::builtin(buffer) && !include_builtins)
            continue;
        num_attribs++;
    }

    size_t num_samplers  = 0;
    size_t num_uniforms  = 0;
    GLint  uniform_count = 0;

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);
    for (GLint i = 0; i < uniform_count; ++i)
    {
        GLenum type = GL_FLOAT;
        GLuint idx  = (GLuint) i;
        GLint  len  = 0;
        GLint  sz   = 0;
        glGetActiveUniform(program, idx, buf_size, &len, &sz, &type, buffer);
        if (gl::builtin(buffer) && !include_builtins)
            continue;

        switch (type)
        {
            case GL_SAMPLER_1D:
            case GL_INT_SAMPLER_1D:
            case GL_UNSIGNED_INT_SAMPLER_1D:
            case GL_SAMPLER_1D_SHADOW:
            case GL_SAMPLER_2D:
            case GL_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_3D:
            case GL_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_3D:
            case GL_SAMPLER_CUBE:
            case GL_INT_SAMPLER_CUBE:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:
            case GL_SAMPLER_CUBE_SHADOW:
            case GL_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_1D_ARRAY_SHADOW:
            case GL_INT_SAMPLER_1D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_2D_ARRAY_SHADOW:
            case GL_INT_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_BUFFER:
            case GL_INT_SAMPLER_BUFFER:
            case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            case GL_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_RECT_SHADOW:
            case GL_INT_SAMPLER_2D_RECT:
            case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_MULTISAMPLE:
            case GL_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
                num_samplers++;
                break;

            default:
                num_uniforms++;
                break;
        }
    }

    if (out_num_attribs)  *out_num_attribs  = num_attribs;
    if (out_num_samplers) *out_num_samplers = num_samplers;
    if (out_num_uniforms) *out_num_uniforms = num_uniforms;
}

void gl::reflect_program_details(
    GLuint                program,
    char                 *buffer,
    size_t                buffer_size,
    bool                  include_builtins,
    uint32_t             *attrib_names,
    gl::attribute_desc_t *attrib_info,
    uint32_t             *sampler_names,
    gl::sampler_desc_t   *sampler_info,
    uint32_t             *uniform_names,
    gl::uniform_desc_t   *uniform_info)
{
    size_t  num_attribs  = 0;
    GLint   attrib_count = 0;
    GLsizei buf_size     = (GLsizei) buffer_size;

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attrib_count);
    for (GLint i = 0; i < attrib_count; ++i)
    {
        GLenum type = GL_FLOAT;
        GLuint idx  = (GLuint) i;
        GLint  len  = 0;
        GLint  loc  = 0;
        GLint  sz   = 0;
        glGetActiveAttrib(program, idx, buf_size, &len, &sz, &type, buffer);
        if (gl::builtin(buffer) && !include_builtins)
            continue;

        attribute_desc_t va;
        loc           = glGetAttribLocation(program, buffer);
        va.DataType   = (GLenum) type;
        va.Location   = (GLint)  loc;
        va.DataSize   = (size_t) gl::data_size(type) * sz;
        va.DataOffset = (size_t) 0; // for application use only
        va.Dimension  = (size_t) sz;
        attrib_names[num_attribs] = gl::shader_name(buffer);
        attrib_info [num_attribs] = va;
        num_attribs++;
    }

    size_t num_samplers  = 0;
    size_t num_uniforms  = 0;
    GLint  uniform_count = 0;
    GLint  texture_unit  = 0;

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);
    for (GLint i = 0; i < uniform_count; ++i)
    {
        GLenum type = GL_FLOAT;
        GLuint idx  = (GLuint) i;
        GLint  len  = 0;
        GLint  loc  = 0;
        GLint  sz   = 0;
        glGetActiveUniform(program, idx, buf_size, &len, &sz, &type, buffer);
        if (gl::builtin(buffer) && !include_builtins)
            continue;

        switch (type)
        {
            case GL_SAMPLER_1D:
            case GL_INT_SAMPLER_1D:
            case GL_UNSIGNED_INT_SAMPLER_1D:
            case GL_SAMPLER_1D_SHADOW:
            case GL_SAMPLER_2D:
            case GL_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_3D:
            case GL_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_3D:
            case GL_SAMPLER_CUBE:
            case GL_INT_SAMPLER_CUBE:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:
            case GL_SAMPLER_CUBE_SHADOW:
            case GL_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_1D_ARRAY_SHADOW:
            case GL_INT_SAMPLER_1D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_2D_ARRAY_SHADOW:
            case GL_INT_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_BUFFER:
            case GL_INT_SAMPLER_BUFFER:
            case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            case GL_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_RECT_SHADOW:
            case GL_INT_SAMPLER_2D_RECT:
            case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_MULTISAMPLE:
            case GL_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
                {
                    sampler_desc_t ts;
                    loc            = glGetUniformLocation(program, buffer);
                    ts.SamplerType = (GLenum) type;
                    ts.BindTarget  = (GLenum) gl::texture_target(type);
                    ts.Location    = (GLint)  loc;
                    ts.ImageUnit   = (GLint)  texture_unit++;
                    sampler_names[num_samplers] = gl::shader_name(buffer);
                    sampler_info [num_samplers] = ts;
                    num_samplers++;
                }
                break;

            default:
                {
                    uniform_desc_t uv;
                    loc            = glGetUniformLocation(program, buffer);
                    uv.DataType    = (GLenum) type;
                    uv.Location    = (GLint)  loc;
                    uv.DataSize    = (size_t) gl::data_size(type) * sz;
                    uv.DataOffset  = (size_t) 0; // for application use only
                    uv.Dimension   = (size_t) sz;
                    uniform_names[num_uniforms] = gl::shader_name(buffer);
                    uniform_info [num_uniforms] = uv;
                    num_uniforms++;
                }
                break;
        }
    }
}

void gl::set_sampler(gl::sampler_desc_t *sampler, GLuint texture)
{
    glActiveTexture(GL_TEXTURE0 + sampler->ImageUnit);
    glBindTexture(sampler->BindTarget, texture);
    glUniform1i(sampler->Location, sampler->ImageUnit);
}

void gl::set_uniform(gl::uniform_desc_t *uniform, void const *value, bool transpose)
{
    GLint          loc = uniform->Location;
    GLsizei        dim = uniform->Dimension;
    GLboolean      tm  = transpose ? GL_TRUE : GL_FALSE;
    GLint const   *id  = (GLint const*)   value;
    GLfloat const *fd  = (GLfloat const*) value;
    switch (uniform->DataType)
    {
        case GL_FLOAT:        glUniform1fv(loc, dim, fd);             break;
        case GL_FLOAT_VEC2:   glUniform2fv(loc, dim, fd);             break;
        case GL_FLOAT_VEC3:   glUniform3fv(loc, dim, fd);             break;
        case GL_FLOAT_VEC4:   glUniform4fv(loc, dim, fd);             break;
        case GL_INT:          glUniform1iv(loc, dim, id);             break;
        case GL_INT_VEC2:     glUniform2iv(loc, dim, id);             break;
        case GL_INT_VEC3:     glUniform3iv(loc, dim, id);             break;
        case GL_INT_VEC4:     glUniform4iv(loc, dim, id);             break;
        case GL_BOOL:         glUniform1iv(loc, dim, id);             break;
        case GL_BOOL_VEC2:    glUniform2iv(loc, dim, id);             break;
        case GL_BOOL_VEC3:    glUniform3iv(loc, dim, id);             break;
        case GL_BOOL_VEC4:    glUniform4iv(loc, dim, id);             break;
        case GL_FLOAT_MAT2:   glUniformMatrix2fv  (loc, dim, tm, fd); break;
        case GL_FLOAT_MAT3:   glUniformMatrix3fv  (loc, dim, tm, fd); break;
        case GL_FLOAT_MAT4:   glUniformMatrix4fv  (loc, dim, tm, fd); break;
        case GL_FLOAT_MAT2x3: glUniformMatrix2x3fv(loc, dim, tm, fd); break;
        case GL_FLOAT_MAT2x4: glUniformMatrix2x4fv(loc, dim, tm, fd); break;
        case GL_FLOAT_MAT3x2: glUniformMatrix3x2fv(loc, dim, tm, fd); break;
        case GL_FLOAT_MAT3x4: glUniformMatrix3x4fv(loc, dim, tm, fd); break;
        case GL_FLOAT_MAT4x2: glUniformMatrix4x2fv(loc, dim, tm, fd); break;
        case GL_FLOAT_MAT4x3: glUniformMatrix4x3fv(loc, dim, tm, fd); break;
        default: break;
    }
}

void gl::shader_source_init(gl::shader_source_t *source)
{
    source->StageCount = 0;
    for (size_t i = 0; i < GL_MAX_SHADER_STAGES; ++i)
    {
        source->StageNames [i] = 0;
        source->StringCount[i] = 0;
        source->SourceCode [i] = NULL;
    }
}

void gl::shader_source_add(gl::shader_source_t *source, GLenum shader_stage, char **source_code, size_t string_count)
{
    if (source->StageCount < GL_MAX_SHADER_STAGES)
    {
        source->StageNames [source->StageCount] = shader_stage;
        source->StringCount[source->StageCount] = GLsizei(string_count);
        source->SourceCode [source->StageCount] = source_code;
        source->StageCount++;
    }
}

bool gl::build_shader(gl::shader_source_t *source, gl::shader_desc_t *shader, GLuint *out_program)
{
    GLuint shader_list[GL_MAX_SHADER_STAGES];
    GLuint program       = 0;
    char  *name_buffer   = NULL;
    size_t num_attribs   = 0;
    size_t num_samplers  = 0;
    size_t num_uniforms  = 0;
    size_t max_name      = 0;

    for (size_t  i = 0; i < source->StageCount;  ++i)
    {
        size_t  ls = 0;
        GLenum  sn = source->StageNames [i];
        GLsizei fc = source->StringCount[i];
        char  **sc = source->SourceCode [i];
        if (!gl::compile_shader(sn, sc, fc, &shader_list[i], &ls))
            goto error_cleanup;
    }

    if (!gl::attach_shaders(shader_list, source->StageCount, &program))
        goto error_cleanup;

    if (!gl::link_program(program, &max_name, NULL))
        goto error_cleanup;

    // flag each attached shader for deletion when the program is deleted.
    // the shaders are automatically detached when the program is deleted.
    for (size_t i = 0; i < source->StageCount; ++i)
        glDeleteShader(shader_list[i]);

    // figure out how many attributes, samplers and uniforms we have.
    name_buffer = (char*) malloc(max_name);
    gl::reflect_program_counts(program, name_buffer, max_name,
        false, &num_attribs, &num_samplers, &num_uniforms);

    if (!gl::shader_desc_alloc(shader, num_attribs, num_samplers, num_uniforms))
        goto error_cleanup;

    // now reflect the shader program to retrieve detailed information
    // about all vertex attributes, texture samplers and uniform variables.
    gl::reflect_program_details(program, name_buffer, max_name, false,
        shader->AttributeNames, shader->Attributes,
        shader->SamplerNames,   shader->Samplers,
        shader->UniformNames,   shader->Uniforms);

    *out_program = program;
    return true;

error_cleanup:
    for (size_t i = 0; i < source->StageCount; ++i)
    {
        if (shader_list[i] != 0)
        {
            glDeleteShader(shader_list[i]);
        }
    }
    if (shader->Metadata != NULL) gl::shader_desc_free(shader);
    if (name_buffer  != NULL)     free(name_buffer);
    if (program != 0)             glDeleteProgram(program);
    *out_program = 0;
    return false;
}

size_t gl::block_dimension(GLenum internal_format)
{
    switch (internal_format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:        return 4;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:       return 4;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:       return 4;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:       return 4;
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:       return 4;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return 4;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return 4;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return 4;
        default:
            break;
    }
    return 1;
}

size_t gl::bytes_per_block(GLenum internal_format)
{
    switch (internal_format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:        return 8;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:       return 8;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:       return 16;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:       return 16;
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:       return 8;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return 8;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return 16;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return 16;
        default:
            break;
    }
    return 0;
}

size_t gl::bytes_per_element(GLenum internal_format, GLenum data_type)
{
    switch (internal_format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
        case GL_RED:
        case GL_R8:
        case GL_R8_SNORM:
        case GL_R16:
        case GL_R16_SNORM:
        case GL_R16F:
        case GL_R32F:
        case GL_R8I:
        case GL_R8UI:
        case GL_R16I:
        case GL_R16UI:
        case GL_R32I:
        case GL_R32UI:
        case GL_R3_G3_B2:
        case GL_RGB4:
        case GL_RGB5:
        case GL_RGB10:
        case GL_RGB12:
        case GL_RGBA2:
        case GL_RGBA4:
        case GL_RGB9_E5:
        case GL_R11F_G11F_B10F:
        case GL_RGB5_A1:
        case GL_RGB10_A2:
        case GL_RGB10_A2UI:
            return (gl::data_size(data_type) * 1);

        case GL_RG:
        case GL_RG8:
        case GL_RG8_SNORM:
        case GL_RG16:
        case GL_RG16_SNORM:
        case GL_RG16F:
        case GL_RG32F:
        case GL_RG8I:
        case GL_RG8UI:
        case GL_RG16I:
        case GL_RG16UI:
        case GL_RG32I:
        case GL_RG32UI:
            return (gl::data_size(data_type) * 2);

        case GL_RGB:
        case GL_RGB8:
        case GL_RGB8_SNORM:
        case GL_RGB16_SNORM:
        case GL_SRGB8:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_RGB8I:
        case GL_RGB8UI:
        case GL_RGB16I:
        case GL_RGB16UI:
        case GL_RGB32I:
        case GL_RGB32UI:
            return (gl::data_size(data_type) * 3);

        case GL_RGBA:
        case GL_RGBA8:
        case GL_RGBA8_SNORM:
        case GL_SRGB8_ALPHA8:
        case GL_RGBA16F:
        case GL_RGBA32F:
        case GL_RGBA8I:
        case GL_RGBA8UI:
        case GL_RGBA16I:
        case GL_RGBA16UI:
        case GL_RGBA32I:
        case GL_RGBA32UI:
            return (gl::data_size(data_type) * 4);

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return gl::bytes_per_block(internal_format);

        default:
            break;
    }
    return 0;
}

size_t gl::bytes_per_row(GLenum internal_format, GLenum data_type, size_t width, size_t alignment)
{
    if (width == 0)  width = 1;
    switch (internal_format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
        case GL_RED:
        case GL_R8:
        case GL_R8_SNORM:
        case GL_R16:
        case GL_R16_SNORM:
        case GL_R16F:
        case GL_R32F:
        case GL_R8I:
        case GL_R8UI:
        case GL_R16I:
        case GL_R16UI:
        case GL_R32I:
        case GL_R32UI:
        case GL_R3_G3_B2:
        case GL_RGB4:
        case GL_RGB5:
        case GL_RGB10:
        case GL_RGB12:
        case GL_RGBA2:
        case GL_RGBA4:
        case GL_RGB9_E5:
        case GL_R11F_G11F_B10F:
        case GL_RGB5_A1:
        case GL_RGB10_A2:
        case GL_RGB10_A2UI:
            return align_up(width * gl::data_size(data_type), alignment);

        case GL_RG:
        case GL_RG8:
        case GL_RG8_SNORM:
        case GL_RG16:
        case GL_RG16_SNORM:
        case GL_RG16F:
        case GL_RG32F:
        case GL_RG8I:
        case GL_RG8UI:
        case GL_RG16I:
        case GL_RG16UI:
        case GL_RG32I:
        case GL_RG32UI:
            return align_up(width * gl::data_size(data_type) * 2, alignment);

        case GL_RGB:
        case GL_RGB8:
        case GL_RGB8_SNORM:
        case GL_RGB16_SNORM:
        case GL_SRGB8:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_RGB8I:
        case GL_RGB8UI:
        case GL_RGB16I:
        case GL_RGB16UI:
        case GL_RGB32I:
        case GL_RGB32UI:
            return align_up(width * gl::data_size(data_type) * 3, alignment);

        case GL_RGBA:
        case GL_RGBA8:
        case GL_RGBA8_SNORM:
        case GL_SRGB8_ALPHA8:
        case GL_RGBA16F:
        case GL_RGBA32F:
        case GL_RGBA8I:
        case GL_RGBA8UI:
        case GL_RGBA16I:
        case GL_RGBA16UI:
        case GL_RGBA32I:
        case GL_RGBA32UI:
            return align_up(width * gl::data_size(data_type) * 4, alignment);

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return align_up(((width + 3) >> 2) * gl::bytes_per_block(internal_format), alignment);

        default:
            break;
    }
    return 0;
}

size_t gl::bytes_per_slice(
    GLenum internal_format,
    GLenum data_type,
    size_t width,
    size_t height,
    size_t alignment)
{
    if (width  == 0) width  = 1;
    if (height == 0) height = 1;
    switch (internal_format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
        case GL_RED:
        case GL_R8:
        case GL_R8_SNORM:
        case GL_R16:
        case GL_R16_SNORM:
        case GL_R16F:
        case GL_R32F:
        case GL_R8I:
        case GL_R8UI:
        case GL_R16I:
        case GL_R16UI:
        case GL_R32I:
        case GL_R32UI:
        case GL_R3_G3_B2:
        case GL_RGB4:
        case GL_RGB5:
        case GL_RGB10:
        case GL_RGB12:
        case GL_RGBA2:
        case GL_RGBA4:
        case GL_RGB9_E5:
        case GL_R11F_G11F_B10F:
        case GL_RGB5_A1:
        case GL_RGB10_A2:
        case GL_RGB10_A2UI:
            return align_up(width * gl::data_size(data_type), alignment) * height;

        case GL_RG:
        case GL_RG8:
        case GL_RG8_SNORM:
        case GL_RG16:
        case GL_RG16_SNORM:
        case GL_RG16F:
        case GL_RG32F:
        case GL_RG8I:
        case GL_RG8UI:
        case GL_RG16I:
        case GL_RG16UI:
        case GL_RG32I:
        case GL_RG32UI:
            return align_up(width * gl::data_size(data_type) * 2, alignment) * height;

        case GL_RGB:
        case GL_RGB8:
        case GL_RGB8_SNORM:
        case GL_RGB16_SNORM:
        case GL_SRGB8:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_RGB8I:
        case GL_RGB8UI:
        case GL_RGB16I:
        case GL_RGB16UI:
        case GL_RGB32I:
        case GL_RGB32UI:
            return align_up(width * gl::data_size(data_type) * 3, alignment) * height;

        case GL_RGBA:
        case GL_RGBA8:
        case GL_RGBA8_SNORM:
        case GL_SRGB8_ALPHA8:
        case GL_RGBA16F:
        case GL_RGBA32F:
        case GL_RGBA8I:
        case GL_RGBA8UI:
        case GL_RGBA16I:
        case GL_RGBA16UI:
        case GL_RGBA32I:
        case GL_RGBA32UI:
            return align_up(width * gl::data_size(data_type) * 4, alignment) * height;

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            // these formats operate on 4x4 blocks of pixels, so if a dimension
            // is not evenly divisible by four, it needs to be rounded up.
            return align_up(((width + 3) >> 2) * gl::bytes_per_block(internal_format), alignment) * ((height + 3) >> 2);

        default:
            break;
    }
    return 0;
}

size_t gl::image_dimension(GLenum internal_format, size_t dimension)
{
    switch (internal_format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            // these formats operate on 4x4 blocks of pixels, so if a dimension
            // is not evenly divisible by four, it needs to be rounded up.
            return (((dimension + 3) >> 2) * gl::block_dimension(internal_format));

        default:
            break;
    }
    return dimension;
}

GLenum gl::pixel_layout(GLenum internal_format)
{
    switch (internal_format)
    {
        case GL_DEPTH_COMPONENT:                     return GL_DEPTH_COMPONENT;
        case GL_DEPTH_STENCIL:                       return GL_DEPTH_STENCIL;
        case GL_RED:                                 return GL_RED;
        case GL_RG:                                  return GL_RG;
        case GL_RGB:                                 return GL_RGB;
        case GL_RGBA:                                return GL_BGRA;
        case GL_R8:                                  return GL_RED;
        case GL_R8_SNORM:                            return GL_RED;
        case GL_R16:                                 return GL_RED;
        case GL_R16_SNORM:                           return GL_RED;
        case GL_RG8:                                 return GL_RG;
        case GL_RG8_SNORM:                           return GL_RG;
        case GL_RG16:                                return GL_RG;
        case GL_RG16_SNORM:                          return GL_RG;
        case GL_R3_G3_B2:                            return GL_RGB;
        case GL_RGB4:                                return GL_RGB;
        case GL_RGB5:                                return GL_RGB;
        case GL_RGB8:                                return GL_RGB;
        case GL_RGB8_SNORM:                          return GL_RGB;
        case GL_RGB10:                               return GL_RGB;
        case GL_RGB12:                               return GL_RGB;
        case GL_RGB16_SNORM:                         return GL_RGB;
        case GL_RGBA2:                               return GL_RGB;
        case GL_RGBA4:                               return GL_RGB;
        case GL_RGB5_A1:                             return GL_RGBA;
        case GL_RGBA8:                               return GL_BGRA;
        case GL_RGBA8_SNORM:                         return GL_BGRA;
        case GL_RGB10_A2:                            return GL_RGBA;
        case GL_RGB10_A2UI:                          return GL_RGBA;
        case GL_RGBA12:                              return GL_RGBA;
        case GL_RGBA16:                              return GL_BGRA;
        case GL_SRGB8:                               return GL_RGB;
        case GL_SRGB8_ALPHA8:                        return GL_BGRA;
        case GL_R16F:                                return GL_RED;
        case GL_RG16F:                               return GL_RG;
        case GL_RGB16F:                              return GL_RGB;
        case GL_RGBA16F:                             return GL_BGRA;
        case GL_R32F:                                return GL_RED;
        case GL_RG32F:                               return GL_RG;
        case GL_RGB32F:                              return GL_RGB;
        case GL_RGBA32F:                             return GL_BGRA;
        case GL_R11F_G11F_B10F:                      return GL_RGB;
        case GL_RGB9_E5:                             return GL_RGB;
        case GL_R8I:                                 return GL_RED;
        case GL_R8UI:                                return GL_RED;
        case GL_R16I:                                return GL_RED;
        case GL_R16UI:                               return GL_RED;
        case GL_R32I:                                return GL_RED;
        case GL_R32UI:                               return GL_RED;
        case GL_RG8I:                                return GL_RG;
        case GL_RG8UI:                               return GL_RG;
        case GL_RG16I:                               return GL_RG;
        case GL_RG16UI:                              return GL_RG;
        case GL_RG32I:                               return GL_RG;
        case GL_RG32UI:                              return GL_RG;
        case GL_RGB8I:                               return GL_RGB;
        case GL_RGB8UI:                              return GL_RGB;
        case GL_RGB16I:                              return GL_RGB;
        case GL_RGB16UI:                             return GL_RGB;
        case GL_RGB32I:                              return GL_RGB;
        case GL_RGB32UI:                             return GL_RGB;
        case GL_RGBA8I:                              return GL_BGRA;
        case GL_RGBA8UI:                             return GL_BGRA;
        case GL_RGBA16I:                             return GL_BGRA;
        case GL_RGBA16UI:                            return GL_BGRA;
        case GL_RGBA32I:                             return GL_BGRA;
        case GL_RGBA32UI:                            return GL_BGRA;
        case GL_COMPRESSED_RED:                      return GL_RED;
        case GL_COMPRESSED_RG:                       return GL_RG;
        case GL_COMPRESSED_RGB:                      return GL_RGB;
        case GL_COMPRESSED_RGBA:                     return GL_RGBA;
        case GL_COMPRESSED_SRGB:                     return GL_RGB;
        case GL_COMPRESSED_SRGB_ALPHA:               return GL_RGBA;
        case GL_COMPRESSED_RED_RGTC1:                return GL_RED;
        case GL_COMPRESSED_SIGNED_RED_RGTC1:         return GL_RED;
        case GL_COMPRESSED_RG_RGTC2:                 return GL_RG;
        case GL_COMPRESSED_SIGNED_RG_RGTC2:          return GL_RG;
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:        return GL_RGB;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:       return GL_RGBA;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:       return GL_RGBA;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:       return GL_RGBA;
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:       return GL_RGB;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return GL_RGBA;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return GL_RGBA;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return GL_RGBA;
        default:
            break;
    }
    return GL_NONE;
}

GLenum gl::texture_target(GLenum sampler_type)
{
    switch (sampler_type)
    {
        case GL_SAMPLER_1D:
        case GL_INT_SAMPLER_1D:
        case GL_UNSIGNED_INT_SAMPLER_1D:
        case GL_SAMPLER_1D_SHADOW:
            return GL_TEXTURE_1D;

        case GL_SAMPLER_2D:
        case GL_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_SAMPLER_2D_SHADOW:
            return GL_TEXTURE_2D;

        case GL_SAMPLER_3D:
        case GL_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
            return GL_TEXTURE_3D;

        case GL_SAMPLER_CUBE:
        case GL_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
        case GL_SAMPLER_CUBE_SHADOW:
            return GL_TEXTURE_CUBE_MAP;

        case GL_SAMPLER_1D_ARRAY:
        case GL_SAMPLER_1D_ARRAY_SHADOW:
        case GL_INT_SAMPLER_1D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            return GL_TEXTURE_1D_ARRAY;

        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            return GL_TEXTURE_2D_ARRAY;

        case GL_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            return GL_TEXTURE_BUFFER;

        case GL_SAMPLER_2D_RECT:
        case GL_SAMPLER_2D_RECT_SHADOW:
        case GL_INT_SAMPLER_2D_RECT:
        case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
            return GL_TEXTURE_RECTANGLE;

        case GL_SAMPLER_2D_MULTISAMPLE:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            return GL_TEXTURE_2D_MULTISAMPLE;

        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

        default:
            break;
    }
    return GL_TEXTURE_1D;
}

size_t gl::level_count(size_t width, size_t height, size_t slice_count, size_t max_levels)
{
    size_t levels = 0;
    size_t major  = 0;

    // select the largest of (width, height, slice_count):
    major = (width > height)      ? width : height;
    major = (major > slice_count) ? major : slice_count;
    // calculate the number of levels down to 1 in the largest dimension.
    while (major > 0)
    {
        major >>= 1;
        levels += 1;
    }
    if (max_levels == 0) return levels;
    else return (max_levels < levels) ? max_levels : levels;
}

size_t gl::level_dimension(size_t dimension, size_t level_index)
{
    size_t  l_dimension  = dimension >> level_index;
    return (l_dimension == 0) ? 1 : l_dimension;
}

void gl::describe_mipmaps(
    GLenum            internal_format,
    GLenum            data_type,
    size_t            width,
    size_t            height,
    size_t            slice_count,
    size_t            alignment,
    size_t            max_levels,
    gl::level_desc_t *level_desc)
{
    size_t slices      = slice_count;
    GLenum type        = data_type;
    GLenum format      = internal_format;
    GLenum layout      = gl::pixel_layout(format);
    size_t bytes_per_e = gl::bytes_per_element(format, type);
    size_t num_levels  = gl::level_count(width, height, slices, max_levels);

    for (size_t i = 0; i < num_levels; ++i)
    {
        size_t lw = gl::level_dimension(width,  i);
        size_t lh = gl::level_dimension(height, i);
        size_t ls = gl::level_dimension(slices, i);
        level_desc[i].Index           = i;
        level_desc[i].Width           = gl::image_dimension(format, lw);
        level_desc[i].Height          = gl::image_dimension(format, lh);
        level_desc[i].Slices          = ls;
        level_desc[i].BytesPerElement = bytes_per_e;
        level_desc[i].BytesPerRow     = gl::bytes_per_row(format, type, level_desc[i].Width, alignment);
        level_desc[i].BytesPerSlice   = level_desc[i].BytesPerRow * level_desc[i].Height;
        level_desc[i].Layout          = layout;
        level_desc[i].Format          = internal_format;
        level_desc[i].DataType        = data_type;
    }
}

void gl::checker_image(size_t width, size_t height, float alpha, void *buffer)
{
    uint8_t  a = (uint8_t)((alpha - 0.5f) * 255.0f);
    uint8_t *p = (uint8_t*) buffer;
    for (size_t i = 0;  i < height; ++i)
    {
        for (size_t j = 0; j < width; ++j)
        {
            uint8_t v = ((((i & 0x8) == 0)) ^ ((j & 0x8) == 0)) ? 1 : 0;
            *p++  = v * 0xFF;
            *p++  = v ? 0x00 : 0xFF;
            *p++  = v * 0xFF;
            *p++  = a;
        }
    }
}

void gl::texture_storage(
    GLenum target,
    GLenum internal_format,
    GLenum data_type,
    GLenum min_filter,
    GLenum mag_filter,
    size_t width,
    size_t height,
    size_t slice_count,
    size_t max_levels)
{
    GLenum layout = gl::pixel_layout(internal_format);

    if (max_levels == 0)
    {
        // specify mipmaps all the way down to 1x1x1.
        max_levels  = gl::level_count(width, height, slice_count, max_levels);
    }

    // make sure that no PBO is bound as the unpack target.
    // we don't want to copy data to the texture now.
    glBindBuffer (GL_PIXEL_UNPACK_BUFFER, 0);

    // specify the maximum number of mipmap levels.
    if (target != GL_TEXTURE_RECTANGLE)
    {
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL,  GLint(max_levels - 1));
    }
    else
    {
        // rectangle textures don't support mipmaps.
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL,  0);
    }

    // specify the filtering and wrapping modes.
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(target, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);

    // use glTexImage to allocate storage for each mip-level of the texture.
    switch (target)
    {
            case GL_TEXTURE_1D:
                {
                    for (size_t lod = 0; lod < max_levels; ++lod)
                    {
                        size_t lw = gl::level_dimension(width, lod);
                        glTexImage1D(target, GLint(lod), internal_format, GLsizei(lw), 0, layout, data_type, NULL);
                    }
                }
                break;

            case GL_TEXTURE_1D_ARRAY:
                {
                    // 1D array textures specify slice_count for height.
                    for (size_t lod = 0; lod < max_levels; ++lod)
                    {
                        size_t lw = gl::level_dimension(width, lod);
                        glTexImage2D(target, GLint(lod), internal_format, GLsizei(lw), GLsizei(slice_count), 0, layout, data_type, NULL);
                    }
                }
                break;

            case GL_TEXTURE_RECTANGLE:
                {
                    // rectangle textures don't support mipmaps.
                    glTexImage2D(target, 0, internal_format, GLsizei(width), GLsizei(height), 0, layout, data_type, NULL);
                }
                break;

            case GL_TEXTURE_2D:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    for (size_t lod = 0; lod < max_levels; ++lod)
                    {
                        size_t lw = gl::level_dimension(width,  lod);
                        size_t lh = gl::level_dimension(height, lod);
                        glTexImage2D(target, GLint(lod), internal_format, GLsizei(lw), GLsizei(lh), 0, layout, data_type, NULL);
                    }
                }
                break;

            case GL_TEXTURE_2D_ARRAY:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                {
                    // 2D array texture specify slice_count as the number of
                    // items; the number of items is not decreased with LOD.
                    for (size_t lod = 0; lod < max_levels; ++lod)
                    {
                        size_t lw = gl::level_dimension(width,  lod);
                        size_t lh = gl::level_dimension(height, lod);
                        glTexImage3D(target, GLint(lod), internal_format, GLsizei(lw), GLsizei(lh), GLsizei(slice_count), 0, layout, data_type, NULL);
                    }
                }
                break;

            case GL_TEXTURE_3D:
                {
                    for (size_t lod = 0; lod < max_levels; ++lod)
                    {
                        size_t lw = gl::level_dimension(width,       lod);
                        size_t lh = gl::level_dimension(height,      lod);
                        size_t ls = gl::level_dimension(slice_count, lod);
                        glTexImage3D(target, GLint(lod), internal_format, GLsizei(lw), GLsizei(lh), GLsizei(ls), 0, layout, data_type, NULL);
                    }
                }
                break;
    }
}

void gl::transfer_pixels_d2h(gl::pixel_transfer_d2h_t *transfer)
{
    if (transfer->PackBuffer != 0)
    {
        // select the PBO as the target of the pack operation.
        glBindBuffer(GL_PIXEL_PACK_BUFFER, transfer->PackBuffer);
    }
    else
    {
        // select the client memory as the target of the pack operation.
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
    if (transfer->TargetWidth != transfer->TransferWidth)
    {
        // transferring into a sub-rectangle of the image; tell GL how
        // many pixels are in a single row of the target image.
        glPixelStorei(GL_PACK_ROW_LENGTH,   GLint(transfer->TargetWidth));
        glPixelStorei(GL_PACK_IMAGE_HEIGHT, GLint(transfer->TargetHeight));
    }

    // perform the setup necessary to have GL calculate any byte offsets.
    if (transfer->TargetX != 0) glPixelStorei(GL_PACK_SKIP_PIXELS, GLint(transfer->TargetX));
    if (transfer->TargetY != 0) glPixelStorei(GL_PACK_SKIP_ROWS,   GLint(transfer->TargetY));
    if (transfer->TargetZ != 0) glPixelStorei(GL_PACK_SKIP_IMAGES, GLint(transfer->TargetZ));

    if (gl::bytes_per_block(transfer->Format) > 0)
    {
        // the texture image is compressed; use glGetCompressedTexImage.
        switch (transfer->Target)
        {
            case GL_TEXTURE_1D:
            case GL_TEXTURE_2D:
            case GL_TEXTURE_3D:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    glGetCompressedTexImage(
                        transfer->Target,
                        GLint(transfer->SourceIndex),
                        transfer->TransferBuffer);
                }
                break;
        }
    }
    else
    {
        // the image is not compressed; read the framebuffer with
        // glReadPixels or the texture image using glGetTexImage.
        switch (transfer->Target)
        {
            case GL_READ_FRAMEBUFFER:
                {
                    // remember, x and y identify the lower-left corner.
                    glReadPixels(
                        GLint(transfer->TransferX),
                        GLint(transfer->TransferY),
                        GLint(transfer->TransferWidth),
                        GLint(transfer->TransferHeight),
                        transfer->Layout,
                        transfer->DataType,
                        transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_1D:
            case GL_TEXTURE_2D:
            case GL_TEXTURE_3D:
            case GL_TEXTURE_1D_ARRAY:
            case GL_TEXTURE_2D_ARRAY:
            case GL_TEXTURE_RECTANGLE:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    glGetTexImage(
                        transfer->Target,
                        GLint(transfer->SourceIndex),
                        transfer->Layout,
                        transfer->DataType,
                        transfer->TransferBuffer);
                }
                break;
        }
    }

    // restore the pack state values to their defaults.
    if (transfer->PackBuffer  != 0)
        glBindBuffer(GL_PIXEL_PACK_BUFFER,  0);
    if (transfer->TargetWidth != transfer->TransferWidth)
    {
        glPixelStorei(GL_PACK_ROW_LENGTH,   0);
        glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
    }
    if (transfer->TargetX != 0) glPixelStorei(GL_PACK_SKIP_PIXELS,  0);
    if (transfer->TargetY != 0) glPixelStorei(GL_PACK_SKIP_ROWS,    0);
    if (transfer->TargetZ != 0) glPixelStorei(GL_PACK_SKIP_IMAGES,  0);
}

void gl::transfer_pixels_h2d(gl::pixel_transfer_h2d_t *transfer)
{
    if (transfer->UnpackBuffer != 0)
    {
        // select the PBO as the source of the unpack operation.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, transfer->UnpackBuffer);
    }
    else
    {
        // select the client memory as the source of the unpack operation.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    if (transfer->SourceWidth != transfer->TransferWidth)
    {
        // transferring a sub-rectangle of the image; tell GL how many
        // pixels are in a single row of the source image.
        glPixelStorei(GL_UNPACK_ROW_LENGTH, GLint(transfer->SourceWidth));
    }
    if (transfer->TransferSlices > 1)
    {
        // transferring an image volume; tell GL how many rows per-slice
        // in the source image, since we may only be transferring a sub-volume.
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, GLint(transfer->SourceHeight));
    }

    // perform the setup necessary to have GL calculate any byte offsets.
    if (transfer->SourceX != 0) glPixelStorei(GL_UNPACK_SKIP_PIXELS, GLint(transfer->SourceX));
    if (transfer->SourceY != 0) glPixelStorei(GL_UNPACK_SKIP_ROWS,   GLint(transfer->SourceY));
    if (transfer->SourceZ != 0) glPixelStorei(GL_UNPACK_SKIP_IMAGES, GLint(transfer->SourceZ));

    if (gl::bytes_per_block(transfer->Format) > 0)
    {
        // the image is compressed; use glCompressedTexSubImage to transfer.
        switch (transfer->Target)
        {
            case GL_TEXTURE_1D:
                {
                    glCompressedTexSubImage1D(
                        transfer->Target,
                        GLint(transfer->TargetIndex),
                        GLint(transfer->TargetX),
                        GLsizei(transfer->TransferWidth),
                        transfer->Format,
                        GLsizei(transfer->TransferSize),
                        transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_2D:
            case GL_TEXTURE_1D_ARRAY:
            case GL_TEXTURE_RECTANGLE:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    glCompressedTexSubImage2D(
                        transfer->Target,
                        GLint(transfer->TargetIndex),
                        GLint(transfer->TargetX),
                        GLint(transfer->TargetY),
                        GLsizei(transfer->TransferWidth),
                        GLsizei(transfer->TransferHeight),
                        transfer->Format,
                        GLsizei(transfer->TransferSize),
                        transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                {
                    glCompressedTexSubImage3D(
                        transfer->Target,
                        GLint(transfer->TargetIndex),
                        GLint(transfer->TargetX),
                        GLint(transfer->TargetY),
                        GLint(transfer->TargetZ),
                        GLsizei(transfer->TransferWidth),
                        GLsizei(transfer->TransferHeight),
                        GLsizei(transfer->TransferSlices),
                        transfer->Format,
                        GLsizei(transfer->TransferSize),
                        transfer->TransferBuffer);
                }
                break;
        }
    }
    else
    {
        // the image is not compressed, use glTexSubImage to transfer data.
        switch (transfer->Target)
        {
            case GL_TEXTURE_1D:
                {
                    glTexSubImage1D(
                        transfer->Target,
                        GLint(transfer->TargetIndex),
                        GLint(transfer->TargetX),
                        GLsizei(transfer->TransferWidth),
                        transfer->Format,
                        transfer->DataType,
                        transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_2D:
            case GL_TEXTURE_1D_ARRAY:
            case GL_TEXTURE_RECTANGLE:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    glTexSubImage2D(
                        transfer->Target,
                        GLint(transfer->TargetIndex),
                        GLint(transfer->TargetX),
                        GLint(transfer->TargetY),
                        GLsizei(transfer->TransferWidth),
                        GLsizei(transfer->TransferHeight),
                        transfer->Format,
                        transfer->DataType,
                        transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                {
                    glTexSubImage3D(
                        transfer->Target,
                        GLint(transfer->TargetIndex),
                        GLint(transfer->TargetX),
                        GLint(transfer->TargetY),
                        GLint(transfer->TargetZ),
                        GLsizei(transfer->TransferWidth),
                        GLsizei(transfer->TransferHeight),
                        GLsizei(transfer->TransferSlices),
                        transfer->Format,
                        transfer->DataType,
                        transfer->TransferBuffer);
                }
                break;
        }
    }

    // restore the unpack state values to their defaults.
    if (transfer->UnpackBuffer  != 0)
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,  0);
    if (transfer->SourceWidth   != transfer->TransferWidth)
        glPixelStorei(GL_UNPACK_ROW_LENGTH,   0);
    if (transfer->TransferSlices > 1)
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    if (transfer->SourceX != 0)
        glPixelStorei(GL_UNPACK_SKIP_PIXELS,  0);
    if (transfer->SourceY != 0)
        glPixelStorei(GL_UNPACK_SKIP_ROWS,    0);
    if (transfer->SourceZ != 0)
        glPixelStorei(GL_UNPACK_SKIP_IMAGES,  0);
}

void gl::create_sprite_batch(gl::sprite_batch_t *batch, size_t capacity)
{
    if (batch)
    {
        batch->Count    = 0;
        batch->Capacity = capacity;
        if (capacity)
        {
            batch->Quads = (gl::sprite_quad_t*)      malloc(capacity * sizeof(gl::sprite_quad_t));
            batch->State = (gl::sprite_sort_data_t*) malloc(capacity * sizeof(gl::sprite_sort_data_t));
            batch->Order = (uint32_t*)               malloc(capacity * sizeof(uint32_t));
        }
        else
        {
            batch->Quads = NULL;
            batch->State = NULL;
            batch->Order = NULL;
        }
    }
}

void gl::delete_sprite_batch(gl::sprite_batch_t *batch)
{
    if (batch)
    {
        if (batch->Capacity)
        {
            free(batch->Order);
            free(batch->State);
            free(batch->Quads);
        }
        batch->Count    = 0;
        batch->Capacity = 0;
        batch->Quads    = NULL;
        batch->State    = NULL;
        batch->Order    = NULL;
    }
}

// need to pass batch->Count + NumToAdd as capacity
void gl::ensure_sprite_batch(gl::sprite_batch_t *batch, size_t capacity)
{
    if (batch->Capacity < capacity)
    {
        batch->Capacity = capacity;
        batch->Quads    = (gl::sprite_quad_t*)      realloc(batch->Quads, capacity * sizeof(gl::sprite_quad_t));
        batch->State    = (gl::sprite_sort_data_t*) realloc(batch->State, capacity * sizeof(gl::sprite_sort_data_t));
        batch->Order    = (uint32_t*)               realloc(batch->Order, capacity * sizeof(uint32_t));
    }
}

void gl::flush_sprite_batch(gl::sprite_batch_t *batch)
{
    batch->Count = 0;
}

void gl::generate_quads(
    gl::sprite_quad_t      *quads,
    gl::sprite_sort_data_t *sdata,
    uint32_t               *indices,
    size_t                  quad_offset,
    gl::sprite_t const     *sprites,
    size_t                  sprite_offset,
    size_t                  sprite_count)
{
    size_t qindex = quad_offset;
    size_t sindex = sprite_offset;
    for (size_t i = 0;  i < sprite_count; ++i, ++qindex, ++sindex)
    {
        gl::sprite_t const     &s = sprites[sindex];
        gl::sprite_sort_data_t &r = sdata[qindex];
        gl::sprite_quad_t      &q = quads[qindex];

        q.Source[0]   = float(s.ImageX);
        q.Source[1]   = float(s.ImageY);
        q.Source[2]   = float(s.ImageWidth);
        q.Source[3]   = float(s.ImageHeight);
        q.Target[0]   = s.ScreenX;
        q.Target[1]   = s.ScreenY;
        q.Target[2]   = s.ImageWidth  * s.ScaleX;
        q.Target[3]   = s.ImageHeight * s.ScaleY;
        q.Origin[0]   = s.OriginX;
        q.Origin[1]   = s.OriginY;
        q.Scale [0]   = 1.0f / s.TextureWidth;
        q.Scale [1]   = 1.0f / s.TextureHeight;
        q.Orientation = s.Orientation;
        q.TintColor   = s.TintColor;

        r.LayerDepth  = s.LayerDepth;
        r.RenderState = s.RenderState;

        indices[qindex] = uint32_t(qindex);
    }
}

void gl::generate_quad_vertices_ptc(
    void                    *buffer,
    size_t                   buffer_offset,
    gl::sprite_quad_t const *quads,
    uint32_t const          *indices,
    size_t                   quad_offset,
    size_t                   quad_count)
{
    static size_t const X      =  0;
    static size_t const Y      =  1;
    static size_t const W      =  2;
    static size_t const H      =  3;
    static float  const XCO[4] = {0.0f, 1.0f, 1.0f, 0.0f};
    static float  const YCO[4] = {0.0f, 0.0f, 1.0f, 1.0f};

    gl::sprite_vertex_ptc_t *vertex_buffer = (gl::sprite_vertex_ptc_t*) buffer;
    size_t                   vertex_offset = buffer_offset;
    for (size_t i = 0; i < quad_count; ++i)
    {
        // figure out which quad we're working with.
        uint32_t const id = indices[quad_offset + i];
        gl::sprite_quad_t const &quad = quads[id];

        // pre-calculate values constant across the quad.
        float    const src_x = quad.Source[X];
        float    const src_y = quad.Source[Y];
        float    const src_w = quad.Source[W];
        float    const src_h = quad.Source[H];
        float    const dst_x = quad.Target[X];
        float    const dst_y = quad.Target[Y];
        float    const dst_w = quad.Target[W];
        float    const dst_h = quad.Target[H];
        float    const ctr_x = quad.Origin[X] / src_w;
        float    const ctr_y = quad.Origin[Y] / src_h;
        float    const scl_u = quad.Scale[X];
        float    const scl_v = quad.Scale[Y];
        float    const angle = quad.Orientation;
        uint32_t const color = quad.TintColor;
        float    const sin_o = sinf(angle);
        float    const cos_o = cosf(angle);

        // calculate values that change per-vertex.
        for (size_t j = 0; j < 4; ++j)
        {
            gl::sprite_vertex_ptc_t &vert = vertex_buffer[vertex_offset++];
            float ofs_x    = XCO[j];
            float ofs_y    = YCO[j];
            float x_dst    = (ofs_x - ctr_x)  *  dst_w;
            float y_dst    = (ofs_y - ctr_y)  *  dst_h;
            vert.XYUV[0]   = (dst_x + (x_dst * cos_o)) - (y_dst * sin_o);
            vert.XYUV[1]   = (dst_y + (x_dst * sin_o)) + (y_dst * cos_o);
            vert.XYUV[2]   = (src_x + (ofs_x * src_w)) *  scl_u;
            vert.XYUV[3]   = 1.0f - ((src_y + (ofs_y * src_h)) *  scl_v);
            vert.TintColor = color;
        }
    }
}

void gl::generate_quad_indices_u16(void *buffer, size_t offset, size_t base_vertex, size_t quad_count)
{
    uint16_t *u16 = (uint16_t*) buffer;
    uint16_t  b16 = (uint16_t ) base_vertex;
    for (size_t i = 0; i < quad_count; ++i)
    {
        u16[offset++]  = (b16 + 1);
        u16[offset++]  = (b16 + 0);
        u16[offset++]  = (b16 + 2);
        u16[offset++]  = (b16 + 2);
        u16[offset++]  = (b16 + 0);
        u16[offset++]  = (b16 + 3);
        b16 += 4;
    }
}

void gl::generate_quad_indices_u32(void *buffer, size_t offset, size_t base_vertex, size_t quad_count)
{
    uint32_t *u32 = (uint32_t*) buffer;
    uint32_t  b32 = (uint32_t ) base_vertex;
    for (size_t i = 0; i < quad_count; ++i)
    {
        u32[offset++]  = (b32 + 1);
        u32[offset++]  = (b32 + 0);
        u32[offset++]  = (b32 + 2);
        u32[offset++]  = (b32 + 2);
        u32[offset++]  = (b32 + 0);
        u32[offset++]  = (b32 + 3);
        b32 += 4;
    }
}

bool gl::create_sprite_effect(gl::sprite_effect_t *effect, size_t quad_count, size_t vertex_size, size_t index_size)
{
    GLuint  vao        =  0;
    GLuint  buffers[2] = {0, 0};
    size_t  vcount     = quad_count  * 4;
    size_t  icount     = quad_count  * 6;
    GLsizei abo_size   = GLsizei(vertex_size * vcount);
    GLsizei eao_size   = GLsizei(index_size  * icount);

    glGenBuffers(2, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, abo_size, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, eao_size, NULL, GL_DYNAMIC_DRAW);
    glGenVertexArrays(1, &vao);

    effect->VertexCapacity   = vcount;
    effect->VertexOffset     = 0;
    effect->VertexSize       = vertex_size;
    effect->IndexCapacity    = icount;
    effect->IndexOffset      = 0;
    effect->IndexSize        = index_size;
    effect->CurrentState     = 0xFFFFFFFFU;
    effect->VertexArray      = vao;
    effect->VertexBuffer     = buffers[0];
    effect->IndexBuffer      = buffers[1];
    effect->BlendEnabled     = GL_FALSE;
    effect->BlendSourceColor = GL_ONE;
    effect->BlendSourceAlpha = GL_ONE;
    effect->BlendTargetColor = GL_ZERO;
    effect->BlendTargetAlpha = GL_ZERO;
    effect->BlendFuncColor   = GL_FUNC_ADD;
    effect->BlendFuncAlpha   = GL_FUNC_ADD;
    effect->BlendColor[0]    = 0.0f;
    effect->BlendColor[1]    = 0.0f;
    effect->BlendColor[2]    = 0.0f;
    effect->BlendColor[3]    = 0.0f;
    return true;
}

void gl::delete_sprite_effect(gl::sprite_effect_t *effect)
{
    GLuint buffers[2] = {
        effect->VertexBuffer,
        effect->IndexBuffer
    };
    glDeleteBuffers(2, buffers);
    glDeleteVertexArrays(1, &effect->VertexArray);
    effect->VertexCapacity = 0;
    effect->VertexOffset   = 0;
    effect->VertexSize     = 0;
    effect->IndexCapacity  = 0;
    effect->IndexOffset    = 0;
    effect->IndexSize      = 0;
    effect->VertexArray    = 0;
    effect->VertexBuffer   = 0;
    effect->IndexBuffer    = 0;
}

void gl::sprite_effect_blend_none(gl::sprite_effect_t *effect)
{
    effect->BlendEnabled     = GL_FALSE;
    effect->BlendSourceColor = GL_ONE;
    effect->BlendSourceAlpha = GL_ONE;
    effect->BlendTargetColor = GL_ZERO;
    effect->BlendTargetAlpha = GL_ZERO;
    effect->BlendFuncColor   = GL_FUNC_ADD;
    effect->BlendFuncAlpha   = GL_FUNC_ADD;
    effect->BlendColor[0]    = 0.0f;
    effect->BlendColor[1]    = 0.0f;
    effect->BlendColor[2]    = 0.0f;
    effect->BlendColor[3]    = 0.0f;
}

void gl::sprite_effect_blend_alpha(gl::sprite_effect_t *effect)
{
    effect->BlendEnabled     = GL_TRUE;
    effect->BlendSourceColor = GL_SRC_COLOR;
    effect->BlendSourceAlpha = GL_SRC_ALPHA;
    effect->BlendTargetColor = GL_ONE_MINUS_SRC_ALPHA;
    effect->BlendTargetAlpha = GL_ONE_MINUS_SRC_ALPHA;
    effect->BlendFuncColor   = GL_FUNC_ADD;
    effect->BlendFuncAlpha   = GL_FUNC_ADD;
    effect->BlendColor[0]    = 0.0f;
    effect->BlendColor[1]    = 0.0f;
    effect->BlendColor[2]    = 0.0f;
    effect->BlendColor[3]    = 0.0f;
}

void gl::sprite_effect_blend_additive(gl::sprite_effect_t *effect)
{
    effect->BlendEnabled     = GL_TRUE;
    effect->BlendSourceColor = GL_SRC_COLOR;
    effect->BlendSourceAlpha = GL_SRC_ALPHA;
    effect->BlendTargetColor = GL_ONE;
    effect->BlendTargetAlpha = GL_ONE;
    effect->BlendFuncColor   = GL_FUNC_ADD;
    effect->BlendFuncAlpha   = GL_FUNC_ADD;
    effect->BlendColor[0]    = 0.0f;
    effect->BlendColor[1]    = 0.0f;
    effect->BlendColor[2]    = 0.0f;
    effect->BlendColor[3]    = 0.0f;
}

void gl::sprite_effect_blend_premultiplied(gl::sprite_effect_t *effect)
{
    effect->BlendEnabled     = GL_TRUE;
    effect->BlendSourceColor = GL_ONE;
    effect->BlendSourceAlpha = GL_ONE;
    effect->BlendTargetColor = GL_ONE_MINUS_SRC_ALPHA;
    effect->BlendTargetAlpha = GL_ONE_MINUS_SRC_ALPHA;
    effect->BlendFuncColor   = GL_FUNC_ADD;
    effect->BlendFuncAlpha   = GL_FUNC_ADD;
    effect->BlendColor[0]    = 0.0f;
    effect->BlendColor[1]    = 0.0f;
    effect->BlendColor[2]    = 0.0f;
    effect->BlendColor[3]    = 0.0f;
}

void gl::sprite_effect_set_viewport(gl::sprite_effect_t *effect, int width, int height)
{
    float *dst16 = effect->Projection;
    float  s_x = 1.0f / (width   * 0.5f);
    float  s_y = 1.0f / (height  * 0.5f);
    dst16[ 0]  = s_x ; dst16[ 1] = 0.0f; dst16[ 2] = 0.0f; dst16[ 3] = 0.0f;
    dst16[ 4]  = 0.0f; dst16[ 5] = -s_y; dst16[ 6] = 0.0f; dst16[ 7] = 0.0f;
    dst16[ 8]  = 0.0f; dst16[ 9] = 0.0f; dst16[10] = 1.0f; dst16[11] = 0.0f;
    dst16[12]  =-1.0f; dst16[13] = 1.0f; dst16[14] = 0.0f; dst16[15] = 1.0f;
}

void gl::sprite_effect_bind_buffers(gl::sprite_effect_t *effect)
{
    glBindVertexArray(effect->VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, effect->VertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, effect->IndexBuffer);
}

void gl::sprite_effect_apply_blendstate(gl::sprite_effect_t *effect)
{
    if (effect->BlendEnabled)
    {
        glEnable(GL_BLEND);
        glBlendColor(effect->BlendColor[0], effect->BlendColor[1], effect->BlendColor[2], effect->BlendColor[3]);
        glBlendFuncSeparate(effect->BlendSourceColor, effect->BlendTargetColor, effect->BlendSourceAlpha, effect->BlendTargetAlpha);
        glBlendEquationSeparate(effect->BlendFuncColor, effect->BlendFuncAlpha);
    }
    else glDisable(GL_BLEND);
}

void gl::sprite_effect_setup_vao_ptc(gl::sprite_effect_t *effect)
{
    glBindVertexArray(effect->VertexArray);
    glEnableVertexAttribArray(GL_SPRITE_PTC_LOCATION_PTX);
    glEnableVertexAttribArray(GL_SPRITE_PTC_LOCATION_CLR);
    glVertexAttribPointer(GL_SPRITE_PTC_LOCATION_PTX, 4, GL_FLOAT,         GL_FALSE, sizeof(gl::sprite_vertex_ptc_t), (GLvoid const*)  0);
    glVertexAttribPointer(GL_SPRITE_PTC_LOCATION_CLR, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(gl::sprite_vertex_ptc_t), (GLvoid const*) 16);
}

size_t gl::sprite_effect_buffer_data_ptc(
    gl::sprite_effect_t     *effect,
    gl::sprite_quad_t const *quads,
    uint32_t const          *indices,
    size_t                   quad_offset,
    size_t                   quad_count,
    size_t                  *base_index_arg)
{
    if (effect->VertexOffset == effect->VertexCapacity)
    {
        // the buffer is completely full. time to discard it and
        // request a new buffer from the driver, to avoid stalls.
        GLsizei abo_size     = GLsizei(effect->VertexCapacity * effect->VertexSize);
        GLsizei eao_size     = GLsizei(effect->IndexCapacity  * effect->IndexSize);
        effect->VertexOffset = 0;
        effect->IndexOffset  = 0;
        glBufferData(GL_ARRAY_BUFFER, abo_size, NULL, GL_DYNAMIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, eao_size, NULL, GL_DYNAMIC_DRAW);
    }

    size_t num_indices  = quad_count * 6;
    size_t num_vertices = quad_count * 4;
    size_t max_vertices = effect->VertexCapacity;
    size_t base_vertex  = effect->VertexOffset;
    size_t vertex_size  = effect->VertexSize;
    size_t max_indices  = effect->IndexCapacity;
    size_t base_index   = effect->IndexOffset;
    size_t index_size   = effect->IndexSize;
    if (max_vertices < base_vertex + num_vertices)
    {
        // not enough space in the buffer to fit everything.
        // only a portion of the desired data will be buffered.
        num_vertices = max_vertices - base_vertex;
        num_indices  = max_indices  - base_index;
    }

    size_t buffer_count =  num_vertices / 4;
    if (buffer_count == 0) return 0;

    GLintptr   v_offset = base_vertex  * vertex_size;
    GLsizeiptr v_size   = num_vertices * vertex_size;
    GLbitfield v_access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
    GLvoid    *v_data   = glMapBufferRange(GL_ARRAY_BUFFER, v_offset, v_size, v_access);
    if (v_data != NULL)
    {
        gl::generate_quad_vertices_ptc(v_data, 0, quads, indices, quad_offset, buffer_count);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    GLintptr   i_offset = base_index  * index_size;
    GLsizeiptr i_size   = num_indices * index_size;
    GLbitfield i_access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
    GLvoid    *i_data   = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, i_offset, i_size, i_access);
    if (i_data != NULL)
    {
        if (index_size == sizeof(uint16_t)) gl::generate_quad_indices_u16(i_data, 0, base_vertex, buffer_count);
        else gl::generate_quad_indices_u32(i_data, 0, base_vertex, buffer_count);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    }

    effect->VertexOffset += buffer_count * 4;
    effect->IndexOffset  += buffer_count * 6;
    *base_index_arg       = base_index;
    return buffer_count;
}

void gl::sprite_effect_draw_batch_ptc(
    gl::sprite_effect_t             *effect,
    gl::sprite_batch_t              *batch,
    gl::sprite_effect_apply_t const *fxfuncs,
    void                            *context)
{
    size_t quad_count = batch->Count;
    size_t quad_index = 0;
    size_t base_index = 0;
    size_t n          = 0;

    fxfuncs->SetupEffect(effect, context);
    effect->CurrentState = 0xFFFFFFFFU;

    while (quad_count > 0)
    {
        n = gl::sprite_effect_buffer_data_ptc(effect, batch->Quads, batch->Order, quad_index, quad_count, &base_index);
        gl::sprite_effect_draw_batch_region_ptc(effect, batch, quad_index, n, base_index, fxfuncs, context);
        base_index  = effect->IndexOffset;
        quad_index += n;
        quad_count -= n;
    }
}

void gl::sprite_effect_draw_batch_region_ptc(
    gl::sprite_effect_t             *effect,
    gl::sprite_batch_t              *batch,
    size_t                           quad_offset,
    size_t                           quad_count,
    size_t                           base_index,
    gl::sprite_effect_apply_t const *fxfuncs,
    void                            *context)
{
    uint32_t state_0 = effect->CurrentState;
    uint32_t state_1 = effect->CurrentState;
    size_t   index   = 0; // index of start of sub-batch
    size_t   nquad   = 0; // count of quads in sub-batch
    size_t   nindex  = 0; // count of indices in sub-batch
    size_t   quad_id = 0; // quad insertion index
    GLsizei  size    = GLsizei(effect->IndexSize);
    GLenum   type    = size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

    for (size_t i = 0; i < quad_count; ++i)
    {
        quad_id = batch->Order[quad_offset + i];
        state_1 = batch->State[quad_id].RenderState;
        if (state_1 != state_0)
        {
            // render the previous sub-batch with the current state,
            // as long as it has at least one quad in the sub-batch.
            if (i > index)
            {
                nquad  = i - index;  // the number of quads being submitted
                nindex = nquad * 6;  // the number of indices being submitted
                glDrawElements(GL_TRIANGLES, GLsizei(nindex), type, GL_BUFFER_OFFSET(base_index * size));
                base_index += nindex;
            }
            // now apply the new state and start a new sub-batch.
            fxfuncs->ApplyState(effect, state_1, context);
            state_0 = state_1;
            index   = i;
        }
    }
    // submit the remainder of the sub-batch.
    nquad  = quad_count - index;
    nindex = nquad * 6;
    glDrawElements(GL_TRIANGLES, GLsizei(nindex), type, GL_BUFFER_OFFSET(base_index * size));
    effect->CurrentState = state_1;
}

bool gl::create_sprite_shader_ptc_clr(gl::sprite_shader_ptc_clr_t *shader)
{
    if (shader)
    {
        gl::shader_source_t     sources;
        gl::shader_source_init(&sources);
        gl::shader_source_add (&sources, GL_VERTEX_SHADER,   (char**) &SpriteShaderPTC_CLR_VSS, 1);
        gl::shader_source_add (&sources, GL_FRAGMENT_SHADER, (char**) &SpriteShaderPTC_CLR_FSS, 1);
        if (gl::build_shader  (&sources, &shader->ShaderDesc, &shader->Program))
        {
            shader->AttribPTX  = gl::find_attribute(&shader->ShaderDesc, "aPTX");
            shader->AttribCLR  = gl::find_attribute(&shader->ShaderDesc, "aCLR");
            shader->UniformMSS = gl::find_uniform  (&shader->ShaderDesc, "uMSS");
            return true;
        }
        else return false;
    }
    else return false;
}

void gl::delete_sprite_shader_ptc_clr(gl::sprite_shader_ptc_clr_t *shader)
{
    if (shader && shader->Program)
    {
        gl::shader_desc_free(&shader->ShaderDesc);
        glDeleteProgram(shader->Program);
        shader->AttribPTX  = NULL;
        shader->AttribCLR  = NULL;
        shader->UniformMSS = NULL;
        shader->Program    = 0;
    }
}

bool gl::create_sprite_shader_ptc_tex(gl::sprite_shader_ptc_tex_t *shader)
{
    if (shader)
    {
        gl::shader_source_t     sources;
        gl::shader_source_init(&sources);
        gl::shader_source_add (&sources, GL_VERTEX_SHADER,   (char**) &SpriteShaderPTC_TEX_VSS, 1);
        gl::shader_source_add (&sources, GL_FRAGMENT_SHADER, (char**) &SpriteShaderPTC_TEX_FSS, 1);
        if (gl::build_shader  (&sources, &shader->ShaderDesc, &shader->Program))
        {
            shader->AttribPTX  = gl::find_attribute(&shader->ShaderDesc, "aPTX");
            shader->AttribCLR  = gl::find_attribute(&shader->ShaderDesc, "aCLR");
            shader->SamplerTEX = gl::find_sampler  (&shader->ShaderDesc, "sTEX");
            shader->UniformMSS = gl::find_uniform  (&shader->ShaderDesc, "uMSS");
            return true;
        }
        else return false;
    }
    else return false;
}

void gl::delete_sprite_shader_ptc_tex(gl::sprite_shader_ptc_tex_t *shader)
{
    if (shader && shader->Program)
    {
        gl::shader_desc_free(&shader->ShaderDesc);
        glDeleteProgram(shader->Program);
        shader->AttribPTX  = NULL;
        shader->AttribCLR  = NULL;
        shader->SamplerTEX = NULL;
        shader->UniformMSS = NULL;
        shader->Program    = 0;
    }
}

