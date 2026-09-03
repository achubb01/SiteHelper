#ifndef PLATFORM_EVENT_SDL_INTERNAL_H
#define PLATFORM_EVENT_SDL_INTERNAL_H

#include <SDL3/SDL.h>

#include "platform_event.h"

void platform_event_sdl_translate(
    const SDL_Event *sdl_event,
    PlatformEvent *event
);

#endif
