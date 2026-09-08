#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "sitehelper_persistence.h"
#include "wall.h"

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
    DomainId room_id,
    int length
)
{
    DomainId wall_id = sitehelper_project_add_wall(project, room_id, (WallPlanSegment){ .end = { .x = 4200 } });
    assert(wall_id != DOMAIN_ID_INVALID);

    Room *room = build_find_room_by_id(&project->structure, room_id);
    assert(room != NULL);

    assert(room_has_wall_id(room, wall_id));
    Wall *wall = build_find_wall_by_id(&project->structure, wall_id);
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
         wall_index < project->structure.wall_count;
         wall_index++) {

        assert(wall_generate(
            &project->structure.walls[wall_index],
            &project->settings
        ));
    }
}

static void make_non_trivial_project(SiteHelperProject *project)
{
    sitehelper_project_init(project);

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
        DomainId room_id = sitehelper_project_add_room(project);
        assert(room_id != DOMAIN_ID_INVALID);

        Wall *first_wall = add_wall(
            project,
            room_id,
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
            room_id,
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

static void assert_project_equal(
    const SiteHelperProject *expected,
    const SiteHelperProject *actual
)
{
    assert(expected->settings.stud_height == actual->settings.stud_height);
    assert(expected->settings.stud_depth == actual->settings.stud_depth);
    assert(expected->settings.stud_width == actual->settings.stud_width);
    assert(expected->settings.stud_spacing == actual->settings.stud_spacing);
    assert(expected->settings.nog_spacing == actual->settings.nog_spacing);
    assert(expected->settings.opening_width_allowance ==
        actual->settings.opening_width_allowance);
    assert(expected->settings.opening_height_allowance ==
        actual->settings.opening_height_allowance);
    assert(expected->settings.stud_spacing_mode ==
        actual->settings.stud_spacing_mode);
    assert(expected->domain_ids.next == actual->domain_ids.next);
    assert(expected->structure.room_count == actual->structure.room_count);
    assert(expected->structure.wall_count == actual->structure.wall_count);

    for (size_t room_index = 0;
         room_index < expected->structure.room_count;
         room_index++) {

        const Room *expected_room = &expected->structure.rooms[room_index];
        const Room *actual_room = &actual->structure.rooms[room_index];

        assert(expected_room->id == actual_room->id);
        assert(expected_room->wall_count == actual_room->wall_count);

        for (size_t wall_index = 0;
             wall_index < expected_room->wall_count;
             wall_index++) {

            assert(expected_room->wall_ids[wall_index] ==
                actual_room->wall_ids[wall_index]);
            const Wall *expected_wall = build_find_wall_by_id_const(
                &expected->structure,
                expected_room->wall_ids[wall_index]
            );
            const Wall *actual_wall = build_find_wall_by_id_const(
                &actual->structure,
                actual_room->wall_ids[wall_index]
            );
            assert(expected_wall != NULL && actual_wall != NULL);

            assert(expected_wall->id == actual_wall->id);
            assert(expected_wall->definition.segment.start.x ==
                actual_wall->definition.segment.start.x);
            assert(expected_wall->definition.segment.start.y ==
                actual_wall->definition.segment.start.y);
            assert(wall_length_mm(expected_wall) ==
                wall_length_mm(actual_wall));
            assert(expected_wall->definition.segment.end.x ==
                actual_wall->definition.segment.end.x);
            assert(expected_wall->definition.segment.end.y ==
                actual_wall->definition.segment.end.y);
            assert(expected_wall->definition.opening_count ==
                actual_wall->definition.opening_count);

            for (size_t opening_index = 0;
                 opening_index < expected_wall->definition.opening_count;
                 opening_index++) {

                const Opening *expected_opening =
                    &expected_wall->definition.openings[opening_index];
                const Opening *actual_opening =
                    &actual_wall->definition.openings[opening_index];

                assert(expected_opening->id == actual_opening->id);
                assert(expected_opening->type == actual_opening->type);
                assert(expected_opening->frame_position ==
                    actual_opening->frame_position);
                assert(expected_opening->frame_bottom ==
                    actual_opening->frame_bottom);
                assert(expected_opening->width == actual_opening->width);
                assert(expected_opening->height == actual_opening->height);
                assert(expected_opening->width_allowance ==
                    actual_opening->width_allowance);
                assert(expected_opening->height_allowance ==
                    actual_opening->height_allowance);
                assert(expected_opening->custom_allowance ==
                    actual_opening->custom_allowance);
            }

            assert_framing_equal(
                &expected_wall->framing,
                &actual_wall->framing
            );
        }
    }
}

static void test_round_trip_rebuilds_framing_and_preserves_identity(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;

    make_non_trivial_project(&original);
    original.domain_ids.next = 100;
    sitehelper_project_init(&loaded);

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

    assert(sitehelper_project_save_file(&original, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.domain_ids.next == 500);
    assert(sitehelper_project_add_room(&loaded) == 500);

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

    write_text_file(unsupported_path,
        "sitehelper_project 5\n");

    assert(sitehelper_project_load_file(&destination, unsupported_path) ==
        SITEHELPER_PERSISTENCE_UNSUPPORTED_VERSION);

    sitehelper_project_destroy(&destination);
    remove(unsupported_path);
}

static void test_invalid_identity_data_is_rejected(void)
{
    SiteHelperProject destination;
    sitehelper_project_init(&destination);

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
    sitehelper_project_init(&loaded);

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
        &destination.structure,
        1
    );
    assert(room_has_wall_id(room, 2));
    const Wall *wall = build_find_wall_by_id_const(&destination.structure, 2);
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
    const Wall *wall = build_find_wall_by_id_const(&destination.structure, 2);
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

static void test_shared_wall_references_round_trip(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;
    sitehelper_project_init(&original);
    sitehelper_project_init(&loaded);

    DomainId first_room = sitehelper_project_add_room(&original);
    DomainId second_room = sitehelper_project_add_room(&original);
    Wall *wall = add_wall(&original, first_room, 4200);
    assert(wall_set_plan_segment(wall, (WallPlanSegment){
        .start = {5000, 3000}, .end = {9200, 3000}
    }));
    assert(wall_generate(wall, &original.settings));
    assert(room_add_wall_reference(
        build_find_room_by_id(&original.structure, second_room), wall->id));

    assert(sitehelper_project_save_file(&original, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) ==
        SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.structure.wall_count == 1);
    assert(room_has_wall_id(
        build_find_room_by_id_const(&loaded.structure, first_room), wall->id));
    assert(room_has_wall_id(
        build_find_room_by_id_const(&loaded.structure, second_room), wall->id));
    assert(build_find_wall_by_id_const(&loaded.structure, wall->id)->
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

static void test_version_four_ordered_segments_round_trip(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;
    sitehelper_project_init(&original);
    sitehelper_project_init(&loaded);
    DomainId room_id = sitehelper_project_add_room(&original);
    const WallPlanSegment segments[] = {
        { .start = {0, 0}, .end = {6000, 0} },
        { .start = {1000, 2000}, .end = {4600, 6800} },
        { .start = {4600, 6800}, .end = {1000, 2000} }
    };
    for (size_t i = 0; i < sizeof segments / sizeof segments[0]; i++) {
        DomainId id = sitehelper_project_add_wall(&original, room_id, segments[i]);
        assert(id != DOMAIN_ID_INVALID);
        Wall *wall = build_find_wall_by_id(&original.structure, id);
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
    assert(strstr(text, "sitehelper_project 4\n") == text);
    assert(strstr(text, "segment 1000 2000 4600 6800 openings 1") != NULL);
    assert(strstr(text, "segment 4600 6800 1000 2000 openings 1") != NULL);
    assert(strstr(text, " origin ") == NULL && strstr(text, " length ") == NULL);
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
    const Wall *wall = build_find_wall_by_id_const(&project.structure, 2);
    assert(wall != NULL);
    assert(wall->definition.segment.start.x == -5000);
    assert(wall->definition.segment.start.y == 3000);
    assert(wall->definition.segment.end.x == -800);
    assert(wall->definition.segment.end.y == 3000);
    assert(project.structure.wall_count == 1 && project.structure.room_count == 2);
    assert(room_has_wall_id(build_find_room_by_id(&project.structure, 1), 2));
    assert(room_has_wall_id(build_find_room_by_id(&project.structure, 3), 2));
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
        SiteHelperProject saved_project = project;
        Room *room = &project.structure.rooms[0];
        Wall *wall = &project.structure.walls[0];
        Room saved_room = *room;
        Wall saved_wall = *wall;
        Opening saved_opening = wall->definition.openings[0];
        DomainId saved_reference = room->wall_ids[1];
        switch (kind) {
            case 0: project.structure.walls = NULL; break;
            case 1: project.structure.wall_capacity = 0; break;
            case 2: project.structure.room_capacity = 0; break;
            case 3: room->wall_ids = NULL; break;
            case 4: wall->definition.openings = NULL; break;
            case 5: project.domain_ids.next = 1; break;
            case 6: wall->id = room->id; break;
            case 7: wall->definition.openings[0].frame_position = 10; break;
            case 8: room->wall_ids[1] = room->wall_ids[0]; break;
        }
        write_text_file(invalid_path, sentinel);
        assert(sitehelper_project_validate(&project).code != SITEHELPER_PROJECT_VALID);
        assert(sitehelper_project_save_file(&project, invalid_path) == SITEHELPER_PERSISTENCE_INVALID_PROJECT);
        FILE *file = fopen(invalid_path, "r");
        char content[128];
        assert(file && fgets(content, sizeof content, file));
        assert(strcmp(content, sentinel) == 0 && fgetc(file) == EOF);
        assert(fclose(file) == 0);
        project = saved_project;
        *room = saved_room;
        *wall = saved_wall;
        wall->definition.openings[0] = saved_opening;
        room->wall_ids[1] = saved_reference;
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
    sitehelper_project_init(&loaded);
    DomainId room = sitehelper_project_add_room(&project);
    DomainId id = sitehelper_project_add_wall(&project, room,
        (WallPlanSegment){{4600, 6800}, {1000, 2000}});
    Wall *wall = build_find_wall_by_id(&project.structure, id);
    assert(wall && wall->framing.stud_count == 0);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    assert(sitehelper_project_save_file(&project, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(wall->framing.stud_count == 0);
    assert(sitehelper_project_load_file(&loaded, round_trip_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_validate(&loaded).code == SITEHELPER_PROJECT_VALID);
    assert(build_find_wall_by_id(&loaded.structure, id)->framing.stud_count > 0);
    assert(loaded.domain_ids.next == project.domain_ids.next);
    sitehelper_project_destroy(&project);
    sitehelper_project_destroy(&loaded);
    assert(remove(round_trip_path) == 0);
}

int main(void)
{
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
    test_shared_wall_references_round_trip();
    test_malformed_version_three_reference_is_transactional();
    test_version_four_ordered_segments_round_trip();
    test_version_three_migrates_shared_horizontal_wall();
    test_invalid_segment_files_are_transactional();

    puts("sitehelper persistence tests passed");
    return 0;
}
