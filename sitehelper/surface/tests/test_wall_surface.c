#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wall_surface.h"
#include "wall.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int reject_allocation(void)
{
    if (fail_after != SIZE_MAX && fail_after-- == 0) {
        allocation_failed = 1;
        return 1;
    }
    return 0;
}
void *__wrap_malloc(size_t size) { return reject_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return reject_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *p, size_t size) { return reject_allocation() ? NULL : __real_realloc(p, size); }
#endif

static BuildSettings settings(void)
{
    return (BuildSettings){.stud_height = 2400, .stud_depth = 90, .stud_width = 35,
        .stud_spacing = 600, .nog_spacing = 1200, .opening_width_allowance = 20,
        .opening_height_allowance = 10, .stud_spacing_mode = STUD_SPACING_MAXIMISE};
}

static Wall fixture(const BuildSettings *s)
{
    Wall wall = {.id = 42};
    assert(wall_set_plan_segment(&wall, (WallPlanSegment){{100, -200}, {6100, -200}}));
    /* Stored order deliberately differs from spatial and identity order. */
    assert(wall_add_opening(&wall, s, 90, OPENING_DOOR, 3500, 0, 900, 2100));
    assert(wall_add_opening(&wall, s, 12, OPENING_WINDOW, 900, 700, 1200, 1000));
    Opening custom = {.id = 4, .type = OPENING_WINDOW, .frame_position = 5000,
        .frame_bottom = 800, .width = 700, .height = 800, .custom_allowance = true,
        .width_allowance = -20, .height_allowance = 30};
    assert(wall_add_opening_definition(&wall, s, &custom));
    return wall; /* Valid authoritative definitions, no generated construction. */
}

static void equivalent(const WallSurface *a, const WallSurface *b)
{
    assert(a->source_wall_id == b->source_wall_id);
    assert(a->width_mm == b->width_mm && a->height_mm == b->height_mm);
    assert(a->opening_count == b->opening_count);
    for (size_t i = 0; i < a->opening_count; i++) {
        const WallSurfaceOpening *x = &a->openings[i], *y = &b->openings[i];
        assert(x->source_opening_id == y->source_opening_id && x->type == y->type);
        assert(x->left_u_mm == y->left_u_mm && x->bottom_z_mm == y->bottom_z_mm);
        assert(x->width_mm == y->width_mm && x->height_mm == y->height_mm);
    }
}

static void assert_zero(const WallSurface *s)
{
    assert(s->source_wall_id == 0 && s->width_mm == 0 && s->height_mm == 0);
    assert(s->openings == NULL && s->opening_count == 0);
}

static void *copy_bytes(const void *source, size_t size)
{
    if (size == 0) { return NULL; }
    void *copy = malloc(size); assert(copy);
    memcpy(copy, source, size);
    return copy;
}

static void unchanged(const WallSurface *surface, const WallSurface *before,
    const WallSurfaceOpening *copy)
{
    assert(memcmp(surface, before, sizeof *surface) == 0);
    if (surface->opening_count) {
        assert(memcmp(surface->openings, copy, surface->opening_count * sizeof *copy) == 0);
    }
}

static void check_apertures(const WallSurface *surface, const Wall *wall, const BuildSettings *s)
{
    assert(surface->source_wall_id == wall->id && surface->width_mm == wall_length_mm(wall));
    assert(surface->opening_count == wall->definition.opening_count);
    for (size_t i = 0; i < surface->opening_count; i++) {
        WallOpeningFrameGeometry expected;
        const Opening *source = &wall->definition.openings[i];
        const WallSurfaceOpening *actual = &surface->openings[i];
        assert(wall_opening_frame_geometry(source, s, &expected));
        assert(actual->source_opening_id == source->id && actual->type == source->type);
        assert(actual->left_u_mm == expected.left_u && actual->bottom_z_mm == expected.bottom_z);
        assert(actual->width_mm == expected.width && actual->height_mm == expected.height);
        assert(expected.right_u <= surface->width_mm && expected.top_z <= surface->height_mm);
    }
}

