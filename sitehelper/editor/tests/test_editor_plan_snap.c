#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "sitehelper_editor.h"
#include "command_history.h"
#include "sitehelper_persistence.h"
#include "wall.h"
#include "wall_snap.h"

static SnapResult result(const SiteHelperEditor *editor)
{
    const SnapResult *snap = sitehelper_editor_get_snap_result(editor); assert(snap);
    return *snap;
}
static PlanMeasurementQuery query(const SiteHelperEditor *editor)
{
    PlanMeasurementQuery measurement;
    assert(sitehelper_editor_get_measurement(editor, &measurement)); return measurement;
}
static EditorAction click(SiteHelperEditor *editor, const SiteHelperProject *project, Vec2 position)
{
    EditorAction action;
    assert(sitehelper_editor_primary_action_in_project(editor, project, position, &action)); return action;
}
static void save(const SiteHelperProject *project, char *text, size_t capacity)
{
    const char *path = "plan_snap_project.tmp";
    assert(sitehelper_project_save_file(project, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file = fopen(path, "rb"); assert(file);
    size_t count = fread(text, 1, capacity - 1, file);
    assert(count < capacity - 1 && !ferror(file)); text[count] = 0;
    assert(fclose(file) == 0 && remove(path) == 0);
}

static void test_project_events_tools_and_ownership(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    DomainId first = sitehelper_project_add_storey(&project, 0);
    DomainId other = sitehelper_project_add_storey(&project, 2700);
    DomainId horizontal = sitehelper_project_add_wall(&project, first, (WallPlanSegment){{100,200},{1100,200}});
    assert(horizontal);
    assert(sitehelper_project_add_wall(&project, first, (WallPlanSegment){{500,-500},{500,1000}}));
    assert(sitehelper_project_add_wall(&project, other, (WallPlanSegment){{1500,1500},{2500,1500}}));
    assert(sitehelper_project_add_room_separator(&project, first, (PlanSegment){{700,0},{700,500}}));
    assert(sitehelper_editor_set_current_storey(&editor, &project, first));
    DomainId next_id = project.domain_ids.next;
    char before[8192], after[8192]; save(&project, before, sizeof before);

    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){1102,205});
    assert(result(&editor).type == SNAP_ENDPOINT && result(&editor).position.x == 1100);
    assert(click(&editor, &project, (Vec2){98,199}).kind == EDITOR_ACTION_NONE);
    WallPlanSegment preview;
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){650,202});
    assert(result(&editor).type == SNAP_WALL_CENTRELINE);
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(preview.start.x == 100 && preview.start.y == 200 && preview.end.x == 650 && preview.end.y == 200);
    EditorAction action = click(&editor, &project, (Vec2){980,202});
    assert(action.kind == EDITOR_ACTION_COMMAND && action.command.type == SITEHELPER_COMMAND_ADD_WALL);
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(preview.end.x == 980 && preview.end.y == 200); /* Actual click, not motion. */
    sitehelper_editor_cancel_tool_interaction(&editor);
    assert(click(&editor, &project, (Vec2){650,202}).kind == EDITOR_ACTION_NONE);
    assert(click(&editor, &project, (Vec2){1102,205}).kind == EDITOR_ACTION_COMMAND);
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(preview.start.x == 650 && preview.start.y == 200 && preview.end.x == 1100 && preview.end.y == 200);
    sitehelper_editor_cancel_tool_interaction(&editor);
    assert(click(&editor, &project, (Vec2){500,200}).kind == EDITOR_ACTION_NONE);
#ifdef SITEHELPER_TEST_TOPOLOGY
    assert(result(&editor).type == SNAP_INTERSECTION);
#else
    assert(result(&editor).type == SNAP_WALL_CENTRELINE);
