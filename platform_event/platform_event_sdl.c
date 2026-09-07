#include <SDL3/SDL.h>

#include "platform_event_sdl.h"
#include "platform_event_sdl_internal.h"

static PlatformKey platform_event_sdl_key(
    SDL_Keycode key
)
{
    switch (key) {
        case SDLK_TAB:
            return PLATFORM_KEY_TAB;

        case SDLK_LEFT:
            return PLATFORM_KEY_LEFT;

        case SDLK_RIGHT:
            return PLATFORM_KEY_RIGHT;

        case SDLK_UP:
            return PLATFORM_KEY_UP;

        case SDLK_DOWN:
            return PLATFORM_KEY_DOWN_ARROW;

        case SDLK_Y:
            return PLATFORM_KEY_Y;

        case SDLK_Z:
            return PLATFORM_KEY_Z;

        default:
            return PLATFORM_KEY_UNKNOWN;
    }
}

static int platform_event_sdl_modifiers(
    SDL_Keymod modifiers
)
{
    int result = PLATFORM_MODIFIER_NONE;

    if (modifiers & SDL_KMOD_CTRL) {
        result |= PLATFORM_MODIFIER_CTRL;
    }

    if (modifiers & SDL_KMOD_SHIFT) {
        result |= PLATFORM_MODIFIER_SHIFT;
    }

    return result;
}

static PlatformMouseButton platform_event_sdl_mouse_button(
    Uint8 button
)
{
    switch (button) {
        case SDL_BUTTON_LEFT:
            return PLATFORM_MOUSE_BUTTON_PRIMARY;

        case SDL_BUTTON_MIDDLE:
            return PLATFORM_MOUSE_BUTTON_MIDDLE;

        case SDL_BUTTON_RIGHT:
            return PLATFORM_MOUSE_BUTTON_SECONDARY;

        default:
            return PLATFORM_MOUSE_BUTTON_UNKNOWN;
    }
}

static int platform_event_sdl_held_buttons(
    SDL_MouseButtonFlags buttons
)
{
    int result = PLATFORM_MOUSE_BUTTON_STATE_NONE;

    if (buttons & SDL_BUTTON_LMASK) {
        result |= PLATFORM_MOUSE_BUTTON_STATE_PRIMARY;
    }

    if (buttons & SDL_BUTTON_MMASK) {
        result |= PLATFORM_MOUSE_BUTTON_STATE_MIDDLE;
    }

    if (buttons & SDL_BUTTON_RMASK) {
        result |= PLATFORM_MOUSE_BUTTON_STATE_SECONDARY;
    }

    return result;
}

void platform_event_sdl_translate(
    const SDL_Event *sdl_event,
    PlatformEvent *event
)
{
    *event = (PlatformEvent){0};

    switch (sdl_event->type) {
        case SDL_EVENT_QUIT:
            event->type = PLATFORM_EVENT_QUIT;
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            event->type = PLATFORM_EVENT_WINDOW_RESIZED;
            event->data.window_resized.width =
                (double)sdl_event->window.data1;
            event->data.window_resized.height =
                (double)sdl_event->window.data2;
            break;

        case SDL_EVENT_KEY_DOWN:
            event->type = PLATFORM_EVENT_KEY_DOWN;
            event->data.key_down.key =
                platform_event_sdl_key(sdl_event->key.key);
            event->data.key_down.modifiers =
                platform_event_sdl_modifiers(sdl_event->key.mod);
            event->data.key_down.repeat =
                sdl_event->key.repeat ? 1 : 0;
            break;

        case SDL_EVENT_MOUSE_MOTION:
            event->type = PLATFORM_EVENT_MOUSE_MOTION;
            event->data.mouse_motion.x =
                (double)sdl_event->motion.x;
            event->data.mouse_motion.y =
                (double)sdl_event->motion.y;
            event->data.mouse_motion.delta_x =
                (double)sdl_event->motion.xrel;
            event->data.mouse_motion.delta_y =
                (double)sdl_event->motion.yrel;
            event->data.mouse_motion.held_buttons =
                platform_event_sdl_held_buttons(
                    sdl_event->motion.state
                );
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            event->type = PLATFORM_EVENT_MOUSE_WHEEL;
            event->data.mouse_wheel.delta_x =
                (double)sdl_event->wheel.x;
            event->data.mouse_wheel.delta_y =
                (double)sdl_event->wheel.y;
            event->data.mouse_wheel.mouse_x =
                (double)sdl_event->wheel.mouse_x;
            event->data.mouse_wheel.mouse_y =
                (double)sdl_event->wheel.mouse_y;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            event->type = PLATFORM_EVENT_MOUSE_BUTTON_DOWN;
            event->data.mouse_button.button =
                platform_event_sdl_mouse_button(
                    sdl_event->button.button
                );
            event->data.mouse_button.x =
                (double)sdl_event->button.x;
            event->data.mouse_button.y =
                (double)sdl_event->button.y;
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            event->type = PLATFORM_EVENT_MOUSE_BUTTON_UP;
            event->data.mouse_button.button =
                platform_event_sdl_mouse_button(
                    sdl_event->button.button
                );
            event->data.mouse_button.x =
                (double)sdl_event->button.x;
            event->data.mouse_button.y =
                (double)sdl_event->button.y;
            break;
    }
}

int platform_event_sdl_poll_event(
    PlatformEvent *event
)
{
    if (event == NULL) {
        return 0;
    }

    SDL_Event sdl_event;

    if (!SDL_PollEvent(&sdl_event)) {
        return 0;
    }

    platform_event_sdl_translate(
        &sdl_event,
        event
    );

    return 1;
}