static void test_lengths_orientations_and_empty(void)
{
    BuildSettings s = settings();
    WallPlanSegment segments[] = {
        {{100, 500}, {6100, 500}}, {{500, -400}, {500, 3200}},
        {{-200, 300}, {2800, 4300}}, {{2800, 4300}, {-200, 300}},
        {{0, 0}, {1, 2}}, {{0, 0}, {1, 1}}, {{INT_MIN, 0}, {-1, 0}}
    };
    int widths[] = {6000, 3600, 5000, 5000, 2, 1, INT_MAX};
    WallSurface surface = {0};
    for (size_t i = 0; i < sizeof widths / sizeof widths[0]; i++) {
        Wall wall = {.id = 10 + i, .definition.segment = segments[i]};
        assert(wall_surface_build(&wall, &s, 2700, &surface).code == WALL_SURFACE_SUCCESS);
        assert(surface.source_wall_id == wall.id && surface.width_mm == widths[i]);
        assert(surface.height_mm == 2700 && surface.opening_count == 0 && surface.openings == NULL);
        s.stud_height = 1; /* Does not set or constrain the requested height. */
    }
    wall_surface_destroy(&surface); assert_zero(&surface);
    wall_surface_destroy(&surface); wall_surface_destroy(NULL); assert_zero(&surface);
}

static void test_apertures_allowances_height_and_order(void)
{
    BuildSettings s = settings();
    Wall wall = fixture(&s);
    WallSurface surface = {0}, again = {0};
    assert(wall_surface_build(&wall, &s, 2800, &surface).code == WALL_SURFACE_SUCCESS);
    check_apertures(&surface, &wall, &s);
    WallSurfaceOpening expected[] = {
        {90, OPENING_DOOR, 3500, 0, 920, 2110},
        {12, OPENING_WINDOW, 900, 700, 1220, 1010},
        {4, OPENING_WINDOW, 5000, 800, 680, 830}
    };
    WallSurface oracle = {.source_wall_id = 42, .width_mm = 6000, .height_mm = 2800,
        .openings = expected, .opening_count = 3}; /* Borrowed test oracle only. */
    equivalent(&surface, &oracle);
    assert(wall_surface_build(&wall, &s, 2800, &again).code == WALL_SURFACE_SUCCESS);
    equivalent(&surface, &again);
    s.stud_height = -1; s.stud_width = 0; s.stud_depth = -1;
    s.nog_spacing = 0; s.stud_spacing = 0;
    assert(wall_surface_build(&wall, &s, 2800, &again).code == WALL_SURFACE_SUCCESS);
    equivalent(&surface, &again); /* No full construction-settings validator. */
    assert(wall_surface_build(&wall, &s, 2110, &again).code == WALL_SURFACE_SUCCESS);
    assert(again.height_mm == 2110); /* An aperture can touch the top boundary. */
    WallSurfaceResult status = wall_surface_build(&wall, &s, 2109, &again);
    assert(status.code == WALL_SURFACE_OPENING_OUT_OF_BOUNDS && status.opening_id == 90);
    assert(again.height_mm == 2110);

    s.opening_width_allowance = -10; s.opening_height_allowance = 0;
    assert(wall_surface_build(&wall, &s, 2800, &again).code == WALL_SURFACE_SUCCESS);
    assert(again.openings[0].width_mm == 890 && again.openings[0].height_mm == 2100);
    assert(again.openings[2].width_mm == 680 && again.openings[2].height_mm == 830);
    Opening swap = wall.definition.openings[0];
    wall.definition.openings[0] = wall.definition.openings[2]; wall.definition.openings[2] = swap;
    assert(wall_surface_build(&wall, &s, 2800, &again).code == WALL_SURFACE_SUCCESS);
    check_apertures(&again, &wall, &s);
    assert(again.openings[0].source_opening_id == 4 && again.openings[2].source_opening_id == 90);
    wall_surface_destroy(&surface); wall_surface_destroy(&again); wall_destroy(&wall);
}

