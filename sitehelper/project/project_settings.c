#include <stdlib.h>

#include "sitehelper_project.h"
#include "project_settings_internal.h"
#include "wall.h"

int project_resolve_build_settings(const BuildSettings *defaults,
    const StoreyBuildSettings *overrides, BuildSettings *output)
{
    if (output == NULL || !build_settings_valid(defaults) ||
        !storey_build_settings_valid(overrides)) { return 0; }
    BuildSettings resolved = *defaults;
    if (overrides->has_stud_height_override) { resolved.stud_height = overrides->stud_height; }
    *output = resolved;
    return 1;
}

int sitehelper_project_resolve_storey_build_settings(const SiteHelperProject *project,
    DomainId storey_id, BuildSettings *output)
{
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, storey_id);
    return storey != NULL && project_resolve_build_settings(&project->settings, &storey->settings, output);
}

static int settings_equal(const BuildSettings *a, const BuildSettings *b)
{
    return a->stud_height == b->stud_height && a->stud_width == b->stud_width &&
        a->stud_depth == b->stud_depth && a->stud_spacing == b->stud_spacing &&
        a->stud_spacing_mode == b->stud_spacing_mode && a->nog_spacing == b->nog_spacing &&
        a->opening_width_allowance == b->opening_width_allowance &&
        a->opening_height_allowance == b->opening_height_allowance;
}

static int changed_settings(const SiteHelperProject *project, const Storey *storey,
    const BuildSettings *defaults, DomainId target, const StoreyBuildSettings *overrides,
    BuildSettings *resolved)
{
    BuildSettings previous;
    /* Both configurations were validated before traversal. */
    project_resolve_build_settings(&project->settings, &storey->settings, &previous);
    project_resolve_build_settings(defaults, storey->id == target ? overrides : &storey->settings, resolved);
    return !settings_equal(&previous, resolved);
}

typedef struct {
    Wall *wall; /* Borrowed only within this synchronous mutation; no collection changes. */
    WallFraming framing;
} PendingFraming;

static int change_settings(SiteHelperProject *project, BuildSettings defaults,
    DomainId target, StoreyBuildSettings overrides)
{
    if (!build_settings_valid(&defaults) || !storey_build_settings_valid(&overrides) ||
        sitehelper_project_validate(project).code != SITEHELPER_PROJECT_VALID) { return 0; }
    Storey *target_storey = target == DOMAIN_ID_INVALID ? NULL :
        sitehelper_project_find_storey_by_id(project, target);
    if (target != DOMAIN_ID_INVALID && target_storey == NULL) { return 0; }

    size_t count = 0;
    for (size_t s = 0; s < project->storey_count; s++) {
        const Storey *storey = &project->storeys[s];
        BuildSettings resolved;
        if (!changed_settings(project, storey, &defaults, target, &overrides, &resolved)) { continue; }
        if (storey->structure.wall_count > SIZE_MAX - count) { return 0; }
        count += storey->structure.wall_count;
    }
    if (count > SIZE_MAX / sizeof(PendingFraming)) { return 0; }
    PendingFraming *pending = count == 0 ? NULL : calloc(count, sizeof *pending);
    if (count != 0 && pending == NULL) { return 0; }

    size_t built = 0;
    for (size_t s = 0; s < project->storey_count; s++) {
        Storey *storey = &project->storeys[s];
        BuildSettings resolved;
        if (!changed_settings(project, storey, &defaults, target, &overrides, &resolved)) { continue; }
        for (size_t w = 0; w < storey->structure.wall_count; w++) {
            Wall *wall = &storey->structure.walls[w];
            Wall candidate = {.id = wall->id, .definition = wall->definition};
            /* Borrow the definition, validate every Opening against the proposed
             * configuration, then generate independently owned framing. */
            if (!wall_apply_plan_segment(&candidate, &resolved, wall->definition.segment)) {
                for (size_t i = 0; i < built; i++) { wall_framing_destroy(&pending[i].framing); }
                free(pending);
                return 0;
            }
            pending[built++] = (PendingFraming){wall, candidate.framing};
        }
    }

    /* No fallible work remains. Definitions and allocator are never changed. */
    project->settings = defaults;
    if (target_storey != NULL) { target_storey->settings = overrides; }
    for (size_t i = 0; i < built; i++) {
        wall_framing_destroy(&pending[i].wall->framing);
        pending[i].wall->framing = pending[i].framing;
    }
    free(pending);
    return 1;
}

int sitehelper_project_set_build_settings(SiteHelperProject *project, const BuildSettings *defaults)
{
    return project != NULL && defaults != NULL &&
        change_settings(project, *defaults, DOMAIN_ID_INVALID, (StoreyBuildSettings){0});
}

int sitehelper_project_set_stud_height(SiteHelperProject *project, int stud_height)
{
    if (project == NULL) { return 0; }
    BuildSettings defaults = project->settings;
    defaults.stud_height = stud_height;
    return sitehelper_project_set_build_settings(project, &defaults);
}

int sitehelper_project_set_storey_stud_height(SiteHelperProject *project,
    DomainId storey_id, int stud_height)
{
    return project != NULL && storey_id != DOMAIN_ID_INVALID &&
        change_settings(project, project->settings, storey_id,
            (StoreyBuildSettings){.has_stud_height_override = true, .stud_height = stud_height});
}

int sitehelper_project_clear_storey_stud_height(SiteHelperProject *project, DomainId storey_id)
{
    return project != NULL && storey_id != DOMAIN_ID_INVALID &&
        change_settings(project, project->settings, storey_id, (StoreyBuildSettings){0});
}
