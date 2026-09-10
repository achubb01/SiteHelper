#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wall_junctions.h"
#include "topology_numeric_internal.h"
#include "command_history.h"
#include "sitehelper_persistence.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t allocations_before_failure = SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int fail_allocation(void)
{
    if (allocations_before_failure != SIZE_MAX && allocations_before_failure-- == 0) {
        allocation_failed = 1; return 1;
    }
    return 0;
}
void *__wrap_malloc(size_t size) { return fail_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return fail_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *pointer, size_t size) { return fail_allocation() ? NULL : __real_realloc(pointer, size); }
#endif

#define SOURCE(id, ax, ay, bx, by) \
    (PlanTopologySource){PLAN_TOPOLOGY_SOURCE_WALL, id, {{ax, ay}, {bx, by}}}

static void rational_equal(PlanTopologyRational a, PlanTopologyRational b)
{
    assert(a.negative == b.negative);
    assert(a.numerator.lo == b.numerator.lo && a.numerator.hi == b.numerator.hi);
    assert(a.denominator.lo == b.denominator.lo && a.denominator.hi == b.denominator.hi);
}

static void fraction(PlanTopologyRational actual, int numerator, unsigned denominator)
{
    rational_equal(actual, topology_rational(numerator, denominator));
}

static void consistent(const WallJunctionSet *set)
{
    size_t offset = 0;
    for (size_t i = 0; i < set->junction_count; i++) {
        const WallJunction *j = &set->junctions[i];
        assert(j->first_participant == offset && j->participant_count >= 2);
        assert(j->participant_count <= set->participant_count - offset);
        if (i) { assert(topology_vertex_compare(set->junctions[i - 1].position, j->position) < 0); }
        for (size_t k = 0; k < j->participant_count; k++) {
            const WallJunctionParticipant *p = &set->participants[offset + k];
            assert(p->wall_id != DOMAIN_ID_INVALID);
            if (k) { assert(set->participants[offset + k - 1].wall_id < p->wall_id); }
        }
        offset += j->participant_count;
    }
    assert(offset == set->participant_count);
    assert((set->junction_count == 0) == (set->junctions == NULL));
    assert((set->participant_count == 0) == (set->participants == NULL));
}

/* Every fixture uses its result after destroying its input snapshot. */
static WallJunctionSet build(const PlanTopologySource *sources, size_t count)
{
    PlanTopology topology = {0};
    WallJunctionSet set = {0};
    assert(plan_topology_build(sources, count, &topology).code == PLAN_TOPOLOGY_SUCCESS);
    assert(wall_junctions_build(&topology, &set) == WALL_JUNCTION_SUCCESS);
    plan_topology_destroy(&topology);
    consistent(&set);
    return set;
}

static void equivalent(const WallJunctionSet *a, const WallJunctionSet *b)
{
    assert(a->junction_count == b->junction_count && a->participant_count == b->participant_count);
    for (size_t i = 0; i < a->junction_count; i++) {
        WallJunction x = a->junctions[i], y = b->junctions[i];
        assert(x.kind == y.kind && x.first_participant == y.first_participant);
        assert(x.participant_count == y.participant_count);
        rational_equal(x.position.x, y.position.x); rational_equal(x.position.y, y.position.y);
    }
    for (size_t i = 0; i < a->participant_count; i++) {
        WallJunctionParticipant x = a->participants[i], y = b->participants[i];
        assert(x.wall_id == y.wall_id && x.position == y.position);
        rational_equal(x.source_t, y.source_t);
    }
}

static void check_participant(WallJunctionParticipant p, DomainId id, int n, unsigned d)
{
    assert(p.wall_id == id);
    fraction(p.source_t, n, d);
    assert(p.position == (n == 0 ? WALL_JUNCTION_PARTICIPANT_START :
        (unsigned)n == d ? WALL_JUNCTION_PARTICIPANT_END : WALL_JUNCTION_PARTICIPANT_INTERIOR));
}

