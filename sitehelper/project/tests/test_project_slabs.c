#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slab.h"
#include "sitehelper_persistence.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX;
static int failed;
void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t s);
void *__real_realloc(void *p, size_t n);
static int fail(void)
{
    if (fail_after != SIZE_MAX && fail_after-- == 0) { failed = 1; return 1; }
    return 0;
}
void *__wrap_malloc(size_t n) { return fail() ? NULL : __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t s) { return fail() ? NULL : __real_calloc(n,s); }
void *__wrap_realloc(void *p, size_t n) { return fail() ? NULL : __real_realloc(p,n); }
#endif

static const PlanPosition rectangle[] = {{-100,20},{9900,20},{9900,8020},{-100,8020}};
static const char *path = "project_slabs_test.txt";

static void fixture(SiteHelperProject *p)
{
    sitehelper_project_init(p);
    DomainId storey = sitehelper_project_add_storey(p, 3000);
    assert(storey == 1);
    assert(sitehelper_project_add_slab(p, storey, rectangle, 4, 100, -50) == 2);
}

static void expect_validation(const SiteHelperProject *p, SiteHelperProjectValidationCode code,
    DomainId subject, DomainId related)
{
    SiteHelperProjectValidation result = sitehelper_project_validate(p);
    assert(result.code == code && result.subject_id == subject && result.related_id == related);
}

static void test_identity_and_ownership(void)
{
    SiteHelperProject p, clone; fixture(&p);
    DomainId room = sitehelper_project_add_room(&p, 1);
    DomainId wall_id = sitehelper_project_add_wall(&p, 1, (WallPlanSegment){{0,0},{6000,0}});
    Wall *wall = sitehelper_project_find_wall_by_id(&p, wall_id);
    DomainId opening = domain_id_generate(&p.domain_ids);
    assert(wall_add_opening(wall, &p.settings, opening, OPENING_WINDOW, 1200, 900, 800, 1000));
    DomainId separator = sitehelper_project_add_room_separator(&p, 1, (PlanSegment){{0,0},{100,100}});
    DomainId level = sitehelper_project_add_storey(&p, 6000);
    DomainId other = sitehelper_project_add_slab(&p, level, rectangle, 4, 200, -100);
    assert(other && sitehelper_project_find_owning_storey(&p, other)->id == level);
    assert(sitehelper_project_find_owning_storey_const(&p, 2)->id == 1);
    assert(sitehelper_project_contains_domain_id(&p, other));
    assert(sitehelper_project_find_slab_by_id_const(&p, 2) == sitehelper_project_find_slab_by_id(&p, 2));
    assert(sitehelper_project_find_slab_by_id(&p, other)->definition.thickness_mm == 200);
    assert(!sitehelper_project_find_slab_by_id(&p, wall_id));
    assert(!sitehelper_project_find_slab_by_id(NULL, 2));
    assert(!sitehelper_project_find_slab_by_id_const(&p, 0));
    expect_validation(&p, SITEHELPER_PROJECT_VALID, 0, 0);
    test_clone_project_authoritative(&p, &clone);
    test_assert_project_authoritative_equal(&p, &clone);
    assert(clone.storeys[0].slabs.items != p.storeys[0].slabs.items);
    assert(clone.storeys[0].slabs.items[0].definition.outline.vertices != p.storeys[0].slabs.items[0].definition.outline.vertices);
    DomainId occupied[] = {1, room, wall_id, opening, separator, 2, other};
    DomainId next = p.domain_ids.next;
    Slab source = {0};
    assert(slab_build(100, rectangle, 4, 100, 0, &source) == SLAB_SUCCESS);
    for (size_t i = 0; i < sizeof occupied / sizeof *occupied; i++) {
        source.id = occupied[i];
        assert(!sitehelper_project_insert_slab(&p, level, &source));
        p.domain_ids.next = occupied[i];
        assert(!sitehelper_project_add_slab(&p, 1, rectangle, 4, 100, 0));
        assert(p.domain_ids.next == occupied[i]);
        p.domain_ids.next = next;
        /* Deliberately corrupt a slab identity: global validation catches every entity kind. */
        DomainId saved = p.storeys[1].slabs.items[0].id;
        if (occupied[i] != saved) {
            p.storeys[1].slabs.items[0].id = occupied[i];
            expect_validation(&p, SITEHELPER_PROJECT_DUPLICATE_ID, occupied[i], 0);
            p.storeys[1].slabs.items[0].id = saved;
        }
        test_assert_project_authoritative_equal(&clone, &p);
    }
    /* Existing project creators must also see slab identities. */
    p.domain_ids.next = 2;
    assert(!sitehelper_project_add_storey(&p, 0));
    assert(!sitehelper_project_add_room(&p, 1));
    assert(!sitehelper_project_add_wall(&p, 1, (WallPlanSegment){{0,0},{6000,0}}));
    assert(!sitehelper_project_add_room_separator(&p, 1, (PlanSegment){{0,0},{100,0}}));
    assert(p.domain_ids.next == 2);
    p.domain_ids.next = UINT64_MAX;
    assert(!sitehelper_project_add_slab(&p, 1, rectangle, 4, 100, 0));
    assert(p.domain_ids.next == UINT64_MAX);
    p.domain_ids.next = next;
    assert(!sitehelper_project_add_slab(&p, 0, rectangle, 4, 100, 0));
    assert(!sitehelper_project_add_slab(&p, 1, rectangle, 2, 100, 0));
    assert(!sitehelper_project_add_slab(NULL, 1, rectangle, 4, 100, 0));
    assert(!sitehelper_project_insert_slab(&p, 1, NULL));
    assert(!sitehelper_project_remove_slab_by_id(&p, wall_id));
    assert(!sitehelper_project_remove_slab_by_id(NULL, 2));
    test_assert_project_authoritative_equal(&clone, &p);
    assert(sitehelper_project_remove_slab_by_id(&p, 2));
    assert(!sitehelper_project_contains_domain_id(&p, 2));
    assert(!sitehelper_project_remove_slab_by_id(&p, 2));
    assert(sitehelper_project_find_slab_by_id(&p, other));
    source.id = 100;
    assert(sitehelper_project_insert_slab(&p, 1, &source));
    p.domain_ids.next = 101;
    source.definition.outline.vertices[0].x++;
    slab_destroy(&source);
    assert(p.storeys[0].slabs.items[0].definition.outline.vertices[0].x == -100);
    sitehelper_project_destroy(&p);
    /* Clone retains independent outlines after destruction of source. */
    assert(clone.storeys[0].slabs.items[0].definition.outline.vertices[0].x == -100);
    expect_validation(&clone, SITEHELPER_PROJECT_VALID, 0, 0);
    sitehelper_project_destroy(&clone); sitehelper_project_destroy(&p);
}

