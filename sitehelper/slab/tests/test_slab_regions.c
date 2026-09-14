#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slab.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
static int failed;
void *__real_malloc(size_t n);
void *__real_calloc(size_t n,size_t s);
void *__real_realloc(void *p,size_t n);
static int fail(void) { if(fail_after!=SIZE_MAX && fail_after--==0) {failed=1;return 1;} return 0; }
void *__wrap_malloc(size_t n) { return fail()?NULL:__real_malloc(n); }
void *__wrap_calloc(size_t n,size_t s) { return fail()?NULL:__real_calloc(n,s); }
void *__wrap_realloc(void *p,size_t n) { return fail()?NULL:__real_realloc(p,n); }
#endif

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition region[]={{2000,2000},{4000,2000},{4000,3000},{2000,3000}};
static const PlanPosition hole[]={{2500,2250},{3500,2250},{3500,2750},{2500,2750}};
static const PlanPosition adjacent[]={{4000,2000},{5000,2000},{5000,3000},{4000,3000}};
static void init(Slab *s) { *s=(Slab){0}; assert(slab_build(42,outer,4,100,0,s)==SLAB_SUCCESS); }
static void outline_equal(const SlabOutline *a,const SlabOutline *b)
{
    assert(a->vertex_count==b->vertex_count);
    assert(memcmp(a->vertices,b->vertices,a->vertex_count*sizeof *a->vertices)==0);
}
static void equal(const Slab *a,const Slab *b)
{
    assert(a->id==b->id && a->definition.thickness_mm==b->definition.thickness_mm && a->definition.top_level_offset_mm==b->definition.top_level_offset_mm);
    outline_equal(&a->definition.outline,&b->definition.outline);
    assert(a->definition.penetrations.count==b->definition.penetrations.count && a->definition.regions.count==b->definition.regions.count);
    for(size_t i=0;i<a->definition.penetrations.count;i++) { outline_equal(&a->definition.penetrations.items[i].outline,&b->definition.penetrations.items[i].outline); }
    for(size_t i=0;i<a->definition.regions.count;i++) {
        const SlabRegion *r=&a->definition.regions.items[i],*s=&b->definition.regions.items[i];
        assert(r->top_level_offset_mm==s->top_level_offset_mm && r->thickness_mm==s->thickness_mm); outline_equal(&r->outline,&s->outline);
    }
    assert(a->definition.edge_rebates.count==b->definition.edge_rebates.count);
    for(size_t i=0;i<a->definition.edge_rebates.count;i++) {
        const SlabEdgeRebate *x=&a->definition.edge_rebates.items[i],*y=&b->definition.edge_rebates.items[i];
        assert(x->edge_index==y->edge_index&&x->start_offset_mm==y->start_offset_mm&&
            x->end_offset_mm==y->end_offset_mm&&x->width_mm==y->width_mm&&x->depth_mm==y->depth_mm);
    }
}
static void reject(Slab *s,const PlanPosition *v,size_t n,int thickness,SlabCode expected)
{
    Slab before={0}; assert(slab_clone(s,&before)==SLAB_SUCCESS);
    unsigned char bytes[sizeof *s]; memcpy(bytes,s,sizeof *s);
    assert(slab_add_region(s,v,n,-50,thickness)==expected);
    assert(memcmp(bytes,s,sizeof *s)==0); equal(s,&before); slab_destroy(&before);
}
static void quantity(const Slab *s,uint64_t net,uint64_t base,uint64_t total_volume)
{
    SlabConstructionQuantities q={0}; assert(slab_measure_construction(&s->definition,&q)==SLAB_SUCCESS);
    assert(q.net_area2_mm2==net && q.base_material_area2_mm2==base && q.region_material_area2_mm2==net-base);
    assert(q.total_volume2_mm3==total_volume);
    SlabMaterialQuantities old; assert(slab_measure_material(&s->definition,&old)==SLAB_SUCCESS);
    assert(old.net_area2_mm2==net && old.net_volume2_mm3==net*(uint64_t)s->definition.thickness_mm);
}
static void properties(const Slab *s,PlanPosition point,SlabPointKind kind,size_t index,int64_t top,int64_t bottom,int thickness)
{
    SlabPointProperties p;
    assert(slab_properties_at_plan_position(s,point,&p)==SLAB_SUCCESS);
    assert(p.kind==kind && p.region_index==index && (thickness ? 3000+(int64_t)p.top_level_offset_mm : p.top_level_offset_mm)==top && (thickness ? 3000+p.bottom_level_offset_mm : p.bottom_level_offset_mm)==bottom && p.thickness_mm==thickness);
}

