#include "wall_query.h"
#include "wall.h"
#include "wall_elevation_presentation.h"
#include <float.h>
#include <math.h>

DomainId wall_plan_find_wall_at_position(const BuildStructure *structure,
    PlanPoint position, double tolerance_mm)
{
    if (structure == NULL || tolerance_mm < 0.0 ||
        !isfinite(position.x) || !isfinite(position.y)) return DOMAIN_ID_INVALID;
    DomainId best=DOMAIN_ID_INVALID;
    double nearest=tolerance_mm;
    for (size_t i=structure->wall_count;i>0;i--) {
        const Wall *wall=&structure->walls[i-1];
        WallPlanGeometry geometry;
        if (!wall_plan_geometry_build(&wall->definition,&geometry)) continue;
        double distance=wall_plan_geometry_distance(&geometry,position);
        if (distance <= nearest &&
            (best == DOMAIN_ID_INVALID || distance < nearest)) {
            best=wall->id; nearest=distance;
        }
    }
    return best;
}

DomainId wall_find_opening_at_position(const Wall *wall,
    const BuildSettings *settings, WallLocalPosition position)
{
    WallElevationOpening opening;
    if (!wall_elevation_presentation_find_opening(
            wall, settings, position, &opening)) {
        return DOMAIN_ID_INVALID;
    }
    return opening.opening_id;
}

WallMemberHit wall_find_member_at_position(
    const Wall *wall,
    WallLocalPosition position
)
{
    WallMemberHit hit = {
        .kind = WALL_MEMBER_NONE,
        .timber = NULL
    };

    WallElevationMember member;
    if (!wall_elevation_presentation_find_member(wall, position, &member)) {
        return hit;
    }

    hit.kind = member.selection_kind;
    hit.timber = member.source;
    return hit;
}
