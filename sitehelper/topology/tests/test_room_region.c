#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "room_region.h"
#include "test_support.h"
#include "sitehelper_editor.h"
#include "sitehelper_persistence.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static int reject_allocations;
static size_t rejected_allocations;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
void *__wrap_malloc(size_t size)
{
    if (reject_allocations) { rejected_allocations++; return NULL; }
    return __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size)
{
    if (reject_allocations) { rejected_allocations++; return NULL; }
    return __real_calloc(count, size);
}
void *__wrap_realloc(void *pointer, size_t size)
{
    if (reject_allocations) { rejected_allocations++; return NULL; }
    return __real_realloc(pointer, size);
}
#endif

#define SOURCE(id, ax, ay, bx, by) \
    (PlanTopologySource){PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR, id, {{ax, ay}, {bx, by}}}

typedef struct { PlanTopology original, copy; } TopologySnapshot;

static TopologySnapshot snapshot(const PlanTopology *t)
{
    TopologySnapshot s = {.original = *t, .copy = *t};
#define COPY(name, count) \
    s.copy.name = t->count ? malloc(t->count * sizeof *t->name) : NULL; \
    assert(!t->count || s.copy.name); \
    if (t->count) { memcpy(s.copy.name, t->name, t->count * sizeof *t->name); }
    COPY(vertices, vertex_count);
    COPY(edges, edge_count);
    COPY(faces, face_count);
    COPY(boundaries, boundary_count);
    COPY(steps, step_count);
#undef COPY
    return s;
}

static void unchanged(const TopologySnapshot *s, const PlanTopology *t)
{
#define CHECK(name, count) \
    assert(t->name == s->original.name && t->count == s->original.count); \
    assert(!t->count || memcmp(t->name, s->copy.name, t->count * sizeof *t->name) == 0)
    CHECK(vertices, vertex_count);
    CHECK(edges, edge_count);
    CHECK(faces, face_count);
    CHECK(boundaries, boundary_count);
    CHECK(steps, step_count);
#undef CHECK
}

static void rectangle_project(SiteHelperProject *project)
{
    sitehelper_project_init(project);
    assert(sitehelper_project_add_storey(project, 0));
    assert(sitehelper_project_add_wall(project, project->storeys[0].id, (WallPlanSegment){{0, 0}, {6000, 0}}));
    assert(sitehelper_project_add_wall(project, project->storeys[0].id, (WallPlanSegment){{6000, 0}, {6000, 4000}}));
    assert(sitehelper_project_add_room_separator(project, project->storeys[0].id, (PlanSegment){{6000, 4000}, {0, 4000}}));
    assert(sitehelper_project_add_room_separator(project, project->storeys[0].id, (PlanSegment){{0, 4000}, {0, 0}}));
}

static PlanTopologyPointResult classify(const PlanTopology *t, PlanPosition p, PlanTopologyPointState expected)
{
    PlanTopologyPointResult r = plan_topology_find_face_at_plan_position(t, p);
    assert(r.code == PLAN_TOPOLOGY_SUCCESS && r.state == expected);
    if (expected == PLAN_TOPOLOGY_POINT_BOUNDED) { assert(r.face_index > 0 && r.face_index < t->face_count); }
    if (expected == PLAN_TOPOLOGY_POINT_UNBOUNDED) { assert(r.face_index == 0); }
    if (expected == PLAN_TOPOLOGY_POINT_ON_BOUNDARY) { assert(r.face_index == SIZE_MAX); }
    return r;
}

