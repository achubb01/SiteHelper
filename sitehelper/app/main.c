#include "appcontext.h"
#include "appstate.h"
#include "appmenu.h"

int main(void)
{
    AppContext context = {0};

    command_run(app_menu_root(), &context);

    return 0;
}