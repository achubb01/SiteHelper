#ifndef BUILD_SETTINGS_H
#define BUILD_SETTINGS_H

#include <stdbool.h>

typedef enum
{
    STUD_SPACING_EVEN,
    STUD_SPACING_MAXIMISE
} StudSpacingMode;

/* All physical dimensions, spacings and allowances below are integer millimetres
 * (see MEASUREMENTS.md); flags and spacing modes are dimensionless.
 * Complete scalar configuration. Stored in Project as defaults; Wall APIs
 * consume a transient, resolved copy supplied by project orchestration. */
typedef struct
{
    int stud_height;
    int stud_depth;
    int stud_width;

    int stud_spacing;
    int nog_spacing;

    int opening_width_allowance;
    int opening_height_allowance;

    StudSpacingMode stud_spacing_mode;
} BuildSettings;

/* Narrow authoritative override. Inherited state has a canonical zero payload;
 * the flag alone selects inheritance, never the numeric value. */
typedef struct {
    bool has_stud_height_override;
    int stud_height;
} StoreyBuildSettings;

int storey_build_settings_valid(const StoreyBuildSettings *settings);

/* Authoritative scalar settings rules. Allowances are checked in context by
 * opening validation; no independent sign restriction is imposed on them. */
int build_settings_valid(const BuildSettings *settings);

#endif
