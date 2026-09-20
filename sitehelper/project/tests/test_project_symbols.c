#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_project.h"
#include "sitehelper_persistence.h"

static void test_symbol_lifecycle_and_validation(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId ground=sitehelper_project_add_storey(&project,0);
    DomainId upper=sitehelper_project_add_storey(&project,2800);
    assert(ground&&upper);

    DomainId id=sitehelper_project_add_plan_symbol(&project,ground,
        DOCUMENT_PLAN_SYMBOL_POINT_MARKER,(PlanPosition){125,-75},(DocumentPlanDirection){0,0});
    assert(id!=DOMAIN_ID_INVALID);
    assert(sitehelper_project_contains_domain_id(&project,id));
    assert(sitehelper_project_find_owning_storey_const(&project,id)==NULL);
    const DocumentPlanSymbol *symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->storey_id==ground&&symbol->anchor.x==125&&symbol->anchor.y==-75);
    assert(sitehelper_project_validate(&project).code==SITEHELPER_PROJECT_VALID);

    assert(sitehelper_project_update_plan_symbol(&project,id,upper,
        DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION,(PlanPosition){500,600},
        (DocumentPlanDirection){3,4}));
    symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->storey_id==upper&&symbol->anchor.x==500&&symbol->anchor.y==600);
    assert(symbol->kind==DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION&&symbol->direction.dx==3&&
        symbol->direction.dy==4);
    assert(!sitehelper_project_update_plan_symbol(&project,id,upper,
        DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION,(PlanPosition){500,600},
        (DocumentPlanDirection){6,8}));

    DomainId next=project.domain_ids.next;
    assert(sitehelper_project_add_plan_symbol(&project,ground,(DocumentPlanSymbolKind)999,
        (PlanPosition){0,0},(DocumentPlanDirection){0,0})==DOMAIN_ID_INVALID);
    assert(project.domain_ids.next==next);

    DocumentPlanSymbol *mutable_symbol=sitehelper_project_find_symbol_by_id(&project,id);
    assert(mutable_symbol);
    mutable_symbol->storey_id=999999;
    SiteHelperProjectValidation invalid=sitehelper_project_validate(&project);
    assert(invalid.code==SITEHELPER_PROJECT_INVALID_SYMBOL);
    assert(invalid.subject_id==id);
    mutable_symbol->storey_id=upper;

    assert(sitehelper_project_remove_symbol_by_id(&project,id));
    assert(!sitehelper_project_contains_domain_id(&project,id));
    sitehelper_project_destroy(&project);
}

