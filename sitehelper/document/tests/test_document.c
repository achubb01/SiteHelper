#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "document.h"
#include "document_plan_query.h"
#include "plan_dimension_geometry.h"

static DocumentAnnotation note(DomainId id, DomainId storey, DomainId target, const char *text)
{
    return (DocumentAnnotation){
        .id = id,
        .kind = DOCUMENT_ANNOTATION_NOTE,
        .anchor = {.storey_id = storey, .position = {123, -456}},
        .target_id = target,
        .text = (char *)text
    };
}

static void test_insert_deep_copies_and_lookup(void)
{
    DocumentModel document;
    document_model_init(&document);
    char text[] = "Check flashing";
    DocumentAnnotation source = note(7, 2, 0, text);
    assert(document_model_insert_annotation(&document, &source) == DOCUMENT_SUCCESS);
    text[0] = 'X';
    const DocumentAnnotation *stored = document_model_find_annotation_by_id_const(&document, 7);
    assert(stored != NULL);
    assert(strcmp(stored->text, "Check flashing") == 0);
    assert(stored->anchor.position.x == 123 && stored->anchor.position.y == -456);
    assert(document_model_insert_annotation(&document, &source) == DOCUMENT_DUPLICATE_ID);
    document_model_destroy(&document);
}

static void test_local_validation_and_remove(void)
{
    DocumentModel document;
    document_model_init(&document);
    DocumentAnnotation a = note(1, 2, 0, "A");
    assert(document_annotation_is_locally_valid(&a));
    a.id = 0; assert(!document_annotation_is_locally_valid(&a)); a.id = 1;
    a.anchor.storey_id = 0; assert(!document_annotation_is_locally_valid(&a)); a.anchor.storey_id = 2;
    a.kind = (DocumentAnnotationKind)99; assert(!document_annotation_is_locally_valid(&a)); a.kind = DOCUMENT_ANNOTATION_NOTE;
    a.text = ""; assert(!document_annotation_is_locally_valid(&a)); a.text = "A";
    assert(document_model_insert_annotation(&document, &a) == DOCUMENT_SUCCESS);
    DocumentAnnotation b = note(3, 2, 0, "B");
    assert(document_model_insert_annotation(&document, &b) == DOCUMENT_SUCCESS);
    assert(document_model_remove_annotation(&document, 1) == DOCUMENT_SUCCESS);
    assert(document.annotation_count == 1);
    assert(document.annotations[0].id == 3);
    assert(document_model_remove_annotation(&document, 1) == DOCUMENT_NOT_FOUND);
    document_model_destroy(&document);
    document_model_destroy(&document);
}

static void test_plan_query_storey_scope_and_ties(void)
{
    DocumentModel document; document_model_init(&document);
    DocumentAnnotation first=note(1,2,0,"First");
    first.anchor.position=(PlanPosition){100,100};
    DocumentAnnotation second=note(2,2,0,"Second");
    second.anchor.position=(PlanPosition){100,100};
    DocumentAnnotation other=note(3,9,0,"Other");
    other.anchor.position=(PlanPosition){100,100};
    assert(document_model_insert_annotation(&document,&first)==DOCUMENT_SUCCESS);
    assert(document_model_insert_annotation(&document,&second)==DOCUMENT_SUCCESS);
    assert(document_model_insert_annotation(&document,&other)==DOCUMENT_SUCCESS);
    assert(document_plan_find_note_at_position(&document,2,(PlanPoint){100,100},0)==2);
    assert(document_plan_find_note_at_position(&document,2,(PlanPoint){110,100},9)==0);
    assert(document_plan_find_note_at_position(&document,2,(PlanPoint){110,100},10)==2);
    assert(document_plan_find_note_at_position(&document,9,(PlanPoint){100,100},0)==3);
    document_model_destroy(&document);
}