static void test_room_states(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    RoomRegionResult r = room_region_resolve(&project, room, NULL);
    assert(r.code == ROOM_REGION_UNPLACED && r.face_index == SIZE_MAX && r.topology == NULL);
    assert(room_region_build_and_resolve(&project, room, NULL).code == ROOM_REGION_UNPLACED);
    assert(sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){{0, 0}, {6000, 0}}));
    assert(sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){{6000, 0}, {6000, 4000}}));
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{6000, 4000}, {0, 4000}}));
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 4000}, {0, 0}}));
    PlanTopology topology = {0};
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){1000, 1000}));
    r = room_region_build_and_resolve(&project, room, &topology);
    assert(r.code == ROOM_REGION_BOUNDED && r.face_index == 1 && r.topology == &topology);
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){-1, 1000}));
    r = room_region_resolve(&project, room, &topology);
    assert(r.code == ROOM_REGION_UNBOUNDED && r.face_index == 0);
    const PlanPosition boundary[] = {{1000, 0}, {0, 1000}, {0, 0}};
    for (size_t i = 0; i < sizeof boundary / sizeof *boundary; i++) {
        assert(sitehelper_project_set_room_location(&project, room, boundary[i]));
        r = room_region_resolve(&project, room, &topology);
        assert(r.code == ROOM_REGION_ON_BOUNDARY && r.face_index == SIZE_MAX);
    }
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    plan_topology_destroy(&topology);
    sitehelper_project_destroy(&project);
}

static void test_empty_open_and_invalid_queries(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    PlanTopology t = {0};
    assert(plan_topology_find_face_at_plan_position(&t, (PlanPosition){0, 0}).code == PLAN_TOPOLOGY_INVALID_ARGUMENT);
    assert(plan_topology_find_face_at_plan_position(NULL, (PlanPosition){0, 0}).state == PLAN_TOPOLOGY_POINT_UNCLASSIFIED);
    assert(room_region_resolve(NULL, room, &t).code == ROOM_REGION_INVALID_ARGUMENT);
    assert(room_region_resolve(&project, DOMAIN_ID_INVALID, &t).code == ROOM_REGION_ROOM_NOT_FOUND);
    assert(room_region_resolve(&project, room + 99, &t).code == ROOM_REGION_ROOM_NOT_FOUND);
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){1000, 1000}));
    RoomRegionResult r = room_region_resolve(&project, room, &t);
    assert(r.code == ROOM_REGION_TOPOLOGY_FAILED && r.topology_result.code == PLAN_TOPOLOGY_INVALID_ARGUMENT);
    assert(r.face_index == SIZE_MAX && r.topology == NULL);
    r = room_region_build_and_resolve(&project, room, &t);
    assert(r.code == ROOM_REGION_UNBOUNDED && r.face_index == 0);
    assert(sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){{0, 0}, {6000, 0}}));
    assert(room_region_build_and_resolve(&project, room, &t).code == ROOM_REGION_UNBOUNDED);
    classify(&t, (PlanPosition){3000, 0}, PLAN_TOPOLOGY_POINT_ON_BOUNDARY);
    classify(&t, (PlanPosition){7000, 0}, PLAN_TOPOLOGY_POINT_UNBOUNDED); /* Supporting line beyond segment. */
    BuildStructure saved = project.storeys[0].structure;
    project.storeys[0].structure.rooms = NULL;
    assert(room_region_resolve(&project, room, &t).code == ROOM_REGION_INVALID_ARGUMENT);
    project.storeys[0].structure = saved;
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    plan_topology_destroy(&t);
    sitehelper_project_destroy(&project);
}

static PlanPosition transform(PlanPosition p, unsigned operation)
{
    if (operation & 4) { p.y = -p.y; }
    for (unsigned i = 0; i < (operation & 3); i++) { p = (PlanPosition){-p.y, p.x}; }
    return p;
}

