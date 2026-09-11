#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "sitehelper_persistence.h"
#include "wall.h"
#include "test_support.h"

static const char *round_trip_path =
    "sitehelper_persistence_round_trip.txt";

static const char *malformed_path =
    "sitehelper_persistence_malformed.txt";

static const char *unsupported_path =
    "sitehelper_persistence_unsupported.txt";

static const char *invalid_path =
    "sitehelper_persistence_invalid.txt";

static void write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    assert(fputs(text, file) != EOF);
    assert(fclose(file) == 0);
}

static Wall *add_wall(
    SiteHelperProject *project,
    int length
)
{
    DomainId wall_id = sitehelper_project_add_wall(project, project->storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });
    assert(wall_id != DOMAIN_ID_INVALID);


    Wall *wall = build_find_wall_by_id(&project->storeys[0].structure, wall_id);
    assert(wall != NULL);
    assert(wall_set_plan_segment(wall, (WallPlanSegment){ .end = { .x = length } }));

    return wall;
}

static void add_opening(
    SiteHelperProject *project,
    Wall *wall,
    OpeningType type,
    int frame_position,
    int frame_bottom,
    int width,
    int height,
    int width_allowance,
    int height_allowance,
    bool custom_allowance
)
{
    DomainId opening_id = domain_id_generate(&project->domain_ids);
    assert(opening_id != DOMAIN_ID_INVALID);

    Opening opening = {
        .id = opening_id,
        .type = type,
        .frame_position = frame_position,
        .frame_bottom = frame_bottom,
        .width = width,
        .height = height,
        .width_allowance = width_allowance,
        .height_allowance = height_allowance,
        .custom_allowance = custom_allowance
    };

    assert(wall_add_opening_definition(
        wall,
        &project->settings,
        &opening
    ));
}

static void generate_project(SiteHelperProject *project)
{
    for (size_t wall_index = 0;
         wall_index < project->storeys[0].structure.wall_count;
         wall_index++) {

        assert(wall_generate(
            &project->storeys[0].structure.walls[wall_index],
            &project->settings
        ));
    }
}

static void make_non_trivial_project(SiteHelperProject *project)
{
    sitehelper_project_init(project);
    assert(sitehelper_project_add_storey(project, 0));

    project->settings = (BuildSettings){
        .stud_height = 2700,
        .stud_depth = 70,
        .stud_width = 45,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .opening_width_allowance = 10,
        .opening_height_allowance = 15,
        .stud_spacing_mode = STUD_SPACING_EVEN
    };

    for (int room_number = 0; room_number < 3; room_number++) {
        DomainId room_id = sitehelper_project_add_room(project, project->storeys[0].id);
        assert(room_id != DOMAIN_ID_INVALID);

        Wall *first_wall = add_wall(
            project,
            7000 + room_number * 100
        );
        assert(wall_set_plan_segment(first_wall, (WallPlanSegment){
            .start = {room_number * 10000, room_number * 3000},
            .end = {room_number * 10000 + 7000 + room_number * 100,
                room_number * 3000}
        }));

        add_opening(
            project,
            first_wall,
            OPENING_WINDOW,
            1200,
            700,
            900,
            1000,
            25,
            30,
            true
        );

        add_opening(
            project,
            first_wall,
            OPENING_DOOR,
            4200,
            0,
            850,
            2100,
            0,
            0,
            false
        );

        Wall *second_wall = add_wall(
            project,
            4600 + room_number * 100
        );
        assert(wall_set_plan_segment(second_wall, (WallPlanSegment){
            .start = {room_number * 10000 + 5000, room_number * 3000 + 1500},
            .end = {room_number * 10000 + 9600 + room_number * 100,
                room_number * 3000 + 1500}
        }));

        add_opening(
            project,
            second_wall,
            OPENING_WINDOW,
            1700,
            800,
            800,
            900,
            0,
            0,
            false
        );
    }

    generate_project(project);
}

static void assert_timber_equal(const Timber *expected, const Timber *actual)
{
    assert(expected->length == actual->length);
    assert(expected->depth == actual->depth);
    assert(expected->width == actual->width);
    assert(expected->position.u == actual->position.u);
    assert(expected->position.z == actual->position.z);
    assert(expected->type == actual->type);

    switch (expected->type) {
        case TIMBER_STUD:
            assert(expected->details.stud.type == actual->details.stud.type);
            break;

        case TIMBER_NOGGIN:
            assert(expected->details.noggin.bay == actual->details.noggin.bay);
            break;

        case TIMBER_PLATE:
            assert(expected->details.plate.placeholder ==
                actual->details.plate.placeholder);
            break;

        case TIMBER_HEADER:
        case TIMBER_SILL:
            break;
    }
}

static void assert_framing_equal(
    const WallFraming *expected,
    const WallFraming *actual
)
{
    assert_timber_equal(&expected->bottomplate, &actual->bottomplate);
    assert_timber_equal(&expected->topplate, &actual->topplate);
    assert(expected->stud_count == actual->stud_count);
    assert(expected->nog_count == actual->nog_count);
    assert(expected->member_count == actual->member_count);

    for (size_t index = 0; index < expected->stud_count; index++) {
        assert_timber_equal(&expected->studs[index], &actual->studs[index]);
    }

    for (size_t index = 0; index < expected->nog_count; index++) {
        assert_timber_equal(&expected->nogs[index], &actual->nogs[index]);
    }

    for (size_t index = 0; index < expected->member_count; index++) {
        assert_timber_equal(&expected->members[index], &actual->members[index]);
    }
}

