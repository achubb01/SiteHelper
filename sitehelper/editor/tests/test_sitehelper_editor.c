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

int main(void)
{
    test_editor_init_has_no_current_room();
    test_editor_init_has_no_current_wall();

    printf(
        "All SiteHelper editor tests passed.\n"
    );

    return 0;
}