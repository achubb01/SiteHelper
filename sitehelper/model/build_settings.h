#ifndef BUILD_SETTINGS_H
#define BUILD_SETTINGS_H

typedef enum
{
    STUD_SPACING_EVEN,
    STUD_SPACING_MAXIMISE
} StudSpacingMode;

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

#endif