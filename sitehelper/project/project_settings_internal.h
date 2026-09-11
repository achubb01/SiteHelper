#ifndef PROJECT_SETTINGS_INTERNAL_H
#define PROJECT_SETTINGS_INTERNAL_H

#include "build_settings.h"

/* The single resolution rule, also used with staged mutation/parser values.
 * Kept at the project orchestration boundary; Wall/model code never calls it. */
int project_resolve_build_settings(const BuildSettings *defaults,
    const StoreyBuildSettings *overrides, BuildSettings *output);

#endif