static void test_framing_independence_inputs_and_lifetime(void)
{
    BuildSettings s = settings();
    Wall wall = fixture(&s);
    assert(wall.framing.stud_count == 0 && wall.framing.studs == NULL);
    WallSurface surface = {0}, other = {0};
    assert(wall_surface_build(&wall, &s, 2700, &surface).code == WALL_SURFACE_SUCCESS);
    WallSurface before_surface = surface;
    WallSurfaceOpening *surface_copy = copy_bytes(surface.openings, surface.opening_count * sizeof *surface_copy);
    Timber sentinel = {.length = INT_MIN, .type = (TimberType)99};
    Wall malformed = {.id = wall.id, .definition = wall.definition, .framing = {
        .studs = &sentinel, .stud_count = SIZE_MAX, .stud_capacity = 0,
        .nogs = NULL, .nog_count = SIZE_MAX, .nog_capacity = SIZE_MAX,
        .members = &sentinel, .member_count = SIZE_MAX, .member_capacity = 1,
        .bottomplate = {.length = -1}, .topplate = {.width = -1}}};
    Wall malformed_before = malformed;
    assert(wall_surface_build(&malformed, &s, 2700, &other).code == WALL_SURFACE_SUCCESS);
    equivalent(&surface, &other);
    assert(memcmp(&malformed, &malformed_before, sizeof malformed) == 0);
    assert(sentinel.length == INT_MIN && sentinel.type == (TimberType)99);
    /* malformed borrows definitions and stack storage: never destroy it. */

    assert(wall_generate(&wall, &s));
    Wall before_wall = wall; BuildSettings before_settings = s;
    Opening *openings_copy = copy_bytes(wall.definition.openings, wall.definition.opening_count * sizeof *openings_copy);
    const Timber *arrays[] = {wall.framing.studs, wall.framing.nogs, wall.framing.members};
    size_t counts[] = {wall.framing.stud_count, wall.framing.nog_count, wall.framing.member_count};
    void *copies[3];
    for (size_t i = 0; i < 3; i++) { copies[i] = copy_bytes(arrays[i], counts[i] * sizeof(Timber)); }
    assert(wall_surface_build(&wall, &s, 2700, &other).code == WALL_SURFACE_SUCCESS);
    equivalent(&surface, &other);
    assert(memcmp(&wall, &before_wall, sizeof wall) == 0);
    assert(memcmp(&s, &before_settings, sizeof s) == 0);
    assert(memcmp(wall.definition.openings, openings_copy, wall.definition.opening_count * sizeof *openings_copy) == 0);
    for (size_t i = 0; i < 3; i++) {
        if (counts[i]) { assert(memcmp(arrays[i], copies[i], counts[i] * sizeof(Timber)) == 0); }
        free(copies[i]);
    }
    free(openings_copy);
    wall.framing.studs[0].length = -1;
    wall.framing.bottomplate.length = INT_MAX;
    assert(wall_surface_build(&wall, &s, 2700, &other).code == WALL_SURFACE_SUCCESS);
    equivalent(&surface, &other);
    wall_framing_destroy(&wall.framing);
    assert(wall_surface_build(&wall, &s, 2700, &other).code == WALL_SURFACE_SUCCESS);
    equivalent(&surface, &other);
    unchanged(&surface, &before_surface, surface_copy);
    wall.definition.openings[0].width = 100;
    wall.id = 123;
    wall.definition.segment = (WallPlanSegment){{0, 0}, {10000, 0}};
    assert(wall_surface_build(&wall, &s, 3000, &other).code == WALL_SURFACE_SUCCESS);
    assert(other.source_wall_id == 123 && other.width_mm == 10000 && other.height_mm == 3000);
    assert(other.openings[0].width_mm == 120);
    wall_destroy(&wall);
    unchanged(&surface, &before_surface, surface_copy);
    free(surface_copy);
    Wall empty = {.id = 8, .definition.segment = {{0, 0}, {1200, 0}}};
    assert(wall_surface_build(&empty, &s, 100, &other).code == WALL_SURFACE_SUCCESS);
    assert(other.opening_count == 0 && other.openings == NULL && other.height_mm == 100);
    wall_surface_destroy(&other); wall_surface_destroy(&surface); assert_zero(&surface);
    wall_surface_destroy(&surface);
}