static void test_quantities_levels_lifetime(void)
{
    Slab s; init(&s);
    quantity(&s,160000000,160000000,16000000000ULL);
    assert(slab_add_region(&s,region,4,-50,50)==SLAB_SUCCESS);
    quantity(&s,160000000,156000000,15800000000ULL);
    properties(&s,(PlanPosition){100,100},SLAB_POINT_BASE,SIZE_MAX,3000,2900,100);
    properties(&s,(PlanPosition){2100,2100},SLAB_POINT_REGION,0,2950,2900,50);
    assert(slab_add_penetration(&s,hole,4)==SLAB_SUCCESS);
    quantity(&s,159000000,156000000,15750000000ULL);
    SlabRegionQuantities r; assert(slab_region_measure(&s.definition,0,&r)==SLAB_SUCCESS);
    assert(r.polygon_area2_mm2==4000000 && r.void_area2_mm2==1000000 && r.material_area2_mm2==3000000 && r.volume2_mm3==150000000 && r.thickness_mm==50);
    assert(slab_add_region(&s,adjacent,4,-50,150)==SLAB_SUCCESS);
    quantity(&s,159000000,154000000,15850000000ULL);
    properties(&s,(PlanPosition){4500,2500},SLAB_POINT_REGION,1,2950,2800,150);
    properties(&s,(PlanPosition){-1,100},SLAB_POINT_OUTSIDE,SIZE_MAX,0,0,0);
    properties(&s,(PlanPosition){3000,2500},SLAB_POINT_PENETRATION,SIZE_MAX,0,0,0);
    properties(&s,(PlanPosition){0,100},SLAB_POINT_OUTER_BOUNDARY,SIZE_MAX,0,0,0);
    properties(&s,(PlanPosition){2500,2500},SLAB_POINT_PENETRATION_BOUNDARY,SIZE_MAX,0,0,0);
    properties(&s,(PlanPosition){4000,2500},SLAB_POINT_REGION_BOUNDARY,SIZE_MAX,0,0,0);
    s.definition.top_level_offset_mm=100;
    properties(&s,(PlanPosition){100,100},SLAB_POINT_BASE,SIZE_MAX,3100,3000,100);
    properties(&s,(PlanPosition){2100,2100},SLAB_POINT_REGION,0,2950,2900,50);
    Slab clone={0}; assert(slab_clone(&s,&clone)==SLAB_SUCCESS); equal(&s,&clone);
    assert(clone.definition.regions.items!=s.definition.regions.items && slab_region_at(&clone,0)->outline.vertices!=slab_region_at(&s,0)->outline.vertices);
    assert(slab_clone(&s,&s)==SLAB_SUCCESS); equal(&s,&clone);
    assert(slab_remove_region(&s,0)==SLAB_SUCCESS);
    assert(s.definition.regions.count==1 && slab_region_at(&s,0)->thickness_mm==150);
    assert(!slab_region_at(&s,1) && !slab_region_at(NULL,0));
    assert(slab_remove_region(&s,1)==SLAB_INVALID_ARGUMENT);
    assert(slab_remove_region(&s,0)==SLAB_SUCCESS);
    slab_destroy(&s); quantity(&clone,159000000,154000000,15850000000ULL); slab_destroy(&clone); slab_destroy(&clone);
}

