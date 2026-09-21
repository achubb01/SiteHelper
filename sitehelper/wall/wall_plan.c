#include "wall.h"

int wall_length_mm(const Wall *wall)
{
    return wall == NULL ? 0 : wall_plan_segment_length_mm(wall->definition.segment);
}

int wall_set_plan_segment(Wall *wall, WallPlanSegment segment)
{
    if (wall == NULL || wall_plan_segment_length_mm(segment) == 0) {
        return 0;
    }
    wall->definition.segment = segment;
    if (!wall_plan_specification_valid(wall->definition.plan_specification)) {
        wall->definition.plan_specification = wall_plan_specification_default();
    }
    return 1;
}

int wall_set_plan_specification(Wall *wall, WallPlanSpecification specification)
{
    if (wall == NULL || !wall_plan_specification_valid(specification)) return 0;
    wall->definition.plan_specification = specification;
    return 1;
}

int wall_apply_plan_segment(Wall *wall, const BuildSettings *settings, WallPlanSegment segment)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }
    /* Borrow only authoritative definitions. Candidate framing is separately
     * owned; never destroy the candidate's borrowed openings. */
    Wall candidate = { .id = wall->id, .definition = wall->definition };
    if (!wall_set_plan_segment(&candidate, segment)) {
        return 0;
    }
    if (!wall_generate(&candidate, settings)) {
        return 0; /* wall_generate releases its partial replacement framing. */
    }
    wall_framing_destroy(&wall->framing);
    wall->definition.segment = candidate.definition.segment;
    wall->framing = candidate.framing;
    return 1;
}
