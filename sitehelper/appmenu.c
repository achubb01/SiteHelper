#include "appmenu.h"
#include "actions.h"

static Command build_setting_children[] = {
    ACTION_COMMAND("Set Build Settings", setBuildSettings),
    ACTION_COMMAND("Describe Standard Stud", describeStandardStud)
};

static Command build_structure_childeren[] = {
    ACTION_COMMAND("Build Room", addRoom),
    ACTION_COMMAND("Build Wall", addWall),
    ACTION_COMMAND("Add Opening", addOpening),
    ACTION_COMMAND("Generate Wall", generateWall),
    ACTION_COMMAND("Select Room", selectRoom),
    ACTION_COMMAND("Select Wall", selectWall),
    ACTION_COMMAND("Describe Build", describeBuild),
    ACTION_COMMAND("PRINT SELECTED WALL", printCurrentWall)
};

static Command main_children[] = {
    MENU_COMMAND("Define Build Settings", build_setting_children),
    MENU_COMMAND("Build Structure", build_structure_childeren)
};

static Command root =
    MENU_COMMAND("Main Menu", main_children);

Command *app_menu_root(void)
{
    return &root;
}