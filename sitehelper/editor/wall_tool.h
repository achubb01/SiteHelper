#ifndef WALL_TOOL_H
#define WALL_TOOL_H

#include "geometry.h"
#include "wall_plan_segment.h"

/* Transient plan-space positions are double millimetres; resolved segments
 * and numeric length constraints use integer millimetres. No screen pixels. */
typedef struct
{
    int active;
    int has_start;
    Vec2 start;
    Vec2 endpoint; /* Normal snapped mouse endpoint. */
    Vec2 pointer;  /* Unconstrained pointer establishes numeric direction. */
    int length_mm; /* Optional transient constraint; zero means unconstrained. */
} WallTool;

typedef enum {
    WALL_LENGTH_OK, WALL_LENGTH_INACTIVE, WALL_LENGTH_NONPOSITIVE,
    WALL_LENGTH_DIRECTIONLESS, WALL_LENGTH_OUT_OF_RANGE
} WallLengthStatus;

void wall_tool_update_direction(WallTool *tool, Vec2 pointer);
void wall_tool_clear_length(WallTool *tool);
WallLengthStatus wall_tool_set_length(WallTool *tool, int length_mm);
/* Pure resolution: success guarantees the model's derived length equals input.
 * Failure leaves output untouched; endpoint coordinates must fit int. */
WallLengthStatus wall_tool_resolve_length(const WallTool *tool, int length_mm,
    WallPlanSegment *segment);

void wall_tool_init(WallTool *tool);
void wall_tool_activate(WallTool *tool);
void wall_tool_cancel(WallTool *tool);
void wall_tool_update(WallTool *tool, Vec2 endpoint);
int wall_tool_begin(WallTool *tool, Vec2 start);
/* Click order establishes U = 0 and the positive-U direction. */
int wall_tool_command_data(
    const WallTool *tool,
    WallPlanSegment *segment
);

#endif
