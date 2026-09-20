#include "app_view.h"
#include "plan_position_conversion.h"
#include "appstate.h"
#include "plan_dimension_geometry.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

void app_views_init(AppViews *views, Camera2D initial_camera)
{
    if (views == NULL) {
        return;
    }
    for (int view = 0; view < EDITOR_VIEW_COUNT; view++) {
        views->cameras[view] = initial_camera;
    }
}

int app_views_set_active(
    AppViews *views, SiteHelperEditor *editor, Renderer2D *renderer, EditorView view
)
{
    if (views == NULL || editor == NULL || renderer == NULL ||
        view < EDITOR_VIEW_PLAN || view >= EDITOR_VIEW_COUNT) {
        return 0;
    }
    views->cameras[editor->active_view] = renderer2d_get_camera(renderer);
    if (!sitehelper_editor_set_active_view(editor, view)) {
        return 0;
    }
    renderer2d_set_camera(renderer, views->cameras[view]);
    return 1;
}

static SlabPlanHit app_slab_selection(const EditorSelection *selection)
{
    SlabPlanHit hit = slab_plan_hit_none();
    if (selection == NULL || selection->scope != EDITOR_SELECTION_SCOPE_PLAN) {
        return hit;
    }
    hit.slab_id = selection->slab_id;
    hit.feature_index = selection->slab_feature_index;
    switch (selection->kind) {
        case EDITOR_SELECTION_SLAB: hit.kind = SLAB_PLAN_HIT_SLAB; break;
        case EDITOR_SELECTION_SLAB_PENETRATION:
            hit.kind = SLAB_PLAN_HIT_PENETRATION; break;
        case EDITOR_SELECTION_SLAB_REGION: hit.kind = SLAB_PLAN_HIT_REGION; break;
        case EDITOR_SELECTION_SLAB_EDGE_REBATE:
            hit.kind = SLAB_PLAN_HIT_EDGE_REBATE; break;
        default: return slab_plan_hit_none();
    }
    return hit;
}

void app_render_slabs(
    Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const SlabPlanRenderStyle *style
)
{
    if (renderer == NULL || project == NULL || editor == NULL || style == NULL ||
        editor->active_view != EDITOR_VIEW_PLAN) {
        return;
    }
    const Storey *storey = sitehelper_project_find_storey_by_id_const(
        project, editor->current_storey_id);
    if (storey == NULL) { return; }
    SlabPlanHit selection = app_slab_selection(&editor->selection);
    slab_plan_render_storey(renderer, storey, &selection, style);
}

void app_render_walls(
    Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const WallRenderStyle *style
)
{
    if (renderer == NULL || project == NULL || editor == NULL || style == NULL) {
        return;
    }
    if (editor->active_view == EDITOR_VIEW_WALL_ELEVATION) {
        const Wall *wall = app_current_wall_const(project, editor);
        if (wall != NULL) {
            const WallSelection *selection = editor_selection_get_wall_member(
                &editor->selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, wall->id
            );
            wall_elevation_render(renderer, wall,
                wall_selection_resolve(selection, wall), style);
        }
        return;
    }
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    if (storey == NULL) { return; }
    for (size_t i = 0; i < storey->structure.wall_count; i++) {
        const Wall *wall = &storey->structure.walls[i];
        wall_plan_render(renderer, wall, wall->id == editor->current_wall_id
            ? style->selected_colour : style->timber_colour);
    }
}

static void draw_note_text(Renderer2D *renderer, Vec2 origin, const char *text, Colour colour)
{
    if (text == NULL) { return; }
    char line[256];
    size_t used=0;
    double y=origin.y;
    for (const char *p=text;;p++) {
        int end=*p == '\0', newline=*p == '\n';
        if (!end && !newline && used + 1 < sizeof line) { line[used++]=*p; continue; }
        line[used]='\0';
        if (used != 0) {
            renderer2d_draw_screen_text(renderer,(Vec2){origin.x,y},line,colour);
        }
        if (used != 0 || newline) { y += 10.0; }
        used=0;
        if (end) { break; }
        if (!newline) { p--; }
    }
}


