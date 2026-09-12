#include "app_input_hud.h"
#include <string.h>

void app_input_draw_hud(Renderer2D *renderer, const AppInput *input, Rect2 viewport)
{
    if (renderer != NULL && input != NULL && input->focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH && viewport.width > 24) {
        const TextEdit *edit = &input->text;
        char display[TEXT_EDIT_CAPACITY];
        size_t count = 0, cursor = 0;
        for (size_t i = 0; i < edit->length; i++) {
            unsigned char c = (unsigned char)edit->text[i];
            if ((c & 0xc0) == 0x80) { continue; }
            if (i < edit->cursor) { cursor++; }
            display[count++] = c < 0x80 ? (char)c : '?';
        }
        display[count] = '\0';
        double available = viewport.width - 24.0;
        size_t columns = available < 16.0 ? 1 : (size_t)(available / 8.0);
        if (columns > TEXT_EDIT_CAPACITY - 1) { columns = TEXT_EDIT_CAPACITY - 1; }
        size_t first = cursor >= columns ? cursor - columns + 1 : 0;
        char visible[TEXT_EDIT_CAPACITY], caret[TEXT_EDIT_CAPACITY];
        size_t shown = count - first;
        if (shown > columns) { shown = columns; }
        memcpy(visible, display + first, shown);
        visible[shown] = '\0';
        memset(caret, ' ', cursor - first);
        caret[cursor - first] = '^';
        caret[cursor - first + 1] = '\0';
        Vec2 origin = {viewport.position.x + 12.0, viewport.position.y + 12.0};
        Colour colour = app_input_valid(input)
            ? (Colour){100, 240, 150, 255} : (Colour){255, 140, 100, 255};
        renderer2d_begin_viewport_clip(renderer);
        renderer2d_fill_screen_rect(renderer,
            (Rect2){origin, available, 68.0}, (Colour){25, 25, 25, 245});
        renderer2d_draw_screen_text(renderer, origin, "Wall length (mm / m)", colour);
        renderer2d_draw_screen_text(renderer, (Vec2){origin.x, origin.y + 16}, visible, colour);
        renderer2d_draw_screen_text(renderer, (Vec2){origin.x, origin.y + 26}, caret, colour);
        renderer2d_draw_screen_text(renderer, (Vec2){origin.x, origin.y + 44},
            app_input_feedback(input), colour);
        renderer2d_end_viewport_clip(renderer);
    }
}
