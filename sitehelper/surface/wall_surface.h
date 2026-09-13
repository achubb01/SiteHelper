#ifndef WALL_SURFACE_H
#define WALL_SURFACE_H

#include "build_structure.h"
#include "build_settings.h"

typedef struct {
    DomainId source_opening_id;
    OpeningType type;
    int left_u_mm, bottom_z_mm;
    int width_mm, height_mm;
} WallSurfaceOpening;

/* Owned, side-neutral derived snapshot, with no identity of its own.
 * Boundary: U = 0 .. width_mm, Z = 0 .. height_mm, in integer millimetres.
 * Openings retain authoritative stored order, IDs and types. They represent
 * clear apertures, not framing assemblies or cladding clearance bands.
 * Provenance IDs are values; no borrowed source pointers. Do not shallow-copy
 * this structure into another owner. Not authoritative state or serialized. */
typedef struct {
    DomainId source_wall_id;
    int width_mm, height_mm;
    WallSurfaceOpening *openings;
    size_t opening_count;
} WallSurface;

typedef enum {
    WALL_SURFACE_SUCCESS = 0,
    WALL_SURFACE_INVALID_ARGUMENT,
    WALL_SURFACE_INVALID_SOURCE_WALL, /* Identity, segment or collection metadata. */
    WALL_SURFACE_INVALID_EXTENT, /* Nonpositive requested height. */
    WALL_SURFACE_INVALID_OPENING, /* Identity/type or canonical geometry failure. */
    WALL_SURFACE_OPENING_OUT_OF_BOUNDS,
    WALL_SURFACE_ALLOCATION_FAILED,
    WALL_SURFACE_NUMERIC_OVERFLOW /* Array/count arithmetic. */
} WallSurfaceCode;

typedef struct {
    WallSurfaceCode code;
    DomainId wall_id, opening_id; /* Offending source IDs when known, otherwise zero. */
} WallSurfaceResult;

/* Reads only Wall identity/definition and effective opening allowances, never
 * generated framing. Width uses the existing checked, rounded Wall length.
 * surface_height_mm must be positive and is explicit query input: stud_height
 * is not overall wall height and is neither read nor validated here.
 * Pass resolved settings for default allowances; custom allowances retain the
 * canonical Wall helper's precedence. Other construction settings are irrelevant.
 *
 * Source Wall/Opening IDs must be nonzero; uniqueness remains a model contract.
 * All apertures must fit inside the requested rectangle; touching bounds is
 * valid, clipping is never performed. This narrow query does not revalidate
 * opening assembly spacing/overlap or whole-project construction rules.
 * The canonical helpers report unrepresentable geometry as invalid geometry:
 * invalid source wall for an unrepresentable length, invalid opening for an
 * unrepresentable effective opening dimension. Right/top bounds use wide extents.
 *
 * Output must be zero-initialized or a previous successful independent result.
 * Success replaces it; every failure preserves its pointers and contents.
 * Input allocations must be valid and unchanged during the query. Metadata
 * checks cannot establish arbitrary pointer validity or actual allocation size.
 * A snapshot can outlive source mutation/destruction; check status on rebuild
 * because failure leaves the older snapshot describing its earlier inputs. */
WallSurfaceResult wall_surface_build(const Wall *wall, const BuildSettings *settings,
    int surface_height_mm, WallSurface *output);

/* Frees the opening array and zeros the result. NULL/zero/repeated calls safe. */
void wall_surface_destroy(WallSurface *surface);

#endif