static void test_exact_rational_geometry_and_symmetries(void)
{
    /* The two interior lines meet at (30/11,30/11). Independent integer
     * line inequalities y-x and 6y+5x-30 identify the four bounded faces. */
    PlanTopologySource sources[] = {SOURCE(1, 0, 0, 6, 0), SOURCE(2, 6, 0, 6, 6),
        SOURCE(3, 6, 6, 0, 6), SOURCE(4, 0, 6, 0, 0),
        SOURCE(5, 0, 0, 6, 6), SOURCE(6, 0, 5, 6, 0)};
    const PlanPosition seeds[] = {{2, 1}, {1, 2}, {5, 2}, {2, 4}};
    for (unsigned operation = 0; operation < 8; operation++) {
        PlanTopologySource copy[6];
        for (size_t i = 0; i < 6; i++) {
            copy[i] = sources[5 - i]; /* Reverse collection order and source direction. */
            copy[i].segment.start = transform(sources[5 - i].segment.end, operation);
            copy[i].segment.end = transform(sources[5 - i].segment.start, operation);
        }
        PlanTopology t = {0};
        assert(plan_topology_build(copy, 6, &t).code == PLAN_TOPOLOGY_SUCCESS);
        assert(t.face_count == 5 && t.vertex_count == 6);
        size_t face[4];
        for (size_t i = 0; i < 4; i++) {
            face[i] = classify(&t, transform(seeds[i], operation), PLAN_TOPOLOGY_POINT_BOUNDED).face_index;
            for (size_t j = 0; j < i; j++) { assert(face[i] != face[j]); }
        }
        for (int x = -1; x <= 7; x++) {
            for (int y = -1; y <= 7; y++) {
                PlanPosition p = transform((PlanPosition){x, y}, operation);
                if (x < 0 || y < 0 || x > 6 || y > 6) { classify(&t, p, PLAN_TOPOLOGY_POINT_UNBOUNDED); }
                else if (x == 0 || y == 0 || x == 6 || y == 6 || x == y || 6 * y + 5 * x == 30) {
                    classify(&t, p, PLAN_TOPOLOGY_POINT_ON_BOUNDARY);
                }
                else {
                    size_t region = (y > x ? 1 : 0) + (6 * y + 5 * x > 30 ? 2 : 0);
                    assert(classify(&t, p, PLAN_TOPOLOGY_POINT_BOUNDED).face_index == face[region]);
                }
            }
        }
        plan_topology_destroy(&t);
    }
    /* A nonintegral vertex itself cannot equal any integer Room location.
     * This second intersection has fractional source parameters 1/2, but
     * representable coordinates (3,3), so a Room can lie exactly on it. */
    SiteHelperProject project;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 0}, {6, 6}}));
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 6}, {6, 0}}));
    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){3, 3}));
    PlanTopology t = {0};
    assert(room_region_build_and_resolve(&project, room, &t).code == ROOM_REGION_ON_BOUNDARY);
    assert(t.vertex_count == 5);
    plan_topology_destroy(&t);
    sitehelper_project_destroy(&project);
    PlanTopologySource extreme[] = {SOURCE(1, INT_MIN, INT_MIN, INT_MAX, INT_MIN),
        SOURCE(2, INT_MAX, INT_MIN, INT_MAX, INT_MAX), SOURCE(3, INT_MAX, INT_MAX, INT_MIN, INT_MAX),
        SOURCE(4, INT_MIN, INT_MAX, INT_MIN, INT_MIN), SOURCE(5, INT_MIN, INT_MIN, INT_MAX, INT_MAX - 1),
        SOURCE(6, INT_MIN, INT_MAX, INT_MAX - 1, INT_MIN)};
    assert(plan_topology_build(extreme, 6, &t).code == PLAN_TOPOLOGY_SUCCESS);
    assert(t.face_count == 5);
    classify(&t, (PlanPosition){0, 0}, PLAN_TOPOLOGY_POINT_BOUNDED);
    classify(&t, (PlanPosition){INT_MIN, 0}, PLAN_TOPOLOGY_POINT_ON_BOUNDARY);
    classify(&t, (PlanPosition){INT_MAX, INT_MAX}, PLAN_TOPOLOGY_POINT_ON_BOUNDARY);
    plan_topology_destroy(&t);
}