static void assert_project_equal(const SiteHelperProject *expected, const SiteHelperProject *actual)
{
    test_assert_project_authoritative_equal(expected, actual);
    for (size_t i = 0; i < expected->storeys[0].structure.wall_count; i++) {
        const Wall *wall = &expected->storeys[0].structure.walls[i];
        const Wall *loaded = build_find_wall_by_id_const(&actual->storeys[0].structure, wall->id);
        assert_framing_equal(&wall->framing, &loaded->framing);
    }
}

static void test_round_trip_rebuilds_framing_and_preserves_identity(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;

    make_non_trivial_project(&original);
    original.domain_ids.next = 100;
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));

    assert(sitehelper_project_save_file(&original, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);

    assert_project_equal(&original, &loaded);

    sitehelper_project_destroy(&loaded);
    sitehelper_project_destroy(&original);
    remove(round_trip_path);
}

static void test_generator_history_survives_round_trip(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;

    make_non_trivial_project(&original);
    original.domain_ids.next = 500;
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));

    assert(sitehelper_project_save_file(&original, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.domain_ids.next == 500);
    assert(sitehelper_project_add_room(&loaded, loaded.storeys[0].id) == 500);

    sitehelper_project_destroy(&loaded);
    sitehelper_project_destroy(&original);
    remove(round_trip_path);
}

static void test_malformed_load_is_transactional(void)
{
    SiteHelperProject destination;
    SiteHelperProject expected;

    make_non_trivial_project(&destination);
    make_non_trivial_project(&expected);

    write_text_file(malformed_path,
        "sitehelper_project 1\n"
        "domain_id_next 8\n"
        "settings 2400 90 35 600\n");

    assert(sitehelper_project_load_file(&destination, malformed_path) ==
        SITEHELPER_PERSISTENCE_MALFORMED_DATA);
    assert_project_equal(&expected, &destination);

    sitehelper_project_destroy(&expected);
    sitehelper_project_destroy(&destination);
    remove(malformed_path);
}

static void test_unsupported_version_is_rejected(void)
{
    SiteHelperProject destination;
    sitehelper_project_init(&destination);
    assert(sitehelper_project_add_storey(&destination, 0));

    write_text_file(unsupported_path,
        "sitehelper_project 10\n");

    assert(sitehelper_project_load_file(&destination, unsupported_path) ==
        SITEHELPER_PERSISTENCE_UNSUPPORTED_VERSION);

    sitehelper_project_destroy(&destination);
    remove(unsupported_path);
}

static void test_invalid_identity_data_is_rejected(void)
{
    SiteHelperProject destination;
    sitehelper_project_init(&destination);
    assert(sitehelper_project_add_storey(&destination, 0));

    write_text_file(invalid_path,
        "sitehelper_project 1\n"
        "domain_id_next 1\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "rooms 1\n"
        "room 1 walls 0\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, invalid_path) ==
        SITEHELPER_PERSISTENCE_INVALID_PROJECT);

    write_text_file(invalid_path,
        "sitehelper_project 1\n"
        "domain_id_next 2\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "rooms 1\n"
        "room 0 walls 0\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, invalid_path) ==
        SITEHELPER_PERSISTENCE_MALFORMED_DATA);

    write_text_file(invalid_path,
        "sitehelper_project 1\n"
        "domain_id_next 3\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "rooms 1\n"
        "room 1 walls 1\n"
        "wall 1 length 4200 openings 0\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, invalid_path) ==
        SITEHELPER_PERSISTENCE_MALFORMED_DATA);

    write_text_file(invalid_path,
        "sitehelper_project 1\n"
        "domain_id_next 0\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "rooms 0\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, invalid_path) ==
        SITEHELPER_PERSISTENCE_MALFORMED_DATA);

    sitehelper_project_destroy(&destination);
    remove(invalid_path);
}

static void test_invalid_domain_data_is_rejected(void)
{
    SiteHelperProject destination;
    sitehelper_project_init(&destination);
    assert(sitehelper_project_add_storey(&destination, 0));

    write_text_file(invalid_path,
        "sitehelper_project 1\n"
        "domain_id_next 2\n"
        "settings 0 90 35 600 1200 0 0 maximise\n"
        "rooms 0\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, invalid_path) ==
        SITEHELPER_PERSISTENCE_INVALID_PROJECT);

    write_text_file(invalid_path,
        "sitehelper_project 1\n"
        "domain_id_next 3\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "rooms 1\n"
        "room 1 walls 1\n"
        "wall 2 length 4200 openings 1\n"
        "opening 3 window 10 800 800 1000 0 0 false\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, invalid_path) ==
        SITEHELPER_PERSISTENCE_INVALID_PROJECT);

    write_text_file(invalid_path,
        "sitehelper_project 1\n"
        "domain_id_next 3\n"
        "settings 2400 90 5000 600 1200 0 0 maximise\n"
        "rooms 1\n"
        "room 1 walls 1\n"
        "wall 2 length 4200 openings 0\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, invalid_path) ==
        SITEHELPER_PERSISTENCE_REGENERATION_FAILED);

    sitehelper_project_destroy(&destination);
    remove(invalid_path);
}

