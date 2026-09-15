#ifndef APP_PROPERTIES_PANEL_H
#define APP_PROPERTIES_PANEL_H

#include "editor_properties.h"
#include "geometry.h"
#include "renderer2d.h"

#define APP_PROPERTY_PANEL_MAX_FIELDS 4

typedef struct {
    EditorProperty property;
    const char *label;
    int millimetres;
} AppPropertyField;

typedef struct {
    const char *title;
    const char *note;
    size_t field_count;
    AppPropertyField fields[APP_PROPERTY_PANEL_MAX_FIELDS];
} AppPropertiesPanel;

/* Builds only the slab-family inspector introduced by Priority 25E4. Wall and
 * opening typed property intents remain available but are not yet surfaced by
 * this minimal panel. */
int app_properties_panel_build(const SiteHelperEditor *editor,
    const SiteHelperProject *project, AppPropertiesPanel *panel);
Rect2 app_properties_panel_field_bounds(Rect2 bounds, size_t field_index);
int app_properties_panel_hit(const AppPropertiesPanel *panel, Rect2 bounds,
    Vec2 point, EditorProperty *property);
const char *app_property_label(EditorProperty property);
void app_properties_panel_draw(Renderer2D *renderer, const AppPropertiesPanel *panel,
    Rect2 bounds, int has_active_property, EditorProperty active_property);

#endif
