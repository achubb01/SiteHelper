#include <stdlib.h>
#include <string.h>

#include "sitehelper_command.h"
#include "sitehelper_command_internal.h"
#include "delete_wall_command_internal.h"
#include "delete_roof_command_internal.h"
#include "wall.h"
#include "slab.h"
#include "roof.h"

typedef struct {
    DomainId storey_id;
    size_t index;
    Slab slab;
} DeletedSlabSnapshot;

typedef struct {
    DomainId slab_id;
    size_t index;
    size_t original_count;
    SlabPenetration feature;
} DeletedSlabPenetrationSnapshot;

typedef struct {
    DomainId slab_id;
    size_t index;
    size_t original_count;
    SlabRegion feature;
} DeletedSlabRegionSnapshot;

typedef struct {
    DomainId slab_id;
    size_t index;
    size_t original_count;
    SlabEdgeRebate feature;
} DeletedSlabEdgeRebateSnapshot;

typedef struct {
    DomainId slab_id;
    int thickness_mm;
    int top_level_offset_mm;
} EditedSlabSnapshot;

typedef struct {
    DomainId slab_id;
    size_t index;
    size_t original_count;
    SlabRegion feature;
} EditedSlabRegionSnapshot;

typedef struct {
    DomainId slab_id;
    size_t index;
    size_t original_count;
    SlabEdgeRebate feature;
} EditedSlabEdgeRebateSnapshot;

typedef struct {
    DomainId slab_id;
    MoveSlabVertexTarget target;
    size_t feature_index;
    size_t original_feature_count;
    size_t vertex_index;
    SlabOutline outline;
    int region_top_level_offset_mm;
    int region_thickness_mm;
} MovedSlabVertexSnapshot;

typedef struct {
    Roof roof;
} EditedRoofSnapshot;

struct SiteHelperCommandUndoState
{
    SiteHelperCommandType type;
    union
    {
        DeletedWallSnapshot deleted_wall;
        struct
        {
            DomainId wall_id;
            WallPlanSegment segment;
        } moved_wall;
        RoomLocationCommand previous_room_location;
        EditOpeningCommand previous_opening;
        struct {
            DomainId storey_id;
            RoomSeparator definition;
            size_t index; /* Delete undo restores stored collection order. */
        } room_separator;
        DeletedSlabSnapshot deleted_slab;
        DeletedSlabPenetrationSnapshot deleted_slab_penetration;
        DeletedSlabRegionSnapshot deleted_slab_region;
        DeletedSlabEdgeRebateSnapshot deleted_slab_edge_rebate;
        EditedSlabSnapshot edited_slab;
        EditedSlabRegionSnapshot edited_slab_region;
        EditedSlabEdgeRebateSnapshot edited_slab_edge_rebate;
        MovedSlabVertexSnapshot moved_slab_vertex;
        DeletedRoofSnapshot deleted_roof;
        EditedRoofSnapshot edited_roof;
        DocumentAnnotation previous_annotation;
        DocumentPlanDimension previous_dimension;
        DocumentPlanSymbol previous_symbol;
        DocumentPlanCallout previous_callout;
        DocumentRevision previous_revision;
        DocumentPlanRevisionCloud previous_revision_cloud;
    };
};

static int outline_copy(const SlabOutline *source, SlabOutline *output)
{
    if (source == NULL || output == NULL || source->vertices == NULL ||
        source->vertex_count < 3 || source->vertex_count > source->vertex_capacity ||
        source->vertex_count > SIZE_MAX / sizeof *source->vertices) { return 0; }
    PlanPosition *vertices=malloc(source->vertex_count * sizeof *vertices);
    if (vertices == NULL) { return 0; }
    memcpy(vertices,source->vertices,source->vertex_count * sizeof *vertices);
    *output=(SlabOutline){vertices,source->vertex_count,source->vertex_count};
    return 1;
}

static int outlines_equal(const SlabOutline *a, const SlabOutline *b)
{
    return a != NULL && b != NULL && a->vertex_count == b->vertex_count &&
        a->vertex_count <= SIZE_MAX / sizeof *a->vertices &&
        a->vertices != NULL && b->vertices != NULL &&
        memcmp(a->vertices,b->vertices,a->vertex_count * sizeof *a->vertices) == 0;
}

static int penetration_matches(const SlabPenetration *feature,
    const PlanPosition *vertices, size_t count)
{
    SlabOutline expected={(PlanPosition *)vertices,count,count};
    return feature != NULL && outlines_equal(&feature->outline,&expected);
}

static int region_matches(const SlabRegion *feature, const PlanPosition *vertices,
    size_t count, int top_level_offset_mm, int thickness_mm)
{
    SlabOutline expected={(PlanPosition *)vertices,count,count};
    return feature != NULL && feature->top_level_offset_mm == top_level_offset_mm &&
        feature->thickness_mm == thickness_mm &&
        outlines_equal(&feature->outline,&expected);
}

static int rebate_equal(const SlabEdgeRebate *a, const SlabEdgeRebate *b)
{
    return a != NULL && b != NULL && a->edge_index == b->edge_index &&
        a->start_offset_mm == b->start_offset_mm &&
        a->end_offset_mm == b->end_offset_mm && a->width_mm == b->width_mm &&
        a->depth_mm == b->depth_mm;
}

static int annotations_equal(const DocumentAnnotation *a, const DocumentAnnotation *b)
{
    return a != NULL && b != NULL && a->id == b->id && a->kind == b->kind &&
        a->anchor.storey_id == b->anchor.storey_id &&
        a->anchor.position.x == b->anchor.position.x &&
        a->anchor.position.y == b->anchor.position.y && a->target_id == b->target_id &&
        a->text != NULL && b->text != NULL && strcmp(a->text, b->text) == 0;
}

static int dimension_refs_equal(DocumentDimensionReference a,
    DocumentDimensionReference b)
{
    return a.kind == b.kind && a.target_id == b.target_id &&
        a.position.x == b.position.x && a.position.y == b.position.y;
}

static int dimensions_equal(const DocumentPlanDimension *a,
    const DocumentPlanDimension *b)
{
    return a != NULL && b != NULL && a->id == b->id &&
        a->storey_id == b->storey_id && dimension_refs_equal(a->first,b->first) &&
        dimension_refs_equal(a->second,b->second) && a->offset_mm == b->offset_mm;
}

static int dimension_matches_edit(const DocumentPlanDimension *dimension,
    const EditPlanDimensionCommand *edit)
{
    if (edit == NULL) { return 0; }
    DocumentPlanDimension expected={edit->dimension_id,edit->storey_id,
        edit->first,edit->second,edit->offset_mm};
    return dimensions_equal(dimension,&expected);
}

static int symbols_equal(const DocumentPlanSymbol *a, const DocumentPlanSymbol *b)
{
    return a != NULL && b != NULL && a->id == b->id &&
        a->storey_id == b->storey_id && a->kind == b->kind &&
        a->anchor.x == b->anchor.x && a->anchor.y == b->anchor.y &&
        a->direction.dx == b->direction.dx && a->direction.dy == b->direction.dy;
}

static int symbol_matches_edit(const DocumentPlanSymbol *symbol,
    const EditPlanSymbolCommand *edit)
{
    if (edit == NULL) { return 0; }
    DocumentPlanSymbol expected={edit->symbol_id,edit->storey_id,edit->kind,edit->anchor,
        edit->direction};
    return symbols_equal(symbol,&expected);
}

static int callouts_equal(const DocumentPlanCallout *a, const DocumentPlanCallout *b)
{
    return a != NULL && b != NULL && a->id == b->id && a->storey_id == b->storey_id &&
        a->target.x == b->target.x && a->target.y == b->target.y &&
        a->label_anchor.x == b->label_anchor.x && a->label_anchor.y == b->label_anchor.y &&
        a->text != NULL && b->text != NULL && strcmp(a->text,b->text) == 0;
}

static int callout_matches_edit(const DocumentPlanCallout *callout,
    const EditPlanCalloutCommand *edit)
{
    if (edit == NULL) { return 0; }
    DocumentPlanCallout expected={.id=edit->callout_id,.storey_id=edit->storey_id,
        .target=edit->target,.label_anchor=edit->label_anchor,.text=edit->text};
    return callouts_equal(callout,&expected);
}


static int revisions_equal(const DocumentRevision *a, const DocumentRevision *b)
{
    return a != NULL && b != NULL && a->id == b->id &&
        strcmp(a->identifier, b->identifier) == 0 &&
        strcmp(a->description, b->description) == 0;
}

static int revision_matches_edit(const DocumentRevision *revision,
    const EditDocumentRevisionCommand *edit)
{
    if (edit == NULL) { return 0; }
    DocumentRevision expected = {.id=edit->revision_id, .identifier=edit->identifier,
        .description=edit->description};
    return revisions_equal(revision, &expected);
}

static int revision_clouds_equal(const DocumentPlanRevisionCloud *a,
    const DocumentPlanRevisionCloud *b)
{
    return a != NULL && b != NULL && a->id == b->id &&
        a->storey_id == b->storey_id && a->revision_id == b->revision_id &&
        a->vertex_count == b->vertex_count &&
        a->vertices != NULL && b->vertices != NULL &&
        a->vertex_count <= SIZE_MAX / sizeof *a->vertices &&
        memcmp(a->vertices, b->vertices,
            a->vertex_count * sizeof *a->vertices) == 0;
}

static int revision_cloud_matches_edit(const DocumentPlanRevisionCloud *cloud,
    const EditPlanRevisionCloudCommand *edit)
{
    return cloud != NULL && edit != NULL && cloud->id == edit->revision_cloud_id &&
        cloud->storey_id == edit->storey_id &&
        cloud->vertex_count == edit->vertex_count && cloud->vertices != NULL &&
        edit->vertices != NULL && edit->vertex_count <= SIZE_MAX / sizeof *edit->vertices &&
        memcmp(cloud->vertices, edit->vertices,
            edit->vertex_count * sizeof *edit->vertices) == 0;
}

static int annotation_matches_edit(const DocumentAnnotation *annotation,
    const EditPlanNoteCommand *edit)
{
    if (edit == NULL) { return 0; }
    DocumentAnnotation expected = {
        .id = edit->annotation_id, .kind = DOCUMENT_ANNOTATION_NOTE,
        .anchor = {.storey_id = edit->storey_id, .position = edit->position},
        .target_id = edit->target_id, .text = edit->text
    };
    return annotations_equal(annotation, &expected);
}

static const SlabOutline *move_vertex_outline_const(const Slab *slab,
    MoveSlabVertexTarget target, size_t feature_index,
    size_t *feature_count, int *region_top, int *region_thickness)
{
    if (feature_count != NULL) { *feature_count=SIZE_MAX; }
    if (region_top != NULL) { *region_top=0; }
    if (region_thickness != NULL) { *region_thickness=0; }
    if (slab == NULL) { return NULL; }
    switch (target) {
        case MOVE_SLAB_VERTEX_OUTLINE:
            if (feature_index != SIZE_MAX) { return NULL; }
            return &slab->definition.outline;
        case MOVE_SLAB_VERTEX_PENETRATION:
            if (feature_index >= slab->definition.penetrations.count) { return NULL; }
            if (feature_count != NULL) { *feature_count=slab->definition.penetrations.count; }
            return &slab->definition.penetrations.items[feature_index].outline;
        case MOVE_SLAB_VERTEX_REGION:
            if (feature_index >= slab->definition.regions.count) { return NULL; }
            if (feature_count != NULL) { *feature_count=slab->definition.regions.count; }
            if (region_top != NULL) {
                *region_top=slab->definition.regions.items[feature_index].top_level_offset_mm;
            }
            if (region_thickness != NULL) {
                *region_thickness=slab->definition.regions.items[feature_index].thickness_mm;
            }
            return &slab->definition.regions.items[feature_index].outline;
        case MOVE_SLAB_VERTEX_TARGET_COUNT:
        default:
            return NULL;
    }
}

static int outline_matches_vertex_move(const SlabOutline *before,
    const SlabOutline *current, size_t vertex_index, PlanPosition moved)
{
    if (before == NULL || current == NULL || before->vertices == NULL ||
        current->vertices == NULL || before->vertex_count != current->vertex_count ||
        vertex_index >= before->vertex_count) { return 0; }
    for (size_t i=0;i<before->vertex_count;i++) {
        PlanPosition expected=i == vertex_index ? moved : before->vertices[i];
        if (current->vertices[i].x != expected.x || current->vertices[i].y != expected.y) {
            return 0;
        }
    }
    return 1;
}

