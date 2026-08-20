#ifndef SITEHELPER_H
#define SITEHELPER_H

#include "../model/sitehelper_model.h"

int build_add_room(BuildStructure *structure, DomainId room_id);
int room_add_wall(Room *room, DomainId wall_id);
Room *build_find_room_by_id(BuildStructure *structure, DomainId room_id);
const Room *build_find_room_by_id_const(const BuildStructure *structure, DomainId room_id);
Wall *room_find_wall_by_id(Room *room, DomainId wall_id);
const Wall *room_find_wall_by_id_const(const Room *room, DomainId wall_id);
int build_set_stud_spacing(BuildSettings *settings, int spacing);
int wall_set_length(Wall *wall, int length);
int wall_set_stud_spacing(Wall *wall, int length);
int wall_add_stud(Wall *wall, const BuildSettings *settings, int position, StudType type);
int wall_add_noggin(Wall *wall, const BuildSettings *settings, size_t bay, int vertical_position);
int wall_generate(Wall *wall, const BuildSettings *settings);
int opening_frame_width(const Opening *opening, const BuildSettings *settings);
int opening_frame_height(const Opening *opening, const BuildSettings *setting);
int wall_add_opening(Wall *wall, const BuildSettings *settings, DomainId opening_id, OpeningType type, int frame_position, int frame_bottom, int width, int height);
Opening *wall_find_opening_by_id(Wall *wall, DomainId opening_id);
const Opening *wall_find_opening_by_id_const(const Wall *wall, DomainId opening_id);
int wall_opening_fits(const Wall *wall, const Opening *opening, const BuildSettings *settings);
int wall_remove_opening_by_id(Wall *wall, DomainId opening_id);

void wall_destroy(Wall *wall);
void wall_framing_destroy(WallFraming *framing);

#endif