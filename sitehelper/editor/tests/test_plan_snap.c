#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "plan_snap.h"
#include "editor_snap_state.h"
#ifdef SITEHELPER_TEST_TOPOLOGY
#include "wall_junctions.h"
#endif

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static long fail_after = -1, allocation_count;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int fail_allocation(void)
{
    long index = allocation_count++;
    return fail_after >= 0 && index == fail_after;
}
void *__wrap_malloc(size_t size) { return fail_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return fail_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *pointer, size_t size) { return fail_allocation() ? NULL : __real_realloc(pointer, size); }
#endif

static SnapSettings defaults(void)
{
    EditorSnapState state; editor_snap_state_init(&state);
    assert(state.settings.wall_centreline_enabled);
    return state.settings;
}

static Storey geometry(Wall *walls, size_t count)
{
    return (Storey){.structure = {.walls = walls, .wall_count = count, .wall_capacity = count}};
}

static SnapResult snap(const Storey *storey, Vec2 point, SnapSettings settings)
{
    SnapCandidate candidates[PLAN_SNAP_CANDIDATE_CAPACITY];
    size_t count = plan_collect_snap_candidates(storey, point, &settings, candidates);
    assert(count <= 3);
    return editor_snap(point, candidates, count, &settings);
}

static void expect(SnapResult result, SnapType type, double x, double y)
{
    assert(result.type == type);
    assert(fabs(result.position.x - x) < 1e-9 && fabs(result.position.y - y) < 1e-9);
}

static void test_endpoints_and_finite_projection(void)
{
    Wall wall = {.id = 1, .definition.segment = {{100,200},{1100,200}}};
    Storey storey = geometry(&wall, 1);
    SnapSettings settings = defaults();
    expect(snap(&storey, (Vec2){95,205}, settings), SNAP_ENDPOINT, 100,200);
    expect(snap(&storey, (Vec2){1105,205}, settings), SNAP_ENDPOINT, 1100,200);
    expect(snap(&storey, (Vec2){650,205}, settings), SNAP_WALL_CENTRELINE, 650,200);
    expect(snap(&storey, (Vec2){1400,205}, settings), SNAP_GRID, 1400,200);
    settings.endpoint_enabled = 0;
    expect(snap(&storey, (Vec2){95,205}, settings), SNAP_WALL_CENTRELINE, 100,200);
    expect(snap(&storey, (Vec2){1105,205}, settings), SNAP_WALL_CENTRELINE, 1100,200);
    wall.definition.segment = (WallPlanSegment){{-100,-200},{200,200}};
    expect(snap(&storey, (Vec2){58,-6}, settings), SNAP_WALL_CENTRELINE, 50,0);
    wall.definition.segment = (WallPlanSegment){{200,200},{-100,-200}};
    expect(snap(&storey, (Vec2){58,-6}, settings), SNAP_WALL_CENTRELINE, 50,0);
    wall.definition.segment = (WallPlanSegment){{-100,-200},{-100,200}};
    expect(snap(&storey, (Vec2){-95,50}, settings), SNAP_WALL_CENTRELINE, -100,50);
    settings.wall_centreline_enabled = 0;
    expect(snap(&storey, (Vec2){-95,50}, settings), SNAP_GRID, -100,100);
    settings.grid_enabled = 0;
    expect(snap(&storey, (Vec2){-95,50}, settings), SNAP_NONE, -95,50);
    SnapCandidate candidates[3];
    assert(!plan_collect_snap_candidates(&storey, (Vec2){NAN,0}, &settings, candidates));
    assert(!plan_collect_snap_candidates(&storey, (Vec2){0,INFINITY}, &settings, candidates));
    assert(!plan_collect_snap_candidates(NULL, (Vec2){0,0}, &settings, candidates));
    assert(!plan_collect_snap_candidates(&storey, (Vec2){0,0}, NULL, candidates));
    assert(!plan_collect_snap_candidates(&storey, (Vec2){0,0}, &settings, NULL));
}

