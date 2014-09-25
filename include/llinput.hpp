/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines the interface to a basic input system built on top of the
/// abstractions for keyboards, mice and joysticks provided by GLFW.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LLINPUT_HPP_INCLUDED
#define LLINPUT_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <stddef.h>
#include <stdint.h>
#include "GLFW/glfw3.h"

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Abstract away Windows 32-bit calling conventions and visibility.
#if defined(_WIN32) && defined(_MSC_VER)
    #define  LLINPUT_CALL_C    __cdecl
    #if   defined(_MSC_VER)
    #define  LLINPUT_IMPORT    __declspec(dllimport)
    #define  LLINPUT_EXPORT    __declspec(dllexport)
    #elif defined(__GNUC__)
    #define  LLINPUT_IMPORT    __attribute__((dllimport))
    #define  LLINPUT_EXPORT    __attribute__((dllexport))
    #else
    #define  LLINPUT_IMPORT
    #define  LLINPUT_EXPORT
    #endif
#else
    #define  LLINPUT_CALL_C
    #if __GNUC__ >= 4
    #define  LLINPUT_IMPORT    __attribute__((visibility("default")))
    #define  LLINPUT_EXPORT    __attribute__((visibility("default")))
    #endif
#endif

/// @summary Define import/export based on whether we're being used as a DLL.
#if defined(LLINPUT_SHARED)
    #ifdef  LLINPUT_EXPORTS
    #define LLINPUT_PUBLIC     LLINPUT_EXPORT
    #else
    #define LLINPUT_PUBLIC     LLINPUT_IMPORT
    #endif
#else
    #define LLINPUT_PUBLIC
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace input {

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary Define the number of uin32_t values required to store the keyboard
/// state data. One bit corresponds to each key.
#define INPUT_KEY_WORDS              10U

/// @summary The key code offset value. This is subtracted from the key identifier
/// before indexing into the key state array.
#define INPUT_KEY_OFFSET             32

/// @summary Define the maximum number of buttons supported on a mouse.
#define INPUT_MAX_MOUSE_BUTTONS      32

/// @summary Define the maximum number of axis inputs supported on a joystick.
#define INPUT_MAX_CONTROLLER_AXES     8

/// @summary Define the maximum number of button inputs supported on a joystick.
#define INPUT_MAX_CONTROLLER_BUTTONS 32

/// @summary Define the maximum number of joystick controllers.
#define INPUT_MAX_CONTROLLERS        16

/*/////////////////
//   Data Types  //
/////////////////*/
/// @summary Internal data associated with the input system attached to a window.
struct context_t
{
    GLFWwindow *Window;
    float       ScaleX;
    float       ScaleY;
    float       MouseX;
    float       MouseY;
    uint32_t    MouseState;
    uint32_t    MouseModifiers;
    uint32_t    KeyboardModifiers;
    uint32_t    KeyboardState[INPUT_KEY_WORDS];
};

/// @summary Represents a snapshot of state for all input devices at a single point in time.
struct snapshot_t
{
    GLFWwindow *Window;
    float       ScaleX;
    float       ScaleY;
    float       MouseX;
    float       MouseY;
    uint32_t    MouseState;
    uint32_t    MouseModifiers;
    uint32_t    KeyboardModifiers;
    uint32_t    KeyboardState[INPUT_KEY_WORDS];
    size_t      ControllerCount;
    int         ControllerIds[INPUT_MAX_CONTROLLERS];
    size_t      ControllerAxisCount[INPUT_MAX_CONTROLLERS];
    size_t      ControllerButtonCount[INPUT_MAX_CONTROLLERS];
    float       ControllerAxes[INPUT_MAX_CONTROLLERS][INPUT_MAX_CONTROLLER_AXES];
    uint8_t     ControllerButtons[INPUT_MAX_CONTROLLERS][INPUT_MAX_CONTROLLER_BUTTONS];
};

/*////////////////
//   Functions  //
////////////////*/
/// @summary Attaches event handlers to the given window.
/// @param window The window to attach event handlers to.
bool attach(GLFWwindow *window);

/// @summary Detaches event handlers from the given window.
/// @param window The window to detach from.
void detach(GLFWwindow *window);

/// @summary Grabs a snapshot of input device state for the specified window.
/// @param dst The snapshot structure to populate.
/// @param window The window whose input state is being queried.
void snapshot(input::snapshot_t *dst, GLFWwindow *window);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace input */

#endif /* !defined(LLINPUT_HPP_INCLUDED) */