static PlanPosition symmetry(PlanPosition p, unsigned operation)
{
    if (operation & 4) { p.y = -p.y; }
    for (unsigned i = 0; i < (operation & 3); i++) { p = (PlanPosition){-p.y, p.x}; }
    return p;
}

static void test_pair_classification_symmetries(void)
{
    const struct {
        PlanTopologySource sources[2];
        WallJunctionKind kind;
        PlanPosition numerator;
        unsigned denominator;
        int t_n[2];
        unsigned t_d[2];
    } cases[] = {
        {{SOURCE(90, -4, 0, 0, 0), SOURCE(7, 0, 0, 0, 4)}, WALL_JUNCTION_CORNER, {0, 0}, 1, {1, 0}, {1, 1}},
        {{SOURCE(90, -4, -2, 0, 0), SOURCE(7, 0, 0, 3, -1)}, WALL_JUNCTION_CORNER, {0, 0}, 1, {1, 0}, {1, 1}},
        {{SOURCE(90, -4, -2, 0, 0), SOURCE(7, 0, 0, 4, 2)}, WALL_JUNCTION_CONTINUOUS, {0, 0}, 1, {1, 0}, {1, 1}},
        /* The terminating wall is END, continuing wall is INTERIOR. */
        {{SOURCE(90, -2, 0, 6, 0), SOURCE(7, 0, 4, 0, 0)}, WALL_JUNCTION_T, {0, 0}, 1, {1, 1}, {4, 1}},
        {{SOURCE(90, -2, 0, 6, 0), SOURCE(7, 0, -2, 0, 4)}, WALL_JUNCTION_CROSS, {0, 0}, 1, {1, 1}, {4, 3}},
        {{SOURCE(90, 0, 0, 1, 2), SOURCE(7, 0, 1, 1, 0)}, WALL_JUNCTION_CROSS, {1, 2}, 3, {1, 1}, {3, 3}}
    };
    for (size_t c = 0; c < sizeof cases / sizeof *cases; c++) {
        for (unsigned op = 0; op < 8; op++) {
            for (unsigned reversals = 0; reversals < 4; reversals++) {
                for (unsigned permutation = 0; permutation < 2; permutation++) {
                    PlanTopologySource sources[2];
                    int n[2];
                    for (unsigned i = 0; i < 2; i++) {
                        PlanTopologySource s = cases[c].sources[i];
                        s.segment.start = symmetry(s.segment.start, op);
                        s.segment.end = symmetry(s.segment.end, op);
                        n[i] = cases[c].t_n[i];
                        if (reversals & (1u << i)) {
                            PlanPosition p = s.segment.start; s.segment.start = s.segment.end; s.segment.end = p;
                            n[i] = (int)cases[c].t_d[i] - n[i];
                        }
                        sources[i ^ permutation] = s;
                    }
                    WallJunctionSet set = build(sources, 2);
                    assert(set.junction_count == 1 && set.participant_count == 2);
                    assert(set.junctions[0].kind == cases[c].kind);
                    PlanPosition p = symmetry(cases[c].numerator, op);
                    fraction(set.junctions[0].position.x, p.x, cases[c].denominator);
                    fraction(set.junctions[0].position.y, p.y, cases[c].denominator);
                    check_participant(set.participants[0], 7, n[1], cases[c].t_d[1]);
                    check_participant(set.participants[1], 90, n[0], cases[c].t_d[0]);
                    wall_junctions_destroy(&set);
                }
            }
        }
    }
}

