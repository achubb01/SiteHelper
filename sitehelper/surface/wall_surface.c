#include <stdlib.h>
#include "wall_surface.h"
#include "wall.h"

static WallSurfaceResult result(WallSurfaceCode code, DomainId wall_id, DomainId opening_id)
{
    return (WallSurfaceResult){code, wall_id, opening_id};
}

void wall_surface_destroy(WallSurface *surface)
{
    if (surface == NULL) { return; }
    free(surface->openings);
    *surface = (WallSurface){0};
}

WallSurfaceResult wall_surface_build(const Wall *wall, const BuildSettings *settings,
    int surface_height_mm, WallSurface *output)
{
    if (wall == NULL || settings == NULL || output == NULL) {
        return result(WALL_SURFACE_INVALID_ARGUMENT, 0, 0);
    }
    if (surface_height_mm <= 0) { return result(WALL_SURFACE_INVALID_EXTENT, wall->id, 0); }
    int width_mm = wall_length_mm(wall);
    if (wall->id == DOMAIN_ID_INVALID || width_mm <= 0) {
        return result(WALL_SURFACE_INVALID_SOURCE_WALL, wall->id, 0);
    }
    const WallDefinition *definition = &wall->definition;
    if (definition->opening_count > definition->opening_capacity ||
        (definition->opening_capacity == 0 && definition->openings != NULL) ||
        (definition->opening_capacity != 0 && definition->openings == NULL)) {
        return result(WALL_SURFACE_INVALID_SOURCE_WALL, wall->id, 0);
    }
    if (definition->opening_capacity > SIZE_MAX / sizeof *definition->openings ||
        definition->opening_count > SIZE_MAX / sizeof(WallSurfaceOpening)) {
        return result(WALL_SURFACE_NUMERIC_OVERFLOW, wall->id, 0);
    }
    WallSurface candidate = {
        .source_wall_id = wall->id, .width_mm = width_mm, .height_mm = surface_height_mm,
        .opening_count = definition->opening_count
    };
    if (candidate.opening_count != 0) {
        candidate.openings = malloc(candidate.opening_count * sizeof *candidate.openings);
        if (candidate.openings == NULL) { return result(WALL_SURFACE_ALLOCATION_FAILED, wall->id, 0); }
    }
    WallSurfaceResult status = result(WALL_SURFACE_SUCCESS, 0, 0);
    for (size_t i = 0; i < definition->opening_count; i++) {
        const Opening *opening = &definition->openings[i];
        WallOpeningFrameGeometry aperture;
        if (opening->id == DOMAIN_ID_INVALID ||
            (opening->type != OPENING_DOOR && opening->type != OPENING_WINDOW) ||
            !wall_opening_frame_geometry(opening, settings, &aperture)) {
            status = result(WALL_SURFACE_INVALID_OPENING, wall->id, opening->id);
            goto cleanup;
        }
        /* The canonical helper supplies checked int64_t extents, including
         * allowances. No second allowance interpretation or int addition. */
        if (aperture.left_u < 0 || aperture.bottom_z < 0 ||
            aperture.right_u > width_mm || aperture.top_z > surface_height_mm) {
            status = result(WALL_SURFACE_OPENING_OUT_OF_BOUNDS, wall->id, opening->id);
            goto cleanup;
        }
        candidate.openings[i] = (WallSurfaceOpening){
            .source_opening_id = opening->id, .type = opening->type,
            .left_u_mm = (int)aperture.left_u, .bottom_z_mm = (int)aperture.bottom_z,
            .width_mm = aperture.width, .height_mm = aperture.height
        };
    }
    wall_surface_destroy(output);
    *output = candidate;
    candidate = (WallSurface){0}; /* Ownership moved to output. */
cleanup:
    wall_surface_destroy(&candidate);
    return status;
}
