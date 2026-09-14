#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "command_history.h"
#include "slab.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
void *__real_malloc(size_t);void *__real_calloc(size_t,size_t);void *__real_realloc(void*,size_t);
static int fail_now(void){return fail_after!=SIZE_MAX&&fail_after--==0;}
void *__wrap_malloc(size_t n){return fail_now()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){return fail_now()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void *p,size_t n){return fail_now()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition p0[]={{1000,1000},{1500,1000},{1500,1500},{1000,1500}};
static const PlanPosition p1[]={{3000,1000},{3500,1000},{3500,1500},{3000,1500}};
static const PlanPosition p2[]={{5000,1000},{5500,1000},{5500,1500},{5000,1500}};
static const PlanPosition p3[]={{7000,1000},{7500,1000},{7500,1500},{7000,1500}};

static DomainId fixture(SiteHelperProject *project)
{
    sitehelper_project_init(project);DomainId storey=sitehelper_project_add_storey(project,0);
    DomainId slab=sitehelper_project_add_slab(project,storey,outer,4,100,0);
    assert(slab!=DOMAIN_ID_INVALID);return slab;
}

static SiteHelperCommand add_penetration(DomainId slab,const PlanPosition *vertices)
{
    AddSlabPenetrationCommand add={0};SiteHelperCommand command={0};
    assert(add_slab_penetration_command_create(slab,vertices,4,&add));
    assert(sitehelper_command_from_add_slab_penetration(&add,&command));
    add_slab_penetration_command_destroy(&add);return command;
}

static SiteHelperCommand add_region(DomainId slab,const PlanPosition *vertices,int top,int thick)
{
    AddSlabRegionCommand add={0};SiteHelperCommand command={0};
    assert(add_slab_region_command_create(slab,vertices,4,top,thick,&add));
    assert(sitehelper_command_from_add_slab_region(&add,&command));
    add_slab_region_command_destroy(&add);return command;
}

static SiteHelperCommand add_rebate(DomainId slab,int start,int end,int depth)
{
    AddSlabEdgeRebateCommand add;SiteHelperCommand command={0};
    assert(add_slab_edge_rebate_command_create(slab,0,start,end,100,depth,&add));
    assert(sitehelper_command_from_add_slab_edge_rebate(&add,&command));return command;
}

static void execute_destroy(SiteHelperCommandHistory *history,SiteHelperProject *project,
    SiteHelperCommand *command,SiteHelperCommandResult *result)
{
    assert(sitehelper_command_history_execute(history,project,command,result));
    sitehelper_command_destroy(command);
}

static void test_add_lifecycles(void)
{
    SiteHelperProject project;DomainId slab_id=fixture(&project),next=project.domain_ids.next;
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
    SiteHelperCommandResult result;

    SiteHelperCommand penetration=add_penetration(slab_id,p0);
    PlanPosition *caller_copy=penetration.data.add_slab_penetration.vertices;
    execute_destroy(&history,&project,&penetration,&result);
    assert(result.type==SITEHELPER_COMMAND_ADD_SLAB_PENETRATION&&
        result.data.slab_feature.slab_id==slab_id&&result.data.slab_feature.feature_index==0);
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    assert(slab->definition.penetrations.count==1&&
        slab->definition.penetrations.items[0].outline.vertices!=caller_copy);
    assert(project.domain_ids.next==next);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(slab->definition.penetrations.count==0);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(slab->definition.penetrations.count==1&&project.domain_ids.next==next);

    SiteHelperCommand region=add_region(slab_id,p1,-50,75);
    execute_destroy(&history,&project,&region,&result);
    assert(result.data.slab_feature.feature_index==0&&slab->definition.regions.count==1);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(sitehelper_command_history_redo(&history,&project));
    assert(slab->definition.regions.items[0].top_level_offset_mm==-50&&
        slab->definition.regions.items[0].thickness_mm==75);

    SiteHelperCommand rebate=add_rebate(slab_id,0,1000,20);
    execute_destroy(&history,&project,&rebate,&result);
    assert(result.data.slab_feature.feature_index==0&&slab->definition.edge_rebates.count==1);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(sitehelper_command_history_redo(&history,&project));
    assert(slab->definition.edge_rebates.items[0].depth_mm==20&&project.domain_ids.next==next);

    sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);
}

