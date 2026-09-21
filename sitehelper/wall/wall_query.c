#include "wall_query.h"
#include "wall.h"
#include <stdint.h>
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
    if (wall == NULL || settings == NULL) { return DOMAIN_ID_INVALID; }
    for (size_t i = 0; i < wall->definition.opening_count; i++) {
        const Opening *opening = &wall->definition.openings[i];
        WallOpeningFrameGeometry frame;
        if (wall_opening_frame_geometry(opening, settings, &frame) &&
            position.u >= frame.left_u && position.u <= frame.right_u &&
            position.z >= frame.bottom_z && position.z <= frame.top_z) {
            return opening->id;
        }
    }
    return DOMAIN_ID_INVALID;
}


static bool timber_has_valid_dimensions(
    const Timber *timber
)
{
    return
        timber != NULL &&
        timber->length > 0 &&
        timber->width > 0;
}


static bool vertical_timber_contains_position(
    const Timber *timber,
    WallLocalPosition position
)
{
    if (!timber_has_valid_dimensions(timber)) {
        return false;
    }

    int left =
        timber->position.u;

    int right =
        timber->position.u +
        timber->width;

    int bottom =
        timber->position.z;

    int top =
        timber->position.z +
        timber->length;

    return
        position.u >= left &&
        position.u <= right &&
        position.z >= bottom &&
        position.z <= top;
}


static bool horizontal_timber_contains_position(
    const Timber *timber,
    WallLocalPosition position
)
{
    if (!timber_has_valid_dimensions(timber)) {
        return false;
    }

    int left =
        timber->position.u;

    int right =
        timber->position.u +
        timber->length;

    int bottom =
        timber->position.z;

    int top =
        timber->position.z +
        timber->width;

    return
        position.u >= left &&
        position.u <= right &&
        position.z >= bottom &&
        position.z <= top;
}


static const Timber *find_vertical_timber_in_array(
    const Timber *timbers,
    size_t count,
    WallLocalPosition position
)
{
    if (timbers == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        if (vertical_timber_contains_position(
                &timbers[i],
                position)) {

            return &timbers[i];
        }
    }

    return NULL;
}


static const Timber *find_horizontal_timber_in_array(
    const Timber *timbers,
    size_t count,
    WallLocalPosition position
)
{
    if (timbers == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        if (horizontal_timber_contains_position(
                &timbers[i],
                position)) {

            return &timbers[i];
        }
    }

    return NULL;
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

    if (wall == NULL) {
        return hit;
    }

    /*
     * Plates are checked first. Therefore, a position shared by a
     * stud and plate will currently select the plate.
     */

    if (horizontal_timber_contains_position(
            &wall->framing.bottomplate,
            position)) {

        hit.kind =
            WALL_MEMBER_BOTTOM_PLATE;

        hit.timber =
            &wall->framing.bottomplate;

        return hit;
    }

    if (horizontal_timber_contains_position(
            &wall->framing.topplate,
            position)) {

        hit.kind =
            WALL_MEMBER_TOP_PLATE;

        hit.timber =
            &wall->framing.topplate;

        return hit;
    }

    const Timber *selected =
        find_vertical_timber_in_array(
            wall->framing.studs,
            wall->framing.stud_count,
            position
        );

    if (selected != NULL) {

        hit.kind =
            WALL_MEMBER_STUD;

        hit.timber =
            selected;

        return hit;
    }

    selected =
        find_horizontal_timber_in_array(
            wall->framing.nogs,
            wall->framing.nog_count,
            position
        );

    if (selected != NULL) {

        hit.kind =
            WALL_MEMBER_NOGGIN;

        hit.timber =
            selected;

        return hit;
    }

    selected =
        find_horizontal_timber_in_array(
            wall->framing.members,
            wall->framing.member_count,
            position
        );

    if (selected != NULL) {

        hit.kind =
            WALL_MEMBER_GENERATED;

        hit.timber =
            selected;
    }

    return hit;
}
