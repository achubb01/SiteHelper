#include "app_properties_panel.h"
#include <stdio.h>
#include <stdarg.h>

static void add_field(AppPropertiesPanel *panel, EditorProperty property,
    const char *label, int millimetres)
{
    if (panel->field_count >= APP_PROPERTY_PANEL_MAX_FIELDS) { return; }
    panel->fields[panel->field_count++]=(AppPropertyField){property,label,millimetres};
}

static void add_info(AppPropertiesPanel *panel, const char *format, ...)
{
    if (panel == NULL || format == NULL ||
        panel->info_count >= APP_PROPERTY_PANEL_MAX_INFO_ROWS) { return; }
    va_list args;
    va_start(args,format);
    (void)vsnprintf(panel->info_rows[panel->info_count],
        sizeof panel->info_rows[panel->info_count],format,args);
    va_end(args);
    panel->info_count++;
}

const char *app_property_label(EditorProperty property)
{
    switch (property) {
        case EDITOR_PROPERTY_WALL_START_X: return "Start X";
        case EDITOR_PROPERTY_WALL_START_Y: return "Start Y";
        case EDITOR_PROPERTY_WALL_END_X: return "End X";
        case EDITOR_PROPERTY_WALL_END_Y: return "End Y";
        case EDITOR_PROPERTY_OPENING_FRAME_POSITION: return "Frame U";
        case EDITOR_PROPERTY_OPENING_FRAME_BOTTOM: return "Frame bottom";
        case EDITOR_PROPERTY_OPENING_WIDTH: return "Width";
        case EDITOR_PROPERTY_OPENING_HEIGHT: return "Height";
        case EDITOR_PROPERTY_OPENING_WIDTH_ALLOWANCE: return "Width allowance";
        case EDITOR_PROPERTY_OPENING_HEIGHT_ALLOWANCE: return "Height allowance";
        case EDITOR_PROPERTY_SLAB_THICKNESS: return "Thickness";
        case EDITOR_PROPERTY_SLAB_TOP_LEVEL: return "Top level";
        case EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL: return "Top level";
        case EDITOR_PROPERTY_SLAB_REGION_THICKNESS: return "Thickness";
        case EDITOR_PROPERTY_SLAB_REBATE_START: return "Start U";
        case EDITOR_PROPERTY_SLAB_REBATE_END: return "End U";
        case EDITOR_PROPERTY_SLAB_REBATE_WIDTH: return "Width";
        case EDITOR_PROPERTY_SLAB_REBATE_DEPTH: return "Depth";
        case EDITOR_PROPERTY_OPENING_TYPE:
        case EDITOR_PROPERTY_OPENING_CUSTOM_ALLOWANCE:
        default: return "Property";
    }
}

static const char *roof_generation_label(RoofPortionGeneration generation)
{
    switch (generation) {
        case ROOF_PORTION_OPPOSING_SLOPES: return "Opposing slopes";
        case ROOF_PORTION_ALL_BOUNDARY_SLOPES: return "All boundary slopes";
        case ROOF_PORTION_SINGLE_SLOPE: return "Single slope";
        default: return "Unknown";
    }
}

static const char *document_selection_title(DocumentObjectKind kind)
{
    switch (kind) {
        case DOCUMENT_OBJECT_NOTE: return "Note";
        case DOCUMENT_OBJECT_DIMENSION: return "Dimension";
        case DOCUMENT_OBJECT_SYMBOL: return "Symbol";
        case DOCUMENT_OBJECT_CALLOUT: return "Callout";
        case DOCUMENT_OBJECT_REVISION_CLOUD: return "Revision cloud";
        case DOCUMENT_OBJECT_NONE:
        default: return "Documentation";
    }
}

