#include "appcontext.h"
#include "appmenu.h"
#include "commandmenu.h"

int main(void)
{
    AppContext context = {
        .value = 1,
        .brightness = 5
    };

    command_run(
        app_menu_root(),
        &context
    );

    return 0;
}