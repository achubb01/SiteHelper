#ifndef SITEHELPER_TEST_SUPPORT_H
#define SITEHELPER_TEST_SUPPORT_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sitehelper_project.h"
#include "wall.h"
#include "roof.h"

static inline int test_timber_equal_value(
    const Timber *expected,
    const Timber *actual
)
{
    if (expected == NULL || actual == NULL) {
        return expected == actual;
    }

    if (expected->length != actual->length ||
        expected->depth != actual->depth ||
        expected->width != actual->width ||
        expected->position.u != actual->position.u ||
        expected->position.z != actual->position.z ||
        expected->type != actual->type) {
        return 0;
    }

    switch (expected->type) {
        case TIMBER_STUD:
            return expected->details.stud.type == actual->details.stud.type;

        case TIMBER_NOGGIN:
            return expected->details.noggin.bay == actual->details.noggin.bay;

        case TIMBER_PLATE:
            return expected->details.plate.placeholder ==
                actual->details.plate.placeholder;

        case TIMBER_HEADER:
        case TIMBER_SILL:
        default:
            return 1;
    }
}

static inline void test_assert_timber_equal(
    const Timber *expected,
    const Timber *actual
)
{
    assert(test_timber_equal_value(expected, actual));
}

static inline size_t test_timber_occurrences(
    const Timber *members,
    size_t member_count,
    const Timber *target
)
{
    size_t occurrences = 0;

    for (size_t i = 0; i < member_count; i++) {
        if (test_timber_equal_value(&members[i], target)) {
            occurrences++;
        }
    }

    return occurrences;
}

static inline void test_assert_timber_multiset_equal(
    const Timber *expected,
    size_t expected_count,
    const Timber *actual,
    size_t actual_count
)
{
    assert(expected_count == actual_count);

    for (size_t i = 0; i < expected_count; i++) {
        assert(
            test_timber_occurrences(
                expected,
                expected_count,
                &expected[i]
            ) ==
            test_timber_occurrences(
                actual,
                actual_count,
                &expected[i]
            )
        );
    }
}

static inline void test_assert_framing_semantically_equal(
    const WallFraming *expected,
    const WallFraming *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    test_assert_timber_equal(
        &expected->bottomplate,
        &actual->bottomplate
    );

    test_assert_timber_equal(
        &expected->topplate,
        &actual->topplate
    );

    test_assert_timber_multiset_equal(
        expected->studs,
        expected->stud_count,
        actual->studs,
        actual->stud_count
    );

    test_assert_timber_multiset_equal(
        expected->nogs,
        expected->nog_count,
        actual->nogs,
        actual->nog_count
    );

    test_assert_timber_multiset_equal(
        expected->members,
        expected->member_count,
        actual->members,
        actual->member_count
    );
}

static inline void test_assert_build_settings_equal(
    const BuildSettings *expected,
    const BuildSettings *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    assert(expected->stud_height == actual->stud_height);
    assert(expected->stud_depth == actual->stud_depth);
    assert(expected->stud_width == actual->stud_width);
    assert(expected->stud_spacing == actual->stud_spacing);
    assert(expected->nog_spacing == actual->nog_spacing);
    assert(expected->opening_width_allowance == actual->opening_width_allowance);
    assert(expected->opening_height_allowance == actual->opening_height_allowance);
    assert(expected->stud_spacing_mode == actual->stud_spacing_mode);
}

static inline void test_assert_opening_equal(
    const Opening *expected,
    const Opening *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    assert(expected->id == actual->id);
    assert(expected->type == actual->type);
    assert(expected->frame_position == actual->frame_position);
    assert(expected->frame_bottom == actual->frame_bottom);
    assert(expected->width == actual->width);
    assert(expected->height == actual->height);
    assert(expected->width_allowance == actual->width_allowance);
    assert(expected->height_allowance == actual->height_allowance);
    assert(expected->custom_allowance == actual->custom_allowance);
}

