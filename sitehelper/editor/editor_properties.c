#include "editor_properties.h"
#include "wall.h"
#include "slab.h"

static const Slab *selected_slab_in_storey(const SiteHelperEditor *editor,
    const SiteHelperProject *project, const Storey **storey_output)
{
    if (editor == NULL || project == NULL ||
        !sitehelper_editor_selection_matches_view(editor) ||
        editor->selection.scope != EDITOR_SELECTION_SCOPE_PLAN ||
        editor->selection.slab_id == DOMAIN_ID_INVALID) { return NULL; }
    const Storey *storey=sitehelper_project_find_storey_by_id_const(project,
        editor->current_storey_id);
    if (storey == NULL) { return NULL; }
    const Slab *slab=slab_collection_find_by_id_const(&storey->slabs,
        editor->selection.slab_id);
    if (slab == NULL || slab_validate(slab) != SLAB_SUCCESS) { return NULL; }
    if (storey_output != NULL) { *storey_output=storey; }
    return slab;
}

int sitehelper_editor_inspect_properties(const SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorProperties *properties)
{
    if (properties == NULL) { return 0; }
    *properties = (EditorProperties){0};
    if (editor == NULL || project == NULL ||
        !sitehelper_editor_selection_matches_view(editor)) { return 0; }
    const EditorSelection *selection = &editor->selection;

    if (selection->kind == EDITOR_SELECTION_SLAB ||
        selection->kind == EDITOR_SELECTION_SLAB_PENETRATION ||
        selection->kind == EDITOR_SELECTION_SLAB_REGION ||
        selection->kind == EDITOR_SELECTION_SLAB_EDGE_REBATE) {
        const Slab *slab=selected_slab_in_storey(editor,project,NULL);
        if (slab == NULL) { return 0; }
        if (selection->kind == EDITOR_SELECTION_SLAB) {
            *properties=(EditorProperties){.kind=selection->kind,.data.slab={
                .slab_id=slab->id,
                .thickness_mm=slab->definition.thickness_mm,
                .top_level_offset_mm=slab->definition.top_level_offset_mm,
                .vertex_count=slab->definition.outline.vertex_count}};
            return 1;
        }
        size_t index=selection->slab_feature_index;
        if (selection->kind == EDITOR_SELECTION_SLAB_PENETRATION) {
            const SlabPenetration *feature=slab_penetration_at(slab,index);
            if (feature == NULL) { return 0; }
            *properties=(EditorProperties){.kind=selection->kind,
                .data.slab_penetration={slab->id,index,feature->outline.vertex_count}};
            return 1;
        }
        if (selection->kind == EDITOR_SELECTION_SLAB_REGION) {
            const SlabRegion *feature=slab_region_at(slab,index);
            if (feature == NULL) { return 0; }
            *properties=(EditorProperties){.kind=selection->kind,.data.slab_region={
                .slab_id=slab->id,.feature_index=index,
                .top_level_offset_mm=feature->top_level_offset_mm,
                .thickness_mm=feature->thickness_mm,
                .vertex_count=feature->outline.vertex_count}};
            return 1;
        }
        const SlabEdgeRebate *feature=slab_edge_rebate_at(slab,index);
        int length_mm;
        if (feature == NULL || slab_edge_rebate_length_mm(feature,&length_mm) != SLAB_SUCCESS) {
            return 0;
        }
        *properties=(EditorProperties){.kind=selection->kind,.data.slab_edge_rebate={
            .slab_id=slab->id,.feature_index=index,.definition=*feature,.length_mm=length_mm}};
        return 1;
    }

    if (selection->kind != EDITOR_SELECTION_WALL &&
        selection->kind != EDITOR_SELECTION_OPENING) { return 0; }
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    if (storey == NULL) { return 0; }
    const Wall *wall = build_find_wall_by_id_const(&storey->structure, selection->wall_id);
    if (wall == NULL) { return 0; }
    if (selection->kind == EDITOR_SELECTION_WALL) {
        BuildSettings resolved;
        if (!sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved)) { return 0; }
        *properties = (EditorProperties){
            .kind = EDITOR_SELECTION_WALL,
            .data.wall = {
                .wall_id = wall->id, .segment = wall->definition.segment,
                .length_mm = wall_length_mm(wall),
                .resolved_stud_height = resolved.stud_height,
                .resolved_stud_spacing = resolved.stud_spacing
            }
        };
        return 1;
    }
    const Opening *opening = wall_find_opening_by_id_const(wall, selection->opening_id);
    if (opening == NULL) { return 0; }
    *properties = (EditorProperties){
        .kind = EDITOR_SELECTION_OPENING,
        .data.opening = {.wall_id = wall->id, .definition = *opening}
    };
    return 1;
}