static void test_add_validation_and_conflicts(void)
{
    SiteHelperProject project;DomainId slab_id=fixture(&project);
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
    SiteHelperCommandResult result;
    PlanPosition outside[]={{0,0},{100,0},{100,100},{0,100}};
    SiteHelperCommand invalid=add_penetration(slab_id,outside);
    assert(!sitehelper_command_history_execute(&history,&project,&invalid,&result));
    assert(history.count==0&&project.storeys[0].slabs.items[0].definition.penetrations.count==0);
    sitehelper_command_destroy(&invalid);

    SiteHelperCommand added=add_penetration(slab_id,p0);
    assert(sitehelper_command_history_execute(&history,&project,&added,&result));
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    assert(slab_remove_penetration(slab,0)==SLAB_SUCCESS);
    assert(slab_add_penetration(slab,p1,4)==SLAB_SUCCESS);
    assert(!sitehelper_command_history_undo(&history,&project));
    assert(history.cursor==1&&slab->definition.penetrations.count==1&&
        slab->definition.penetrations.items[0].outline.vertices[0].x==p1[0].x);
    sitehelper_command_destroy(&added);
    sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);
}

static void test_polygon_variants(void)
{
    const PlanPosition concave[]={{1000,1000},{3000,1000},{3000,3000},
        {2000,2000},{1000,3000}};
    const PlanPosition clockwise[]={{7000,1000},{7000,2000},{8000,2000},{8000,1000}};
    SiteHelperProject project;DomainId slab_id=fixture(&project);
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
    SiteHelperCommandResult result;SiteHelperCommand command={0};
    AddSlabPenetrationCommand penetration={0};
    assert(add_slab_penetration_command_create(slab_id,concave,5,&penetration));
    assert(sitehelper_command_from_add_slab_penetration(&penetration,&command));
    add_slab_penetration_command_destroy(&penetration);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    AddSlabRegionCommand region={0};
    assert(add_slab_region_command_create(slab_id,clockwise,4,-25,80,&region));
    assert(sitehelper_command_from_add_slab_region(&region,&command));
    add_slab_region_command_destroy(&region);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    assert(slab->definition.penetrations.items[0].outline.vertex_count==5&&
        slab->definition.regions.items[0].outline.vertices[0].x==7000);
    sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_relationship_validation_through_commands(void)
{
    SiteHelperProject project;DomainId slab_id=fixture(&project);
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
    SiteHelperCommandResult result;

    assert(slab_add_penetration(slab,p0,4)==SLAB_SUCCESS);
    PlanPosition overlap_hole[]={{1200,1200},{1700,1200},{1700,1700},{1200,1700}};
    SiteHelperCommand command=add_penetration(slab_id,overlap_hole);
    assert(!sitehelper_command_history_execute(&history,&project,&command,&result));
    assert(history.count==0&&slab->definition.penetrations.count==1);sitehelper_command_destroy(&command);

    PlanPosition crossing_region0[]={{1200,800},{1800,800},{1800,1300},{1200,1300}};
    command=add_region(slab_id,crossing_region0,-50,75);
    assert(!sitehelper_command_history_execute(&history,&project,&command,&result));
    assert(history.count==0&&slab->definition.regions.count==0);sitehelper_command_destroy(&command);

    PlanPosition edge_region[]={{0,3000},{2000,3000},{2000,4000},{0,4000}};
    PlanPosition adjacent_region[]={{2000,3000},{4000,3000},{4000,4000},{2000,4000}};
    command=add_region(slab_id,edge_region,-50,75);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));sitehelper_command_destroy(&command);
    command=add_region(slab_id,adjacent_region,-25,90);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));sitehelper_command_destroy(&command);
    PlanPosition overlap_region[]={{1000,3500},{3000,3500},{3000,4500},{1000,4500}};
    command=add_region(slab_id,overlap_region,0,100);
    assert(!sitehelper_command_history_execute(&history,&project,&command,&result));
    assert(slab->definition.regions.count==2);sitehelper_command_destroy(&command);
    PlanPosition crossing_hole[]={{1500,2500},{2500,2500},{2500,3500},{1500,3500}};
    command=add_penetration(slab_id,crossing_hole);
    assert(!sitehelper_command_history_execute(&history,&project,&command,&result));
    assert(slab->definition.penetrations.count==1);sitehelper_command_destroy(&command);

    command=add_rebate(slab_id,0,1000,20);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));sitehelper_command_destroy(&command);
    command=add_rebate(slab_id,1000,2000,40); /* Exact adjacency. */
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));sitehelper_command_destroy(&command);
    command=add_rebate(slab_id,500,1500,30);
    assert(!sitehelper_command_history_execute(&history,&project,&command,&result));sitehelper_command_destroy(&command);
    AddSlabEdgeRebateCommand bad;assert(add_slab_edge_rebate_command_create(
        slab_id,99,0,100,100,20,&bad));
    assert(sitehelper_command_from_add_slab_edge_rebate(&bad,&command));
    assert(!sitehelper_command_history_execute(&history,&project,&command,&result));sitehelper_command_destroy(&command);

    sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);
}

