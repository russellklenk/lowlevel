/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines the interface to a basic input system built on top of the
/// abstractions for keyboards, mice and joysticks provided by GLFW, but it
/// should be fairly easy to port to use another underlying system.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LLINPUT_HPP_INCLUDED
#define LLINPUT_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <stddef.h>
#include <stdint.h>

#ifndef  LLINPUT_EXTERNAL_GLFW
#include "GLFW/glfw3.h"
#endif

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
/// @summary Define the maximum number of concurrent input contexts. Since we
/// don't want to stuff the context in the window's 'extra' data, because the
/// user might need that slot, we have to keep a global list of context objects.
#ifndef LLINPUT_MAX_CONTEXTS
#define LLINPUT_MAX_CONTEXTS           4
#endif

/// @summary Define the number of uin32_t values required to store the keyboard
/// state data. One bit corresponds to each key.
#define LLINPUT_KEY_WORDS              10U

/// @summary The key code offset value. This is subtracted from the key identifier
/// before indexing into the key state array.
#define LLINPUT_KEY_OFFSET             32

/// @summary Define the maximum number of buttons supported on a mouse.
#define LLINPUT_MAX_MOUSE_BUTTONS      32

/// @summary Define the maximum number of axis inputs supported on a joystick.
#define LLINPUT_MAX_CONTROLLER_AXES     4

/// @summary Define the maximum number of button inputs supported on a joystick.
#define LLINPUT_MAX_CONTROLLER_BUTTONS 32

/// @summary Define the maximum number of joystick controllers.
#define LLINPUT_MAX_CONTROLLERS        16

/*/////////////////
//   Data Types  //
/////////////////*/
#define KW   LLINPUT_KEY_WORDS              /// Alias for line length below
#define MC   LLINPUT_MAX_CONTROLLERS        /// Alias for line length below
#define MA   LLINPUT_MAX_CONTROLLER_AXES    /// Alias for line length below
#define MB   LLINPUT_MAX_CONTROLLER_BUTTONS /// Alias for line length below

/// @summary Internal data associated with the input system attached to a window.
struct context_t
{
    GLFWwindow  *Window;                    /// The window associated with the context.
    float        ScaleX;                    /// Horizontal scale from client->pixels.
    float        ScaleY;                    /// Vertical scale from client->pixels.
    float        MouseX;                    /// Mouse position, in pixels, relative to client top-left.
    float        MouseY;                    /// Mouse position, in pixels, relative to client top-left.
    uint32_t     MouseState;                /// One bit set for each button down.
    uint32_t     MouseModifiers;            /// Combination of GLFW_MOD_x, or zero.
    uint32_t     KeyboardModifiers;         /// Combination of GLFW_MOD_x, or zero.
    uint32_t     KeyboardState[KW];         /// One bit set for each key down.
};

/// @summary Represents a snapshot of state for all input devices at a single
/// point in time. Several of these values are copied from the context.
struct snapshot_t
{
    GLFWwindow  *Window;                    /// The window associated with the context.
    double       Time;                      /// The time at which the snapshot was taken, in seconds.
    float        ScaleX;                    /// Horizontal scale from client->pixels.
    float        ScaleY;                    /// Vertical scale from client->pixels.
    float        MouseX;                    /// Mouse position, in pixels, relative to client top-left.
    float        MouseY;                    /// Mouse position, in pixels, relative to client top-left.
    uint32_t     MouseState;                /// One bit set for each button down.
    uint32_t     MouseModifiers;            /// Combination of GLFW_MOD_x, or zero.
    uint32_t     KeyboardModifiers;         /// Combination of GLFW_MOD_x, or zero.
    uint32_t     KeyboardState[KW];         /// One bit set for each key down.
    size_t       ControllerCount;           /// The number of controllers attached to the system.
    int          ControllerIds[MC];         /// The unique identifiers for each controller.
    size_t       ControllerAxisCount[MC];   /// The number of axis values for each controller.
    float        ControllerAxes[MC][MA];    /// The current axis values for each controller.
    size_t       ControllerButtonCount[MC]; /// The number of button values for each controller.
    uint8_t      ControllerButtons[MC][MB]; /// The current button values for each controller.
};

