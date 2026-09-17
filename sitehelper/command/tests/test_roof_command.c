#include <assert.h>
#include <stdio.h>

#include "command_history.h"
#include "test_support.h"
#include "roof.h"

static const PlanPosition main_support[]={{0,0},{12000,0},{12000,8000},{0,8000}};
static const PlanPosition wing_support[]={{4000,4000},{8000,4000},{8000,11000},{4000,11000}};
static const PlanPosition small_support[]={{0,0},{8000,0},{8000,6000},{0,6000}};

static RoofPortionSpec gable(const PlanPosition *support, int64_t slope, RoofDirection direction)
{
    return (RoofPortionSpec){support,4,ROOF_PORTION_OPPOSING_SLOPES,slope,0,direction,
        ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE};
}

static SiteHelperCommand make_create(DomainId storey_id)
{
    RoofPortionSpec spec=gable(main_support,414214,(RoofDirection){1,0});
    CreateRoofCommand create={0}; SiteHelperCommand command={0};
    assert(create_roof_command_create(storey_id,&spec,&create));
    assert(sitehelper_command_from_create_roof(&create,&command));
    create_roof_command_destroy(&create);
    return command;
}

static void test_create_history_exact_identity(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    sitehelper_command_history_init(&h); SiteHelperCommand command=make_create(storey);
    DomainId expected_roof=p.domain_ids.next, expected_portion=expected_roof+1;
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(result.type==SITEHELPER_COMMAND_CREATE_ROOF);
    assert(result.data.roof.roof_id==expected_roof&&result.data.roof.portion_id==expected_portion);
    const Roof *roof=sitehelper_project_find_roof_by_id_const(&p,expected_roof);
    assert(roof&&roof->definition.portion_count==1&&roof->definition.portions[0].id==expected_portion);
    assert(roof->definition.portions[0].support_vertices!=command.data.create_roof.support_vertices);
    DomainId watermark=p.domain_ids.next;

    /* History owns a deep clone; caller command may die immediately. */
    sitehelper_command_destroy(&command);
    for(int i=0;i<3;i++){
        assert(sitehelper_command_history_undo(&h,&p));
        assert(!sitehelper_project_find_roof_by_id(&p,expected_roof));
        assert(!sitehelper_project_contains_domain_id(&p,expected_portion));
        assert(p.domain_ids.next==watermark); /* Undo never rewinds global identity. */
        assert(sitehelper_command_history_redo(&h,&p));
        roof=sitehelper_project_find_roof_by_id_const(&p,expected_roof);
        assert(roof&&roof->definition.portions[0].id==expected_portion);
        assert(p.domain_ids.next==watermark);
    }
    assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);
    sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

static DomainId add_compound_roof(SiteHelperProject *p, DomainId storey, DomainId *a, DomainId *b)
{
    RoofPortionSpec main=gable(main_support,414214,(RoofDirection){1,0});
    DomainId roof=sitehelper_project_add_roof(p,storey,&main,a); assert(roof&&*a);
    RoofPortionSpec wing=gable(wing_support,577350,(RoofDirection){0,1});
    *b=sitehelper_project_add_roof_portion_composed(p,roof,&wing,*a,ROOF_COMPOSITION_INTERSECTS);
    assert(*b); return roof;
}

