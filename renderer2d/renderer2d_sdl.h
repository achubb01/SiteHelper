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

/* This SDL adapter already owns the native window. Call on the main thread.
 * Idempotent; returns zero on SDL failure. No native objects escape. */
int renderer2d_sdl_set_text_input(RendererBackend *backend, int enabled);

#endif
