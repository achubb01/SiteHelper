#ifndef APP_PROPERTIES_PANEL_H
#define APP_PROPERTIES_PANEL_H

#include "editor_properties.h"
#include "geometry.h"
#include "renderer2d.h"

#define APP_PROPERTY_PANEL_MAX_FIELDS 8
#define APP_PROPERTY_PANEL_MAX_INFO_ROWS 6

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
    size_t info_count;
    char info_rows[APP_PROPERTY_PANEL_MAX_INFO_ROWS][96];
} AppPropertiesPanel;

/* Contextual inspector. Editable integer-mm fields reuse the typed editor
 * property boundary; read-only rows expose useful workspace-specific summaries. */
int app_properties_panel_build(const SiteHelperEditor *editor,
    const SiteHelperProject *project, AppPropertiesPanel *panel);
Rect2 app_properties_panel_field_bounds(Rect2 bounds, size_t field_index);
int app_properties_panel_hit(const AppPropertiesPanel *panel, Rect2 bounds,
    Vec2 point, EditorProperty *property);
const char *app_property_label(EditorProperty property);
void app_properties_panel_draw(Renderer2D *renderer, const AppPropertiesPanel *panel,
    Rect2 bounds, int has_active_property, EditorProperty active_property);

#endif