static void test_dimension_collection(void)
{
    DocumentModel document; document_model_init(&document);
    DocumentPlanDimension dimension = {
        .id = 10, .storey_id = 2,
        .first = {.kind = DOCUMENT_DIMENSION_FIXED_POINT, .position = {0, 0}},
        .second = {.kind = DOCUMENT_DIMENSION_WALL_END, .target_id = 7},
        .offset_mm = -250
    };
    assert(document_plan_dimension_is_locally_valid(&dimension));
    assert(document_model_insert_dimension(&document, &dimension) == DOCUMENT_SUCCESS);
    assert(document_model_insert_dimension(&document, &dimension) == DOCUMENT_DUPLICATE_ID);
    const DocumentPlanDimension *stored = document_model_find_dimension_by_id_const(&document, 10);
    assert(stored && stored->second.target_id == 7 && stored->offset_mm == -250);
    dimension.first.target_id = 99;
    assert(!document_plan_dimension_is_locally_valid(&dimension));
    assert(document_model_remove_dimension(&document, 10) == DOCUMENT_SUCCESS);
    assert(document.dimension_count == 0);
    assert(document_model_remove_dimension(&document, 10) == DOCUMENT_NOT_FOUND);
    document_model_destroy(&document);
}


static void test_symbol_collection_and_plan_query(void)
{
    DocumentModel document;
    document_model_init(&document);

    DocumentPlanSymbol first = {
        .id = 20,
        .storey_id = 2,
        .kind = DOCUMENT_PLAN_SYMBOL_POINT_MARKER,
        .anchor = {100, 100}
    };
    DocumentPlanSymbol second = {
        .id = 21,
        .storey_id = 2,
        .kind = DOCUMENT_PLAN_SYMBOL_POINT_MARKER,
        .anchor = {100, 100}
    };
    DocumentPlanSymbol other_storey = {
        .id = 22,
        .storey_id = 9,
        .kind = DOCUMENT_PLAN_SYMBOL_POINT_MARKER,
        .anchor = {100, 100}
    };
    DocumentPlanSymbol directional = {
        .id = 23,
        .storey_id = 2,
        .kind = DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION,
        .anchor = {200, 200},
        .direction = {3, 4}
    };

    assert(document_plan_symbol_is_locally_valid(&first));
    assert(document_model_insert_symbol(&document, &first) == DOCUMENT_SUCCESS);
    assert(document_model_insert_symbol(&document, &first) == DOCUMENT_DUPLICATE_ID);
    assert(document_model_insert_symbol(&document, &second) == DOCUMENT_SUCCESS);
    assert(document_model_insert_symbol(&document, &other_storey) == DOCUMENT_SUCCESS);
    assert(document_plan_symbol_is_locally_valid(&directional));
    assert(document_model_insert_symbol(&document, &directional) == DOCUMENT_SUCCESS);

    const DocumentPlanSymbol *stored = document_model_find_symbol_by_id_const(&document, 20);
    assert(stored != NULL);
    assert(stored->anchor.x == 100 && stored->anchor.y == 100);

    assert(document_plan_find_symbol_at_position(
        &document, 2, (PlanPoint){100, 100}, 0.0) == 21);
    assert(document_plan_find_symbol_at_position(
        &document, 2, (PlanPoint){110, 100}, 9.0) == DOMAIN_ID_INVALID);
    assert(document_plan_find_symbol_at_position(
        &document, 2, (PlanPoint){110, 100}, 10.0) == 21);
    assert(document_plan_find_symbol_at_position(
        &document, 9, (PlanPoint){100, 100}, 0.0) == 22);
    assert(document_plan_find_symbol_at_position(
        &document, 2, (PlanPoint){200, 200}, 0.0) == 23);
    directional.direction=(DocumentPlanDirection){6,8};
    assert(!document_plan_symbol_is_locally_valid(&directional));

    first.kind = (DocumentPlanSymbolKind)99;
    assert(!document_plan_symbol_is_locally_valid(&first));
    assert(document_model_remove_symbol(&document, 20) == DOCUMENT_SUCCESS);
    assert(document_model_find_symbol_by_id_const(&document, 20) == NULL);
    assert(document_model_remove_symbol(&document, 20) == DOCUMENT_NOT_FOUND);

    document_model_destroy(&document);
}


