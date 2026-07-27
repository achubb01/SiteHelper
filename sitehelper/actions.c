#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "actions.h"
#include "appcontext.h"
#include "sitehelper.h"
#include "appstate.h"

//Setting Menu
void setBuildSettings(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        fprintf(
            stderr,
            "Application context is unavailable\n"
        );
        return;
    }

    char buffer[256];

    printf("Select Stud Height: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read stud height\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->settings.stud_height = atoi(buffer);

    printf("Select Timber Width: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read timber width\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->settings.stud_width = atoi(buffer);

    printf("Select Timber Depth: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read timber depth\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->settings.stud_depth = atoi(buffer);

    printf("Select Noggin Spacing: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read noggin spacing\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->settings.nog_spacing = atoi(buffer);

    printf("Select Stud Spacing: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read stud spacing\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->settings.stud_spacing = atoi(buffer);

    printf("1. Even\n2. Maximum\nSelect Spacing Mode: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read stud spacing mode\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->settings.stud_spacing_mode = atoi(buffer) -1;
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

    printf("Standard Stud Dimensions are:\nHeight: %i\nWidth: %i\nDepth: %i\nNoggin spacing is set to: %i\nStud spacing is set to: %i\nStud spacing mode is set to %i\n", app->settings.stud_height, app->settings.stud_width, app->settings.stud_depth, app->settings.nog_spacing, app->settings.stud_spacing, app->settings.stud_spacing_mode);
}


//Build Menu
void addRoom(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    if (!build_add_room(&app->structure)) {
        fprintf(stderr, "Could not add room\n");
        return;
    }

    app->current_room =
    app->structure.room_count - 1;

    app->room_selected = true;
    app->wall_selected = false;

    printf(
        "Room added. Total rooms: %zu\n",
        app->structure.room_count
    );
}

void addWall(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    if (app->structure.room_count == 0) {
        printf("Create a room before adding a wall\n");
        return;
    }

    Room *room =
    &app->structure.rooms[
        app->current_room
    ];

    if (!room_add_wall(room)) {
        fprintf(stderr, "Could not add wall\n");
        return;
    }

    app->current_wall =
    room->wall_count - 1;

    app->wall_selected = true;

    printf(
        "Wall added. Total walls in current room: %zu\n",
        room->wall_count
    );
}

void generateWall(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    Wall *wall = app_current_wall(app);

    if (wall == NULL) {
        printf("Please select a valid wall first.\n");
        return;
    }

    setWallLength(app);

    wall = app_current_wall(app);

    if (wall == NULL) {
        fprintf(
            stderr,
            "Current wall became invalid\n"
        );
        return;
    }

    if (!wall_generate(
            wall,
            &app->settings)) {

        printf("Failed to generate wall\n");
        return;
    }

    printf(
        "Wall generated: %zu studs, %zu noggins\n",
        wall->stud_count,
        wall->nog_count
    );
}

void setWallLength(void *context)
{
    AppContext *app = context;
    char buffer[256];

    if (app == NULL) {
        return;
    }

    Wall *wall = app_current_wall(app);

    if (wall == NULL) {
        printf("Select a wall first.\n");
        return;
    }

    printf("Enter wall length: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read wall length.\n");
        return;
    }

    char *end;
    long input = strtol(buffer, &end, 10);

    if (end == buffer) {
        printf("Please enter a number.\n");
        return;
    }

    if (!wall_set_length(wall, (int)input)) {
        printf("Invalid wall length.\n");
        return;
    }

    printf("Wall length set to %ld mm.\n", input);
}

void setStudSpacing(void *context)
{
    AppContext *app = context;
    char buffer[256];

    if (app == NULL) {
        return;
    }

    printf("Enter stud spacing: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(
            stderr,
            "Failed to read stud spacing.\n"
        );
        return;
    }

    char *end;
    long input = strtol(buffer, &end, 10);

    if (end == buffer) {
        printf("Please enter a number.\n");
        return;
    }

    if (!build_set_stud_spacing(
            &app->settings,
            (int)input)) {

        printf("Invalid stud spacing.\n");
        return;
    }

    printf(
        "Stud spacing set to %ld mm.\n",
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

    if (app->structure.room_count == 0) {
        printf("No rooms currently built.\n");
        return;
    }

    for (size_t i = 0;
         i < app->structure.room_count;
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
        selection > (long)app->structure.room_count) {

        printf("Invalid room selection.\n");
        return;
    }

    app->current_room = (size_t)(selection - 1);
    app->room_selected = true;

    /*
     * A wall selected in another room may not exist
     * in the newly selected room.
     */
    app->wall_selected = false;

    printf("Room %ld selected.\n", selection);
}

void selectWall(void *context)
{
    AppContext *app = context;
    char buffer[256];

    if (app == NULL) {
        return;
    }

    if (!app->room_selected) {
        printf("Select a room first.\n");
        return;
    }

    if (app->current_room >=
        app->structure.room_count) {

        fprintf(stderr, "Selected room is invalid.\n");
        app->room_selected = false;
        app->wall_selected = false;
        return;
    }

    Room *room =
        &app->structure.rooms[
            app->current_room
        ];

    if (room->wall_count == 0) {
        printf("The selected room has no walls.\n");
        return;
    }

    for (size_t i = 0;
         i < room->wall_count;
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
        selection > (long)room->wall_count) {

        printf("Invalid wall selection.\n");
        return;
    }

    app->current_wall = (size_t)(selection - 1);
    app->wall_selected = true;

    printf("Wall %zu selected in room %zu\n", app->current_wall, app->current_room);
}

void describeBuild(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    printf("\n=== BUILD DESCRIPTION ===\n");

    printf(
        "Rooms: %zu\n",
        app->structure.room_count
    );

    printf(
        "Default stud dimensions: %d x %d x %d mm\n",
        app->settings.stud_height,
        app->settings.stud_width,
        app->settings.stud_depth
    );

    printf(
        "Maximum stud spacing: %d mm\n",
        app->settings.stud_spacing
    );

    printf(
        "Maximum noggin spacing: %d mm\n",
        app->settings.nog_spacing
    );

    for (size_t room_index = 0;
         room_index < app->structure.room_count;
         room_index++) {

        Room *room =
            &app->structure.rooms[room_index];

        printf(
            "\nRoom %zu\n",
            room_index + 1
        );

        printf(
            "  Walls: %zu\n",
            room->wall_count
        );

        for (size_t wall_index = 0;
            wall_index < room->wall_count;
            wall_index++) {

            Wall *wall =
                &room->walls[wall_index];

            printf(
                "\n  Wall %zu\n",
                wall_index + 1
            );

            printf(
                "    Length: %d mm\n",
                wall->bottomplate.length
            );

            printf(
                "    Studs: %zu\n",
                wall->stud_count
            );

            for (size_t stud_index = 0;
                stud_index < wall->stud_count;
                stud_index++) {

                Timber *stud =
                    &wall->studs[stud_index];

                printf(
                    "      Stud %zu: "
                    "position = %d mm, "
                    "length = %d mm\n",
                    stud_index + 1,
                    stud->position.x,
                    stud->length
                );
            }

            printf(
                "    Noggins: %zu\n",
                wall->nog_count
            );

            if (wall->nog_count > 0) {

                int current_height =
                    wall->nogs[0].position.y;

                printf(
                    "      Noggin row at %d mm\n",
                    current_height
                );

                for (size_t nog_index = 0;
                    nog_index < wall->nog_count;
                    nog_index++) {

                    Timber *noggin =
                        &wall->nogs[nog_index];

                    int height =
                        noggin->position.y;

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
    }

    printf("\n=========================\n");
}

void printCurrentWall(void *context) {
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    printf("Room is %zu\nWall selected is Wall %zu", app->current_room, app->current_wall);
}

void addOpening(void *context)
{
    AppContext *app = context;
    char buffer[256];

    if (app == NULL) {
        return;
    }

    Wall *wall = app_current_wall(app);

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

    printf("Enter framed opening position: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read opening position.\n");
        return;
    }

    long position = strtol(buffer, &end, 10);

    if (end == buffer || position < 0) {
        printf("Invalid opening position.\n");
        return;
    }

    printf("Enter nominal opening width: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read opening width.\n");
        return;
    }

    long width = strtol(buffer, &end, 10);

    if (end == buffer || width <= 0) {
        printf("Invalid opening width.\n");
        return;
    }

    printf("Enter nominal opening height: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read opening height.\n");
        return;
    }

    long height = strtol(buffer, &end, 10);

    if (end == buffer || height <= 0) {
        printf("Invalid opening height.\n");
        return;
    }

    if (!wall_add_opening(
            wall,
            &app->settings,
            type,
            0,
            (int)position,
            (int)width,
            (int)height)) {

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