/// @summary Define the data associated with events reported for a single controller.
struct controller_ev_t
{
    size_t       AxisCount;                 /// The number of valid axes.
    float        AxisValues[MA];            /// The current value for each axis.
    float        AxisDeltas[MA];            /// The change in each axis value.
    size_t       ButtonCount;               /// The number of valid button states.
    uint8_t      ButtonStates[MB];          /// The current state of each button.
    int16_t      ButtonDeltas[MB];          /// The change in button status, in [-255, +255].
};

/// @summary Describes the set of input events generated by comparing two input snapshots.
struct events_t
{
    double       Time;                      /// The time of the latest snapshot, in seconds.
    double       TimeDelta;                 /// The time delta between snapshots, in seconds.
    float        MousePosition[2];          /// Mouse position X/Y in pixels.
    float        MouseDelta[2];             /// Mouse delta X/Y in pixels.
    uint32_t     MouseChanges;              /// State change events for each mouse button.
    uint32_t     MouseStates;               /// Current state of each mouse button.
    uint32_t     KeyChanges[KW];            /// State change events for each key.
    uint32_t     KeyStates [KW];            /// Current state of each key.
    uint32_t     ControllerConnect;         /// Bitmap indicating controller connections.
    uint32_t     ControllerDisconnect;      /// Bitmap indicating controller disconnections.
    size_t       ControllerCount;           /// The number of connected controllers.
    int          ControllerIds[MC];         /// The unique identifier for each connected controller.
    input::controller_ev_t Controller[MC];  /// State and events for each connected controller.
};

#undef MB  // LLINPUT_MAX_CONTROLLER_BUTTONS alias for line length above
#undef MA  // LLINPUT_MAX_CONTROLLER_AXES alias for line length above
#undef MC  // LLINPUT_MAX_CONTROLLERS alias for line length above
#undef KW  // LLINPUT_KEY_WORDS alias for line length above

/*////////////////
//   Functions  //
////////////////*/
/// @summary Creates a new input context and attached input event handlers to
/// the given GLFW window.
/// @param context The context to initialize.
/// @param window The window to monitor for input events.
/// @return true if the context was initialized.
bool create_context(input::context_t *context, GLFWwindow *window);

/// @summary Deletes an input context and detaches it from its associated window.
/// @param context The input context to delete.
void delete_context(input::context_t *context);

/// @summary Grabs a snapshot of input device state for the specified window.
/// @param dst The snapshot structure to populate.
/// @param window The window whose input state is being queried.
void snapshot(input::snapshot_t *dst, input::context_t *context);

/// @summary Given two input snapshots, s0 and s1 (s0 occurring before s1),
/// generates a description of input device events and deltas. The resulting
/// event description can then be queried to determine whether a key or button
/// was pressed during the tick s0->s1.
/// @param ev The event structure to populate.
/// @param s0 The input snapshot at time t=0.
/// @param s1 The input snapshot at time t=1.
void events(input::events_t *ev, input::snapshot_t const *s0, input::snapshot_t const *s1);

/*////////////////////////
//   Inline Functions   //
////////////////////////*/
/// @summary Determine whether a key is currently not pressed.
/// @param ev The set of input events to query.
/// @param key The unique identifier of the key (GLFW_KEY_x.)
/// @return true if the key currently not pressed.
static inline bool is_key_up(input::events_t const *ev, int key)
{
    size_t   word = (key - LLINPUT_KEY_OFFSET) >> 5;
    uint32_t bitx = (key - LLINPUT_KEY_OFFSET) & 0x1F;
    uint32_t mask = (1  << bitx);
    return  (ev->KeyStates[word] & mask) == 0;
}

/// @summary Determine whether a key is currently pressed.
/// @param ev The set of input events to query.
/// @param key The unique identifier of the key (GLFW_KEY_x.)
/// @return true if the key currently pressed.
static inline bool is_key_down(input::events_t const *ev, int key)
{
    size_t   word = (key - LLINPUT_KEY_OFFSET) >> 5;
    uint32_t bitx = (key - LLINPUT_KEY_OFFSET) & 0x1F;
    uint32_t mask = (1  << bitx);
    return  (ev->KeyStates[word] & mask) != 0;
}

