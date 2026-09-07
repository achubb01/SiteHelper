#ifndef POSITION_H
#define POSITION_H

/* Physical building-plan coordinates, in millimetres. */
typedef struct PlanPosition
{
    int x;
    int y;
} PlanPosition;

/* Calculated physical building-plan coordinates, in millimetres; not persisted. */
typedef struct PlanPoint
{
    double x;
    double y;
} PlanPoint;

/* Wall elevation coordinates, in millimetres, independent of plan placement. */
typedef struct WallLocalPosition
{
    int u; /* Distance along the wall from U = 0. */
    int z; /* Height above the wall base. */
} WallLocalPosition;

#endif