static inline void test_assert_wall_definition_equal(
    const Wall *expected,
    const Wall *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    assert(expected->id == actual->id);
    assert(expected->definition.segment.start.x == actual->definition.segment.start.x);
    assert(expected->definition.segment.start.y == actual->definition.segment.start.y);
    assert(expected->definition.segment.end.x == actual->definition.segment.end.x);
    assert(expected->definition.segment.end.y == actual->definition.segment.end.y);
    assert(expected->definition.plan_specification.thickness_mm == actual->definition.plan_specification.thickness_mm);
    assert(expected->definition.plan_specification.alignment == actual->definition.plan_specification.alignment);
    assert(expected->definition.opening_count == actual->definition.opening_count);

    for (size_t i = 0; i < expected->definition.opening_count; i++) {
        const Opening *expected_opening = &expected->definition.openings[i];
        const Opening *actual_opening = wall_find_opening_by_id_const(
            actual,
            expected_opening->id
        );

        assert(actual_opening != NULL);
        test_assert_opening_equal(expected_opening, actual_opening);
    }
}

static inline void test_assert_slab_equal(const Slab *expected, const Slab *actual)
{
    assert(expected && actual && expected->id == actual->id);
    assert(expected->definition.thickness_mm == actual->definition.thickness_mm);
    assert(expected->definition.top_level_offset_mm == actual->definition.top_level_offset_mm);
    const SlabOutline *a = &expected->definition.outline, *b = &actual->definition.outline;
    assert(a->vertex_count == b->vertex_count);
    for (size_t i = 0; i < a->vertex_count; i++) {
        assert(a->vertices[i].x == b->vertices[i].x && a->vertices[i].y == b->vertices[i].y);
    }
    const SlabPenetrationCollection *pa = &expected->definition.penetrations, *pb = &actual->definition.penetrations;
    assert(pa->count == pb->count);
    for (size_t i = 0; i < pa->count; i++) {
        const SlabOutline *oa = &pa->items[i].outline, *ob = &pb->items[i].outline;
        assert(oa->vertex_count == ob->vertex_count);
        for (size_t j = 0; j < oa->vertex_count; j++) {
            assert(oa->vertices[j].x == ob->vertices[j].x && oa->vertices[j].y == ob->vertices[j].y);
        }
    }
    const SlabRegionCollection *ra = &expected->definition.regions, *rb = &actual->definition.regions;
    assert(ra->count == rb->count);
    for (size_t i = 0; i < ra->count; i++) {
        assert(ra->items[i].top_level_offset_mm == rb->items[i].top_level_offset_mm);
        assert(ra->items[i].thickness_mm == rb->items[i].thickness_mm);
        const SlabOutline *oa = &ra->items[i].outline, *ob = &rb->items[i].outline;
        assert(oa->vertex_count == ob->vertex_count);
        for (size_t j = 0; j < oa->vertex_count; j++) {
            assert(oa->vertices[j].x == ob->vertices[j].x && oa->vertices[j].y == ob->vertices[j].y);
        }
    }
    const SlabEdgeRebateCollection *ea=&expected->definition.edge_rebates;
    const SlabEdgeRebateCollection *eb=&actual->definition.edge_rebates;
    assert(ea->count == eb->count);
    for (size_t i=0; i<ea->count; i++) {
        const SlabEdgeRebate *a=&ea->items[i], *b=&eb->items[i];
        assert(a->edge_index==b->edge_index && a->start_offset_mm==b->start_offset_mm &&
            a->end_offset_mm==b->end_offset_mm && a->width_mm==b->width_mm &&
            a->depth_mm==b->depth_mm);
    }
}


