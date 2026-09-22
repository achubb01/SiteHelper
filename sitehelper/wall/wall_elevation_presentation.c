#include <limits.h>
#include <stdint.h>

#include "wall_elevation_presentation.h"

static WallElevationMemberRole stud_role(StudType type)
{
    switch (type) {
        case STUD_COMMON: return WALL_ELEVATION_MEMBER_ROLE_COMMON_STUD;
        case STUD_KING: return WALL_ELEVATION_MEMBER_ROLE_KING_STUD;
        case STUD_TRIMMER: return WALL_ELEVATION_MEMBER_ROLE_TRIMMER_STUD;
        case STUD_CRIPPLE: return WALL_ELEVATION_MEMBER_ROLE_CRIPPLE_STUD;
    }
    return WALL_ELEVATION_MEMBER_ROLE_UNKNOWN;
}

static WallElevationMemberRole generated_role(TimberType type)
{
    switch (type) {
        case TIMBER_HEADER: return WALL_ELEVATION_MEMBER_ROLE_HEADER;
        case TIMBER_SILL: return WALL_ELEVATION_MEMBER_ROLE_SILL;
        default: return WALL_ELEVATION_MEMBER_ROLE_UNKNOWN;
    }
}

const char *wall_elevation_member_role_name(WallElevationMemberRole role)
{
    switch (role) {
        case WALL_ELEVATION_MEMBER_ROLE_BOTTOM_PLATE: return "Bottom plate";
        case WALL_ELEVATION_MEMBER_ROLE_TOP_PLATE: return "Top plate";
        case WALL_ELEVATION_MEMBER_ROLE_COMMON_STUD: return "Common stud";
        case WALL_ELEVATION_MEMBER_ROLE_KING_STUD: return "King stud";
        case WALL_ELEVATION_MEMBER_ROLE_TRIMMER_STUD: return "Trimmer stud";
        case WALL_ELEVATION_MEMBER_ROLE_CRIPPLE_STUD: return "Cripple stud";
        case WALL_ELEVATION_MEMBER_ROLE_NOGGIN: return "Noggin";
        case WALL_ELEVATION_MEMBER_ROLE_HEADER: return "Header";
        case WALL_ELEVATION_MEMBER_ROLE_SILL: return "Sill";
        case WALL_ELEVATION_MEMBER_ROLE_UNKNOWN:
        default: return "Framing member";
    }
}

const char *wall_elevation_opening_type_name(OpeningType type)
{
    switch (type) {
        case OPENING_DOOR: return "Door";
        case OPENING_WINDOW: return "Window";
        default: return "Opening";
    }
}

static WallElevationRect member_bounds(
    const Timber *timber,
    WallElevationMemberOrientation orientation
)
{
    if (orientation == WALL_ELEVATION_MEMBER_VERTICAL) {
        return (WallElevationRect){
            .u = timber->position.u,
            .z = timber->position.z,
            .width = timber->width,
            .height = timber->length
        };
    }

    return (WallElevationRect){
        .u = timber->position.u,
        .z = timber->position.z,
        .width = timber->length,
        .height = timber->width
    };
}

static int set_member(
    WallElevationMember *output,
    WallMemberKind selection_kind,
    WallElevationMemberRole role,
    WallElevationMemberOrientation orientation,
    const Timber *source
)
{
    if (output == NULL || source == NULL) {
        return 0;
    }

    *output = (WallElevationMember){
        .selection_kind = selection_kind,
        .role = role,
        .orientation = orientation,
        .bounds = member_bounds(source, orientation),
        .source = source
    };

    return 1;
}

static int rect_contains(WallElevationRect rect, WallLocalPosition position)
{
    if (rect.width <= 0 || rect.height <= 0) {
        return 0;
    }

    int64_t right = (int64_t)rect.u + rect.width;
    int64_t top = (int64_t)rect.z + rect.height;

    return
        position.u >= rect.u &&
        (int64_t)position.u <= right &&
        position.z >= rect.z &&
        (int64_t)position.z <= top;
}

int wall_elevation_presentation_bounds(
    const Wall *wall,
    const BuildSettings *settings,
    WallElevationRect *bounds
)
{
    if (wall == NULL || settings == NULL || bounds == NULL ||
        !build_settings_valid(settings)) {
        return 0;
    }

    int length = wall_length_mm(wall);
    int64_t height = (int64_t)settings->stud_height + settings->stud_width;
    if (length <= 0 || height <= 0 || height > INT_MAX) {
        return 0;
    }

    *bounds = (WallElevationRect){
        .u = 0,
        .z = 0,
        .width = length,
        .height = (int)height
    };
    return 1;
}

