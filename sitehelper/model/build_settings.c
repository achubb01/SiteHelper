#include <stddef.h>

#include "build_settings.h"

int build_settings_valid(const BuildSettings *settings)
{
    if (settings == NULL || settings->stud_height <= 0 ||
        settings->stud_depth <= 0 || settings->stud_width <= 0 ||
        settings->stud_spacing <= 0 || settings->nog_spacing <= 0) {
        return 0;
    }
    return settings->stud_spacing_mode == STUD_SPACING_EVEN ||
        settings->stud_spacing_mode == STUD_SPACING_MAXIMISE;
}
