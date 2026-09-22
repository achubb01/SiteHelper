#ifndef RENDERER2D_BACKEND_H
#define RENDERER2D_BACKEND_H

#include "../geometry/geometry.h"

typedef struct Colour {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Colour;

/* Geometric backend arguments are screen pixels, already transformed by
 * renderer2d; no physical construction-unit interpretation occurs here. */
typedef void (*BackendClearFn)(
    void *context,
    Colour colour
);

typedef void (*BackendDrawRectFn)(
    void *context,
    Rect2 rect,
    Colour colour
);

typedef void (*BackendFillRectFn)(
    void *context,
    Rect2 rect,
    Colour colour
);

typedef void (*BackendDrawLineFn)(
    void *context,
    Vec2 start,
    Vec2 end,
    Colour colour
);

typedef void (*BackendFillTriangleFn)(
    void *context,
    Vec2 a, Vec2 b, Vec2 c,
    Colour colour
);

typedef void (*BackendPresentFn)(
    void *context
);

typedef void (*BackendSetClipRectFn)(
    void *context,
    Rect2 rect
);

typedef void (*BackendClearClipRectFn)(
    void *context
);

typedef struct RendererBackend {
    void *context;

    BackendClearFn clear;
    BackendDrawRectFn draw_rect;
    BackendFillRectFn fill_rect;
    BackendDrawLineFn draw_line;
    BackendFillTriangleFn fill_triangle;

    BackendSetClipRectFn set_clip_rect;
    BackendClearClipRectFn clear_clip_rect;

    BackendPresentFn present;
    /* Optional temporary debug typography; screen pixels, no camera transform. */
    void (*draw_screen_text)(void *context, Vec2 position, const char *text, Colour colour);
} RendererBackend;

#endif