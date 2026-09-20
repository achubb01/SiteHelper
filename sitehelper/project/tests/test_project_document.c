#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_project.h"
#include "sitehelper_persistence.h"

static void test_project_owned_plan_notes(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId ground = sitehelper_project_add_storey(&project, 0);
    DomainId upper = sitehelper_project_add_storey(&project, 2700);
    assert(ground && upper);
    DomainId wall = sitehelper_project_add_wall(&project, ground,
        (WallPlanSegment){{0, 0}, {4200, 0}});
    DomainId other_wall = sitehelper_project_add_wall(&project, upper,
        (WallPlanSegment){{0, 0}, {4200, 0}});
    assert(wall && other_wall);

    DomainId next = project.domain_ids.next;
    DomainId free_note = sitehelper_project_add_plan_note(&project, ground,
        (PlanPosition){100, 200}, DOMAIN_ID_INVALID, "Verify setout");
    assert(free_note == next && project.domain_ids.next == next + 1);
    DomainId linked_note = sitehelper_project_add_plan_note(&project, ground,
        (PlanPosition){300, 400}, wall, "Check lintel\nagainst schedule");
    assert(linked_note != DOMAIN_ID_INVALID);
    assert(project.document.annotation_count == 2);
    assert(sitehelper_project_contains_domain_id(&project, free_note));
    assert(sitehelper_project_find_owning_storey_const(&project, free_note) == NULL);
    const DocumentAnnotation *annotation = sitehelper_project_find_annotation_by_id_const(
        &project, linked_note);
    assert(annotation && annotation->target_id == wall);
    assert(annotation->anchor.storey_id == ground);
    assert(strcmp(annotation->text, "Check lintel\nagainst schedule") == 0);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);

    next = project.domain_ids.next;
    assert(sitehelper_project_add_plan_note(&project, ground, (PlanPosition){0}, other_wall,
        "cross-storey") == DOMAIN_ID_INVALID);
    assert(sitehelper_project_add_plan_note(&project, ground, (PlanPosition){0}, free_note,
        "annotation target") == DOMAIN_ID_INVALID);
    assert(sitehelper_project_add_plan_note(&project, 999999, (PlanPosition){0}, 0,
        "missing storey") == DOMAIN_ID_INVALID);
    assert(sitehelper_project_add_plan_note(&project, ground, (PlanPosition){0}, 0,
        "") == DOMAIN_ID_INVALID);
    assert(project.domain_ids.next == next);
    assert(project.document.annotation_count == 2);

    assert(sitehelper_project_remove_annotation_by_id(&project, free_note));
    assert(!sitehelper_project_contains_domain_id(&project, free_note));
    assert(!sitehelper_project_remove_annotation_by_id(&project, free_note));
    sitehelper_project_destroy(&project);
}

static void test_validation_catches_annotation_corruption(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId a = sitehelper_project_add_storey(&project, 0);
    DomainId b = sitehelper_project_add_storey(&project, 3000);
    DomainId wall = sitehelper_project_add_wall(&project, a,
        (WallPlanSegment){{0,0},{1000,0}});
    DomainId other = sitehelper_project_add_wall(&project, b,
        (WallPlanSegment){{0,0},{1000,0}});
    DomainId note_id = sitehelper_project_add_plan_note(&project, a,
        (PlanPosition){0}, wall, "A");
    assert(note_id);
    DocumentAnnotation *note = sitehelper_project_find_annotation_by_id(&project, note_id);
    assert(note);

    DomainId saved_target = note->target_id;
    note->target_id = other;
    SiteHelperProjectValidation result = sitehelper_project_validate(&project);
    assert(result.code == SITEHELPER_PROJECT_INVALID_ANNOTATION_REFERENCE);
    assert(result.subject_id == note_id && result.related_id == other);
    note->target_id = saved_target;

    note->target_id = 888888; /* Deleted/missing physical targets are weak refs. */
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    note->target_id = saved_target;

    DomainId saved_storey = note->anchor.storey_id;
    note->anchor.storey_id = 999999;
    result = sitehelper_project_validate(&project);
    assert(result.code == SITEHELPER_PROJECT_INVALID_ANNOTATION_REFERENCE);
    note->anchor.storey_id = saved_storey;

    char *saved_text = note->text;
    note->text = NULL;
    result = sitehelper_project_validate(&project);
    assert(result.code == SITEHELPER_PROJECT_INVALID_ANNOTATION);
    note->text = saved_text;

    note->id = wall;
    result = sitehelper_project_validate(&project);
    assert(result.code == SITEHELPER_PROJECT_DUPLICATE_ID);
    note->id = note_id;
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    sitehelper_project_destroy(&project);
}

static void test_persistence_round_trip(void)
{
    const char *path = "/tmp/sitehelper_project_document_test.txt";
    SiteHelperProject project, loaded;
    sitehelper_project_init(&project);
    sitehelper_project_init(&loaded);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    DomainId wall = sitehelper_project_add_wall(&project, storey,
        (WallPlanSegment){{-100, 20}, {4100, 20}});
    DomainId first = sitehelper_project_add_plan_note(&project, storey,
        (PlanPosition){25, -75}, 0, "Free standing note with spaces");
    DomainId second = sitehelper_project_add_plan_note(&project, storey,
        (PlanPosition){300, 20}, wall, "Line one\nLine two: UTF-8-ish bytes \xC2\xB5");
    assert(first && second);
    DomainId next = project.domain_ids.next;

    assert(sitehelper_project_save_file(&project, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.document.annotation_count == 2);
    assert(loaded.domain_ids.next == next);
    const DocumentAnnotation *a = sitehelper_project_find_annotation_by_id_const(&loaded, first);
    const DocumentAnnotation *b = sitehelper_project_find_annotation_by_id_const(&loaded, second);
    assert(a && b);
    assert(strcmp(a->text, "Free standing note with spaces") == 0);
    assert(b->target_id == wall && strcmp(b->text, "Line one\nLine two: UTF-8-ish bytes \xC2\xB5") == 0);
    assert(sitehelper_project_validate(&loaded).code == SITEHELPER_PROJECT_VALID);

    FILE *file = fopen(path, "r");
    assert(file);
    char header[64];
    assert(fgets(header, sizeof header, file));
    assert(strcmp(header, "sitehelper_project 22\n") == 0);
    fclose(file);
    remove(path);
    sitehelper_project_destroy(&loaded);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_project_owned_plan_notes();
    test_validation_catches_annotation_corruption();
    test_persistence_round_trip();
    puts("All project document tests passed.");
    return 0;
}
