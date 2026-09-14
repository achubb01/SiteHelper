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
void *__real_realloc(void *p,size_t n);
static int fail(void) {if(fail_after!=SIZE_MAX && fail_after--==0){failed=1;return 1;}return 0;}
void *__wrap_malloc(size_t n){return fail()?NULL:__real_malloc(n);}
void *__wrap_realloc(void *p,size_t n){return fail()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition rectangle[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static void init(Slab *s){*s=(Slab){0};assert(slab_build(42,rectangle,4,100,0,s)==SLAB_SUCCESS);}
static void equal(const Slab *a,const Slab *b)
{
    assert(a->id==b->id && a->definition.thickness_mm==b->definition.thickness_mm &&
        a->definition.top_level_offset_mm==b->definition.top_level_offset_mm &&
        a->definition.penetrations.count==b->definition.penetrations.count &&
        a->definition.regions.count==b->definition.regions.count &&
        a->definition.edge_rebates.count==b->definition.edge_rebates.count);
    assert(a->definition.outline.vertex_count==b->definition.outline.vertex_count);
    assert(memcmp(a->definition.outline.vertices,b->definition.outline.vertices,
        a->definition.outline.vertex_count*sizeof *a->definition.outline.vertices)==0);
    for(size_t i=0;i<a->definition.penetrations.count;i++) {
        const SlabOutline *x=&a->definition.penetrations.items[i].outline,*y=&b->definition.penetrations.items[i].outline;
        assert(x->vertex_count==y->vertex_count&&memcmp(x->vertices,y->vertices,x->vertex_count*sizeof *x->vertices)==0);
    }
    for(size_t i=0;i<a->definition.regions.count;i++) {
        const SlabRegion *x=&a->definition.regions.items[i],*y=&b->definition.regions.items[i];
        assert(x->top_level_offset_mm==y->top_level_offset_mm&&x->thickness_mm==y->thickness_mm&&
            x->outline.vertex_count==y->outline.vertex_count&&memcmp(x->outline.vertices,y->outline.vertices,
                x->outline.vertex_count*sizeof *x->outline.vertices)==0);
    }
    for(size_t i=0;i<a->definition.edge_rebates.count;i++) {
        const SlabEdgeRebate *x=&a->definition.edge_rebates.items[i],*y=&b->definition.edge_rebates.items[i];
        assert(x->edge_index==y->edge_index && x->start_offset_mm==y->start_offset_mm &&
            x->end_offset_mm==y->end_offset_mm && x->width_mm==y->width_mm && x->depth_mm==y->depth_mm);
    }
}
static void reject(Slab *s,size_t edge,int start,int end,int width,int depth,SlabCode expected)
{
    Slab before={0};assert(slab_clone(s,&before)==SLAB_SUCCESS);
    unsigned char bytes[sizeof *s];memcpy(bytes,s,sizeof *s);
    assert(slab_add_edge_rebate(s,edge,start,end,width,depth)==expected);
    assert(memcmp(bytes,s,sizeof *s)==0);equal(s,&before);slab_destroy(&before);
}

static void test_creation_lengths_and_lifetime(void)
{
    Slab s;init(&s);assert(s.definition.edge_rebates.count==0 && !s.definition.edge_rebates.items);
    int length=-1;assert(slab_edge_local_length_mm(&s.definition,0,&length)==SLAB_SUCCESS && length==10000);
    assert(slab_edge_local_length_mm(&s.definition,3,&length)==SLAB_SUCCESS && length==8000);
    assert(slab_add_edge_rebate(&s,0,0,10000,110,15)==SLAB_SUCCESS); /* No compliance minimum. */
    assert(slab_add_edge_rebate(&s,1,100,500,75,400)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&s,3,0,8000,90,20)==SLAB_SUCCESS); /* Closing edge. */
    assert(slab_edge_rebate_length_mm(slab_edge_rebate_at(&s,1),&length)==SLAB_SUCCESS && length==400);
    assert(!slab_edge_rebate_at(&s,3) && !slab_edge_rebate_at(NULL,0));
    SlabConstructionQuantities before,after;
    Slab plain;init(&plain);assert(slab_measure_construction(&plain.definition,&before)==SLAB_SUCCESS);
    assert(slab_measure_construction(&s.definition,&after)==SLAB_SUCCESS);
    assert(memcmp(&before,&after,sizeof before)==0); /* Rebate volume intentionally excluded. */
    Slab clone={0};assert(slab_clone(&s,&clone)==SLAB_SUCCESS);equal(&s,&clone);
    assert(clone.definition.edge_rebates.items!=s.definition.edge_rebates.items);
    assert(slab_remove_edge_rebate(&s,1)==SLAB_SUCCESS && s.definition.edge_rebates.count==2);
    assert(slab_remove_edge_rebate(&s,2)==SLAB_INVALID_ARGUMENT);
    slab_destroy(&s);assert(clone.definition.edge_rebates.count==3&&
        clone.definition.edge_rebates.items[2].edge_index==3);
    slab_destroy(&clone);slab_destroy(&clone);slab_destroy(&plain);
}

static void test_invalid_and_overlap(void)
{
    Slab s;init(&s);
    reject(&s,4,0,1,1,1,SLAB_EDGE_REBATE_INVALID_EDGE);
    reject(&s,SIZE_MAX,0,1,1,1,SLAB_EDGE_REBATE_INVALID_EDGE);
    reject(&s,0,-1,10,1,1,SLAB_EDGE_REBATE_INVALID_INTERVAL);
    reject(&s,0,10,10,1,1,SLAB_EDGE_REBATE_INVALID_INTERVAL);
    reject(&s,0,11,10,1,1,SLAB_EDGE_REBATE_INVALID_INTERVAL);
    reject(&s,0,0,10001,1,1,SLAB_EDGE_REBATE_INVALID_INTERVAL);
    reject(&s,0,0,1,0,1,SLAB_EDGE_REBATE_INVALID_DIMENSIONS);
    reject(&s,0,0,1,-1,1,SLAB_EDGE_REBATE_INVALID_DIMENSIONS);
    reject(&s,0,0,1,1,0,SLAB_EDGE_REBATE_INVALID_DIMENSIONS);
    reject(&s,0,0,1,1,-1,SLAB_EDGE_REBATE_INVALID_DIMENSIONS);
    assert(slab_add_edge_rebate(&s,0,100,300,100,20)==SLAB_SUCCESS);
    const int intervals[][2]={{100,300},{50,150},{250,350},{150,250},{0,400}};
    for(size_t i=0;i<5;i++)reject(&s,0,intervals[i][0],intervals[i][1],i?80:200,i?50:10,SLAB_EDGE_REBATE_OVERLAP);
    assert(slab_add_edge_rebate(&s,0,0,100,100,20)==SLAB_SUCCESS); /* Adjacent. */
    assert(slab_add_edge_rebate(&s,0,300,400,100,20)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&s,0,500,600,100,20)==SLAB_SUCCESS); /* Gap. */
    slab_destroy(&s);
    for(int reverse=0;reverse<2;reverse++) {
        init(&s);
        assert(slab_add_edge_rebate(&s,0,reverse?200:100,reverse?400:300,1,1)==SLAB_SUCCESS);
        reject(&s,0,reverse?100:200,reverse?300:400,1,1,SLAB_EDGE_REBATE_OVERLAP);
        slab_destroy(&s);
    }
}

