#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "build_structure.h"

int build_contains_domain_id(const BuildStructure *structure, DomainId id)
{
    if (structure == NULL || id == DOMAIN_ID_INVALID) {
        return 0;
    }
    for (size_t i = 0; i < structure->room_count; i++) {
        if (structure->rooms[i].id == id) { return 1; }
    }
    for (size_t i = 0; i < structure->wall_count; i++) {
        const Wall *wall = &structure->walls[i];
        if (wall->id == id) { return 1; }
        for (size_t j = 0; j < wall->definition.opening_count; j++) {
            if (wall->definition.openings[j].id == id) { return 1; }
        }
    }
    for (size_t i = 0; i < structure->room_separator_count; i++) {
        if (structure->room_separators[i].id == id) { return 1; }
    }
    return 0;
}

RoomSeparator *build_find_room_separator_by_id(BuildStructure *structure, DomainId id)
{
    if (structure == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < structure->room_separator_count; i++) {
        if (structure->room_separators[i].id == id) { return &structure->room_separators[i]; }
    }
    return NULL;
}

const RoomSeparator *build_find_room_separator_by_id_const(const BuildStructure *structure, DomainId id)
{
    if (structure == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < structure->room_separator_count; i++) {
        if (structure->room_separators[i].id == id) { return &structure->room_separators[i]; }
    }
    return NULL;
}

int build_insert_room_separator(BuildStructure *structure,
    const RoomSeparator *separator, size_t index)
{
    if (structure == NULL || separator == NULL || separator->id == DOMAIN_ID_INVALID ||
        !plan_segment_valid(separator->segment) || index > structure->room_separator_count ||
        structure->room_separator_count > structure->room_separator_capacity ||
        (structure->room_separator_capacity != 0 && structure->room_separators == NULL) ||
        build_contains_domain_id(structure, separator->id)) {
        return 0;
    }
    RoomSeparator candidate = *separator;
    if (structure->room_separator_count == structure->room_separator_capacity) {
        size_t maximum = SIZE_MAX / sizeof *structure->room_separators;
        size_t capacity = structure->room_separator_capacity;
        if (capacity >= maximum) { return 0; }
        size_t grown = capacity == 0 ? 1 : capacity > maximum / 2 ? maximum : capacity * 2;
        RoomSeparator *storage = realloc(structure->room_separators, grown * sizeof *storage);
        if (storage == NULL) { return 0; }
        structure->room_separators = storage;
        structure->room_separator_capacity = grown;
    }
    memmove(&structure->room_separators[index + 1], &structure->room_separators[index],
        (structure->room_separator_count - index) * sizeof candidate);
    structure->room_separators[index] = candidate;
    structure->room_separator_count++;
    return 1;
}

int build_remove_room_separator_by_id(BuildStructure *structure, DomainId id)
{
    RoomSeparator *separator = build_find_room_separator_by_id(structure, id);
    if (separator == NULL) { return 0; }
    size_t index = (size_t)(separator - structure->room_separators);
    memmove(separator, separator + 1,
        (structure->room_separator_count - index - 1) * sizeof *separator);
    structure->room_separator_count--;
    structure->room_separators[structure->room_separator_count] = (RoomSeparator){0};
    return 1;
}
