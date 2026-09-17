#include <assert.h>
#include <stdio.h>

#include "command_history.h"
#include "roof.h"
#include "test_support.h"

static const PlanPosition main_support[]={{0,0},{12000,0},{12000,8000},{0,8000}};
static const PlanPosition wing_support[]={{4000,4000},{8000,4000},{8000,11000},{4000,11000}};

static RoofPortionSpec gable(const PlanPosition *support, int64_t slope, RoofDirection direction)
{
    return (RoofPortionSpec){support,4,ROOF_PORTION_OPPOSING_SLOPES,slope,0,direction,
        ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE};
}

static SiteHelperCommand wrap_edit(RoofSourceEditCommand *edit)
{
    SiteHelperCommand command={0};
    assert(sitehelper_command_from_roof_source_edit(edit,&command));
    roof_source_edit_command_destroy(edit);
    return command;
}

static DomainId add_compound(SiteHelperProject *p, DomainId storey, DomainId *a, DomainId *b)
{
    RoofPortionSpec main=gable(main_support,414214,(RoofDirection){1,0});
    DomainId roof=sitehelper_project_add_roof(p,storey,&main,a); assert(roof&&*a);
    RoofPortionSpec wing=gable(wing_support,414214,(RoofDirection){0,1});
    *b=sitehelper_project_add_roof_portion_composed(p,roof,&wing,*a,
        ROOF_COMPOSITION_INTERSECTS); assert(*b);
    return roof;
}

static void test_add_portion_undo_redo_reuses_identity(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    RoofPortionSpec main=gable(main_support,414214,(RoofDirection){1,0});
    DomainId a; DomainId roof_id=sitehelper_project_add_roof(&p,storey,&main,&a); assert(roof_id&&a);
    Roof before={0}; assert(roof_clone(sitehelper_project_find_roof_by_id_const(&p,roof_id),&before)==ROOF_SUCCESS);

    RoofPortionSpec wing=gable(wing_support,577350,(RoofDirection){0,1});
    RoofSourceEditCommand edit={0}; assert(roof_source_edit_command_create_add_portion(
        roof_id,a,ROOF_COMPOSITION_INTERSECTS,&wing,&edit));
    SiteHelperCommand command=wrap_edit(&edit); sitehelper_command_history_init(&h);
    DomainId expected_id=p.domain_ids.next;
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(result.type==SITEHELPER_COMMAND_EDIT_ROOF_SOURCE);
    assert(result.data.roof.roof_id==roof_id&&result.data.roof.portion_id==expected_id);
    assert(sitehelper_project_find_roof_portion_by_id_const(&p,expected_id));
    DomainId watermark=p.domain_ids.next;

    sitehelper_command_destroy(&command); /* History owns an independent deep clone. */
    assert(sitehelper_command_history_undo(&h,&p));
    test_assert_roof_equal(&before,sitehelper_project_find_roof_by_id_const(&p,roof_id));
    assert(!sitehelper_project_contains_domain_id(&p,expected_id));
    assert(p.domain_ids.next==watermark);
    assert(sitehelper_command_history_redo(&h,&p));
    const Roof *after=sitehelper_project_find_roof_by_id_const(&p,roof_id);
    assert(after&&after->definition.portion_count==2&&after->definition.portions[1].id==expected_id);
    assert(p.domain_ids.next==watermark);
    assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);

    roof_destroy(&before); sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