int app_properties_panel_build(const SiteHelperEditor *editor,
    const SiteHelperProject *project, AppPropertiesPanel *panel)
{
    if (panel == NULL) { return 0; }
    *panel=(AppPropertiesPanel){0};
    if (editor == NULL || project == NULL) { return 0; }

    if (editor->selection.kind == EDITOR_SELECTION_DOCUMENT &&
        editor_workspace_accepts_selection(editor->active_workspace,
            EDITOR_SELECTION_DOCUMENT) &&
        editor_selection_is_document_kind(&editor->selection,
            editor->selection.document.kind)) {
        panel->title=document_selection_title(editor->selection.document.kind);
        panel->note="Edit with the active documentation tool";
        return 1;
    }

    EditorProperties p;
    if (!sitehelper_editor_inspect_properties(editor,project,&p)) { return 0; }
    switch (p.kind) {
        case EDITOR_SELECTION_WALL:
            panel->title="Wall";
            add_field(panel,EDITOR_PROPERTY_WALL_START_X,"Start X",p.data.wall.segment.start.x);
            add_field(panel,EDITOR_PROPERTY_WALL_START_Y,"Start Y",p.data.wall.segment.start.y);
            add_field(panel,EDITOR_PROPERTY_WALL_END_X,"End X",p.data.wall.segment.end.x);
            add_field(panel,EDITOR_PROPERTY_WALL_END_Y,"End Y",p.data.wall.segment.end.y);
            add_info(panel,"Length: %d mm",p.data.wall.length_mm);
            add_info(panel,"Stud height: %d mm",p.data.wall.resolved_stud_height);
            add_info(panel,"Stud spacing: %d mm",p.data.wall.resolved_stud_spacing);
            return 1;
        case EDITOR_SELECTION_OPENING:
            panel->title="Opening";
            add_field(panel,EDITOR_PROPERTY_OPENING_FRAME_POSITION,"Frame U",
                p.data.opening.definition.frame_position);
            add_field(panel,EDITOR_PROPERTY_OPENING_FRAME_BOTTOM,"Frame bottom",
                p.data.opening.definition.frame_bottom);
            add_field(panel,EDITOR_PROPERTY_OPENING_WIDTH,"Width",
                p.data.opening.definition.width);
            add_field(panel,EDITOR_PROPERTY_OPENING_HEIGHT,"Height",
                p.data.opening.definition.height);
            add_field(panel,EDITOR_PROPERTY_OPENING_WIDTH_ALLOWANCE,"Width allowance",
                p.data.opening.definition.width_allowance);
            add_field(panel,EDITOR_PROPERTY_OPENING_HEIGHT_ALLOWANCE,"Height allowance",
                p.data.opening.definition.height_allowance);
            return 1;
        case EDITOR_SELECTION_SLAB:
            panel->title="Slab";
            add_field(panel,EDITOR_PROPERTY_SLAB_THICKNESS,"Thickness",p.data.slab.thickness_mm);
            add_field(panel,EDITOR_PROPERTY_SLAB_TOP_LEVEL,"Top level",p.data.slab.top_level_offset_mm);
            add_info(panel,"Vertices: %zu",p.data.slab.vertex_count);
            return 1;
        case EDITOR_SELECTION_SLAB_PENETRATION:
            panel->title="Slab void";
            add_info(panel,"Vertices: %zu",p.data.slab_penetration.vertex_count);
            panel->note="Use Geometry to edit the polygon";
            return 1;
        case EDITOR_SELECTION_SLAB_REGION:
            panel->title="Slab region";
            add_field(panel,EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL,"Top level",
                p.data.slab_region.top_level_offset_mm);
            add_field(panel,EDITOR_PROPERTY_SLAB_REGION_THICKNESS,"Thickness",
                p.data.slab_region.thickness_mm);
            add_info(panel,"Vertices: %zu",p.data.slab_region.vertex_count);
            return 1;
        case EDITOR_SELECTION_SLAB_EDGE_REBATE:
            panel->title="Edge rebate";
            add_field(panel,EDITOR_PROPERTY_SLAB_REBATE_START,"Start U",
                p.data.slab_edge_rebate.definition.start_offset_mm);
            add_field(panel,EDITOR_PROPERTY_SLAB_REBATE_END,"End U",
                p.data.slab_edge_rebate.definition.end_offset_mm);
            add_field(panel,EDITOR_PROPERTY_SLAB_REBATE_WIDTH,"Width",
                p.data.slab_edge_rebate.definition.width_mm);
            add_field(panel,EDITOR_PROPERTY_SLAB_REBATE_DEPTH,"Depth",
                p.data.slab_edge_rebate.definition.depth_mm);
            add_info(panel,"Length: %d mm",p.data.slab_edge_rebate.length_mm);
            return 1;
        case EDITOR_SELECTION_ROOF:
            panel->title="Roof";
            add_info(panel,"Portions: %zu",p.data.roof.portion_count);
            add_info(panel,"Compositions: %zu",p.data.roof.composition_count);
            add_info(panel,"Terminations: %zu",p.data.roof.termination_count);
            panel->note="Roof source editing is not surfaced yet";
            return 1;
        case EDITOR_SELECTION_ROOF_PORTION:
            panel->title="Roof portion";
            add_info(panel,"Type: %s",roof_generation_label(p.data.roof_portion.generation));
            add_info(panel,"Slope: %.3f",(double)p.data.roof_portion.slope_ppm/1000000.0);
            add_info(panel,"Reference Z: %d mm",p.data.roof_portion.reference_z_mm);
            add_info(panel,"Direction: %d, %d",p.data.roof_portion.direction.x,
                p.data.roof_portion.direction.y);
            add_info(panel,"Support vertices: %zu",p.data.roof_portion.support_vertex_count);
            panel->note="Roof source editing is not surfaced yet";
            return 1;
        default:
            return 0;
    }
}