static void test_empty_lone_and_separators(void)
{
    WallJunctionSet set = build(NULL, 0);
    assert(set.junction_count == 0);
    wall_junctions_destroy(&set);
    PlanTopologySource sources[] = {SOURCE(1, -4, 0, 4, 0), SOURCE(2, 0, -4, 0, 4),
        SOURCE(3, -4, -4, 4, 4)};
    set = build(sources, 1);
    assert(set.junction_count == 0); wall_junctions_destroy(&set);
    sources[1].kind = sources[2].kind = PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR;
    set = build(sources, 3); /* Wall has two incident edges, but only one identity. */
    assert(set.junction_count == 0); wall_junctions_destroy(&set);
    sources[0].kind = PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR;
    set = build(sources, 3);
    assert(set.junction_count == 0); wall_junctions_destroy(&set);
    sources[0].kind = sources[1].kind = PLAN_TOPOLOGY_SOURCE_WALL;
    set = build(sources, 3); /* Separator cannot promote a CROSS to MULTIWAY. */
    assert(set.junction_count == 1 && set.junctions[0].kind == WALL_JUNCTION_CROSS);
    assert(set.participant_count == 2); wall_junctions_destroy(&set);
    sources[1] = SOURCE(2, 4, 0, 4, 4);
    sources[1].kind = PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR;
    set = build(sources, 2); /* Endpoint join also ignores virtual sources. */
    assert(set.junction_count == 0); wall_junctions_destroy(&set);
    wall_junctions_destroy(&set); wall_junctions_destroy(NULL);
    assert(set.junctions == NULL && set.participants == NULL && set.participant_count == 0);
}

static void test_multiway_and_multiple_ordering(void)
{
    PlanTopologySource concurrent[] = {SOURCE(99, 0, 0, 1, 2), SOURCE(7, 0, 1, 1, 0),
        SOURCE(42, 0, 2, 1, -2)};
    WallJunctionSet set = build(concurrent, 3);
    assert(set.junction_count == 1 && set.junctions[0].kind == WALL_JUNCTION_MULTIWAY);
    assert(set.participant_count == 3);
    fraction(set.junctions[0].position.x, 1, 3); fraction(set.junctions[0].position.y, 2, 3);
    check_participant(set.participants[0], 7, 1, 3);
    check_participant(set.participants[1], 42, 1, 3);
    check_participant(set.participants[2], 99, 1, 3);
    wall_junctions_destroy(&set);
    PlanTopologySource split_tee[] = {SOURCE(3, -4, 0, 0, 0), SOURCE(1, 0, 0, 4, 0),
        SOURCE(2, 0, 0, 0, 4)};
    set = build(split_tee, 3);
    assert(set.junction_count == 1 && set.junctions[0].kind == WALL_JUNCTION_MULTIWAY);
    check_participant(set.participants[0], 1, 0, 1);
    check_participant(set.participants[1], 2, 0, 1);
    check_participant(set.participants[2], 3, 1, 1);
    wall_junctions_destroy(&set);

    PlanTopologySource sources[] = {SOURCE(80, 8, 0, 0, 0), SOURCE(90, 2, -3, 2, 3),
        SOURCE(2, 4, 0, 4, 3), SOURCE(60, 6, -3, 6, 3)};
    WallJunctionSet expected = build(sources, 4);
    assert(expected.junction_count == 3 && expected.participant_count == 6);
    const DomainId ids[] = {80, 90, 2, 80, 60, 80};
    const int n[] = {3, 1, 0, 1, 1, 1};
    const unsigned d[] = {4, 2, 1, 2, 2, 4};
    for (size_t i = 0; i < 3; i++) {
        fraction(expected.junctions[i].position.x, (int)(2 * i + 2), 1);
        assert(expected.junctions[i].kind == (i == 1 ? WALL_JUNCTION_T : WALL_JUNCTION_CROSS));
    }
    for (size_t i = 0; i < 6; i++) { check_participant(expected.participants[i], ids[i], n[i], d[i]); }
    /* All 24 source permutations, with fixed identities and endpoint orders. */
    for (size_t a = 0; a < 4; a++) for (size_t b = 0; b < 4; b++) {
        if (a == b) { continue; }
        for (size_t c = 0; c < 4; c++) {
            if (c == a || c == b) { continue; }
            size_t last = 6 - a - b - c;
            PlanTopologySource copy[] = {sources[a], sources[b], sources[c], sources[last]};
            set = build(copy, 4); equivalent(&expected, &set); wall_junctions_destroy(&set);
        }
    }
    wall_junctions_destroy(&expected);
}