/* Regions cannot coincide with penetrations. Use independent fixtures per collection. */
static void populate_penetrations(Slab *slab)
{
    assert(slab_add_penetration(slab,p0,4)==SLAB_SUCCESS);
    assert(slab_add_penetration(slab,p1,4)==SLAB_SUCCESS);
    assert(slab_add_penetration(slab,p2,4)==SLAB_SUCCESS);
}
static void populate_regions(Slab *slab)
{
    assert(slab_add_region(slab,p0,4,-10,90)==SLAB_SUCCESS);
    assert(slab_add_region(slab,p1,4,-20,80)==SLAB_SUCCESS);
    assert(slab_add_region(slab,p2,4,-30,70)==SLAB_SUCCESS);
}
static void populate_rebates(Slab *slab)
{
    assert(slab_add_edge_rebate(slab,0,0,1000,100,10)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,2000,3000,100,20)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,4000,5000,100,30)==SLAB_SUCCESS);
}

static void test_delete_middle_each(void)
{
    for(int kind=0;kind<3;kind++){
        SiteHelperProject project;DomainId slab_id=fixture(&project);
        Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
        if(kind==0)populate_penetrations(slab);else if(kind==1)populate_regions(slab);else populate_rebates(slab);
        Slab expected={0};assert(slab_clone(slab,&expected)==SLAB_SUCCESS);
        SiteHelperCommand command={0};
        if(kind==0){DeleteSlabPenetrationCommand d;assert(delete_slab_penetration_command_create(slab_id,1,&d));
            assert(sitehelper_command_from_delete_slab_penetration(&d,&command));}
        else if(kind==1){DeleteSlabRegionCommand d;assert(delete_slab_region_command_create(slab_id,1,&d));
            assert(sitehelper_command_from_delete_slab_region(&d,&command));}
        else {DeleteSlabEdgeRebateCommand d;assert(delete_slab_edge_rebate_command_create(slab_id,1,&d));
            assert(sitehelper_command_from_delete_slab_edge_rebate(&d,&command));}
        SiteHelperCommandHistory history;sitehelper_command_history_init(&history);SiteHelperCommandResult result;
        assert(sitehelper_command_history_execute(&history,&project,&command,&result));
        assert(result.data.slab_feature.feature_index==1);
        assert(sitehelper_command_history_undo(&history,&project));
        test_assert_slab_equal(&expected,slab);
        assert(sitehelper_command_history_redo(&history,&project));
        if(kind==0)assert(slab->definition.penetrations.count==2&&slab->definition.penetrations.items[1].outline.vertices[0].x==p2[0].x);
        else if(kind==1)assert(slab->definition.regions.count==2&&slab->definition.regions.items[1].top_level_offset_mm==-30);
        else assert(slab->definition.edge_rebates.count==2&&slab->definition.edge_rebates.items[1].depth_mm==30);
        sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&history);
        slab_destroy(&expected);sitehelper_project_destroy(&project);
    }
}

