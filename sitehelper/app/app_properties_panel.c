#include "app_properties_panel.h"
#include <stdio.h>

static void add_field(AppPropertiesPanel *panel, EditorProperty property,
    const char *label, int millimetres)
{
    if (panel->field_count >= APP_PROPERTY_PANEL_MAX_FIELDS) { return; }
    panel->fields[panel->field_count++]=(AppPropertyField){property,label,millimetres};
}

const char *app_property_label(EditorProperty property)
{
    switch (property) {
        case EDITOR_PROPERTY_SLAB_THICKNESS: return "Thickness";
        case EDITOR_PROPERTY_SLAB_TOP_LEVEL: return "Top level";
        case EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL: return "Top level";
        case EDITOR_PROPERTY_SLAB_REGION_THICKNESS: return "Thickness";
        case EDITOR_PROPERTY_SLAB_REBATE_START: return "Start U";
        case EDITOR_PROPERTY_SLAB_REBATE_END: return "End U";
        case EDITOR_PROPERTY_SLAB_REBATE_WIDTH: return "Width";
        case EDITOR_PROPERTY_SLAB_REBATE_DEPTH: return "Depth";
        default: return "Property";
    }
}

int app_properties_panel_build(const SiteHelperEditor *editor,
    const SiteHelperProject *project, AppPropertiesPanel *panel)
{
    if (panel == NULL) { return 0; }
    *panel=(AppPropertiesPanel){0};
    EditorProperties p;
    if (!sitehelper_editor_inspect_properties(editor,project,&p)) { return 0; }
    switch (p.kind) {
        case EDITOR_SELECTION_SLAB:
            panel->title="Slab";
            add_field(panel,EDITOR_PROPERTY_SLAB_THICKNESS,"Thickness",p.data.slab.thickness_mm);
            add_field(panel,EDITOR_PROPERTY_SLAB_TOP_LEVEL,"Top level",p.data.slab.top_level_offset_mm);
            return 1;
        case EDITOR_SELECTION_SLAB_PENETRATION:
            panel->title="Slab void";
            panel->note="Polygon geometry editing deferred";
            return 1;
        case EDITOR_SELECTION_SLAB_REGION:
            panel->title="Slab region";
            add_field(panel,EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL,"Top level",
                p.data.slab_region.top_level_offset_mm);
            add_field(panel,EDITOR_PROPERTY_SLAB_REGION_THICKNESS,"Thickness",
                p.data.slab_region.thickness_mm);
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
    if (panel->field_count == 0 && panel->note != NULL) {
        renderer2d_draw_screen_text(renderer,(Vec2){bounds.position.x+12,bounds.position.y+46},
            panel->note,muted);
    }
}
