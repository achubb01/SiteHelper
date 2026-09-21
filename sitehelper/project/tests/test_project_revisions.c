#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sitehelper_project.h"
#include "sitehelper_persistence.h"

static const PlanPosition cloud_vertices[] = {
    {0,0},{1000,0},{1000,600},{0,600}
};

static void test_revision_lifecycle_and_weak_cloud_grouping(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey = sitehelper_project_add_storey(&project, 0); assert(storey);
    DomainId revision = sitehelper_project_add_revision(&project, "A", "Kitchen layout update");
    assert(revision != DOMAIN_ID_INVALID);
    const DocumentRevision *stored = sitehelper_project_find_revision_by_id_const(&project, revision);
    assert(stored && strcmp(stored->identifier, "A") == 0 &&
        strcmp(stored->description, "Kitchen layout update") == 0);
    assert(sitehelper_project_find_owning_storey_const(&project, revision) == NULL);

    DomainId cloud = sitehelper_project_add_plan_revision_cloud(&project, storey,
        cloud_vertices, 4); assert(cloud);
    assert(sitehelper_project_set_revision_cloud_revision(&project, cloud, revision));
    const DocumentPlanRevisionCloud *grouped =
        sitehelper_project_find_revision_cloud_by_id_const(&project, cloud);
    assert(grouped && grouped->revision_id == revision);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);

    /* Geometry edits do not accidentally detach lifecycle grouping. */
    const PlanPosition moved[]={{50,0},{900,0},{900,500},{50,500}};
    assert(sitehelper_project_update_plan_revision_cloud(&project, cloud, storey, moved, 4));
    grouped = sitehelper_project_find_revision_cloud_by_id_const(&project, cloud);
    assert(grouped && grouped->revision_id == revision);

    /* Deletion leaves an intentional weak link. Restoration of the same stable
     * ID reconnects without mutating the cloud. */
    DocumentRevision snapshot = {0};
    assert(document_revision_clone(stored, &snapshot) == DOCUMENT_SUCCESS);
    assert(sitehelper_project_remove_revision_by_id(&project, revision));
    assert(!sitehelper_project_contains_domain_id(&project, revision));
    grouped = sitehelper_project_find_revision_cloud_by_id_const(&project, cloud);
    assert(grouped && grouped->revision_id == revision);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    assert(sitehelper_project_insert_revision(&project, &snapshot));
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    document_revision_destroy(&snapshot);

    assert(sitehelper_project_update_revision(&project, revision, "B", "Resolved layout"));
    stored = sitehelper_project_find_revision_by_id_const(&project, revision);
    assert(stored && strcmp(stored->identifier,"B")==0 &&
        strcmp(stored->description,"Resolved layout")==0);
    assert(sitehelper_project_set_revision_cloud_revision(&project, cloud, DOMAIN_ID_INVALID));
    grouped = sitehelper_project_find_revision_cloud_by_id_const(&project, cloud);
    assert(grouped && grouped->revision_id == DOMAIN_ID_INVALID);
    assert(!sitehelper_project_set_revision_cloud_revision(&project, cloud, 999999));

    sitehelper_project_destroy(&project);
}

static void test_wrong_live_reference_is_rejected_by_validation(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey = sitehelper_project_add_storey(&project, 0); assert(storey);
    DomainId cloud = sitehelper_project_add_plan_revision_cloud(&project, storey,
        cloud_vertices, 4); assert(cloud);
    DomainId wall = sitehelper_project_add_wall(&project, storey,
        (WallPlanSegment){{0,0},{1000,0}}); assert(wall);
    DocumentPlanRevisionCloud *mutable =
        sitehelper_project_find_revision_cloud_by_id(&project, cloud); assert(mutable);
    mutable->revision_id = wall;
    SiteHelperProjectValidation result = sitehelper_project_validate(&project);
    assert(result.code == SITEHELPER_PROJECT_INVALID_REVISION_CLOUD);
    assert(result.subject_id == cloud && result.related_id == wall);
    sitehelper_project_destroy(&project);
}