Rect2 app_properties_panel_field_bounds(Rect2 bounds, size_t field_index)
{
    return (Rect2){
        .position={bounds.position.x+10.0,bounds.position.y+42.0+field_index*30.0},
        .width=bounds.width > 20.0 ? bounds.width-20.0 : 0.0,
        .height=24.0
    };
}

int app_properties_panel_hit(const AppPropertiesPanel *panel, Rect2 bounds,
    Vec2 point, EditorProperty *property)
{
    if (panel == NULL || property == NULL || !rect2_contains_point(bounds,point)) { return 0; }
    for (size_t i=0;i<panel->field_count;i++) {
        if (rect2_contains_point(app_properties_panel_field_bounds(bounds,i),point)) {
            *property=panel->fields[i].property;
            return 1;
        }
    }
    return 0;
}

void app_properties_panel_draw(Renderer2D *renderer, const AppPropertiesPanel *panel,
    Rect2 bounds, int has_active_property, EditorProperty active_property)
{
    if (renderer == NULL || panel == NULL || panel->title == NULL || bounds.width <= 0.0) { return; }
    Colour normal={225,225,225,255}, muted={155,155,155,255}, active={120,220,165,255};
    renderer2d_draw_screen_text(renderer,(Vec2){bounds.position.x+12,bounds.position.y+16},
        panel->title,normal);
    for (size_t i=0;i<panel->field_count;i++) {
        Rect2 row=app_properties_panel_field_bounds(bounds,i);
        int selected=has_active_property && panel->fields[i].property == active_property;
        renderer2d_fill_screen_rect(renderer,row,selected ? (Colour){55,75,65,255} : (Colour){55,55,55,255});
        char text[96];
        snprintf(text,sizeof text,"%s: %d mm",panel->fields[i].label,panel->fields[i].millimetres);
        renderer2d_draw_screen_text(renderer,(Vec2){row.position.x+6,row.position.y+8},
            text,selected ? active : normal);
    }
    double info_y=bounds.position.y+46.0+panel->field_count*30.0;
    for (size_t i=0;i<panel->info_count;i++) {
        renderer2d_draw_screen_text(renderer,(Vec2){bounds.position.x+12,info_y},
            panel->info_rows[i],muted);
        info_y += 16.0;
    }
    if (panel->note != NULL) {
        renderer2d_draw_screen_text(renderer,(Vec2){bounds.position.x+12,info_y+4.0},
            panel->note,muted);
    }
}