static void test_diagonal_corners_regions_and_stale_edges(void)
{
    const PlanPosition triangle[]={{0,0},{3,4},{0,4}};
    Slab s={0};assert(slab_build(7,triangle,3,100,0,&s)==SLAB_SUCCESS);
    int length;assert(slab_edge_local_length_mm(&s.definition,0,&length)==SLAB_SUCCESS && length==5);
    assert(slab_add_edge_rebate(&s,0,0,5,1,1)==SLAB_SUCCESS);slab_destroy(&s);
    const PlanPosition reverse_triangle[]={{3,4},{0,0},{0,4}};
    assert(slab_build(7,reverse_triangle,3,100,0,&s)==SLAB_SUCCESS);
    assert(slab_edge_local_length_mm(&s.definition,0,&length)==SLAB_SUCCESS && length==5);
    assert(slab_add_edge_rebate(&s,0,0,5,1,1)==SLAB_SUCCESS);slab_destroy(&s);
    const PlanPosition concave[]={{0,0},{10,0},{10,10},{6,10},{6,4},{4,4},{4,10},{0,10}};
    assert(slab_build(8,concave,8,100,0,&s)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&s,2,0,4,2,15)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&s,3,0,6,2,15)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&s,4,0,2,2,15)==SLAB_SUCCESS); /* Re-entrant corner. */
    assert(slab_add_edge_rebate(&s,7,0,10,2,15)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&s,0,0,10,2,15)==SLAB_SUCCESS); /* Convex closing corner. */
    slab_destroy(&s);
    init(&s);
    const PlanPosition edge_region[]={{0,0},{5000,0},{5000,1000},{0,1000}};
    assert(slab_add_region(&s,edge_region,4,-50,50)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&s,0,0,5000,110,20)==SLAB_SUCCESS);
    assert(s.definition.edge_rebates.items[0].depth_mm==20);
    s.definition.regions.items[0].top_level_offset_mm=-75;
    assert(slab_validate(&s)==SLAB_SUCCESS && s.definition.edge_rebates.items[0].depth_mm==20);
    slab_destroy(&s);init(&s);
    /* Direct malformed state models a stale reference after an unsupported raw outline edit. */
    assert(slab_add_edge_rebate(&s,3,0,8000,110,20)==SLAB_SUCCESS);
    size_t old_count=s.definition.outline.vertex_count;
    s.definition.outline.vertex_count=3;
    assert(slab_validate(&s)==SLAB_EDGE_REBATE_INVALID_EDGE);
    s.definition.outline.vertex_count=1;
    assert(slab_validate(&s)==SLAB_INVALID_OUTLINE);
    s.definition.outline.vertex_count=old_count;assert(slab_validate(&s)==SLAB_SUCCESS);
    unsigned char before[sizeof s];memcpy(before,&s,sizeof s);
    assert(slab_build(s.id,s.definition.outline.vertices,2,100,0,&s)==SLAB_INVALID_OUTLINE);
    assert(memcmp(before,&s,sizeof s)==0);
    /* A successful full rebuild is replacement, so stale subordinate references
     * are discarded instead of being geometrically remapped. */
    assert(slab_build(s.id,s.definition.outline.vertices,3,100,0,&s)==SLAB_SUCCESS);
    assert(s.definition.edge_rebates.count==0&&slab_validate(&s)==SLAB_SUCCESS);slab_destroy(&s);
    const PlanPosition long_edge[]={{INT_MIN,0},{INT_MAX,0},{INT_MIN,1}};
    assert(slab_build(9,long_edge,3,100,0,&s)==SLAB_SUCCESS);
    reject(&s,0,0,1,1,1,SLAB_NUMERIC_OVERFLOW);
    int unchanged=77;
    assert(slab_edge_local_length_mm(&s.definition,0,&unchanged)==SLAB_NUMERIC_OVERFLOW && unchanged==77);
    slab_destroy(&s);
}

