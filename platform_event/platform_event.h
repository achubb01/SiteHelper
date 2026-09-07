#ifndef PLATFORM_EVENT_H
#define PLATFORM_EVENT_H

typedef enum
{
    PLATFORM_EVENT_NONE,
    PLATFORM_EVENT_QUIT,
    PLATFORM_EVENT_WINDOW_RESIZED,
    PLATFORM_EVENT_KEY_DOWN,
    PLATFORM_EVENT_MOUSE_MOTION,
    PLATFORM_EVENT_MOUSE_WHEEL,
    PLATFORM_EVENT_MOUSE_BUTTON_DOWN,
    PLATFORM_EVENT_MOUSE_BUTTON_UP
} PlatformEventType;

typedef enum
{
    PLATFORM_KEY_UNKNOWN,
    PLATFORM_KEY_LEFT,
    PLATFORM_KEY_RIGHT,
    PLATFORM_KEY_UP,
    PLATFORM_KEY_DOWN_ARROW,
    PLATFORM_KEY_Y,
    PLATFORM_KEY_Z,
    PLATFORM_KEY_TAB
} PlatformKey;

typedef enum
{
    PLATFORM_MODIFIER_NONE = 0,
    PLATFORM_MODIFIER_CTRL = 1 << 0,
    PLATFORM_MODIFIER_SHIFT = 1 << 1
} PlatformModifier;

typedef enum
{
    PLATFORM_MOUSE_BUTTON_UNKNOWN,
    PLATFORM_MOUSE_BUTTON_PRIMARY,
    PLATFORM_MOUSE_BUTTON_MIDDLE,
    PLATFORM_MOUSE_BUTTON_SECONDARY
} PlatformMouseButton;

typedef enum
{
    PLATFORM_MOUSE_BUTTON_STATE_NONE = 0,
    PLATFORM_MOUSE_BUTTON_STATE_PRIMARY = 1 << 0,
    PLATFORM_MOUSE_BUTTON_STATE_MIDDLE = 1 << 1,
    PLATFORM_MOUSE_BUTTON_STATE_SECONDARY = 1 << 2
} PlatformMouseButtonState;

typedef struct
{
    PlatformEventType type;

    union
    {
        struct
        {
            double width;
            double height;
        } window_resized;

        struct
        {
            PlatformKey key;
            int modifiers;
            int repeat;
        } key_down;

        struct
        {
            double x;
            double y;
            double delta_x;
            double delta_y;
            int held_buttons;
        } mouse_motion;

        struct
        {
            double delta_x;
            double delta_y;
            double mouse_x;
            double mouse_y;
        } mouse_wheel;

        struct
        {
            PlatformMouseButton button;
            double x;
            double y;
        } mouse_button;
    } data;
} PlatformEvent;

#endif