static void check_rectangle_boundary(RoomRegionResult result)
{
    assert(result.code == ROOM_REGION_BOUNDED);
    const PlanTopology *t = result.topology;
    PlanTopologyFace face = t->faces[result.face_index];
    assert(face.boundary_count == 1);
    PlanTopologyBoundary boundary = t->boundaries[face.first_boundary];
    assert(boundary.step_count == 4);
    /* The least canonical edge is the left vertical edge. Its reverse is
     * the bounded walk's least directed step, so the established 7D order
     * starts at the top-left corner and proceeds CCW. */
    const PlanPosition expected[] = {{0, 4000}, {0, 0}, {6000, 0}, {6000, 4000}};
    for (size_t i = 0; i < 4; i++) {
        PlanTopologyBoundaryStep step = t->steps[boundary.first_step + i];
        PlanTopologyEdge edge = t->edges[step.edge];
        PlanTopologyVertex vertex = t->vertices[step.reversed ? edge.end_vertex : edge.start_vertex];
        assert(!vertex.x.negative && vertex.x.numerator.hi == 0 && vertex.x.numerator.lo == (uint64_t)expected[i].x);
        assert(!vertex.y.negative && vertex.y.numerator.hi == 0 && vertex.y.numerator.lo == (uint64_t)expected[i].y);
        assert(vertex.x.denominator.lo == 1 && vertex.x.denominator.hi == 0);
        assert(vertex.y.denominator.lo == 1 && vertex.y.denominator.hi == 0);
        assert((step.reversed ? edge.reverse_face : edge.forward_face) == result.face_index);
    }
}

static void test_concave_and_disconnected_faces(void)
{
    PlanTopologySource sources[] = {SOURCE(1, 0, 0, 6, 0), SOURCE(2, 6, 0, 6, 2),
        SOURCE(3, 6, 2, 2, 2), SOURCE(4, 2, 2, 2, 6), SOURCE(5, 2, 6, 0, 6), SOURCE(6, 0, 6, 0, 0),
        SOURCE(7, 10, 0, 12, 0), SOURCE(8, 12, 0, 12, 2), SOURCE(9, 12, 2, 10, 2), SOURCE(10, 10, 2, 10, 0)};
    PlanTopology t = {0};
    assert(plan_topology_build(sources, 10, &t).code == PLAN_TOPOLOGY_SUCCESS);
    assert(t.face_count == 3 && t.faces[0].boundary_count == 2);
    size_t l_face = classify(&t, (PlanPosition){1, 4}, PLAN_TOPOLOGY_POINT_BOUNDED).face_index;
    assert(classify(&t, (PlanPosition){4, 1}, PLAN_TOPOLOGY_POINT_BOUNDED).face_index == l_face);
    assert(classify(&t, (PlanPosition){11, 1}, PLAN_TOPOLOGY_POINT_BOUNDED).face_index != l_face);
    classify(&t, (PlanPosition){4, 4}, PLAN_TOPOLOGY_POINT_UNBOUNDED); /* Inside L's bounding box only. */
    classify(&t, (PlanPosition){8, 1}, PLAN_TOPOLOGY_POINT_UNBOUNDED);
    classify(&t, (PlanPosition){2, 2}, PLAN_TOPOLOGY_POINT_ON_BOUNDARY);
    plan_topology_destroy(&t);
}

static void same_files(const char *a, const char *b)
{
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    assert(fa && fb);
    int ca, cb;
    do { ca = fgetc(fa); cb = fgetc(fb); assert(ca == cb); } while (ca != EOF);
    assert(!ferror(fa) && !ferror(fb));
    fclose(fa); fclose(fb);
    assert(remove(a) == 0 && remove(b) == 0);
}

