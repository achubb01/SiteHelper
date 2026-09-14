#include <assert.h>
#include <stdlib.h>
#include "command_history.h"
#include "test_support.h"
#include "slab.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
void *__real_malloc(size_t); void *__real_calloc(size_t,size_t); void *__real_realloc(void*,size_t);
static int fail(void){return fail_after!=SIZE_MAX&&fail_after--==0;}
void *__wrap_malloc(size_t n){return fail()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){return fail()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void*p,size_t n){return fail()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition rectangle[]={{0,0},{10000,0},{10000,8000},{0,8000}};

static void add_complete_slab_state(Slab *slab)
{
    PlanPosition hole[]={{4000,3000},{5000,3000},{5000,4000},{4000,4000}};
    PlanPosition region[]={{0,0},{3000,0},{3000,2000},{0,2000}};
    assert(slab_add_penetration(slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(slab,region,4,-50,75)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,0,2500,110,20)==SLAB_SUCCESS);
}

static SiteHelperCommand create_command(DomainId storey)
{
    CreateSlabCommand create={0}; SiteHelperCommand command={0};
    assert(create_slab_command_create(storey,rectangle,4,100,-25,&create));
    assert(sitehelper_command_from_create_slab(&create,&command));
    create_slab_command_destroy(&create); return command;
}

static void test_create_history(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,3000);
    sitehelper_command_history_init(&h); SiteHelperCommand command=create_command(storey);
    DomainId expected=p.domain_ids.next;
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(result.type==SITEHELPER_COMMAND_CREATE_SLAB&&result.data.slab.slab_id==expected);
    const Slab *slab=sitehelper_project_find_slab_by_id_const(&p,expected);
    assert(slab&&slab->definition.thickness_mm==100&&slab->definition.top_level_offset_mm==-25);
    assert(slab->definition.outline.vertices!=command.data.create_slab.vertices);
    /* History owns an independent clone after execute. */
    sitehelper_command_destroy(&command);
    DomainId next=p.domain_ids.next;
    for(int i=0;i<3;i++){
        assert(sitehelper_command_history_undo(&h,&p)); assert(!sitehelper_project_find_slab_by_id(&p,expected));
        assert(p.domain_ids.next==next); assert(sitehelper_command_history_redo(&h,&p));
        assert(sitehelper_project_find_slab_by_id(&p,expected));
    }
    SiteHelperCommand invalid=create_command(9999);
    assert(!sitehelper_command_history_execute(&h,&p,&invalid,&result));
    assert(p.storeys[0].slabs.count==1&&p.domain_ids.next==next);
    sitehelper_command_destroy(&invalid);
    sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

static void test_owning_redo_branch_destruction(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    sitehelper_command_history_init(&h);

    SiteHelperCommand first=create_command(storey);
    assert(sitehelper_command_history_execute(&h,&p,&first,&result));
    sitehelper_command_destroy(&first);
    assert(sitehelper_command_history_undo(&h,&p));

    /* Executing after undo destroys the obsolete owning CREATE_SLAB entry. */
    SiteHelperCommand replacement=create_command(storey);
    assert(sitehelper_command_history_execute(&h,&p,&replacement,&result));
    sitehelper_command_destroy(&replacement);
    assert(h.count==1&&h.cursor==1&&p.storeys[0].slabs.count==1);

    sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

static void test_delete_complete_snapshot(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    DomainId id=sitehelper_project_add_slab(&p,storey,rectangle,4,100,0);
    Slab *slab=sitehelper_project_find_slab_by_id(&p,id);
    add_complete_slab_state(slab);
    Slab expected={0}; assert(slab_clone(slab,&expected)==SLAB_SUCCESS);
    DeleteSlabCommand deletion; SiteHelperCommand command={0};
    assert(delete_slab_command_create(id,&deletion));
    assert(sitehelper_command_from_delete_slab(&deletion,&command));
    sitehelper_command_history_init(&h);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(!sitehelper_project_find_slab_by_id(&p,id));
    assert(sitehelper_command_history_undo(&h,&p));
    test_assert_slab_equal(&expected,sitehelper_project_find_slab_by_id_const(&p,id));
    assert(sitehelper_command_history_redo(&h,&p)); assert(!sitehelper_project_find_slab_by_id(&p,id));
    Slab collision={0}; assert(slab_build(id,rectangle,4,100,0,&collision)==SLAB_SUCCESS);
    assert(sitehelper_project_insert_slab(&p,storey,&collision));
    /* A conflicting external mutation cannot be overwritten by delete undo. */
    assert(!sitehelper_command_history_undo(&h,&p));
    assert(sitehelper_project_find_slab_by_id(&p,id));
    slab_destroy(&collision); slab_destroy(&expected); sitehelper_command_destroy(&command);
    sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

static void test_delete_rejects_ambiguous_owner(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p);
    assert(sitehelper_project_add_storey(&p,0)==1);
    DomainId owner=sitehelper_project_add_storey(&p,3000);
    DomainId id=sitehelper_project_add_slab(&p,owner,rectangle,4,100,0);
    assert(id!=DOMAIN_ID_INVALID);
    p.storeys[0].id=id; /* Deliberately malformed cross-Storey ID collision. */

    DeleteSlabCommand deletion; SiteHelperCommand command={0};
    assert(delete_slab_command_create(id,&deletion));
    assert(sitehelper_command_from_delete_slab(&deletion,&command));
    sitehelper_command_history_init(&h);
    assert(!sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(h.count==0&&h.cursor==0&&p.storeys[1].slabs.count==1);

    sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&h);
    sitehelper_project_destroy(&p);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    size_t constructor_failures=0;
    for(size_t point=0;point<8;point++){
        fail_after=SIZE_MAX;
        CreateSlabCommand command={0};
        assert(create_slab_command_create(7,rectangle,4,100,-25,&command));
        PlanPosition *previous_vertices=command.vertices;
        fail_after=point;
        int ok=create_slab_command_create(8,rectangle,4,125,-40,&command);
        fail_after=SIZE_MAX;
        if(!ok){
            constructor_failures++;
            assert(command.vertices==previous_vertices&&command.storey_id==7&&
                command.thickness_mm==100&&command.top_level_offset_mm==-25);
            assert(create_slab_command_create(8,rectangle,4,125,-40,&command));
        }
        assert(command.storey_id==8&&command.thickness_mm==125&&
            command.top_level_offset_mm==-40);
        create_slab_command_destroy(&command);
        if(ok) break;
    }
    assert(constructor_failures==1);

    size_t create_history_failures=0;
    for(size_t point=0;point<20;point++){
        fail_after=SIZE_MAX;
        SiteHelperProject p,before; SiteHelperCommandHistory h;
        sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
        SiteHelperCommand command=create_command(storey); sitehelper_command_history_init(&h);
        test_clone_project_authoritative(&p,&before);
        fail_after=point; SiteHelperCommandResult result;
        int ok=sitehelper_command_history_execute(&h,&p,&command,&result);
        fail_after=SIZE_MAX;
        if(!ok){create_history_failures++;test_assert_project_authoritative_equal(&before,&p);assert(h.count==0&&h.cursor==0);
            assert(sitehelper_command_history_execute(&h,&p,&command,&result));}
        sitehelper_project_destroy(&before);sitehelper_command_destroy(&command);
        sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
        if(ok) break;
    }
    assert(create_history_failures==4);
    size_t delete_execute_failures=0;
    for(size_t point=0;point<24;point++){
        fail_after=SIZE_MAX;
        SiteHelperProject p,before;SiteHelperCommandHistory h;SiteHelperCommandResult result;
        sitehelper_project_init(&p);DomainId storey=sitehelper_project_add_storey(&p,0);
        DomainId id=sitehelper_project_add_slab(&p,storey,rectangle,4,100,0);
        add_complete_slab_state(sitehelper_project_find_slab_by_id(&p,id));
        DeleteSlabCommand deletion;SiteHelperCommand command={0};
        assert(delete_slab_command_create(id,&deletion));assert(sitehelper_command_from_delete_slab(&deletion,&command));
        sitehelper_command_history_init(&h);test_clone_project_authoritative(&p,&before);
        fail_after=point;int ok=sitehelper_command_history_execute(&h,&p,&command,&result);fail_after=SIZE_MAX;
        if(!ok){delete_execute_failures++;test_assert_project_authoritative_equal(&before,&p);assert(h.count==0);
            assert(sitehelper_command_history_execute(&h,&p,&command,&result));}
        assert(!sitehelper_project_find_slab_by_id(&p,id));
        sitehelper_project_destroy(&before);sitehelper_command_destroy(&command);
        sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
        if(ok) break;
    }
    assert(delete_execute_failures==8);
    size_t delete_undo_failures=0;
    for(size_t point=0;point<24;point++){
        fail_after=SIZE_MAX;
        SiteHelperProject p;SiteHelperCommandHistory h;SiteHelperCommandResult result;
        sitehelper_project_init(&p);DomainId storey=sitehelper_project_add_storey(&p,0);
        DomainId id=sitehelper_project_add_slab(&p,storey,rectangle,4,100,0);
        add_complete_slab_state(sitehelper_project_find_slab_by_id(&p,id));
        DeleteSlabCommand deletion;SiteHelperCommand command={0};
        assert(delete_slab_command_create(id,&deletion));assert(sitehelper_command_from_delete_slab(&deletion,&command));
        sitehelper_command_history_init(&h);assert(sitehelper_command_history_execute(&h,&p,&command,&result));
        fail_after=point;int ok=sitehelper_command_history_undo(&h,&p);fail_after=SIZE_MAX;
        if(!ok){delete_undo_failures++;assert(!sitehelper_project_find_slab_by_id(&p,id)&&h.cursor==1);
            assert(sitehelper_command_history_undo(&h,&p));}
        assert(sitehelper_project_find_slab_by_id(&p,id)&&h.cursor==0);
        sitehelper_command_destroy(&command);sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
        if(ok) break;
    }
    assert(delete_undo_failures==6);
}
#endif

int main(void){test_create_history();test_owning_redo_branch_destruction();
test_delete_complete_snapshot();test_delete_rejects_ambiguous_owner();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
test_allocation_failures();
#endif
return 0;}