static void test_deterministic_ties_and_large_collections(void)
{
    Wall walls[] = {
        {.id = 1, .definition.segment = {{-10,-100},{-10,100}}},
        {.id = 2, .definition.segment = {{10,-100},{10,100}}}
    };
    Storey storey = geometry(walls, 2);
    SnapSettings settings = defaults();
    expect(snap(&storey, (Vec2){0,0}, settings), SNAP_WALL_CENTRELINE, -10,0);
    expect(snap(&storey, (Vec2){0,-105}, settings), SNAP_ENDPOINT, -10,-100);
    Wall temp = walls[0]; walls[0] = walls[1]; walls[1] = temp;
    expect(snap(&storey, (Vec2){0,0}, settings), SNAP_WALL_CENTRELINE, -10,0);
    expect(snap(&storey, (Vec2){0,-105}, settings), SNAP_ENDPOINT, -10,-100);

    enum { COUNT = 600 };
    Wall *many = calloc(COUNT, sizeof *many); assert(many);
    for (int i = 0; i < COUNT; i++) {
        many[i] = (Wall){.id = (DomainId)i + 1,
            .definition.segment = {{i*1000,0},{i*1000,100}}};
    }
    storey = geometry(many, COUNT);
    expect(snap(&storey, (Vec2){599001,102}, settings), SNAP_ENDPOINT, 599000,100);
    expect(snap(&storey, (Vec2){599001,52}, settings), SNAP_WALL_CENTRELINE, 599000,52);
    free(many);
}

static void test_junctions_and_source_filter(void)
{
    Wall walls[] = {
        {.id = 1, .definition.segment = {{0,0},{101,101}}},
        {.id = 2, .definition.segment = {{0,101},{101,0}}}
    };
    Storey storey = geometry(walls, 2);
    /* An overlapping invisible separator would make topology fail if included. */
    RoomSeparator separator = {.id = 3, .segment = {{0,0},{101,101}}};
    storey.structure.room_separators = &separator;
    storey.structure.room_separator_count = storey.structure.room_separator_capacity = 1;
    SnapSettings settings = defaults();
    settings.wall_centreline_enabled = 0;
    settings.endpoint_enabled = 0;
#ifdef SITEHELPER_TEST_TOPOLOGY
    PlanTopologySource sources[] = {
        {PLAN_TOPOLOGY_SOURCE_WALL, 1, {{0,0},{101,101}}},
        {PLAN_TOPOLOGY_SOURCE_WALL, 2, {{0,101},{101,0}}}
    };
    PlanTopology topology = {0}; WallJunctionSet junctions = {0};
    assert(plan_topology_build(sources, 2, &topology).code == PLAN_TOPOLOGY_SUCCESS);
    assert(wall_junctions_build(&topology, &junctions) == WALL_JUNCTION_SUCCESS);
    assert(junctions.junction_count == 1 && junctions.junctions[0].kind == WALL_JUNCTION_CROSS);
    PlanTopologyVertex exact = junctions.junctions[0].position;
    assert(exact.x.numerator.lo == 101 && exact.x.denominator.lo == 2);
    assert(exact.y.numerator.lo == 101 && exact.y.denominator.lo == 2);
    expect(snap(&storey, (Vec2){49,52}, settings), SNAP_INTERSECTION, 50.5,50.5);
    wall_junctions_destroy(&junctions); plan_topology_destroy(&topology);
    settings = defaults();
    expect(snap(&storey, (Vec2){50.5,50.5}, settings), SNAP_INTERSECTION, 50.5,50.5);
    /* Nearest distance still wins: proximity to a line does not become a
     * farther intersection merely because intersections have tie priority. */
    expect(snap(&storey, (Vec2){40,40}, settings), SNAP_WALL_CENTRELINE, 40,40);
    walls[1].definition.segment = (WallPlanSegment){{101,101},{200,101}};
    expect(snap(&storey, (Vec2){101,101}, settings), SNAP_INTERSECTION, 101,101);
    walls[1].definition.segment = (WallPlanSegment){{0,101},{101,0}};
#else
    expect(snap(&storey, (Vec2){49,52}, settings), SNAP_GRID, 0,100);
#endif
    settings = defaults();
    /* Existing topology explicitly rejects collinear overlaps. Only the
     * intersection producer fails; endpoint/centreline/grid remain usable. */
    walls[1].definition.segment = walls[0].definition.segment;
    expect(snap(&storey, (Vec2){0,0}, settings), SNAP_ENDPOINT, 0,0);
    expect(snap(&storey, (Vec2){50,50}, settings), SNAP_WALL_CENTRELINE, 50,50);
    expect(snap(&storey, (Vec2){900,900}, settings), SNAP_GRID, 900,900);
    /* A Wall/RoomSeparator crossing alone never creates a junction. */
    storey.structure.wall_count = 1;
    separator.segment = (PlanSegment){{0,101},{101,0}};
    settings.endpoint_enabled = settings.wall_centreline_enabled = 0;
    expect(snap(&storey, (Vec2){50.5,50.5}, settings), SNAP_GRID, 100,100);
}