static inline void test_assert_roof_equal(const Roof *expected, const Roof *actual)
{
    assert(expected && actual && expected->id == actual->id);
    const RoofDefinition *a=&expected->definition,*b=&actual->definition;
    assert(a->portion_count==b->portion_count && a->composition_count==b->composition_count &&
        a->termination_count==b->termination_count);
    for(size_t i=0;i<a->portion_count;i++){
        const RoofPortionDefinition *pa=&a->portions[i];
        const RoofPortionDefinition *pb=roof_find_portion_by_id_const(actual,pa->id);
        assert(pb && pa->support_vertex_count==pb->support_vertex_count && pa->generation==pb->generation &&
            pa->slope_ppm==pb->slope_ppm && pa->reference_z_mm==pb->reference_z_mm &&
            pa->direction.x==pb->direction.x && pa->direction.y==pb->direction.y &&
            pa->single_slope_reference==pb->single_slope_reference);
        for(size_t j=0;j<pa->support_vertex_count;j++)
            assert(pa->support_vertices[j].x==pb->support_vertices[j].x && pa->support_vertices[j].y==pb->support_vertices[j].y);
    }
    for(size_t i=0;i<a->composition_count;i++){
        assert(a->compositions[i].first_portion_id==b->compositions[i].first_portion_id &&
            a->compositions[i].second_portion_id==b->compositions[i].second_portion_id &&
            a->compositions[i].kind==b->compositions[i].kind);
    }
    for(size_t i=0;i<a->termination_count;i++){
        assert(a->terminations[i].portion_id==b->terminations[i].portion_id &&
            a->terminations[i].end==b->terminations[i].end &&
            a->terminations[i].termination_offset_mm==b->terminations[i].termination_offset_mm);
    }
}