void app_render_plan_dimensions(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor)
{
    if (renderer == NULL || project == NULL || editor == NULL ||
        editor->active_view != EDITOR_VIEW_PLAN || editor->current_storey_id == DOMAIN_ID_INVALID) {
        return;
    }
    Camera2D camera=renderer2d_get_camera(renderer);
    Viewport2D viewport=renderer2d_get_viewport(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) return;
    for (size_t i=0;i<project->document.dimension_count;i++) {
        const DocumentPlanDimension *dimension=&project->document.dimensions[i];
        if (dimension->storey_id != editor->current_storey_id) continue;
        PlanPosition a,b; int distance_mm;
        if (!sitehelper_project_resolve_plan_dimension(project,dimension->id,&a,&b,&distance_mm)) continue;
        DocumentPlanDimensionGeometry geometry;
        if (!document_plan_dimension_geometry(a,b,dimension->offset_mm,&geometry)) continue;
        int selected=editor_selection_matches_document(&editor->selection,
            DOCUMENT_OBJECT_DIMENSION,dimension->id);
        Colour colour=selected ? (Colour){255,220,40,255} : (Colour){210,210,210,255};
        renderer2d_draw_line(renderer,(Vec2){geometry.source_first.x,geometry.source_first.y},
            (Vec2){geometry.line_first.x,geometry.line_first.y},colour);
        renderer2d_draw_line(renderer,(Vec2){geometry.source_second.x,geometry.source_second.y},
            (Vec2){geometry.line_second.x,geometry.line_second.y},colour);
        renderer2d_draw_line(renderer,(Vec2){geometry.line_first.x,geometry.line_first.y},
            (Vec2){geometry.line_second.x,geometry.line_second.y},colour);

        /* Fixed-pixel diagonal terminal ticks. */
        const double dx=geometry.line_second.x-geometry.line_first.x;
        const double dy=geometry.line_second.y-geometry.line_first.y;
        const double length=hypot(dx,dy);
        if (length > 0.0 && isfinite(length)) {
            const double ux=dx/length, uy=dy/length;
            const double nx=-uy, ny=ux;
            const double half=4.0/camera.scale;
            const double tx=(ux+nx)*half, ty=(uy+ny)*half;
            PlanPoint ends[2]={geometry.line_first,geometry.line_second};
            for (size_t e=0;e<2;e++) {
                renderer2d_draw_line(renderer,
                    (Vec2){ends[e].x-tx,ends[e].y-ty},
                    (Vec2){ends[e].x+tx,ends[e].y+ty},colour);
            }
        }
        Vec2 label=camera_world_to_screen(&camera,viewport,
            (Vec2){geometry.midpoint.x,geometry.midpoint.y});
        if (isfinite(label.x)&&isfinite(label.y)) {
            char text[64];
            snprintf(text,sizeof text,"%d mm",distance_mm);
            label.x += 4.0; label.y -= 12.0;
            renderer2d_draw_screen_text(renderer,label,text,colour);
        }
    }
}