int sitehelper_editor_property_millimetres(const SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorProperty property, int *millimetres)
{
    if (millimetres == NULL) { return 0; }
    EditorProperties p;
    if (!sitehelper_editor_inspect_properties(editor,project,&p)) { return 0; }
    int value;
    switch (property) {
        case EDITOR_PROPERTY_WALL_START_X:
            if (p.kind != EDITOR_SELECTION_WALL) { return 0; }
            value=p.data.wall.segment.start.x; break;
        case EDITOR_PROPERTY_WALL_START_Y:
            if (p.kind != EDITOR_SELECTION_WALL) { return 0; }
            value=p.data.wall.segment.start.y; break;
        case EDITOR_PROPERTY_WALL_END_X:
            if (p.kind != EDITOR_SELECTION_WALL) { return 0; }
            value=p.data.wall.segment.end.x; break;
        case EDITOR_PROPERTY_WALL_END_Y:
            if (p.kind != EDITOR_SELECTION_WALL) { return 0; }
            value=p.data.wall.segment.end.y; break;
        case EDITOR_PROPERTY_OPENING_FRAME_POSITION:
            if (p.kind != EDITOR_SELECTION_OPENING) { return 0; }
            value=p.data.opening.definition.frame_position; break;
        case EDITOR_PROPERTY_OPENING_FRAME_BOTTOM:
            if (p.kind != EDITOR_SELECTION_OPENING) { return 0; }
            value=p.data.opening.definition.frame_bottom; break;
        case EDITOR_PROPERTY_OPENING_WIDTH:
            if (p.kind != EDITOR_SELECTION_OPENING) { return 0; }
            value=p.data.opening.definition.width; break;
        case EDITOR_PROPERTY_OPENING_HEIGHT:
            if (p.kind != EDITOR_SELECTION_OPENING) { return 0; }
            value=p.data.opening.definition.height; break;
        case EDITOR_PROPERTY_OPENING_WIDTH_ALLOWANCE:
            if (p.kind != EDITOR_SELECTION_OPENING) { return 0; }
            value=p.data.opening.definition.width_allowance; break;
        case EDITOR_PROPERTY_OPENING_HEIGHT_ALLOWANCE:
            if (p.kind != EDITOR_SELECTION_OPENING) { return 0; }
            value=p.data.opening.definition.height_allowance; break;
        case EDITOR_PROPERTY_SLAB_THICKNESS:
            if (p.kind != EDITOR_SELECTION_SLAB) { return 0; }
            value=p.data.slab.thickness_mm; break;
        case EDITOR_PROPERTY_SLAB_TOP_LEVEL:
            if (p.kind != EDITOR_SELECTION_SLAB) { return 0; }
            value=p.data.slab.top_level_offset_mm; break;
        case EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL:
            if (p.kind != EDITOR_SELECTION_SLAB_REGION) { return 0; }
            value=p.data.slab_region.top_level_offset_mm; break;
        case EDITOR_PROPERTY_SLAB_REGION_THICKNESS:
            if (p.kind != EDITOR_SELECTION_SLAB_REGION) { return 0; }
            value=p.data.slab_region.thickness_mm; break;
        case EDITOR_PROPERTY_SLAB_REBATE_START:
            if (p.kind != EDITOR_SELECTION_SLAB_EDGE_REBATE) { return 0; }
            value=p.data.slab_edge_rebate.definition.start_offset_mm; break;
        case EDITOR_PROPERTY_SLAB_REBATE_END:
            if (p.kind != EDITOR_SELECTION_SLAB_EDGE_REBATE) { return 0; }
            value=p.data.slab_edge_rebate.definition.end_offset_mm; break;
        case EDITOR_PROPERTY_SLAB_REBATE_WIDTH:
            if (p.kind != EDITOR_SELECTION_SLAB_EDGE_REBATE) { return 0; }
            value=p.data.slab_edge_rebate.definition.width_mm; break;
        case EDITOR_PROPERTY_SLAB_REBATE_DEPTH:
            if (p.kind != EDITOR_SELECTION_SLAB_EDGE_REBATE) { return 0; }
            value=p.data.slab_edge_rebate.definition.depth_mm; break;
        case EDITOR_PROPERTY_OPENING_TYPE:
        case EDITOR_PROPERTY_OPENING_CUSTOM_ALLOWANCE:
        default: return 0;
    }
    *millimetres=value;
    return 1;
}