static void test_full_range_exact_copies(void)
{
    PlanTopologySource sources[] = {SOURCE(1, INT_MIN, INT_MIN, INT_MAX, INT_MAX - 1),
        SOURCE(2, INT_MIN, INT_MAX, INT_MAX - 1, INT_MIN)};
    PlanTopology topology = {0};
    WallJunctionSet set = {0};
    assert(plan_topology_build(sources, 2, &topology).code == PLAN_TOPOLOGY_SUCCESS);
    assert(wall_junctions_build(&topology, &set) == WALL_JUNCTION_SUCCESS);
    assert(set.junction_count == 1 && set.participant_count == 2);
    assert(set.junctions[0].kind == WALL_JUNCTION_CROSS);
    PlanTopologyVertex p = set.junctions[0].position;
    assert(p.x.denominator.hi || p.y.denominator.hi);
    size_t incidences = 0;
    for (size_t i = 0; i < topology.edge_count; i++) {
        PlanTopologyEdge e = topology.edges[i];
        bool start = topology_vertex_compare(topology.vertices[e.start_vertex], p) == 0;
        bool end = topology_vertex_compare(topology.vertices[e.end_vertex], p) == 0;
        assert(start != end);
        PlanTopologyVertex v = topology.vertices[start ? e.start_vertex : e.end_vertex];
        rational_equal(v.x, p.x); rational_equal(v.y, p.y);
        rational_equal(set.participants[e.source_id - 1].source_t, start ? e.source_t_start : e.source_t_end);
        incidences++;
    }
    assert(incidences == 4); /* Four edge incidences become two participants. */
    plan_topology_destroy(&topology); wall_junctions_destroy(&set);
    sources[0] = SOURCE(1, INT_MIN, INT_MIN, INT_MAX, INT_MAX);
    sources[1] = SOURCE(2, INT_MAX, INT_MAX, INT_MIN, INT_MAX);
    set = build(sources, 2); /* Endpoint direction determinant exceeds signed 64 bits. */
    assert(set.junction_count == 1 && set.junctions[0].kind == WALL_JUNCTION_CORNER);
    wall_junctions_destroy(&set);
}

static void rebuild_project(const SiteHelperProject *project, PlanTopology *topology,
    WallJunctionSet *set, size_t count)
{
    assert(sitehelper_project_validate(project).code == SITEHELPER_PROJECT_VALID);
    assert(plan_topology_build_from_storey(&project->storeys[0], topology).code == PLAN_TOPOLOGY_SUCCESS);
    assert(wall_junctions_build(topology, set) == WALL_JUNCTION_SUCCESS);
    assert(set->junction_count == count);
    consistent(set);
}