void app_render_plan_dimension_preview(Renderer2D *renderer, const SiteHelperEditor *editor)
{
    if (renderer == NULL || editor == NULL || editor->active_view != EDITOR_VIEW_PLAN) { return; }
    DocumentPlanDimensionGeometry geometry;
    int distance_mm,ready;
    if (!sitehelper_editor_get_plan_dimension_preview(editor,&geometry,&distance_mm,&ready)) { return; }
    Colour colour=ready ? (Colour){100,220,150,255} : (Colour){100,180,255,255};
    renderer2d_draw_line(renderer,(Vec2){geometry.source_first.x,geometry.source_first.y},
        (Vec2){geometry.line_first.x,geometry.line_first.y},colour);
    renderer2d_draw_line(renderer,(Vec2){geometry.source_second.x,geometry.source_second.y},
        (Vec2){geometry.line_second.x,geometry.line_second.y},colour);
    renderer2d_draw_line(renderer,(Vec2){geometry.line_first.x,geometry.line_first.y},
        (Vec2){geometry.line_second.x,geometry.line_second.y},colour);
    Camera2D camera=renderer2d_get_camera(renderer);
    Viewport2D viewport=renderer2d_get_viewport(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) { return; }
    const double dx=geometry.line_second.x-geometry.line_first.x;
    const double dy=geometry.line_second.y-geometry.line_first.y;
    const double length=hypot(dx,dy);
    if (length > 0.0 && isfinite(length)) {
        const double ux=dx/length,uy=dy/length,nx=-uy,ny=ux;
        const double half=4.0/camera.scale;
        const double tx=(ux+nx)*half,ty=(uy+ny)*half;
        PlanPoint ends[2]={geometry.line_first,geometry.line_second};
        for (size_t e=0;e<2;e++) {
            renderer2d_draw_line(renderer,(Vec2){ends[e].x-tx,ends[e].y-ty},
                (Vec2){ends[e].x+tx,ends[e].y+ty},colour);
        }
    }
    Vec2 label=camera_world_to_screen(&camera,viewport,
        (Vec2){geometry.midpoint.x,geometry.midpoint.y});
    if (isfinite(label.x)&&isfinite(label.y)) {
        char text[64]; snprintf(text,sizeof text,"%d mm",distance_mm);
        label.x+=4.0; label.y-=12.0;
        renderer2d_draw_screen_text(renderer,label,text,colour);
    }
}

static void draw_view_direction_marker(Renderer2D *renderer, PlanPosition anchor,
    double dx, double dy, Colour colour)
{
    Camera2D camera=renderer2d_get_camera(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) { return; }
    double length=hypot(dx,dy);
    if (!(length > 0.0) || !isfinite(length)) { return; }
    double ux=dx/length,uy=dy/length,nx=-uy,ny=ux;
    double radius=5.0/camera.scale;
    double shaft=20.0/camera.scale;
    double head=7.0/camera.scale;
    double half=4.0/camera.scale;
    double x=anchor.x,y=anchor.y;
    /* Diamond at the authored anchor, then an oriented arrow shaft/head. */
    renderer2d_draw_line(renderer,(Vec2){x+nx*radius,y+ny*radius},
        (Vec2){x+ux*radius,y+uy*radius},colour);
    renderer2d_draw_line(renderer,(Vec2){x+ux*radius,y+uy*radius},
        (Vec2){x-nx*radius,y-ny*radius},colour);
    renderer2d_draw_line(renderer,(Vec2){x-nx*radius,y-ny*radius},
        (Vec2){x-ux*radius,y-uy*radius},colour);
    renderer2d_draw_line(renderer,(Vec2){x-ux*radius,y-uy*radius},
        (Vec2){x+nx*radius,y+ny*radius},colour);
    Vec2 tip={x+ux*shaft,y+uy*shaft};
    Vec2 base={tip.x-ux*head,tip.y-uy*head};
    renderer2d_draw_line(renderer,(Vec2){x+ux*radius,y+uy*radius},tip,colour);
    renderer2d_draw_line(renderer,tip,(Vec2){base.x+nx*half,base.y+ny*half},colour);
    renderer2d_draw_line(renderer,tip,(Vec2){base.x-nx*half,base.y-ny*half},colour);
}

