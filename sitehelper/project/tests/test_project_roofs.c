#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_project.h"
#include "sitehelper_persistence.h"
#include "roof.h"

static const PlanPosition main_support[]={{0,0},{12000,0},{12000,8000},{0,8000}};
static const PlanPosition wing_support[]={{4000,4000},{8000,4000},{8000,11000},{4000,11000}};

static RoofPortionSpec gable(const PlanPosition *support, RoofDirection direction, int64_t slope)
{
    return (RoofPortionSpec){support,4,ROOF_PORTION_OPPOSING_SLOPES,slope,0,direction,ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE};
}

int main(void)
{
    SiteHelperProject p; sitehelper_project_init(&p);
    DomainId lower=sitehelper_project_add_storey(&p,0), upper=sitehelper_project_add_storey(&p,2700);
    assert(lower && upper);
    DomainId first=0; RoofPortionSpec main=gable(main_support,(RoofDirection){1,0},414214);
    DomainId roof=sitehelper_project_add_roof(&p,upper,&main,&first);
    assert(roof && first && roof != first);
    assert(sitehelper_project_find_roof_by_id(&p,roof));
    assert(sitehelper_project_find_roof_portion_by_id(&p,first));
    assert(sitehelper_project_find_owning_storey(&p,roof)->id==upper);
    assert(sitehelper_project_find_owning_storey(&p,first)->id==upper);
    assert(sitehelper_project_contains_domain_id(&p,roof) && sitehelper_project_contains_domain_id(&p,first));

    RoofPortionSpec wing=gable(wing_support,(RoofDirection){0,1},577350);
    DomainId second=sitehelper_project_add_roof_portion_composed(&p,roof,&wing,first,
        ROOF_COMPOSITION_INTERSECTS); assert(second);
    assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);
    /* Roof and portion identities participate in the same global namespace. */
    Roof *owned=sitehelper_project_find_roof_by_id(&p,roof); assert(owned);
    DomainId saved_second=owned->definition.portions[1].id;
    owned->definition.portions[1].id=roof;
    assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_DUPLICATE_ID);
    owned->definition.portions[1].id=saved_second;
    assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);
    RoofPrototypeGeometry g={0};
    assert(roof_build_derived_geometry(sitehelper_project_find_roof_by_id(&p,roof),&g)==ROOF_SUCCESS);
    assert(g.plane_count==4); roof_prototype_geometry_destroy(&g);

    DomainId lower_portion=0;
    DomainId lower_roof=sitehelper_project_add_roof(&p,lower,&main,&lower_portion);
    assert(lower_roof && lower_portion && p.storeys[0].roofs.count==1 && p.storeys[1].roofs.count==1);
    assert(sitehelper_project_find_owning_storey(&p,lower_roof)->id==lower);

    DomainId next=p.domain_ids.next;
    assert(!sitehelper_project_add_roof_composition(&p,roof,(RoofComposition){first,999999,ROOF_COMPOSITION_INTERSECTS}));
    assert(p.domain_ids.next==next && sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);

    /* Priority 26G3 persists authoritative roof source state in format 15. */
    const char *path="/tmp/sitehelper_roof_g1_round_trip.txt";
    assert(sitehelper_project_save_file(&p,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *f=fopen(path,"r"); assert(f);
    char header[64]={0}; assert(fgets(header,sizeof header,f)); assert(fclose(f)==0);
    assert(strcmp(header,"sitehelper_project 22\n")==0);
    SiteHelperProject loaded; sitehelper_project_init(&loaded);
    assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.storey_count==2);
    assert(sitehelper_project_find_roof_by_id_const(&loaded,roof));
    assert(sitehelper_project_find_roof_portion_by_id_const(&loaded,first));
    assert(sitehelper_project_find_roof_portion_by_id_const(&loaded,second));
    assert(sitehelper_project_find_roof_by_id_const(&loaded,lower_roof));
    assert(sitehelper_project_validate(&loaded).code==SITEHELPER_PROJECT_VALID);
    sitehelper_project_destroy(&loaded); remove(path);

    assert(!sitehelper_project_remove_roof_by_id(&p,first));
    assert(sitehelper_project_remove_roof_by_id(&p,roof));
    assert(sitehelper_project_remove_roof_by_id(&p,lower_roof));
    assert(!sitehelper_project_contains_domain_id(&p,roof) && !sitehelper_project_contains_domain_id(&p,first));
    assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);
    sitehelper_project_destroy(&p);
    puts("project roofs: ok"); return 0;
}