static void test_validation(void)
{
    SiteHelperProject p; fixture(&p);
    SlabCollection *c = &p.storeys[0].slabs;
    SlabCollection saved = *c; /* Non-owning test bookkeeping; restored before destruction. */
    SlabCollection bad[] = {{NULL,1,1},{saved.items,2,1},{saved.items,0,0}};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        *c = bad[i]; expect_validation(&p, SITEHELPER_PROJECT_INVALID_SLAB_COLLECTION, 1, 1);
        assert(!sitehelper_project_add_slab(&p, 1, rectangle, 4, 100, 0));
        *c = saved;
    }
    c->capacity = SIZE_MAX;
    expect_validation(&p, SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW, 1, 1); *c = saved;
    Slab *s = &c->items[0]; SlabOutline outline = s->definition.outline;
    SlabOutline bad_outlines[] = {{NULL,4,4},{outline.vertices,5,4},{outline.vertices,0,0}};
    for (size_t i = 0; i < sizeof bad_outlines / sizeof *bad_outlines; i++) {
        s->definition.outline = bad_outlines[i];
        expect_validation(&p, SITEHELPER_PROJECT_INVALID_SLAB_OUTLINE_COLLECTION, 2, 1);
        s->definition.outline = outline;
    }
    s->definition.outline.vertex_capacity = SIZE_MAX;
    expect_validation(&p, SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW, 2, 1);
    s->definition.outline = outline;
    s->id = 0; expect_validation(&p, SITEHELPER_PROJECT_INVALID_SLAB_ID, 0, 1); s->id = 2;
    s->definition.thickness_mm = 0;
    expect_validation(&p, SITEHELPER_PROJECT_INVALID_SLAB_THICKNESS, 2, 1);
    s->definition.thickness_mm = -100;
    expect_validation(&p, SITEHELPER_PROJECT_INVALID_SLAB_THICKNESS, 2, 1);
    s->definition.thickness_mm = 100;
    for (size_t n = 0; n < 3; n++) {
        s->definition.outline.vertex_count = n;
        expect_validation(&p, SITEHELPER_PROJECT_INVALID_SLAB_GEOMETRY, 2, 1);
    }
    s->definition.outline = outline;
    PlanPosition crossing[] = {{0,0},{4,4},{0,4},{4,0}};
    memcpy(outline.vertices, crossing, sizeof crossing);
    expect_validation(&p, SITEHELPER_PROJECT_INVALID_SLAB_GEOMETRY, 2, 1);
    PlanPosition extreme[] = {{INT_MIN,INT_MIN},{INT_MAX,INT_MIN},{INT_MAX,INT_MAX},{INT_MIN,INT_MAX}};
    memcpy(outline.vertices, extreme, sizeof extreme);
    expect_validation(&p, SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW, 2, 1);
    memcpy(outline.vertices, rectangle, sizeof rectangle);
    p.domain_ids.next = 2;
    expect_validation(&p, SITEHELPER_PROJECT_INVALID_ID_GENERATOR, 2, 0);
    p.domain_ids.next = 3;
    expect_validation(&p, SITEHELPER_PROJECT_VALID, 0, 0);
    sitehelper_project_destroy(&p);
}