static void test_valid_geometry(void)
{
    Slab s; init(&s);
    const PlanPosition corner[]={{0,0},{1000,0},{1000,1000},{0,1000}};
    const PlanPosition point_touch[]={{1000,1000},{1500,1000},{1500,1500},{1000,1500}};
    assert(slab_add_region(&s,corner,4,50,200)==SLAB_SUCCESS);
    assert(slab_add_region(&s,point_touch,4,0,100)==SLAB_SUCCESS); slab_destroy(&s);
    const PlanPosition l[]={{0,0},{10,0},{10,6},{6,6},{6,10},{0,10}};
    const PlanPosition concave[]={{0,0},{5,0},{5,2},{2,2},{2,5},{0,5}};
    for(int winding=0;winding<4;winding++) {
        PlanPosition a[6],b[6];
        for(size_t i=0;i<6;i++) {a[i]=l[(winding&1)?5-i:i];b[i]=concave[(winding&2)?5-i:i];}
        assert(slab_build(1,a,6,3,0,&s)==SLAB_SUCCESS);
        assert(slab_add_region(&s,b,6,-2,5)==SLAB_SUCCESS);
        quantity(&s,168,136,568);
        slab_destroy(&s);
    }
    init(&s);
    const PlanPosition half[]={{1,1},{4,1},{1,2}};
    assert(slab_add_region(&s,half,3,10,3)==SLAB_SUCCESS);
    quantity(&s,160000000,159999997,15999999709ULL); slab_destroy(&s);
    /* Complete replacement and subdivided coincident boundary are valid. */
    init(&s); const PlanPosition subdivided[]={{0,0},{5000,0},{10000,0},{10000,8000},{0,8000}};
    assert(slab_add_region(&s,subdivided,5,-50,50)==SLAB_SUCCESS);
    quantity(&s,160000000,0,8000000000ULL); slab_destroy(&s);
    /* Shared edges with extra vertices and a T-junction, both windings. */
    for(int reverse=0;reverse<2;reverse++) {
        init(&s); assert(slab_add_region(&s,region,4,0,100)==SLAB_SUCCESS);
        const PlanPosition right[]={{4000,2000},{5000,2000},{5000,2500},{4000,2500},{4000,2250}};
        PlanPosition b[5];for(size_t i=0;i<5;i++){b[i]=right[reverse?4-i:i];}
        assert(slab_add_region(&s,b,5,-50,50)==SLAB_SUCCESS); slab_destroy(&s);
    }
    PlanPosition huge_origin[]={{INT_MAX-10,INT_MIN},{INT_MAX,INT_MIN},{INT_MAX,INT_MIN+10},{INT_MAX-10,INT_MIN+10}};
    PlanPosition small[]={{INT_MAX-10,INT_MIN},{INT_MAX-5,INT_MIN},{INT_MAX-5,INT_MIN+5},{INT_MAX-10,INT_MIN+5}};
    assert(slab_build(1,huge_origin,4,3,0,&s)==SLAB_SUCCESS);
    assert(slab_add_region(&s,small,4,INT_MIN,5)==SLAB_SUCCESS); quantity(&s,200,150,700);
    SlabPointProperties p; assert(slab_properties_at_plan_position(&s,(PlanPosition){INT_MAX-9,INT_MIN+1},&p)==SLAB_SUCCESS);
    assert(p.top_level_offset_mm==INT_MIN && p.bottom_level_offset_mm==(int64_t)INT_MIN-5 && (int64_t)INT_MIN+p.bottom_level_offset_mm==(int64_t)INT_MIN*2-5);
    slab_destroy(&s);
}

