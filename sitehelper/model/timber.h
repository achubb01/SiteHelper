#ifndef TIMBER_H
#define TIMBER_H

#include <stddef.h>

#include "position.h"

typedef enum
{
    TIMBER_STUD,
    TIMBER_NOGGIN,
    TIMBER_PLATE,
    /* Generated opening-framing role only. The current Wall generator does not
     * structurally size or approve a lintel/header; see structural/README.md. */
    TIMBER_HEADER,
    TIMBER_SILL
} TimberType;

typedef enum
{
    STUD_COMMON,
    STUD_KING,
    STUD_TRIMMER,
    STUD_CRIPPLE
} StudType;

typedef enum {
    WALL_MEMBER_NONE,
    WALL_MEMBER_BOTTOM_PLATE,
    WALL_MEMBER_TOP_PLATE,
    WALL_MEMBER_STUD,
    WALL_MEMBER_NOGGIN,
    WALL_MEMBER_GENERATED
} WallMemberKind;

typedef struct
{
    StudType type;
} Stud;

typedef struct
{
    size_t bay;
} Noggin;

typedef struct
{
    int placeholder;
} Plate;

/* Member length/depth/width and wall-local U/Z position are integer millimetres,
 * including generated framing. Kinds, types and bay indices are not distances. */
typedef struct Timber
{
    int length;
    int depth;
    int width;

    WallLocalPosition position;
    TimberType type;

    union
    {
        Stud stud;
        Noggin noggin;
        Plate plate;
    } details;
} Timber;

#endif