static void test_symbol_persistence_round_trip(void)
{
    const char *path="/tmp/sitehelper_project_symbols_test.txt";
    SiteHelperProject project,loaded;
    sitehelper_project_init(&project);
    sitehelper_project_init(&loaded);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId first=sitehelper_project_add_plan_symbol(&project,storey,
        DOCUMENT_PLAN_SYMBOL_POINT_MARKER,(PlanPosition){100,200},(DocumentPlanDirection){0,0});
    DomainId second=sitehelper_project_add_plan_symbol(&project,storey,
        DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION,(PlanPosition){-300,400},
        (DocumentPlanDirection){-3,4});
    assert(storey&&first&&second);
    DomainId next=project.domain_ids.next;

    assert(sitehelper_project_save_file(&project,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file=fopen(path,"r"); assert(file);
    char header[64]; assert(fgets(header,sizeof header,file));
    assert(strcmp(header,"sitehelper_project 22\n")==0);
    fclose(file);

    assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.domain_ids.next==next&&loaded.document.symbol_count==2);
    const DocumentPlanSymbol *a=sitehelper_project_find_symbol_by_id_const(&loaded,first);
    const DocumentPlanSymbol *b=sitehelper_project_find_symbol_by_id_const(&loaded,second);
    assert(a&&b&&a->kind==DOCUMENT_PLAN_SYMBOL_POINT_MARKER&&
        a->anchor.x==100&&a->anchor.y==200&&
        b->kind==DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION&&b->anchor.x==-300&&b->anchor.y==400&&
        b->direction.dx==-3&&b->direction.dy==4);
    assert(sitehelper_project_validate(&loaded).code==SITEHELPER_PROJECT_VALID);

    remove(path);
    sitehelper_project_destroy(&loaded);
    sitehelper_project_destroy(&project);
}


static void test_version_seventeen_loads_with_no_symbols(void)
{
    const char *current_path = "/tmp/sitehelper_project_symbols_v18.txt";
    const char *legacy_path = "/tmp/sitehelper_project_symbols_v17.txt";
    SiteHelperProject source, loaded;
    sitehelper_project_init(&source);
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&source, 0) != DOMAIN_ID_INVALID);
    assert(source.document.symbol_count == 0);
    assert(sitehelper_project_save_file(&source, current_path) == SITEHELPER_PERSISTENCE_SUCCESS);

    FILE *input = fopen(current_path, "r");
    FILE *output = fopen(legacy_path, "w");
    assert(input != NULL && output != NULL);
    char line[512];
    int first_line = 1;
    while (fgets(line, sizeof line, input) != NULL) {
        if (first_line) {
            assert(strcmp(line, "sitehelper_project 22\n") == 0);
            assert(fputs("sitehelper_project 17\n", output) >= 0);
            first_line = 0;
            continue;
        }
        if (strcmp(line, "symbols 0\n") == 0 || strcmp(line, "callouts 0\n") == 0 ||
            strcmp(line, "revisions 0\n") == 0 || strcmp(line, "revision_clouds 0\n") == 0) {
            continue;
        }
        assert(fputs(line, output) >= 0);
    }
    assert(fclose(output) == 0);
    assert(fclose(input) == 0);

    assert(sitehelper_project_load_file(&loaded, legacy_path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.document.symbol_count == 0);
    assert(sitehelper_project_validate(&loaded).code == SITEHELPER_PROJECT_VALID);

    remove(current_path);
    remove(legacy_path);
    sitehelper_project_destroy(&loaded);
    sitehelper_project_destroy(&source);
}

static void test_version_nineteen_loads_point_symbols(void)
{
    const char *current_path="/tmp/sitehelper_project_symbols_v20.txt";
    const char *legacy_path="/tmp/sitehelper_project_symbols_v19.txt";
    SiteHelperProject source,loaded;
    sitehelper_project_init(&source); sitehelper_project_init(&loaded);
    DomainId storey=sitehelper_project_add_storey(&source,0);
    DomainId symbol=sitehelper_project_add_plan_symbol(&source,storey,
        DOCUMENT_PLAN_SYMBOL_POINT_MARKER,(PlanPosition){25,-50},
        (DocumentPlanDirection){0,0});
    assert(storey&&symbol);
    assert(sitehelper_project_save_file(&source,current_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *input=fopen(current_path,"r"),*output=fopen(legacy_path,"w");
    assert(input&&output);
    char line[512]; int first=1;
    while (fgets(line,sizeof line,input)) {
        if (first) {
            assert(strcmp(line,"sitehelper_project 22\n")==0);
            assert(fputs("sitehelper_project 20\n",output)>=0);
            first=0;
        } else if (strcmp(line,"revisions 0\n")!=0 && strcmp(line,"revision_clouds 0\n")!=0) {
            assert(fputs(line,output)>=0);
        }
    }
    assert(fclose(output)==0); assert(fclose(input)==0);
    assert(sitehelper_project_load_file(&loaded,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    const DocumentPlanSymbol *restored=sitehelper_project_find_symbol_by_id_const(&loaded,symbol);
    assert(restored&&restored->kind==DOCUMENT_PLAN_SYMBOL_POINT_MARKER&&
        restored->anchor.x==25&&restored->anchor.y==-50&&
        restored->direction.dx==0&&restored->direction.dy==0);
    remove(current_path); remove(legacy_path);
    sitehelper_project_destroy(&loaded); sitehelper_project_destroy(&source);
}

int main(void)
{
    test_symbol_lifecycle_and_validation();
    test_symbol_persistence_round_trip();
    test_version_seventeen_loads_with_no_symbols();
    test_version_nineteen_loads_point_symbols();
    puts("All project symbol tests passed.");
    return 0;
}
