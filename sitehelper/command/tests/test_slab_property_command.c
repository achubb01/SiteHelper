#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "command_history.h"
#include "slab.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
void *__real_malloc(size_t); void *__real_calloc(size_t,size_t); void *__real_realloc(void*,size_t);
static int fail_now(void){return fail_after!=SIZE_MAX&&fail_after--==0;}
void *__wrap_malloc(size_t n){return fail_now()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){return fail_now()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void *p,size_t n){return fail_now()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition a[]={{0,0},{2000,0},{2000,1500},{0,1500}};
static const PlanPosition b[]={{3000,0},{5000,0},{5000,1500},{3000,1500}};
static const PlanPosition c[]={{6000,0},{8000,0},{8000,1500},{6000,1500}};

static DomainId fixture(SiteHelperProject *project)
{
    sitehelper_project_init(project);
    DomainId storey=sitehelper_project_add_storey(project,0);
    DomainId slab=sitehelper_project_add_slab(project,storey,outer,4,100,0);
    assert(slab!=DOMAIN_ID_INVALID);
    return slab;
}

static SiteHelperCommand wrap_slab(DomainId slab,int thickness,int top)
{
    EditSlabCommand edit; SiteHelperCommand command={0};
    assert(edit_slab_command_create(slab,thickness,top,&edit));
    assert(sitehelper_command_from_edit_slab(&edit,&command)); return command;
}
static SiteHelperCommand wrap_region(DomainId slab,size_t index,int top,int thickness)
{
    EditSlabRegionCommand edit; SiteHelperCommand command={0};
    assert(edit_slab_region_command_create(slab,index,top,thickness,&edit));
    assert(sitehelper_command_from_edit_slab_region(&edit,&command)); return command;
}
static SiteHelperCommand wrap_rebate(DomainId slab,size_t index,size_t edge,
    int start,int end,int width,int depth)
{
    EditSlabEdgeRebateCommand edit; SiteHelperCommand command={0};
    assert(edit_slab_edge_rebate_command_create(slab,index,edge,start,end,width,depth,&edit));
    assert(sitehelper_command_from_edit_slab_edge_rebate(&edit,&command)); return command;
}

static void test_lifecycles(void)
{
    SiteHelperProject p; DomainId id=fixture(&p);
    Slab *slab=sitehelper_project_find_slab_by_id(&p,id);
    assert(slab_add_region(slab,a,4,-10,80)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,0,1000,100,20)==SLAB_SUCCESS);
    SiteHelperCommandHistory h; sitehelper_command_history_init(&h);
    SiteHelperCommandResult result;

    SiteHelperCommand command=wrap_slab(id,125,-25);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    sitehelper_command_destroy(&command);
    assert(slab->definition.thickness_mm==125&&slab->definition.top_level_offset_mm==-25);
    assert(sitehelper_command_history_undo(&h,&p));
    assert(slab->definition.thickness_mm==100&&slab->definition.top_level_offset_mm==0);
    assert(sitehelper_command_history_redo(&h,&p));
    assert(slab->definition.thickness_mm==125&&slab->definition.top_level_offset_mm==-25);

    command=wrap_region(id,0,-60,90);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));sitehelper_command_destroy(&command);
    assert(slab->definition.regions.items[0].top_level_offset_mm==-60&&
        slab->definition.regions.items[0].thickness_mm==90);
    assert(sitehelper_command_history_undo(&h,&p));
    assert(slab->definition.regions.items[0].top_level_offset_mm==-10&&
        slab->definition.regions.items[0].thickness_mm==80);
    assert(sitehelper_command_history_redo(&h,&p));

    command=wrap_rebate(id,0,0,100,900,150,30);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));sitehelper_command_destroy(&command);
    assert(slab->definition.edge_rebates.items[0].start_offset_mm==100&&
        slab->definition.edge_rebates.items[0].end_offset_mm==900&&
        slab->definition.edge_rebates.items[0].width_mm==150&&
        slab->definition.edge_rebates.items[0].depth_mm==30);
    assert(sitehelper_command_history_undo(&h,&p));
    assert(slab->definition.edge_rebates.items[0].start_offset_mm==0&&
        slab->definition.edge_rebates.items[0].end_offset_mm==1000);
    assert(sitehelper_command_history_redo(&h,&p));

    /* Invalid dimensions are rejected without appending a history entry. */
    size_t count=h.count,cursor=h.cursor;
    command=wrap_slab(id,0,999);
    assert(!sitehelper_command_history_execute(&h,&p,&command,&result));
    sitehelper_command_destroy(&command);
    assert(h.count==count&&h.cursor==cursor&&slab->definition.thickness_mm==125);

    sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
}

static void test_region_index_conflict(void)
{
    SiteHelperProject p;DomainId id=fixture(&p);Slab *slab=sitehelper_project_find_slab_by_id(&p,id);
    assert(slab_add_region(slab,a,4,-10,80)==SLAB_SUCCESS);
    assert(slab_add_region(slab,b,4,-20,70)==SLAB_SUCCESS);
    SiteHelperCommandHistory h;sitehelper_command_history_init(&h);SiteHelperCommandResult result;
    SiteHelperCommand command=wrap_region(id,0,-30,60);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));sitehelper_command_destroy(&command);
    assert(sitehelper_command_history_undo(&h,&p));
    /* Same count/index/scalars but a different polygon must not be treated as the old feature. */
    assert(slab_remove_region(slab,0)==SLAB_SUCCESS);
    assert(slab_insert_region_at(slab,0,c,4,-10,80)==SLAB_SUCCESS);
    assert(!sitehelper_command_history_redo(&h,&p));
    assert(h.cursor==0&&slab->definition.regions.items[0].outline.vertices[0].x==6000);
    sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
}


