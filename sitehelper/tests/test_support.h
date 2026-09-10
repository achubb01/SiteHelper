#ifndef SITEHELPER_TEST_SUPPORT_H
#define SITEHELPER_TEST_SUPPORT_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sitehelper_project.h"
#include "wall.h"

static inline int test_timber_equal_value(
    const Timber *expected,
    const Timber *actual
)
{
    if (expected == NULL || actual == NULL) {
        return expected == actual;
    }

    if (expected->length != actual->length ||
        expected->depth != actual->depth ||
        expected->width != actual->width ||
        expected->position.u != actual->position.u ||
        expected->position.z != actual->position.z ||
        expected->type != actual->type) {
        return 0;
    }

    switch (expected->type) {
        case TIMBER_STUD:
            return expected->details.stud.type == actual->details.stud.type;

        case TIMBER_NOGGIN:
            return expected->details.noggin.bay == actual->details.noggin.bay;

        case TIMBER_PLATE:
            return expected->details.plate.placeholder ==
                actual->details.plate.placeholder;

        case TIMBER_HEADER:
        case TIMBER_SILL:
        default:
            return 1;
    }
}

static inline void test_assert_timber_equal(
    const Timber *expected,
    const Timber *actual
)
{
    assert(test_timber_equal_value(expected, actual));
}

static inline size_t test_timber_occurrences(
    const Timber *members,
    size_t member_count,
    const Timber *target
)
{
    size_t occurrences = 0;

    for (size_t i = 0; i < member_count; i++) {
        if (test_timber_equal_value(&members[i], target)) {
            occurrences++;
        }
    }

    return occurrences;
}

static inline void test_assert_timber_multiset_equal(
    const Timber *expected,
    size_t expected_count,
    const Timber *actual,
    size_t actual_count
)
{
    assert(expected_count == actual_count);

    for (size_t i = 0; i < expected_count; i++) {
        assert(
            test_timber_occurrences(
                expected,
                expected_count,
                &expected[i]
            ) ==
            test_timber_occurrences(
                actual,
                actual_count,
                &expected[i]
            )
        );
    }
}

static inline void test_assert_framing_semantically_equal(
    const WallFraming *expected,
    const WallFraming *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    test_assert_timber_equal(
        &expected->bottomplate,
        &actual->bottomplate
    );

    test_assert_timber_equal(
        &expected->topplate,
        &actual->topplate
    );

    test_assert_timber_multiset_equal(
        expected->studs,
        expected->stud_count,
        actual->studs,
        actual->stud_count
    );

    test_assert_timber_multiset_equal(
        expected->nogs,
        expected->nog_count,
        actual->nogs,
        actual->nog_count
    );

    test_assert_timber_multiset_equal(
        expected->members,
        expected->member_count,
        actual->members,
        actual->member_count
    );
}

static inline void test_assert_build_settings_equal(
    const BuildSettings *expected,
    const BuildSettings *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    assert(expected->stud_height == actual->stud_height);
    assert(expected->stud_depth == actual->stud_depth);
    assert(expected->stud_width == actual->stud_width);
    assert(expected->stud_spacing == actual->stud_spacing);
    assert(expected->nog_spacing == actual->nog_spacing);
    assert(expected->opening_width_allowance == actual->opening_width_allowance);
    assert(expected->opening_height_allowance == actual->opening_height_allowance);
    assert(expected->stud_spacing_mode == actual->stud_spacing_mode);
}

static inline void test_assert_opening_equal(
    const Opening *expected,
    const Opening *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    assert(expected->id == actual->id);
    assert(expected->type == actual->type);
    assert(expected->frame_position == actual->frame_position);
    assert(expected->frame_bottom == actual->frame_bottom);
    assert(expected->width == actual->width);
    assert(expected->height == actual->height);
    assert(expected->width_allowance == actual->width_allowance);
    assert(expected->height_allowance == actual->height_allowance);
    assert(expected->custom_allowance == actual->custom_allowance);
}