static void test_callout_collection_and_plan_query(void)
{
    DocumentModel document; document_model_init(&document);
    char text[]="Leader note";
    DocumentPlanCallout first={.id=30,.storey_id=2,.target={0,0},.label_anchor={1000,0},.text=text};
    DocumentPlanCallout second={.id=31,.storey_id=2,.target={0,0},.label_anchor={1000,0},.text="Second"};
    DocumentPlanCallout other={.id=32,.storey_id=9,.target={0,0},.label_anchor={1000,0},.text="Other"};
    assert(document_plan_callout_is_locally_valid(&first));
    assert(document_model_insert_callout(&document,&first)==DOCUMENT_SUCCESS);
    text[0]='X';
    const DocumentPlanCallout *stored=document_model_find_callout_by_id_const(&document,30);
    assert(stored&&strcmp(stored->text,"Leader note")==0);
    assert(document_model_insert_callout(&document,&second)==DOCUMENT_SUCCESS);
    assert(document_model_insert_callout(&document,&other)==DOCUMENT_SUCCESS);
    assert(document_plan_find_callout_at_position(&document,2,(PlanPoint){500,0},0.0)==31);
    assert(document_plan_find_callout_at_position(&document,2,(PlanPoint){500,11},10.0)==DOMAIN_ID_INVALID);
    assert(document_plan_find_callout_at_position(&document,2,(PlanPoint){500,10},10.0)==31);
    assert(document_plan_find_callout_at_position(&document,9,(PlanPoint){500,0},0.0)==32);
    first.label_anchor=first.target;
    assert(!document_plan_callout_is_locally_valid(&first));
    assert(document_model_remove_callout(&document,30)==DOCUMENT_SUCCESS);
    assert(document_model_find_callout_by_id_const(&document,30)==NULL);
    document_model_destroy(&document);
}



static void test_revision_cloud_collection_and_plan_query(void)
{
    DocumentModel document; document_model_init(&document);
    PlanPosition a_vertices[]={{0,0},{1000,0},{1000,800},{0,800}};
    PlanPosition b_vertices[]={{0,0},{1000,0},{900,700},{0,700}};
    DocumentPlanRevisionCloud a={.id=50,.storey_id=2,.vertices=a_vertices,.vertex_count=4};
    DocumentPlanRevisionCloud b={.id=51,.storey_id=2,.vertices=b_vertices,.vertex_count=4};
    assert(document_plan_revision_cloud_is_locally_valid(&a));
    assert(document_model_insert_revision_cloud(&document,&a)==DOCUMENT_SUCCESS);
    assert(document_model_insert_revision_cloud(&document,&b)==DOCUMENT_SUCCESS);
    a_vertices[0].x=99;
    const DocumentPlanRevisionCloud *stored=document_model_find_revision_cloud_by_id_const(&document,50);
    assert(stored&&stored->vertices[0].x==0);
    /* Exact overlap tie on the first edge chooses later-authored b. */
    assert(document_plan_find_revision_cloud_at_position(&document,2,(PlanPoint){500,0},0.0)==51);
    assert(document_plan_find_revision_cloud_at_position(&document,3,(PlanPoint){500,0},20.0)==DOMAIN_ID_INVALID);
    assert(document_plan_find_revision_cloud_at_position(&document,2,(PlanPoint){500,25},20.0)==DOMAIN_ID_INVALID);
    PlanPosition invalid[]={{0,0},{100,0},{0,0}};
    DocumentPlanRevisionCloud bad={.id=52,.storey_id=2,.vertices=invalid,.vertex_count=3};
    assert(!document_plan_revision_cloud_is_locally_valid(&bad));
    document_model_destroy(&document);
}

static void test_revision_collection_deep_copy(void)
{
    DocumentModel document; document_model_init(&document);
    char identifier[]="A"; char description[]="First issue";
    DocumentRevision revision={.id=60,.identifier=identifier,.description=description};
    assert(document_revision_is_locally_valid(&revision));
    assert(document_model_insert_revision(&document,&revision)==DOCUMENT_SUCCESS);
    identifier[0]='B'; description[0]='X';
    const DocumentRevision *stored=document_model_find_revision_by_id_const(&document,60);
    assert(stored&&strcmp(stored->identifier,"A")==0&&strcmp(stored->description,"First issue")==0);
    assert(document_model_insert_revision(&document,&revision)==DOCUMENT_DUPLICATE_ID);
    DocumentRevision bad={.id=61,.identifier="",.description=""};
    assert(!document_revision_is_locally_valid(&bad));
    assert(document_model_remove_revision(&document,60)==DOCUMENT_SUCCESS);
    assert(document_model_find_revision_by_id_const(&document,60)==NULL);
    document_model_destroy(&document);
}