/// @summary Determine whether a key was pressed during a tick.
/// @param ev The set of input events to query.
/// @param key The unique identifier of the key (GLFW_KEY_x.)
/// @return true if the key was just pressed.
static inline bool was_key_pressed(input::events_t const *ev, int key)
{
    size_t   word = (key - LLINPUT_KEY_OFFSET) >> 5;
    uint32_t bitx = (key - LLINPUT_KEY_OFFSET) & 0x1F;
    uint32_t mask = (1  << bitx);
    return ((ev->KeyChanges[word] & ev->KeyStates[word]) & mask) != 0;
}

/// @summary Determine whether a key was released during a tick.
/// @param ev The set of input events to query.
/// @param key The unique identifier of the key (GLFW_KEY_x.)
/// @return true if the key was just released.
static inline bool was_key_released(input::events_t const *ev, int key)
{
    size_t   word = (key - LLINPUT_KEY_OFFSET) >> 5;
    uint32_t bitx = (key - LLINPUT_KEY_OFFSET) & 0x1F;
    uint32_t mask = (1  << bitx);
    return ((ev->KeyChanges[word] & ~ev->KeyStates[word]) & mask) != 0;
}

/// @summary Determine whether a mouse button is currently not pressed.
/// @param ev The set of input events to query.
/// @param button The unique identifier of the mouse button (GLFW_MOUSE_BUTTON_x.)
/// @return true if the mouse button is currently not pressed.
static inline bool is_mouse_button_up(input::events_t const *ev, int button)
{
    uint32_t mask = (1 << button);
    return  (ev->MouseStates & mask) == 0;
}

/// @summary Determine whether a mouse button is currently pressed.
/// @param ev The set of input events to query.
/// @param button The unique identifier of the mouse button (GLFW_MOUSE_BUTTON_x.)
/// @return true if the mouse button is currently pressed.
static inline bool is_mouse_button_down(input::events_t const *ev, int button)
{
    uint32_t mask = (1 << button);
    return  (ev->MouseStates & mask) != 0;
}

/// @summary Determine whether a mouse button was pressed during a tick.
/// @param ev The set of input events to query.
/// @param button The unique identifier of the mouse button (GLFW_MOUSE_BUTTON_x.)
/// @return true if the mouse button was just pressed.
static inline bool was_mouse_button_pressed(input::events_t const *ev, int button)
{
    uint32_t mask = (1 << button);
    return ((ev->MouseChanges &  ev->MouseStates) & mask) != 0;
}

/// @summary Determine whether a mouse button was released during a tick.
/// @param ev The set of input events to query.
/// @param button The unique identifier of the mouse button (GLFW_MOUSE_BUTTON_x.)
/// @return true if the mouse button was just released.
static inline bool was_mouse_button_released(input::events_t const *ev, int button)
{
    uint32_t mask = (1 << button);
    return ((ev->MouseChanges & ~ev->MouseStates) & mask) != 0;
}

/// @summary Determine whether a controller is currently connected to the system.
/// @param ev The set of input events to query.
/// @param id The unique identifier of the controller (GLFW_JOYSTICK_x.)
/// @return true if the controller is currently connected.
static inline bool is_controller_connected(input::events_t const *ev, int id)
{
    size_t const n = ev->ControllerCount;
    int const *ids = ev->ControllerIds;
    for (size_t  i = 0; i < n; ++i)
    {
        if (ids[i] == id)
            return true;
    }
    return false;
}

/// @summary Determine whether a controller was connected during a tick.
/// @param ev The set of input events to query.
/// @param id The unique identifier of the controller (GLFW_JOYSTICK_x.)
/// @return true if the controller was just connected.
static inline bool was_controller_connected(input::events_t const *ev, int id)
{
    uint32_t mask = (1 << id);
    return  (ev->ControllerConnect & mask) != 0;
}

/// @summary Determine whether a controller was disconnected during a tick.
/// @param ev The set of input events to query.
/// @param id The unique identifier of the controller (GLFW_JOYSTICK_x.)
/// @return true if the controller was just disconnected.
static inline bool was_controller_disconnected(input::events_t const *ev, int id)
{
    uint32_t mask = (1 << id);
    return  (ev->ControllerDisconnect & mask) != 0;
}

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace input */

#endif /* !defined(LLINPUT_HPP_INCLUDED) */