static int moved_vertex_snapshot_matches(const Slab *slab,
    const MovedSlabVertexSnapshot *snapshot, int after_move,
    PlanPosition new_position)
{
    if (slab == NULL || snapshot == NULL || slab->id != snapshot->slab_id) { return 0; }
    size_t feature_count=SIZE_MAX;
    int region_top=0,region_thickness=0;
    const SlabOutline *outline=move_vertex_outline_const(slab,snapshot->target,
        snapshot->feature_index,&feature_count,&region_top,&region_thickness);
    if (outline == NULL || snapshot->vertex_index >= outline->vertex_count) { return 0; }
    if (snapshot->target != MOVE_SLAB_VERTEX_OUTLINE &&
        feature_count != snapshot->original_feature_count) { return 0; }
    if (snapshot->target == MOVE_SLAB_VERTEX_REGION &&
        (region_top != snapshot->region_top_level_offset_mm ||
         region_thickness != snapshot->region_thickness_mm)) { return 0; }
    return after_move ? outline_matches_vertex_move(&snapshot->outline,outline,
        snapshot->vertex_index,new_position) : outlines_equal(&snapshot->outline,outline);
}

static int restore_moved_vertex(Slab *slab, const MovedSlabVertexSnapshot *snapshot)
{
    PlanPosition old=snapshot->outline.vertices[snapshot->vertex_index];
    SlabCode code;
    switch (snapshot->target) {
        case MOVE_SLAB_VERTEX_OUTLINE:
            code=slab_set_outline_vertex(slab,snapshot->vertex_index,old);
            break;
        case MOVE_SLAB_VERTEX_PENETRATION:
            code=slab_set_penetration_vertex(slab,snapshot->feature_index,
                snapshot->vertex_index,old);
            break;
        case MOVE_SLAB_VERTEX_REGION:
            code=slab_set_region_vertex(slab,snapshot->feature_index,
                snapshot->vertex_index,old);
            break;
        case MOVE_SLAB_VERTEX_TARGET_COUNT:
        default:
            return 0;
    }
    return code == SLAB_SUCCESS;
}

static int capture_feature_snapshot(const SiteHelperProject *project,
    const SiteHelperCommand *command, SiteHelperCommandUndoState *state)
{
    DomainId slab_id=DOMAIN_ID_INVALID;
    size_t index=SIZE_MAX;
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION) {
        slab_id=command->data.delete_slab_penetration.slab_id;
        index=command->data.delete_slab_penetration.feature_index;
    } else if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_REGION) {
        slab_id=command->data.delete_slab_region.slab_id;
        index=command->data.delete_slab_region.feature_index;
    } else if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE) {
        slab_id=command->data.delete_slab_edge_rebate.slab_id;
        index=command->data.delete_slab_edge_rebate.feature_index;
    } else { return 0; }
    const Slab *slab=sitehelper_project_find_slab_by_id_const(project,slab_id);
    if (slab == NULL || slab_validate(slab) != SLAB_SUCCESS) { return 0; }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION) {
        const SlabPenetration *feature=slab_penetration_at(slab,index);
        if (feature == NULL || !outline_copy(&feature->outline,
            &state->deleted_slab_penetration.feature.outline)) { return 0; }
        state->deleted_slab_penetration.slab_id=slab_id;
        state->deleted_slab_penetration.index=index;
        state->deleted_slab_penetration.original_count=
            slab->definition.penetrations.count;
    } else if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_REGION) {
        const SlabRegion *feature=slab_region_at(slab,index);
        if (feature == NULL || !outline_copy(&feature->outline,
            &state->deleted_slab_region.feature.outline)) { return 0; }
        state->deleted_slab_region.slab_id=slab_id;
        state->deleted_slab_region.index=index;
        state->deleted_slab_region.original_count=slab->definition.regions.count;
        state->deleted_slab_region.feature.top_level_offset_mm=feature->top_level_offset_mm;
        state->deleted_slab_region.feature.thickness_mm=feature->thickness_mm;
    } else {
        const SlabEdgeRebate *feature=slab_edge_rebate_at(slab,index);
        if (feature == NULL) { return 0; }
        state->deleted_slab_edge_rebate=(DeletedSlabEdgeRebateSnapshot){slab_id,index,
            slab->definition.edge_rebates.count,*feature};
    }
    return 1;
}

int sitehelper_command_capture_undo_state(
    const SiteHelperProject *project, const SiteHelperCommand *command,
    SiteHelperCommandUndoState **state)
{
    if (project == NULL || command == NULL || state == NULL) {
        return 0;
    }
    *state = NULL;
    if (command->type != SITEHELPER_COMMAND_DELETE_WALL &&
        command->type != SITEHELPER_COMMAND_EDIT_OPENING &&
        command->type != SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT &&
        command->type != SITEHELPER_COMMAND_SET_ROOM_LOCATION &&
        command->type != SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR &&
        command->type != SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT &&
        command->type != SITEHELPER_COMMAND_DELETE_SLAB &&
        command->type != SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION &&
        command->type != SITEHELPER_COMMAND_DELETE_SLAB_REGION &&
        command->type != SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE &&
        command->type != SITEHELPER_COMMAND_EDIT_SLAB &&
        command->type != SITEHELPER_COMMAND_EDIT_SLAB_REGION &&
        command->type != SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE &&
        command->type != SITEHELPER_COMMAND_MOVE_SLAB_VERTEX &&
        command->type != SITEHELPER_COMMAND_DELETE_ROOF &&
        command->type != SITEHELPER_COMMAND_EDIT_ROOF_SOURCE &&
        command->type != SITEHELPER_COMMAND_EDIT_PLAN_NOTE &&
        command->type != SITEHELPER_COMMAND_DELETE_PLAN_NOTE &&
        command->type != SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION &&
        command->type != SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION &&
        command->type != SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL &&
        command->type != SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL &&
        command->type != SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT &&
        command->type != SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT &&
        command->type != SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION &&
        command->type != SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION &&
        command->type != SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION &&
        command->type != SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD &&
        command->type != SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD) {
        return 1;
    }
    SiteHelperCommandUndoState *candidate = calloc(1, sizeof *candidate);
    if (candidate == NULL) {
        return 0;
    }
    candidate->type = command->type;
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION ||
        command->type == SITEHELPER_COMMAND_DELETE_SLAB_REGION ||
        command->type == SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE) {
        if (!capture_feature_snapshot(project,command,candidate)) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_SLAB) {
        const Slab *slab=sitehelper_project_find_slab_by_id_const(project,
            command->data.edit_slab.slab_id);
        if (slab == NULL || slab_validate(slab) != SLAB_SUCCESS) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
        candidate->edited_slab=(EditedSlabSnapshot){slab->id,
            slab->definition.thickness_mm,slab->definition.top_level_offset_mm};
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_SLAB_REGION) {
        const EditSlabRegionCommand *edit=&command->data.edit_slab_region;
        const Slab *slab=sitehelper_project_find_slab_by_id_const(project,edit->slab_id);
        const SlabRegion *region=slab == NULL ? NULL : slab_region_at(slab,edit->feature_index);
        if (slab == NULL || slab_validate(slab) != SLAB_SUCCESS || region == NULL ||
            !outline_copy(&region->outline,&candidate->edited_slab_region.feature.outline)) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
        candidate->edited_slab_region.slab_id=edit->slab_id;
        candidate->edited_slab_region.index=edit->feature_index;
        candidate->edited_slab_region.original_count=slab->definition.regions.count;
        candidate->edited_slab_region.feature.top_level_offset_mm=region->top_level_offset_mm;
        candidate->edited_slab_region.feature.thickness_mm=region->thickness_mm;
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE) {
        const EditSlabEdgeRebateCommand *edit=&command->data.edit_slab_edge_rebate;
        const Slab *slab=sitehelper_project_find_slab_by_id_const(project,edit->slab_id);
        const SlabEdgeRebate *rebate=slab == NULL ? NULL : slab_edge_rebate_at(slab,edit->feature_index);
        if (slab == NULL || slab_validate(slab) != SLAB_SUCCESS || rebate == NULL ||
            rebate->edge_index != edit->edge_index) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
        candidate->edited_slab_edge_rebate=(EditedSlabEdgeRebateSnapshot){
            edit->slab_id,edit->feature_index,slab->definition.edge_rebates.count,*rebate};
    }
    else if (command->type == SITEHELPER_COMMAND_MOVE_SLAB_VERTEX) {
        const MoveSlabVertexCommand *move=&command->data.move_slab_vertex;
        const Slab *slab=sitehelper_project_find_slab_by_id_const(project,move->slab_id);
        size_t feature_count=SIZE_MAX;
        int region_top=0,region_thickness=0;
        const SlabOutline *outline=slab == NULL ? NULL : move_vertex_outline_const(slab,
            move->target,move->feature_index,&feature_count,&region_top,&region_thickness);
        if (slab == NULL || slab_validate(slab) != SLAB_SUCCESS || outline == NULL ||
            move->vertex_index >= outline->vertex_count ||
            !outline_copy(outline,&candidate->moved_slab_vertex.outline)) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
        candidate->moved_slab_vertex.slab_id=move->slab_id;
        candidate->moved_slab_vertex.target=move->target;
        candidate->moved_slab_vertex.feature_index=move->feature_index;
        candidate->moved_slab_vertex.original_feature_count=feature_count;
        candidate->moved_slab_vertex.vertex_index=move->vertex_index;
        candidate->moved_slab_vertex.region_top_level_offset_mm=region_top;
        candidate->moved_slab_vertex.region_thickness_mm=region_thickness;
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION ||
        command->type == SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION) {
        DomainId id = command->type == SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION
            ? command->data.edit_plan_dimension.dimension_id
            : command->data.delete_plan_dimension.dimension_id;
        const DocumentPlanDimension *dimension = sitehelper_project_find_dimension_by_id_const(
            project,id);
        if (dimension == NULL) { sitehelper_command_destroy_undo_state(candidate); return 0; }
        candidate->previous_dimension=*dimension;
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL ||
        command->type == SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL) {
        DomainId id = command->type == SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL
            ? command->data.edit_plan_symbol.symbol_id
            : command->data.delete_plan_symbol.symbol_id;
        const DocumentPlanSymbol *symbol = sitehelper_project_find_symbol_by_id_const(project,id);
        if (symbol == NULL) { sitehelper_command_destroy_undo_state(candidate); return 0; }
        candidate->previous_symbol=*symbol;
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT ||
        command->type == SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT) {
        DomainId id=command->type == SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT
            ? command->data.edit_plan_callout.callout_id
            : command->data.delete_plan_callout.callout_id;
        const DocumentPlanCallout *callout=sitehelper_project_find_callout_by_id_const(project,id);
        if (callout == NULL || document_plan_callout_clone(callout,&candidate->previous_callout)
                != DOCUMENT_SUCCESS) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION ||
        command->type == SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION) {
        DomainId id = command->type == SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION
            ? command->data.edit_document_revision.revision_id
            : command->data.delete_document_revision.revision_id;
        const DocumentRevision *revision = sitehelper_project_find_revision_by_id_const(
            project, id);
        if (revision == NULL || document_revision_clone(revision,
                &candidate->previous_revision) != DOCUMENT_SUCCESS) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION ||
        command->type == SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD ||
        command->type == SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD) {
        DomainId id = command->type == SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION
            ? command->data.set_plan_revision_cloud_revision.revision_cloud_id
            : command->type == SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD
                ? command->data.edit_plan_revision_cloud.revision_cloud_id
                : command->data.delete_plan_revision_cloud.revision_cloud_id;
        const DocumentPlanRevisionCloud *cloud =
            sitehelper_project_find_revision_cloud_by_id_const(project, id);
        if (cloud == NULL ||
            document_plan_revision_cloud_clone(cloud,
                &candidate->previous_revision_cloud) != DOCUMENT_SUCCESS) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_NOTE ||
        command->type == SITEHELPER_COMMAND_DELETE_PLAN_NOTE) {
        DomainId id = command->type == SITEHELPER_COMMAND_EDIT_PLAN_NOTE
            ? command->data.edit_plan_note.annotation_id
            : command->data.delete_plan_note.annotation_id;
        const DocumentAnnotation *annotation = sitehelper_project_find_annotation_by_id_const(
            project, id);
        if (annotation == NULL || document_annotation_clone(annotation,
                &candidate->previous_annotation) != DOCUMENT_SUCCESS) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_ROOF_SOURCE) {
        const Roof *roof = sitehelper_project_find_roof_by_id_const(project,
            command->data.edit_roof_source.roof_id);
        if (roof == NULL || roof_clone(roof, &candidate->edited_roof.roof) != ROOF_SUCCESS) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_DELETE_ROOF) {
        if (!deleted_roof_snapshot_capture(project, &command->data.delete_roof,
                &candidate->deleted_roof)) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_DELETE_SLAB) {
        const Slab *slab=sitehelper_project_find_slab_by_id_const(project,
            command->data.delete_slab.slab_id);
        const Storey *owner=slab == NULL ? NULL :
            sitehelper_project_find_owning_storey_const(project,slab->id);
        if (owner == NULL ||
            slab_collection_find_by_id_const(&owner->slabs,slab->id) != slab ||
            slab_clone(slab,&candidate->deleted_slab.slab) != SLAB_SUCCESS) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
        candidate->deleted_slab.storey_id=owner->id;
        candidate->deleted_slab.index=(size_t)(slab-owner->slabs.items);
    }
    else if (command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR ||
        command->type == SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT) {
        DomainId id = command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR
            ? command->data.delete_room_separator.separator_id
            : command->data.move_room_separator_endpoint.separator_id;
        const RoomSeparator *separator = sitehelper_project_find_room_separator_by_id_const(project, id);
        if (separator == NULL) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
        const Storey *storey = sitehelper_project_find_owning_storey_const(project, id);
        candidate->room_separator.storey_id = storey->id;
        candidate->room_separator.definition = *separator;
        candidate->room_separator.index = (size_t)(separator - storey->structure.room_separators);
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_OPENING) {
        const EditOpeningCommand *edit = &command->data.edit_opening;
        const Wall *wall = sitehelper_project_find_wall_by_id_const(project, edit->wall_id);
        const Opening *opening = wall_find_opening_by_id_const(wall, edit->opening_id);
        if (!edit_opening_command_create(edit->wall_id, edit->opening_id,
                opening, &candidate->previous_opening)) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_SET_ROOM_LOCATION) {
        const Room *room = sitehelper_project_find_room_by_id_const(project,
            command->data.room_location.room_id);
        if (room == NULL) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
        candidate->previous_room_location = (RoomLocationCommand){
            .room_id = room->id, .has_location = room->has_location,
            .location = room->has_location ? room->location : (PlanPosition){0}
        };
    }
    else if (command->type == SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT) {
        const Wall *wall = sitehelper_project_find_wall_by_id_const(project,
            command->data.move_wall_endpoint.wall_id);
        if (wall == NULL) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
        candidate->moved_wall.wall_id = wall->id;
        candidate->moved_wall.segment = wall->definition.segment;
    }
    else if (!deleted_wall_snapshot_capture(project, &command->data.delete_wall,
            &candidate->deleted_wall)) {
        sitehelper_command_destroy_undo_state(candidate);
        return 0;
    }
    *state = candidate;
    return 1;
}

