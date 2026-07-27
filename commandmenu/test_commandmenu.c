#include <assert.h>
#include <stdio.h>

#include "commandmenu.h"
#include "commandmenu_internal.h"

static void dummy_action(void *context)
{
    (void)context;
}

static Command submenu_children[] = {
    ACTION_COMMAND("Brightness", dummy_action)
};

static Command root_children[] = {
    MENU_COMMAND("Settings", submenu_children),
    ACTION_COMMAND("Doubler", dummy_action),
    ACTION_COMMAND("Tripler", dummy_action)
};

static Command root =
    MENU_COMMAND("Main Menu", root_children);

static void prepare_test_menu(void)
{
    root.previous = NULL;

    root_children[0].previous = &root;

    submenu_children[0].previous =
        &root_children[0];
}

static void test_prefix_matching(void)
{
    assert(command_prefix_match("set", "Settings"));
    assert(command_prefix_match("SET", "Settings"));
    assert(command_prefix_match("Settings", "Settings"));

    assert(!command_prefix_match(
        "SettingsExtra",
        "Settings"
    ));

    assert(!command_prefix_match("search", "Settings"));
    assert(!command_prefix_match("x", "Settings"));
}

static void test_numeric_selection(void)
{
    CommandResult result;

    result = command_select(&root, "1");

    assert(result.type == COMMAND_RESULT_MATCH);
    assert(result.command == &root_children[0]);

    result = command_select(&root, "2");

    assert(result.type == COMMAND_RESULT_MATCH);
    assert(result.command == &root_children[1]);

    result = command_select(&root, "4");

    assert(result.type == COMMAND_RESULT_QUIT);
    assert(result.command == NULL);

    result = command_select(&root, "0");
    assert(result.type == COMMAND_RESULT_UNKNOWN);

    result = command_select(&root, "999");
    assert(result.type == COMMAND_RESULT_UNKNOWN);
}

static void test_text_selection(void)
{
    CommandResult result;

    result = command_select(&root, "Settings");

    assert(result.type == COMMAND_RESULT_MATCH);
    assert(result.command == &root_children[0]);

    result = command_select(&root, "set");

    assert(result.type == COMMAND_RESULT_MATCH);
    assert(result.command == &root_children[0]);

    result = command_select(&root, "SET");

    assert(result.type == COMMAND_RESULT_MATCH);
    assert(result.command == &root_children[0]);

    result = command_select(&root, "dou");

    assert(result.type == COMMAND_RESULT_MATCH);
    assert(result.command == &root_children[1]);
}

static void test_navigation_selection(void)
{
    Command *settings = &root_children[0];
    CommandResult result;

    result = command_select(&root, "quit");

    assert(result.type == COMMAND_RESULT_QUIT);
    assert(result.command == NULL);

    result = command_select(&root, "QU");

    assert(result.type == COMMAND_RESULT_QUIT);

    result = command_select(settings, "back");

    assert(result.type == COMMAND_RESULT_BACK);
    assert(result.command == NULL);

    result = command_select(settings, "BA");

    assert(result.type == COMMAND_RESULT_BACK);
}

static void test_invalid_navigation(void)
{
    Command *settings = &root_children[0];
    CommandResult result;

    result = command_select(&root, "back");
    assert(result.type == COMMAND_RESULT_UNKNOWN);

    result = command_select(settings, "quit");
    assert(result.type == COMMAND_RESULT_UNKNOWN);
}

static void test_back_ambiguity(void)
{
    Command *settings = &root_children[0];
    CommandResult result;

    result = command_select(settings, "b");

    assert(result.type == COMMAND_RESULT_AMBIGUOUS);
    assert(result.command == NULL);

    result = command_select(settings, "ba");

    assert(result.type == COMMAND_RESULT_BACK);

    result = command_select(settings, "br");

    assert(result.type == COMMAND_RESULT_MATCH);
    assert(result.command == &submenu_children[0]);
}

static void test_invalid_input(void)
{
    CommandResult result;

    result = command_select(&root, "");
    assert(result.type == COMMAND_RESULT_UNKNOWN);

    result = command_select(&root, "unknown");
    assert(result.type == COMMAND_RESULT_UNKNOWN);

    result = command_select(&root, "2abc");
    assert(result.type == COMMAND_RESULT_UNKNOWN);

    result = command_select(&root, "-1");
    assert(result.type == COMMAND_RESULT_UNKNOWN);
}

int main(void)
{
    prepare_test_menu();

    test_prefix_matching();
    test_numeric_selection();
    test_text_selection();
    test_navigation_selection();
    test_invalid_navigation();
    test_back_ambiguity();
    test_invalid_input();

    printf("All command menu tests passed.\n");

    return 0;
}