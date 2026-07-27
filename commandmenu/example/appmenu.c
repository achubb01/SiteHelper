#include "appmenu.h"
#include "actions.h"

static Command display_children[] = {
    ACTION_COMMAND("Brightness", change_brightness)
};

static Command settings_children[] = {
    MENU_COMMAND("Display", display_children)
};

static Command main_children[] = {
    MENU_COMMAND("Settings", settings_children),
    ACTION_COMMAND("Doubler", doubleornuttin),
    ACTION_COMMAND("Tripler", tripleornuttin)
};

static Command root =
    MENU_COMMAND("Main Menu", main_children);

Command *app_menu_root(void)
{
    return &root;
}