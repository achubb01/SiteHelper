#ifndef SITEHELPER_H
#define SITEHELPER_H

#include "../model/sitehelper_model.h"

typedef struct
{
    OpeningType type;

    int frame_position; /* Wall-local U, in millimetres. */
    int frame_bottom;   /* Wall-local Z, in millimetres. */

    int width;
    int height;

    int width_allowance;
    int height_allowance;
    bool custom_allowance;
} WallOpeningProposal;

typedef enum
{
    WALL_OPENING_VALID = 0,

    WALL_OPENING_INVALID_ARGUMENT,
    WALL_OPENING_INVALID_TYPE,
    WALL_OPENING_INVALID_DIMENSIONS,
    WALL_OPENING_INVALID_HEIGHT,

    WALL_OPENING_TOO_CLOSE_TO_LEFT_END,
    WALL_OPENING_TOO_CLOSE_TO_RIGHT_END,

    WALL_OPENING_OVERLAPS_OPENING
} WallOpeningValidationCode;

typedef struct
{
    WallOpeningValidationCode code;

    DomainId conflicting_opening_id;
} WallOpeningValidation;

/* All construction APIs in this subsystem consume a complete effective
 * BuildSettings value supplied by their caller. They never resolve ownership
 * or inheritance. Live project callers must resolve the owning Storey first. */
WallOpeningValidation wall_validate_opening(
    const Wall *wall,
    const BuildSettings *settings,
    const WallOpeningProposal *proposal
);

int build_add_room(BuildStructure *structure, DomainId room_id);
int build_append_wall(BuildStructure *structure, Wall *wall);
int build_remove_wall_by_id(BuildStructure *structure, DomainId wall_id);
Room *build_find_room_by_id(BuildStructure *structure, DomainId room_id);
const Room *build_find_room_by_id_const(const BuildStructure *structure, DomainId room_id);
Wall *build_find_wall_by_id(BuildStructure *structure, DomainId wall_id);
const Wall *build_find_wall_by_id_const(
    const BuildStructure *structure,
    DomainId wall_id
);
int build_set_stud_spacing(BuildSettings *settings, int spacing);
/* Invalid geometry returns 0; mutation commits both ordered endpoints at once. */
int wall_length_mm(const Wall *wall);
int wall_set_plan_segment(Wall *wall, WallPlanSegment segment);
/* Validate geometry and all existing openings, then regenerate and commit the
 * complete segment/framing together. Failure leaves the wall unchanged.
 * Borrows openings without changing their local definitions or identities. */
int wall_apply_plan_segment(Wall *wall, const BuildSettings *settings, WallPlanSegment segment);
int wall_set_stud_spacing(Wall *wall, int length);
int wall_add_stud(Wall *wall, const BuildSettings *settings, int position, StudType type);
int wall_add_noggin(Wall *wall, const BuildSettings *settings, size_t bay, int vertical_position);
/* Generates framing entirely in U/Z; only derived length affects framing. */
int wall_generate(Wall *wall, const BuildSettings *settings);
/* Checked frame dimensions including allowances; return 0 if non-positive or
 * unrepresentable as an int. */
int opening_frame_width(const Opening *opening, const BuildSettings *settings);
int opening_frame_height(const Opening *opening, const BuildSettings *setting);
/* Adds a complete authoritative opening after normal opening validation. */
int wall_add_opening_definition(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening
);
int wall_add_opening(Wall *wall, const BuildSettings *settings, DomainId opening_id, OpeningType type, int frame_position, int frame_bottom, int width, int height);
Opening *wall_find_opening_by_id(Wall *wall, DomainId opening_id);
const Opening *wall_find_opening_by_id_const(const Wall *wall, DomainId opening_id);
int wall_remove_opening_by_id(Wall *wall, DomainId opening_id);

void wall_destroy(Wall *wall);
void wall_framing_destroy(WallFraming *framing);
void room_destroy(Room *room);

#endif