static void test_delete_conflicts_and_missing_parent(void)
{
    SiteHelperProject project;DomainId slab_id=fixture(&project);
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);populate_penetrations(slab);
    DeleteSlabPenetrationCommand deletion;SiteHelperCommand command={0};
    assert(delete_slab_penetration_command_create(slab_id,1,&deletion));
    assert(sitehelper_command_from_delete_slab_penetration(&deletion,&command));
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    assert(slab_add_penetration(slab,p3,4)==SLAB_SUCCESS); /* Count conflict. */
    assert(!sitehelper_command_history_undo(&history,&project));assert(history.cursor==1);
    assert(slab->definition.penetrations.count==3);
    sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);

    slab_id=fixture(&project);slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    populate_rebates(slab);DeleteSlabEdgeRebateCommand rebate_delete;
    assert(delete_slab_edge_rebate_command_create(slab_id,1,&rebate_delete));
    assert(sitehelper_command_from_delete_slab_edge_rebate(&rebate_delete,&command));
    sitehelper_command_history_init(&history);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    assert(sitehelper_project_remove_slab_by_id(&project,slab_id));
    assert(!sitehelper_command_history_undo(&history,&project)&&history.cursor==1);
    sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);

    /* After a normal undo, replacing the restored index with another value of
     * the same collection size must make redo fail without deleting it. */
    slab_id=fixture(&project);slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    populate_regions(slab);DeleteSlabRegionCommand region_delete;
    assert(delete_slab_region_command_create(slab_id,1,&region_delete));
    assert(sitehelper_command_from_delete_slab_region(&region_delete,&command));
    sitehelper_command_history_init(&history);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    assert(sitehelper_command_history_undo(&history,&project));
    assert(slab_remove_region(slab,1)==SLAB_SUCCESS);
    assert(slab_insert_region_at(slab,1,p3,4,-40,60)==SLAB_SUCCESS);
    assert(!sitehelper_command_history_redo(&history,&project)&&history.cursor==0);
    assert(slab->definition.regions.count==3&&
        slab->definition.regions.items[1].top_level_offset_mm==-40);
    sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_owning_redo_branch_destruction(void)
{
    SiteHelperProject project;DomainId slab_id=fixture(&project);
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
    SiteHelperCommandResult result;
    SiteHelperCommand region=add_region(slab_id,p0,-10,90);
    assert(sitehelper_command_history_execute(&history,&project,&region,&result));
    sitehelper_command_destroy(&region);
    assert(sitehelper_command_history_undo(&history,&project));
    SiteHelperCommand penetration=add_penetration(slab_id,p1);
    assert(sitehelper_command_history_execute(&history,&project,&penetration,&result));
    sitehelper_command_destroy(&penetration);
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    assert(history.count==1&&history.cursor==1&&slab->definition.regions.count==0&&
        slab->definition.penetrations.count==1);
    sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
typedef SiteHelperCommand (*MakeAdd)(DomainId,const PlanPosition *);
static size_t sweep_add_history(MakeAdd make,const PlanPosition *vertices)
{
    size_t failures=0;
    for(size_t point=0;point<12;point++){
        fail_after=SIZE_MAX;SiteHelperProject project,before;DomainId slab=fixture(&project);
        SiteHelperCommand command=make(slab,vertices);SiteHelperCommandHistory history;
        sitehelper_command_history_init(&history);test_clone_project_authoritative(&project,&before);
        SiteHelperCommandResult result;fail_after=point;
        int ok=sitehelper_command_history_execute(&history,&project,&command,&result);fail_after=SIZE_MAX;
        if(!ok){failures++;test_assert_project_authoritative_equal(&before,&project);
            assert(history.count==0&&history.cursor==0);
            assert(sitehelper_command_history_execute(&history,&project,&command,&result));}
        sitehelper_project_destroy(&before);sitehelper_command_destroy(&command);
        sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);if(ok)break;
    }
    return failures;
}

static size_t sweep_delete(int kind,int undo)
{
    size_t failures=0;
    for(size_t point=0;point<12;point++){
        fail_after=SIZE_MAX;SiteHelperProject project,before;DomainId slab_id=fixture(&project);
        Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
        SiteHelperCommand command={0};
        if(kind==0){assert(slab_add_penetration(slab,p0,4)==SLAB_SUCCESS);DeleteSlabPenetrationCommand d;
            assert(delete_slab_penetration_command_create(slab_id,0,&d));assert(sitehelper_command_from_delete_slab_penetration(&d,&command));}
        else if(kind==1){assert(slab_add_region(slab,p0,4,-10,90)==SLAB_SUCCESS);DeleteSlabRegionCommand d;
            assert(delete_slab_region_command_create(slab_id,0,&d));assert(sitehelper_command_from_delete_slab_region(&d,&command));}
        else {assert(slab_add_edge_rebate(slab,0,0,1000,100,20)==SLAB_SUCCESS);DeleteSlabEdgeRebateCommand d;
            assert(delete_slab_edge_rebate_command_create(slab_id,0,&d));assert(sitehelper_command_from_delete_slab_edge_rebate(&d,&command));}
        SiteHelperCommandHistory history;sitehelper_command_history_init(&history);SiteHelperCommandResult result;
        if(!undo){test_clone_project_authoritative(&project,&before);fail_after=point;
            int ok=sitehelper_command_history_execute(&history,&project,&command,&result);fail_after=SIZE_MAX;
            if(!ok){failures++;test_assert_project_authoritative_equal(&before,&project);assert(history.count==0);
                assert(sitehelper_command_history_execute(&history,&project,&command,&result));}
            sitehelper_project_destroy(&before);sitehelper_command_destroy(&command);
            sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);if(ok)break;
        }else{
            assert(sitehelper_command_history_execute(&history,&project,&command,&result));
            /* Force indexed restoration to grow its now-empty collection. */
            if(kind==0){free(slab->definition.penetrations.items);slab->definition.penetrations=(SlabPenetrationCollection){0};}
            else if(kind==1){free(slab->definition.regions.items);slab->definition.regions=(SlabRegionCollection){0};}
            else {free(slab->definition.edge_rebates.items);slab->definition.edge_rebates=(SlabEdgeRebateCollection){0};}
            fail_after=point;int ok=sitehelper_command_history_undo(&history,&project);fail_after=SIZE_MAX;
            if(!ok){failures++;assert(history.cursor==1);assert(sitehelper_command_history_undo(&history,&project));}
            assert(history.cursor==0);sitehelper_command_destroy(&command);
            sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);if(ok)break;
        }
    }
    return failures;
}