static void test_project_history_persistence_and_staleness(void)
{
    SiteHelperProject project, before, loaded;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0)); sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));
    SiteHelperCommandHistory history;
    sitehelper_command_history_init(&history);
    PlanTopology topology = {0};
    WallJunctionSet set = {0};
    DomainId through = sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){{0, 0}, {6000, 0}});
    assert(through);
    rebuild_project(&project, &topology, &set, 0);
    WallCommand wall;
    SiteHelperCommand command;
    SiteHelperCommandResult result;
    assert(wall_command_create(1, (WallPlanSegment){{2000, 3000}, {2000, 0}}, &wall));
    assert(sitehelper_command_from_wall(&wall, &command));
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    DomainId terminating = result.data.add_wall.wall_id;
    rebuild_project(&project, &topology, &set, 1);
    assert(set.junctions[0].kind == WALL_JUNCTION_T);
    check_participant(set.participants[0], through, 1, 3);
    check_participant(set.participants[1], terminating, 1, 1);
    assert(sitehelper_command_history_undo(&history, &project));
    /* The old set and even a new query over the old topology still describe T. */
    assert(set.junction_count == 1);
    assert(wall_junctions_build(&topology, &set) == WALL_JUNCTION_SUCCESS);
    assert(set.junction_count == 1);
    rebuild_project(&project, &topology, &set, 0);
    assert(sitehelper_command_history_redo(&history, &project));
    rebuild_project(&project, &topology, &set, 1);
    MoveWallEndpointCommand move;
    assert(move_wall_endpoint_command_create(terminating, WALL_ENDPOINT_END, (PlanPosition){2000, 1000}, &move));
    assert(sitehelper_command_from_move_wall_endpoint(&move, &command));
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    rebuild_project(&project, &topology, &set, 0);
    for (int cycle = 0; cycle < 3; cycle++) {
        assert(sitehelper_command_history_undo(&history, &project));
        rebuild_project(&project, &topology, &set, 1);
        assert(sitehelper_command_history_redo(&history, &project));
        rebuild_project(&project, &topology, &set, 0);
    }
    assert(sitehelper_command_history_undo(&history, &project));
    DeleteWallCommand deletion;
    assert(delete_wall_command_create(terminating, &deletion));
    assert(sitehelper_command_from_delete_wall(&deletion, &command));
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    rebuild_project(&project, &topology, &set, 0);
    for (int cycle = 0; cycle < 3; cycle++) {
        assert(sitehelper_command_history_undo(&history, &project));
        rebuild_project(&project, &topology, &set, 1);
        assert(sitehelper_command_history_redo(&history, &project));
        rebuild_project(&project, &topology, &set, 0);
    }
    assert(sitehelper_command_history_undo(&history, &project));
    rebuild_project(&project, &topology, &set, 1);
    WallJunctionSet expected = {0};
    assert(wall_junctions_build(&topology, &expected) == WALL_JUNCTION_SUCCESS);
    Wall swap = project.storeys[0].structure.walls[0];
    project.storeys[0].structure.walls[0] = project.storeys[0].structure.walls[1]; project.storeys[0].structure.walls[1] = swap;
    test_clone_project_authoritative(&project, &before);
    rebuild_project(&project, &topology, &set, 1);
    equivalent(&expected, &set);
    test_assert_project_authoritative_equal(&before, &project);
    const char *path = "wall_junctions_roundtrip.tmp";
    assert(sitehelper_project_save_file(&project, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(remove(path) == 0);
    test_assert_project_authoritative_equal(&project, &loaded);
    rebuild_project(&loaded, &topology, &set, 1);
    equivalent(&expected, &set);
    /* Unsupported overlap remains valid authoritative geometry. A failed
     * topology rebuild preserves its old snapshot; no stale fallback occurs. */
    assert(sitehelper_project_add_wall(&loaded, loaded.storeys[0].id, (WallPlanSegment){{1000, 0}, {4000, 0}}));
    assert(sitehelper_project_validate(&loaded).code == SITEHELPER_PROJECT_VALID);
    assert(plan_topology_build_from_storey(&loaded.storeys[0], &topology).code == PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP);
    equivalent(&expected, &set);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project); sitehelper_project_destroy(&before); sitehelper_project_destroy(&loaded);
    plan_topology_destroy(&topology);
    equivalent(&expected, &set); /* No borrowed project or topology lifetime. */
    wall_junctions_destroy(&expected); wall_junctions_destroy(&set);
}