void app_render_plan_symbols(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor)
{
    if (renderer == NULL || project == NULL || editor == NULL ||
        editor->active_view != EDITOR_VIEW_PLAN || editor->current_storey_id == DOMAIN_ID_INVALID) {
        return;
    }
    Camera2D camera=renderer2d_get_camera(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) { return; }
    const double inner=4.0/camera.scale;
    const double outer=7.0/camera.scale;
    for (size_t i=0;i<project->document.symbol_count;i++) {
        const DocumentPlanSymbol *symbol=&project->document.symbols[i];
        if (symbol->storey_id != editor->current_storey_id) { continue; }
        int selected=editor_selection_matches_document(&editor->selection,
            DOCUMENT_OBJECT_SYMBOL,symbol->id);
        Colour colour=selected ? (Colour){255,220,40,255} : (Colour){120,210,255,255};
        double x=symbol->anchor.x,y=symbol->anchor.y;
        if (symbol->kind == DOCUMENT_PLAN_SYMBOL_POINT_MARKER) {
            renderer2d_draw_line(renderer,(Vec2){x-inner,y-inner},(Vec2){x+inner,y-inner},colour);
            renderer2d_draw_line(renderer,(Vec2){x+inner,y-inner},(Vec2){x+inner,y+inner},colour);
            renderer2d_draw_line(renderer,(Vec2){x+inner,y+inner},(Vec2){x-inner,y+inner},colour);
            renderer2d_draw_line(renderer,(Vec2){x-inner,y+inner},(Vec2){x-inner,y-inner},colour);
            renderer2d_draw_line(renderer,(Vec2){x-outer,y},(Vec2){x+outer,y},colour);
            renderer2d_draw_line(renderer,(Vec2){x,y-outer},(Vec2){x,y+outer},colour);
        } else if (symbol->kind == DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION) {
            draw_view_direction_marker(renderer,symbol->anchor,
                symbol->direction.dx,symbol->direction.dy,colour);
        }
    }

    PlanPosition anchor; PlanPoint direction_point; int ready;
    if (sitehelper_editor_get_view_direction_preview(editor,&anchor,&direction_point,&ready)) {
        Colour colour=ready ? (Colour){120,210,255,255} : (Colour){140,140,140,255};
        draw_view_direction_marker(renderer,anchor,
            direction_point.x-anchor.x,direction_point.y-anchor.y,colour);
    }
}

static void draw_callout_arrow(Renderer2D *renderer, PlanPosition target,
    PlanPosition label, Colour colour)
{
    Camera2D camera=renderer2d_get_camera(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) { return; }
    double dx=(double)label.x-target.x,dy=(double)label.y-target.y;
    double length=hypot(dx,dy);
    if (!(length > 0.0) || !isfinite(length)) { return; }
    double ux=dx/length,uy=dy/length;
    double nx=-uy,ny=ux;
    double size=7.0/camera.scale;
    Vec2 t={(double)target.x,(double)target.y};
    Vec2 a={t.x+ux*size+nx*size*0.55,t.y+uy*size+ny*size*0.55};
    Vec2 b={t.x+ux*size-nx*size*0.55,t.y+uy*size-ny*size*0.55};
    renderer2d_draw_line(renderer,t,a,colour);
    renderer2d_draw_line(renderer,t,b,colour);
}

void app_render_plan_callouts(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor)
{
    if (renderer == NULL || project == NULL || editor == NULL ||
        editor->active_view != EDITOR_VIEW_PLAN || editor->current_storey_id == DOMAIN_ID_INVALID) {
        return;
    }
    Camera2D camera=renderer2d_get_camera(renderer);
    Viewport2D viewport=renderer2d_get_viewport(renderer);
    for (size_t i=0;i<project->document.callout_count;i++) {
        const DocumentPlanCallout *callout=&project->document.callouts[i];
        if (callout->storey_id != editor->current_storey_id) { continue; }
        int selected=editor_selection_matches_document(&editor->selection,
            DOCUMENT_OBJECT_CALLOUT,callout->id);
        Colour colour=selected ? (Colour){255,220,40,255} : (Colour){235,190,100,255};
        renderer2d_draw_line(renderer,(Vec2){callout->target.x,callout->target.y},
            (Vec2){callout->label_anchor.x,callout->label_anchor.y},colour);
        draw_callout_arrow(renderer,callout->target,callout->label_anchor,colour);
        Vec2 screen=camera_world_to_screen(&camera,viewport,
            (Vec2){callout->label_anchor.x,callout->label_anchor.y});
        if (isfinite(screen.x) && isfinite(screen.y)) {
            screen.x+=6.0; screen.y-=12.0;
            renderer2d_draw_screen_text(renderer,screen,callout->text,colour);
        }
    }
}

