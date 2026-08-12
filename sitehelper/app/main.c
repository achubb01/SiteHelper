#include "appcontext.h"
#include "appstate.h"
#include "appmenu.h"
#include "sitehelper_project.h"

int main(void)
{
    AppContext context = {0};

    sitehelper_project_init(
        &context.project
    );

    sitehelper_editor_init(
        &context.editor
    );

    command_run(
        app_menu_root(),
        &context
    );

    sitehelper_project_destroy(
        &context.project
    );

    return 0;
}