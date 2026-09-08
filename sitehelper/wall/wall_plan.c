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
    /* Validate each definition against the preceding openings, as insertion
     * does. Every pair is checked once, with no self-overlap or allocation. */
    candidate.definition.opening_count = 0;
    for (size_t i = 0; i < wall->definition.opening_count; i++) {
        const Opening *opening = &wall->definition.openings[i];
        WallOpeningProposal proposal = {
            .type = opening->type,
            .frame_position = opening->frame_position,
            .frame_bottom = opening->frame_bottom,
            .width = opening->width,
            .height = opening->height,
            .width_allowance = opening->width_allowance,
            .height_allowance = opening->height_allowance,
            .custom_allowance = opening->custom_allowance
        };
        if (wall_validate_opening(&candidate, settings, &proposal).code != WALL_OPENING_VALID) {
            return 0;
        }
        candidate.definition.opening_count++;
    }
    if (!wall_generate(&candidate, settings)) {
        return 0; /* wall_generate releases its partial replacement framing. */
    }
    wall_framing_destroy(&wall->framing);
    wall->definition.segment = candidate.definition.segment;
    wall->framing = candidate.framing;
    return 1;
}
