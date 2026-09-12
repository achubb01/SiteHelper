#include <SDL3/SDL.h>
#include <stdlib.h>

#include "renderer2d_sdl.h"

typedef struct SDLBackendState {
    SDL_Window *window;
    SDL_Renderer *renderer;
} SDLBackendState;

/* Temporary numeric HUD only: SDL's built-in ASCII debug font. Replace this
 * callback when production typography arrives; input semantics are independent. */
static void sdl_draw_screen_text(void *context, Vec2 position, const char *text, Colour colour)
{
    SDLBackendState *state = context;
    SDL_SetRenderDrawColor(state->renderer, colour.r, colour.g, colour.b, colour.a);
    SDL_RenderDebugText(state->renderer, (float)position.x, (float)position.y, text);
}

int renderer2d_sdl_set_text_input(RendererBackend *backend, int enabled)
{
    if (backend == NULL || backend->context == NULL) { return 0; }
    SDLBackendState *state = backend->context;
    if (!!SDL_TextInputActive(state->window) == !!enabled) { return 1; }
    return enabled ? SDL_StartTextInput(state->window) : SDL_StopTextInput(state->window);
}

static void sdl_set_clip_rect(
    void *context,
    Rect2 rect
)
{
    SDLBackendState *state = context;

    if (
        state == NULL
        || state->renderer == NULL
    ) {
        return;
    }

    SDL_Rect clip_rect = {
        .x = (int)rect.position.x,
        .y = (int)rect.position.y,
        .w = (int)rect.width,
        .h = (int)rect.height
    };

    SDL_SetRenderClipRect(
        state->renderer,
        &clip_rect
    );
}

static void sdl_clear_clip_rect(
    void *context
)
{
    SDLBackendState *state = context;

    if (
        state == NULL
        || state->renderer == NULL
    ) {
        return;
    }

    SDL_SetRenderClipRect(
        state->renderer,
        NULL
    );
}

static void sdl_draw_rect(
    void *context,
    Rect2 rect,
    Colour colour
)
{
    if (context == NULL) {
        return;
    }

    SDLBackendState *state = context;

    SDL_FRect sdl_rect = {
        .x = (float)rect.position.x,
        .y = (float)rect.position.y,
        .w = (float)rect.width,
        .h = (float)rect.height
    };

    SDL_SetRenderDrawColor(
        state->renderer,
        colour.r,
        colour.g,
        colour.b,
        colour.a
    );

    SDL_RenderRect(
        state->renderer,
        &sdl_rect
    );
}

static void sdl_fill_rect(
    void *context,
    Rect2 rect,
    Colour colour
)
{
    if (context == NULL) {
        return;
    }

    SDLBackendState *state = context;

    SDL_FRect sdl_rect = {
        .x = (float)rect.position.x,
        .y = (float)rect.position.y,
        .w = (float)rect.width,
        .h = (float)rect.height
    };

    SDL_SetRenderDrawColor(
        state->renderer,
        colour.r,
        colour.g,
        colour.b,
        colour.a
    );

    SDL_RenderFillRect(
        state->renderer,
        &sdl_rect
    );
}

static void sdl_draw_line(
    void *context,
    Vec2 start,
    Vec2 end,
    Colour colour
)
{
    if (context == NULL) {
        return;
    }

    SDLBackendState *state = context;

    SDL_SetRenderDrawColor(
        state->renderer,
        colour.r,
        colour.g,
        colour.b,
        colour.a
    );

    SDL_RenderLine(
        state->renderer,
        (float)start.x,
        (float)start.y,
        (float)end.x,
        (float)end.y
    );
}

static void sdl_clear(
    void *context,
    Colour colour
)
{
    if (context == NULL) {
        return;
    }

    SDLBackendState *state = context;

    SDL_SetRenderDrawColor(
        state->renderer,
        colour.r,
        colour.g,
        colour.b,
        colour.a
    );

    SDL_RenderClear(state->renderer);
}

static void sdl_present(
    void *context
)
{
    if (context == NULL) {
        return;
    }

    SDLBackendState *state = context;

    SDL_RenderPresent(state->renderer);
}



RendererBackend renderer2d_sdl_create_backend(
    const char *title,
    int width,
    int height
)
{
    RendererBackend backend = {0};

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return backend;
    }

    SDLBackendState *state = malloc(sizeof(SDLBackendState));

    if (state == NULL) {
        SDL_Quit();
        return backend;
    }

    if (!SDL_CreateWindowAndRenderer(
            title,
            width,
            height,
            SDL_WINDOW_RESIZABLE,
            &state->window,
            &state->renderer
        )) {
        free(state);
        SDL_Quit();
        return backend;
    }

    backend.context = state;
    backend.clear = sdl_clear;
    backend.draw_rect = sdl_draw_rect;
    backend.fill_rect = sdl_fill_rect;
    backend.draw_line = sdl_draw_line;
    backend.set_clip_rect = sdl_set_clip_rect;
    backend.clear_clip_rect = sdl_clear_clip_rect;
    backend.present = sdl_present;
    backend.draw_screen_text = sdl_draw_screen_text;


    return backend;
}

void renderer2d_sdl_destroy_backend(
    RendererBackend *backend
)
{
    if (
        backend == NULL
        || backend->context == NULL
    ) {
        return;
    }

    SDLBackendState *state =
        backend->context;

    if (state->renderer != NULL) {
        SDL_DestroyRenderer(
            state->renderer
        );
    }

    if (state->window != NULL) {
        SDL_DestroyWindow(
            state->window
        );
    }

    free(state);

    *backend = (RendererBackend){0};

    SDL_Quit();
}
