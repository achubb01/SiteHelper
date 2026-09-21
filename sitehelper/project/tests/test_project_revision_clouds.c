#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_project.h"
#include "sitehelper_persistence.h"

static const PlanPosition cloud_a[]={{0,0},{1000,0},{1000,700},{0,700}};
static const PlanPosition cloud_b[]={{50,50},{800,100},{700,600},{100,500}};

static void test_lifecycle_and_validation(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId id=sitehelper_project_add_plan_revision_cloud(&project,storey,cloud_a,4);
    assert(id&&sitehelper_project_contains_domain_id(&project,id));
    assert(sitehelper_project_find_owning_storey_const(&project,id)==NULL);
    const DocumentPlanRevisionCloud *cloud=sitehelper_project_find_revision_cloud_by_id_const(&project,id);
    assert(cloud&&cloud->vertex_count==4&&cloud->vertices!=cloud_a&&cloud->vertices[2].y==700);
    assert(sitehelper_project_validate(&project).code==SITEHELPER_PROJECT_VALID);
    assert(sitehelper_project_update_plan_revision_cloud(&project,id,storey,cloud_b,4));
    cloud=sitehelper_project_find_revision_cloud_by_id_const(&project,id);
    assert(cloud&&cloud->vertices[1].x==800);
    DomainId next=project.domain_ids.next;
    const PlanPosition invalid[]={{0,0},{100,0}};
    assert(sitehelper_project_add_plan_revision_cloud(&project,storey,invalid,2)==DOMAIN_ID_INVALID);
    assert(project.domain_ids.next==next);
    DocumentPlanRevisionCloud *mutable=sitehelper_project_find_revision_cloud_by_id(&project,id);
    assert(mutable); mutable->storey_id=999999;
    SiteHelperProjectValidation result=sitehelper_project_validate(&project);
    assert(result.code==SITEHELPER_PROJECT_INVALID_REVISION_CLOUD&&result.subject_id==id);
    mutable->storey_id=storey;
    assert(sitehelper_project_remove_revision_cloud_by_id(&project,id));
    assert(!sitehelper_project_contains_domain_id(&project,id));
    sitehelper_project_destroy(&project);
}

static void test_persistence_and_v20_compatibility(void)
{
    const char *path="/tmp/sitehelper_revision_cloud_v22.txt";
    const char *legacy="/tmp/sitehelper_revision_cloud_v20.txt";
    SiteHelperProject project,loaded,old; sitehelper_project_init(&project);
    sitehelper_project_init(&loaded); sitehelper_project_init(&old);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId id=sitehelper_project_add_plan_revision_cloud(&project,storey,cloud_a,4); assert(id);
    DomainId next=project.domain_ids.next;
    assert(sitehelper_project_save_file(&project,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *f=fopen(path,"r"); assert(f); char header[64]; assert(fgets(header,sizeof header,f));
    assert(strcmp(header,"sitehelper_project 23\n")==0); fclose(f);
    assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    const DocumentPlanRevisionCloud *cloud=sitehelper_project_find_revision_cloud_by_id_const(&loaded,id);
    assert(cloud&&cloud->vertex_count==4&&cloud->vertices[3].y==700&&loaded.domain_ids.next==next);

    /* A current file with no lifecycle metadata/clouds can be represented as v20 by
     * removing the v21 cloud collection and v22 revision collection. */
    SiteHelperProject empty; sitehelper_project_init(&empty);
    assert(sitehelper_project_add_storey(&empty,0));
    assert(sitehelper_project_save_file(&empty,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *in=fopen(path,"r"),*out=fopen(legacy,"w"); assert(in&&out);
    char line[512]; int first=1;
    while (fgets(line,sizeof line,in)) {
        if (first) { assert(strcmp(line,"sitehelper_project 23\n")==0);
            assert(fputs("sitehelper_project 20\n",out)>=0); first=0; continue; }
        if (strcmp(line,"revisions 0\n")==0 || strcmp(line,"revision_clouds 0\n")==0) continue;
        assert(fputs(line,out)>=0);
    }
    assert(fclose(out)==0&&fclose(in)==0);
    assert(sitehelper_project_load_file(&old,legacy)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(old.document.revision_cloud_count==0&&sitehelper_project_validate(&old).code==SITEHELPER_PROJECT_VALID);
    sitehelper_project_destroy(&empty);
    remove(path); remove(legacy);
    sitehelper_project_destroy(&old); sitehelper_project_destroy(&loaded); sitehelper_project_destroy(&project);
}

int main(void)
{
    test_lifecycle_and_validation();
    test_persistence_and_v20_compatibility();
    puts("All project revision cloud tests passed.");
    return 0;
}
