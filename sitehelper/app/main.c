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

    DomainId storey_id = sitehelper_project_add_storey(&context.project, 0);
    if (storey_id == DOMAIN_ID_INVALID) { return 1; }
    sitehelper_editor_set_current_storey(&context.editor, &context.project, storey_id);

    sitehelper_command_history_init(
        &context.history
    );

    command_run(
        app_menu_root(),
        &context
    );

    sitehelper_command_history_destroy(
        &context.history
    );

    sitehelper_project_destroy(
        &context.project
    );

    return 0;
}