static size_t sweep_add_rebate(void)
{
    size_t failures=0;
    for(size_t point=0;point<6;point++){
        fail_after=SIZE_MAX;SiteHelperProject project,before;DomainId slab=fixture(&project);
        SiteHelperCommand command=add_rebate(slab,0,1000,20);
        SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
        test_clone_project_authoritative(&project,&before);SiteHelperCommandResult result;
        fail_after=point;int ok=sitehelper_command_history_execute(&history,&project,&command,&result);
        fail_after=SIZE_MAX;
        if(!ok){failures++;test_assert_project_authoritative_equal(&before,&project);
            assert(history.count==0&&history.cursor==0);
            assert(sitehelper_command_history_execute(&history,&project,&command,&result));}
        sitehelper_project_destroy(&before);sitehelper_command_destroy(&command);
        sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);if(ok)break;
    }
    return failures;
}

static void test_allocation_failures(void)
{
    size_t constructors=0;
    fail_after=0;AddSlabPenetrationCommand penetration={0};
    assert(!add_slab_penetration_command_create(2,p0,4,&penetration));constructors++;fail_after=SIZE_MAX;
    fail_after=0;AddSlabRegionCommand region={0};
    assert(!add_slab_region_command_create(2,p0,4,0,100,&region));constructors++;fail_after=SIZE_MAX;
    size_t add_pen=sweep_add_history(add_penetration,p0);
    size_t add_reg=0;
    for(size_t point=0;point<12;point++){
        fail_after=SIZE_MAX;SiteHelperProject project,before;DomainId slab=fixture(&project);
        SiteHelperCommand command=add_region(slab,p0,-10,90);SiteHelperCommandHistory history;
        sitehelper_command_history_init(&history);test_clone_project_authoritative(&project,&before);
        SiteHelperCommandResult result;fail_after=point;int ok=sitehelper_command_history_execute(&history,&project,&command,&result);fail_after=SIZE_MAX;
        if(!ok){add_reg++;test_assert_project_authoritative_equal(&before,&project);assert(history.count==0);
            assert(sitehelper_command_history_execute(&history,&project,&command,&result));}
        sitehelper_project_destroy(&before);sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&history);sitehelper_project_destroy(&project);if(ok)break;
    }
    size_t del_pen=sweep_delete(0,0),del_reg=sweep_delete(1,0),del_reb=sweep_delete(2,0);
    size_t add_reb=sweep_add_rebate();
    size_t restore_pen=sweep_delete(0,1),restore_reg=sweep_delete(1,1),restore_reb=sweep_delete(2,1);
    assert(constructors==2&&add_pen==4&&add_reg==4&&add_reb==2);
    assert(del_pen==3&&del_reg==3&&del_reb==2);
    assert(restore_pen==2&&restore_reg==2&&restore_reb==1);
    printf("feature command allocation failures: constructors=%zu add_penetration=%zu add_region=%zu add_rebate=%zu "
        "delete_penetration=%zu delete_region=%zu delete_rebate=%zu restore_penetration=%zu "
        "restore_region=%zu restore_rebate=%zu\n",constructors,add_pen,add_reg,add_reb,del_pen,del_reg,
        del_reb,restore_pen,restore_reg,restore_reb);
}
#endif

int main(void)
{
    test_add_lifecycles();test_add_validation_and_conflicts();test_polygon_variants();
    test_relationship_validation_through_commands();test_delete_middle_each();
    test_delete_conflicts_and_missing_parent();test_owning_redo_branch_destruction();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    return 0;
}
