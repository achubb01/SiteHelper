#ifndef RENDERER2D_SDL_H
#define RENDERER2D_SDL_H

#include "renderer2d.h"
#include "renderer2d_backend.h"


RendererBackend renderer2d_sdl_create_backend(
    const char *title,
    int width,
    int height
);

void renderer2d_sdl_destroy_backend(
    RendererBackend *backend
);

int renderer2d_sdl_process_events(void);

int renderer2d_sdl_poll_event(
    Renderer2DEvent *event
);

#endif