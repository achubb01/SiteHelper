#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>

#include "actions.h"
#include "length_parse.h"
#include "sitehelper_model.h"
#include "wall.h"
#include "appstate.h"
#include "appcontext.h"
#include "sitehelper_command.h"

/* Gather defaults separately; input failure never partially changes the project. */
static int read_setting(const char *prompt, int *value)
{
    char buffer[256], *end;
    printf("%s", prompt);
    if (fgets(buffer, sizeof buffer, stdin) == NULL) { return 0; }
    errno = 0;
    long number = strtol(buffer, &end, 10);
    if (end == buffer || errno == ERANGE || number < INT_MIN || number > INT_MAX) { return 0; }
    end += strspn(end, " \t\r\n");
    if (*end != '\0') { return 0; }
    *value = (int)number;
    return 1;
}

/* Construction input shares the GUI parser. Reject an overlong line whole,
 * so a valid prefix cannot acquire a different meaning or feed the next prompt. */
static int read_measurement(const char *prompt, int *millimetres)
{
    char buffer[256];
    printf("%s", prompt);
    if (fgets(buffer, sizeof buffer, stdin) == NULL) { return 0; }
    if (strchr(buffer, '\n') == NULL) {
        int ch = getchar();
        if (ch != EOF) {
            while (ch != '\n' && ch != EOF) { ch = getchar(); }
            return 0;
        }
    }
    return length_parse_mm(buffer, millimetres) == LENGTH_PARSE_OK;
}

void setBuildSettings(void *context)
{
    AppContext *app = context;
    if (app == NULL) { return; }
    BuildSettings defaults = app->project.settings;
    int mode;
    if (!read_measurement("Select default Stud Height (mm or m): ", &defaults.stud_height) ||
        !read_measurement("Select Timber Width (mm or m): ", &defaults.stud_width) ||
        !read_measurement("Select Timber Depth (mm or m): ", &defaults.stud_depth) ||
        !read_measurement("Select Noggin Spacing (mm or m): ", &defaults.nog_spacing) ||
        !read_measurement("Select Stud Spacing (mm or m): ", &defaults.stud_spacing) ||
        !read_setting("1. Even\n2. Maximum\nSelect Spacing Mode: ", &mode) ||
        mode < 1 || mode > 2) {
        printf("Invalid settings input.\n");
        return;
    }
    defaults.stud_spacing_mode = (StudSpacingMode)(mode - 1);
    if (!sitehelper_project_set_build_settings(&app->project, &defaults)) {
        printf("Settings could not be applied to all affected Walls.\n");
        return;
    }
    sitehelper_editor_reconcile(&app->editor, &app->project);
}

void describeStandardStud (void *context){
    AppContext *app = context;

    if (app == NULL) {
        fprintf(
            stderr,
            "Application context is unavailable\n"
        );
        return;
    }

    printf("Standard Stud Dimensions are:\nHeight: %i mm\nWidth: %i mm\nDepth: %i mm\nNoggin spacing is set to: %i mm\nStud spacing is set to: %i mm\nStud spacing mode is set to %i\n", app->project.settings.stud_height, app->project.settings.stud_width, app->project.settings.stud_depth, app->project.settings.nog_spacing, app->project.settings.stud_spacing, app->project.settings.stud_spacing_mode);
}


//Build Menu
void addRoom(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }
    Storey *storey = sitehelper_project_find_storey_by_id(&app->project, app->editor.current_storey_id);
    if (storey == NULL) { printf("No active Storey.\n"); return; }


    DomainId room_id =
        sitehelper_project_add_room(
            &app->project, app->editor.current_storey_id
        );

    if (room_id == DOMAIN_ID_INVALID) {
        fprintf(
            stderr,
            "Could not add room\n"
        );
        return;
    }

    app->editor.current_room_id =
        room_id;

    printf(
        "Room added. Total rooms: %zu\n",
        storey->structure.room_count
    );
}

void addWall(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }
    Storey *storey = sitehelper_project_find_storey_by_id(&app->project, app->editor.current_storey_id);
    if (storey == NULL) { printf("No active Storey.\n"); return; }


    DomainId wall_id =
        sitehelper_project_add_wall(
            &app->project, app->editor.current_storey_id,
            /* Legacy CLI starts with an explicit horizontal 4200 mm segment. */
            (WallPlanSegment){ .end = { .x = 4200 } }
        );

    if (wall_id == DOMAIN_ID_INVALID) {
        fprintf(
            stderr,
            "Could not add wall\n"
        );
        return;
    }

    app->editor.current_wall_id =
        wall_id;

    printf(
        "Wall added. Total Storey walls: %zu\n",
        storey->structure.wall_count
    );
}

