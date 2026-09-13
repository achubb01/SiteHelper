#include "editor_properties.h"
#include "wall.h"

int sitehelper_editor_inspect_properties(const SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorProperties *properties)
{
    if (properties == NULL) { return 0; }
    *properties = (EditorProperties){0};
    if (editor == NULL || project == NULL ||
        !sitehelper_editor_selection_matches_view(editor)) { return 0; }
    const EditorSelection *selection = &editor->selection;
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
