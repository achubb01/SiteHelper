#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "command_history.h"
#include "slab.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
void *__real_malloc(size_t); void *__real_calloc(size_t,size_t); void *__real_realloc(void*,size_t);
static int fail_now(void){return fail_after!=SIZE_MAX&&fail_after--==0;}
void *__wrap_malloc(size_t n){return fail_now()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){return fail_now()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void *p,size_t n){return fail_now()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition hole[]={{1000,1000},{2500,1000},{2500,2500},{1000,2500}};
static const PlanPosition region[]={{5000,1000},{7000,1000},{7000,3000},{5000,3000}};

typedef struct {
    SiteHelperProject project;
    SiteHelperCommandHistory history;
    DomainId slab_id;
} Fixture;

static void fixture_init(Fixture *f)
{
    sitehelper_project_init(&f->project);
    DomainId storey=sitehelper_project_add_storey(&f->project,0);
    f->slab_id=sitehelper_project_add_slab(&f->project,storey,outer,4,100,0);
    assert(f->slab_id!=DOMAIN_ID_INVALID);
    Slab *slab=sitehelper_project_find_slab_by_id(&f->project,f->slab_id);
    assert(slab_add_penetration(slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(slab,region,4,-20,80)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,0,7000,100,20)==SLAB_SUCCESS);
    sitehelper_command_history_init(&f->history);
}

static void fixture_destroy(Fixture *f)
{
    sitehelper_command_history_destroy(&f->history);
    sitehelper_project_destroy(&f->project);
}

static SiteHelperCommand move_command(DomainId slab_id, MoveSlabVertexTarget target,
    size_t feature, size_t vertex, PlanPosition position)
{
    MoveSlabVertexCommand move; SiteHelperCommand command={0};
    assert(move_slab_vertex_command_create(slab_id,target,feature,vertex,position,&move));
    assert(sitehelper_command_from_move_slab_vertex(&move,&command));
    return command;
}

static void execute_move(Fixture *f, SiteHelperCommand *command,
    SiteHelperCommandResult *result)
{
    assert(sitehelper_command_history_execute(&f->history,&f->project,command,result));
    assert(result->type==SITEHELPER_COMMAND_MOVE_SLAB_VERTEX);
}

static void test_lifecycles(void)
{
    Fixture f; fixture_init(&f); SiteHelperCommandResult result;
    Slab *slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    DomainId next=f.project.domain_ids.next;

    SiteHelperCommand outer_move=move_command(f.slab_id,MOVE_SLAB_VERTEX_OUTLINE,SIZE_MAX,1,
        (PlanPosition){11000,0});
    execute_move(&f,&outer_move,&result);
    assert(result.data.slab_vertex.slab_id==f.slab_id&&
        result.data.slab_vertex.feature_index==SIZE_MAX&&result.data.slab_vertex.vertex_index==1);
    assert(slab->definition.outline.vertices[1].x==11000&&f.project.domain_ids.next==next);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(slab->definition.outline.vertices[1].x==10000);
    assert(sitehelper_command_history_redo(&f.history,&f.project));
    assert(slab->definition.outline.vertices[1].x==11000);
    sitehelper_command_destroy(&outer_move);

    SiteHelperCommand pen_move=move_command(f.slab_id,MOVE_SLAB_VERTEX_PENETRATION,0,1,
        (PlanPosition){3000,1000});
    execute_move(&f,&pen_move,&result);
    assert(slab->definition.penetrations.items[0].outline.vertices[1].x==3000);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(slab->definition.penetrations.items[0].outline.vertices[1].x==2500);
    assert(sitehelper_command_history_redo(&f.history,&f.project));
    assert(slab->definition.penetrations.items[0].outline.vertices[1].x==3000);
    sitehelper_command_destroy(&pen_move);

    SiteHelperCommand reg_move=move_command(f.slab_id,MOVE_SLAB_VERTEX_REGION,0,2,
        (PlanPosition){7500,3000});
    execute_move(&f,&reg_move,&result);
    assert(slab->definition.regions.items[0].outline.vertices[2].x==7500);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(slab->definition.regions.items[0].outline.vertices[2].x==7000);
    assert(sitehelper_command_history_redo(&f.history,&f.project));
    assert(slab->definition.regions.items[0].outline.vertices[2].x==7500);
    sitehelper_command_destroy(&reg_move);

    fixture_destroy(&f);
}

static void test_validation_and_conflicts(void)
{
    Fixture f; fixture_init(&f); SiteHelperCommandResult result;
    Slab *slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    SiteHelperCommand invalid=move_command(f.slab_id,MOVE_SLAB_VERTEX_OUTLINE,SIZE_MAX,1,
        (PlanPosition){6000,0});
    assert(!sitehelper_command_history_execute(&f.history,&f.project,&invalid,&result));
    assert(f.history.count==0&&slab->definition.outline.vertices[1].x==10000);
    sitehelper_command_destroy(&invalid);

    SiteHelperCommand move=move_command(f.slab_id,MOVE_SLAB_VERTEX_REGION,0,2,
        (PlanPosition){7500,3000});
    execute_move(&f,&move,&result);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(slab_set_region_vertex(slab,0,1,(PlanPosition){6800,1000})==SLAB_SUCCESS);
    assert(!sitehelper_command_history_redo(&f.history,&f.project));
    assert(f.history.cursor==0&&slab->definition.regions.items[0].outline.vertices[1].x==6800);
    sitehelper_command_destroy(&move);
    fixture_destroy(&f);

    /* A region index with changed scalar properties is not the same historical
     * target even when its outline still matches the pre-move geometry. */
    fixture_init(&f); slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    move=move_command(f.slab_id,MOVE_SLAB_VERTEX_REGION,0,2,(PlanPosition){7500,3000});
    execute_move(&f,&move,&result);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(slab_set_region_properties(slab,0,-30,80)==SLAB_SUCCESS);
    assert(!sitehelper_command_history_redo(&f.history,&f.project));
    assert(f.history.cursor==0&&slab->definition.regions.items[0].top_level_offset_mm==-30);
    sitehelper_command_destroy(&move);
    fixture_destroy(&f);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    size_t failures=0;
    for(size_t point=0;point<8;point++){
        fail_after=SIZE_MAX;
        Fixture f; fixture_init(&f); SiteHelperProject before;
        test_clone_project_authoritative(&f.project,&before);
        SiteHelperCommand command=move_command(f.slab_id,MOVE_SLAB_VERTEX_OUTLINE,SIZE_MAX,1,
            (PlanPosition){11000,0});
        SiteHelperCommandResult result; fail_after=point;
        int ok=sitehelper_command_history_execute(&f.history,&f.project,&command,&result);
        fail_after=SIZE_MAX;
        if(!ok){
            failures++;
            test_assert_project_authoritative_equal(&before,&f.project);
            assert(f.history.count==0&&f.history.cursor==0);
            assert(sitehelper_command_history_execute(&f.history,&f.project,&command,&result));
        }
        sitehelper_project_destroy(&before); sitehelper_command_destroy(&command);
        fixture_destroy(&f);
        if(ok) break;
    }
    assert(failures==3);
}
#endif

int main(void)
{
    test_lifecycles();
    test_validation_and_conflicts();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    return 0;
}