static inline void test_assert_project_model_equal(
    const SiteHelperProject *expected,
    const SiteHelperProject *actual
)
{
    assert(expected != NULL);
    assert(actual != NULL);

    test_assert_build_settings_equal(&expected->settings, &actual->settings);
    assert(expected->storey_count == actual->storey_count);
    for (size_t level = 0; level < expected->storey_count; level++) {
        assert(expected->storeys[level].id == actual->storeys[level].id);
        assert(expected->storeys[level].slabs.count == actual->storeys[level].slabs.count);
        for (size_t i = 0; i < expected->storeys[level].slabs.count; i++) {
            test_assert_slab_equal(&expected->storeys[level].slabs.items[i], &actual->storeys[level].slabs.items[i]);
        }
        assert(expected->storeys[level].roofs.count == actual->storeys[level].roofs.count);
        for (size_t i = 0; i < expected->storeys[level].roofs.count; i++) {
            const Roof *er=&expected->storeys[level].roofs.items[i];
            const Roof *ar=roof_collection_find_by_id_const(&actual->storeys[level].roofs,er->id);
            assert(ar); test_assert_roof_equal(er,ar);
        }
        assert(expected->storeys[level].elevation_mm == actual->storeys[level].elevation_mm);
        assert(expected->storeys[level].settings.has_stud_height_override == actual->storeys[level].settings.has_stud_height_override);
        assert(expected->storeys[level].settings.stud_height == actual->storeys[level].settings.stud_height);
        assert(expected->storeys[level].structure.room_count == actual->storeys[level].structure.room_count);
        assert(expected->storeys[level].structure.wall_count == actual->storeys[level].structure.wall_count);
        assert(expected->storeys[level].structure.room_separator_count == actual->storeys[level].structure.room_separator_count);
        for (size_t i = 0; i < expected->storeys[level].structure.room_separator_count; i++) {
            const RoomSeparator *a = &expected->storeys[level].structure.room_separators[i];
            const RoomSeparator *b = &actual->storeys[level].structure.room_separators[i];
            assert(a->id == b->id);
            assert(a->segment.start.x == b->segment.start.x && a->segment.start.y == b->segment.start.y);
            assert(a->segment.end.x == b->segment.end.x && a->segment.end.y == b->segment.end.y);
        }

        for (size_t i = 0; i < expected->storeys[level].structure.room_count; i++) {
            const Room *expected_room = &expected->storeys[level].structure.rooms[i];
            const Room *actual_room = build_find_room_by_id_const(
                &actual->storeys[level].structure,
                expected_room->id
            );

            assert(actual_room != NULL);
            assert(expected_room->has_location == actual_room->has_location);
            if (expected_room->has_location) {
                assert(expected_room->location.x == actual_room->location.x);
                assert(expected_room->location.y == actual_room->location.y);
            }
        }

        for (size_t i = 0; i < expected->storeys[level].structure.wall_count; i++) {
            const Wall *expected_wall = &expected->storeys[level].structure.walls[i];
            const Wall *actual_wall = build_find_wall_by_id_const(
                &actual->storeys[level].structure,
                expected_wall->id
            );

            assert(actual_wall != NULL);
            test_assert_wall_definition_equal(expected_wall, actual_wall);
        }
    }
    assert(expected->document.annotation_count == actual->document.annotation_count);
    for (size_t i = 0; i < expected->document.annotation_count; i++) {
        const DocumentAnnotation *a = &expected->document.annotations[i];
        const DocumentAnnotation *b = sitehelper_project_find_annotation_by_id_const(actual, a->id);
        assert(b != NULL);
        assert(a->kind == b->kind);
        assert(a->anchor.storey_id == b->anchor.storey_id);
        assert(a->anchor.position.x == b->anchor.position.x);
        assert(a->anchor.position.y == b->anchor.position.y);
        assert(a->target_id == b->target_id);
        assert(a->text != NULL && b->text != NULL && strcmp(a->text, b->text) == 0);
    }
    assert(expected->document.dimension_count == actual->document.dimension_count);
    for (size_t i = 0; i < expected->document.dimension_count; i++) {
        const DocumentPlanDimension *a = &expected->document.dimensions[i];
        const DocumentPlanDimension *b = sitehelper_project_find_dimension_by_id_const(actual, a->id);
        assert(b != NULL);
        assert(a->storey_id == b->storey_id && a->offset_mm == b->offset_mm);
        assert(memcmp(&a->first, &b->first, sizeof a->first) == 0);
        assert(memcmp(&a->second, &b->second, sizeof a->second) == 0);
    }
    assert(expected->document.symbol_count == actual->document.symbol_count);
    for (size_t i = 0; i < expected->document.symbol_count; i++) {
        const DocumentPlanSymbol *a = &expected->document.symbols[i];
        const DocumentPlanSymbol *b = sitehelper_project_find_symbol_by_id_const(actual, a->id);
        assert(b != NULL);
        assert(a->storey_id == b->storey_id && a->kind == b->kind);
        assert(a->anchor.x == b->anchor.x && a->anchor.y == b->anchor.y);
    }
    assert(expected->document.callout_count == actual->document.callout_count);
    for (size_t i=0;i<expected->document.callout_count;i++) {
        const DocumentPlanCallout *a=&expected->document.callouts[i];
        const DocumentPlanCallout *b=sitehelper_project_find_callout_by_id_const(actual,a->id);
        assert(b != NULL);
        assert(a->storey_id == b->storey_id);
        assert(a->target.x == b->target.x && a->target.y == b->target.y);
        assert(a->label_anchor.x == b->label_anchor.x &&
            a->label_anchor.y == b->label_anchor.y);
        assert(a->text != NULL && b->text != NULL && strcmp(a->text,b->text) == 0);
    }
    assert(expected->document.revision_cloud_count == actual->document.revision_cloud_count);
    for (size_t i=0;i<expected->document.revision_cloud_count;i++) {
        const DocumentPlanRevisionCloud *a=&expected->document.revision_clouds[i];
        const DocumentPlanRevisionCloud *b=
            sitehelper_project_find_revision_cloud_by_id_const(actual,a->id);
        assert(b != NULL);
        assert(a->storey_id == b->storey_id);
        assert(a->vertex_count == b->vertex_count);
        assert(a->vertex_count == 0 ||
            memcmp(a->vertices,b->vertices,a->vertex_count*sizeof *a->vertices) == 0);
    }
}

static inline void test_assert_project_authoritative_equal(
    const SiteHelperProject *expected,
    const SiteHelperProject *actual
)
{
    test_assert_project_model_equal(expected, actual);
    assert(expected->domain_ids.next == actual->domain_ids.next);
}