static void test_round_trip(void)
{
    SiteHelperProject p, loaded; fixture(&p); sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_slab(&p, 1, rectangle, 4, 150, INT_MIN));
    DomainId level = sitehelper_project_add_storey(&p, 6000);
    const PlanPosition triangle[] = {{3,0},{0,1},{0,0}};
    DomainId last = sitehelper_project_add_slab(&p, level, triangle, 3, 3, 200);
    assert(last);
    assert(sitehelper_project_add_storey(&p, -100)); /* Empty Storey. */
    assert(sitehelper_project_save_file(&p, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    test_assert_project_authoritative_equal(&p, &loaded);
    assert(loaded.storeys[2].slabs.count == 0);
    SlabQuantities q;
    assert(slab_measure(&sitehelper_project_find_slab_by_id(&loaded,last)->definition, &q) == SLAB_SUCCESS);
    assert(q.area2_mm2 == 3 && q.volume2_mm3 == 9);
    DomainId next = loaded.domain_ids.next;
    assert(next > last && sitehelper_project_add_slab(&loaded, level, rectangle, 4, 100, 0) == next);
    FILE *f = fopen(path, "r"); assert(f); char text[4096];
    size_t n = fread(text, 1, sizeof text - 1, f); text[n] = '\0';
    assert(feof(f) && !ferror(f) && fclose(f) == 0);
    assert(strstr(text,"sitehelper_project 22\n") == text);
    assert(strstr(text,"slabs 2\n") && strstr(text,"slabs 0\n"));
    assert(strstr(text,"top_level_offset -50 thickness 100 outline 4"));
    assert(!strstr(text,"area") && !strstr(text,"volume") && !strstr(text,"perimeter"));
    /* Invalid authoritative slab state must fail before truncating a saved file. */
    p.storeys[0].slabs.items[0].definition.thickness_mm = 0;
    assert(sitehelper_project_save_file(&p,path) == SITEHELPER_PERSISTENCE_INVALID_PROJECT);
    p.storeys[0].slabs.items[0].definition.thickness_mm = 100;
    assert(sitehelper_project_load_file(&loaded,path) == SITEHELPER_PERSISTENCE_SUCCESS);
    test_assert_project_authoritative_equal(&p,&loaded);
    sitehelper_project_destroy(&p); sitehelper_project_destroy(&loaded);
    sitehelper_project_init(&p); sitehelper_project_init(&loaded);
    assert(sitehelper_project_save_file(&p,path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded,path) == SITEHELPER_PERSISTENCE_SUCCESS);
    test_assert_project_authoritative_equal(&p,&loaded);
    sitehelper_project_destroy(&p); sitehelper_project_destroy(&loaded);
}

static void test_legacy(void)
{
    for (int version = 1; version <= 10; version++) {
        FILE *f = fopen(path,"w"); assert(f);
        fprintf(f,"sitehelper_project %d\ndomain_id_next 2\nsettings 2400 90 35 600 1200 0 0 maximise\n",version);
        if (version >= 8) { fputs("storeys 1\nstorey 1 elevation 0\n",f); }
        if (version >= 9) { fputs("stud_height inherit\n",f); }
        if (version >= 3) { fputs("walls 0\n",f); }
        fputs("rooms 0\n",f);
        if (version >= 7) { fputs("room_separators 0\n",f); }
        if (version >= 8) { fputs("end_storey\n",f); }
        fputs("end_project\n",f); assert(fclose(f) == 0);
        SiteHelperProject p; fixture(&p);
        assert(sitehelper_project_load_file(&p,path) == SITEHELPER_PERSISTENCE_SUCCESS);
        assert(p.storey_count == 1 && !p.storeys[0].slabs.count && !p.storeys[0].slabs.items);
        sitehelper_project_destroy(&p);
    }
}

#define VALID_SLAB "slab 6 top_level_offset -50 thickness 100 outline 3\nvertex 0 0\nvertex 3 0\nvertex 0 1\nend_slab\n"
static void write_slab_input(const char *slabs)
{
    FILE *f = fopen(path,"w"); assert(f);
    fputs("sitehelper_project 11\ndomain_id_next 20\nsettings 2400 90 35 600 1200 0 0 maximise\n"
        "storeys 1\nstorey 1 elevation 0\nstud_height inherit\nwalls 1\n"
        "wall 2 segment 0 0 6000 0 openings 1\nopening 4 window 1200 900 800 1000 0 0 false\n"
        "rooms 1\nroom 3 placement unplaced\nend_room\n"
        "room_separators 1\nroom_separator 5 segment 0 0 100 0\n",f);
    fputs(slabs,f); fputs("end_storey\nend_project\n",f); assert(fclose(f) == 0);
}

static void assert_load_preserved(SiteHelperProject *p, const SiteHelperProject *expected)
{
    unsigned char before[sizeof *p]; memcpy(before,p,sizeof *p);
    Slab *items = p->storeys[0].slabs.items;
    PlanPosition *vertices = items[0].definition.outline.vertices;
    assert(sitehelper_project_load_file(p,path) != SITEHELPER_PERSISTENCE_SUCCESS);
    assert(memcmp(before,p,sizeof *p) == 0 && items == p->storeys[0].slabs.items);
    assert(vertices == items[0].definition.outline.vertices);
    test_assert_project_authoritative_equal(expected,p);
}

static void test_corrupt_persistence(void)
{
    const char *bad[] = {
        "", "slabs -1\n", "slabs word\n", "slabs 18446744073709551615\n", "slabs 1\nslab 6\n",
        "slabs 1\nslab 0 top_level_offset 0 thickness 100 outline 3\n",
        "slabs 1\nslab -6 top_level_offset 0 thickness 100 outline 3\n",
        "slabs 1\nslab 6 top_level_offset 2147483648 thickness 100 outline 3\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 0 outline 3\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness -1 outline 3\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 2\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 0\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 1\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline -3\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 18446744073709551615\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 3\nvertex 2147483648 0\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 3\nvertex 0 nope\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 3\nvertex 0 0\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 3\nvertex 0 0\nvertex 3 0\nvertex 0 1\n",
        "slabs 1\nslab 6 top_level_offset 0 thickness 100 outline 4\nvertex 0 0\nvertex 4 4\nvertex 0 4\nvertex 4 0\nend_slab\n",
        "slabs 2\n" VALID_SLAB VALID_SLAB,
        "slabs 1\n" VALID_SLAB "unexpected\n"
    };
    SiteHelperProject p, before; fixture(&p); test_clone_project_authoritative(&p,&before);
    /* First prove the test grammar is valid. */
    SiteHelperProject control; sitehelper_project_init(&control);
    write_slab_input("slabs 1\n" VALID_SLAB);
    assert(sitehelper_project_load_file(&control,path) == SITEHELPER_PERSISTENCE_SUCCESS);
    sitehelper_project_destroy(&control);
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        write_slab_input(bad[i]); assert_load_preserved(&p,&before);
    }
    for (DomainId id = 1; id <= 5; id++) {
        char text[256];
        snprintf(text,sizeof text,"slabs 1\nslab %" PRIu64 " top_level_offset 0 thickness 100 outline 3\n"
            "vertex 0 0\nvertex 3 0\nvertex 0 1\nend_slab\n",id);
        write_slab_input(text); assert_load_preserved(&p,&before);
    }
    /* Restored watermark must exceed slab identities, never silently collide. */
    write_slab_input("slabs 1\nslab 20 top_level_offset 0 thickness 100 outline 3\n"
        "vertex 0 0\nvertex 3 0\nvertex 0 1\nend_slab\n");
    assert_load_preserved(&p,&before);
    sitehelper_project_destroy(&p); sitehelper_project_destroy(&before);
}