void generateWall(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    Wall *wall = app_current_wall(
        &app->project,
        &app->editor
    );

    if (wall == NULL) {
        printf("Please select a valid wall first.\n");
        return;
    }

    setWallLength(app);

    wall = app_current_wall(
        &app->project,
        &app->editor
    );

    if (wall == NULL) {
        fprintf(
            stderr,
            "Current wall became invalid\n"
        );
        return;
    }

    BuildSettings resolved;
    if (!sitehelper_project_resolve_storey_build_settings(&app->project, app->editor.current_storey_id, &resolved) ||
        !wall_generate(wall, &resolved)) {

        printf("Failed to generate wall\n");
        return;
    }

    printf(
        "Wall generated: %zu studs, %zu noggins\n",
        wall->framing.stud_count,
        wall->framing.nog_count
    );
}

void setWallLength(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    Wall *wall = app_current_wall(
        &app->project,
        &app->editor
    );

    if (wall == NULL) {
        printf("Select a wall first.\n");
        return;
    }

    int input;
    if (!read_measurement("Enter wall length (mm or m): ", &input)) {
        printf("Invalid wall length.\n");
        return;
    }

    PlanPosition start = wall->definition.segment.start;
    BuildSettings resolved;
    /* Legacy length editing deliberately establishes a horizontal segment. */
    if (input <= 0 ||
        start.x > INT_MAX - input ||
        !sitehelper_project_resolve_storey_build_settings(&app->project, app->editor.current_storey_id, &resolved) ||
        !wall_apply_plan_segment(wall, &resolved, (WallPlanSegment){
            .start = start,
            .end = { .x = start.x + input, .y = start.y }
        })) {
        printf("Invalid wall length.\n");
        return;
    }

    sitehelper_editor_reconcile(&app->editor, &app->project);
    printf("Wall length set to %d mm.\n", input);
}

void setStudSpacing(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    int input;
    if (!read_measurement("Enter stud spacing (mm or m): ", &input)) {
        printf("Invalid stud spacing.\n");
        return;
    }

    BuildSettings defaults = app->project.settings;
    if (!build_set_stud_spacing(&defaults, input) ||
        !sitehelper_project_set_build_settings(&app->project, &defaults)) {

        printf("Invalid stud spacing.\n");
        return;
    }

    sitehelper_editor_reconcile(&app->editor, &app->project);

    printf(
        "Stud spacing set to %d mm.\n",
        input
    );
}

void selectRoom(void *context)
{
    AppContext *app = context;
    char buffer[256];

    if (app == NULL) {
        return;
    }
    Storey *storey = sitehelper_project_find_storey_by_id(&app->project, app->editor.current_storey_id);
    if (storey == NULL) { printf("No active Storey.\n"); return; }


    if (storey->structure.room_count == 0) {
        printf("No rooms currently built.\n");
        return;
    }

    for (size_t i = 0;
         i < storey->structure.room_count;
         i++) {

        printf(
            "%zu. Room %zu\n",
            i + 1,
            i + 1
        );
    }

    printf("Select room: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read room selection\n");
        return;
    }

    char *end;
    long selection = strtol(buffer, &end, 10);

    if (end == buffer ||
        selection < 1 ||
        selection > (long)storey->structure.room_count) {

        printf("Invalid room selection.\n");
        return;
    }

    size_t room_index =
        (size_t)(selection - 1);

    Room *room =
        &storey->structure.rooms[
            room_index
        ];

    app->editor.current_room_id =
        room->id;

    printf("Room %ld selected.\n", selection);
}

void selectWall(void *context)
{
    AppContext *app = context;
    char buffer[256];

    if (app == NULL) {
        return;
    }
    Storey *storey = sitehelper_project_find_storey_by_id(&app->project, app->editor.current_storey_id);
    if (storey == NULL) { printf("No active Storey.\n"); return; }


    if (storey->structure.wall_count == 0) {
        printf("The project has no walls.\n");
        return;
    }

    for (size_t i = 0;
         i < storey->structure.wall_count;
         i++) {

        printf(
            "%zu. Wall %zu\n",
            i + 1,
            i + 1
        );
    }

    printf("Select wall: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read wall selection\n");
        return;
    }

    char *end;
    long selection = strtol(buffer, &end, 10);

    if (end == buffer ||
        selection < 1 ||
        selection > (long)storey->structure.wall_count) {

        printf("Invalid wall selection.\n");
        return;
    }

    size_t wall_index =
        (size_t)(selection - 1);

    Wall *wall = &storey->structure.walls[wall_index];

    app->editor.current_wall_id =
        wall->id;

    printf(
        "Wall %ld selected.\n",
        selection
    );
}

