/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements an interface to the input system based on the
/// abstractions for keyboards, mice and joysticks provided by GLFW. This
/// shouldn't be too hard to port to another underlying system.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include "llinput.hpp"

/*/////////////////
//   Constants   //
/////////////////*/

/*///////////////
//   Globals   //
///////////////*/
/// @summary A global table of all input contexts. Nasty, but GLFW doesn't
/// allow us to pass through a context pointer in the callbacks, and we don't
/// want to take over the user pointer on the window.
static input::context_t* gContextList[LLINPUT_MAX_CONTEXTS] = {NULL};

/// @summary A global table of GLFW window handles associated with input contexts.
static GLFWwindow*       gWindowList[LLINPUT_MAX_CONTEXTS]  = {NULL};

/// @summary The number of valid input context records.
static size_t            gContextCount = 0;

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Finds the input context attached to a given window.
/// @param win The window associated with the input context.
/// @return The input context record allocated to the window, or NULL.
static input::context_t* find_context(GLFWwindow *win)
{
    size_t const n = gContextCount;
    for (size_t  i = 0; i < n; ++i)
    {
        if (gWindowList[i] == win)
            return gContextList[i];
    }
    return NULL;
}

/// @summary Generates a bitmap where a bit is set if the associated controller is connected.
/// @param state The input state snapshot to generate the bitmap for.
/// @return The bitmap representing the set of connected controllers.
static uint32_t controller_bitmap(input::snapshot_t const *state)
{
    uint32_t bitmap = 0;
    for (size_t i = 0; i < state->ControllerCount; ++i)
    {
        bitmap |= (1 << state->ControllerIds[i]);
    }
    return bitmap;
}

/// @summary Callback invoked by GLFW to report the current cursor position.
/// @param win The window reporting with the event.
/// @param x The current position of the mouse cursor, relative to the window client area.
/// @param y The current position of the mouse cursor, relative to the window client area.
static void glfw_cursor(GLFWwindow *win, double x, double y)
{
    input::context_t *ctx = find_context(win);
    if (ctx != NULL)
    {
        ctx->MouseX = float(x) * ctx->ScaleX;
        ctx->MouseY = float(y) * ctx->ScaleY;
    }
}

/// @summary Callback invoked by GLFW to report that a mouse button was pressed or released.
/// @param win The window reporting the event.
/// @param button The button identifier.
/// @param action One of GLFW_PRESS or GLFW_RELEASE.
/// @param modifiers A combination of GLFW_MOD_x flags, or zero, indicating which modifiers are active.
static void glfw_button(GLFWwindow *win, int button, int action, int modifiers)
{
    input::context_t *ctx = find_context(win);
    if (ctx != NULL)
    {
        if (action == GLFW_PRESS)
        {
            ctx->MouseState    |= (1 << button);
            ctx->MouseModifiers = modifiers;
        }
        else
        {
            ctx->MouseState    &=~(1 << button);
            ctx->MouseModifiers = 0;
        }
    }
}

/// @summary Callback invoked by GLFW to report a keyboard event.
/// @param win The window reporting the event.
/// @param key The GLFW key identifier.
/// @param scancode The raw key scancode reported by the system.
/// @param action One of GLFW_PRESS, GLFW_RELEASE or GLFW_REPEAT (ignored).
/// @param modifiers A combination of GLFW_MOD_x flags, or zero, indicating which modifiers are active.
static void glfw_key(GLFWwindow *win, int key, int scancode, int action, int modifiers)
{
    input::context_t *ctx = find_context(win);
    if (ctx != NULL)
    {
        size_t   word = (key - LLINPUT_KEY_OFFSET) >> 5;
        uint32_t bitx = (key - LLINPUT_KEY_OFFSET) & 0x1F;

        if (action == GLFW_PRESS)
        {
            ctx->KeyboardState[word] |= (1 << bitx);
            ctx->KeyboardModifiers    = modifiers;
        }
        else if (action == GLFW_RELEASE)
        {
            ctx->KeyboardState[word] &=~(1 << bitx);
            ctx->KeyboardModifiers    = 0;
        }
    }
}