void sitehelper_command_destroy_undo_state(SiteHelperCommandUndoState *state)
{
    if (state == NULL) {
        return;
    }
    if (state->type == SITEHELPER_COMMAND_DELETE_WALL) {
        deleted_wall_snapshot_destroy(&state->deleted_wall);
    } else if (state->type == SITEHELPER_COMMAND_DELETE_SLAB) {
        slab_destroy(&state->deleted_slab.slab);
    } else if (state->type == SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION) {
        slab_outline_destroy(&state->deleted_slab_penetration.feature.outline);
    } else if (state->type == SITEHELPER_COMMAND_DELETE_SLAB_REGION) {
        slab_outline_destroy(&state->deleted_slab_region.feature.outline);
    } else if (state->type == SITEHELPER_COMMAND_EDIT_SLAB_REGION) {
        slab_outline_destroy(&state->edited_slab_region.feature.outline);
    } else if (state->type == SITEHELPER_COMMAND_MOVE_SLAB_VERTEX) {
        slab_outline_destroy(&state->moved_slab_vertex.outline);
    } else if (state->type == SITEHELPER_COMMAND_DELETE_ROOF) {
        deleted_roof_snapshot_destroy(&state->deleted_roof);
    } else if (state->type == SITEHELPER_COMMAND_EDIT_ROOF_SOURCE) {
        roof_destroy(&state->edited_roof.roof);
    } else if (state->type == SITEHELPER_COMMAND_EDIT_PLAN_NOTE ||
        state->type == SITEHELPER_COMMAND_DELETE_PLAN_NOTE) {
        document_annotation_destroy(&state->previous_annotation);
    } else if (state->type == SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT ||
        state->type == SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT) {
        document_plan_callout_destroy(&state->previous_callout);
    } else if (state->type == SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION ||
        state->type == SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION) {
        document_revision_destroy(&state->previous_revision);
    } else if (state->type == SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION ||
        state->type == SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD ||
        state->type == SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD) {
        document_plan_revision_cloud_destroy(&state->previous_revision_cloud);
    }
    free(state);
}

int sitehelper_command_undo_with_state(
    SiteHelperProject *project, const SiteHelperCommand *command,
    const SiteHelperCommandResult *result, const SiteHelperCommandUndoState *state)
{
    if (project == NULL || command == NULL || result == NULL || command->type != result->type) {
        return 0;
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT) {
        const DocumentPlanCallout *current=sitehelper_project_find_callout_by_id_const(
            project,command->data.edit_plan_callout.callout_id);
        return state != NULL && state->type == command->type &&
            result->data.callout.callout_id == command->data.edit_plan_callout.callout_id &&
            callout_matches_edit(current,&command->data.edit_plan_callout) &&
            sitehelper_project_replace_callout(project,&state->previous_callout);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT) {
        return state != NULL && state->type == command->type &&
            result->data.callout.callout_id == command->data.delete_plan_callout.callout_id &&
            sitehelper_project_find_callout_by_id_const(project,
                command->data.delete_plan_callout.callout_id) == NULL &&
            sitehelper_project_insert_callout(project,&state->previous_callout);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION) {
        const DocumentRevision *current=sitehelper_project_find_revision_by_id_const(
            project,command->data.edit_document_revision.revision_id);
        return state != NULL && state->type == command->type &&
            result->data.revision.revision_id == command->data.edit_document_revision.revision_id &&
            revision_matches_edit(current,&command->data.edit_document_revision) &&
            sitehelper_project_replace_revision(project,&state->previous_revision);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION) {
        return state != NULL && state->type == command->type &&
            result->data.revision.revision_id == command->data.delete_document_revision.revision_id &&
            sitehelper_project_find_revision_by_id_const(project,
                command->data.delete_document_revision.revision_id) == NULL &&
            sitehelper_project_insert_revision(project,&state->previous_revision);
    }
    if (command->type == SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION) {
        const DocumentPlanRevisionCloud *current =
            sitehelper_project_find_revision_cloud_by_id_const(project,
                command->data.set_plan_revision_cloud_revision.revision_cloud_id);
        return state != NULL && state->type == command->type &&
            result->data.revision_cloud.revision_cloud_id ==
                command->data.set_plan_revision_cloud_revision.revision_cloud_id &&
            current != NULL &&
            current->revision_id == command->data.set_plan_revision_cloud_revision.revision_id &&
            sitehelper_project_replace_revision_cloud(
                project, &state->previous_revision_cloud);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD) {
        const DocumentPlanRevisionCloud *current =
            sitehelper_project_find_revision_cloud_by_id_const(
                project, command->data.edit_plan_revision_cloud.revision_cloud_id);
        return state != NULL && state->type == command->type &&
            result->data.revision_cloud.revision_cloud_id ==
                command->data.edit_plan_revision_cloud.revision_cloud_id &&
            revision_cloud_matches_edit(current,
                &command->data.edit_plan_revision_cloud) &&
            current->revision_id == state->previous_revision_cloud.revision_id &&
            sitehelper_project_replace_revision_cloud(
                project, &state->previous_revision_cloud);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD) {
        return state != NULL && state->type == command->type &&
            result->data.revision_cloud.revision_cloud_id ==
                command->data.delete_plan_revision_cloud.revision_cloud_id &&
            sitehelper_project_find_revision_cloud_by_id_const(project,
                command->data.delete_plan_revision_cloud.revision_cloud_id) == NULL &&
            sitehelper_project_insert_revision_cloud(
                project, &state->previous_revision_cloud);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL) {
        const DocumentPlanSymbol *current=sitehelper_project_find_symbol_by_id_const(
            project,command->data.edit_plan_symbol.symbol_id);
        return state != NULL && state->type == command->type &&
            result->data.symbol.symbol_id == command->data.edit_plan_symbol.symbol_id &&
            symbol_matches_edit(current,&command->data.edit_plan_symbol) &&
            sitehelper_project_replace_symbol(project,&state->previous_symbol);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL) {
        return state != NULL && state->type == command->type &&
            result->data.symbol.symbol_id == command->data.delete_plan_symbol.symbol_id &&
            sitehelper_project_find_symbol_by_id_const(project,
                command->data.delete_plan_symbol.symbol_id) == NULL &&
            sitehelper_project_insert_symbol(project,&state->previous_symbol);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION) {
        const DocumentPlanDimension *current=sitehelper_project_find_dimension_by_id_const(
            project,command->data.edit_plan_dimension.dimension_id);
        return state != NULL && state->type == command->type &&
            result->data.dimension.dimension_id == command->data.edit_plan_dimension.dimension_id &&
            dimension_matches_edit(current,&command->data.edit_plan_dimension) &&
            sitehelper_project_replace_dimension(project,&state->previous_dimension);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION) {
        return state != NULL && state->type == command->type &&
            result->data.dimension.dimension_id == command->data.delete_plan_dimension.dimension_id &&
            sitehelper_project_find_dimension_by_id_const(project,
                command->data.delete_plan_dimension.dimension_id) == NULL &&
            sitehelper_project_insert_dimension(project,&state->previous_dimension);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_NOTE) {
        const DocumentAnnotation *current = sitehelper_project_find_annotation_by_id_const(
            project, command->data.edit_plan_note.annotation_id);
        return state != NULL && state->type == command->type &&
            result->data.annotation.annotation_id == command->data.edit_plan_note.annotation_id &&
            annotation_matches_edit(current, &command->data.edit_plan_note) &&
            sitehelper_project_replace_annotation(project, &state->previous_annotation);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_NOTE) {
        return state != NULL && state->type == command->type &&
            result->data.annotation.annotation_id == command->data.delete_plan_note.annotation_id &&
            sitehelper_project_find_annotation_by_id_const(project,
                command->data.delete_plan_note.annotation_id) == NULL &&
            sitehelper_project_insert_annotation(project, &state->previous_annotation);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_ROOF_SOURCE) {
        if (state == NULL || state->type != command->type ||
            state->edited_roof.roof.id != command->data.edit_roof_source.roof_id ||
            result->data.roof.roof_id != command->data.edit_roof_source.roof_id) { return 0; }
        Roof expected = {0};
        if (!roof_source_edit_command_build_expected(&state->edited_roof.roof,
                &command->data.edit_roof_source, result->data.roof.portion_id, &expected)) {
            roof_destroy(&expected); return 0;
        }
        const Roof *current = sitehelper_project_find_roof_by_id_const(project, expected.id);
        int matches = current != NULL && roof_authority_equal(current, &expected);
        roof_destroy(&expected);
        return matches && sitehelper_project_replace_roof(project, &state->edited_roof.roof);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_ROOF) {
        DomainId id = command->data.delete_roof.roof_id;
        return state != NULL && state->type == command->type &&
            result->data.roof.roof_id == id && state->deleted_roof.roof.id == id &&
            deleted_roof_snapshot_restore(project, &state->deleted_roof);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION) {
        const DeletedSlabPenetrationSnapshot *s=state == NULL ? NULL :
            &state->deleted_slab_penetration;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        return state != NULL && state->type == command->type &&
            s->slab_id == command->data.delete_slab_penetration.slab_id &&
            s->index == command->data.delete_slab_penetration.feature_index &&
            result->data.slab_feature.slab_id == s->slab_id &&
            result->data.slab_feature.feature_index == s->index && slab != NULL &&
            s->original_count != 0 &&
            slab->definition.penetrations.count == s->original_count - 1 &&
            slab_insert_penetration_at(slab,s->index,s->feature.outline.vertices,
                s->feature.outline.vertex_count) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_REGION) {
        const DeletedSlabRegionSnapshot *s=state == NULL ? NULL :
            &state->deleted_slab_region;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        return state != NULL && state->type == command->type &&
            s->slab_id == command->data.delete_slab_region.slab_id &&
            s->index == command->data.delete_slab_region.feature_index &&
            result->data.slab_feature.slab_id == s->slab_id &&
            result->data.slab_feature.feature_index == s->index && slab != NULL &&
            s->original_count != 0 &&
            slab->definition.regions.count == s->original_count - 1 &&
            slab_insert_region_at(slab,s->index,s->feature.outline.vertices,
                s->feature.outline.vertex_count,s->feature.top_level_offset_mm,
                s->feature.thickness_mm) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE) {
        const DeletedSlabEdgeRebateSnapshot *s=state == NULL ? NULL :
            &state->deleted_slab_edge_rebate;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        return state != NULL && state->type == command->type &&
            s->slab_id == command->data.delete_slab_edge_rebate.slab_id &&
            s->index == command->data.delete_slab_edge_rebate.feature_index &&
            result->data.slab_feature.slab_id == s->slab_id &&
            result->data.slab_feature.feature_index == s->index && slab != NULL &&
            s->original_count != 0 &&
            slab->definition.edge_rebates.count == s->original_count - 1 &&
            slab_insert_edge_rebate_at(slab,s->index,s->feature.edge_index,
                s->feature.start_offset_mm,s->feature.end_offset_mm,
                s->feature.width_mm,s->feature.depth_mm) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_SLAB) {
        const EditedSlabSnapshot *s=state == NULL ? NULL : &state->edited_slab;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        const EditSlabCommand *edit=&command->data.edit_slab;
        return state != NULL && state->type == command->type && slab != NULL &&
            s->slab_id == edit->slab_id && result->data.slab.slab_id == s->slab_id &&
            slab->definition.thickness_mm == edit->thickness_mm &&
            slab->definition.top_level_offset_mm == edit->top_level_offset_mm &&
            slab_set_base_properties(slab,s->thickness_mm,s->top_level_offset_mm) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_SLAB_REGION) {
        const EditedSlabRegionSnapshot *s=state == NULL ? NULL : &state->edited_slab_region;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        const SlabRegion *region=slab == NULL ? NULL : slab_region_at(slab,s->index);
        const EditSlabRegionCommand *edit=&command->data.edit_slab_region;
        return state != NULL && state->type == command->type && slab != NULL && region != NULL &&
            s->slab_id == edit->slab_id && s->index == edit->feature_index &&
            result->data.slab_feature.slab_id == s->slab_id &&
            result->data.slab_feature.feature_index == s->index &&
            slab->definition.regions.count == s->original_count &&
            outlines_equal(&region->outline,&s->feature.outline) &&
            region->top_level_offset_mm == edit->top_level_offset_mm &&
            region->thickness_mm == edit->thickness_mm &&
            slab_set_region_properties(slab,s->index,s->feature.top_level_offset_mm,
                s->feature.thickness_mm) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE) {
        const EditedSlabEdgeRebateSnapshot *s=state == NULL ? NULL : &state->edited_slab_edge_rebate;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        const SlabEdgeRebate *rebate=slab == NULL ? NULL : slab_edge_rebate_at(slab,s->index);
        const EditSlabEdgeRebateCommand *edit=&command->data.edit_slab_edge_rebate;
        SlabEdgeRebate expected={edit->edge_index,edit->start_offset_mm,edit->end_offset_mm,
            edit->width_mm,edit->depth_mm};
        return state != NULL && state->type == command->type && slab != NULL &&
            s->slab_id == edit->slab_id && s->index == edit->feature_index &&
            result->data.slab_feature.slab_id == s->slab_id &&
            result->data.slab_feature.feature_index == s->index &&
            slab->definition.edge_rebates.count == s->original_count &&
            rebate_equal(rebate,&expected) &&
            slab_set_edge_rebate_properties(slab,s->index,s->feature.start_offset_mm,
                s->feature.end_offset_mm,s->feature.width_mm,s->feature.depth_mm) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_MOVE_SLAB_VERTEX) {
        const MovedSlabVertexSnapshot *s=state == NULL ? NULL : &state->moved_slab_vertex;
        const MoveSlabVertexCommand *move=&command->data.move_slab_vertex;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        return state != NULL && state->type == command->type && s != NULL &&
            move->slab_id == s->slab_id && move->target == s->target &&
            move->feature_index == s->feature_index && move->vertex_index == s->vertex_index &&
            result->data.slab_vertex.slab_id == s->slab_id &&
            result->data.slab_vertex.feature_index == s->feature_index &&
            result->data.slab_vertex.vertex_index == s->vertex_index &&
            moved_vertex_snapshot_matches(slab,s,1,move->new_position) &&
            restore_moved_vertex(slab,s);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB) {
        DomainId id=command->data.delete_slab.slab_id;
        return state != NULL && state->type == command->type &&
            result->data.slab.slab_id == id && state->deleted_slab.slab.id == id &&
            project->domain_ids.next != DOMAIN_ID_INVALID && id < project->domain_ids.next &&
            sitehelper_project_insert_slab_at(project,state->deleted_slab.storey_id,
                &state->deleted_slab.slab,state->deleted_slab.index);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_OPENING) {
        if (state == NULL || state->type != command->type ||
            state->previous_opening.wall_id != command->data.edit_opening.wall_id ||
            state->previous_opening.opening_id != command->data.edit_opening.opening_id ||
            state->previous_opening.wall_id != result->data.edit_opening.wall_id ||
            state->previous_opening.opening_id != result->data.edit_opening.opening_id) {
            return 0;
        }
        return edit_opening_command_execute(project, &state->previous_opening);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR ||
        command->type == SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT) {
        DomainId id = command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR
            ? command->data.delete_room_separator.separator_id
            : command->data.move_room_separator_endpoint.separator_id;
        if (state == NULL || state->type != command->type ||
            state->room_separator.definition.id != id || result->data.room_separator.separator_id != id) {
            return 0;
        }
        if (command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR) {
            return sitehelper_project_insert_room_separator(project, state->room_separator.storey_id,
                &state->room_separator.definition, state->room_separator.index);
        }
        return sitehelper_project_set_room_separator_segment(project, id,
            state->room_separator.definition.segment);
    }
    if (command->type == SITEHELPER_COMMAND_SET_ROOM_LOCATION) {
        if (state == NULL || state->type != command->type ||
            state->previous_room_location.room_id != command->data.room_location.room_id ||
            state->previous_room_location.room_id != result->data.room_location.room_id) {
            return 0;
        }
        return room_location_command_execute(project, &state->previous_room_location);
    }
    if (command->type == SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT) {
        if (state == NULL || state->type != command->type ||
            state->moved_wall.wall_id != command->data.move_wall_endpoint.wall_id ||
            state->moved_wall.wall_id != result->data.move_wall_endpoint.wall_id) {
            return 0;
        }
        Wall *wall = sitehelper_project_find_wall_by_id(project, state->moved_wall.wall_id);
        const Storey *storey = sitehelper_project_find_owning_storey_const(project, state->moved_wall.wall_id);
        BuildSettings resolved;
        return storey != NULL && sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved) &&
            wall_apply_plan_segment(wall, &resolved, state->moved_wall.segment);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_WALL) {
        if (state == NULL || state->type != command->type ||
            state->deleted_wall.wall_id != command->data.delete_wall.wall_id ||
            state->deleted_wall.wall_id != result->data.delete_wall.wall_id) {
            return 0;
        }
        return deleted_wall_snapshot_restore(project, &state->deleted_wall);
    }
    return sitehelper_command_undo(project, command, result);
}

