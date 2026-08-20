#include <SDL3/SDL.h>
#include <stdlib.h>

#include "renderer2d_sdl.h"

typedef struct SDLBackendState {
    SDL_Window *window;
    SDL_Renderer *renderer;
} SDLBackendState;

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

int renderer2d_sdl_process_events(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return 0;
        }
    }

    return 1;
}

int renderer2d_sdl_poll_event(
    Renderer2DEvent *event
)
{
    if (event == NULL) {
        return 0;
    }

    SDL_Event sdl_event;

    if (!SDL_PollEvent(&sdl_event)) {
        return 0;
    }

    *event = (Renderer2DEvent){0};

    if (sdl_event.type == SDL_EVENT_QUIT) {
        event->quit_requested = 1;
    }

    else if (sdl_event.type == SDL_EVENT_WINDOW_RESIZED) {
        event->viewport_resized = 1;

        event->viewport_width =
            (double)sdl_event.window.data1;

        event->viewport_height =
            (double)sdl_event.window.data2;
    }

    else if (sdl_event.type == SDL_EVENT_KEY_DOWN) {

        if (
            sdl_event.key.key == SDLK_Z
            && (sdl_event.key.mod & SDL_KMOD_CTRL)
            && !sdl_event.key.repeat
        ) {
            event->undo_requested = 1;
        }
        else {
            switch (sdl_event.key.key) {
                case SDLK_LEFT:
                    event->move_left = 1;
                    break;

                case SDLK_RIGHT:
                    event->move_right = 1;
                    break;

                case SDLK_UP:
                    event->move_up = 1;
                    break;

                case SDLK_DOWN:
                    event->move_down = 1;
                    break;
            }
        }
    }

    else if (
    sdl_event.type == SDL_EVENT_MOUSE_MOTION
    ) {
        event->mouse_moved = 1;

        event->mouse_x =
            (double)sdl_event.motion.x;

        event->mouse_y =
            (double)sdl_event.motion.y;

        if (
            sdl_event.motion.state
            & SDL_BUTTON_MMASK
        ) {
            event->pan_dragged = 1;

            event->mouse_delta_x =
                (double)sdl_event.motion.xrel;

            event->mouse_delta_y =
                (double)sdl_event.motion.yrel;
        }
    }

    else if (sdl_event.type == SDL_EVENT_MOUSE_WHEEL) {
        event->mouse_wheel = 1;

        event->wheel_y = (double)sdl_event.wheel.y;

        event->mouse_x = (double)sdl_event.wheel.mouse_x;
        event->mouse_y = (double)sdl_event.wheel.mouse_y;
    }

    else if (
        sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
        &&
        sdl_event.button.button == SDL_BUTTON_LEFT
    ) {
        event->primary_mouse_pressed = 1;

        event->mouse_x = (double)sdl_event.button.x;
        event->mouse_y = (double)sdl_event.button.y;
    }
    
    else if (
        sdl_event.type == SDL_EVENT_MOUSE_BUTTON_UP
        &&
        sdl_event.button.button == SDL_BUTTON_LEFT
    ) {
        event->primary_mouse_released = 1;

        event->mouse_x =
            (double)sdl_event.button.x;

        event->mouse_y =
            (double)sdl_event.button.y;
    }

    return 1;
}