static void test_multiple_rooms_read_only_and_lifetime(void)
{
    SiteHelperProject project, before;
    rectangle_project(&project);
    DomainId a = sitehelper_project_add_room(&project, project.storeys[0].id), b = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(sitehelper_project_set_room_location(&project, a, (PlanPosition){1000, 1000}));
    assert(sitehelper_project_set_room_location(&project, b, (PlanPosition){5000, 1000}));
    PlanTopology t = {0};
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_SUCCESS);
    TopologySnapshot saved = snapshot(&t);
    test_clone_project_authoritative(&project, &before);
    const char *file_a = "room_region_before.tmp", *file_b = "room_region_after.tmp";
    assert(sitehelper_project_save_file(&project, file_a) == SITEHELPER_PERSISTENCE_SUCCESS);
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    editor.current_wall_id = project.storeys[0].structure.walls[0].id;
    const DomainId navigation[] = {a, b, DOMAIN_ID_INVALID};
    for (size_t i = 0; i < 3; i++) {
        editor.current_room_id = navigation[i];
        sitehelper_editor_reconcile(&editor, &project);
        assert(editor.current_wall_id == project.storeys[0].structure.walls[0].id);
        RoomRegionResult ra = room_region_resolve(&project, a, &t), rb = room_region_resolve(&project, b, &t);
        assert(ra.room_id == a && rb.room_id == b && ra.face_index == rb.face_index);
        check_rectangle_boundary(ra);
    }
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_allocations = 1;
    assert(room_region_resolve(&project, a, &t).code == ROOM_REGION_BOUNDED);
    classify(&t, (PlanPosition){1000, 1000}, PLAN_TOPOLOGY_POINT_BOUNDED);
    reject_allocations = 0;
    assert(rejected_allocations == 0);
#endif
    unchanged(&saved, &t);
    test_assert_project_authoritative_equal(&before, &project);
    assert(sitehelper_project_save_file(&project, file_b) == SITEHELPER_PERSISTENCE_SUCCESS);
    same_files(file_a, file_b);
    plan_topology_destroy(&saved.copy);
    sitehelper_project_destroy(&before);
    Wall wall = project.storeys[0].structure.walls[0]; project.storeys[0].structure.walls[0] = project.storeys[0].structure.walls[1]; project.storeys[0].structure.walls[1] = wall;
    RoomSeparator separator = project.storeys[0].structure.room_separators[0];
    project.storeys[0].structure.room_separators[0] = project.storeys[0].structure.room_separators[1]; project.storeys[0].structure.room_separators[1] = separator;
    for (size_t i = 0; i < project.storeys[0].structure.wall_count; i++) {
        WallPlanSegment *s = &project.storeys[0].structure.walls[i].definition.segment;
        PlanPosition p = s->start; s->start = s->end; s->end = p;
    }
    for (size_t i = 0; i < project.storeys[0].structure.room_separator_count; i++) {
        PlanSegment *s = &project.storeys[0].structure.room_separators[i].segment;
        PlanPosition p = s->start; s->start = s->end; s->end = p;
    }
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_SUCCESS);
    check_rectangle_boundary(room_region_resolve(&project, a, &t));
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{3000, 0}, {3000, 4000}}));
    /* Existing output intentionally still describes its original snapshot. */
    assert(room_region_resolve(&project, a, &t).face_index == room_region_resolve(&project, b, &t).face_index);
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_SUCCESS);
    RoomRegionResult ra = room_region_resolve(&project, a, &t), rb = room_region_resolve(&project, b, &t);
    assert(ra.code == ROOM_REGION_BOUNDED && rb.code == ROOM_REGION_BOUNDED && ra.face_index != rb.face_index);
    size_t left_face = ra.face_index, right_face = rb.face_index;
    separator = project.storeys[0].structure.room_separators[0];
    project.storeys[0].structure.room_separators[0] = project.storeys[0].structure.room_separators[2]; project.storeys[0].structure.room_separators[2] = separator;
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_SUCCESS);
    ra = room_region_resolve(&project, a, &t); rb = room_region_resolve(&project, b, &t);
    assert(ra.face_index == left_face && rb.face_index == right_face); /* Equivalent snapshot ordering. */
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    /* Result geometry survives project destruction, but expires with topology.
     * Do not dereference earlier resolution results after a rebuild. */
    sitehelper_project_destroy(&project);
    assert(ra.topology->faces[ra.face_index].bounded);
    classify(&t, (PlanPosition){1000, 1000}, PLAN_TOPOLOGY_POINT_BOUNDED);
    plan_topology_destroy(&t);
    assert(plan_topology_find_face_at_plan_position(&t, (PlanPosition){1000, 1000}).code == PLAN_TOPOLOGY_INVALID_ARGUMENT);
}

