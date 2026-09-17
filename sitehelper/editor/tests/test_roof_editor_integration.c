#include <assert.h>

#include "sitehelper_editor.h"

static RoofPortionSpec rectangle(PlanPosition *v)
{
    v[0]=(PlanPosition){0,0};v[1]=(PlanPosition){10000,0};
    v[2]=(PlanPosition){10000,8000};v[3]=(PlanPosition){0,8000};
    return (RoofPortionSpec){v,4,ROOF_PORTION_OPPOSING_SLOPES,414214,0,{1,0},
        ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE};
}

int main(void)
{
    SiteHelperProject p;sitehelper_project_init(&p);
    DomainId storey_id=sitehelper_project_add_storey(&p,0);
    PlanPosition v[4];DomainId portion_id;
    RoofPortionSpec spec=rectangle(v);
    DomainId roof_id=sitehelper_project_add_roof(&p,storey_id,&spec,&portion_id);
    assert(roof_id != DOMAIN_ID_INVALID && portion_id != DOMAIN_ID_INVALID);
    const Storey *storey=sitehelper_project_find_storey_by_id_const(&p,storey_id);

    SiteHelperEditor e;sitehelper_editor_init(&e);
    assert(sitehelper_editor_set_current_storey(&e,&p,storey_id));
    assert(sitehelper_editor_select_roof_at_position(&e,storey,(PlanPoint){5000,4000},0.0));
    assert(e.selection.kind==EDITOR_SELECTION_ROOF_PORTION);
    assert(e.selection.roof_id==roof_id && e.selection.roof_portion_id==portion_id);

    /* Reconciliation preserves a live ID-only roof selection. */
    sitehelper_editor_reconcile(&e,&p);
    assert(e.selection.kind==EDITOR_SELECTION_ROOF_PORTION);

    assert(sitehelper_project_remove_roof_by_id(&p,roof_id));
    sitehelper_editor_reconcile(&e,&p);
    assert(editor_selection_is_empty(&e.selection));

    /* Invalid/missing hit clears rather than leaving a stale selection. */
    storey=sitehelper_project_find_storey_by_id_const(&p,storey_id);
    assert(sitehelper_editor_select_roof_at_position(&e,storey,(PlanPoint){5000,4000},0.0));
    assert(editor_selection_is_empty(&e.selection));

    sitehelper_editor_destroy(&e);
    sitehelper_project_destroy(&p);
    return 0;
}