static void test_invalid_and_relationships(void)
{
    Slab s; init(&s);
    for(size_t n=0;n<3;n++) { reject(&s,region,n,50,SLAB_INVALID_REGION_OUTLINE); }
    reject(&s,region,4,0,SLAB_INVALID_REGION_THICKNESS); reject(&s,region,4,-1,SLAB_INVALID_REGION_THICKNESS);
    reject(&s,region,SIZE_MAX,50,SLAB_NUMERIC_OVERFLOW);
    const PlanPosition bad[][5]={{{1,1},{4,1},{4,1},{1,4}},{{1,1},{4,1},{4,4},{1,1}},
        {{1,1},{2,2},{3,3}},{{1,1},{4,4},{1,4},{4,1}},{{1,1},{5,1},{3,1},{5,5},{1,5}}};
    const size_t counts[]={4,4,3,4,5};
    for(size_t i=0;i<5;i++) { reject(&s,bad[i],counts[i],50,SLAB_INVALID_REGION_OUTLINE); }
    const PlanPosition outside[]={{-1,1},{100,1},{100,100},{-1,100}};
    reject(&s,outside,4,50,SLAB_REGION_OUTSIDE);
    const PlanPosition extreme[]={{INT_MIN,INT_MIN},{INT_MAX,INT_MIN},{INT_MAX,INT_MAX},{INT_MIN,INT_MAX}};
    reject(&s,extreme,4,50,SLAB_NUMERIC_OVERFLOW);
    assert(slab_add_region(&s,region,4,0,100)==SLAB_SUCCESS);
    const PlanPosition overlap[][4]={{{3000,2500},{4500,2500},{4500,3500},{3000,3500}},
        {{2500,2200},{3000,2200},{3000,2500},{2500,2500}},{{1500,1500},{4500,1500},{4500,3500},{1500,3500}},
        {{3000,1000},{3500,1000},{3500,4000},{3000,4000}}};
    for(size_t i=0;i<4;i++) { reject(&s,overlap[i],4,50,SLAB_REGION_OVERLAP); }
    reject(&s,region,4,50,SLAB_REGION_OVERLAP);
    const PlanPosition duplicate_split[]={{2000,2000},{3000,2000},{4000,2000},{4000,3000},{2000,3000}};
    reject(&s,duplicate_split,5,50,SLAB_REGION_OVERLAP); slab_destroy(&s);
    const PlanPosition u[]={{0,0},{10,0},{10,10},{7,10},{7,3},{3,3},{3,10},{0,10}};
    const PlanPosition bridge[]={{1,3},{9,3},{9,7},{1,7}};
    assert(slab_build(1,u,8,100,0,&s)==SLAB_SUCCESS);
    reject(&s,bridge,4,50,SLAB_REGION_OUTSIDE);
    const PlanPosition vertex_bridge[]={{0,0},{10,0},{10,10},{7,10},{3,10},{0,10}};
    reject(&s,vertex_bridge,6,50,SLAB_REGION_OUTSIDE); /* All vertices on the exterior. */
    slab_destroy(&s);
}

/* An independent interval oracle, also transformed into oblique polygons, checks
 * adjacency, nesting and small overlaps without repeating polygon predicates. */
static void test_adjacency_grid(void)
{
    size_t cases=0;
    for(int oblique=0;oblique<2;oblique++) {
        for(int ax=0;ax<3;ax++) { for(int ar=ax+1;ar<=3;ar++) {
        for(int ay=0;ay<3;ay++) { for(int at=ay+1;at<=3;at++) {
        for(int bx=0;bx<3;bx++) { for(int br=bx+1;br<=3;br++) {
        for(int by=0;by<3;by++) { for(int bt=by+1;bt<=3;bt++) {
            PlanPosition a[]={{ax,ay},{ar,ay},{ar,at},{ax,at}};
            PlanPosition b[]={{bx,by},{br,by},{br,bt},{bx,bt}};
            if(oblique) {
                for(size_t i=0;i<4;i++) {
                    a[i]=(PlanPosition){2*a[i].x+a[i].y,a[i].x+3*a[i].y};
                    b[i]=(PlanPosition){2*b[i].x+b[i].y,b[i].x+3*b[i].y};
                }
                PlanPosition temp=b[0];b[0]=b[3];b[3]=temp;temp=b[1];b[1]=b[2];b[2]=temp;
            }
            int overlap=ax<br && bx<ar && ay<bt && by<at;
            Slab slab;init(&slab);
            assert(slab_add_region(&slab,a,4,0,100)==SLAB_SUCCESS);
            assert(slab_add_region(&slab,b,4,-50,50)==(overlap?SLAB_REGION_OVERLAP:SLAB_SUCCESS));
            assert(slab_validate(&slab)==SLAB_SUCCESS);slab_destroy(&slab);cases++;
        } } } } } } } }
    }
    /* Complementary concave regions sharing several edges; insertion
     * order must not change their volume or classify adjacency as overlap. */
    const PlanPosition a[]={{0,0},{8,0},{8,2},{2,2},{2,8},{0,8}};
    const PlanPosition b[]={{2,2},{4,2},{8,2},{8,8},{2,8},{2,4}};
    for(int order=0;order<2;order++) {
        Slab slab;init(&slab);
        assert(slab_add_region(&slab,order?b:a,6,order?-50:0,order?50:100)==SLAB_SUCCESS);
        assert(slab_add_region(&slab,order?a:b,6,order?0:-50,order?100:50)==SLAB_SUCCESS);
        assert(slab_validate(&slab)==SLAB_SUCCESS);
        quantity(&slab,160000000,159999872,15999996400ULL);slab_destroy(&slab);
    }
    Slab slab;init(&slab);
    /* Four quadrants meeting at one vertex. */
    for(int x=0;x<2;x++) { for(int y=0;y<2;y++) {
        PlanPosition q[]={{x,y},{x+1,y},{x+1,y+1},{x,y+1}};
        assert(slab_add_region(&slab,q,4,x*50,100)==SLAB_SUCCESS);
    } }
    slab_destroy(&slab);
    printf("region adjacency oracle: %zu rectangle/oblique pairs passed\n",cases);
}

