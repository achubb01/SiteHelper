#include "./appcontext.h"
#include "./appmenu.h"
#include "../commandmenu/commandmenu.h"

int main(void) {
    AppContext context = {0};

    command_run(app_menu_root(), &context);

    return 0;
}