static void test_metadata_and_outputs(void)
{
    Slab s;init(&s);assert(slab_add_edge_rebate(&s,0,0,100,1,1)==SLAB_SUCCESS);
    SlabEdgeRebateCollection saved=s.definition.edge_rebates;
    SlabEdgeRebateCollection bad[]={{NULL,1,1},{saved.items,2,1},{saved.items,0,0},{saved.items,1,SIZE_MAX}};
    for(size_t i=0;i<4;i++) {
        s.definition.edge_rebates=bad[i];
        assert(slab_validate(&s)==(i==3?SLAB_NUMERIC_OVERFLOW:SLAB_INVALID_EDGE_REBATE_COLLECTION));
        assert(!slab_edge_rebate_at(&s,0));
    }
    s.definition.edge_rebates=saved;
    SlabEdgeRebate r={0,0,1,1,1};assert(slab_edge_rebate_validate(&s.definition,&r)==SLAB_SUCCESS);
    int output=77;assert(slab_edge_local_length_mm(&s.definition,99,&output)==SLAB_EDGE_REBATE_INVALID_EDGE && output==77);
    assert(slab_edge_rebate_length_mm(NULL,&output)==SLAB_INVALID_ARGUMENT && output==77);
    assert(slab_add_edge_rebate(NULL,0,0,1,1,1)==SLAB_INVALID_ARGUMENT);
    slab_destroy(&s);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocations(void)
{
    size_t adds=0,clones=0;
    for(size_t i=0;i<5;i++) {
        Slab s,before={0};init(&s);assert(slab_clone(&s,&before)==SLAB_SUCCESS);
        unsigned char bytes[sizeof s];memcpy(bytes,&s,sizeof s);fail_after=i;failed=0;
        SlabCode code=slab_add_edge_rebate(&s,0,0,100,1,1);fail_after=SIZE_MAX;
        if(failed){adds++;assert(code==SLAB_ALLOCATION_FAILED && memcmp(bytes,&s,sizeof s)==0);equal(&s,&before);assert(slab_add_edge_rebate(&s,0,0,100,1,1)==SLAB_SUCCESS);}
        else assert(code==SLAB_SUCCESS);
        slab_destroy(&s);slab_destroy(&before);if(!failed)break;
    }
    Slab source;init(&source);assert(slab_add_edge_rebate(&source,0,0,100,1,1)==SLAB_SUCCESS);
    for(size_t i=0;i<6;i++) {
        Slab s,before={0};init(&s);assert(slab_clone(&s,&before)==SLAB_SUCCESS);
        unsigned char bytes[sizeof s];memcpy(bytes,&s,sizeof s);fail_after=i;failed=0;
        SlabCode code=slab_clone(&source,&s);fail_after=SIZE_MAX;
        if(failed){clones++;assert(code==SLAB_ALLOCATION_FAILED && memcmp(bytes,&s,sizeof s)==0);equal(&s,&before);assert(slab_clone(&source,&s)==SLAB_SUCCESS);}
        else assert(code==SLAB_SUCCESS);
        equal(&source,&s);slab_destroy(&s);slab_destroy(&before);if(!failed)break;
    }
    assert(adds==1 && clones==2);slab_destroy(&source);
    printf("edge rebate allocation sweep: %zu add, %zu clone failures; retries passed\n",adds,clones);
}
#endif

int main(void)
{
    test_creation_lengths_and_lifetime();test_invalid_and_overlap();
    test_diagonal_corners_regions_and_stale_edges();test_metadata_and_outputs();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocations();
#endif
    puts("slab edge rebate tests passed");return 0;
}
