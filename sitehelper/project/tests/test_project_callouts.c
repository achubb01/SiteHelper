#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_project.h"
#include "sitehelper_persistence.h"

static void test_callout_lifecycle_and_validation(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId ground=sitehelper_project_add_storey(&project,0);
    DomainId upper=sitehelper_project_add_storey(&project,3000);
    assert(ground&&upper);

    DomainId id=sitehelper_project_add_plan_callout(&project,ground,
        (PlanPosition){100,200},(PlanPosition){700,500},"Check flashing");
    assert(id!=DOMAIN_ID_INVALID);
    assert(sitehelper_project_contains_domain_id(&project,id));
    assert(sitehelper_project_find_owning_storey_const(&project,id)==NULL);
    const DocumentPlanCallout *callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&callout->storey_id==ground&&strcmp(callout->text,"Check flashing")==0);
    assert(sitehelper_project_validate(&project).code==SITEHELPER_PROJECT_VALID);

    assert(sitehelper_project_update_plan_callout(&project,id,upper,
        (PlanPosition){-20,40},(PlanPosition){300,900},"Upper\ncallout"));
    callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&callout->storey_id==upper&&callout->target.x==-20&&
        callout->label_anchor.y==900&&strcmp(callout->text,"Upper\ncallout")==0);

    DomainId next=project.domain_ids.next;
    assert(sitehelper_project_add_plan_callout(&project,ground,(PlanPosition){1,1},
        (PlanPosition){1,1},"bad")==DOMAIN_ID_INVALID);
    assert(project.domain_ids.next==next);

    DocumentPlanCallout *mutable=sitehelper_project_find_callout_by_id(&project,id);
    assert(mutable);
    mutable->storey_id=999999;
    SiteHelperProjectValidation invalid=sitehelper_project_validate(&project);
    assert(invalid.code==SITEHELPER_PROJECT_INVALID_CALLOUT&&invalid.subject_id==id);
    mutable->storey_id=upper;

    assert(sitehelper_project_remove_callout_by_id(&project,id));
    assert(!sitehelper_project_contains_domain_id(&project,id));
    sitehelper_project_destroy(&project);
}

static void test_callout_persistence_round_trip(void)
{
    const char *path="/tmp/sitehelper_project_callouts_test.txt";
    SiteHelperProject project,loaded;
    sitehelper_project_init(&project); sitehelper_project_init(&loaded);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId id=sitehelper_project_add_plan_callout(&project,storey,
        (PlanPosition){10,20},(PlanPosition){500,700},"Line one\nLine two");
    assert(id);
    DomainId next=project.domain_ids.next;
    assert(sitehelper_project_save_file(&project,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file=fopen(path,"r"); assert(file);
    char header[64]; assert(fgets(header,sizeof header,file));
    assert(strcmp(header,"sitehelper_project 23\n")==0); fclose(file);
    assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.domain_ids.next==next&&loaded.document.callout_count==1);
    const DocumentPlanCallout *callout=sitehelper_project_find_callout_by_id_const(&loaded,id);
    assert(callout&&callout->target.x==10&&callout->label_anchor.y==700&&
        strcmp(callout->text,"Line one\nLine two")==0);
    assert(sitehelper_project_validate(&loaded).code==SITEHELPER_PROJECT_VALID);
    remove(path);
    sitehelper_project_destroy(&loaded); sitehelper_project_destroy(&project);
}


static void test_version_eighteen_loads_with_no_callouts(void)
{
    const char *current_path="/tmp/sitehelper_project_callouts_v19.txt";
    const char *legacy_path="/tmp/sitehelper_project_callouts_v18.txt";
    SiteHelperProject source,loaded;
    sitehelper_project_init(&source); sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&source,0)!=DOMAIN_ID_INVALID);
    assert(sitehelper_project_save_file(&source,current_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *input=fopen(current_path,"r"),*output=fopen(legacy_path,"w");
    assert(input&&output);
    char line[512]; int first=1;
    while (fgets(line,sizeof line,input)) {
        if (first) {
            assert(strcmp(line,"sitehelper_project 23\n")==0);
            assert(fputs("sitehelper_project 18\n",output)>=0);
            first=0; continue;
        }
        if (strcmp(line,"callouts 0\n")==0 || strcmp(line,"revisions 0\n")==0 || strcmp(line,"revision_clouds 0\n")==0) { continue; }
        assert(fputs(line,output)>=0);
    }
    assert(fclose(output)==0&&fclose(input)==0);
    assert(sitehelper_project_load_file(&loaded,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.document.callout_count==0);
    assert(sitehelper_project_validate(&loaded).code==SITEHELPER_PROJECT_VALID);
    remove(current_path); remove(legacy_path);
    sitehelper_project_destroy(&loaded); sitehelper_project_destroy(&source);
}

int main(void)
{
    test_callout_lifecycle_and_validation();
    test_callout_persistence_round_trip();
    test_version_eighteen_loads_with_no_callouts();
    puts("All project callout tests passed.");
    return 0;
}