static void rewrite_v22_as_v21(const char *source, const char *destination)
{
    FILE *in=fopen(source,"r"), *out=fopen(destination,"w"); assert(in&&out);
    char line[1024]; int first=1;
    while (fgets(line,sizeof line,in)) {
        if (first) {
            assert(strcmp(line,"sitehelper_project 23\n")==0);
            assert(fputs("sitehelper_project 21\n",out)>=0);
            first=0; continue;
        }
        if (strcmp(line,"revisions 0\n")==0) { continue; }
        if (strncmp(line,"revision_cloud ",15)==0) {
            char *revision = strstr(line," revision 0 vertices ");
            assert(revision != NULL);
            char rewritten[1024];
            size_t prefix=(size_t)(revision-line);
            assert(prefix < sizeof rewritten);
            memcpy(rewritten,line,prefix);
            rewritten[prefix]='\0';
            assert(snprintf(rewritten+prefix,sizeof rewritten-prefix," vertices %s",
                revision+strlen(" revision 0 vertices ")) > 0);
            assert(fputs(rewritten,out)>=0);
            continue;
        }
        assert(fputs(line,out)>=0);
    }
    assert(fclose(out)==0 && fclose(in)==0);
}

static void test_v22_persistence_and_v21_compatibility(void)
{
    const char *path="/tmp/sitehelper_revision_v22.txt";
    const char *legacy="/tmp/sitehelper_revision_v21.txt";
    SiteHelperProject project, loaded, old;
    sitehelper_project_init(&project); sitehelper_project_init(&loaded); sitehelper_project_init(&old);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId revision=sitehelper_project_add_revision(&project,"P2","Coordination review\nsecond line");
    assert(revision);
    DomainId cloud=sitehelper_project_add_plan_revision_cloud(&project,storey,cloud_vertices,4);
    assert(cloud && sitehelper_project_set_revision_cloud_revision(&project,cloud,revision));
    DomainId next=project.domain_ids.next;
    assert(sitehelper_project_save_file(&project,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *f=fopen(path,"r"); assert(f); char header[64]; assert(fgets(header,sizeof header,f));
    assert(strcmp(header,"sitehelper_project 23\n")==0); fclose(f);
    assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    const DocumentRevision *loaded_revision=sitehelper_project_find_revision_by_id_const(&loaded,revision);
    const DocumentPlanRevisionCloud *loaded_cloud=sitehelper_project_find_revision_cloud_by_id_const(&loaded,cloud);
    assert(loaded_revision && strcmp(loaded_revision->identifier,"P2")==0 &&
        strcmp(loaded_revision->description,"Coordination review\nsecond line")==0);
    assert(loaded_cloud && loaded_cloud->revision_id==revision && loaded.domain_ids.next==next);

    /* A v21 cloud has no lifecycle record/link fields and therefore loads as unassigned. */
    SiteHelperProject unassigned; sitehelper_project_init(&unassigned);
    DomainId old_storey=sitehelper_project_add_storey(&unassigned,0); assert(old_storey);
    DomainId old_cloud=sitehelper_project_add_plan_revision_cloud(&unassigned,old_storey,cloud_vertices,4);
    assert(old_cloud);
    assert(sitehelper_project_save_file(&unassigned,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    rewrite_v22_as_v21(path,legacy);
    assert(sitehelper_project_load_file(&old,legacy)==SITEHELPER_PERSISTENCE_SUCCESS);
    const DocumentPlanRevisionCloud *legacy_cloud=
        sitehelper_project_find_revision_cloud_by_id_const(&old,old_cloud);
    assert(legacy_cloud && legacy_cloud->revision_id==DOMAIN_ID_INVALID);
    assert(old.document.revision_count==0 && sitehelper_project_validate(&old).code==SITEHELPER_PROJECT_VALID);

    sitehelper_project_destroy(&unassigned);
    sitehelper_project_destroy(&old); sitehelper_project_destroy(&loaded); sitehelper_project_destroy(&project);
    remove(path); remove(legacy);
}

int main(void)
{
    test_revision_lifecycle_and_weak_cloud_grouping();
    test_wrong_live_reference_is_rejected_by_validation();
    test_v22_persistence_and_v21_compatibility();
    puts("All project revision lifecycle tests passed.");
    return 0;
}