/*///////////////////////
//  Public Functions   //
///////////////////////*/
bool input::create_context(input::context_t *context, GLFWwindow *window)
{
    input::context_t *ctx = find_context(window);
    if (ctx)
    {
        // there's already a context attached to this window.
        // this is only okay if it's the same as the one being attached.
        return (ctx == context);
    }

    if (gContextCount == LLINPUT_MAX_CONTEXTS)
    {
        // no additional contexts can be attached.
        return false;
    }

    size_t const index  = gContextCount++;
    gContextList[index] = context;
    gWindowList[index]  = window;

    // to support HiDPI devices, we need to be able to calculate
    // the scale factor from window client space to pixels.
    int win_w  = 0;
    int win_h  = 0;
    int buf_w  = 0;
    int buf_h  = 0;
    glfwGetWindowSize(window, &win_w, &win_h);
    glfwGetFramebufferSize(window, &buf_w, &buf_h);

    // initialize the input context to its default state.
    context->Window            = window;
    context->ScaleX            = float(buf_w) / float(win_w);
    context->ScaleY            = float(buf_h) / float(win_h);
    context->MouseX            = 0;
    context->MouseY            = 0;
    context->MouseState        = 0;
    context->MouseModifiers    = 0;
    context->KeyboardModifiers = 0;
    for (size_t i = 0; i < LLINPUT_KEY_WORDS; ++i)
        context->KeyboardState[i] = 0;

    glfwSetKeyCallback(window, glfw_key);
    glfwSetCursorPosCallback(window, glfw_cursor);
    glfwSetMouseButtonCallback(window, glfw_button);
    return true;
}

void input::delete_context(input::context_t *context)
{
    bool   found = false;
    size_t index = 0;
    size_t const n = gContextCount;
    for (size_t  i = 0; i < n; ++i)
    {
        if (gContextList[i] == context)
        {
            index = i;
            found = true;
            break;
        }
    }

    glfwSetKeyCallback(gWindowList[index], NULL);
    glfwSetCursorPosCallback(gWindowList[index], NULL);
    glfwSetMouseButtonCallback(gWindowList[index], NULL);
    gWindowList [index] = gWindowList[gContextCount  - 1];
    gContextList[index] = gContextList[gContextCount - 1];
    gContextCount--;
}

void input::snapshot(input::snapshot_t *dst, input::context_t *context)
{
    // copy event-driven attributes from the context.
    dst->Window            = context->Window;
    dst->Time              = glfwGetTime();
    dst->ScaleX            = context->ScaleX;
    dst->ScaleY            = context->ScaleY;
    dst->MouseX            = context->MouseX;
    dst->MouseY            = context->MouseY;
    dst->MouseState        = context->MouseState;
    dst->MouseModifiers    = context->MouseModifiers;
    dst->KeyboardModifiers = context->KeyboardModifiers;
    for (size_t i = 0; i < LLINPUT_KEY_WORDS; ++i)
        dst->KeyboardState[i] = context->KeyboardState[i];

    // poll for the current joystick state.
    size_t  ncontrollers = 0;
    dst->ControllerCount = 0;
    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; ++i)
    {
        if (glfwJoystickPresent(i) == GL_TRUE)
        {
            int naxes    = 0;
            int nbuttons = 0;
            float   const *axes    = glfwGetJoystickAxes(i, &naxes);
            uint8_t const *buttons = glfwGetJoystickButtons(i, &nbuttons);

            if (naxes > LLINPUT_MAX_CONTROLLER_AXES)
                naxes = LLINPUT_MAX_CONTROLLER_AXES;

            if (nbuttons > LLINPUT_MAX_CONTROLLER_BUTTONS)
                nbuttons = LLINPUT_MAX_CONTROLLER_BUTTONS;

            dst->ControllerIds[ncontrollers] = i;
            dst->ControllerAxisCount[ncontrollers] = size_t(naxes);
            dst->ControllerButtonCount[ncontrollers] = size_t(nbuttons);

            for (int j = 0; j < naxes; ++j)
            {
                dst->ControllerAxes[ncontrollers][j] = axes[j];
            }
            for (int j = 0; j < nbuttons; ++j)
            {
                dst->ControllerButtons[ncontrollers][j] = buttons[j];
            }
            ncontrollers++;

            if (ncontrollers == LLINPUT_MAX_CONTROLLERS)
                break;
        }
    }
    dst->ControllerCount = ncontrollers;
}

