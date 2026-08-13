#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"

static void test_editor_init_has_no_current_room(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        editor.current_room_id ==
        DOMAIN_ID_INVALID
    );
}

static void test_editor_init_has_no_current_wall(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        editor.current_wall_id ==
        DOMAIN_ID_INVALID
    );
}

static void test_editor_init_defaults_to_select_tool(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        sitehelper_editor_get_active_tool(
            &editor
        )
        == EDITOR_TOOL_SELECT
    );
}

static void test_editor_can_change_active_tool(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    int result =
        sitehelper_editor_set_active_tool(
            &editor,
            EDITOR_TOOL_OPENING
        );

    assert(result == 1);

    assert(
        sitehelper_editor_get_active_tool(
            &editor
        )
        == EDITOR_TOOL_OPENING
    );
}

static void test_editor_rejects_invalid_tool(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    int result =
        sitehelper_editor_set_active_tool(
            &editor,
            EDITOR_TOOL_COUNT
        );

    assert(result == 0);

    assert(
        sitehelper_editor_get_active_tool(
            &editor
        )
        == EDITOR_TOOL_SELECT
    );
}

int main(void)
{
    test_editor_init_has_no_current_room();
    test_editor_init_has_no_current_wall();
    test_editor_init_defaults_to_select_tool();
    test_editor_can_change_active_tool();
    test_editor_rejects_invalid_tool();

    printf(
        "All SiteHelper editor tests passed.\n"
    );

    return 0;
}