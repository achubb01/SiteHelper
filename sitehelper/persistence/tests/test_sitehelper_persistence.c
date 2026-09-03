#include <assert.h>
#include <stdio.h>
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
    DomainId wall_id = sitehelper_project_add_wall(project, room_id);
    assert(wall_id != DOMAIN_ID_INVALID);

    Room *room = build_find_room_by_id(&project->structure, room_id);
    assert(room != NULL);

    assert(room_has_wall_id(room, wall_id));
    Wall *wall = build_find_wall_by_id(&project->structure, wall_id);
    assert(wall != NULL);
    assert(wall_set_length(wall, length));

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
        assert(wall_set_origin(first_wall, (Position){
            .x = room_number * 10000,
            .y = room_number * 3000
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
        assert(wall_set_origin(second_wall, (Position){
            .x = room_number * 10000 + 5000,
            .y = room_number * 3000 + 1500
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
    assert(expected->position.x == actual->position.x);
    assert(expected->position.y == actual->position.y);
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
            assert(expected_wall->definition.origin.x ==
                actual_wall->definition.origin.x);
            assert(expected_wall->definition.origin.y ==
                actual_wall->definition.origin.y);
            assert(expected_wall->definition.length ==
                actual_wall->definition.length);
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
        "sitehelper_project 4\n");

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
    assert(wall->definition.origin.x == 0);
    assert(wall->definition.origin.y == 0);

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
    assert(wall->definition.origin.x == 5000);
    assert(wall->definition.origin.y == 3000);

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

static void test_version_three_shared_wall_references_round_trip(void)
{
    SiteHelperProject original;
    SiteHelperProject loaded;
    sitehelper_project_init(&original);
    sitehelper_project_init(&loaded);

    DomainId first_room = sitehelper_project_add_room(&original);
    DomainId second_room = sitehelper_project_add_room(&original);
    Wall *wall = add_wall(&original, first_room, 4200);
    wall->definition.origin = (Position){ 5000, 3000 };
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
        definition.origin.x == 5000);

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

int main(void)
{
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
    test_version_three_shared_wall_references_round_trip();
    test_malformed_version_three_reference_is_transactional();

    puts("sitehelper persistence tests passed");
    return 0;
}