static void test_set_portion_and_divergence_fail_closed(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    DomainId a,b; DomainId roof_id=add_compound(&p,storey,&a,&b);
    Roof before={0}; assert(roof_clone(sitehelper_project_find_roof_by_id_const(&p,roof_id),&before)==ROOF_SUCCESS);
    RoofPortionSpec changed=gable(wing_support,577350,(RoofDirection){0,1});
    RoofSourceEditCommand edit={0}; assert(roof_source_edit_command_create_set_portion(
        roof_id,b,&changed,&edit)); SiteHelperCommand command=wrap_edit(&edit);
    sitehelper_command_history_init(&h); assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(sitehelper_project_find_roof_portion_by_id_const(&p,b)->slope_ppm==577350);

    Roof *current=sitehelper_project_find_roof_by_id(&p,roof_id); assert(current);
    current->definition.portions[1].reference_z_mm=1; /* External divergent mutation. */
    assert(!sitehelper_command_history_undo(&h,&p)); assert(h.cursor==1);
    current->definition.portions[1].reference_z_mm=0;
    assert(sitehelper_command_history_undo(&h,&p));
    test_assert_roof_equal(&before,sitehelper_project_find_roof_by_id_const(&p,roof_id));
    assert(sitehelper_command_history_redo(&h,&p));
    assert(sitehelper_project_find_roof_portion_by_id_const(&p,b)->slope_ppm==577350);

    roof_destroy(&before); sitehelper_command_destroy(&command);
    sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

static void test_remove_portion_restores_full_authority(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    DomainId a,b; DomainId roof_id=add_compound(&p,storey,&a,&b);
    Roof before={0}; assert(roof_clone(sitehelper_project_find_roof_by_id_const(&p,roof_id),&before)==ROOF_SUCCESS);
    RoofSourceEditCommand edit={0}; assert(roof_source_edit_command_create_remove_portion(
        roof_id,b,&edit)); SiteHelperCommand command=wrap_edit(&edit); sitehelper_command_history_init(&h);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    const Roof *roof=sitehelper_project_find_roof_by_id_const(&p,roof_id);
    assert(roof&&roof->definition.portion_count==1&&roof->definition.composition_count==0);
    assert(!sitehelper_project_contains_domain_id(&p,b)); DomainId watermark=p.domain_ids.next;
    assert(sitehelper_command_history_undo(&h,&p));
    test_assert_roof_equal(&before,sitehelper_project_find_roof_by_id_const(&p,roof_id));
    assert(sitehelper_project_contains_domain_id(&p,b)&&p.domain_ids.next==watermark);
    assert(sitehelper_command_history_redo(&h,&p));
    assert(!sitehelper_project_contains_domain_id(&p,b)&&p.domain_ids.next==watermark);

    roof_destroy(&before); sitehelper_command_destroy(&command);
    sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

static void test_composition_set_and_invalid_disconnect(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    DomainId a,b; DomainId roof_id=add_compound(&p,storey,&a,&b);
    RoofSourceEditCommand edit={0}; assert(roof_source_edit_command_create_set_composition(
        roof_id,(RoofComposition){b,a,ROOF_COMPOSITION_INTERSECTS},&edit));
    SiteHelperCommand command=wrap_edit(&edit); sitehelper_command_history_init(&h);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    const Roof *roof=sitehelper_project_find_roof_by_id_const(&p,roof_id);
    assert(roof->definition.composition_count==1);
    assert(roof->definition.compositions[0].first_portion_id==b);
    assert(sitehelper_command_history_undo(&h,&p));
    roof=sitehelper_project_find_roof_by_id_const(&p,roof_id);
    assert(roof->definition.compositions[0].first_portion_id==a);
    assert(sitehelper_command_history_redo(&h,&p));
    sitehelper_command_destroy(&command); sitehelper_command_history_destroy(&h);

    /* Removing the only relationship would leave unresolved compound authority;
     * regeneration rejects it transactionally and history remains untouched. */
    Roof before={0}; assert(roof_clone(sitehelper_project_find_roof_by_id_const(&p,roof_id),&before)==ROOF_SUCCESS);
    assert(roof_source_edit_command_create_remove_composition(roof_id,a,b,&edit));
    command=wrap_edit(&edit); sitehelper_command_history_init(&h); DomainId watermark=p.domain_ids.next;
    assert(!sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(h.count==0&&h.cursor==0&&p.domain_ids.next==watermark);
    test_assert_roof_equal(&before,sitehelper_project_find_roof_by_id_const(&p,roof_id));
    roof_destroy(&before); sitehelper_command_destroy(&command); sitehelper_command_history_destroy(&h);
    sitehelper_project_destroy(&p);
}

static void test_termination_set_update_remove_history(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    RoofPortionSpec main=gable(main_support,414214,(RoofDirection){1,0});
    DomainId portion; DomainId roof_id=sitehelper_project_add_roof(&p,storey,&main,&portion); assert(roof_id);
    sitehelper_command_history_init(&h);

    RoofSourceEditCommand edit={0}; assert(roof_source_edit_command_create_set_termination(
        roof_id,(RoofTermination){portion,ROOF_END_NEGATIVE_AXIS,2000},&edit));
    SiteHelperCommand command=wrap_edit(&edit); assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(sitehelper_project_find_roof_by_id_const(&p,roof_id)->definition.termination_count==1);
    sitehelper_command_destroy(&command);

    assert(roof_source_edit_command_create_set_termination(roof_id,
        (RoofTermination){portion,ROOF_END_NEGATIVE_AXIS,1500},&edit));
    command=wrap_edit(&edit); assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(sitehelper_project_find_roof_by_id_const(&p,roof_id)->definition.terminations[0].termination_offset_mm==1500);
    assert(sitehelper_command_history_undo(&h,&p));
    assert(sitehelper_project_find_roof_by_id_const(&p,roof_id)->definition.terminations[0].termination_offset_mm==2000);
    assert(sitehelper_command_history_redo(&h,&p));
    sitehelper_command_destroy(&command);

    assert(roof_source_edit_command_create_remove_termination(roof_id,portion,
        ROOF_END_NEGATIVE_AXIS,&edit)); command=wrap_edit(&edit);
    assert(sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(sitehelper_project_find_roof_by_id_const(&p,roof_id)->definition.termination_count==0);
    assert(sitehelper_command_history_undo(&h,&p));
    assert(sitehelper_project_find_roof_by_id_const(&p,roof_id)->definition.termination_count==1);
    assert(sitehelper_command_history_redo(&h,&p));
    assert(sitehelper_project_find_roof_by_id_const(&p,roof_id)->definition.termination_count==0);

    sitehelper_command_destroy(&command); sitehelper_command_history_destroy(&h);
    sitehelper_project_destroy(&p);
}

static void test_invalid_portion_edit_is_transactional(void)
{
    SiteHelperProject p; SiteHelperCommandHistory h; SiteHelperCommandResult result;
    sitehelper_project_init(&p); DomainId storey=sitehelper_project_add_storey(&p,0);
    RoofPortionSpec main=gable(main_support,414214,(RoofDirection){1,0});
    DomainId portion; DomainId roof_id=sitehelper_project_add_roof(&p,storey,&main,&portion); assert(roof_id);
    Roof before={0}; assert(roof_clone(sitehelper_project_find_roof_by_id_const(&p,roof_id),&before)==ROOF_SUCCESS);
    RoofPortionSpec invalid=gable(main_support,0,(RoofDirection){1,0});
    RoofSourceEditCommand edit={0}; assert(roof_source_edit_command_create_set_portion(
        roof_id,portion,&invalid,&edit)); SiteHelperCommand command=wrap_edit(&edit);
    sitehelper_command_history_init(&h); DomainId watermark=p.domain_ids.next;
    assert(!sitehelper_command_history_execute(&h,&p,&command,&result));
    assert(h.count==0&&h.cursor==0&&p.domain_ids.next==watermark);
    test_assert_roof_equal(&before,sitehelper_project_find_roof_by_id_const(&p,roof_id));

    roof_destroy(&before); sitehelper_command_destroy(&command);
    sitehelper_command_history_destroy(&h); sitehelper_project_destroy(&p);
}

int main(void)
{
    test_add_portion_undo_redo_reuses_identity();
    test_set_portion_and_divergence_fail_closed();
    test_remove_portion_restores_full_authority();
    test_composition_set_and_invalid_disconnect();
    test_termination_set_update_remove_history();
    test_invalid_portion_edit_is_transactional();
    puts("roof source-edit commands/history: ok");
    return 0;
}