static void test_delete_restores_full_roof_and_order(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    RoofPortionSpec first_spec=gable(small_support,176327,(RoofDirection){0,1});
    DomainId first_portion; DomainId first_roof=sitehelper_project_add_roof(&p,storey,&first_spec,&first_portion);
    assert(first_roof&&first_portion);
    DomainId a,b; DomainId target=add_compound_roof(&p,storey,&a,&b);
    RoofPortionSpec third_spec=gable(small_support,267949,(RoofDirection){1,0});
    DomainId third_portion; DomainId third_roof=sitehelper_project_add_roof(&p,storey,&third_spec,&third_portion);
    assert(third_roof&&third_portion&&p.storeys[0].roofs.count==3);

    Roof expected={0}; assert(roof_clone(sitehelper_project_find_roof_by_id_const(&p,target),&expected)==ROOF_SUCCESS);
    DomainId watermark=p.domain_ids.next;
    DeleteRoofCommand deletion; SiteHelperCommand command={0};
    assert(delete_roof_command_create(target,&deletion));
    assert(sitehelper_command_from_delete_roof(&deletion,&command));
    sitehelper_command_history_init(&h);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(result.type==SITEHELPER_COMMAND_DELETE_ROOF&&result.data.roof.roof_id==target);
    assert(!sitehelper_project_find_roof_by_id(&p,target)&&p.storeys[0].roofs.count==2);
    assert(p.storeys[0].roofs.items[0].id==first_roof&&p.storeys[0].roofs.items[1].id==third_roof);

    assert(sitehelper_command_history_undo(&h,&p));
    assert(p.storeys[0].roofs.count==3&&p.storeys[0].roofs.items[1].id==target);
    test_assert_roof_equal(&expected,sitehelper_project_find_roof_by_id_const(&p,target));
    assert(sitehelper_project_find_roof_portion_by_id_const(&p,a));
    assert(sitehelper_project_find_roof_portion_by_id_const(&p,b));
    assert(p.domain_ids.next==watermark);
    RoofPrototypeGeometry geometry={0};
    assert(roof_build_derived_geometry(sitehelper_project_find_roof_by_id_const(&p,target),&geometry)==ROOF_SUCCESS);
    assert(geometry.plane_count==4); roof_prototype_geometry_destroy(&geometry);

    assert(sitehelper_command_history_redo(&h,&p));
    assert(!sitehelper_project_find_roof_by_id(&p,target));
    assert(sitehelper_command_history_undo(&h,&p));
    test_assert_roof_equal(&expected,sitehelper_project_find_roof_by_id_const(&p,target));
    assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);

    roof_destroy(&expected); sitehelper_command_destroy(&command);
    sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

static void test_delete_undo_refuses_identity_collision(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    DomainId a,b; DomainId roof_id=add_compound_roof(&p,storey,&a,&b);
    DeleteRoofCommand deletion; SiteHelperCommand command={0};
    assert(delete_roof_command_create(roof_id,&deletion));
    assert(sitehelper_command_from_delete_roof(&deletion,&command));
    sitehelper_command_history_init(&h);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(h.cursor==1&&!sitehelper_project_find_roof_by_id(&p,roof_id));

    /* External malformed state claims one of the nested identities. History must
     * fail closed rather than overwrite it or move the cursor. */
    DomainId room=sitehelper_project_add_room(&p,storey); assert(room);
    Room *claimed=sitehelper_project_find_room_by_id(&p,room); assert(claimed);
    DomainId original_room_id=claimed->id; claimed->id=b;
    assert(!sitehelper_command_history_undo(&h,&p));
    assert(h.cursor==1&&!sitehelper_project_find_roof_by_id(&p,roof_id));
    claimed->id=original_room_id;
    assert(sitehelper_command_history_undo(&h,&p));
    assert(h.cursor==0&&sitehelper_project_find_roof_by_id(&p,roof_id));

    sitehelper_command_destroy(&command); sitehelper_command_history_destroy(&h);
    sitehelper_project_destroy(&p);
}

static void test_failed_create_preserves_history_and_ids(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); assert(sitehelper_project_add_storey(&p,0));
    sitehelper_command_history_init(&h); SiteHelperCommand command=make_create(99999);
    DomainId watermark=p.domain_ids.next;
    assert(!sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(result.type==SITEHELPER_COMMAND_NONE&&h.count==0&&h.cursor==0);
    assert(p.domain_ids.next==watermark&&p.storeys[0].roofs.count==0);
    sitehelper_command_destroy(&command); sitehelper_command_history_destroy(&h);
    sitehelper_project_destroy(&p);
}

int main(void)
{
    test_create_history_exact_identity();
    test_delete_restores_full_roof_and_order();
    test_delete_undo_refuses_identity_collision();
    test_failed_create_preserves_history_and_ids();
    puts("roof commands/history: ok");
    return 0;
}
