#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_project.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static int checking;
static size_t allocations;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
void *__wrap_malloc(size_t size)
{
    if (checking) { allocations++; return NULL; }
    return __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size)
{
    if (checking) { allocations++; return NULL; }
    return __real_calloc(count, size);
}
void *__wrap_realloc(void *pointer, size_t size)
{
    if (checking) { allocations++; return NULL; }
    return __real_realloc(pointer, size);
}
#endif

/* Stack-owned storage makes exact read-only checks possible even for corrupt
 * metadata. Never pass this fixture to the owning project destructor. */
typedef struct
{
    SiteHelperProject project;
    Room rooms[2];
    Wall walls[2];
    DomainId references[2][2];
    Opening openings[2][2];
} Fixture;

static void fixture_init(Fixture *f)
{
    memset(f, 0, sizeof *f);
    sitehelper_project_init(&f->project);
    f->project.domain_ids.next = 8;
    f->project.structure = (BuildStructure){
        .rooms = f->rooms, .room_count = 2, .room_capacity = 2,
        .walls = f->walls, .wall_count = 2, .wall_capacity = 2
    };
    f->references[0][0] = 3;
    f->references[0][1] = 4;
    f->references[1][0] = 3; /* Shared physical wall. */
    f->rooms[0] = (Room){.id = 1, .wall_ids = f->references[0], .wall_count = 2, .wall_capacity = 2};
    f->rooms[1] = (Room){.id = 2, .wall_ids = f->references[1], .wall_count = 1, .wall_capacity = 2};
    for (size_t i = 0; i < 2; i++) {
        f->walls[i] = (Wall){.id = 3 + i, .definition = {
            .segment = {{4600, 6800}, {1000, 2000}},
            .openings = f->openings[i], .opening_count = i == 0 ? 2 : 1, .opening_capacity = 2
        }};
    }
    f->openings[0][0] = (Opening){.id = 5, .type = OPENING_DOOR,
        .frame_position = 500, .width = 800, .height = 2000};
    f->openings[0][1] = (Opening){.id = 6, .type = OPENING_WINDOW,
        .frame_position = 2800, .frame_bottom = 900, .width = 800, .height = 1000,
        .custom_allowance = true, .width_allowance = 12, .height_allowance = 15};
    f->openings[1][0] = (Opening){.id = 7, .type = OPENING_DOOR,
        .frame_position = 1000, .width = 800, .height = 2000};
}

static void assert_result(const SiteHelperProject *project,
    SiteHelperProjectValidationCode code, DomainId subject, DomainId related)
{
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    checking = 1;
    allocations = 0;
#endif
    for (int repeat = 0; repeat < 3; repeat++) {
        SiteHelperProjectValidation result = sitehelper_project_validate(project);
        assert(result.code == code);
        assert(result.subject_id == subject);
        assert(result.related_id == related);
    }
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    checking = 0;
    assert(allocations == 0);
#endif
}

static void check(Fixture *f, SiteHelperProjectValidationCode code, DomainId subject, DomainId related)
{
    unsigned char before[sizeof *f];
    memcpy(before, f, sizeof *f);
    assert_result(&f->project, code, subject, related);
    assert(memcmp(before, f, sizeof *f) == 0);
}

static void test_valid_project_semantics(void)
{
    SiteHelperProject empty;
    sitehelper_project_init(&empty);
    assert_result(&empty, SITEHELPER_PROJECT_VALID, 0, 0);
    assert_result(NULL, SITEHELPER_PROJECT_INVALID_ARGUMENT, 0, 0);
    Fixture f;
    fixture_init(&f);
    check(&f, SITEHELPER_PROJECT_VALID, 0, 0); /* Shared walls, no framing. */
    f.project.structure.room_count = 1;
    check(&f, SITEHELPER_PROJECT_VALID, 0, 0); /* Normal room and walls. */
    f.project.structure.room_count = 0;
    check(&f, SITEHELPER_PROJECT_VALID, 0, 0); /* Unreferenced walls are valid. */
    f.walls[0].framing.stud_count = SIZE_MAX;
    f.walls[0].framing.topplate.length = -1;
    f.walls[1].framing.nog_count = SIZE_MAX;
    f.walls[1].framing.member_count = SIZE_MAX;
    check(&f, SITEHELPER_PROJECT_VALID, 0, 0); /* Derived state is ignored. */
    f.project.domain_ids.next = UINT64_MAX;
    check(&f, SITEHELPER_PROJECT_VALID, 0, 0); /* Preserve allocator boundary. */
}

static void test_settings(void)
{
    Fixture f;
    for (int field = 0; field < 6; field++) {
        fixture_init(&f);
        int *positive_fields[] = {&f.project.settings.stud_height, &f.project.settings.stud_depth,
            &f.project.settings.stud_width, &f.project.settings.stud_spacing, &f.project.settings.nog_spacing};
        if (field < 5) { *positive_fields[field] = 0; }
        else { f.project.settings.stud_spacing_mode = (StudSpacingMode)999; }
        check(&f, SITEHELPER_PROJECT_INVALID_SETTINGS, 0, 0);
    }
    fixture_init(&f);
    f.project.settings.opening_width_allowance = -10;
    f.project.settings.opening_height_allowance = -10;
    check(&f, SITEHELPER_PROJECT_VALID, 0, 0); /* No new allowance policy. */
}