static inline void test_assert_wall_definition_equal(
    const Wall *expected,
    const Wall *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    assert(expected->id == actual->id);
    assert(expected->definition.segment.start.x == actual->definition.segment.start.x);
    assert(expected->definition.segment.start.y == actual->definition.segment.start.y);
    assert(expected->definition.segment.end.x == actual->definition.segment.end.x);
    assert(expected->definition.segment.end.y == actual->definition.segment.end.y);
    assert(expected->definition.opening_count == actual->definition.opening_count);

    for (size_t i = 0; i < expected->definition.opening_count; i++) {
        const Opening *expected_opening = &expected->definition.openings[i];
        const Opening *actual_opening = wall_find_opening_by_id_const(
            actual,
            expected_opening->id
        );

        assert(actual_opening != NULL);
        test_assert_opening_equal(expected_opening, actual_opening);
    }
}

static inline void test_assert_project_model_equal(
    const SiteHelperProject *expected,
    const SiteHelperProject *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    test_assert_build_settings_equal(&expected->settings, &actual->settings);
    assert(expected->structure.room_count == actual->structure.room_count);
    assert(expected->structure.wall_count == actual->structure.wall_count);
    assert(expected->structure.room_separator_count == actual->structure.room_separator_count);
    for (size_t i = 0; i < expected->structure.room_separator_count; i++) {
        const RoomSeparator *a = &expected->structure.room_separators[i];
        const RoomSeparator *b = &actual->structure.room_separators[i];
        assert(a->id == b->id);
        assert(a->segment.start.x == b->segment.start.x && a->segment.start.y == b->segment.start.y);
        assert(a->segment.end.x == b->segment.end.x && a->segment.end.y == b->segment.end.y);
    }

    for (size_t i = 0; i < expected->structure.room_count; i++) {
        const Room *expected_room = &expected->structure.rooms[i];
        const Room *actual_room = build_find_room_by_id_const(
            &actual->structure,
            expected_room->id
        );

        assert(actual_room != NULL);
        assert(expected_room->has_location == actual_room->has_location);
        if (expected_room->has_location) {
            assert(expected_room->location.x == actual_room->location.x);
            assert(expected_room->location.y == actual_room->location.y);
        }
    }

    for (size_t i = 0; i < expected->structure.wall_count; i++) {
        const Wall *expected_wall = &expected->structure.walls[i];
        const Wall *actual_wall = build_find_wall_by_id_const(
            &actual->structure,
            expected_wall->id
        );

        assert(actual_wall != NULL);
        test_assert_wall_definition_equal(expected_wall, actual_wall);
    }
}

static inline void test_assert_project_authoritative_equal(
    const SiteHelperProject *expected,
    const SiteHelperProject *actual
)
{
    test_assert_project_model_equal(expected, actual);
    assert(expected->domain_ids.next == actual->domain_ids.next);
}

static inline void test_clone_wall_definition(
    const Wall *source,
    Wall *destination
)
{
    assert(source != NULL);
    assert(destination != NULL);

    *destination = (Wall){
        .id = source->id,
        .definition = {
            .segment = source->definition.segment
        }
    };

    if (source->definition.opening_count == 0) {
        return;
    }

    destination->definition.openings = malloc(
        source->definition.opening_count *
        sizeof *destination->definition.openings
    );

    assert(destination->definition.openings != NULL);

    memcpy(
        destination->definition.openings,
        source->definition.openings,
        source->definition.opening_count *
        sizeof *destination->definition.openings
    );

    destination->definition.opening_count =
        source->definition.opening_count;
    destination->definition.opening_capacity =
        source->definition.opening_count;
}

static inline void test_clone_project_authoritative(
    const SiteHelperProject *source,
    SiteHelperProject *destination
)
{
    assert(source != NULL);
    assert(destination != NULL);

    *destination = (SiteHelperProject){
        .settings = source->settings,
        .domain_ids = source->domain_ids
    };

    for (size_t i = 0; i < source->structure.wall_count; i++) {
        Wall wall = {0};
        test_clone_wall_definition(&source->structure.walls[i], &wall);
        assert(build_append_wall(&destination->structure, &wall));
    }

    for (size_t i = 0; i < source->structure.room_count; i++) {
        const Room *source_room = &source->structure.rooms[i];

        assert(build_add_room(&destination->structure, source_room->id));

        Room *destination_room = build_find_room_by_id(
            &destination->structure,
            source_room->id
        );

        assert(destination_room != NULL);
        if (source_room->has_location) {
            assert(sitehelper_project_set_room_location(destination,
                source_room->id, source_room->location));
        }
    }
    for (size_t i = 0; i < source->structure.room_separator_count; i++) {
        assert(build_insert_room_separator(&destination->structure, &source->structure.room_separators[i], i));
    }
}

#endif