static void test_minimal_project_round_trips(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;

    sitehelper_project_init(&original);
    assert(sitehelper_project_add_storey(&original, 0));
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));

    assert(sitehelper_project_save_file(&original, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert_project_equal(&original, &loaded);

    sitehelper_project_destroy(&loaded);
    sitehelper_project_destroy(&original);
    remove(round_trip_path);
}

static void test_version_one_wall_defaults_origin_to_zero(void)
{
    SiteHelperProject destination;
    sitehelper_project_init(&destination);
    assert(sitehelper_project_add_storey(&destination, 0));

    write_text_file(round_trip_path,
        "sitehelper_project 1\n"
        "domain_id_next 3\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "rooms 1\n"
        "room 1 walls 1\n"
        "wall 2 length 4200 openings 0\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);

    const Room *room = build_find_room_by_id_const(
        &destination.storeys[0].structure,
        1
    );
    assert(room != NULL);
    const Wall *wall = build_find_wall_by_id_const(&destination.storeys[0].structure, 2);
    assert(wall != NULL);
    assert(wall->definition.segment.start.x == 0);
    assert(wall->definition.segment.start.y == 0);
    assert(wall->definition.segment.end.x == 4200);
    assert(wall->definition.segment.end.y == 0);

    sitehelper_project_destroy(&destination);
    remove(round_trip_path);
}

static void test_version_two_wall_preserves_origin(void)
{
    SiteHelperProject destination;
    sitehelper_project_init(&destination);
    assert(sitehelper_project_add_storey(&destination, 0));

    write_text_file(round_trip_path,
        "sitehelper_project 2\n"
        "domain_id_next 3\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "rooms 1\n"
        "room 1 walls 1\n"
        "wall 2 origin 5000 3000 length 4200 openings 0\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    const Wall *wall = build_find_wall_by_id_const(&destination.storeys[0].structure, 2);
    assert(wall != NULL);
    assert(wall->definition.segment.start.x == 5000);
    assert(wall->definition.segment.start.y == 3000);
    assert(wall->definition.segment.end.x == 9200);
    assert(wall->definition.segment.end.y == 3000);

    sitehelper_project_destroy(&destination);
    remove(round_trip_path);
}

static void test_malformed_version_two_origin_is_transactional(void)
{
    SiteHelperProject destination;
    SiteHelperProject expected;
    make_non_trivial_project(&destination);
    make_non_trivial_project(&expected);

    write_text_file(malformed_path,
        "sitehelper_project 2\n"
        "domain_id_next 3\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "rooms 1\n"
        "room 1 walls 1\n"
        "wall 2 origin invalid 3000 length 4200 openings 0\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, malformed_path) ==
        SITEHELPER_PERSISTENCE_MALFORMED_DATA);
    assert_project_equal(&expected, &destination);

    sitehelper_project_destroy(&expected);
    sitehelper_project_destroy(&destination);
    remove(malformed_path);
}

static void test_independent_rooms_and_wall_round_trip(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;
    sitehelper_project_init(&original);
    assert(sitehelper_project_add_storey(&original, 0));
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));

    DomainId first_room = sitehelper_project_add_room(&original, original.storeys[0].id);
    DomainId second_room = sitehelper_project_add_room(&original, original.storeys[0].id);
    Wall *wall = add_wall(&original, 4200);
    assert(wall_set_plan_segment(wall, (WallPlanSegment){
        .start = {5000, 3000}, .end = {9200, 3000}
    }));
    assert(wall_generate(wall, &original.settings));

    assert(sitehelper_project_save_file(&original, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.storeys[0].structure.wall_count == 1);
    assert(build_find_room_by_id_const(&loaded.storeys[0].structure, first_room));
    assert(build_find_room_by_id_const(&loaded.storeys[0].structure, second_room));
    assert(build_find_wall_by_id_const(&loaded.storeys[0].structure, wall->id)->
        definition.segment.start.x == 5000);

    sitehelper_project_destroy(&loaded);
    sitehelper_project_destroy(&original);
    remove(round_trip_path);
}

static void test_malformed_version_three_reference_is_transactional(void)
{
    SiteHelperProject destination;
    SiteHelperProject expected;
    make_non_trivial_project(&destination);
    make_non_trivial_project(&expected);

    write_text_file(malformed_path,
        "sitehelper_project 3\n"
        "domain_id_next 4\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "walls 1\n"
        "wall 2 origin 0 0 length 4200 openings 0\n"
        "rooms 1\n"
        "room 1 wall_refs 1\n"
        "wall_ref 99\n"
        "end_room\n"
        "end_project\n");

    assert(sitehelper_project_load_file(&destination, malformed_path) ==
        SITEHELPER_PERSISTENCE_MALFORMED_DATA);
    assert_project_equal(&expected, &destination);

    sitehelper_project_destroy(&expected);
    sitehelper_project_destroy(&destination);
    remove(malformed_path);
}

static void test_version_nine_ordered_segments_round_trip(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;
    sitehelper_project_init(&original);
    assert(sitehelper_project_add_storey(&original, 0));
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));
    DomainId room_id = sitehelper_project_add_room(&original, original.storeys[0].id);
    assert(room_id == 2);
    const WallPlanSegment segments[] = {
        { .start = {0, 0}, .end = {6000, 0} },
        { .start = {1000, 2000}, .end = {4600, 6800} },
        { .start = {4600, 6800}, .end = {1000, 2000} }
    };
    for (size_t i = 0; i < sizeof segments / sizeof segments[0]; i++) {
        DomainId id = sitehelper_project_add_wall(&original, original.storeys[0].id, segments[i]);
        assert(id != DOMAIN_ID_INVALID);
        Wall *wall = build_find_wall_by_id(&original.storeys[0].structure, id);
        add_opening(&original, wall, OPENING_WINDOW, 1200, 700, 900, 1000,
            25, 30, true);
    }
    generate_project(&original);
    assert(sitehelper_project_save_file(&original, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file = fopen(round_trip_path, "r");
    assert(file != NULL);
    char text[4096];
    size_t count = fread(text, 1, sizeof text - 1, file);
    assert(!ferror(file) && feof(file));
    text[count] = '\0';
    assert(fclose(file) == 0);
    assert(strstr(text, "sitehelper_project 9\n") == text);
    assert(strstr(text, "segment 1000 2000 4600 6800 openings 1") != NULL);
    assert(strstr(text, "segment 4600 6800 1000 2000 openings 1") != NULL);
    assert(strstr(text, " origin ") == NULL && strstr(text, " length ") == NULL);
    assert(strstr(text, "wall_ref") == NULL);
    assert(strstr(text, "room 2 placement unplaced\nend_room\n") != NULL);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert_project_equal(&original, &loaded);
    sitehelper_project_destroy(&original);
    sitehelper_project_destroy(&loaded);
    remove(round_trip_path);
}

static void test_version_three_migrates_shared_horizontal_wall(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    write_text_file(round_trip_path,
        "sitehelper_project 3\n"
        "domain_id_next 4\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "walls 1\n"
        "wall 2 origin -5000 3000 length 4200 openings 0\n"
        "rooms 2\n"
        "room 1 wall_refs 1\nwall_ref 2\nend_room\n"
        "room 3 wall_refs 1\nwall_ref 2\nend_room\nend_project\n");
    assert(sitehelper_project_load_file(&project, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    const Wall *wall = build_find_wall_by_id_const(&project.storeys[0].structure, 2);
    assert(wall != NULL);
    assert(wall->definition.segment.start.x == -5000);
    assert(wall->definition.segment.start.y == 3000);
    assert(wall->definition.segment.end.x == -800);
    assert(wall->definition.segment.end.y == 3000);
    assert(project.storeys[0].structure.wall_count == 1 && project.storeys[0].structure.room_count == 2);
    sitehelper_project_destroy(&project);
    remove(round_trip_path);
}

static void test_invalid_segment_files_are_transactional(void)
{
    SiteHelperProject destination;
    SiteHelperProject expected;
    make_non_trivial_project(&destination);
    make_non_trivial_project(&expected);
    const char *records[] = {
        "segment 10 20 10 20 openings 0",
        "segment invalid 20 30 40 openings 0",
        "segment 0 0 6000 openings 0",
        "segment 0 0 6000 0 length 6000 openings 0",
        "segment 0 0 999999999999999999999999 0 openings 0"
    };
    char text[1024];
    for (size_t i = 0; i < sizeof records / sizeof records[0]; i++) {
        snprintf(text, sizeof text,
            "sitehelper_project 4\ndomain_id_next 5\n"
            "settings 2400 90 35 600 1200 0 0 maximise\n"
            "walls 2\nwall 4 segment 0 0 6000 0 openings 0\n"
            "wall 2 %s\nrooms 1\n"
            "room 1 wall_refs 1\nwall_ref 2\nend_room\nend_project\n",
            records[i]);
        write_text_file(malformed_path, text);
        assert(sitehelper_project_load_file(&destination, malformed_path) ==
            (i == 0 ? SITEHELPER_PERSISTENCE_INVALID_PROJECT :
                SITEHELPER_PERSISTENCE_MALFORMED_DATA));
        assert_project_equal(&expected, &destination);
    }
    snprintf(text, sizeof text,
        "sitehelper_project 4\ndomain_id_next 3\n"
        "settings 2400 90 35 600 1200 0 0 maximise\n"
        "walls 1\nwall 2 segment %d 0 %d 0 openings 0\n"
        "rooms 0\nend_project\n", INT_MIN, INT_MAX);
    write_text_file(malformed_path, text);
    assert(sitehelper_project_load_file(&destination, malformed_path) ==
        SITEHELPER_PERSISTENCE_INVALID_PROJECT);
    assert_project_equal(&expected, &destination);

    /* Historical origin + length must not overflow during migration. */
    for (int version = 2; version <= 3; version++) {
        snprintf(text, sizeof text,
            "sitehelper_project %d\ndomain_id_next 3\n"
            "settings 2400 90 35 600 1200 0 0 maximise\n%s"
            "wall 2 origin %d 0 length 4200 openings 0\n",
            version, version == 2 ? "rooms 1\nroom 1 walls 1\n" : "walls 1\n",
            INT_MAX);
        write_text_file(malformed_path, text);
        assert(sitehelper_project_load_file(&destination, malformed_path) ==
            SITEHELPER_PERSISTENCE_INVALID_PROJECT);
        assert_project_equal(&expected, &destination);
    }
    sitehelper_project_destroy(&destination);
    sitehelper_project_destroy(&expected);
    remove(malformed_path);
}

static void test_save_consumes_project_validation_before_opening_file(void)
{
    SiteHelperProject project, expected;
    make_non_trivial_project(&project);
    make_non_trivial_project(&expected);
    const char *sentinel = "existing file must survive rejected save\n";
    for (int kind = 0; kind < 9; kind++) {
        DomainIdGenerator saved_ids = project.domain_ids;
        Wall *saved_walls = project.storeys[0].structure.walls;
        size_t saved_wall_capacity = project.storeys[0].structure.wall_capacity;
        size_t saved_room_capacity = project.storeys[0].structure.room_capacity;
        Room *room = &project.storeys[0].structure.rooms[0];
        Wall *wall = &project.storeys[0].structure.walls[0];
        Room saved_room = *room;
        Wall saved_wall = *wall;
        Opening saved_opening = wall->definition.openings[0];
        switch (kind) {
            case 0: project.storeys[0].structure.walls = NULL; break;
            case 1: project.storeys[0].structure.wall_capacity = 0; break;
            case 2: project.storeys[0].structure.room_capacity = 0; break;
            case 3: room->id = DOMAIN_ID_INVALID; break;
            case 4: wall->definition.openings = NULL; break;
            case 5: project.domain_ids.next = 1; break;
            case 6: wall->id = room->id; break;
            case 7: wall->definition.openings[0].frame_position = 10; break;
            case 8: wall->definition.opening_capacity = 0; break;
        }
        write_text_file(invalid_path, sentinel);
        assert(sitehelper_project_validate(&project).code != SITEHELPER_PROJECT_VALID);
        assert(sitehelper_project_save_file(&project, invalid_path) == SITEHELPER_PERSISTENCE_INVALID_PROJECT);
        FILE *file = fopen(invalid_path, "r");
        char content[128];
        assert(file && fgets(content, sizeof content, file));
        assert(strcmp(content, sentinel) == 0 && fgetc(file) == EOF);
        assert(fclose(file) == 0);
        project.domain_ids = saved_ids;
        project.storeys[0].structure.walls = saved_walls;
        project.storeys[0].structure.wall_capacity = saved_wall_capacity;
        project.storeys[0].structure.room_capacity = saved_room_capacity;
        *room = saved_room;
        *wall = saved_wall;
        wall->definition.openings[0] = saved_opening;
        assert_project_equal(&expected, &project);
    }
    sitehelper_project_destroy(&project);
    sitehelper_project_destroy(&expected);
    assert(remove(invalid_path) == 0);
}

static void test_complete_invalid_candidates_and_regeneration_are_transactional(void)
{
    SiteHelperProject destination, expected;
    make_non_trivial_project(&destination);
    make_non_trivial_project(&expected);
    const struct {
        const char *text;
        SiteHelperPersistenceResult result;
    } cases[] = {
        /* Syntax and incremental construction succeed; the final project
         * validation rejects the allocator watermark before regeneration. */
        {"sitehelper_project 4\ndomain_id_next 2\n"
         "settings 2400 90 35 600 1200 0 0 maximise\n"
         "walls 1\nwall 2 segment 0 0 4200 0 openings 0\nrooms 0\nend_project\n",
         SITEHELPER_PERSISTENCE_INVALID_PROJECT},
        {"sitehelper_project 4\ndomain_id_next 3\n"
         "settings 2400 90 5000 600 1200 0 0 maximise\n"
         "walls 1\nwall 2 segment 0 0 4200 0 openings 0\nrooms 0\nend_project\n",
         SITEHELPER_PERSISTENCE_REGENERATION_FAILED},
        {"sitehelper_project 4\ndomain_id_next 5\n"
         "settings 2400 90 35 600 1200 0 0 maximise\n"
         "walls 1\nwall 2 segment 0 0 4200 0 openings 2\n"
         "opening 3 door 500 0 800 2000 0 0 false\n"
         "opening 4 door 600 0 800 2000 0 0 false\nrooms 0\nend_project\n",
         SITEHELPER_PERSISTENCE_INVALID_PROJECT},
        {"sitehelper_project 4\ndomain_id_next 3\n"
         "settings 2400 90 35 600 1200 0 0 maximise\n"
         "walls 1\nwall 2 segment 0 0 4200 0 openings 0\n"
         "rooms 1\nroom 1 wall_refs 2\nwall_ref 2\nwall_ref 2\nend_room\nend_project\n",
         SITEHELPER_PERSISTENCE_MALFORMED_DATA}
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        write_text_file(invalid_path, cases[i].text);
        assert(sitehelper_project_load_file(&destination, invalid_path) == cases[i].result);
        assert_project_equal(&expected, &destination);
        assert(sitehelper_project_validate(&destination).code == SITEHELPER_PROJECT_VALID);
    }
    assert(remove(invalid_path) == 0);
    assert(sitehelper_project_load_file(&destination, invalid_path) == SITEHELPER_PERSISTENCE_IO_ERROR);
    assert_project_equal(&expected, &destination);
    sitehelper_project_destroy(&destination);
    sitehelper_project_destroy(&expected);
}

static void test_save_accepts_authoritative_project_without_framing(void)
{
    SiteHelperProject project, loaded;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));
    assert(project.storeys[0].structure.room_count == 0);
    DomainId id = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{4600, 6800}, {1000, 2000}});
    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, id);
    assert(wall && wall->framing.stud_count == 0);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    assert(sitehelper_project_save_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(wall->framing.stud_count == 0);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_validate(&loaded).code == SITEHELPER_PROJECT_VALID);
    assert(build_find_wall_by_id(&loaded.storeys[0].structure, id)->framing.stud_count > 0);
    assert(loaded.domain_ids.next == project.domain_ids.next);
    sitehelper_project_destroy(&project);
    sitehelper_project_destroy(&loaded);
    assert(remove(round_trip_path) == 0);
}