static void test_collection_metadata_precedes_traversal(void)
{
    for (int missing_storage = 0; missing_storage < 2; missing_storage++) {
        Fixture f;
        fixture_init(&f);
        if (missing_storage) { f.project.structure.rooms = NULL; }
        else { f.project.structure.room_capacity = 1; }
        check(&f, SITEHELPER_PROJECT_INVALID_ROOM_COLLECTION, 0, 0);
        fixture_init(&f);
        if (missing_storage) { f.project.structure.walls = NULL; }
        else { f.project.structure.wall_capacity = 1; }
        check(&f, SITEHELPER_PROJECT_INVALID_WALL_COLLECTION, 0, 0);
        fixture_init(&f);
        if (missing_storage) { f.rooms[1].wall_ids = NULL; }
        else { f.rooms[1].wall_capacity = 0; }
        check(&f, SITEHELPER_PROJECT_INVALID_ROOM_REFERENCE_COLLECTION, 2, 0);
        fixture_init(&f);
        if (missing_storage) { f.walls[1].definition.openings = NULL; }
        else { f.walls[1].definition.opening_capacity = 0; }
        check(&f, SITEHELPER_PROJECT_INVALID_OPENING_COLLECTION, 4, 0);
    }
    Fixture f;
    fixture_init(&f);
    f.rooms[0].id = 0;
    f.walls[1].definition.openings = NULL;
    check(&f, SITEHELPER_PROJECT_INVALID_OPENING_COLLECTION, 4, 0);
}

static void test_identity_validity(void)
{
    Fixture f;
    fixture_init(&f);
    f.rooms[0].id = 0;
    check(&f, SITEHELPER_PROJECT_INVALID_ROOM_ID, 0, 0);
    fixture_init(&f);
    f.walls[0].id = 0;
    check(&f, SITEHELPER_PROJECT_INVALID_WALL_ID, 0, 0);
    fixture_init(&f);
    f.openings[0][0].id = 0;
    check(&f, SITEHELPER_PROJECT_INVALID_OPENING_ID, 0, 3);

    /* Room-room, wall-wall, room-wall, room-opening, wall-opening,
     * same-wall opening and different-wall opening collisions. */
    for (int kind = 0; kind < 7; kind++) {
        fixture_init(&f);
        DomainId duplicate = 0;
        switch (kind) {
            case 0: duplicate = f.rooms[1].id = f.rooms[0].id; break;
            case 1: duplicate = f.walls[1].id = f.walls[0].id; break;
            case 2: duplicate = f.walls[0].id = f.rooms[0].id; break;
            case 3: duplicate = f.openings[0][0].id = f.rooms[0].id; break;
            case 4: duplicate = f.openings[0][0].id = f.walls[0].id; break;
            case 5: duplicate = f.openings[0][1].id = f.openings[0][0].id; break;
            case 6: duplicate = f.openings[1][0].id = f.openings[0][0].id; break;
        }
        check(&f, SITEHELPER_PROJECT_DUPLICATE_ID, duplicate, 0);
    }
    for (DomainId next = 0; next <= 7; next++) {
        fixture_init(&f);
        f.project.domain_ids.next = next;
        check(&f, SITEHELPER_PROJECT_INVALID_ID_GENERATOR, 7, 0);
    }
}

static void test_references_and_geometry(void)
{
    Fixture f;
    fixture_init(&f);
    f.references[0][0] = 99;
    check(&f, SITEHELPER_PROJECT_UNRESOLVED_WALL_REFERENCE, 1, 99);
    f.references[0][0] = 0;
    check(&f, SITEHELPER_PROJECT_UNRESOLVED_WALL_REFERENCE, 1, 0);
    fixture_init(&f);
    f.references[0][1] = f.references[0][0];
    check(&f, SITEHELPER_PROJECT_DUPLICATE_WALL_REFERENCE, 1, 3);
    fixture_init(&f);
    f.walls[0].definition.segment.end = f.walls[0].definition.segment.start;
    check(&f, SITEHELPER_PROJECT_INVALID_WALL_GEOMETRY, 3, 0);
    f.walls[0].definition.segment = (WallPlanSegment){{INT_MIN, INT_MIN}, {INT_MAX, INT_MAX}};
    check(&f, SITEHELPER_PROJECT_INVALID_WALL_GEOMETRY, 3, 0);
}

static void test_stored_openings(void)
{
    Fixture f;
    for (int kind = 0; kind < 10; kind++) {
        fixture_init(&f);
        Opening *opening = &f.openings[0][1];
        switch (kind) {
            case 0: opening->type = (OpeningType)999; break;
            case 1: opening->width = 0; break;
            case 2: opening->height = 3000; break;
            case 3: opening->frame_position = 10; break;
            case 4: opening->frame_position = 5800; break;
            case 5: opening->width_allowance = -1000; break;
            case 6: opening->width = INT_MAX; break;
            case 7: opening->frame_bottom = INT_MAX; break;
            case 8: opening->frame_position = INT_MAX; break;
            case 9: opening->height = INT_MAX; break;
        }
        check(&f, SITEHELPER_PROJECT_INVALID_OPENING, 6, 3);
    }
    fixture_init(&f);
    f.openings[0][1].frame_position = 600;
    check(&f, SITEHELPER_PROJECT_OVERLAPPING_OPENINGS, 6, 5);
}

int main(void)
{
    test_valid_project_semantics();
    test_settings();
    test_collection_metadata_precedes_traversal();
    test_identity_validity();
    test_references_and_geometry();
    test_stored_openings();
    puts("project validation tests passed (read-only, deterministic, allocation-free)");
    return 0;
}