int sitehelper_command_redo_with_state(
    SiteHelperProject *project, const SiteHelperCommand *command,
    const SiteHelperCommandResult *result, const SiteHelperCommandUndoState *state)
{
    if (project == NULL || command == NULL || result == NULL ||
        command->type != result->type) { return 0; }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT) {
        const DocumentPlanCallout *current=sitehelper_project_find_callout_by_id_const(
            project,command->data.edit_plan_callout.callout_id);
        return state != NULL && state->type == command->type &&
            result->data.callout.callout_id == command->data.edit_plan_callout.callout_id &&
            callouts_equal(current,&state->previous_callout) &&
            edit_plan_callout_command_execute(project,&command->data.edit_plan_callout);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT) {
        const DocumentPlanCallout *current=sitehelper_project_find_callout_by_id_const(
            project,command->data.delete_plan_callout.callout_id);
        return state != NULL && state->type == command->type &&
            result->data.callout.callout_id == command->data.delete_plan_callout.callout_id &&
            callouts_equal(current,&state->previous_callout) &&
            delete_plan_callout_command_execute(project,&command->data.delete_plan_callout);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION) {
        const DocumentRevision *current=sitehelper_project_find_revision_by_id_const(
            project,command->data.edit_document_revision.revision_id);
        return state != NULL && state->type == command->type &&
            result->data.revision.revision_id == command->data.edit_document_revision.revision_id &&
            revisions_equal(current,&state->previous_revision) &&
            edit_document_revision_command_execute(project,&command->data.edit_document_revision);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION) {
        const DocumentRevision *current=sitehelper_project_find_revision_by_id_const(
            project,command->data.delete_document_revision.revision_id);
        return state != NULL && state->type == command->type &&
            result->data.revision.revision_id == command->data.delete_document_revision.revision_id &&
            revisions_equal(current,&state->previous_revision) &&
            delete_document_revision_command_execute(project,&command->data.delete_document_revision);
    }
    if (command->type == SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION) {
        const DocumentPlanRevisionCloud *current =
            sitehelper_project_find_revision_cloud_by_id_const(project,
                command->data.set_plan_revision_cloud_revision.revision_cloud_id);
        return state != NULL && state->type == command->type &&
            result->data.revision_cloud.revision_cloud_id ==
                command->data.set_plan_revision_cloud_revision.revision_cloud_id &&
            revision_clouds_equal(current, &state->previous_revision_cloud) &&
            set_plan_revision_cloud_revision_command_execute(
                project, &command->data.set_plan_revision_cloud_revision);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD) {
        const DocumentPlanRevisionCloud *current =
            sitehelper_project_find_revision_cloud_by_id_const(
                project, command->data.edit_plan_revision_cloud.revision_cloud_id);
        return state != NULL && state->type == command->type &&
            result->data.revision_cloud.revision_cloud_id ==
                command->data.edit_plan_revision_cloud.revision_cloud_id &&
            revision_clouds_equal(current, &state->previous_revision_cloud) &&
            edit_plan_revision_cloud_command_execute(
                project, &command->data.edit_plan_revision_cloud);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD) {
        const DocumentPlanRevisionCloud *current =
            sitehelper_project_find_revision_cloud_by_id_const(
                project, command->data.delete_plan_revision_cloud.revision_cloud_id);
        return state != NULL && state->type == command->type &&
            result->data.revision_cloud.revision_cloud_id ==
                command->data.delete_plan_revision_cloud.revision_cloud_id &&
            revision_clouds_equal(current, &state->previous_revision_cloud) &&
            delete_plan_revision_cloud_command_execute(
                project, &command->data.delete_plan_revision_cloud);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL) {
        const DocumentPlanSymbol *current=sitehelper_project_find_symbol_by_id_const(
            project,command->data.edit_plan_symbol.symbol_id);
        return state != NULL && state->type == command->type &&
            result->data.symbol.symbol_id == command->data.edit_plan_symbol.symbol_id &&
            symbols_equal(current,&state->previous_symbol) &&
            edit_plan_symbol_command_execute(project,&command->data.edit_plan_symbol);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL) {
        const DocumentPlanSymbol *current=sitehelper_project_find_symbol_by_id_const(
            project,command->data.delete_plan_symbol.symbol_id);
        return state != NULL && state->type == command->type &&
            result->data.symbol.symbol_id == command->data.delete_plan_symbol.symbol_id &&
            symbols_equal(current,&state->previous_symbol) &&
            delete_plan_symbol_command_execute(project,&command->data.delete_plan_symbol);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION) {
        const DocumentPlanDimension *current=sitehelper_project_find_dimension_by_id_const(
            project,command->data.edit_plan_dimension.dimension_id);
        return state != NULL && state->type == command->type &&
            result->data.dimension.dimension_id == command->data.edit_plan_dimension.dimension_id &&
            dimensions_equal(current,&state->previous_dimension) &&
            edit_plan_dimension_command_execute(project,&command->data.edit_plan_dimension);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION) {
        const DocumentPlanDimension *current=sitehelper_project_find_dimension_by_id_const(
            project,command->data.delete_plan_dimension.dimension_id);
        return state != NULL && state->type == command->type &&
            result->data.dimension.dimension_id == command->data.delete_plan_dimension.dimension_id &&
            dimensions_equal(current,&state->previous_dimension) &&
            delete_plan_dimension_command_execute(project,&command->data.delete_plan_dimension);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_NOTE) {
        const DocumentAnnotation *current = sitehelper_project_find_annotation_by_id_const(
            project, command->data.edit_plan_note.annotation_id);
        return state != NULL && state->type == command->type &&
            result->data.annotation.annotation_id == command->data.edit_plan_note.annotation_id &&
            annotations_equal(current, &state->previous_annotation) &&
            edit_plan_note_command_execute(project, &command->data.edit_plan_note);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_PLAN_NOTE) {
        const DocumentAnnotation *current = sitehelper_project_find_annotation_by_id_const(
            project, command->data.delete_plan_note.annotation_id);
        return state != NULL && state->type == command->type &&
            result->data.annotation.annotation_id == command->data.delete_plan_note.annotation_id &&
            annotations_equal(current, &state->previous_annotation) &&
            delete_plan_note_command_execute(project, &command->data.delete_plan_note);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_ROOF_SOURCE) {
        if (state == NULL || state->type != command->type ||
            state->edited_roof.roof.id != command->data.edit_roof_source.roof_id ||
            result->data.roof.roof_id != command->data.edit_roof_source.roof_id) { return 0; }
        const Roof *current = sitehelper_project_find_roof_by_id_const(project,
            state->edited_roof.roof.id);
        return current != NULL && roof_authority_equal(current, &state->edited_roof.roof) &&
            roof_source_edit_command_redo(project, &command->data.edit_roof_source,
                result->data.roof.portion_id);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_ROOF) {
        DomainId id = command->data.delete_roof.roof_id;
        const Roof *roof = sitehelper_project_find_roof_by_id_const(project, id);
        return state != NULL && state->type == command->type &&
            result->data.roof.roof_id == id && state->deleted_roof.roof.id == id &&
            roof != NULL && delete_roof_command_execute(project, &command->data.delete_roof);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION) {
        const DeletedSlabPenetrationSnapshot *s=state == NULL ? NULL :
            &state->deleted_slab_penetration;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        const SlabPenetration *feature=slab == NULL ? NULL : slab_penetration_at(slab,s->index);
        if (state == NULL || state->type != command->type ||
            command->data.delete_slab_penetration.slab_id != s->slab_id ||
            command->data.delete_slab_penetration.feature_index != s->index ||
            result->data.slab_feature.slab_id != s->slab_id ||
            result->data.slab_feature.feature_index != s->index || feature == NULL ||
            slab->definition.penetrations.count != s->original_count ||
            !outlines_equal(&feature->outline,&s->feature.outline)) { return 0; }
        return slab_remove_penetration(slab,s->index) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_REGION) {
        const DeletedSlabRegionSnapshot *s=state == NULL ? NULL :
            &state->deleted_slab_region;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        const SlabRegion *feature=slab == NULL ? NULL : slab_region_at(slab,s->index);
        if (state == NULL || state->type != command->type ||
            command->data.delete_slab_region.slab_id != s->slab_id ||
            command->data.delete_slab_region.feature_index != s->index ||
            result->data.slab_feature.slab_id != s->slab_id ||
            result->data.slab_feature.feature_index != s->index || feature == NULL ||
            slab->definition.regions.count != s->original_count ||
            feature->top_level_offset_mm != s->feature.top_level_offset_mm ||
            feature->thickness_mm != s->feature.thickness_mm ||
            !outlines_equal(&feature->outline,&s->feature.outline)) { return 0; }
        return slab_remove_region(slab,s->index) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE) {
        const DeletedSlabEdgeRebateSnapshot *s=state == NULL ? NULL :
            &state->deleted_slab_edge_rebate;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        const SlabEdgeRebate *feature=slab == NULL ? NULL : slab_edge_rebate_at(slab,s->index);
        if (state == NULL || state->type != command->type ||
            command->data.delete_slab_edge_rebate.slab_id != s->slab_id ||
            command->data.delete_slab_edge_rebate.feature_index != s->index ||
            result->data.slab_feature.slab_id != s->slab_id ||
            result->data.slab_feature.feature_index != s->index ||
            slab == NULL || slab->definition.edge_rebates.count != s->original_count ||
            !rebate_equal(feature,&s->feature)) { return 0; }
        return slab_remove_edge_rebate(slab,s->index) == SLAB_SUCCESS;
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_SLAB) {
        const EditedSlabSnapshot *s=state == NULL ? NULL : &state->edited_slab;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        return state != NULL && state->type == command->type && slab != NULL &&
            command->data.edit_slab.slab_id == s->slab_id &&
            result->data.slab.slab_id == s->slab_id &&
            slab->definition.thickness_mm == s->thickness_mm &&
            slab->definition.top_level_offset_mm == s->top_level_offset_mm &&
            edit_slab_command_execute(project,&command->data.edit_slab);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_SLAB_REGION) {
        const EditedSlabRegionSnapshot *s=state == NULL ? NULL : &state->edited_slab_region;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        const SlabRegion *region=slab == NULL ? NULL : slab_region_at(slab,s->index);
        return state != NULL && state->type == command->type && slab != NULL && region != NULL &&
            command->data.edit_slab_region.slab_id == s->slab_id &&
            command->data.edit_slab_region.feature_index == s->index &&
            result->data.slab_feature.slab_id == s->slab_id &&
            result->data.slab_feature.feature_index == s->index &&
            slab->definition.regions.count == s->original_count &&
            outlines_equal(&region->outline,&s->feature.outline) &&
            region->top_level_offset_mm == s->feature.top_level_offset_mm &&
            region->thickness_mm == s->feature.thickness_mm &&
            edit_slab_region_command_execute(project,&command->data.edit_slab_region);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE) {
        const EditedSlabEdgeRebateSnapshot *s=state == NULL ? NULL : &state->edited_slab_edge_rebate;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        const SlabEdgeRebate *rebate=slab == NULL ? NULL : slab_edge_rebate_at(slab,s->index);
        return state != NULL && state->type == command->type && slab != NULL &&
            command->data.edit_slab_edge_rebate.slab_id == s->slab_id &&
            command->data.edit_slab_edge_rebate.feature_index == s->index &&
            result->data.slab_feature.slab_id == s->slab_id &&
            result->data.slab_feature.feature_index == s->index &&
            slab->definition.edge_rebates.count == s->original_count &&
            rebate_equal(rebate,&s->feature) &&
            edit_slab_edge_rebate_command_execute(project,&command->data.edit_slab_edge_rebate);
    }
    if (command->type == SITEHELPER_COMMAND_MOVE_SLAB_VERTEX) {
        const MovedSlabVertexSnapshot *s=state == NULL ? NULL : &state->moved_slab_vertex;
        const MoveSlabVertexCommand *move=&command->data.move_slab_vertex;
        Slab *slab=s == NULL ? NULL : sitehelper_project_find_slab_by_id(project,s->slab_id);
        return state != NULL && state->type == command->type && s != NULL &&
            move->slab_id == s->slab_id && move->target == s->target &&
            move->feature_index == s->feature_index && move->vertex_index == s->vertex_index &&
            result->data.slab_vertex.slab_id == s->slab_id &&
            result->data.slab_vertex.feature_index == s->feature_index &&
            result->data.slab_vertex.vertex_index == s->vertex_index &&
            moved_vertex_snapshot_matches(slab,s,0,move->new_position) &&
            move_slab_vertex_command_execute(project,move);
    }
    return sitehelper_command_redo(project,command,result);
}