static void test_invalid_arguments_and_replacement(void)
{
    PlanTopologySource sources[] = {SOURCE(1, -4, 0, 4, 0), SOURCE(2, 0, -4, 0, 4)};
    WallJunctionSet set = build(sources, 2), expected = build(sources, 2);
    WallJunctionSet original = set; /* Non-owning snapshot of pointers. */
    PlanTopology topology = {0};
    assert(wall_junctions_build(NULL, &set) == WALL_JUNCTION_INVALID_ARGUMENT);
    assert(wall_junctions_build(&topology, &set) == WALL_JUNCTION_INVALID_ARGUMENT);
    assert(wall_junctions_build(&topology, NULL) == WALL_JUNCTION_INVALID_ARGUMENT);
    equivalent(&expected, &set);
    assert(set.junctions == original.junctions && set.participants == original.participants);
    assert(plan_topology_build(NULL, 0, &topology).code == PLAN_TOPOLOGY_SUCCESS);
    assert(wall_junctions_build(&topology, &set) == WALL_JUNCTION_SUCCESS);
    consistent(&set); assert(set.junction_count == 0);
    plan_topology_destroy(&topology);
    assert(wall_junctions_build(&topology, &set) == WALL_JUNCTION_INVALID_ARGUMENT);
    wall_junctions_destroy(&set); wall_junctions_destroy(&set); wall_junctions_destroy(&expected);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failure_transactions(void)
{
    PlanTopologySource sources[] = {SOURCE(1, -4, 0, 4, 0), SOURCE(2, 0, -4, 0, 4),
        SOURCE(3, 2, -4, 2, 4)};
    PlanTopology topology = {0};
    assert(plan_topology_build(sources, 3, &topology).code == PLAN_TOPOLOGY_SUCCESS);
    PlanTopology original_topology = topology;
    PlanTopologyEdge *edges = malloc(topology.edge_count * sizeof *edges);
    PlanTopologyVertex *vertices = malloc(topology.vertex_count * sizeof *vertices);
    assert(edges && vertices);
    memcpy(edges, topology.edges, topology.edge_count * sizeof *edges);
    memcpy(vertices, topology.vertices, topology.vertex_count * sizeof *vertices);
    size_t failures = 0;
    for (int populated = 0; populated < 2; populated++) {
        for (size_t fail_at = 0; fail_at < 32; fail_at++) {
            WallJunctionSet set = populated ? build(sources, 2) : (WallJunctionSet){0};
            WallJunctionSet expected = populated ? build(sources, 2) : (WallJunctionSet){0};
            WallJunctionSet original = set;
            allocation_failed = 0;
            allocations_before_failure = fail_at;
            WallJunctionCode code = wall_junctions_build(&topology, &set);
            allocations_before_failure = SIZE_MAX;
            if (allocation_failed) {
                failures++;
                assert(code == WALL_JUNCTION_ALLOCATION_FAILED);
                equivalent(&expected, &set);
                assert(set.junctions == original.junctions && set.participants == original.participants);
                assert(wall_junctions_build(&topology, &set) == WALL_JUNCTION_SUCCESS);
            } else { assert(code == WALL_JUNCTION_SUCCESS); }
            assert(set.junction_count == 2 && set.participant_count == 4);
            assert(memcmp(&topology, &original_topology, sizeof topology) == 0);
            assert(memcmp(edges, topology.edges, topology.edge_count * sizeof *edges) == 0);
            assert(memcmp(vertices, topology.vertices, topology.vertex_count * sizeof *vertices) == 0);
            wall_junctions_destroy(&set); wall_junctions_destroy(&expected);
            if (!allocation_failed) { break; }
            assert(fail_at < 31);
        }
    }
    assert(failures == 6); /* Workspace + two owned arrays, zero and populated outputs. */
    printf("junction allocation failures checked: %zu\n", failures);
    free(edges); free(vertices); plan_topology_destroy(&topology);
}
#endif

int main(void)
{
    test_pair_classification_symmetries();
    test_empty_lone_and_separators();
    test_multiway_and_multiple_ordering();
    test_full_range_exact_copies();
    test_project_history_persistence_and_staleness();
    test_invalid_arguments_and_replacement();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failure_transactions();
#endif
    puts("wall junction tests passed");
    return 0;
}