#endif
    assert(click(&editor, &project, (Vec2){1102,205}).kind == EDITOR_ACTION_COMMAND);
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(preview.start.x == 500 && preview.start.y == 200);
    sitehelper_editor_cancel_tool_interaction(&editor);
    assert(click(&editor, &project, (Vec2){1102,205}).kind == EDITOR_ACTION_NONE);
    assert(click(&editor, &project, (Vec2){500,200}).kind == EDITOR_ACTION_COMMAND);
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(preview.end.x == 500 && preview.end.y == 200);

    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_MEASURE));
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){1102,205});
    assert(click(&editor, &project, (Vec2){98,199}).kind == EDITOR_ACTION_NONE);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){650,202});
    assert(query(&editor).start.x == 100 && query(&editor).end.x == 650 && query(&editor).distance_mm == 550);
    assert(click(&editor, &project, (Vec2){500,200}).kind == EDITOR_ACTION_NONE);
    assert(query(&editor).completed && query(&editor).distance_mm == 400);
    /* Snap refresh reflects changed project geometry even without motion. */
    Wall *wall = sitehelper_project_find_wall_by_id(&project, horizontal); assert(wall);
    WallPlanSegment original = wall->definition.segment;
    wall->definition.segment = (WallPlanSegment){{200,300},{1200,300}};
    assert(click(&editor, &project, (Vec2){198,299}).kind == EDITOR_ACTION_NONE);
    assert(query(&editor).start.x == 200 && query(&editor).start.y == 300);
    wall->definition.segment = original;
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){1500,1500});
    assert(result(&editor).type == SNAP_GRID); /* Other Storey's endpoint is invisible. */
    assert(sitehelper_editor_set_current_storey(&editor, &project, other));
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){1500,1500});
    assert(result(&editor).type == SNAP_ENDPOINT);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){100,200});
    assert(result(&editor).type == SNAP_GRID);
    sitehelper_editor_update_snap_in_project(&editor, NULL, (Vec2){1,2});
    assert(!sitehelper_editor_has_snap(&editor));
    save(&project, after, sizeof after);
    assert(strcmp(before, after) == 0 && project.domain_ids.next == next_id);
    assert(history.count == 0 && history.cursor == 0);
    sitehelper_command_history_destroy(&history); sitehelper_project_destroy(&project);
}

static void test_fractional_measurement_and_elevation(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    DomainId wall_id = sitehelper_project_add_wall(&project, storey, (WallPlanSegment){{0,0},{1001,1001}});
    assert(wall_id);
    assert(sitehelper_project_add_wall(&project, storey, (WallPlanSegment){{0,1001},{1001,0}}));
    assert(sitehelper_editor_set_current_storey(&editor, &project, storey));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_MEASURE));
    /* Isolate the intersection target; centreline remains nearer for general
     * off-junction pointer positions under the generic nearest-distance rule. */
    editor.snap.settings.wall_centreline_enabled = 0;
#ifdef SITEHELPER_TEST_TOPOLOGY
    assert(click(&editor, &project, (Vec2){499,502}).kind == EDITOR_ACTION_NONE);
    assert(query(&editor).start.x == 500.5 && query(&editor).start.y == 500.5);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){0,0});
    assert(query(&editor).end.x == 0);
    assert(click(&editor, &project, (Vec2){0,0}).kind == EDITOR_ACTION_NONE);
    assert(fabs(query(&editor).distance_mm - hypot(500.5,500.5)) < 1e-9);
    assert(click(&editor, &project, (Vec2){0,0}).kind == EDITOR_ACTION_NONE);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){499,502});
    assert(query(&editor).end.x == 500.5 && !query(&editor).completed);
    assert(click(&editor, &project, (Vec2){501,499}).kind == EDITOR_ACTION_NONE);
    assert(query(&editor).end.x == 500.5 && query(&editor).end.y == 500.5 && query(&editor).completed);
#endif
    Wall *wall = sitehelper_project_find_wall_by_id(&project, wall_id);
    assert(wall_generate(wall, &project.settings));
    editor.current_wall_id = wall_id;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    SnapCandidate candidates[256];
    size_t count = wall_collect_snap_candidates(wall, candidates, 256); assert(count);
    Vec2 pointer = candidates[0].position;
    SnapResult expected = editor_snap(pointer, candidates, count, &editor.snap.settings);
    sitehelper_editor_pointer_move_in_project(&editor, &project, pointer);
    SnapResult actual = result(&editor);
    assert(actual.type == expected.type && actual.position.x == expected.position.x && actual.position.y == expected.position.y);
    editor.snap.settings.wall_centreline_enabled = 1;
    sitehelper_editor_update_snap_in_project(&editor, &project, pointer);
    actual = result(&editor);
    assert(actual.type == expected.type && actual.position.x == expected.position.x && actual.position.y == expected.position.y);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_project_events_tools_and_ownership(); test_fractional_measurement_and_elevation();
    puts("Editor Plan snapping tests passed");
}