#ifdef SITEHELPER_TEST_TOPOLOGY
static void test_more_than_256_junctions(void)
{
    Wall walls[36] = {0};
    for (int i = 0; i < 18; i++) {
        walls[i] = (Wall){.id = (DomainId)i + 1,
            .definition.segment = {{0,i*100},{1900,i*100}}};
        walls[18+i] = (Wall){.id = (DomainId)i + 19,
            .definition.segment = {{(i+1)*100,-100},{(i+1)*100,1900}}};
    }
    Storey storey = geometry(walls, 36);
    SnapSettings settings = defaults();
    settings.endpoint_enabled = settings.wall_centreline_enabled = 0;
    /* 18 x 18 = 324 junctions; the nearest is last in exact vertex order. */
    expect(snap(&storey, (Vec2){1802,1701}, settings), SNAP_INTERSECTION, 1800,1700);
    /* Equidistant junctions use the same lexicographic tie rule. */
    expect(snap(&storey, (Vec2){1750,1700}, settings), SNAP_INTERSECTION, 1700,1700);
}
#endif

#if defined(SITEHELPER_TEST_TOPOLOGY) && defined(SITEHELPER_TEST_WRAP_ALLOC)
static void test_all_intersection_allocations_fall_back(void)
{
    Wall walls[] = {
        {.id = 1, .definition.segment = {{0,0},{1000,1000}}},
        {.id = 2, .definition.segment = {{0,1000},{1000,0}}}
    };
    Storey storey = geometry(walls, 2);
    SnapSettings settings = defaults();
    allocation_count = 0;
    expect(snap(&storey, (Vec2){500,500}, settings), SNAP_INTERSECTION, 500,500);
    long count = allocation_count; assert(count > 3);
    for (long i = 0; i < count; i++) {
        fail_after = i; allocation_count = 0;
        expect(snap(&storey, (Vec2){500,500}, settings), SNAP_WALL_CENTRELINE, 500,500);
        allocation_count = 0;
        expect(snap(&storey, (Vec2){0,0}, settings), SNAP_ENDPOINT, 0,0);
        allocation_count = 0;
        expect(snap(&storey, (Vec2){9000,9000}, settings), SNAP_GRID, 9000,9000);
    }
    fail_after = -1;
    expect(snap(&storey, (Vec2){500,500}, settings), SNAP_INTERSECTION, 500,500);
}
#endif

int main(void)
{
    test_endpoints_and_finite_projection();
    test_deterministic_ties_and_large_collections();
    test_junctions_and_source_filter();
#ifdef SITEHELPER_TEST_TOPOLOGY
    test_more_than_256_junctions();
#endif
#if defined(SITEHELPER_TEST_TOPOLOGY) && defined(SITEHELPER_TEST_WRAP_ALLOC)
    test_all_intersection_allocations_fall_back();
#endif
    puts("Plan candidate tests passed");
}