static void test_edit_conflicts(void)
{
    SiteHelperProject p;DomainId id=fixture(&p);Slab *slab=sitehelper_project_find_slab_by_id(&p,id);
    SiteHelperCommandHistory h;sitehelper_command_history_init(&h);SiteHelperCommandResult result;

    SiteHelperCommand command=wrap_slab(id,125,-25);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));sitehelper_command_destroy(&command);
    assert(sitehelper_command_history_undo(&h,&p));
    assert(slab_set_base_properties(slab,110,-5)==SLAB_SUCCESS);
    assert(!sitehelper_command_history_redo(&h,&p));
    assert(h.cursor==0&&slab->definition.thickness_mm==110&&
        slab->definition.top_level_offset_mm==-5);
    sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);

    id=fixture(&p);slab=sitehelper_project_find_slab_by_id(&p,id);
    assert(slab_add_edge_rebate(slab,0,0,1000,100,20)==SLAB_SUCCESS);
    sitehelper_command_history_init(&h);
    command=wrap_rebate(id,0,0,100,900,150,30);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));sitehelper_command_destroy(&command);
    assert(sitehelper_command_history_undo(&h,&p));
    assert(slab_set_edge_rebate_properties(slab,0,2000,3000,100,20)==SLAB_SUCCESS);
    assert(!sitehelper_command_history_redo(&h,&p));
    assert(h.cursor==0&&slab->definition.edge_rebates.items[0].start_offset_mm==2000&&
        slab->definition.edge_rebates.items[0].end_offset_mm==3000);
    sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_scalar_capture_allocation_failures(void)
{
    size_t base_failures=0,rebate_failures=0;
    for(size_t point=0;point<6;point++){
        fail_after=SIZE_MAX;SiteHelperProject p;DomainId id=fixture(&p);
        Slab *slab=sitehelper_project_find_slab_by_id(&p,id);
        SiteHelperCommandHistory h;sitehelper_command_history_init(&h);
        SiteHelperCommand command=wrap_slab(id,125,-25);SiteHelperCommandResult result;
        fail_after=point;int ok=sitehelper_command_history_execute(&h,&p,&command,&result);fail_after=SIZE_MAX;
        if(!ok){
            base_failures++;assert(h.count==0&&h.cursor==0&&slab->definition.thickness_mm==100);
            assert(sitehelper_command_history_execute(&h,&p,&command,&result));
        }
        sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
        if(ok)break;
    }
    for(size_t point=0;point<6;point++){
        fail_after=SIZE_MAX;SiteHelperProject p;DomainId id=fixture(&p);
        Slab *slab=sitehelper_project_find_slab_by_id(&p,id);
        assert(slab_add_edge_rebate(slab,0,0,1000,100,20)==SLAB_SUCCESS);
        SiteHelperCommandHistory h;sitehelper_command_history_init(&h);
        SiteHelperCommand command=wrap_rebate(id,0,0,100,900,150,30);SiteHelperCommandResult result;
        fail_after=point;int ok=sitehelper_command_history_execute(&h,&p,&command,&result);fail_after=SIZE_MAX;
        if(!ok){
            rebate_failures++;assert(h.count==0&&h.cursor==0&&
                slab->definition.edge_rebates.items[0].start_offset_mm==0);
            assert(sitehelper_command_history_execute(&h,&p,&command,&result));
        }
        sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
        if(ok)break;
    }
    assert(base_failures==2&&rebate_failures==2);
}

static void test_region_capture_allocation_failures(void)
{
    size_t failures=0;
    for(size_t point=0;point<8;point++){
        fail_after=SIZE_MAX;SiteHelperProject p;DomainId id=fixture(&p);
        Slab *slab=sitehelper_project_find_slab_by_id(&p,id);
        assert(slab_add_region(slab,a,4,-10,80)==SLAB_SUCCESS);
        SiteHelperCommandHistory h;sitehelper_command_history_init(&h);
        SiteHelperCommand command=wrap_region(id,0,-30,60);SiteHelperCommandResult result;
        fail_after=point;int ok=sitehelper_command_history_execute(&h,&p,&command,&result);fail_after=SIZE_MAX;
        if(!ok){
            failures++;assert(h.count==0&&h.cursor==0);
            assert(slab->definition.regions.items[0].top_level_offset_mm==-10&&
                slab->definition.regions.items[0].thickness_mm==80);
            assert(sitehelper_command_history_execute(&h,&p,&command,&result));
        }
        sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
        if(ok)break;
    }
    assert(failures==3); /* history reserve, undo-state allocation, region outline snapshot */
}
#endif

int main(void)
{
    test_lifecycles();test_region_index_conflict();test_edit_conflicts();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_scalar_capture_allocation_failures();
    test_region_capture_allocation_failures();
#endif
    return 0;
}