int sitehelper_editor_create_property_command(const SiteHelperEditor *editor,
    const SiteHelperProject *project, const EditorPropertyEdit *edit,
    SiteHelperCommand *command)
{
    if (command == NULL) { return 0; }
    *command = (SiteHelperCommand){0};
    EditorProperties properties;
    if (edit == NULL || !sitehelper_editor_inspect_properties(editor, project, &properties)) { return 0; }
    if (properties.kind == EDITOR_SELECTION_WALL) {
        WallEndpoint endpoint;
        PlanPosition position;
        WallPlanSegment segment = properties.data.wall.segment;
        switch (edit->property) {
            case EDITOR_PROPERTY_WALL_START_X:
            case EDITOR_PROPERTY_WALL_START_Y:
                endpoint = WALL_ENDPOINT_START;
                position = segment.start;
                break;
            case EDITOR_PROPERTY_WALL_END_X:
            case EDITOR_PROPERTY_WALL_END_Y:
                endpoint = WALL_ENDPOINT_END;
                position = segment.end;
                break;
            default: return 0;
        }
        if (edit->property == EDITOR_PROPERTY_WALL_START_X ||
            edit->property == EDITOR_PROPERTY_WALL_END_X) {
            position.x = edit->value.millimetres;
        }
        else { position.y = edit->value.millimetres; }
        MoveWallEndpointCommand move;
        return move_wall_endpoint_command_create(properties.data.wall.wall_id,
            endpoint, position, &move) && sitehelper_command_from_move_wall_endpoint(&move, command);
    }

    if (properties.kind == EDITOR_SELECTION_OPENING) {
        Opening definition = properties.data.opening.definition;
        switch (edit->property) {
            case EDITOR_PROPERTY_OPENING_TYPE: definition.type = edit->value.opening_type; break;
            case EDITOR_PROPERTY_OPENING_FRAME_POSITION: definition.frame_position = edit->value.millimetres; break;
            case EDITOR_PROPERTY_OPENING_FRAME_BOTTOM: definition.frame_bottom = edit->value.millimetres; break;
            case EDITOR_PROPERTY_OPENING_WIDTH: definition.width = edit->value.millimetres; break;
            case EDITOR_PROPERTY_OPENING_HEIGHT: definition.height = edit->value.millimetres; break;
            case EDITOR_PROPERTY_OPENING_CUSTOM_ALLOWANCE: definition.custom_allowance = edit->value.custom_allowance; break;
            case EDITOR_PROPERTY_OPENING_WIDTH_ALLOWANCE: definition.width_allowance = edit->value.millimetres; break;
            case EDITOR_PROPERTY_OPENING_HEIGHT_ALLOWANCE: definition.height_allowance = edit->value.millimetres; break;
            default: return 0;
        }
        EditOpeningCommand opening;
        return edit_opening_command_create(properties.data.opening.wall_id, definition.id,
            &definition, &opening) && sitehelper_command_from_edit_opening(&opening, command);
    }

    if (properties.kind == EDITOR_SELECTION_SLAB) {
        int thickness=properties.data.slab.thickness_mm;
        int top=properties.data.slab.top_level_offset_mm;
        if (edit->property == EDITOR_PROPERTY_SLAB_THICKNESS) {
            thickness=edit->value.millimetres;
        } else if (edit->property == EDITOR_PROPERTY_SLAB_TOP_LEVEL) {
            top=edit->value.millimetres;
        } else { return 0; }
        EditSlabCommand slab;
        return edit_slab_command_create(properties.data.slab.slab_id,thickness,top,&slab) &&
            sitehelper_command_from_edit_slab(&slab,command);
    }

    if (properties.kind == EDITOR_SELECTION_SLAB_REGION) {
        int top=properties.data.slab_region.top_level_offset_mm;
        int thickness=properties.data.slab_region.thickness_mm;
        if (edit->property == EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL) {
            top=edit->value.millimetres;
        } else if (edit->property == EDITOR_PROPERTY_SLAB_REGION_THICKNESS) {
            thickness=edit->value.millimetres;
        } else { return 0; }
        EditSlabRegionCommand region;
        return edit_slab_region_command_create(properties.data.slab_region.slab_id,
            properties.data.slab_region.feature_index,top,thickness,&region) &&
            sitehelper_command_from_edit_slab_region(&region,command);
    }

    if (properties.kind == EDITOR_SELECTION_SLAB_EDGE_REBATE) {
        SlabEdgeRebate d=properties.data.slab_edge_rebate.definition;
        switch (edit->property) {
            case EDITOR_PROPERTY_SLAB_REBATE_START: d.start_offset_mm=edit->value.millimetres; break;
            case EDITOR_PROPERTY_SLAB_REBATE_END: d.end_offset_mm=edit->value.millimetres; break;
            case EDITOR_PROPERTY_SLAB_REBATE_WIDTH: d.width_mm=edit->value.millimetres; break;
            case EDITOR_PROPERTY_SLAB_REBATE_DEPTH: d.depth_mm=edit->value.millimetres; break;
            default: return 0;
        }
        EditSlabEdgeRebateCommand rebate;
        return edit_slab_edge_rebate_command_create(
            properties.data.slab_edge_rebate.slab_id,
            properties.data.slab_edge_rebate.feature_index,d.edge_index,
            d.start_offset_mm,d.end_offset_mm,d.width_mm,d.depth_mm,&rebate) &&
            sitehelper_command_from_edit_slab_edge_rebate(&rebate,command);
    }

    return 0;
}
