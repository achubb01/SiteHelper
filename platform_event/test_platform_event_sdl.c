#include <assert.h>
#include <stdio.h>

#include <SDL3/SDL.h>

#include "platform_event.h"
#include "platform_event_sdl_internal.h"

static void test_quit_event(void)
{
    SDL_Event sdl_event = {
        .type = SDL_EVENT_QUIT
    };
    PlatformEvent event;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.type == PLATFORM_EVENT_QUIT);
}

static void test_resize_event(void)
{
    SDL_Event sdl_event = {
        .type = SDL_EVENT_WINDOW_RESIZED
    };

    sdl_event.window.data1 = 1280;
    sdl_event.window.data2 = 720;

    PlatformEvent event;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.type == PLATFORM_EVENT_WINDOW_RESIZED);
    assert(event.data.window_resized.width == 1280.0);
    assert(event.data.window_resized.height == 720.0);
}

static void test_key_events(void)
{
    SDL_Event sdl_event = {
        .type = SDL_EVENT_KEY_DOWN
    };

    sdl_event.key.key = SDLK_LEFT;

    PlatformEvent event;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.type == PLATFORM_EVENT_KEY_DOWN);
    assert(event.data.key_down.key == PLATFORM_KEY_LEFT);
    assert(event.data.key_down.modifiers == PLATFORM_MODIFIER_NONE);
    assert(!event.data.key_down.repeat);

    sdl_event.key.key = SDLK_TAB;
    platform_event_sdl_translate(&sdl_event, &event);
    assert(event.data.key_down.key == PLATFORM_KEY_TAB);

    sdl_event.key.key = SDLK_Z;
    sdl_event.key.mod = SDL_KMOD_CTRL;
    sdl_event.key.repeat = 0;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.data.key_down.key == PLATFORM_KEY_Z);
    assert(event.data.key_down.modifiers & PLATFORM_MODIFIER_CTRL);
    assert(!event.data.key_down.repeat);

    sdl_event.key.key = SDLK_Y;
    sdl_event.key.mod = SDL_KMOD_CTRL;
    sdl_event.key.repeat = 0;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.data.key_down.key == PLATFORM_KEY_Y);
    assert(event.data.key_down.modifiers & PLATFORM_MODIFIER_CTRL);
    assert(!(event.data.key_down.modifiers & PLATFORM_MODIFIER_SHIFT));
    assert(!event.data.key_down.repeat);

    sdl_event.key.key = SDLK_Z;
    sdl_event.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    sdl_event.key.repeat = 1;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.data.key_down.key == PLATFORM_KEY_Z);
    assert(event.data.key_down.modifiers & PLATFORM_MODIFIER_CTRL);
    assert(event.data.key_down.modifiers & PLATFORM_MODIFIER_SHIFT);
    assert(event.data.key_down.repeat);
}

static void test_mouse_motion_event(void)
{
    SDL_Event sdl_event = {
        .type = SDL_EVENT_MOUSE_MOTION
    };

    sdl_event.motion.x = 120.0f;
    sdl_event.motion.y = 75.0f;
    sdl_event.motion.xrel = -4.0f;
    sdl_event.motion.yrel = 9.0f;
    sdl_event.motion.state = SDL_BUTTON_MMASK;

    PlatformEvent event;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.type == PLATFORM_EVENT_MOUSE_MOTION);
    assert(event.data.mouse_motion.x == 120.0);
    assert(event.data.mouse_motion.y == 75.0);
    assert(event.data.mouse_motion.delta_x == -4.0);
    assert(event.data.mouse_motion.delta_y == 9.0);
    assert(
        event.data.mouse_motion.held_buttons
        & PLATFORM_MOUSE_BUTTON_STATE_MIDDLE
    );
}

static void test_mouse_button_event(void)
{
    SDL_Event sdl_event = {
        .type = SDL_EVENT_MOUSE_BUTTON_DOWN
    };

    sdl_event.button.button = SDL_BUTTON_LEFT;
    sdl_event.button.x = 20.0f;
    sdl_event.button.y = 35.0f;

    PlatformEvent event;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.type == PLATFORM_EVENT_MOUSE_BUTTON_DOWN);
    assert(
        event.data.mouse_button.button
        == PLATFORM_MOUSE_BUTTON_PRIMARY
    );
    assert(event.data.mouse_button.x == 20.0);
    assert(event.data.mouse_button.y == 35.0);

    sdl_event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    sdl_event.button.button = SDL_BUTTON_MIDDLE;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.type == PLATFORM_EVENT_MOUSE_BUTTON_UP);
    assert(
        event.data.mouse_button.button
        == PLATFORM_MOUSE_BUTTON_MIDDLE
    );
}

static void test_mouse_wheel_event(void)
{
    SDL_Event sdl_event = {
        .type = SDL_EVENT_MOUSE_WHEEL
    };

    sdl_event.wheel.x = 2.0f;
    sdl_event.wheel.y = -3.0f;
    sdl_event.wheel.mouse_x = 440.0f;
    sdl_event.wheel.mouse_y = 320.0f;

    PlatformEvent event;

    platform_event_sdl_translate(&sdl_event, &event);

    assert(event.type == PLATFORM_EVENT_MOUSE_WHEEL);
    assert(event.data.mouse_wheel.delta_x == 2.0);
    assert(event.data.mouse_wheel.delta_y == -3.0);
    assert(event.data.mouse_wheel.mouse_x == 440.0);
    assert(event.data.mouse_wheel.mouse_y == 320.0);
}

int main(void)
{
    test_quit_event();
    test_resize_event();
    test_key_events();
    test_mouse_motion_event();
    test_mouse_button_event();
    test_mouse_wheel_event();

    printf("All platform event SDL tests passed.\n");

    return 0;
}