int sitehelper_command_from_edit_opening(const EditOpeningCommand *edit,
    SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_EDIT_OPENING, .data.edit_opening = *edit
    };
    return 1;
}

int sitehelper_command_from_create_slab(const CreateSlabCommand *create,
    SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_CREATE_SLAB};
    if (!create_slab_command_clone(create,&candidate.data.create_slab)) { return 0; }
    *command=candidate; return 1;
}

int sitehelper_command_from_delete_slab(const DeleteSlabCommand *deletion,
    SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_DELETE_SLAB,
        .data.delete_slab=*deletion}; return 1;
}

int sitehelper_command_from_add_slab_penetration(
    const AddSlabPenetrationCommand *add, SiteHelperCommand *command)
{
    if (add == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_ADD_SLAB_PENETRATION};
    if (!add_slab_penetration_command_clone(add,
        &candidate.data.add_slab_penetration)) { return 0; }
    *command=candidate; return 1;
}

int sitehelper_command_from_delete_slab_penetration(
    const DeleteSlabPenetrationCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION,
        .data.delete_slab_penetration=*deletion}; return 1;
}

int sitehelper_command_from_add_slab_region(
    const AddSlabRegionCommand *add, SiteHelperCommand *command)
{
    if (add == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_ADD_SLAB_REGION};
    if (!add_slab_region_command_clone(add,&candidate.data.add_slab_region)) { return 0; }
    *command=candidate; return 1;
}

int sitehelper_command_from_delete_slab_region(
    const DeleteSlabRegionCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_DELETE_SLAB_REGION,
        .data.delete_slab_region=*deletion}; return 1;
}

int sitehelper_command_from_add_slab_edge_rebate(
    const AddSlabEdgeRebateCommand *add, SiteHelperCommand *command)
{
    if (add == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_ADD_SLAB_EDGE_REBATE,
        .data.add_slab_edge_rebate=*add}; return 1;
}

int sitehelper_command_from_delete_slab_edge_rebate(
    const DeleteSlabEdgeRebateCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE,
        .data.delete_slab_edge_rebate=*deletion}; return 1;
}

int sitehelper_command_from_edit_slab(const EditSlabCommand *edit, SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_EDIT_SLAB,.data.edit_slab=*edit};
    return 1;
}

int sitehelper_command_from_edit_slab_region(const EditSlabRegionCommand *edit, SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_EDIT_SLAB_REGION,
        .data.edit_slab_region=*edit};
    return 1;
}

int sitehelper_command_from_edit_slab_edge_rebate(const EditSlabEdgeRebateCommand *edit,
    SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE,
        .data.edit_slab_edge_rebate=*edit};
    return 1;
}

int sitehelper_command_from_move_slab_vertex(const MoveSlabVertexCommand *move,
    SiteHelperCommand *command)
{
    if (move == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_MOVE_SLAB_VERTEX,
        .data.move_slab_vertex=*move};
    return 1;
}

int sitehelper_command_from_create_roof(const CreateRoofCommand *create,
    SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate = {.type = SITEHELPER_COMMAND_CREATE_ROOF};
    if (!create_roof_command_clone(create, &candidate.data.create_roof)) { return 0; }
    *command = candidate;
    return 1;
}

int sitehelper_command_from_delete_roof(const DeleteRoofCommand *deletion,
    SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){.type = SITEHELPER_COMMAND_DELETE_ROOF,
        .data.delete_roof = *deletion};
    return 1;
}

int sitehelper_command_from_roof_source_edit(const RoofSourceEditCommand *edit,
    SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate = {.type = SITEHELPER_COMMAND_EDIT_ROOF_SOURCE};
    if (!roof_source_edit_command_clone(edit, &candidate.data.edit_roof_source)) { return 0; }
    *command = candidate;
    return 1;
}

int sitehelper_command_from_create_plan_note(const CreatePlanNoteCommand *create,
    SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate = {.type = SITEHELPER_COMMAND_CREATE_PLAN_NOTE};
    if (!create_plan_note_command_clone(create, &candidate.data.create_plan_note)) { return 0; }
    *command = candidate;
    return 1;
}

int sitehelper_command_from_edit_plan_note(const EditPlanNoteCommand *edit,
    SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate = {.type = SITEHELPER_COMMAND_EDIT_PLAN_NOTE};
    if (!edit_plan_note_command_clone(edit, &candidate.data.edit_plan_note)) { return 0; }
    *command = candidate;
    return 1;
}

int sitehelper_command_from_delete_plan_note(const DeletePlanNoteCommand *deletion,
    SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){.type = SITEHELPER_COMMAND_DELETE_PLAN_NOTE,
        .data.delete_plan_note = *deletion};
    return 1;
}

int sitehelper_command_from_create_plan_dimension(const CreatePlanDimensionCommand *create,
    SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_CREATE_PLAN_DIMENSION,
        .data.create_plan_dimension=*create};
    return 1;
}

int sitehelper_command_from_edit_plan_dimension(const EditPlanDimensionCommand *edit,
    SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION,
        .data.edit_plan_dimension=*edit};
    return 1;
}

