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

/*/////////////////
//   Functions   //
/////////////////*/
/// @summary Given a value from the DXGI_FORMAT enumeration, determine the
/// appropriate OpenGL format, base format and data type values. This is useful
/// when loading texture data from a DDS container.
/// @param dxgi A value of the DXGI_FORMAT enumeration (data::dxgi_format_e).
/// @param out_format On return, stores the corresponding OpenGL format.
/// @param out_baseformat On return, stores the corresponding OpenGL base format.
/// @param out_datatype On return, stores the corresponding OpenGL data type.
/// @return true if the input format could be mapped to OpenGL.
GLDRAW2D_PUBLIC bool dxgi_format_to_gl(uint32_t dxgi, GLenum *out_format, GLenum *out_baseformat, GLenum *out_datatype);
    // we need:
    // a function to convert from DXGI_FORMAT->GL (format, data type, base format)
    // a sprite batch for solid-colored quads
    // a sprite batch for textured quads
    // a system to manage the creation and deletion of sprites
    // all-in-one functions for logic and rendering of text & gui controls

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace r2d */

#endif /* GLDRAW2D_HPP_INCLUDED */