static void test_failures_are_transactional(void)
{
    BuildSettings s = settings();
    Wall wall = fixture(&s);
    for (int populated = 0; populated < 2; populated++) {
        WallSurface surface = {0};
        if (populated) { assert(wall_surface_build(&wall, &s, 2700, &surface).code == WALL_SURFACE_SUCCESS); }
        WallSurface before = surface;
        WallSurfaceOpening *copy = copy_bytes(surface.openings, surface.opening_count * sizeof *copy);
        for (int fault = 0; fault < 23; fault++) {
            Wall bad = {.id = wall.id, .definition = wall.definition};
            Opening entries[3]; memcpy(entries, wall.definition.openings, sizeof entries);
            bad.definition.openings = entries; bad.definition.opening_capacity = 3;
            Opening *opening = &entries[2];
            const Wall *input = &bad; const BuildSettings *configuration = &s;
            WallSurface *output = &surface;
            int height = 2700;
            WallSurfaceCode expected = WALL_SURFACE_INVALID_ARGUMENT;
            DomainId offending = 0;
            switch (fault) {
            case 0: input = NULL; break;
            case 1: configuration = NULL; break;
            case 2: output = NULL; break;
            case 3: height = 0; expected = WALL_SURFACE_INVALID_EXTENT; break;
            case 4: height = -1; expected = WALL_SURFACE_INVALID_EXTENT; break;
            case 5: bad.id = 0; expected = WALL_SURFACE_INVALID_SOURCE_WALL; break;
            case 6: bad.definition.segment = (WallPlanSegment){0}; expected = WALL_SURFACE_INVALID_SOURCE_WALL; break;
            case 7: bad.definition.segment = (WallPlanSegment){{INT_MIN, 0}, {INT_MAX, 0}};
                expected = WALL_SURFACE_INVALID_SOURCE_WALL; break;
            case 8: opening->id = 0; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 9: opening->type = (OpeningType)99; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 10: opening->width = 0; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 11: opening->height = -1; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 12: opening->frame_position = -1; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 13: opening->frame_bottom = -1; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 14: opening->width_allowance = -opening->width; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 15: opening->height_allowance = INT_MIN; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 16: opening->width = INT_MAX; opening->width_allowance = 1; expected = WALL_SURFACE_INVALID_OPENING; break;
            case 17: opening->frame_position = INT_MAX; expected = WALL_SURFACE_OPENING_OUT_OF_BOUNDS; break;
            case 18: opening->frame_bottom = INT_MAX; expected = WALL_SURFACE_OPENING_OUT_OF_BOUNDS; break;
            case 19: bad.definition.opening_count = 4; expected = WALL_SURFACE_INVALID_SOURCE_WALL; break;
            case 20: bad.definition.openings = NULL; expected = WALL_SURFACE_INVALID_SOURCE_WALL; break;
            case 21: bad.definition.opening_capacity = SIZE_MAX; expected = WALL_SURFACE_NUMERIC_OVERFLOW; break;
            case 22: height = 2109; expected = WALL_SURFACE_OPENING_OUT_OF_BOUNDS; break;
            }
            if (fault >= 8 && fault <= 18) { offending = opening->id; }
            if (fault == 22) { offending = entries[0].id; }
            Wall before_wall = bad; BuildSettings before_settings = s;
            Opening before_entries[3]; memcpy(before_entries, entries, sizeof entries);
            WallSurfaceResult status = wall_surface_build(input, configuration, height, output);
            assert(status.code == expected && status.opening_id == offending);
            if (fault > 2) { assert(status.wall_id == bad.id); }
            unchanged(&surface, &before, copy);
            assert(memcmp(&bad, &before_wall, sizeof bad) == 0);
            assert(memcmp(entries, before_entries, sizeof entries) == 0);
            assert(memcmp(&s, &before_settings, sizeof s) == 0);
        }
        free(copy);
        assert(wall_surface_build(&wall, &s, 2800, &surface).code == WALL_SURFACE_SUCCESS);
        check_apertures(&surface, &wall, &s);
        wall_surface_destroy(&surface);
    }
    wall_destroy(&wall);
}