int sitehelper_command_from_delete_plan_dimension(const DeletePlanDimensionCommand *deletion,
    SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION,
        .data.delete_plan_dimension=*deletion};
    return 1;
}

int sitehelper_command_from_create_plan_symbol(const CreatePlanSymbolCommand *create,
    SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_CREATE_PLAN_SYMBOL,
        .data.create_plan_symbol=*create};
    return 1;
}

int sitehelper_command_from_edit_plan_symbol(const EditPlanSymbolCommand *edit,
    SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL,
        .data.edit_plan_symbol=*edit};
    return 1;
}

int sitehelper_command_from_delete_plan_symbol(const DeletePlanSymbolCommand *deletion,
    SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL,
        .data.delete_plan_symbol=*deletion};
    return 1;
}

int sitehelper_command_from_create_plan_callout(const CreatePlanCalloutCommand *create,
    SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT};
    if (!create_plan_callout_command_clone(create,&candidate.data.create_plan_callout)) { return 0; }
    *command=candidate;
    return 1;
}

int sitehelper_command_from_edit_plan_callout(const EditPlanCalloutCommand *edit,
    SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT};
    if (!edit_plan_callout_command_clone(edit,&candidate.data.edit_plan_callout)) { return 0; }
    *command=candidate;
    return 1;
}

int sitehelper_command_from_delete_plan_callout(const DeletePlanCalloutCommand *deletion,
    SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT,
        .data.delete_plan_callout=*deletion};
    return 1;
}


int sitehelper_command_from_create_document_revision(
    const CreateDocumentRevisionCommand *create, SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_CREATE_DOCUMENT_REVISION};
    if (!create_document_revision_command_clone(create,&candidate.data.create_document_revision)) {
        return 0;
    }
    sitehelper_command_destroy(command); *command=candidate; return 1;
}

int sitehelper_command_from_edit_document_revision(
    const EditDocumentRevisionCommand *edit, SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION};
    if (!edit_document_revision_command_clone(edit,&candidate.data.edit_document_revision)) {
        return 0;
    }
    sitehelper_command_destroy(command); *command=candidate; return 1;
}

int sitehelper_command_from_delete_document_revision(
    const DeleteDocumentRevisionCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION,
        .data.delete_document_revision=*deletion};
    sitehelper_command_destroy(command); *command=candidate; return 1;
}

int sitehelper_command_from_set_plan_revision_cloud_revision(
    const SetPlanRevisionCloudRevisionCommand *set, SiteHelperCommand *command)
{
    if (set == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION,
        .data.set_plan_revision_cloud_revision = *set
    };
    return 1;
}

int sitehelper_command_from_create_plan_revision_cloud(
    const CreatePlanRevisionCloudCommand *create, SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate = {.type = SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD};
    if (!create_plan_revision_cloud_command_clone(create,
            &candidate.data.create_plan_revision_cloud)) {
        return 0;
    }
    *command = candidate;
    return 1;
}

int sitehelper_command_from_edit_plan_revision_cloud(
    const EditPlanRevisionCloudCommand *edit, SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate = {.type = SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD};
    if (!edit_plan_revision_cloud_command_clone(edit,
            &candidate.data.edit_plan_revision_cloud)) {
        return 0;
    }
    *command = candidate;
    return 1;
}

int sitehelper_command_from_delete_plan_revision_cloud(
    const DeletePlanRevisionCloudCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD,
        .data.delete_plan_revision_cloud = *deletion
    };
    return 1;
}