static void test_void_interaction_orders(void)
{
    const PlanPosition crossing[]={{3000,2500},{4500,2500},{4500,3500},{3000,3500}};
    const PlanPosition edge_crossing[]={{3000,2000},{4500,2000},{4500,3000},{3000,3000}};
    const PlanPosition separate[]={{6000,2000},{7000,2000},{7000,3000},{6000,3000}};
    Slab reference={0};
    for(int order=0;order<2;order++) {
        Slab s; init(&s);
        if(order) {assert(slab_add_region(&s,region,4,-50,50)==SLAB_SUCCESS);assert(slab_add_penetration(&s,hole,4)==SLAB_SUCCESS);}
        else {assert(slab_add_penetration(&s,hole,4)==SLAB_SUCCESS);assert(slab_add_region(&s,region,4,-50,50)==SLAB_SUCCESS);}
        assert(slab_add_penetration(&s,separate,4)==SLAB_SUCCESS);
        assert(slab_validate(&s)==SLAB_SUCCESS);
        quantity(&s,157000000,154000000,15550000000ULL);
        if(!order) {assert(slab_clone(&s,&reference)==SLAB_SUCCESS);} else {equal(&s,&reference);}
        slab_destroy(&s);
        const PlanPosition *crossings[]={crossing,edge_crossing};
        for(size_t i=0;i<2;i++) {
            init(&s);
            if(order) {
                assert(slab_add_region(&s,region,4,-50,50)==SLAB_SUCCESS);
                unsigned char before[sizeof s];memcpy(before,&s,sizeof s);
                assert(slab_add_penetration(&s,crossings[i],4)==SLAB_REGION_PENETRATION_INTERSECTION);
                assert(memcmp(before,&s,sizeof s)==0);
            } else {
                assert(slab_add_penetration(&s,crossings[i],4)==SLAB_SUCCESS);
                reject(&s,region,4,50,SLAB_REGION_PENETRATION_INTERSECTION);
            }
            slab_destroy(&s);
        }
        /* Neither strict containment nor coincidence may erase all region
         * material. Both mutation orders must fail without changing ownership. */
        const PlanPosition *empty_regions[]={hole,region};
        for(size_t i=0;i<2;i++) {
            init(&s);
            if(order) {
                assert(slab_add_region(&s,empty_regions[i],4,-50,50)==SLAB_SUCCESS);
                Slab before={0};assert(slab_clone(&s,&before)==SLAB_SUCCESS);
                unsigned char bytes[sizeof s];memcpy(bytes,&s,sizeof s);
                assert(slab_add_penetration(&s,region,4)==SLAB_REGION_PENETRATION_INTERSECTION);
                assert(memcmp(bytes,&s,sizeof s)==0);equal(&s,&before);
                slab_destroy(&before);
            } else {
                assert(slab_add_penetration(&s,region,4)==SLAB_SUCCESS);
                reject(&s,empty_regions[i],4,50,SLAB_REGION_PENETRATION_INTERSECTION);
            }
            assert(slab_validate(&s)==SLAB_SUCCESS);slab_destroy(&s);
        }
    }
    slab_destroy(&reference);
}

