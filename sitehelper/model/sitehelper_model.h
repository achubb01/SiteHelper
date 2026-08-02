#ifndef SITEHELPER_MODEL_H
#define SITEHELPER_MODEL_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TIMBER_STUD,
    TIMBER_NOGGIN,
    TIMBER_PLATE,
    TIMBER_HEADER,
    TIMBER_SILL
} TimberType;

typedef enum {
    STUD_SPACING_EVEN,
    STUD_SPACING_MAXIMISE
} StudSpacingMode;

typedef enum {
    OPENING_DOOR,
    OPENING_WINDOW
} OpeningType;

typedef enum {
    STUD_COMMON,
    STUD_KING,
    STUD_TRIMMER,
    STUD_CRIPPLE
} StudType;

typedef struct {
    int x;
    int y;
}Position;



typedef struct {
    OpeningType type;

    int frame_position;
    int frame_bottom;

    int width;
    int height;

    int width_allowance;
    int height_allowance;

    bool custom_allowance;
} Opening;

typedef struct {
    StudType type;
} Stud;

typedef struct {
    size_t bay;
} Noggin;

typedef struct {
    int placeholder;
} Plate;

typedef struct Timber {
    int length;
    int depth;
    int width;

    Position position;

    TimberType type;

    union {
        Stud stud;
        Noggin noggin;
        Plate plate;
    } details;
} Timber;

typedef struct Wall {
    Timber *studs;
    size_t stud_count;
    size_t stud_capacity;

    Timber *nogs;
    size_t nog_count;
    size_t nog_capacity;

    Timber *members;
    size_t member_count;
    size_t member_capacity;

    Timber bottomplate;
    Timber topplate;

    Opening *openings;
    size_t opening_count;
    size_t opening_capacity;
} Wall;

typedef struct Room {
    Wall *walls;
    size_t wall_count;
    size_t wall_capacity;
} Room;

typedef struct {
    Room *rooms;
    size_t room_count;
    size_t room_capacity;
} BuildStructure;

typedef struct {
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