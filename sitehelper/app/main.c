#include "appcontext.h"
#include "appstate.h"
#include "appmenu.h"

int main(void)
{
    AppContext context = {0};

    domain_id_generator_init(
        &context.domain_ids
    );

    command_run(
        app_menu_root(),
        &context
    );

    return 0;
}