static void test_later_storey_collisions_and_truncation(void)
{
    SiteHelperProject source, destination, before;
    fixture(&source); fixture(&destination); test_clone_project_authoritative(&destination,&before);
    DomainId level = sitehelper_project_add_storey(&source, 6000);
    DomainId room = sitehelper_project_add_room(&source,level);
    assert(room == 4 && sitehelper_project_save_file(&source,path) == SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *f = fopen(path,"r"); assert(f); char original[4096];
    size_t count = fread(original,1,sizeof original-1,f); original[count] = '\0';
    assert(feof(f) && !ferror(f) && fclose(f) == 0);
    /* A later Storey/object must reject an ID already restored by an earlier slab. */
    for (int kind = 0; kind < 2; kind++) {
        char text[4096]; memcpy(text,original,count+1);
        char *identity = strstr(text,kind == 0 ? "storey 3" : "room 4"); assert(identity);
        identity[kind == 0 ? 7 : 5] = '2';
        f = fopen(path,"w"); assert(f); assert(fputs(text,f) >= 0 && fclose(f) == 0);
        assert_load_preserved(&destination,&before);
    }
    const char *terminators[] = {"end_slab", "end_storey", "end_project"};
    for (size_t i = 0; i < sizeof terminators / sizeof *terminators; i++) {
        const char *end = strstr(original,terminators[i]); assert(end);
        f = fopen(path,"w"); assert(f);
        size_t length = (size_t)(end-original);
        assert(fwrite(original,1,length,f) == length && fclose(f) == 0);
        assert_load_preserved(&destination,&before);
    }
    sitehelper_project_destroy(&source); sitehelper_project_destroy(&destination); sitehelper_project_destroy(&before);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    size_t add_failures = 0, load_failures = 0;
    for (size_t index = 0; index < 32; index++) {
        SiteHelperProject p, before; fixture(&p); test_clone_project_authoritative(&p,&before);
        Slab *items = p.storeys[0].slabs.items; PlanPosition *vertices = items[0].definition.outline.vertices;
        fail_after = index; failed = 0;
        DomainId id = sitehelper_project_add_slab(&p,1,rectangle,4,100,0);
        fail_after = SIZE_MAX;
        if (failed) {
            add_failures++; assert(!id); test_assert_project_authoritative_equal(&p,&before);
            assert(p.storeys[0].slabs.items == items && items[0].definition.outline.vertices == vertices);
            assert(sitehelper_project_add_slab(&p,1,rectangle,4,100,0));
        } else { assert(id); }
        sitehelper_project_destroy(&p); sitehelper_project_destroy(&before);
        if (!failed) { break; }
    }
    SiteHelperProject source; fixture(&source);
    assert(sitehelper_project_add_slab(&source,1,rectangle,4,120,-20));
    DomainId level = sitehelper_project_add_storey(&source,6000);
    assert(sitehelper_project_add_slab(&source,level,rectangle,4,150,-100));
    assert(sitehelper_project_save_file(&source,path) == SITEHELPER_PERSISTENCE_SUCCESS);
    for (size_t index = 0; index < 64; index++) {
        SiteHelperProject p, before; fixture(&p); test_clone_project_authoritative(&p,&before);
        unsigned char bytes[sizeof p]; memcpy(bytes,&p,sizeof p);
        Slab *items = p.storeys[0].slabs.items; PlanPosition *vertices = items[0].definition.outline.vertices;
        fail_after = index; failed = 0;
        SiteHelperPersistenceResult result = sitehelper_project_load_file(&p,path);
        fail_after = SIZE_MAX;
        if (failed) {
            load_failures++; assert(result == SITEHELPER_PERSISTENCE_ALLOCATION_FAILED);
            assert(memcmp(bytes,&p,sizeof p) == 0);
            assert(p.storeys[0].slabs.items == items && items[0].definition.outline.vertices == vertices);
            test_assert_project_authoritative_equal(&before,&p);
            assert(sitehelper_project_load_file(&p,path) == SITEHELPER_PERSISTENCE_SUCCESS);
        } else { assert(result == SITEHELPER_PERSISTENCE_SUCCESS); }
        test_assert_project_authoritative_equal(&source,&p);
        sitehelper_project_destroy(&p); sitehelper_project_destroy(&before);
        if (!failed) { break; }
    }
    assert(add_failures == 2 && load_failures >= 8 && load_failures < 64);
    printf("allocation sweep: %zu add failures, %zu load failures; every retry succeeded\n", add_failures, load_failures);
    sitehelper_project_destroy(&source);
}
#endif

int main(void)
{
    test_identity_and_ownership(); test_validation(); test_round_trip(); test_legacy(); test_corrupt_persistence();
    test_later_storey_collisions_and_truncation();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    assert(remove(path) == 0);
    puts("project slab identity, validation and persistence tests passed");
    return 0;
}
