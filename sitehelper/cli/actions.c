#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "actions.h"
#include "sitehelper_model.h"
#include "wall.h"
#include "appstate.h"
#include "appcontext.h"

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

    app->project.settings.stud_height = atoi(buffer);

    printf("Select Timber Width: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read timber width\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->project.settings.stud_width = atoi(buffer);

    printf("Select Timber Depth: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read timber depth\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->project.settings.stud_depth = atoi(buffer);

    printf("Select Noggin Spacing: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read noggin spacing\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->project.settings.nog_spacing = atoi(buffer);

    printf("Select Stud Spacing: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read stud spacing\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->project.settings.stud_spacing = atoi(buffer);

    printf("1. Even\n2. Maximum\nSelect Spacing Mode: ");

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fprintf(stderr, "Failed to read stud spacing mode\n");
        return;
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    app->project.settings.stud_spacing_mode = atoi(buffer) -1;
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

    printf("Standard Stud Dimensions are:\nHeight: %i\nWidth: %i\nDepth: %i\nNoggin spacing is set to: %i\nStud spacing is set to: %i\nStud spacing mode is set to %i\n", app->project.settings.stud_height, app->project.settings.stud_width, app->project.settings.stud_depth, app->project.settings.nog_spacing, app->project.settings.stud_spacing, app->project.settings.stud_spacing_mode);
}


//Build Menu
void addRoom(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    DomainId room_id =
        domain_id_generate(
            &app->project.domain_ids
        );

    if (room_id == DOMAIN_ID_INVALID) {
        fprintf(
            stderr,
            "Could not allocate room identity\n"
        );
        return;
    }

    if (!build_add_room(
            &app->project.structure,
            room_id)) {

        fprintf(
            stderr,
            "Could not add room\n"
        );
        return;
    }

    app->current_room_id =
        room_id;

    app->current_wall_id =
        DOMAIN_ID_INVALID;

    app->room_selected = true;
    app->wall_selected = false;

    printf(
        "Room added. Total rooms: %zu\n",
        app->project.structure.room_count
    );
}

void addWall(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    if (app->project.structure.room_count == 0) {
        printf("Create a room before adding a wall\n");
        return;
    }

    Room *room =
        app_current_room(
            app
        );

    if (room == NULL) {
        printf(
            "Select a valid room before adding a wall\n"
        );
        return;
    }

    DomainId wall_id =
        domain_id_generate(
            &app->project.domain_ids
        );

    if (wall_id == DOMAIN_ID_INVALID) {
        fprintf(
            stderr,
            "Could not allocate wall identity\n"
        );
        return;
    }

    if (!room_add_wall(
            room,
            wall_id)) {

        fprintf(
            stderr,
            "Could not add wall\n"
        );
        return;
    }

    app->current_wall_id =
        wall_id;

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
            &app->project.settings)) {

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
            &app->project.settings,
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

    if (app->project.structure.room_count == 0) {
        printf("No rooms currently built.\n");
        return;
    }

    for (size_t i = 0;
         i < app->project.structure.room_count;
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
        selection > (long)app->project.structure.room_count) {

        printf("Invalid room selection.\n");
        return;
    }

    size_t room_index =
        (size_t)(selection - 1);

    Room *room =
        &app->project.structure.rooms[
            room_index
        ];

    app->current_room_id =
        room->id;

    app->current_wall_id =
        DOMAIN_ID_INVALID;

    app->room_selected = true;
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

    Room *room =
        app_current_room(
            app
        );

    if (room == NULL) {
        fprintf(
            stderr,
            "Selected room is invalid.\n"
        );

        app->current_room_id =
            DOMAIN_ID_INVALID;

        app->current_wall_id =
            DOMAIN_ID_INVALID;

        app->room_selected = false;
        app->wall_selected = false;

        return;
    }

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

    size_t wall_index =
        (size_t)(selection - 1);

    Wall *wall =
        &room->walls[
            wall_index
        ];

    app->current_wall_id =
        wall->id;

    app->wall_selected = true;

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

    printf("\n=== BUILD DESCRIPTION ===\n");

    printf(
        "Rooms: %zu\n",
        app->project.structure.room_count
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

    for (size_t room_index = 0;
         room_index < app->project.structure.room_count;
         room_index++) {

        Room *room =
            &app->project.structure.rooms[room_index];

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
                wall->definition.length
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
                    stud->position.x,
                    stud->length
                );
            }

            printf(
                "    Noggins: %zu\n",
                wall->framing.nog_count
            );

            if (wall->framing.nog_count > 0) {

                int current_height =
                    wall->framing.nogs[0].position.y;

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

void printCurrentWall(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        return;
    }

    Room *room =
        app_current_room(
            app
        );

    Wall *wall =
        app_current_wall(
            app
        );

    if (room == NULL) {
        printf(
            "No valid room selected.\n"
        );
        return;
    }

    if (wall == NULL) {
        printf(
            "A room is selected, "
            "but no valid wall is selected.\n"
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

    DomainId opening_id =
        domain_id_generate(
            &app->project.domain_ids
        );

    if (opening_id == DOMAIN_ID_INVALID) {
        fprintf(
            stderr,
            "Could not allocate opening identity.\n"
        );
        return;
    }

    if (!wall_add_opening(
            wall,
            &app->project.settings,
            opening_id,
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