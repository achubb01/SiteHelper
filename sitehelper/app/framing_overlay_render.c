#include "presentation_overlay_render.h"

#include <math.h>

#include "presentation_interaction_style.h"

static Colour framing_overlay_selected_colour(const AppInteractionStyle *style)
{
    return style != NULL ? style->selected_colour :
        (Colour){255, 210, 70, 255};
}

static Colour framing_overlay_hovered_colour(const AppInteractionStyle *style)
{
    return style != NULL && style->hovered_colour.a != 0 ? style->hovered_colour :
        (Colour){90, 220, 255, 255};
}

static void draw_grip(Renderer2D *renderer, Vec2 centre, double size,
    Colour colour)
{
    renderer2d_fill_rect(renderer, (Rect2){
        .position={centre.x-size*0.5,centre.y-size*0.5},
        .width=size,.height=size
    }, colour);
}

static void draw_selected_opening_edit_overlay(
    Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const AppInteractionStyle *interaction_style)
{
    if (renderer == NULL || project == NULL || editor == NULL ||
        editor->active_workspace != EDITOR_WORKSPACE_FRAMING ||
        editor->active_view != EDITOR_VIEW_WALL_ELEVATION ||
        editor->active_tool != EDITOR_TOOL_SELECT ||
        editor->selection.kind != EDITOR_SELECTION_OPENING ||
        editor->selection.scope != EDITOR_SELECTION_SCOPE_WALL_ELEVATION) {
        return;
    }

    const Storey *storey=sitehelper_project_find_storey_by_id_const(
        project,editor->current_storey_id);
    const Wall *wall=storey != NULL ? build_find_wall_by_id_const(
        &storey->structure,editor->selection.wall_id) : NULL;
    const Opening *opening=wall != NULL ? wall_find_opening_by_id_const(
        wall,editor->selection.opening_id) : NULL;
    BuildSettings settings;
    if (opening == NULL || !sitehelper_project_resolve_storey_build_settings(
            project,editor->current_storey_id,&settings)) {
        return;
    }

    WallOpeningEdit edit={0};
    int has_edit=sitehelper_editor_get_opening_edit(editor,wall->id,opening->id,&edit);
    const Opening *presented=(has_edit && edit.active) ? &edit.candidate : opening;
    WallOpeningFrameGeometry frame;
    if (!wall_opening_frame_geometry(presented,&settings,&frame)) { return; }

    Colour selected=framing_overlay_selected_colour(interaction_style);
    Colour hovered=framing_overlay_hovered_colour(interaction_style);
    Colour candidate=selected;
    if (has_edit && edit.active) {
        candidate=edit.validation.code == WALL_OPENING_VALID ? hovered :
            (Colour){235,80,80,255};
        renderer2d_draw_rect(renderer,(Rect2){
            .position={(double)frame.left_u,(double)frame.bottom_z},
            .width=(double)frame.width,.height=(double)frame.height
        },candidate);
    }

    Camera2D camera=renderer2d_get_camera(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) { return; }
    double size=9.0/camera.scale;
    double left=(double)frame.left_u, right=(double)frame.right_u;
    double bottom=(double)frame.bottom_z, top=(double)frame.top_z;
    double centre_u=(left+right)*0.5, centre_z=(bottom+top)*0.5;
    struct Grip { WallOpeningEditHandle handle; Vec2 centre; } grips[] = {
        {WALL_OPENING_EDIT_HANDLE_LEFT,{left,centre_z}},
        {WALL_OPENING_EDIT_HANDLE_RIGHT,{right,centre_z}},
        {WALL_OPENING_EDIT_HANDLE_BOTTOM,{centre_u,bottom}},
        {WALL_OPENING_EDIT_HANDLE_TOP,{centre_u,top}},
        {WALL_OPENING_EDIT_HANDLE_MOVE,{centre_u,centre_z}}
    };
    WallOpeningEditHandle emphasized=WALL_OPENING_EDIT_HANDLE_NONE;
    if (has_edit) {
        emphasized=edit.active ? edit.active_handle : edit.hovered_handle;
    }
    for (size_t i=0;i<sizeof grips/sizeof grips[0];i++) {
        Colour colour=grips[i].handle == emphasized ? hovered : selected;
        if (has_edit && edit.active && edit.validation.code != WALL_OPENING_VALID) {
            colour=(Colour){235,80,80,255};
        }
        draw_grip(renderer,grips[i].centre,size,colour);
    }
}

void app_render_framing_overlay(
    Renderer2D *renderer,
    const SiteHelperProject *project,
    const SiteHelperEditor *editor,
    const AppInteractionStyle *interaction_style
)
{
    if (renderer == NULL || editor == NULL) {
        return;
    }

    Rect2 preview_rect;
    if (sitehelper_editor_get_opening_preview_rect(editor, &preview_rect)) {
        renderer2d_draw_rect(
            renderer,
            preview_rect,
            (Colour){100, 180, 255, 255}
        );
    }

    WallPlanSegment preview_segment;
    if (sitehelper_editor_get_wall_preview_segment(editor, &preview_segment)) {
        renderer2d_draw_line(
            renderer,
            (Vec2){preview_segment.start.x, preview_segment.start.y},
            (Vec2){preview_segment.end.x, preview_segment.end.y},
            (Colour){100, 220, 150, 255}
        );
    }

    draw_selected_opening_edit_overlay(
        renderer,project,editor,interaction_style);
}
