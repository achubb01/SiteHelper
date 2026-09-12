#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "actions.h"
#include "appcontext.h"
#include "appstate.h"
#include "wall.h"

/* The fixture is private to this executable in its CTest working directory. */
static const char *input_path = "cli_measurements_input.tmp";
static void input(const char *text)
{
    FILE *file = fopen(input_path, "w");
    assert(file != NULL);
    assert(fputs(text, file) >= 0);
    assert(fclose(file) == 0);
    assert(freopen(input_path, "r", stdin) != NULL);
}

int main(void)
{
    AppContext app = {0};
    sitehelper_project_init(&app.project);
    sitehelper_editor_init(&app.editor);
    sitehelper_command_history_init(&app.history);
    DomainId storey = sitehelper_project_add_storey(&app.project, 0);
    assert(sitehelper_editor_set_current_storey(&app.editor, &app.project, storey));

    input("2.7m\n45mm\n90\n0.6m\n600mm\n1\n");
    setBuildSettings(&app);
    assert(app.project.settings.stud_height == 2700);
    assert(app.project.settings.stud_width == 45 && app.project.settings.stud_depth == 90);
    assert(app.project.settings.nog_spacing == 600 && app.project.settings.stud_spacing == 600);
    assert(app.project.settings.stud_spacing_mode == STUD_SPACING_EVEN);
    input("2.8m\n45.5mm\n");
    setBuildSettings(&app);
    assert(app.project.settings.stud_height == 2700 && app.project.settings.stud_width == 45);

    app.editor.current_wall_id = sitehelper_project_add_wall(&app.project, storey,
        (WallPlanSegment){{100,200},{6100,200}});
    assert(app.editor.current_wall_id != DOMAIN_ID_INVALID);
    input("4.2m\n"); setWallLength(&app);
    assert(wall_length_mm(app_current_wall(&app.project, &app.editor)) == 4200);
    assert(app_current_wall(&app.project, &app.editor)->definition.segment.end.x == 4300);
    const char *bad[] = {"4.2mx\n", "4200junk\n", "4200.1mm\n", "0.0001m\n",
        "2147483648\n", "-1m\n", "0\n", "nan\n"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        input(bad[i]); setWallLength(&app);
        assert(wall_length_mm(app_current_wall(&app.project, &app.editor)) == 4200);
    }
    input("6000mm\n"); setWallLength(&app);
    assert(wall_length_mm(app_current_wall(&app.project, &app.editor)) == 6000);
    input("0.45m\n"); setStudSpacing(&app);
    assert(app.project.settings.stud_spacing == 450);
    input("600.1mm\n"); setStudSpacing(&app);
    assert(app.project.settings.stud_spacing == 450);
    /* Reject the full overlong input and consume it through newline. */
    char overlong[600];
    memset(overlong, ' ', sizeof overlong);
    memcpy(overlong, "300", 3);
    memcpy(overlong + 500, "junk\n600mm\n", 12);
    overlong[512] = 0;
    input(overlong); setStudSpacing(&app);
    assert(app.project.settings.stud_spacing == 450);
    setStudSpacing(&app);
    assert(app.project.settings.stud_spacing == 600);

    DomainId next_id = app.project.domain_ids.next;
    input("2\n1m\n0.6001m\n"); addOpening(&app);
    assert(app.history.count == 0 && app.project.domain_ids.next == next_id);
    input("2\n1m\n600mm\n0.6m\n0.9m\n"); addOpening(&app);
    Wall *wall = app_current_wall(&app.project, &app.editor);
    assert(app.history.count == 1 && app.history.entries[0].command.type == SITEHELPER_COMMAND_ADD_OPENING);
    assert(wall->definition.opening_count == 1);
    Opening opening = wall->definition.openings[0];
    assert(opening.frame_position == 1000 && opening.width == 600 && opening.height == 600 && opening.frame_bottom == 900);
    assert(sitehelper_command_history_undo(&app.history, &app.project));
    assert(app_current_wall(&app.project, &app.editor)->definition.opening_count == 0);
    assert(sitehelper_command_history_redo(&app.history, &app.project));
    assert(app_current_wall(&app.project, &app.editor)->definition.openings[0].frame_bottom == 900);

    assert(fclose(stdin) == 0);
    assert(remove(input_path) == 0);
    sitehelper_command_history_destroy(&app.history);
    sitehelper_project_destroy(&app.project);
    puts("CLI measurement tests passed");
}