static void test_concave_void_quantities(void)
{
    const PlanPosition exterior[]={{0,0},{10,0},{10,6},{6,6},{6,10},{0,10}};
    const PlanPosition region_l[]={{0,0},{5,0},{5,2},{2,2},{2,5},{0,5}};
    const PlanPosition half_void[]={{1,1},{3,1},{1,2}};
    const PlanPosition another_void[]={{1,3},{2,3},{1,4}};
    for(int order=0;order<2;order++) {
        Slab s={0};assert(slab_build(1,exterior,6,3,0,&s)==SLAB_SUCCESS);
        if(order) {assert(slab_add_region(&s,region_l,6,-1,5)==SLAB_SUCCESS);}
        assert(slab_add_penetration(&s,half_void,3)==SLAB_SUCCESS);
        assert(slab_add_penetration(&s,another_void,3)==SLAB_SUCCESS);
        if(!order) {assert(slab_add_region(&s,region_l,6,-1,5)==SLAB_SUCCESS);}
        /* A void touches the region boundary without crossing it. Exact half
         * units survive subtraction from a concave region and thickness use. */
        SlabRegionQuantities q;
        assert(slab_region_measure(&s.definition,0,&q)==SLAB_SUCCESS);
        assert(q.polygon_area2_mm2==32 && q.void_area2_mm2==3 && q.material_area2_mm2==29 && q.volume2_mm3==145);
        quantity(&s,165,136,553);slab_destroy(&s);
    }
}