static void test_numeric_aperture_boundaries(void)
{
    BuildSettings s = {.opening_width_allowance = INT_MAX, .opening_height_allowance = INT_MIN};
    Opening opening = {.id = 9, .type = OPENING_WINDOW, .width = INT_MAX, .height = INT_MAX,
        .custom_allowance = true};
    Wall wall = {.id = 8, .definition = {.segment = {{0, 0}, {INT_MAX, 0}},
        .openings = &opening, .opening_count = 1, .opening_capacity = 1}};
    WallSurface surface = {0};
    /* Aperture-only boundary fixture: no framing-assembly margins are implied. */
    assert(wall_surface_build(&wall, &s, INT_MAX, &surface).code == WALL_SURFACE_SUCCESS);
    assert(surface.width_mm == INT_MAX && surface.height_mm == INT_MAX);
    assert(surface.openings[0].width_mm == INT_MAX && surface.openings[0].height_mm == INT_MAX);
    WallSurface before = surface;
    WallSurfaceOpening copy = surface.openings[0];
    opening.custom_allowance = false;
    assert(wall_surface_build(&wall, &s, INT_MAX, &surface).code == WALL_SURFACE_INVALID_OPENING);
    unchanged(&surface, &before, &copy);
    s.opening_width_allowance = 0; s.opening_height_allowance = 1;
    assert(wall_surface_build(&wall, &s, INT_MAX, &surface).code == WALL_SURFACE_INVALID_OPENING);
    unchanged(&surface, &before, &copy);
    s.opening_height_allowance = 0;
    opening.frame_position = INT_MAX; opening.width = 1;
    assert(wall_surface_build(&wall, &s, INT_MAX, &surface).code == WALL_SURFACE_OPENING_OUT_OF_BOUNDS);
    unchanged(&surface, &before, &copy);
    opening.frame_position = 0; opening.frame_bottom = INT_MAX; opening.height = 1;
    assert(wall_surface_build(&wall, &s, INT_MAX, &surface).code == WALL_SURFACE_OPENING_OUT_OF_BOUNDS);
    unchanged(&surface, &before, &copy);
    wall_surface_destroy(&surface);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    BuildSettings s = settings();
    Wall wall = fixture(&s);
    size_t failures = 0;
    for (int populated = 0; populated < 2; populated++) {
        for (size_t n = 0; ; n++) {
            WallSurface surface = {0};
            if (populated) { assert(wall_surface_build(&wall, &s, 2700, &surface).code == WALL_SURFACE_SUCCESS); }
            WallSurface before = surface;
            WallSurfaceOpening *copy = copy_bytes(surface.openings, surface.opening_count * sizeof *copy);
            fail_after = n; allocation_failed = 0;
            WallSurfaceResult status = wall_surface_build(&wall, &s, 2800, &surface);
            fail_after = SIZE_MAX;
            if (allocation_failed) {
                failures++;
                assert(status.code == WALL_SURFACE_ALLOCATION_FAILED && status.wall_id == wall.id);
                unchanged(&surface, &before, copy);
                assert(wall_surface_build(&wall, &s, 2800, &surface).code == WALL_SURFACE_SUCCESS);
            } else { assert(status.code == WALL_SURFACE_SUCCESS); }
            assert(surface.height_mm == 2800); check_apertures(&surface, &wall, &s);
            free(copy); wall_surface_destroy(&surface);
            if (!allocation_failed) { break; }
            assert(n < 16);
        }
    }
    assert(failures >= 2);
    printf("wall-surface allocation failures checked: %zu\n", failures);
    wall_destroy(&wall);
}
#endif

int main(void)
{
    test_lengths_orientations_and_empty();
    test_apertures_allowances_height_and_order();
    test_framing_independence_inputs_and_lifetime();
    test_failures_are_transactional();
    test_numeric_aperture_boundaries();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    puts("wall-surface tests passed");
    return 0;
}