static void test_document_object_ref_storey_scope(void)
{
    DocumentModel document; document_model_init(&document);
    DocumentAnnotation n=note(40,2,0,"N");
    DocumentPlanDimension d={.id=41,.storey_id=3,
        .first={.kind=DOCUMENT_DIMENSION_FIXED_POINT,.position={0,0}},
        .second={.kind=DOCUMENT_DIMENSION_FIXED_POINT,.position={100,0}}};
    DocumentPlanSymbol s={.id=42,.storey_id=4,.kind=DOCUMENT_PLAN_SYMBOL_POINT_MARKER,
        .anchor={5,6}};
    DocumentPlanCallout c={.id=43,.storey_id=5,.target={0,0},
        .label_anchor={100,100},.text="C"};
    PlanPosition cloud_vertices[]={{0,0},{100,0},{100,100},{0,100}};
    DocumentPlanRevisionCloud r={.id=44,.storey_id=6,.vertices=cloud_vertices,.vertex_count=4};
    assert(document_model_insert_annotation(&document,&n)==DOCUMENT_SUCCESS);
    assert(document_model_insert_dimension(&document,&d)==DOCUMENT_SUCCESS);
    assert(document_model_insert_symbol(&document,&s)==DOCUMENT_SUCCESS);
    assert(document_model_insert_callout(&document,&c)==DOCUMENT_SUCCESS);
    assert(document_model_insert_revision_cloud(&document,&r)==DOCUMENT_SUCCESS);

    assert(document_object_ref_is_valid((DocumentObjectRef){DOCUMENT_OBJECT_NOTE,40}));
    assert(!document_object_ref_is_valid((DocumentObjectRef){DOCUMENT_OBJECT_NONE,40}));
    assert(!document_object_ref_is_valid((DocumentObjectRef){DOCUMENT_OBJECT_NOTE,0}));
    assert(document_model_object_storey_id(&document,
        (DocumentObjectRef){DOCUMENT_OBJECT_NOTE,40})==2);
    assert(document_model_object_storey_id(&document,
        (DocumentObjectRef){DOCUMENT_OBJECT_DIMENSION,41})==3);
    assert(document_model_object_storey_id(&document,
        (DocumentObjectRef){DOCUMENT_OBJECT_SYMBOL,42})==4);
    assert(document_model_object_storey_id(&document,
        (DocumentObjectRef){DOCUMENT_OBJECT_CALLOUT,43})==5);
    assert(document_model_object_storey_id(&document,
        (DocumentObjectRef){DOCUMENT_OBJECT_REVISION_CLOUD,44})==6);
    assert(document_model_object_storey_id(&document,
        (DocumentObjectRef){DOCUMENT_OBJECT_NOTE,999})==DOMAIN_ID_INVALID);
    assert(document_model_object_storey_id(NULL,
        (DocumentObjectRef){DOCUMENT_OBJECT_NOTE,40})==DOMAIN_ID_INVALID);
    document_model_destroy(&document);
}

static void test_dimension_drafting_geometry(void)
{
    DocumentPlanDimensionGeometry geometry;
    assert(document_plan_dimension_geometry((PlanPosition){0,0},(PlanPosition){1000,0},200,&geometry));
    assert(geometry.line_first.x==0.0&&geometry.line_first.y==200.0);
    assert(geometry.line_second.x==1000.0&&geometry.line_second.y==200.0);
    assert(geometry.midpoint.x==500.0&&geometry.midpoint.y==200.0);
    assert(document_plan_dimension_hit_distance(&geometry,(PlanPoint){500,200})==0.0);
    assert(document_plan_dimension_hit_distance(&geometry,(PlanPoint){0,100})==0.0);
    assert(document_plan_dimension_hit_distance(&geometry,(PlanPoint){500,250})==50.0);
    assert(!document_plan_dimension_geometry((PlanPosition){0,0},(PlanPosition){0,0},200,&geometry));
}

int main(void)
{
    test_insert_deep_copies_and_lookup();
    test_local_validation_and_remove();
    test_plan_query_storey_scope_and_ties();
    test_dimension_collection();
    test_symbol_collection_and_plan_query();
    test_callout_collection_and_plan_query();
    test_revision_cloud_collection_and_plan_query();
    test_revision_collection_deep_copy();
    test_document_object_ref_storey_scope();
    test_dimension_drafting_geometry();
    puts("All document model tests passed.");
    return 0;
}
