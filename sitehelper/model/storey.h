#ifndef STOREY_H
#define STOREY_H

#include "build_structure.h"

/* Authoritative, exclusively owned by SiteHelperProject. Containment owns all
 * Rooms, Walls and virtual separators; openings remain owned by Walls.
 * elevation_mm is the reference plane's vertical offset from project datum.
 * Wall-local Z=0 is relative to this plane. Negative and equal elevations are
 * valid; neither elevation nor array position determines identity. */
typedef struct Storey {
    DomainId id;
    int elevation_mm;
    BuildStructure structure;
} Storey;

#endif