void app_render_plan_callout_preview(Renderer2D *renderer, const SiteHelperEditor *editor)
{
    if (renderer == NULL || editor == NULL) { return; }
    PlanPosition target;
    PlanPoint label;
    int ready;
    if (!sitehelper_editor_get_plan_callout_preview(editor,&target,&label,&ready)) { return; }
    Colour colour=ready ? (Colour){120,220,150,255} : (Colour){130,190,255,255};
    renderer2d_draw_line(renderer,(Vec2){target.x,target.y},(Vec2){label.x,label.y},colour);
    PlanPosition label_position;
    if (plan_position_from_point(label,&label_position) &&
        !(label_position.x == target.x && label_position.y == target.y)) {
        draw_callout_arrow(renderer,target,label_position,colour);
    }
}

void app_render_plan_notes(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor)
{
    if (renderer == NULL || project == NULL || editor == NULL ||
        editor->active_view != EDITOR_VIEW_PLAN || editor->current_storey_id == DOMAIN_ID_INVALID) {
        return;
    }
    Camera2D camera=renderer2d_get_camera(renderer);
    Viewport2D viewport=renderer2d_get_viewport(renderer);
    for (size_t i=0;i<project->document.annotation_count;i++) {
        const DocumentAnnotation *note=&project->document.annotations[i];
        if (note->kind != DOCUMENT_ANNOTATION_NOTE ||
            note->anchor.storey_id != editor->current_storey_id) { continue; }
        Vec2 screen=camera_world_to_screen(&camera,viewport,
            (Vec2){note->anchor.position.x,note->anchor.position.y});
        if (!isfinite(screen.x)||!isfinite(screen.y)) { continue; }
        int selected=editor_selection_matches_document(&editor->selection,
            DOCUMENT_OBJECT_NOTE,note->id);
        Colour colour=selected ? (Colour){255,220,40,255} : (Colour){235,235,180,255};
        double size=selected ? 8.0 : 6.0;
        renderer2d_fill_screen_rect(renderer,(Rect2){
            .position={screen.x-size*.5,screen.y-size*.5},.width=size,.height=size},colour);
        draw_note_text(renderer,(Vec2){screen.x+8.0,screen.y-4.0},note->text,colour);
    }
}


static double revision_cloud_signed_area(const PlanPosition *vertices, size_t count)
{
    long double twice_area=0.0L;
    if (vertices == NULL || count < 3) { return 0.0; }
    for (size_t i=0;i<count;i++) {
        const PlanPosition a=vertices[i], b=vertices[(i+1)%count];
        twice_area += (long double)a.x*b.y - (long double)b.x*a.y;
    }
    return (double)(twice_area*0.5L);
}

static void draw_revision_cloud_edge(Renderer2D *renderer, Vec2 a, Vec2 b,
    double outward_sign, Colour colour)
{
    Camera2D camera=renderer2d_get_camera(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) { return; }
    double dx=b.x-a.x,dy=b.y-a.y,length=hypot(dx,dy);
    if (!(length > 0.0) || !isfinite(length)) { return; }
    double ux=dx/length,uy=dy/length;
    /* Right normal; sign flips it for clockwise boundaries. */
    double nx=uy*outward_sign,ny=-ux*outward_sign;
    double desired=18.0/camera.scale;
    size_t lobes=(size_t)ceil(length/desired);
    if (lobes < 1) { lobes=1; }
    double amplitude=4.0/camera.scale;
    Vec2 previous=a;
    const size_t samples_per_lobe=4;
    const size_t samples=lobes*samples_per_lobe;
    for (size_t i=1;i<=samples;i++) {
        double t=(double)i/(double)samples;
        double phase=fmod(t*(double)lobes,1.0);
        if (i == samples) { phase=1.0; }
        double offset=amplitude*sin(3.14159265358979323846*phase);
        Vec2 point={a.x+dx*t+nx*offset,a.y+dy*t+ny*offset};
        renderer2d_draw_line(renderer,previous,point,colour);
        previous=point;
    }
}

static void draw_revision_cloud_boundary(Renderer2D *renderer,
    const PlanPosition *vertices, size_t count, Colour colour)
{
    if (renderer == NULL || vertices == NULL || count < 2) { return; }
    double sign=revision_cloud_signed_area(vertices,count) >= 0.0 ? 1.0 : -1.0;
    for (size_t i=0;i<count;i++) {
        PlanPosition a=vertices[i],b=vertices[(i+1)%count];
        draw_revision_cloud_edge(renderer,(Vec2){a.x,a.y},(Vec2){b.x,b.y},sign,colour);
    }
}