size_t wall_elevation_presentation_member_count(const Wall *wall)
{
    if (wall == NULL) {
        return 0;
    }

    if (wall->framing.stud_count > SIZE_MAX - 2 ||
        wall->framing.nog_count > SIZE_MAX - 2 - wall->framing.stud_count ||
        wall->framing.member_count >
            SIZE_MAX - 2 - wall->framing.stud_count - wall->framing.nog_count) {
        return 0;
    }

    return 2 + wall->framing.stud_count + wall->framing.nog_count +
        wall->framing.member_count;
}

int wall_elevation_presentation_member_at(
    const Wall *wall,
    size_t index,
    WallElevationMember *member
)
{
    if (wall == NULL || member == NULL) {
        return 0;
    }

    if (index == 0) {
        return set_member(member, WALL_MEMBER_BOTTOM_PLATE,
            WALL_ELEVATION_MEMBER_ROLE_BOTTOM_PLATE,
            WALL_ELEVATION_MEMBER_HORIZONTAL, &wall->framing.bottomplate);
    }
    if (index == 1) {
        return set_member(member, WALL_MEMBER_TOP_PLATE,
            WALL_ELEVATION_MEMBER_ROLE_TOP_PLATE,
            WALL_ELEVATION_MEMBER_HORIZONTAL, &wall->framing.topplate);
    }

    index -= 2;
    if (index < wall->framing.stud_count) {
        const Timber *stud = &wall->framing.studs[index];
        return set_member(member, WALL_MEMBER_STUD,
            stud_role(stud->details.stud.type),
            WALL_ELEVATION_MEMBER_VERTICAL, stud);
    }

    index -= wall->framing.stud_count;
    if (index < wall->framing.nog_count) {
        return set_member(member, WALL_MEMBER_NOGGIN,
            WALL_ELEVATION_MEMBER_ROLE_NOGGIN,
            WALL_ELEVATION_MEMBER_HORIZONTAL, &wall->framing.nogs[index]);
    }

    index -= wall->framing.nog_count;
    if (index < wall->framing.member_count) {
        const Timber *generated = &wall->framing.members[index];
        return set_member(member, WALL_MEMBER_GENERATED,
            generated_role(generated->type),
            WALL_ELEVATION_MEMBER_HORIZONTAL, generated);
    }

    return 0;
}

int wall_elevation_presentation_find_member(
    const Wall *wall,
    WallLocalPosition position,
    WallElevationMember *member
)
{
    if (wall == NULL || member == NULL) {
        return 0;
    }

    size_t count = wall_elevation_presentation_member_count(wall);
    for (size_t i = 0; i < count; i++) {
        WallElevationMember candidate;
        if (wall_elevation_presentation_member_at(wall, i, &candidate) &&
            rect_contains(candidate.bounds, position)) {
            *member = candidate;
            return 1;
        }
    }

    return 0;
}

size_t wall_elevation_presentation_opening_count(const Wall *wall)
{
    return wall != NULL ? wall->definition.opening_count : 0;
}

int wall_elevation_presentation_opening_at(
    const Wall *wall,
    const BuildSettings *settings,
    size_t index,
    WallElevationOpening *opening
)
{
    if (wall == NULL || settings == NULL || opening == NULL ||
        index >= wall->definition.opening_count) {
        return 0;
    }

    const Opening *source = &wall->definition.openings[index];
    WallOpeningFrameGeometry frame;
    if (!wall_opening_frame_geometry(source, settings, &frame) ||
        frame.left_u < INT_MIN || frame.left_u > INT_MAX ||
        frame.bottom_z < INT_MIN || frame.bottom_z > INT_MAX) {
        return 0;
    }

    *opening = (WallElevationOpening){
        .opening_id = source->id,
        .type = source->type,
        .clear_bounds = {
            .u = (int)frame.left_u,
            .z = (int)frame.bottom_z,
            .width = frame.width,
            .height = frame.height
        }
    };
    return 1;
}

int wall_elevation_presentation_find_opening(
    const Wall *wall,
    const BuildSettings *settings,
    WallLocalPosition position,
    WallElevationOpening *opening
)
{
    if (wall == NULL || settings == NULL || opening == NULL) {
        return 0;
    }

    size_t count = wall_elevation_presentation_opening_count(wall);
    for (size_t i = 0; i < count; i++) {
        WallElevationOpening candidate;
        if (wall_elevation_presentation_opening_at(
                wall, settings, i, &candidate) &&
            rect_contains(candidate.clear_bounds, position)) {
            *opening = candidate;
            return 1;
        }
    }

    return 0;
}