int sitehelper_command_clone(const SiteHelperCommand *source, SiteHelperCommand *output)
{
    if (source == NULL || output == NULL || source->type <= SITEHELPER_COMMAND_NONE ||
        source->type >= SITEHELPER_COMMAND_COUNT) { return 0; }
    SiteHelperCommand candidate=*source;
    if (source->type == SITEHELPER_COMMAND_CREATE_ROOF) {
        candidate.data.create_roof = (CreateRoofCommand){0};
        if (!create_roof_command_clone(&source->data.create_roof,
            &candidate.data.create_roof)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_EDIT_ROOF_SOURCE) {
        candidate.data.edit_roof_source = (RoofSourceEditCommand){0};
        if (!roof_source_edit_command_clone(&source->data.edit_roof_source,
            &candidate.data.edit_roof_source)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_CREATE_PLAN_NOTE) {
        candidate.data.create_plan_note = (CreatePlanNoteCommand){0};
        if (!create_plan_note_command_clone(&source->data.create_plan_note,
            &candidate.data.create_plan_note)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_EDIT_PLAN_NOTE) {
        candidate.data.edit_plan_note = (EditPlanNoteCommand){0};
        if (!edit_plan_note_command_clone(&source->data.edit_plan_note,
            &candidate.data.edit_plan_note)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT) {
        candidate.data.create_plan_callout=(CreatePlanCalloutCommand){0};
        if (!create_plan_callout_command_clone(&source->data.create_plan_callout,
            &candidate.data.create_plan_callout)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT) {
        candidate.data.edit_plan_callout=(EditPlanCalloutCommand){0};
        if (!edit_plan_callout_command_clone(&source->data.edit_plan_callout,
            &candidate.data.edit_plan_callout)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_CREATE_DOCUMENT_REVISION) {
        candidate.data.create_document_revision=(CreateDocumentRevisionCommand){0};
        if (!create_document_revision_command_clone(&source->data.create_document_revision,
                &candidate.data.create_document_revision)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION) {
        candidate.data.edit_document_revision=(EditDocumentRevisionCommand){0};
        if (!edit_document_revision_command_clone(&source->data.edit_document_revision,
                &candidate.data.edit_document_revision)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD) {
        candidate.data.create_plan_revision_cloud=(CreatePlanRevisionCloudCommand){0};
        if (!create_plan_revision_cloud_command_clone(
                &source->data.create_plan_revision_cloud,
                &candidate.data.create_plan_revision_cloud)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD) {
        candidate.data.edit_plan_revision_cloud=(EditPlanRevisionCloudCommand){0};
        if (!edit_plan_revision_cloud_command_clone(
                &source->data.edit_plan_revision_cloud,
                &candidate.data.edit_plan_revision_cloud)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_CREATE_SLAB) {
        candidate.data.create_slab=(CreateSlabCommand){0};
        if (!create_slab_command_clone(&source->data.create_slab,
            &candidate.data.create_slab)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_ADD_SLAB_PENETRATION) {
        candidate.data.add_slab_penetration=(AddSlabPenetrationCommand){0};
        if (!add_slab_penetration_command_clone(&source->data.add_slab_penetration,
            &candidate.data.add_slab_penetration)) { return 0; }
    } else if (source->type == SITEHELPER_COMMAND_ADD_SLAB_REGION) {
        candidate.data.add_slab_region=(AddSlabRegionCommand){0};
        if (!add_slab_region_command_clone(&source->data.add_slab_region,
            &candidate.data.add_slab_region)) { return 0; }
    }
    *output=candidate; return 1;
}

void sitehelper_command_destroy(SiteHelperCommand *command)
{
    if (command == NULL) { return; }
    if (command->type == SITEHELPER_COMMAND_CREATE_ROOF) {
        create_roof_command_destroy(&command->data.create_roof);
    } else if (command->type == SITEHELPER_COMMAND_EDIT_ROOF_SOURCE) {
        roof_source_edit_command_destroy(&command->data.edit_roof_source);
    } else if (command->type == SITEHELPER_COMMAND_CREATE_PLAN_NOTE) {
        create_plan_note_command_destroy(&command->data.create_plan_note);
    } else if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_NOTE) {
        edit_plan_note_command_destroy(&command->data.edit_plan_note);
    } else if (command->type == SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT) {
        create_plan_callout_command_destroy(&command->data.create_plan_callout);
    } else if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT) {
        edit_plan_callout_command_destroy(&command->data.edit_plan_callout);
    } else if (command->type == SITEHELPER_COMMAND_CREATE_DOCUMENT_REVISION) {
        create_document_revision_command_destroy(&command->data.create_document_revision);
    } else if (command->type == SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION) {
        edit_document_revision_command_destroy(&command->data.edit_document_revision);
    } else if (command->type == SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD) {
        create_plan_revision_cloud_command_destroy(
            &command->data.create_plan_revision_cloud);
    } else if (command->type == SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD) {
        edit_plan_revision_cloud_command_destroy(
            &command->data.edit_plan_revision_cloud);
    } else if (command->type == SITEHELPER_COMMAND_CREATE_SLAB) {
        create_slab_command_destroy(&command->data.create_slab);
    } else if (command->type == SITEHELPER_COMMAND_ADD_SLAB_PENETRATION) {
        add_slab_penetration_command_destroy(&command->data.add_slab_penetration);
    } else if (command->type == SITEHELPER_COMMAND_ADD_SLAB_REGION) {
        add_slab_region_command_destroy(&command->data.add_slab_region);
    }
    *command=(SiteHelperCommand){0};
}

int sitehelper_command_from_delete_wall(
    const DeleteWallCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) {
        return 0;
    }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_DELETE_WALL,
        .data.delete_wall = *deletion
    };
    return 1;
}

int sitehelper_command_from_move_wall_endpoint(
    const MoveWallEndpointCommand *move, SiteHelperCommand *command)
{
    if (move == NULL || command == NULL) {
        return 0;
    }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT,
        .data.move_wall_endpoint = *move
    };
    return 1;
}

int sitehelper_command_from_room_location(
    const RoomLocationCommand *placement, SiteHelperCommand *command)
{
    if (placement == NULL || command == NULL) {
        return 0;
    }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_SET_ROOM_LOCATION,
        .data.room_location = *placement
    };
    return 1;
}

int sitehelper_command_from_add_room_separator(const AddRoomSeparatorCommand *add, SiteHelperCommand *command)
{
    if (add == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){.type = SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR, .data.add_room_separator = *add};
    return 1;
}

int sitehelper_command_from_delete_room_separator(const DeleteRoomSeparatorCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){.type = SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR, .data.delete_room_separator = *deletion};
    return 1;
}

int sitehelper_command_from_move_room_separator_endpoint(const MoveRoomSeparatorEndpointCommand *move, SiteHelperCommand *command)
{
    if (move == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){.type = SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT, .data.move_room_separator_endpoint = *move};
    return 1;
}

int sitehelper_command_from_opening(
    const OpeningCommand *opening,
    SiteHelperCommand *command
)
{
    if (
        opening == NULL
        || command == NULL
    ) {
        return 0;
    }

    *command = (SiteHelperCommand){
        .type =
            SITEHELPER_COMMAND_ADD_OPENING,

        .data.opening =
            *opening
    };

    return 1;
}

int sitehelper_command_from_wall(
    const WallCommand *wall,
    SiteHelperCommand *command
)
{
    if (wall == NULL || command == NULL) {
        return 0;
    }

    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_ADD_WALL,
        .data.wall = *wall
    };

    return 1;
}

int sitehelper_command_execute(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    SiteHelperCommandResult *result
)
{
    if (
        project == NULL
        || command == NULL
        || result == NULL
    ) {
        return 0;
    }

    /*
     * Failure must never leave stale
     * information in the result.
     */
    *result =
        (SiteHelperCommandResult){
            .type =
                SITEHELPER_COMMAND_NONE
        };

    switch (command->type) {

        case SITEHELPER_COMMAND_ADD_SLAB_PENETRATION:
        case SITEHELPER_COMMAND_ADD_SLAB_REGION:
        case SITEHELPER_COMMAND_ADD_SLAB_EDGE_REBATE: {
            size_t index;
            DomainId slab_id;
            int ok;
            if (command->type == SITEHELPER_COMMAND_ADD_SLAB_PENETRATION) {
                slab_id=command->data.add_slab_penetration.slab_id;
                ok=add_slab_penetration_command_execute(project,
                    &command->data.add_slab_penetration,&index);
            } else if (command->type == SITEHELPER_COMMAND_ADD_SLAB_REGION) {
                slab_id=command->data.add_slab_region.slab_id;
                ok=add_slab_region_command_execute(project,
                    &command->data.add_slab_region,&index);
            } else {
                slab_id=command->data.add_slab_edge_rebate.slab_id;
                ok=add_slab_edge_rebate_command_execute(project,
                    &command->data.add_slab_edge_rebate,&index);
            }
            if (!ok) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.slab_feature={slab_id,index}};
            return 1;
        }
        case SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION:
        case SITEHELPER_COMMAND_DELETE_SLAB_REGION:
        case SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE: {
            DomainId slab_id;
            size_t index;
            int ok;
            if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION) {
                slab_id=command->data.delete_slab_penetration.slab_id;
                index=command->data.delete_slab_penetration.feature_index;
                ok=delete_slab_penetration_command_execute(project,
                    &command->data.delete_slab_penetration);
            } else if (command->type == SITEHELPER_COMMAND_DELETE_SLAB_REGION) {
                slab_id=command->data.delete_slab_region.slab_id;
                index=command->data.delete_slab_region.feature_index;
                ok=delete_slab_region_command_execute(project,
                    &command->data.delete_slab_region);
            } else {
                slab_id=command->data.delete_slab_edge_rebate.slab_id;
                index=command->data.delete_slab_edge_rebate.feature_index;
                ok=delete_slab_edge_rebate_command_execute(project,
                    &command->data.delete_slab_edge_rebate);
            }
            if (!ok) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.slab_feature={slab_id,index}};
            return 1;
        }

        case SITEHELPER_COMMAND_EDIT_SLAB:
            if (!edit_slab_command_execute(project,&command->data.edit_slab)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.slab={command->data.edit_slab.slab_id}};
            return 1;
        case SITEHELPER_COMMAND_EDIT_SLAB_REGION:
            if (!edit_slab_region_command_execute(project,&command->data.edit_slab_region)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.slab_feature={command->data.edit_slab_region.slab_id,
                    command->data.edit_slab_region.feature_index}};
            return 1;
        case SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE:
            if (!edit_slab_edge_rebate_command_execute(project,&command->data.edit_slab_edge_rebate)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.slab_feature={command->data.edit_slab_edge_rebate.slab_id,
                    command->data.edit_slab_edge_rebate.feature_index}};
            return 1;
        case SITEHELPER_COMMAND_MOVE_SLAB_VERTEX:
            if (!move_slab_vertex_command_execute(project,&command->data.move_slab_vertex)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.slab_vertex={command->data.move_slab_vertex.slab_id,
                    command->data.move_slab_vertex.feature_index,
                    command->data.move_slab_vertex.vertex_index}};
            return 1;

        case SITEHELPER_COMMAND_CREATE_PLAN_NOTE: {
            DomainId id;
            if (!create_plan_note_command_execute(project, &command->data.create_plan_note, &id)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.annotation = {id}};
            return 1;
        }
        case SITEHELPER_COMMAND_EDIT_PLAN_NOTE:
            if (!edit_plan_note_command_execute(project, &command->data.edit_plan_note)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.annotation = {command->data.edit_plan_note.annotation_id}};
            return 1;
        case SITEHELPER_COMMAND_DELETE_PLAN_NOTE:
            if (!delete_plan_note_command_execute(project, &command->data.delete_plan_note)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.annotation = {command->data.delete_plan_note.annotation_id}};
            return 1;

        case SITEHELPER_COMMAND_CREATE_PLAN_DIMENSION: {
            DomainId id;
            if (!create_plan_dimension_command_execute(project,
                    &command->data.create_plan_dimension,&id)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,.data.dimension={id}};
            return 1;
        }
        case SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION:
            if (!edit_plan_dimension_command_execute(project,
                    &command->data.edit_plan_dimension)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.dimension={command->data.edit_plan_dimension.dimension_id}};
            return 1;
        case SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION:
            if (!delete_plan_dimension_command_execute(project,
                    &command->data.delete_plan_dimension)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.dimension={command->data.delete_plan_dimension.dimension_id}};
            return 1;

        case SITEHELPER_COMMAND_CREATE_PLAN_SYMBOL: {
            DomainId id;
            if (!create_plan_symbol_command_execute(project,&command->data.create_plan_symbol,&id)) {
                return 0;
            }
            *result=(SiteHelperCommandResult){.type=command->type,.data.symbol={id}};
            return 1;
        }
        case SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL:
            if (!edit_plan_symbol_command_execute(project,&command->data.edit_plan_symbol)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.symbol={command->data.edit_plan_symbol.symbol_id}};
            return 1;
        case SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL:
            if (!delete_plan_symbol_command_execute(project,&command->data.delete_plan_symbol)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.symbol={command->data.delete_plan_symbol.symbol_id}};
            return 1;

        case SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT: {
            DomainId id;
            if (!create_plan_callout_command_execute(project,&command->data.create_plan_callout,&id)) {
                return 0;
            }
            *result=(SiteHelperCommandResult){.type=command->type,.data.callout={id}};
            return 1;
        }
        case SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT:
            if (!edit_plan_callout_command_execute(project,&command->data.edit_plan_callout)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.callout={command->data.edit_plan_callout.callout_id}};
            return 1;
        case SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT:
            if (!delete_plan_callout_command_execute(project,&command->data.delete_plan_callout)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.callout={command->data.delete_plan_callout.callout_id}};
            return 1;

        case SITEHELPER_COMMAND_CREATE_DOCUMENT_REVISION: {
            DomainId id;
            if (!create_document_revision_command_execute(project,
                    &command->data.create_document_revision,&id)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,.data.revision={id}};
            return 1;
        }
        case SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION:
            if (!edit_document_revision_command_execute(project,
                    &command->data.edit_document_revision)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.revision={command->data.edit_document_revision.revision_id}};
            return 1;
        case SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION:
            if (!delete_document_revision_command_execute(project,
                    &command->data.delete_document_revision)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.revision={command->data.delete_document_revision.revision_id}};
            return 1;

        case SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION:
            if (!set_plan_revision_cloud_revision_command_execute(project,
                    &command->data.set_plan_revision_cloud_revision)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.revision_cloud={
                    command->data.set_plan_revision_cloud_revision.revision_cloud_id}};
            return 1;

        case SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD: {
            DomainId id;
            if (!create_plan_revision_cloud_command_execute(project,
                    &command->data.create_plan_revision_cloud, &id)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){.type=command->type,
                .data.revision_cloud={id}};
            return 1;
        }
        case SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD:
            if (!edit_plan_revision_cloud_command_execute(project,
                    &command->data.edit_plan_revision_cloud)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){.type=command->type,
                .data.revision_cloud={
                    command->data.edit_plan_revision_cloud.revision_cloud_id}};
            return 1;
        case SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD:
            if (!delete_plan_revision_cloud_command_execute(project,
                    &command->data.delete_plan_revision_cloud)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){.type=command->type,
                .data.revision_cloud={
                    command->data.delete_plan_revision_cloud.revision_cloud_id}};
            return 1;

        case SITEHELPER_COMMAND_CREATE_ROOF: {
            DomainId roof_id, portion_id;
            if (!create_roof_command_execute(project, &command->data.create_roof,
                    &roof_id, &portion_id)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.roof = {roof_id, portion_id}};
            return 1;
        }
        case SITEHELPER_COMMAND_DELETE_ROOF:
            if (!delete_roof_command_execute(project, &command->data.delete_roof)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.roof = {command->data.delete_roof.roof_id, DOMAIN_ID_INVALID}};
            return 1;
        case SITEHELPER_COMMAND_EDIT_ROOF_SOURCE: {
            DomainId portion_id = DOMAIN_ID_INVALID;
            if (!roof_source_edit_command_execute(project, &command->data.edit_roof_source,
                    &portion_id)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.roof = {command->data.edit_roof_source.roof_id, portion_id}};
            return 1;
        }

        case SITEHELPER_COMMAND_CREATE_SLAB: {
            DomainId id;
            if (!create_slab_command_execute(project,&command->data.create_slab,&id)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,.data.slab={id}};
            return 1;
        }
        case SITEHELPER_COMMAND_DELETE_SLAB:
            if (!delete_slab_command_execute(project,&command->data.delete_slab)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.slab={command->data.delete_slab.slab_id}};
            return 1;

        case SITEHELPER_COMMAND_EDIT_OPENING:
            if (!edit_opening_command_execute(project, &command->data.edit_opening)) { return 0; }
            *result = (SiteHelperCommandResult){
                .type = command->type,
                .data.edit_opening = {command->data.edit_opening.wall_id,
                    command->data.edit_opening.opening_id}
            };
            return 1;

        case SITEHELPER_COMMAND_ADD_OPENING:
        {
            DomainId opening_id =
                DOMAIN_ID_INVALID;

            if (!opening_command_execute(
                    project,
                    &command->data.opening,
                    &opening_id)) {

                return 0;
            }

            *result =
                (SiteHelperCommandResult){
                    .type =
                        SITEHELPER_COMMAND_ADD_OPENING,

                    .data.add_opening = {

                        .wall_id =
                            command->data.opening.wall_id,

                        .opening_id =
                            opening_id
                    }
                };

            return 1;
        }

        case SITEHELPER_COMMAND_ADD_WALL:
        {
            DomainId wall_id = DOMAIN_ID_INVALID;

            if (!wall_command_execute(
                    project,
                    &command->data.wall,
                    &wall_id)) {

                return 0;
            }

            *result = (SiteHelperCommandResult){
                .type = SITEHELPER_COMMAND_ADD_WALL,
                .data.add_wall = {
                    .wall_id = wall_id
                }
            };

            return 1;
        }

        case SITEHELPER_COMMAND_DELETE_WALL:
            if (!delete_wall_command_execute(project, &command->data.delete_wall)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){
                .type = SITEHELPER_COMMAND_DELETE_WALL,
                .data.delete_wall.wall_id = command->data.delete_wall.wall_id
            };
            return 1;

        case SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT:
            if (!move_wall_endpoint_command_execute(project, &command->data.move_wall_endpoint)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){
                .type = SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT,
                .data.move_wall_endpoint.wall_id = command->data.move_wall_endpoint.wall_id
            };
            return 1;

        case SITEHELPER_COMMAND_SET_ROOM_LOCATION:
            if (!room_location_command_execute(project, &command->data.room_location)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){
                .type = SITEHELPER_COMMAND_SET_ROOM_LOCATION,
                .data.room_location.room_id = command->data.room_location.room_id
            };
            return 1;

        case SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR: {
            DomainId id;
            if (!add_room_separator_command_execute(project, &command->data.add_room_separator, &id)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type, .data.room_separator.separator_id = id};
            return 1;
        }
        case SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR:
            if (!delete_room_separator_command_execute(project, &command->data.delete_room_separator)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.room_separator.separator_id = command->data.delete_room_separator.separator_id};
            return 1;
        case SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT:
            if (!move_room_separator_endpoint_command_execute(project, &command->data.move_room_separator_endpoint)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.room_separator.separator_id = command->data.move_room_separator_endpoint.separator_id};
            return 1;

        case SITEHELPER_COMMAND_NONE:
        case SITEHELPER_COMMAND_COUNT:
        default:
            return 0;
    }
}