static void test_topology_failures_are_not_spatial_answers(void)
{
    SiteHelperProject project, before;
    rectangle_project(&project);
    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){1000, 1000}));
    PlanTopology t = {0};
    assert(room_region_build_and_resolve(&project, room, &t).code == ROOM_REGION_BOUNDED);
    TopologySnapshot saved = snapshot(&t);
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_allocations = 1;
    RoomRegionResult failed = room_region_build_and_resolve(&project, room, &t);
    reject_allocations = 0;
    assert(failed.code == ROOM_REGION_TOPOLOGY_FAILED && failed.topology_result.code == PLAN_TOPOLOGY_ALLOCATION_FAILED);
    assert(failed.face_index == SIZE_MAX && failed.topology == NULL);
    assert(rejected_allocations == 1);
    unchanged(&saved, &t);
#endif
    DomainId overlap = sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 0}, {3000, 0}});
    assert(overlap);
    test_clone_project_authoritative(&project, &before);
    RoomRegionResult r = room_region_build_and_resolve(&project, room, &t);
    assert(r.code == ROOM_REGION_TOPOLOGY_FAILED && r.topology_result.code == PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP);
    assert(r.topology_result.source_id == project.storeys[0].structure.walls[0].id && r.topology_result.related_source_id == overlap);
    assert(r.topology == NULL && r.face_index == SIZE_MAX);
    unchanged(&saved, &t);
    test_assert_project_authoritative_equal(&before, &project);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    assert(sitehelper_project_clear_room_location(&project, room));
    assert(room_region_build_and_resolve(&project, room, &t).code == ROOM_REGION_UNPLACED);
    unchanged(&saved, &t); /* Unsupported geometry was not examined for unplaced Room. */
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){1000, 1000}));
    assert(sitehelper_project_remove_room_separator_by_id(&project, overlap));
    DomainId inside = sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{2000, 2000}, {2500, 2500}});
    r = room_region_build_and_resolve(&project, room, &t);
    assert(r.code == ROOM_REGION_TOPOLOGY_FAILED && r.topology_result.code == PLAN_TOPOLOGY_UNSUPPORTED_NESTING);
    unchanged(&saved, &t);
    assert(sitehelper_project_remove_room_separator_by_id(&project, inside));
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 2000}, {2000, 2000}}));
    r = room_region_build_and_resolve(&project, room, &t);
    assert(r.code == ROOM_REGION_TOPOLOGY_FAILED && r.topology_result.code == PLAN_TOPOLOGY_UNSUPPORTED_NON_SIMPLE_FACE);
    unchanged(&saved, &t);
    plan_topology_destroy(&saved.copy);
    plan_topology_destroy(&t);
    sitehelper_project_destroy(&project);
    sitehelper_project_destroy(&before);
}

int main(void)
{
    test_room_states();
    test_empty_open_and_invalid_queries();
    test_exact_rational_geometry_and_symmetries();
    test_concave_and_disconnected_faces();
    test_multiple_rooms_read_only_and_lifetime();
    test_topology_failures_are_not_spatial_answers();
    puts("room region tests passed");
    return 0;
}
