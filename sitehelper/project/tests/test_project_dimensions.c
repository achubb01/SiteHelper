#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_project.h"
#include "sitehelper_persistence.h"
#include "wall.h"

static DocumentDimensionReference fixed(int x, int y)
{
    return (DocumentDimensionReference){
        .kind = DOCUMENT_DIMENSION_FIXED_POINT,
        .position = {x, y}
    };
}

static DocumentDimensionReference wall_ref(DocumentDimensionReferenceKind kind, DomainId wall_id)
{
    return (DocumentDimensionReference){.kind = kind, .target_id = wall_id};
}

static void test_fixed_and_associative_resolution(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    DomainId wall = sitehelper_project_add_wall(&project, storey,
        (WallPlanSegment){{100, 200}, {3100, 4200}});
    assert(storey && wall);

    DomainId fixed_id = sitehelper_project_add_plan_dimension(&project, storey,
        fixed(0, 0), fixed(3000, 4000), 250);
    assert(fixed_id != DOMAIN_ID_INVALID);
    PlanPosition a, b;
    int distance = 0;
    assert(sitehelper_project_resolve_plan_dimension(&project, fixed_id, &a, &b, &distance));
    assert(a.x == 0 && a.y == 0 && b.x == 3000 && b.y == 4000 && distance == 5000);
    const DocumentPlanDimension *fixed_dimension =
        sitehelper_project_find_dimension_by_id_const(&project, fixed_id);
    assert(fixed_dimension && fixed_dimension->offset_mm == 250);
    assert(sitehelper_project_find_owning_storey_const(&project, fixed_id) == NULL);

    DomainId associated = sitehelper_project_add_plan_dimension(&project, storey,
        wall_ref(DOCUMENT_DIMENSION_WALL_START, wall),
        wall_ref(DOCUMENT_DIMENSION_WALL_END, wall), -300);
    assert(associated != DOMAIN_ID_INVALID);
    assert(sitehelper_project_resolve_plan_dimension(&project, associated, &a, &b, &distance));
    assert(a.x == 100 && a.y == 200 && b.x == 3100 && b.y == 4200);
    assert(distance == 5000);

    Wall *stored_wall = sitehelper_project_find_wall_by_id(&project, wall);
    assert(stored_wall != NULL);
    assert(wall_set_plan_segment(stored_wall, (WallPlanSegment){{500, 600}, {3500, 4600}}));
    assert(sitehelper_project_resolve_plan_dimension(&project, associated, &a, &b, &distance));
    assert(a.x == 500 && a.y == 600 && b.x == 3500 && b.y == 4600 && distance == 5000);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    sitehelper_project_destroy(&project);
}

static void test_reference_rules_and_weak_deletion(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId ground = sitehelper_project_add_storey(&project, 0);
    DomainId upper = sitehelper_project_add_storey(&project, 2700);
    DomainId wall = sitehelper_project_add_wall(&project, ground,
        (WallPlanSegment){{0, 0}, {2400, 0}});
    DomainId other = sitehelper_project_add_wall(&project, upper,
        (WallPlanSegment){{0, 0}, {2400, 0}});
    assert(ground && upper && wall && other);

    DomainId next = project.domain_ids.next;
    assert(sitehelper_project_add_plan_dimension(&project, ground,
        wall_ref(DOCUMENT_DIMENSION_WALL_START, other), fixed(100, 0), 100) == DOMAIN_ID_INVALID);
    assert(sitehelper_project_add_plan_dimension(&project, ground,
        fixed(10, 10), fixed(10, 10), 100) == DOMAIN_ID_INVALID);
    assert(project.domain_ids.next == next);

    DomainId id = sitehelper_project_add_plan_dimension(&project, ground,
        wall_ref(DOCUMENT_DIMENSION_WALL_START, wall), fixed(3000, 0), 100);
    assert(id);
    assert(sitehelper_project_remove_wall_by_id(&project, wall));
    PlanPosition a, b; int distance;
    assert(!sitehelper_project_resolve_plan_dimension(&project, id, &a, &b, &distance));
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    assert(sitehelper_project_remove_dimension_by_id(&project, id));
    assert(!sitehelper_project_contains_domain_id(&project, id));
    sitehelper_project_destroy(&project);
}

static void test_persistence_round_trip(void)
{
    const char *path = "/tmp/sitehelper_project_dimensions_test.txt";
    SiteHelperProject project, loaded;
    sitehelper_project_init(&project);
    sitehelper_project_init(&loaded);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    DomainId wall = sitehelper_project_add_wall(&project, storey,
        (WallPlanSegment){{-200, 50}, {3800, 50}});
    DomainId dimension = sitehelper_project_add_plan_dimension(&project, storey,
        wall_ref(DOCUMENT_DIMENSION_WALL_START, wall), fixed(4800, 50), -450);
    assert(dimension);
    DomainId next = project.domain_ids.next;

    assert(sitehelper_project_save_file(&project, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.domain_ids.next == next);
    assert(loaded.document.dimension_count == 1);
    const DocumentPlanDimension *stored = sitehelper_project_find_dimension_by_id_const(&loaded, dimension);
    assert(stored && stored->offset_mm == -450 && stored->first.target_id == wall);
    PlanPosition a, b; int distance;
    assert(sitehelper_project_resolve_plan_dimension(&loaded, dimension, &a, &b, &distance));
    assert(a.x == -200 && a.y == 50 && b.x == 4800 && b.y == 50 && distance == 5000);

    FILE *file = fopen(path, "r");
    assert(file != NULL);
    char header[64];
    assert(fgets(header, sizeof header, file) != NULL);
    assert(strcmp(header, "sitehelper_project 22\n") == 0);
    fclose(file);
    remove(path);
    sitehelper_project_destroy(&loaded);
    sitehelper_project_destroy(&project);
}

static void test_validation_rejects_wrong_live_reference_type(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    DomainId slab = sitehelper_project_add_slab(&project, storey,
        (PlanPosition[]){{0,0},{1000,0},{1000,1000},{0,1000}}, 4, 100, 0);
    DomainId dimension = sitehelper_project_add_plan_dimension(&project, storey,
        fixed(0,0), fixed(1000,0), 100);
    assert(slab && dimension);
    DocumentPlanDimension *stored = sitehelper_project_find_dimension_by_id(&project, dimension);
    assert(stored);
    stored->first = wall_ref(DOCUMENT_DIMENSION_WALL_START, slab);
    SiteHelperProjectValidation result = sitehelper_project_validate(&project);
    assert(result.code == SITEHELPER_PROJECT_INVALID_DIMENSION_REFERENCE);
    assert(result.subject_id == dimension && result.related_id == slab);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_fixed_and_associative_resolution();
    test_reference_rules_and_weak_deletion();
    test_persistence_round_trip();
    test_validation_rejects_wrong_live_reference_type();
    puts("All project dimension tests passed.");
    return 0;
}
