#include <assert.h>
#include <stdio.h>

#include "opening_tool.h"

static void test_init_sets_default_state(void)
{
    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    assert(tool.active == 0);
    assert(tool.preview_valid == 0);

    assert(tool.type == OPENING_WINDOW);

    assert(tool.width == 1200);
    assert(tool.height == 1200);
    assert(tool.bottom == 900);
}

static void test_activate_sets_active(void)
{
    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    opening_tool_activate(
        &tool
    );

    assert(tool.active == 1);
}

static void test_update_preview_stores_position_when_active(void)
{
    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    opening_tool_activate(
        &tool
    );

    opening_tool_update_preview(
        &tool,
        (Vec2){
            .x = 1500.0,
            .y = 900.0
        }
    );

    assert(tool.preview_valid == 1);

    assert(
        tool.preview_position.x
        == 1500.0
    );

    assert(
        tool.preview_position.y
        == 900.0
    );
}

static void test_update_preview_is_ignored_when_inactive(void)
{
    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    opening_tool_update_preview(
        &tool,
        (Vec2){
            .x = 1500.0,
            .y = 900.0
        }
    );

    assert(tool.preview_valid == 0);

    assert(
        tool.preview_position.x
        == 0.0
    );

    assert(
        tool.preview_position.y
        == 0.0
    );
}

static void test_cancel_clears_active_and_preview(void)
{
    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    opening_tool_activate(
        &tool
    );

    opening_tool_update_preview(
        &tool,
        (Vec2){
            .x = 1500.0,
            .y = 900.0
        }
    );

    opening_tool_cancel(
        &tool
    );

    assert(tool.active == 0);
    assert(tool.preview_valid == 0);
}

static void test_active_tool_tracks_latest_preview_position(void)
{
    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    opening_tool_activate(
        &tool
    );

    opening_tool_update_preview(
        &tool,
        (Vec2){
            .x = 600.0,
            .y = 1200.0
        }
    );

    assert(tool.preview_valid == 1);
    assert(tool.preview_position.x == 600.0);
    assert(tool.preview_position.y == 1200.0);

    opening_tool_update_preview(
        &tool,
        (Vec2){
            .x = 1200.0,
            .y = 900.0
        }
    );

    assert(tool.preview_valid == 1);
    assert(tool.preview_position.x == 1200.0);
    assert(tool.preview_position.y == 900.0);
}

static void test_cancel_prevents_further_preview_updates(void)
{
    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    opening_tool_activate(
        &tool
    );

    opening_tool_update_preview(
        &tool,
        (Vec2){600.0, 900.0}
    );

    opening_tool_cancel(
        &tool
    );

    opening_tool_update_preview(
        &tool,
        (Vec2){1800.0, 1200.0}
    );

    assert(tool.active == 0);
    assert(tool.preview_valid == 0);

    /*
     * update_preview() was ignored after cancellation.
     */
    assert(tool.preview_position.x == 600.0);
    assert(tool.preview_position.y == 900.0);
}

static void test_preview_rect_uses_tool_dimensions(void)
{
    OpeningTool tool;

    opening_tool_init(&tool);
    opening_tool_activate(&tool);

    opening_tool_update_preview(
        &tool,
        (Vec2){1500.0, 1000.0}
    );

    Rect2 rect;

    assert(
        opening_tool_preview_rect(
            &tool,
            &rect
        )
    );

    assert(rect.position.x == 1500.0);
    assert(rect.position.y == 900.0);
    assert(rect.width == 1200.0);
    assert(rect.height == 1200.0);
}

static void test_preview_rect_rejects_invalid_preview(void)
{
    OpeningTool tool;

    opening_tool_init(&tool);

    Rect2 rect;

    assert(
        !opening_tool_preview_rect(
            &tool,
            &rect
        )
    );
}

int main(void)
{
    test_init_sets_default_state();
    test_activate_sets_active();
    test_update_preview_stores_position_when_active();
    test_update_preview_is_ignored_when_inactive();
    test_cancel_clears_active_and_preview();
    test_active_tool_tracks_latest_preview_position();
    test_cancel_prevents_further_preview_updates();
    test_preview_rect_uses_tool_dimensions();
    test_preview_rect_rejects_invalid_preview();

    printf(
        "All opening tool tests passed.\n"
    );

    return 0;
}