static void test_versions_one_through_six_migrate_without_separators(void)
{
    for (int version = 1; version <= 6; version++) {
        char text[2048];
        const char *geometry = version == 1 ? "length 6000" :
            version < 4 ? "origin 1000 2000 length 6000" : "segment 4600 6800 1000 2000";
        snprintf(text, sizeof text,
            "sitehelper_project %d\ndomain_id_next 5\n"
            "settings 2400 90 35 600 1200 0 0 maximise\n%s"
            "wall 2 %s openings 1\n"
            "opening 3 window 1200 900 800 1000 12 15 true\n%s",
            version,
            version < 3 ? "rooms 2\nroom 1 walls 1\n" : "walls 1\n",
            geometry,
            version < 3 ? "end_room\nroom 4 walls 0\nend_room\nend_project\n" :
                version == 6 ? "rooms 2\nroom 1 placement placed -12 34\nend_room\n"
                    "room 4 placement unplaced\nend_room\nend_project\n" :
                version == 5 ? "rooms 2\nroom 1\nend_room\nroom 4\nend_room\nend_project\n" :
                "rooms 2\nroom 1 wall_refs 1\nwall_ref 2\nend_room\n"
                "room 4 wall_refs 1\nwall_ref 2\nend_room\nend_project\n");
        write_text_file(round_trip_path, text);
        SiteHelperProject project, loaded;
        sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
        sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));
        assert(sitehelper_project_load_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
        assert(project.storeys[0].structure.room_count == 2 && project.storeys[0].structure.wall_count == 1);
        const Room *room = build_find_room_by_id_const(&project.storeys[0].structure, 1);
        assert(room && room->has_location == (version == 6));
        if (version == 6) { assert(room->location.x == -12 && room->location.y == 34); }
        assert(!build_find_room_by_id(&project.storeys[0].structure, 4)->has_location);
        assert(project.storeys[0].structure.room_separator_count == 0 && project.storeys[0].structure.room_separators == NULL);
        assert(project.storeys[0].id == 5 && project.domain_ids.next == 6);
        const Wall *wall = build_find_wall_by_id_const(&project.storeys[0].structure, 2);
        assert(wall && wall_length_mm(wall) == 6000);
        WallPlanSegment expected = version == 1 ? (WallPlanSegment){{0, 0}, {6000, 0}} :
            version < 4 ? (WallPlanSegment){{1000, 2000}, {7000, 2000}} :
                (WallPlanSegment){{4600, 6800}, {1000, 2000}};
        assert(wall->definition.segment.start.x == expected.start.x);
        assert(wall->definition.segment.start.y == expected.start.y);
        assert(wall->definition.segment.end.x == expected.end.x);
        assert(wall->definition.segment.end.y == expected.end.y);
        assert(wall->definition.opening_count == 1);
        const Opening *opening = wall_find_opening_by_id_const(wall, 3);
        assert(opening && opening->frame_position == 1200 && opening->width == 800);
        assert(opening->custom_allowance && opening->width_allowance == 12 && opening->height_allowance == 15);
        assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
        assert(sitehelper_project_save_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
        FILE *file = fopen(round_trip_path, "r");
        assert(file);
        size_t count = fread(text, 1, sizeof text - 1, file);
        assert(feof(file) && !ferror(file));
        text[count] = '\0';
        assert(fclose(file) == 0);
        assert(strstr(text, "sitehelper_project 9\n") == text);
        assert(strstr(text, "wall_ref") == NULL);
        assert(strstr(text, version == 6 ? "room 1 placement placed -12 34\nend_room\n" :
                "room 1 placement unplaced\nend_room\n") &&
            strstr(text, "room 4 placement unplaced\nend_room\n"));
        assert(sitehelper_project_load_file(&loaded, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
        assert_project_equal(&project, &loaded);
        sitehelper_project_destroy(&loaded);
        sitehelper_project_destroy(&project);
        assert(remove(round_trip_path) == 0);
    }
}

static void test_legacy_duplicate_references_and_v5_references_are_rejected(void)
{
    SiteHelperProject destination, expected;
    make_non_trivial_project(&destination);
    make_non_trivial_project(&expected);
    for (int version = 3; version <= 5; version++) {
        char text[1024];
        snprintf(text, sizeof text,
            "sitehelper_project %d\ndomain_id_next 4\n"
            "settings 2400 90 35 600 1200 0 0 maximise\n"
            "walls 2\nwall 2 %s openings 0\nwall 3 %s openings 0\n"
            "rooms 1\nroom 1 wall_refs 2\nwall_ref 2\nwall_ref 2\nend_room\nend_project\n",
            version,
            version == 3 ? "origin 0 0 length 4200" : "segment 0 0 4200 0",
            version == 3 ? "origin 0 5000 length 4200" : "segment 0 5000 4200 5000");
        write_text_file(malformed_path, text);
        assert(sitehelper_project_load_file(&destination, malformed_path) == SITEHELPER_PERSISTENCE_MALFORMED_DATA);
        assert_project_equal(&expected, &destination);
    }
    sitehelper_project_destroy(&destination);
    sitehelper_project_destroy(&expected);
    assert(remove(malformed_path) == 0);
}

static void test_room_placement_round_trip(void)
{
    SiteHelperProject project, loaded;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));
    DomainId unplaced = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId origin = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId far = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId cleared = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(sitehelper_project_set_room_location(&project, origin, (PlanPosition){0}));
    assert(sitehelper_project_set_room_location(&project, far, (PlanPosition){INT_MIN, INT_MAX}));
    assert(sitehelper_project_set_room_location(&project, cleared, (PlanPosition){12, -34}));
    assert(sitehelper_project_clear_room_location(&project, cleared));
    assert(sitehelper_project_save_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file = fopen(round_trip_path, "r");
    char text[2048];
    assert(file);
    size_t count = fread(text, 1, sizeof text - 1, file);
    assert(feof(file) && !ferror(file) && fclose(file) == 0);
    text[count] = '\0';
    assert(strstr(text, "sitehelper_project 9\n") == text);
    assert(strstr(text, "room 2 placement unplaced\nend_room\n"));
    assert(strstr(text, "room 3 placement placed 0 0\nend_room\n"));
    assert(strstr(text, "room 5 placement unplaced\nend_room\n"));
    assert(sitehelper_project_load_file(&loaded, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert_project_equal(&project, &loaded);
    assert(!build_find_room_by_id_const(&loaded.storeys[0].structure, unplaced)->has_location);
    assert(build_find_room_by_id_const(&loaded.storeys[0].structure, origin)->has_location);
    assert(!build_find_room_by_id_const(&loaded.storeys[0].structure, cleared)->has_location);
    assert(sitehelper_project_set_room_location(&loaded, far, (PlanPosition){12345, -67890}));
    assert(sitehelper_project_save_file(&loaded, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert_project_equal(&loaded, &project);
    sitehelper_project_destroy(&project);
    sitehelper_project_destroy(&loaded);
    assert(remove(round_trip_path) == 0);
}

static void test_malformed_room_placement_is_transactional(void)
{
    const char *records[] = {
        "room 2\nend_room\n", /* v6 requires explicit placement state. */
        "room 2 placement unknown\nend_room\n",
        "room 2 placement placed 1\nend_room\n",
        "room 2 placement placed 1 2.5\nend_room\n",
        "room 2 placement placed 999999999999999999999999 2\nend_room\n",
        "room 2 placement placed 1 -999999999999999999999999\nend_room\n",
        "room 2 placement placed x 2\nend_room\n",
        "room 2 placement unplaced 0 0\nend_room\n",
        "room 2 placement placed 1 2 placement unplaced\nend_room\n",
        "room 2 placement placed 1 2\n", /* Missing end_room. */
        "room 2 placement placed 1 2 3\nend_room\n"
    };
    SiteHelperProject destination, expected;
    make_non_trivial_project(&destination);
    make_non_trivial_project(&expected);
    DomainId room_id = destination.storeys[0].structure.rooms[0].id;
    assert(sitehelper_project_set_room_location(&destination, room_id, (PlanPosition){-7, 9}));
    assert(sitehelper_project_set_room_location(&expected, room_id, (PlanPosition){-7, 9}));
    for (size_t i = 0; i < sizeof records / sizeof records[0]; i++) {
        char text[1024];
        snprintf(text, sizeof text,
            "sitehelper_project 7\ndomain_id_next 3\n"
            "settings 2400 90 35 600 1200 0 0 maximise\nwalls 0\nrooms 2\n"
            "room 1 placement placed 123 -456\nend_room\n%sroom_separators 0\nend_project\n", records[i]);
        write_text_file(malformed_path, text);
        assert(sitehelper_project_load_file(&destination, malformed_path) == SITEHELPER_PERSISTENCE_MALFORMED_DATA);
        assert_project_equal(&expected, &destination);
    }
    sitehelper_project_destroy(&destination);
    sitehelper_project_destroy(&expected);
    assert(remove(malformed_path) == 0);
}

static void test_room_separators_round_trip(void)
{
    SiteHelperProject project, loaded;
    make_non_trivial_project(&project);
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));
    assert(sitehelper_project_set_room_location(&project, project.storeys[0].structure.rooms[0].id, (PlanPosition){-17, 29}));
    PlanSegment segments[] = {
        {{5000, 6000}, {1000, 2000}}, {{INT_MIN, INT_MAX}, {INT_MAX, INT_MIN}},
        {{5000, 6000}, {1000, 2000}}
    };
    DomainId next = project.domain_ids.next;
    for (size_t i = 0; i < sizeof segments / sizeof segments[0]; i++) {
        assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, segments[i]) == next + i);
    }
    assert(sitehelper_project_save_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert_project_equal(&project, &loaded);
    assert(loaded.storeys[0].structure.room_separator_count == 3 && loaded.domain_ids.next == next + 3);
    assert(sitehelper_project_validate(&loaded).code == SITEHELPER_PROJECT_VALID);
    /* Save checks authoritative separator metadata before opening/truncating a file. */
    RoomSeparator *storage = project.storeys[0].structure.room_separators;
    project.storeys[0].structure.room_separators = NULL;
    assert(sitehelper_project_save_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_INVALID_PROJECT);
    project.storeys[0].structure.room_separators = storage;
    assert(sitehelper_project_load_file(&loaded, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert_project_equal(&project, &loaded);
    sitehelper_project_destroy(&project);
    sitehelper_project_destroy(&loaded);
    assert(remove(round_trip_path) == 0);

    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, segments[0]));
    assert(sitehelper_project_save_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert_project_equal(&project, &loaded);
    assert(loaded.storeys[0].structure.room_count == 0 && loaded.storeys[0].structure.wall_count == 0);
    sitehelper_project_destroy(&project);
    sitehelper_project_destroy(&loaded);
    assert(remove(round_trip_path) == 0);
}

static void test_malformed_room_separators_are_transactional(void)
{
    struct { const char *records; SiteHelperPersistenceResult result; } cases[] = {
        {"", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators -1\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 4 segment 0 0 0 0\n", SITEHELPER_PERSISTENCE_INVALID_PROJECT},
        {"room_separators 1\nroom_separator 0 segment 0 0 1 2\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 1 segment 0 0 1 2\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 2 segment 0 0 1 2\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 3 segment 0 0 1 2\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 2\nroom_separator 4 segment 0 0 1 2\nroom_separator 4 segment 2 3 4 5\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 4 segment 0 1 x 2\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 4 segment 0 1 2\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 4 segment 9999999999999999999999 1 2 3\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 4 segment 0 1 2 -9999999999999999999999\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 0\nroom_separator 4 segment 0 1 2 3\n", SITEHELPER_PERSISTENCE_MALFORMED_DATA},
        {"room_separators 1\nroom_separator 6 segment 0 1 2 3\n", SITEHELPER_PERSISTENCE_INVALID_PROJECT}
    };
    SiteHelperProject destination, expected;
    make_non_trivial_project(&destination);
    make_non_trivial_project(&expected);
    assert(sitehelper_project_add_room_separator(&destination, destination.storeys[0].id, (PlanSegment){{-1, 2}, {3, -4}}));
    assert(sitehelper_project_add_room_separator(&expected, expected.storeys[0].id, (PlanSegment){{-1, 2}, {3, -4}}));
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        char text[2048];
        snprintf(text, sizeof text,
            "sitehelper_project 7\ndomain_id_next 6\nsettings 2400 90 35 600 1200 0 0 maximise\n"
            "walls 1\nwall 2 segment 0 0 6000 0 openings 1\n"
            "opening 3 window 1200 900 800 1000 0 0 false\n"
            "rooms 1\nroom 1 placement placed 0 0\nend_room\n%send_project\n", cases[i].records);
        write_text_file(malformed_path, text);
        assert(sitehelper_project_load_file(&destination, malformed_path) == cases[i].result);
        assert_project_equal(&expected, &destination);
    }
    sitehelper_project_destroy(&destination);
    sitehelper_project_destroy(&expected);
    assert(remove(malformed_path) == 0);
}

int main(void)
{
    test_room_separators_round_trip();
    test_malformed_room_separators_are_transactional();
    test_room_placement_round_trip();
    test_malformed_room_placement_is_transactional();
    test_versions_one_through_six_migrate_without_separators();
    test_legacy_duplicate_references_and_v5_references_are_rejected();
    test_save_consumes_project_validation_before_opening_file();
    test_complete_invalid_candidates_and_regeneration_are_transactional();
    test_save_accepts_authoritative_project_without_framing();
    test_round_trip_rebuilds_framing_and_preserves_identity();
    test_generator_history_survives_round_trip();
    test_malformed_load_is_transactional();
    test_unsupported_version_is_rejected();
    test_invalid_identity_data_is_rejected();
    test_invalid_domain_data_is_rejected();
    test_minimal_project_round_trips();
    test_version_one_wall_defaults_origin_to_zero();
    test_version_two_wall_preserves_origin();
    test_malformed_version_two_origin_is_transactional();
    test_independent_rooms_and_wall_round_trip();
    test_malformed_version_three_reference_is_transactional();
    test_version_nine_ordered_segments_round_trip();
    test_version_three_migrates_shared_horizontal_wall();
    test_invalid_segment_files_are_transactional();

    puts("sitehelper persistence tests passed");
    return 0;
}