int sitehelper_command_undo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
)
{
    if (
        project == NULL
        || command == NULL
        || result == NULL
    ) {
        return 0;
    }

    if (
        command->type
        != result->type
    ) {
        return 0;
    }

    switch (command->type) {

        case SITEHELPER_COMMAND_ADD_SLAB_PENETRATION: {
            const AddSlabPenetrationCommand *add=&command->data.add_slab_penetration;
            Slab *slab=sitehelper_project_find_slab_by_id(project,add->slab_id);
            size_t index=result->data.slab_feature.feature_index;
            const SlabPenetration *feature=slab == NULL ? NULL : slab_penetration_at(slab,index);
            return result->data.slab_feature.slab_id == add->slab_id &&
                slab != NULL && slab->definition.penetrations.count != 0 &&
                index == slab->definition.penetrations.count - 1 &&
                penetration_matches(feature,add->vertices,add->vertex_count) &&
                slab_remove_penetration(slab,index) == SLAB_SUCCESS;
        }
        case SITEHELPER_COMMAND_ADD_SLAB_REGION: {
            const AddSlabRegionCommand *add=&command->data.add_slab_region;
            Slab *slab=sitehelper_project_find_slab_by_id(project,add->slab_id);
            size_t index=result->data.slab_feature.feature_index;
            const SlabRegion *feature=slab == NULL ? NULL : slab_region_at(slab,index);
            return result->data.slab_feature.slab_id == add->slab_id &&
                slab != NULL && slab->definition.regions.count != 0 &&
                index == slab->definition.regions.count - 1 &&
                region_matches(feature,add->vertices,add->vertex_count,
                    add->top_level_offset_mm,add->thickness_mm) &&
                slab_remove_region(slab,index) == SLAB_SUCCESS;
        }
        case SITEHELPER_COMMAND_ADD_SLAB_EDGE_REBATE: {
            const AddSlabEdgeRebateCommand *add=&command->data.add_slab_edge_rebate;
            SlabEdgeRebate expected={add->edge_index,add->start_offset_mm,
                add->end_offset_mm,add->width_mm,add->depth_mm};
            Slab *slab=sitehelper_project_find_slab_by_id(project,add->slab_id);
            size_t index=result->data.slab_feature.feature_index;
            return result->data.slab_feature.slab_id == add->slab_id && slab != NULL &&
                slab->definition.edge_rebates.count != 0 &&
                index == slab->definition.edge_rebates.count - 1 &&
                rebate_equal(slab_edge_rebate_at(slab,index),&expected) &&
                slab_remove_edge_rebate(slab,index) == SLAB_SUCCESS;
        }

        case SITEHELPER_COMMAND_CREATE_PLAN_NOTE:
            return result->data.annotation.annotation_id != DOMAIN_ID_INVALID &&
                create_plan_note_command_undo(project, &command->data.create_plan_note,
                    result->data.annotation.annotation_id);
        case SITEHELPER_COMMAND_CREATE_PLAN_DIMENSION:
            return result->data.dimension.dimension_id != DOMAIN_ID_INVALID &&
                create_plan_dimension_command_undo(project,&command->data.create_plan_dimension,
                    result->data.dimension.dimension_id);
        case SITEHELPER_COMMAND_CREATE_PLAN_SYMBOL:
            return result->data.symbol.symbol_id != DOMAIN_ID_INVALID &&
                create_plan_symbol_command_undo(project,&command->data.create_plan_symbol,
                    result->data.symbol.symbol_id);

        case SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT:
            return result->data.callout.callout_id != DOMAIN_ID_INVALID &&
                create_plan_callout_command_undo(project,&command->data.create_plan_callout,
                    result->data.callout.callout_id);
        case SITEHELPER_COMMAND_CREATE_DOCUMENT_REVISION:
            return result->data.revision.revision_id != DOMAIN_ID_INVALID &&
                create_document_revision_command_undo(project,
                    &command->data.create_document_revision,result->data.revision.revision_id);
        case SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD:
            return result->data.revision_cloud.revision_cloud_id != DOMAIN_ID_INVALID &&
                create_plan_revision_cloud_command_undo(project,
                    &command->data.create_plan_revision_cloud,
                    result->data.revision_cloud.revision_cloud_id);

        case SITEHELPER_COMMAND_CREATE_ROOF:
            return result->data.roof.roof_id != DOMAIN_ID_INVALID &&
                result->data.roof.portion_id != DOMAIN_ID_INVALID &&
                create_roof_command_undo(project, &command->data.create_roof,
                    result->data.roof.roof_id, result->data.roof.portion_id);

        case SITEHELPER_COMMAND_CREATE_SLAB:
            return result->data.slab.slab_id != DOMAIN_ID_INVALID &&
                create_slab_command_undo(project,&command->data.create_slab,
                    result->data.slab.slab_id);

        case SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR: {
            Storey *storey = sitehelper_project_find_storey_by_id(project, command->data.add_room_separator.storey_id);
            return storey != NULL && build_remove_room_separator_by_id(&storey->structure, result->data.room_separator.separator_id);
        }

        case SITEHELPER_COMMAND_ADD_OPENING:

            return opening_command_undo(
                project,
                &command->data.opening,
                result->data.add_opening.opening_id
            );

        case SITEHELPER_COMMAND_ADD_WALL:
            return wall_command_undo(
                project,
                &command->data.wall,
                result->data.add_wall.wall_id
            );

        case SITEHELPER_COMMAND_EDIT_PLAN_NOTE:
        case SITEHELPER_COMMAND_DELETE_PLAN_NOTE:
        case SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION:
        case SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION:
        case SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL:
        case SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL:
        case SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT:
        case SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT:
        case SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION:
        case SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION:
        case SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION:
        case SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD:
        case SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD:
        case SITEHELPER_COMMAND_DELETE_WALL:
        case SITEHELPER_COMMAND_DELETE_ROOF:
        case SITEHELPER_COMMAND_EDIT_ROOF_SOURCE:
        case SITEHELPER_COMMAND_DELETE_SLAB:
        case SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION:
        case SITEHELPER_COMMAND_DELETE_SLAB_REGION:
        case SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE:
        case SITEHELPER_COMMAND_EDIT_SLAB:
        case SITEHELPER_COMMAND_EDIT_SLAB_REGION:
        case SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE:
        case SITEHELPER_COMMAND_MOVE_SLAB_VERTEX:
        case SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT:
        case SITEHELPER_COMMAND_EDIT_OPENING:
        case SITEHELPER_COMMAND_SET_ROOM_LOCATION:
        case SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR:
        case SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT:
            return 0; /* Requires history-owned state, never a public result. */

        case SITEHELPER_COMMAND_NONE:
        case SITEHELPER_COMMAND_COUNT:
        default:
            return 0;
    }
}

int sitehelper_command_redo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
)
{
    if (
        project == NULL
        || command == NULL
        || result == NULL
    ) {
        return 0;
    }

    if (
        command->type
        != result->type
    ) {
        return 0;
    }

    switch (command->type) {

        case SITEHELPER_COMMAND_ADD_SLAB_PENETRATION: {
            const AddSlabPenetrationCommand *add=&command->data.add_slab_penetration;
            Slab *slab=sitehelper_project_find_slab_by_id(project,add->slab_id);
            return result->data.slab_feature.slab_id == add->slab_id && slab != NULL &&
                slab->definition.penetrations.count == result->data.slab_feature.feature_index &&
                slab_insert_penetration_at(slab,result->data.slab_feature.feature_index,
                    add->vertices,add->vertex_count) == SLAB_SUCCESS;
        }
        case SITEHELPER_COMMAND_ADD_SLAB_REGION: {
            const AddSlabRegionCommand *add=&command->data.add_slab_region;
            Slab *slab=sitehelper_project_find_slab_by_id(project,add->slab_id);
            return result->data.slab_feature.slab_id == add->slab_id && slab != NULL &&
                slab->definition.regions.count == result->data.slab_feature.feature_index &&
                slab_insert_region_at(slab,result->data.slab_feature.feature_index,
                    add->vertices,add->vertex_count,add->top_level_offset_mm,
                    add->thickness_mm) == SLAB_SUCCESS;
        }
        case SITEHELPER_COMMAND_ADD_SLAB_EDGE_REBATE: {
            const AddSlabEdgeRebateCommand *add=&command->data.add_slab_edge_rebate;
            Slab *slab=sitehelper_project_find_slab_by_id(project,add->slab_id);
            return result->data.slab_feature.slab_id == add->slab_id && slab != NULL &&
                slab->definition.edge_rebates.count == result->data.slab_feature.feature_index &&
                slab_insert_edge_rebate_at(slab,result->data.slab_feature.feature_index,
                    add->edge_index,add->start_offset_mm,add->end_offset_mm,
                    add->width_mm,add->depth_mm) == SLAB_SUCCESS;
        }
        case SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION:
        case SITEHELPER_COMMAND_DELETE_SLAB_REGION:
        case SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE:
        case SITEHELPER_COMMAND_EDIT_SLAB:
        case SITEHELPER_COMMAND_EDIT_SLAB_REGION:
        case SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE:
        case SITEHELPER_COMMAND_MOVE_SLAB_VERTEX:
        case SITEHELPER_COMMAND_EDIT_ROOF_SOURCE:
            return 0; /* History-owned snapshot is required for exact verification. */

        case SITEHELPER_COMMAND_CREATE_PLAN_NOTE:
            return result->data.annotation.annotation_id != DOMAIN_ID_INVALID &&
                create_plan_note_command_redo(project, &command->data.create_plan_note,
                    result->data.annotation.annotation_id);
        case SITEHELPER_COMMAND_CREATE_PLAN_DIMENSION:
            return result->data.dimension.dimension_id != DOMAIN_ID_INVALID &&
                create_plan_dimension_command_redo(project,&command->data.create_plan_dimension,
                    result->data.dimension.dimension_id);
        case SITEHELPER_COMMAND_CREATE_PLAN_SYMBOL:
            return result->data.symbol.symbol_id != DOMAIN_ID_INVALID &&
                create_plan_symbol_command_redo(project,&command->data.create_plan_symbol,
                    result->data.symbol.symbol_id);
        case SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT:
            return result->data.callout.callout_id != DOMAIN_ID_INVALID &&
                create_plan_callout_command_redo(project,&command->data.create_plan_callout,
                    result->data.callout.callout_id);
        case SITEHELPER_COMMAND_CREATE_DOCUMENT_REVISION:
            return result->data.revision.revision_id != DOMAIN_ID_INVALID &&
                create_document_revision_command_redo(project,
                    &command->data.create_document_revision,result->data.revision.revision_id);
        case SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD:
            return result->data.revision_cloud.revision_cloud_id != DOMAIN_ID_INVALID &&
                create_plan_revision_cloud_command_redo(project,
                    &command->data.create_plan_revision_cloud,
                    result->data.revision_cloud.revision_cloud_id);
        case SITEHELPER_COMMAND_EDIT_PLAN_NOTE:
        case SITEHELPER_COMMAND_DELETE_PLAN_NOTE:
        case SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION:
        case SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION:
        case SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL:
        case SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL:
        case SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT:
        case SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT:
        case SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION:
        case SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION:
        case SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION:
        case SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD:
        case SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD:
            return 0; /* History snapshot required for exact verification/restoration. */

        case SITEHELPER_COMMAND_CREATE_ROOF:
            return create_roof_command_redo(project, &command->data.create_roof,
                result->data.roof.roof_id, result->data.roof.portion_id);
        case SITEHELPER_COMMAND_DELETE_ROOF:
            return command->data.delete_roof.roof_id == result->data.roof.roof_id &&
                delete_roof_command_execute(project, &command->data.delete_roof);

        case SITEHELPER_COMMAND_CREATE_SLAB:
            return create_slab_command_redo(project,&command->data.create_slab,
                result->data.slab.slab_id);
        case SITEHELPER_COMMAND_DELETE_SLAB:
            return command->data.delete_slab.slab_id == result->data.slab.slab_id &&
                delete_slab_command_execute(project,&command->data.delete_slab);

        case SITEHELPER_COMMAND_EDIT_OPENING:
            if (command->data.edit_opening.wall_id != result->data.edit_opening.wall_id ||
                command->data.edit_opening.opening_id != result->data.edit_opening.opening_id) { return 0; }
            return edit_opening_command_execute(project, &command->data.edit_opening);

        case SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR: {
            RoomSeparator separator = {.id = result->data.room_separator.separator_id,
                .segment = command->data.add_room_separator.segment};
            Storey *storey = sitehelper_project_find_storey_by_id(project, command->data.add_room_separator.storey_id);
            return storey != NULL && sitehelper_project_insert_room_separator(project, storey->id, &separator,
                storey->structure.room_separator_count);
        }
        case SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR:
            if (command->data.delete_room_separator.separator_id != result->data.room_separator.separator_id) { return 0; }
            return delete_room_separator_command_execute(project, &command->data.delete_room_separator);
        case SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT:
            if (command->data.move_room_separator_endpoint.separator_id != result->data.room_separator.separator_id) { return 0; }
            return move_room_separator_endpoint_command_execute(project, &command->data.move_room_separator_endpoint);

        case SITEHELPER_COMMAND_ADD_OPENING:

            return opening_command_redo(
                project,
                &command->data.opening,
                result->data.add_opening.opening_id
            );

        case SITEHELPER_COMMAND_ADD_WALL:
            return wall_command_redo(
                project,
                &command->data.wall,
                result->data.add_wall.wall_id
            );

        case SITEHELPER_COMMAND_DELETE_WALL:
            if (command->data.delete_wall.wall_id != result->data.delete_wall.wall_id) {
                return 0;
            }
            return delete_wall_command_execute(project, &command->data.delete_wall);

        case SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT:
            if (command->data.move_wall_endpoint.wall_id != result->data.move_wall_endpoint.wall_id) {
                return 0;
            }
            return move_wall_endpoint_command_execute(project, &command->data.move_wall_endpoint);

        case SITEHELPER_COMMAND_SET_ROOM_LOCATION:
            if (command->data.room_location.room_id != result->data.room_location.room_id) {
                return 0;
            }
            return room_location_command_execute(project, &command->data.room_location);

        case SITEHELPER_COMMAND_NONE:
        case SITEHELPER_COMMAND_COUNT:
        default:
            return 0;
    }
}