void app_render_plan_revision_clouds(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor)
{
    if (renderer == NULL || project == NULL || editor == NULL ||
        editor->active_view != EDITOR_VIEW_PLAN || editor->current_storey_id == DOMAIN_ID_INVALID) {
        return;
    }
    for (size_t i=0;i<project->document.revision_cloud_count;i++) {
        const DocumentPlanRevisionCloud *cloud=&project->document.revision_clouds[i];
        if (cloud->storey_id != editor->current_storey_id ||
            !document_plan_revision_cloud_is_locally_valid(cloud)) { continue; }
        int selected=editor_selection_matches_document(&editor->selection,
            DOCUMENT_OBJECT_REVISION_CLOUD,cloud->id);
        Colour colour=selected ? (Colour){255,220,40,255} : (Colour){235,95,180,255};
        draw_revision_cloud_boundary(renderer,cloud->vertices,cloud->vertex_count,colour);
    }
}

void app_render_plan_revision_cloud_preview(Renderer2D *renderer,
    const SiteHelperEditor *editor)
{
    if (renderer == NULL || editor == NULL || editor->active_view != EDITOR_VIEW_PLAN) { return; }
    const PlanPosition *vertices=NULL; size_t count=0; PlanPoint preview={0}; int has_preview=0;
    if (!sitehelper_editor_get_plan_revision_cloud_preview(editor,&vertices,&count,
            &preview,&has_preview)) { return; }
    Colour colour=(Colour){255,145,210,255};
    for (size_t i=1;i<count;i++) {
        renderer2d_draw_line(renderer,(Vec2){vertices[i-1].x,vertices[i-1].y},
            (Vec2){vertices[i].x,vertices[i].y},colour);
    }
    if (has_preview && count != 0) {
        renderer2d_draw_line(renderer,(Vec2){vertices[count-1].x,vertices[count-1].y},
            (Vec2){preview.x,preview.y},colour);
    }
    if (count >= 3) {
        Vec2 last=has_preview ? (Vec2){preview.x,preview.y} :
            (Vec2){vertices[count-1].x,vertices[count-1].y};
        renderer2d_draw_line(renderer,last,(Vec2){vertices[0].x,vertices[0].y},
            (Colour){180,100,160,180});
    }
}

void app_render_measurement(Renderer2D *renderer, const SiteHelperEditor *editor)
{
    PlanMeasurementQuery query;
    if (renderer == NULL || !sitehelper_editor_get_measurement(editor, &query)) { return; }
    Vec2 a = {query.start.x, query.start.y}, b = {query.end.x, query.end.y};
    /* Half each coordinate before adding to avoid overflowing a finite midpoint. */
    Vec2 midpoint = {a.x * 0.5 + b.x * 0.5, a.y * 0.5 + b.y * 0.5};
    Camera2D camera = renderer2d_get_camera(renderer);
    Viewport2D viewport = renderer2d_get_viewport(renderer);
    Vec2 label = camera_world_to_screen(&camera, viewport, midpoint);
    Vec2 screen_a = camera_world_to_screen(&camera, viewport, a);
    Vec2 screen_b = camera_world_to_screen(&camera, viewport, b);
    if (!isfinite(label.x) || !isfinite(label.y) ||
        !isfinite(screen_a.x) || !isfinite(screen_a.y) ||
        !isfinite(screen_b.x) || !isfinite(screen_b.y)) { return; }
    Colour colour = query.completed ? (Colour){255, 220, 90, 255} : (Colour){110, 210, 255, 255};
    renderer2d_draw_line(renderer, a, b, colour);
    /* Enough room for any finite double's fixed integer digits plus suffix.
     * Explicit round gives nearest mm, half upward, without narrowing to int. */
    char text[DBL_MAX_10_EXP + 32];
    snprintf(text, sizeof text, "%.0f mm", round(query.distance_mm));
    label.y -= 12.0; /* Pixels above the physical midpoint. */
    renderer2d_draw_screen_text(renderer, label, text, colour);
}
