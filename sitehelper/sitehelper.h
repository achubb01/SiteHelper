#ifndef SITEHELPER_H
#define SITEHELPER_H

#include "appcontext.h"

int build_add_room(BuildStructure *structure);
int room_add_wall(Room *room);
int build_set_stud_spacing(BuildSettings *settings, int spacing);
int wall_set_length(Wall *wall, int length);
int wall_set_stud_spacing(Wall *wall, int length);
int wall_add_stud(Wall *wall, const BuildSettings *settings, int position, StudType type);
int wall_add_noggin(Wall *wall, const BuildSettings *settings, size_t bay, int vertical_position);
int wall_generate(Wall *wall, const BuildSettings *settings);
int opening_frame_width(const Opening *opening, const BuildSettings *settings);
int opening_frame_height(const Opening *opening, const BuildSettings *setting);
int wall_add_opening(Wall *wall, const BuildSettings *settings, OpeningType type, int frame_position, int frame_bottom, int width, int height);
int wall_opening_fits(const Wall *wall, const Opening *opening, const BuildSettings *settings);

#endif