void input::events(input::events_t *ev, input::snapshot_t const *s0, input::snapshot_t const *s1)
{
    double dt = 0.0;

    if (s0->Time < s1->Time)
        dt = s1->Time - s0->Time;
    else
        dt = s0->Time - s1->Time;

    ev->Time             = s1->Time;
    ev->TimeDelta        = dt;
    ev->MousePosition[0] = s1->MouseX;
    ev->MousePosition[1] = s1->MouseY;
    ev->MouseDelta[0]    = s1->MouseX - s0->MouseX;
    ev->MouseDelta[1]    = s1->MouseY - s0->MouseY;
    ev->MouseChanges     = s1->MouseState ^ s0->MouseState;
    ev->MouseStates      = s1->MouseState;
    for (size_t i = 0; i < LLINPUT_KEY_WORDS; ++i)
    {
        ev->KeyChanges[i]  = s1->KeyboardState[i] ^ s0->KeyboardState[i];
        ev->KeyStates[i]   = s1->KeyboardState[i];
    }

    uint32_t curr_connected    = controller_bitmap(s1);
    uint32_t prev_connected    = controller_bitmap(s0);
    uint32_t connected_changes = (curr_connected    ^  prev_connected);
    ev->ControllerConnect      = (connected_changes &  curr_connected);
    ev->ControllerDisconnect   = (connected_changes & ~curr_connected);
    ev->ControllerCount        = s1->ControllerCount;

    int    const *id0 = s0->ControllerIds;
    int    const *id1 = s1->ControllerIds;
    size_t const   n0 = s0->ControllerCount;
    size_t const   n1 = s1->ControllerCount;
    for (size_t i = 0;  i < n1; ++i)
    {   // find controller s1->ControllerIds[i] in s0.
        int const  c1 = id1[i];
        size_t  index = 0;
        bool    found = false;
        for (size_t j = 0; j < n0; ++j)
        {
            if (id0[j] == c1)
            {
                index = j;
                found = true;
                break;
            }
        }

        input::controller_ev_t &cev = ev->Controller[i];
        ev->ControllerIds[i] = c1;
        cev.AxisCount        = s1->ControllerAxisCount[i];
        cev.ButtonCount      = s1->ControllerButtonCount[i];
        if (found)
        {   // calculate delta values.
            for (size_t k = 0; k < cev.AxisCount; ++k)
            {
                cev.AxisValues[k]  = s1->ControllerAxes[i][k];
                cev.AxisDeltas[k]  = s1->ControllerAxes[i][k] - s0->ControllerAxes[index][k];
            }
            for (size_t k = 0; k < cev.ButtonCount; ++k)
            {
                cev.ButtonStates[k]= s1->ControllerButtons[i][k];
                cev.ButtonDeltas[k]= s1->ControllerButtons[i][k] - s0->ControllerButtons[index][k];
            }
        }
        else
        {   // just copy the current values.
            for (size_t k = 0; k < cev.AxisCount; ++k)
            {
                cev.AxisValues[k]  = s1->ControllerAxes[i][k];
                cev.AxisDeltas[k]  = 0.0;
            }
            for (size_t k = 0; k < cev.ButtonCount; ++k)
            {
                cev.ButtonStates[k]= s1->ControllerButtons[i][k];
                cev.ButtonDeltas[k]= 0;
            }
        }
    }
}