void describeBuild(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }
    Storey *storey = sitehelper_project_find_storey_by_id(&app->project, app->editor.current_storey_id);
    if (storey == NULL) { printf("No active Storey.\n"); return; }


    printf("\n=== BUILD DESCRIPTION ===\n");

    printf(
        "Rooms: %zu\n",
        storey->structure.room_count
    );

    printf(
        "Default stud dimensions: %d x %d x %d mm\n",
        app->project.settings.stud_height,
        app->project.settings.stud_width,
        app->project.settings.stud_depth
    );

    printf(
        "Maximum stud spacing: %d mm\n",
        app->project.settings.stud_spacing
    );

    printf(
        "Maximum noggin spacing: %d mm\n",
        app->project.settings.nog_spacing
    );

    printf("Storey walls: %zu\n", storey->structure.wall_count);
    for (size_t wall_index = 0;
         wall_index < storey->structure.wall_count;
         wall_index++) {
        Wall *wall = &storey->structure.walls[wall_index];

        printf(
            "\n  Wall %zu\n",
            wall_index + 1
        );

        printf(
            "    Length: %d mm\n",
            wall_length_mm(wall)
        );

        printf(
            "    Studs: %zu\n",
            wall->framing.stud_count
        );

        for (size_t stud_index = 0;
            stud_index < wall->framing.stud_count;
            stud_index++) {

            Timber *stud =
                &wall->framing.studs[stud_index];

            printf(
                "      Stud %zu: "
                "position = %d mm, "
                "length = %d mm\n",
                stud_index + 1,
                stud->position.u,
                stud->length
            );
        }

        printf(
            "    Noggins: %zu\n",
            wall->framing.nog_count
        );

        if (wall->framing.nog_count > 0) {

            int current_height =
                wall->framing.nogs[0].position.z;

            printf(
                "      Noggin row at %d mm\n",
                current_height
            );

            for (size_t nog_index = 0;
                nog_index < wall->framing.nog_count;
                nog_index++) {

                Timber *noggin =
                    &wall->framing.nogs[nog_index];

                int height =
                    noggin->position.z;

                if (height != current_height) {

                    current_height = height;

                    printf(
                        "\n      Noggin row at %d mm\n",
                        current_height
                    );
                }

                printf(
                    "        Bay %zu: %d mm\n",
                    noggin->details.noggin.bay + 1,
                    noggin->length
                );
            }
        }
    }

    printf("\n=========================\n");
}

void printCurrentWall(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    Wall *wall =
        app_current_wall(
            &app->project,
            &app->editor
        );

    if (wall == NULL) {
        printf(
            "No valid wall is selected.\n"
        );
        return;
    }

    printf(
        "Current wall selected successfully.\n"
    );
}

void addOpening(void *context)
{
    AppContext *app = context;
    char buffer[256];

    if (app == NULL) {
        return;
    }

    Wall *wall = app_current_wall(
        &app->project,
        &app->editor
    );

    if (wall == NULL) {
        printf("Select a wall first.\n");
        return;
    }

    printf(
        "Opening type:\n"
        "1. Door\n"
        "2. Window\n"
        "Select type: "
    );

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read opening type.\n");
        return;
    }

    char *end;
    long type_input = strtol(buffer, &end, 10);

    if (end == buffer ||
        type_input < 1 ||
        type_input > 2) {

        printf("Invalid opening type.\n");
        return;
    }

    OpeningType type =
        type_input == 1
            ? OPENING_DOOR
            : OPENING_WINDOW;

    int position, width, height;
    if (!read_measurement("Enter framed opening position U (mm or m): ", &position) || position < 0) {
        printf("Invalid opening position.\n");
        return;
    }
    if (!read_measurement("Enter nominal opening width (mm or m): ", &width) || width <= 0) {
        printf("Invalid opening width.\n");
        return;
    }
    if (!read_measurement("Enter nominal opening height (mm or m): ", &height) || height <= 0) {
        printf("Invalid opening height.\n");
        return;
    }
    int frame_bottom = 0;
    if (type == OPENING_WINDOW &&
        (!read_measurement("Enter framed opening bottom Z (mm or m): ", &frame_bottom) || frame_bottom < 0)) {
        printf("Invalid opening bottom height.\n");
        return;
    }

    OpeningCommand opening_command;

    if (!opening_command_create(
            app->editor.current_wall_id,
            type,
            position,
            frame_bottom,
            width,
            height,
            &opening_command)) {

        printf(
            "Could not create opening command.\n"
        );

        return;
    }

    SiteHelperCommand command;

    if (!sitehelper_command_from_opening(
            &opening_command,
            &command)) {

        printf(
            "Could not create application command.\n"
        );

        return;
    }

    SiteHelperCommandResult result;

    if (!sitehelper_command_history_execute(
            &app->history,
            &app->project,
            &command,
            &result)) {

        printf(
            "Could not add opening. "
            "Check that it fits within the wall.\n"
        );

        return;
    }

    printf(
        "%s opening added.\n",
        type == OPENING_DOOR
            ? "Door"
            : "Window"
    );
}