static void test_failure_outputs(void)
{
    Slab s;init(&s);assert(slab_add_region(&s,region,4,0,100)==SLAB_SUCCESS);
    SlabRegionCollection saved=s.definition.regions;
    SlabRegionCollection bad[]={{NULL,1,1},{saved.items,2,1},{saved.items,0,0},{saved.items,1,SIZE_MAX}};
    for(size_t i=0;i<4;i++) {
        s.definition.regions=bad[i];assert(slab_validate(&s)==(i==3?SLAB_NUMERIC_OVERFLOW:SLAB_INVALID_REGION_COLLECTION));
        assert(!slab_region_at(&s,0));
    }
    s.definition.regions=saved;
    SlabConstructionQuantities q={1,2,3,4}; unsigned char bytes[sizeof q];memcpy(bytes,&q,sizeof q);
    SlabPointProperties point={.kind=SLAB_POINT_BASE};unsigned char pb[sizeof point];memcpy(pb,&point,sizeof point);
    saved.items[0].thickness_mm=0;
    assert(slab_measure_construction(&s.definition,&q)==SLAB_INVALID_REGION_THICKNESS && memcmp(bytes,&q,sizeof q)==0);
    assert(slab_properties_at_plan_position(&s,(PlanPosition){100,100},&point)==SLAB_INVALID_REGION_THICKNESS && memcmp(pb,&point,sizeof point)==0);
    saved.items[0].thickness_mm=100;
    SlabRegionQuantities rq={1,2,3,4,5};unsigned char rb[sizeof rq];memcpy(rb,&rq,sizeof rq);
    assert(slab_region_measure(&s.definition,1,&rq)==SLAB_INVALID_ARGUMENT && memcmp(rb,&rq,sizeof rq)==0);
    assert(slab_region_measure(NULL,0,&rq)==SLAB_INVALID_ARGUMENT && memcmp(rb,&rq,sizeof rq)==0);
    assert(slab_measure_construction(NULL,&q)==SLAB_INVALID_ARGUMENT && memcmp(bytes,&q,sizeof q)==0);
    assert(slab_properties_at_plan_position(NULL,(PlanPosition){0,0},&point)==SLAB_INVALID_ARGUMENT && memcmp(pb,&point,sizeof point)==0);
    SlabOutline outline=saved.items[0].outline;
    saved.items[0].outline=(SlabOutline){NULL,4,4};
    assert(slab_region_validate(&saved.items[0])==SLAB_INVALID_REGION_OUTLINE_COLLECTION);
    assert(slab_region_measure(&s.definition,0,&rq)==SLAB_INVALID_REGION_OUTLINE_COLLECTION && memcmp(rb,&rq,sizeof rq)==0);
    saved.items[0].outline=outline;slab_destroy(&s);
    const PlanPosition large[]={{0,0},{INT_MAX,0},{INT_MAX,INT_MAX},{0,INT_MAX}};
    assert(slab_build(1,large,4,1,0,&s)==SLAB_SUCCESS);
    assert(slab_add_region(&s,large,4,0,INT_MAX)==SLAB_SUCCESS);
    assert(slab_measure_construction(&s.definition,&q)==SLAB_NUMERIC_OVERFLOW && memcmp(bytes,&q,sizeof q)==0);
    slab_destroy(&s);
    /* Each volume fits, but their sum does not (region+base, then region+region). */
    const PlanPosition left[]={{0,0},{INT_MAX/2,0},{INT_MAX/2,INT_MAX},{0,INT_MAX}};
    const PlanPosition right[]={{INT_MAX/2,0},{INT_MAX,0},{INT_MAX,INT_MAX},{INT_MAX/2,INT_MAX}};
    assert(slab_build(1,large,4,3,0,&s)==SLAB_SUCCESS);
    assert(slab_add_region(&s,left,4,0,3)==SLAB_SUCCESS);
    assert(slab_region_measure(&s.definition,0,&rq)==SLAB_SUCCESS);
    assert(slab_measure_construction(&s.definition,&q)==SLAB_NUMERIC_OVERFLOW && memcmp(bytes,&q,sizeof q)==0);
    assert(slab_add_region(&s,right,4,-50,3)==SLAB_SUCCESS);
    assert(slab_region_measure(&s.definition,1,&rq)==SLAB_SUCCESS);
    assert(slab_measure_construction(&s.definition,&q)==SLAB_NUMERIC_OVERFLOW && memcmp(bytes,&q,sizeof q)==0);
    slab_destroy(&s);
    assert(slab_add_region(NULL,region,4,0,50)==SLAB_INVALID_ARGUMENT);
    assert(slab_remove_region(NULL,0)==SLAB_INVALID_ARGUMENT && slab_region_validate(NULL)==SLAB_INVALID_ARGUMENT);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocations(void)
{
    size_t adds=0,clones=0;
    for(size_t index=0;index<10;index++) {
        Slab s,before={0};init(&s);assert(slab_add_region(&s,region,4,0,100)==SLAB_SUCCESS);assert(slab_clone(&s,&before)==SLAB_SUCCESS);
        unsigned char bytes[sizeof s];memcpy(bytes,&s,sizeof s);
        fail_after=index;failed=0;SlabCode code=slab_add_region(&s,adjacent,4,-50,50);fail_after=SIZE_MAX;
        if(failed){adds++;assert(code==SLAB_ALLOCATION_FAILED && memcmp(bytes,&s,sizeof s)==0);equal(&s,&before);assert(slab_add_region(&s,adjacent,4,-50,50)==SLAB_SUCCESS);}
        else {assert(code==SLAB_SUCCESS);}slab_destroy(&s);slab_destroy(&before);if(!failed){break;}
    }
    Slab source;init(&source);assert(slab_add_region(&source,region,4,-50,50)==SLAB_SUCCESS);assert(slab_add_region(&source,adjacent,4,0,100)==SLAB_SUCCESS);assert(slab_add_penetration(&source,hole,4)==SLAB_SUCCESS);
    for(size_t index=0;index<20;index++) {
        Slab s,before={0};init(&s);assert(slab_add_region(&s,adjacent,4,-25,75)==SLAB_SUCCESS);assert(slab_clone(&s,&before)==SLAB_SUCCESS);
        unsigned char bytes[sizeof s];memcpy(bytes,&s,sizeof s);
        fail_after=index;failed=0;SlabCode code=slab_clone(&source,&s);fail_after=SIZE_MAX;
        if(failed){clones++;assert(code==SLAB_ALLOCATION_FAILED && memcmp(bytes,&s,sizeof s)==0);equal(&s,&before);assert(slab_clone(&source,&s)==SLAB_SUCCESS);}
        else{assert(code==SLAB_SUCCESS);}equal(&source,&s);slab_destroy(&s);slab_destroy(&before);if(!failed){break;}
    }
    assert(adds==2 && clones==6);slab_destroy(&source);
    printf("region allocation sweep: %zu add, %zu clone failures; retries passed\n",adds,clones);
}
#endif
int main(void)
{
    test_quantities_levels_lifetime();test_valid_geometry();test_invalid_and_relationships();test_void_interaction_orders();test_adjacency_grid();test_concave_void_quantities();test_failure_outputs();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocations();
#endif
    puts("slab region tests passed");return 0;
}
