#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slab.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX;
static int failed;
void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t s);
void *__real_realloc(void *p, size_t n);
static int fail(void) { if (fail_after != SIZE_MAX && fail_after-- == 0) { failed=1; return 1; } return 0; }
void *__wrap_malloc(size_t n) { return fail() ? NULL : __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t s) { return fail() ? NULL : __real_calloc(n,s); }
void *__wrap_realloc(void *p, size_t n) { return fail() ? NULL : __real_realloc(p,n); }
#endif

static const PlanPosition outer[] = {{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition hole[] = {{2000,2000},{4000,2000},{4000,3000},{2000,3000}};
static const PlanPosition second[] = {{5000,2000},{6000,2000},{6000,3000},{5000,3000}};
static void init(Slab *s) { *s=(Slab){0}; assert(slab_build(42,outer,4,100,-50,s)==SLAB_SUCCESS); }
static void outline_equal(const SlabOutline *a, const SlabOutline *b)
{
    assert(a->vertex_count==b->vertex_count);
    assert(memcmp(a->vertices,b->vertices,a->vertex_count*sizeof *a->vertices)==0);
}
static void equal(const Slab *a, const Slab *b)
{
    assert(a->id==b->id && a->definition.thickness_mm==b->definition.thickness_mm &&
        a->definition.top_level_offset_mm==b->definition.top_level_offset_mm);
    outline_equal(&a->definition.outline,&b->definition.outline);
    assert(a->definition.penetrations.count==b->definition.penetrations.count);
    for(size_t i=0;i<a->definition.penetrations.count;i++) {
        outline_equal(&a->definition.penetrations.items[i].outline,&b->definition.penetrations.items[i].outline);
    }
}
static void reject(Slab *s, const PlanPosition *v, size_t n, SlabCode expected)
{
    Slab before={0}; assert(slab_clone(s,&before)==SLAB_SUCCESS);
    unsigned char bytes[sizeof *s]; memcpy(bytes,s,sizeof *s);
    assert(slab_add_penetration(s,v,n)==expected);
    assert(memcmp(bytes,s,sizeof *s)==0); equal(s,&before); slab_destroy(&before);
}
static void quantities(const Slab *s, uint64_t gross, uint64_t removed)
{
    SlabMaterialQuantities q={0}; assert(slab_measure_material(&s->definition,&q)==SLAB_SUCCESS);
    assert(q.gross_area2_mm2==gross && q.void_area2_mm2==removed && q.net_area2_mm2==gross-removed);
    uint64_t t=(uint64_t)s->definition.thickness_mm;
    assert(q.gross_volume2_mm3==gross*t && q.void_volume2_mm3==removed*t && q.net_volume2_mm3==(gross-removed)*t);
    SlabQuantities old={0}; assert(slab_measure(&s->definition,&old)==SLAB_SUCCESS);
    assert(old.area2_mm2==gross && old.volume2_mm3==gross*t);
}

static void test_valid_and_quantities(void)
{
    Slab s; init(&s); quantities(&s,160000000,0);
    assert(slab_add_penetration(&s,hole,4)==SLAB_SUCCESS);
    quantities(&s,160000000,4000000);
    assert(slab_penetration_at(&s,0)->outline.vertices!=hole);
    SlabQuantities old; assert(slab_measure(&s.definition,&old)==SLAB_SUCCESS);
    assert(fabs(old.perimeter_mm-36000)<1e-8);
    assert(slab_add_penetration(&s,second,4)==SLAB_SUCCESS);
    quantities(&s,160000000,6000000);
    const PlanPosition triangle[]={{1,1},{4,1},{1,2}};
    assert(slab_add_penetration(&s,triangle,3)==SLAB_SUCCESS);
    quantities(&s,160000000,6000003); /* Exact half-mm² and half-mm³ increments. */
    assert(slab_remove_penetration(&s,1)==SLAB_SUCCESS);
    assert(s.definition.penetrations.count==2 && slab_penetration_at(&s,1)->outline.vertex_count==3);
    quantities(&s,160000000,4000003);
    unsigned char bytes[sizeof s]; memcpy(bytes,&s,sizeof s);
    assert(slab_remove_penetration(&s,2)==SLAB_INVALID_ARGUMENT && memcmp(bytes,&s,sizeof s)==0);
    assert(!slab_penetration_at(&s,2) && !slab_penetration_at(NULL,0));
    Slab clone={0}; assert(slab_clone(&s,&clone)==SLAB_SUCCESS); equal(&s,&clone);
    assert(clone.definition.penetrations.items!=s.definition.penetrations.items);
    assert(slab_penetration_at(&clone,0)->outline.vertices!=slab_penetration_at(&s,0)->outline.vertices);
    assert(slab_clone(&s,&s)==SLAB_SUCCESS); equal(&s,&clone);
    assert(slab_remove_penetration(&s,0)==SLAB_SUCCESS);
    assert(slab_remove_penetration(&s,0)==SLAB_SUCCESS); quantities(&s,160000000,0);
    slab_destroy(&s); quantities(&clone,160000000,4000003); slab_destroy(&clone); slab_destroy(&clone);

    const PlanPosition concave_outer[]={{0,0},{10,0},{10,6},{6,6},{6,10},{0,10}};
    const PlanPosition concave_hole[]={{1,1},{5,1},{5,2},{2,2},{2,5},{1,5}};
    for(int winding=0;winding<4;winding++) {
        PlanPosition a[6],b[6];
        for(size_t i=0;i<6;i++) { a[i]=concave_outer[(winding&1)?5-i:i]; b[i]=concave_hole[(winding&2)?5-i:i]; }
        assert(slab_build(1,a,6,3,0,&s)==SLAB_SUCCESS);
        assert(slab_add_penetration(&s,b,6)==SLAB_SUCCESS);
        quantities(&s,168,14);
        const PlanPosition near_edge[]={{7,1},{9,1},{9,3},{7,3}};
        assert(slab_add_penetration(&s,near_edge,4)==SLAB_SUCCESS); quantities(&s,168,22);
        slab_destroy(&s);
    }
    /* Translation near the integer limits must not affect containment. */
    PlanPosition a[4]={{INT_MAX-10,INT_MIN},{INT_MAX,INT_MIN},{INT_MAX,INT_MIN+10},{INT_MAX-10,INT_MIN+10}};
    PlanPosition b[3]={{INT_MAX-9,INT_MIN+1},{INT_MAX-6,INT_MIN+1},{INT_MAX-9,INT_MIN+2}};
    assert(slab_build(1,a,4,3,0,&s)==SLAB_SUCCESS);
    assert(slab_add_penetration(&s,b,3)==SLAB_SUCCESS); quantities(&s,200,3); slab_destroy(&s);
}

static void test_invalid_outlines(void)
{
    Slab s; init(&s);
    const PlanPosition bad[][5]={
        {{1,1},{4,1},{4,1},{1,4}},
        {{1,1},{4,1},{4,4},{1,1}},
        {{1,1},{2,2},{3,3}},
        {{1,1},{4,4},{1,4},{4,1}},
        {{1,1},{5,1},{3,1},{5,5},{1,5}}
    };
    const size_t counts[]={4,4,3,4,5};
    for(size_t n=0;n<3;n++) { reject(&s,hole,n,SLAB_INVALID_PENETRATION_OUTLINE); }
    for(size_t i=0;i<5;i++) {
        SlabPenetration p={.outline={(PlanPosition *)bad[i],counts[i],counts[i]}};
        assert(slab_penetration_validate(&p)==SLAB_INVALID_PENETRATION_OUTLINE);
        reject(&s,bad[i],counts[i],SLAB_INVALID_PENETRATION_OUTLINE);
    }
    reject(&s,NULL,4,SLAB_INVALID_PENETRATION_OUTLINE);
    reject(&s,hole,SIZE_MAX,SLAB_NUMERIC_OVERFLOW);
    const PlanPosition extreme[]={{INT_MIN,INT_MIN},{INT_MAX,INT_MIN},{INT_MAX,INT_MAX},{INT_MIN,INT_MAX}};
    reject(&s,extreme,4,SLAB_NUMERIC_OVERFLOW);
    slab_destroy(&s);
    /* Independently valid polygons whose relationship predicate overflows. */
    const PlanPosition wide[]={{INT_MIN,-100},{INT_MAX,-100},{INT_MAX,-90},{INT_MIN,-90}};
    const PlanPosition far[]={{0,INT_MAX-10},{5,INT_MAX-10},{5,INT_MAX-5},{0,INT_MAX-5}};
    assert(slab_build(1,wide,4,1,0,&s)==SLAB_SUCCESS);
    reject(&s,far,4,SLAB_NUMERIC_OVERFLOW); slab_destroy(&s);
    assert(slab_add_penetration(NULL,hole,4)==SLAB_INVALID_ARGUMENT);
    assert(slab_remove_penetration(NULL,0)==SLAB_INVALID_ARGUMENT);
    assert(slab_penetration_validate(NULL)==SLAB_INVALID_ARGUMENT);
}

static void test_containment(void)
{
    Slab s; init(&s);
    const PlanPosition bad[][4]={
        {{-5,1},{-1,1},{-1,5},{-5,5}}, /* Outside. */
        {{-1,1},{1,1},{1,5},{-1,5}}, /* Cross. */
        {{0,2},{2,1},{3,2},{2,3}}, /* Point touch. */
        {{0,1},{2,1},{2,3},{0,3}}, /* Shared edge. */
        {{0,0},{2,1},{3,3},{1,2}}  /* Shared vertex. */
    };
    for(size_t i=0;i<5;i++) { reject(&s,bad[i],4,SLAB_PENETRATION_OUTSIDE); }
    slab_destroy(&s);
    const PlanPosition u[]={{0,0},{10,0},{10,10},{7,10},{7,3},{3,3},{3,10},{0,10}};
    const PlanPosition bridge[]={{1,5},{9,5},{9,7},{1,7}};
    assert(slab_build(1,u,8,100,0,&s)==SLAB_SUCCESS);
    /* Every bridge vertex is strictly inside an arm; its horizontal edges leave the slab. */
    reject(&s,bridge,4,SLAB_PENETRATION_OUTSIDE);
    slab_destroy(&s);
    const PlanPosition diamond[]={{0,5},{5,0},{10,5},{5,10}};
    const PlanPosition inside[]={{3,5},{2,4},{3,4}}; /* Ray through an exterior vertex. */
    const PlanPosition touch[]={{1,4},{2,4},{2,5}};
    const PlanPosition outside[]={{1,3},{2,4},{2,5}};
    for(int winding=0;winding<2;winding++) {
        PlanPosition v[4];
        for(size_t i=0;i<4;i++) { v[i]=diamond[winding?3-i:i]; }
        assert(slab_build(1,v,4,3,0,&s)==SLAB_SUCCESS);
        reject(&s,touch,3,SLAB_PENETRATION_OUTSIDE);
        reject(&s,outside,3,SLAB_PENETRATION_OUTSIDE);
        assert(slab_add_penetration(&s,inside,3)==SLAB_SUCCESS);
        quantities(&s,100,1);
        slab_destroy(&s);
    }
}

static void test_interactions(void)
{
    Slab s; init(&s); assert(slab_add_penetration(&s,hole,4)==SLAB_SUCCESS);
    const PlanPosition bad[][4]={
        {{3000,1000},{3500,1000},{3500,4000},{3000,4000}}, /* Crossing. */
        {{3000,2500},{4500,2500},{4500,3500},{3000,3500}}, /* Overlap. */
        {{2500,2200},{3000,2200},{3000,2500},{2500,2500}}, /* Nested. */
        {{1500,1500},{4500,1500},{4500,3500},{1500,3500}}, /* Enclosing. */
        {{4000,3000},{4500,3000},{4500,3500},{4000,3500}}, /* Point touch. */
        {{4000,2000},{4500,2000},{4500,3000},{4000,3000}}  /* Edge touch. */
    };
    for(size_t i=0;i<6;i++) { reject(&s,bad[i],4,SLAB_PENETRATION_OVERLAP); }
    reject(&s,hole,4,SLAB_PENETRATION_OVERLAP);
    assert(slab_add_penetration(&s,second,4)==SLAB_SUCCESS);
    slab_destroy(&s);
}

static void test_malformed_and_quantity_failure(void)
{
    Slab s; init(&s); assert(slab_add_penetration(&s,hole,4)==SLAB_SUCCESS);
    SlabPenetrationCollection saved=s.definition.penetrations;
    SlabPenetrationCollection bad[]={{NULL,1,1},{saved.items,2,1},{saved.items,0,0},{saved.items,1,SIZE_MAX}};
    for(size_t i=0;i<4;i++) {
        s.definition.penetrations=bad[i];
        SlabCode code=i==3?SLAB_NUMERIC_OVERFLOW:SLAB_INVALID_PENETRATION_COLLECTION;
        assert(slab_validate(&s)==code && slab_remove_penetration(&s,0)==code);
        assert(!slab_penetration_at(&s,0));
    }
    s.definition.penetrations=saved;
    SlabOutline o=saved.items[0].outline;
    saved.items[0].outline.vertices=NULL;
    assert(slab_validate(&s)==SLAB_INVALID_PENETRATION_OUTLINE_COLLECTION);
    SlabMaterialQuantities q={1,2,3,4,5,6}; unsigned char bytes[sizeof q]; memcpy(bytes,&q,sizeof q);
    assert(slab_measure_material(&s.definition,&q)==SLAB_INVALID_PENETRATION_OUTLINE_COLLECTION);
    assert(memcmp(bytes,&q,sizeof q)==0);
    saved.items[0].outline=o;
    slab_destroy(&s);
    const PlanPosition large[]={{0,0},{INT_MAX,0},{INT_MAX,INT_MAX},{0,INT_MAX}};
    assert(slab_build(1,large,4,INT_MAX,0,&s)==SLAB_SUCCESS);
    assert(slab_add_penetration(&s,hole,4)==SLAB_SUCCESS);
    assert(slab_measure_material(&s.definition,&q)==SLAB_NUMERIC_OVERFLOW);
    assert(memcmp(bytes,&q,sizeof q)==0);
    assert(slab_measure_material(NULL,&q)==SLAB_INVALID_ARGUMENT);
    assert(slab_measure_material(&s.definition,NULL)==SLAB_INVALID_ARGUMENT);
    slab_destroy(&s);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocations(void)
{
    size_t add_failures=0,copy_failures=0;
    for(size_t index=0;index<10;index++) {
        Slab s,before={0}; init(&s); assert(slab_add_penetration(&s,hole,4)==SLAB_SUCCESS);
        assert(slab_clone(&s,&before)==SLAB_SUCCESS);
        unsigned char bytes[sizeof s]; memcpy(bytes,&s,sizeof s);
        fail_after=index; failed=0;
        SlabCode code=slab_add_penetration(&s,second,4);
        fail_after=SIZE_MAX;
        if(failed) {
            add_failures++; assert(code==SLAB_ALLOCATION_FAILED);
            assert(memcmp(bytes,&s,sizeof s)==0); equal(&s,&before);
            assert(slab_add_penetration(&s,second,4)==SLAB_SUCCESS);
        } else { assert(code==SLAB_SUCCESS); }
        slab_destroy(&s); slab_destroy(&before);
        if(!failed) { break; }
    }
    Slab source; init(&source); assert(slab_add_penetration(&source,hole,4)==SLAB_SUCCESS);
    assert(slab_add_penetration(&source,second,4)==SLAB_SUCCESS);
    for(size_t index=0;index<10;index++) {
        Slab s,before={0}; init(&s); assert(slab_add_penetration(&s,second,4)==SLAB_SUCCESS);
        assert(slab_clone(&s,&before)==SLAB_SUCCESS);
        unsigned char bytes[sizeof s]; memcpy(bytes,&s,sizeof s);
        fail_after=index; failed=0;
        SlabCode code=slab_clone(&source,&s);
        fail_after=SIZE_MAX;
        if(failed) {
            copy_failures++; assert(code==SLAB_ALLOCATION_FAILED);
            assert(memcmp(bytes,&s,sizeof s)==0); equal(&s,&before);
            assert(slab_clone(&source,&s)==SLAB_SUCCESS);
        } else { assert(code==SLAB_SUCCESS); }
        equal(&s,&source); slab_destroy(&s); slab_destroy(&before);
        if(!failed) { break; }
    }
    assert(add_failures==2 && copy_failures==4);
    printf("penetration allocation sweep: %zu add, %zu clone failures; retries passed\n",add_failures,copy_failures);
    slab_destroy(&source);
}
#endif
int main(void)
{
    test_valid_and_quantities(); test_invalid_outlines(); test_containment(); test_interactions();
    test_malformed_and_quantity_failure();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocations();
#endif
    puts("slab penetration geometry tests passed");
    return 0;
}