static inline void test_clone_wall_definition(
    const Wall *source,
    Wall *destination
)
{
    assert(source != NULL);
    assert(destination != NULL);

    *destination = (Wall){
        .id = source->id,
        .definition = {
            .segment = source->definition.segment,
            .plan_specification = source->definition.plan_specification
        }
    };

    if (source->definition.opening_count == 0) {
        return;
    }

    destination->definition.openings = malloc(
        source->definition.opening_count *
        sizeof *destination->definition.openings
    );

    assert(destination->definition.openings != NULL);

    memcpy(
        destination->definition.openings,
        source->definition.openings,
        source->definition.opening_count *
        sizeof *destination->definition.openings
    );

    destination->definition.opening_count =
        source->definition.opening_count;
    destination->definition.opening_capacity =
        source->definition.opening_count;
}

static inline void test_clone_project_authoritative(
    const SiteHelperProject *source,
    SiteHelperProject *destination
)
{
    assert(source != NULL);
    assert(destination != NULL);

    *destination = (SiteHelperProject){
        .settings = source->settings,
        .domain_ids = source->domain_ids
    };

    for (size_t level = 0; level < source->storey_count; level++) {
        assert(sitehelper_project_insert_storey(destination, source->storeys[level].id,
            source->storeys[level].elevation_mm));
        destination->storeys[level].settings = source->storeys[level].settings;
        for (size_t i = 0; i < source->storeys[level].slabs.count; i++) {
            assert(sitehelper_project_insert_slab(destination, source->storeys[level].id, &source->storeys[level].slabs.items[i]));
        }
        if (source->storeys[level].roofs.count) {
            RoofCollection *dc=&destination->storeys[level].roofs;
            dc->items=calloc(source->storeys[level].roofs.count,sizeof *dc->items);
            assert(dc->items); dc->capacity=source->storeys[level].roofs.count;
            for(size_t i=0;i<source->storeys[level].roofs.count;i++){
                assert(roof_clone(&source->storeys[level].roofs.items[i],&dc->items[i])==ROOF_SUCCESS);
                dc->count++;
            }
        }
        for (size_t i = 0; i < source->storeys[level].structure.wall_count; i++) {
            Wall wall = {0};
            test_clone_wall_definition(&source->storeys[level].structure.walls[i], &wall);
            assert(build_append_wall(&destination->storeys[level].structure, &wall));
        }

        for (size_t i = 0; i < source->storeys[level].structure.room_count; i++) {
            const Room *source_room = &source->storeys[level].structure.rooms[i];

            assert(build_add_room(&destination->storeys[level].structure, source_room->id));

            Room *destination_room = build_find_room_by_id(
                &destination->storeys[level].structure,
                source_room->id
            );

            assert(destination_room != NULL);
            if (source_room->has_location) {
                assert(sitehelper_project_set_room_location(destination,
                    source_room->id, source_room->location));
            }
        }
        for (size_t i = 0; i < source->storeys[level].structure.room_separator_count; i++) {
            assert(build_insert_room_separator(&destination->storeys[level].structure, &source->storeys[level].structure.room_separators[i], i));
        }
    }
    for (size_t i = 0; i < source->document.annotation_count; i++) {
        assert(sitehelper_project_insert_annotation(destination, &source->document.annotations[i]));
    }
    for (size_t i = 0; i < source->document.dimension_count; i++) {
        assert(sitehelper_project_insert_dimension(destination, &source->document.dimensions[i]));
    }
    for (size_t i = 0; i < source->document.symbol_count; i++) {
        assert(sitehelper_project_insert_symbol(destination, &source->document.symbols[i]));
    }
    for (size_t i=0;i<source->document.callout_count;i++) {
        assert(sitehelper_project_insert_callout(destination,&source->document.callouts[i]));
    }
    for (size_t i=0;i<source->document.revision_cloud_count;i++) {
        assert(sitehelper_project_insert_revision_cloud(
            destination,&source->document.revision_clouds[i]));
    }
}

#endif
