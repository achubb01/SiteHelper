#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_support.h"
#include "command_history.h"
#include "sitehelper_persistence.h"
#include "app_view.h"
#include "appstate.h"
#ifdef SITEHELPER_TEST_TOPOLOGY
#include "room_region.h"
#include "wall_junctions.h"
#endif

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX;
static int failed;
static int reject_all;
static size_t allocation_count;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int reject(void)
{
    allocation_count++;
    if (reject_all || (fail_after != SIZE_MAX && fail_after-- == 0)) { failed = 1; return 1; }
    return 0;
}
void *__wrap_malloc(size_t size) { return reject() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return reject() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *p, size_t size) { return reject() ? NULL : __real_realloc(p, size); }
#endif

static const WallPlanSegment span = {{0, 0}, {6000, 0}};
static const PlanSegment separator_span = {{0, 0}, {0, 4000}};

static void valid(const SiteHelperProject *project)
{
    assert(sitehelper_project_validate(project).code == SITEHELPER_PROJECT_VALID);
}

static void test_lifecycle_and_ownership(void)
{
    SiteHelperProject project;
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 1; allocation_count = 0;
#endif
    sitehelper_project_init(&project);
    valid(&project);
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 0; assert(allocation_count == 0);
#endif
    assert(project.storeys == NULL && project.storey_count == 0 && project.domain_ids.next == 1);
    const int elevations[] = {0, -2700, 2700, 0};
    DomainId ids[4], rooms[4], walls[4], separators[4];
    for (size_t i = 0; i < 4; i++) {
        DomainId next = project.domain_ids.next;
        ids[i] = sitehelper_project_add_storey(&project, elevations[i]);
        assert(ids[i] == next && project.domain_ids.next == next + 1);
        const Storey *s = sitehelper_project_find_storey_by_id_const(&project, ids[i]);
        assert(s && s->elevation_mm == elevations[i]);
        assert(s->structure.walls == NULL && s->structure.rooms == NULL && s->structure.room_separators == NULL);
        rooms[i] = sitehelper_project_add_room(&project, ids[i]);
        walls[i] = sitehelper_project_add_wall(&project, ids[i], span);
        separators[i] = sitehelper_project_add_room_separator(&project, ids[i], separator_span);
        assert(rooms[i] && walls[i] && separators[i]);
        valid(&project);
    }
    assert(project.storey_count == 4);
    for (size_t i = 0; i < 4; i++) {
        Storey *s = sitehelper_project_find_storey_by_id(&project, ids[i]);
        assert(s == &project.storeys[i]);
        assert(sitehelper_project_find_room_by_id(&project, rooms[i]) == &s->structure.rooms[0]);
        const Storey *owner = NULL;
        assert(sitehelper_project_find_room_with_owner_const(&project, rooms[i], &owner) == &s->structure.rooms[0]);
        assert(owner == s);
        assert(!sitehelper_project_find_room_with_owner_const(&project, walls[i], &owner) && owner == NULL);
        assert(sitehelper_project_find_wall_by_id_const(&project, walls[i]) == &s->structure.walls[0]);
        assert(sitehelper_project_find_room_separator_by_id_const(&project, separators[i]) == &s->structure.room_separators[0]);
        assert(sitehelper_project_find_owning_storey_const(&project, walls[i])->id == ids[i]);
        for (size_t j = 0; j < 4; j++) {
            if (j != i) {
                assert(build_find_room_by_id(&s->structure, rooms[j]) == NULL);
                assert(build_find_wall_by_id(&s->structure, walls[j]) == NULL);
                assert(build_find_room_separator_by_id(&s->structure, separators[j]) == NULL);
            }
        }
    }
    SiteHelperProject before;
    test_clone_project_authoritative(&project, &before);
    assert(!sitehelper_project_add_room(&project, 0));
    assert(!sitehelper_project_add_wall(&project, UINT64_MAX, span));
    assert(!sitehelper_project_add_room_separator(&project, rooms[0], separator_span));
    assert(!sitehelper_project_insert_storey(&project, walls[3], 0));
    RoomSeparator collision = {.id = rooms[3], .segment = separator_span};
    assert(!sitehelper_project_insert_room_separator(&project, ids[0], &collision, 0));
    test_assert_project_authoritative_equal(&before, &project);
    for (int kind = 0; kind < 4; kind++) {
        project.domain_ids.next = UINT64_MAX;
        DomainId id = kind == 0 ? sitehelper_project_add_storey(&project, 0) :
            kind == 1 ? sitehelper_project_add_room(&project, ids[0]) :
            kind == 2 ? sitehelper_project_add_wall(&project, ids[0], span) :
            sitehelper_project_add_room_separator(&project, ids[0], separator_span);
        assert(id == 0 && project.domain_ids.next == UINT64_MAX);
    }
    project.domain_ids = before.domain_ids;
    test_assert_project_authoritative_equal(&before, &project);
    sitehelper_project_destroy(&before); sitehelper_project_destroy(&project);
    assert(project.storeys == NULL && project.storey_count == 0 && project.storey_capacity == 0);
    sitehelper_project_destroy(&project); sitehelper_project_destroy(NULL);
}

static void expect_validation(const SiteHelperProject *p, SiteHelperProjectValidationCode code)
{
    /* Borrowed top-level bytes only, never an independent owning snapshot. */
    SiteHelperProject before = *p;
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 1; allocation_count = 0;
#endif
    assert(sitehelper_project_validate(p).code == code);
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 0; assert(allocation_count == 0);
#endif
    assert(memcmp(p, &before, sizeof before) == 0);
}

static void test_global_validation(void)
{
    SiteHelperProject p;
    sitehelper_project_init(&p);
    DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, 0);
    DomainId ra = sitehelper_project_add_room(&p, a), rb = sitehelper_project_add_room(&p, b);
    DomainId wa = sitehelper_project_add_wall(&p, a, span), wb = sitehelper_project_add_wall(&p, b, span);
    DomainId sep = sitehelper_project_add_room_separator(&p, b, separator_span);
    OpeningCommand opening;
    DomainId opening_id;
    assert(opening_command_create(wa, OPENING_WINDOW, 1200, 900, 800, 1000, &opening));
    assert(opening_command_execute(&p, &opening, &opening_id));
    valid(&p);
    Storey *storage = p.storeys;
    p.storeys = NULL; expect_validation(&p, SITEHELPER_PROJECT_INVALID_STOREY_COLLECTION); p.storeys = storage;
    size_t capacity = p.storey_capacity;
    p.storey_capacity = 1; expect_validation(&p, SITEHELPER_PROJECT_INVALID_STOREY_COLLECTION); p.storey_capacity = capacity;
    p.storeys[1].id = 0; expect_validation(&p, SITEHELPER_PROJECT_INVALID_STOREY_ID);
    p.storeys[1].id = a; expect_validation(&p, SITEHELPER_PROJECT_DUPLICATE_ID);
    p.storeys[1].id = wa; expect_validation(&p, SITEHELPER_PROJECT_DUPLICATE_ID); p.storeys[1].id = b;
    p.storeys[1].structure.rooms[0].id = ra; expect_validation(&p, SITEHELPER_PROJECT_DUPLICATE_ID);
    p.storeys[1].structure.rooms[0].id = rb;
    p.storeys[1].structure.room_separators[0].id = opening_id; expect_validation(&p, SITEHELPER_PROJECT_DUPLICATE_ID);
    p.storeys[1].structure.room_separators[0].id = sep;
    p.storeys[1].structure.walls[0].id = wa; expect_validation(&p, SITEHELPER_PROJECT_DUPLICATE_ID);
    p.storeys[1].structure.walls[0].id = wb;
    DomainId next = p.domain_ids.next;
    p.domain_ids.next = opening_id; expect_validation(&p, SITEHELPER_PROJECT_INVALID_ID_GENERATOR); p.domain_ids.next = next;
    /* Later Storey metadata must be checked before an earlier ID scan. */
    Wall *walls = p.storeys[1].structure.walls;
    p.storeys[1].structure.walls = NULL; expect_validation(&p, SITEHELPER_PROJECT_INVALID_WALL_COLLECTION);
    p.storeys[1].structure.walls = walls;
    walls[0].definition.segment.end = span.start; expect_validation(&p, SITEHELPER_PROJECT_INVALID_WALL_GEOMETRY);
    walls[0].definition.segment = span;
    p.storeys[1].structure.room_separators[0].segment = (PlanSegment){0};
    expect_validation(&p, SITEHELPER_PROJECT_INVALID_ROOM_SEPARATOR_GEOMETRY);
    p.storeys[1].structure.room_separators[0].segment = separator_span;
    sitehelper_project_find_wall_by_id(&p, wa)->definition.openings[0].height = 9000;
    expect_validation(&p, SITEHELPER_PROJECT_INVALID_OPENING);
    sitehelper_project_find_wall_by_id(&p, wa)->definition.openings[0].height = 1000;
    valid(&p); sitehelper_project_destroy(&p);
}

static void test_commands_and_history(void)
{
    SiteHelperProject p;
    sitehelper_project_init(&p);
    DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, 2700);
    SiteHelperCommandHistory history;
    sitehelper_command_history_init(&history);
    WallCommand add;
    SiteHelperCommand command;
    SiteHelperCommandResult result;
    assert(wall_command_create(b, span, &add));
    assert(sitehelper_command_from_wall(&add, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    DomainId wall = result.data.add_wall.wall_id;
    assert(p.storeys[0].structure.wall_count == 0 && p.storeys[1].structure.wall_count == 1);
    assert(sitehelper_command_history_undo(&history, &p));
    assert(sitehelper_command_history_redo(&history, &p));
    assert(sitehelper_project_find_owning_storey(&p, wall)->id == b);
    OpeningCommand opening;
    assert(opening_command_create(wall, OPENING_WINDOW, 1200, 900, 800, 1000, &opening));
    assert(sitehelper_command_from_opening(&opening, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    DomainId opening_id = result.data.add_opening.opening_id;
    assert(sitehelper_project_find_owning_storey_const(&p, opening_id)->id == b);
    assert(sitehelper_command_history_undo(&history, &p));
    /* Redo identity collision on another Storey fails without moving cursor. */
    RoomSeparator collision = {.id = opening_id, .segment = separator_span};
    assert(sitehelper_project_insert_room_separator(&p, a, &collision, 0));
    size_t cursor = history.cursor;
    assert(!sitehelper_command_history_redo(&history, &p)); assert(history.cursor == cursor);
    assert(sitehelper_project_remove_room_separator_by_id(&p, opening_id));
    assert(sitehelper_command_history_redo(&history, &p));
    MoveWallEndpointCommand move;
    assert(move_wall_endpoint_command_create(wall, WALL_ENDPOINT_END, (PlanPosition){6000, 6000}, &move));
    assert(sitehelper_command_from_move_wall_endpoint(&move, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(sitehelper_project_find_wall_by_id(&p, wall)->definition.segment.end.y == 6000);
    assert(sitehelper_command_history_undo(&history, &p));
    assert(sitehelper_project_find_wall_by_id(&p, wall)->definition.segment.end.y == 0);
    DeleteWallCommand deletion;
    assert(delete_wall_command_create(wall, &deletion));
    assert(sitehelper_command_from_delete_wall(&deletion, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(sitehelper_project_find_wall_by_id(&p, wall) == NULL);
    collision.id = wall;
    assert(sitehelper_project_insert_room_separator(&p, a, &collision, 0));
    cursor = history.cursor;
    assert(!sitehelper_command_history_undo(&history, &p)); assert(history.cursor == cursor);
    assert(sitehelper_project_remove_room_separator_by_id(&p, wall));
    assert(sitehelper_command_history_undo(&history, &p));
    assert(sitehelper_project_find_owning_storey_const(&p, wall)->id == b);
    assert(sitehelper_project_find_wall_by_id(&p, wall)->definition.openings[0].id == opening_id);
    AddRoomSeparatorCommand add_separator;
    assert(add_room_separator_command_create(b, separator_span, &add_separator));
    assert(sitehelper_command_from_add_room_separator(&add_separator, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    DomainId sep = result.data.room_separator.separator_id;
    assert(sitehelper_command_history_undo(&history, &p));
    assert(sitehelper_command_history_redo(&history, &p));
    DeleteRoomSeparatorCommand delete_separator;
    assert(delete_room_separator_command_create(sep, &delete_separator));
    assert(sitehelper_command_from_delete_room_separator(&delete_separator, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(sitehelper_command_history_undo(&history, &p));
    assert(sitehelper_project_find_owning_storey_const(&p, sep)->id == b);
    MoveRoomSeparatorEndpointCommand separator_move;
    assert(move_room_separator_endpoint_command_create(sep, ROOM_SEPARATOR_ENDPOINT_END,
        (PlanPosition){1000, 4000}, &separator_move));
    assert(sitehelper_command_from_move_room_separator_endpoint(&separator_move, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(sitehelper_project_find_room_separator_by_id(&p, sep)->segment.end.x == 1000);
    assert(sitehelper_command_history_undo(&history, &p));
    assert(sitehelper_project_find_room_separator_by_id(&p, sep)->segment.end.x == 0);
    DomainId next = p.domain_ids.next;
    cursor = history.cursor;
    assert(wall_command_create(UINT64_MAX, span, &add));
    assert(sitehelper_command_from_wall(&add, &command));
    assert(!sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(p.domain_ids.next == next && history.cursor == cursor);

    DomainId room = sitehelper_project_add_room(&p, b);
    RoomLocationCommand placement;
    assert(room_location_command_create(room, (PlanPosition){3000, 2000}, &placement));
    assert(sitehelper_command_from_room_location(&placement, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(sitehelper_project_find_room_by_id(&p, room)->has_location);
    assert(sitehelper_command_history_undo(&history, &p));
    assert(!sitehelper_project_find_room_by_id(&p, room)->has_location);
    valid(&p); sitehelper_command_history_destroy(&history); sitehelper_project_destroy(&p);
}

static void write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "w"); assert(f); assert(fputs(text, f) >= 0); assert(fclose(f) == 0);
}

static void test_persistence(void)
{
    const char *path = "storey_roundtrip.tmp";
    SiteHelperProject p, loaded, before;
    sitehelper_project_init(&p); sitehelper_project_init(&loaded);
    int elevations[] = {2700, -1000, 0, 2700};
    for (size_t i = 0; i < 4; i++) {
        DomainId s = sitehelper_project_add_storey(&p, elevations[i]);
        if (i == 2) { continue; } /* Empty Storey retains its position/identity. */
        DomainId room = sitehelper_project_add_room(&p, s);
        assert(sitehelper_project_set_room_location(&p, room, (PlanPosition){-10, 20}));
        DomainId wall = sitehelper_project_add_wall(&p, s, span);
        assert(sitehelper_project_add_room_separator(&p, s, separator_span));
        OpeningCommand opening; DomainId id;
        assert(opening_command_create(wall, OPENING_WINDOW, 1200, 900, 800, 1000, &opening));
        assert(opening_command_execute(&p, &opening, &id));
        assert(opening_command_create(wall, OPENING_DOOR, 4000, 0, 800, 2000, &opening));
        assert(opening_command_execute(&p, &opening, &id));
    }
    /* Preserve nested stored order, including after an explicit reorder. */
    DomainId extra_room = sitehelper_project_add_room(&p, p.storeys[1].id);
    DomainId extra_wall = sitehelper_project_add_wall(&p, p.storeys[1].id, span);
    assert(extra_room && extra_wall);
    assert(wall_generate(sitehelper_project_find_wall_by_id(&p, extra_wall), &p.settings));
    assert(sitehelper_project_add_room_separator(&p, p.storeys[1].id, separator_span));
    BuildStructure *local = &p.storeys[1].structure;
    Room room_swap = local->rooms[0]; local->rooms[0] = local->rooms[1]; local->rooms[1] = room_swap;
    Wall wall_swap = local->walls[0]; local->walls[0] = local->walls[1]; local->walls[1] = wall_swap;
    RoomSeparator separator_swap = local->room_separators[0];
    local->room_separators[0] = local->room_separators[1]; local->room_separators[1] = separator_swap;
    Wall *with_openings = &local->walls[1];
    Opening opening_swap = with_openings->definition.openings[0];
    with_openings->definition.openings[0] = with_openings->definition.openings[1];
    with_openings->definition.openings[1] = opening_swap;
    assert(wall_generate(with_openings, &p.settings));
    assert(sitehelper_project_save_file(&p, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    test_assert_project_authoritative_equal(&p, &loaded);
    for (size_t i = 0; i < loaded.storey_count; i++) {
        const BuildStructure *expected = &p.storeys[i].structure, *actual = &loaded.storeys[i].structure;
        for (size_t k = 0; k < expected->room_count; k++) { assert(expected->rooms[k].id == actual->rooms[k].id); }
        for (size_t k = 0; k < expected->room_separator_count; k++) {
            assert(expected->room_separators[k].id == actual->room_separators[k].id);
        }
        for (size_t k = 0; k < expected->wall_count; k++) {
            assert(expected->walls[k].id == actual->walls[k].id);
            test_assert_framing_semantically_equal(&expected->walls[k].framing, &actual->walls[k].framing);
            for (size_t o = 0; o < expected->walls[k].definition.opening_count; o++) {
                assert(expected->walls[k].definition.openings[o].id == actual->walls[k].definition.openings[o].id);
            }
        }
    }
    for (size_t i = 0; i < loaded.storey_count; i++) {
        if (i == 2) { continue; }
        test_assert_framing_semantically_equal(&p.storeys[i].structure.walls[0].framing,
            &loaded.storeys[i].structure.walls[0].framing);
    }
    test_clone_project_authoritative(&loaded, &before);
    const char *invalid[] = {
        "storeys 1\nstorey 0 elevation 0\nwalls 0\nrooms 0\nroom_separators 0\nend_storey\n",
        "storeys 1\nstorey 1 elevation 2147483648\n",
        "storeys 2\nstorey 1 elevation 0\nwalls 0\nrooms 0\nroom_separators 0\nend_storey\nstorey 1 elevation 0\n",
        "storeys 1\nstorey 1 elevation 0\nwalls 0\nrooms 1\nroom 1 placement unplaced\nend_room\nroom_separators 0\nend_storey\n",
        "storeys 1\nstorey 1 elevation 0\nwalls 0\nrooms 0\nroom_separators 0\n",
        "storeys 2\nstorey 1 elevation 0\nwalls 0\nrooms 1\nroom 3 placement unplaced\nend_room\nroom_separators 0\nend_storey\nstorey 2 elevation 0\nwalls 0\nrooms 1\nroom 3 placement unplaced\nend_room\nroom_separators 0\nend_storey\n"
    };
    char text[2048];
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        snprintf(text, sizeof text, "sitehelper_project 8\ndomain_id_next 100\nsettings 2400 90 35 600 1200 0 0 maximise\n%send_project\n", invalid[i]);
        write_file(path, text);
        assert(sitehelper_project_load_file(&loaded, path) != SITEHELPER_PERSISTENCE_SUCCESS);
        test_assert_project_authoritative_equal(&before, &loaded);
    }
    /* A Storey itself participates in v8's allocator watermark, even empty. */
    const char *bad_watermarks[] = {"0", "1", "2", "18446744073709551615"};
    for (size_t i = 0; i < 3; i++) {
        snprintf(text, sizeof text, "sitehelper_project 8\ndomain_id_next %s\n"
            "settings 2400 90 35 600 1200 0 0 maximise\n"
            "storeys 1\nstorey 2 elevation 0\nwalls 0\nrooms 0\nroom_separators 0\nend_storey\nend_project\n",
            bad_watermarks[i]);
        write_file(path, text);
        assert(sitehelper_project_load_file(&loaded, path) != SITEHELPER_PERSISTENCE_SUCCESS);
        test_assert_project_authoritative_equal(&before, &loaded);
    }
    /* Every historical grammar preserves existing IDs, using the old watermark
     * for the new Storey. Includes rooms, walls and openings in every version. */
    for (int version = 1; version <= 7; version++) {
        const char *geometry = version == 1 ? "length 6000" : version < 4 ? "origin 0 0 length 6000" : "segment 0 0 6000 0";
        const char *room = version < 3 ? "end_room\n" : version < 5 ? "rooms 1\nroom 1 wall_refs 1\nwall_ref 2\nend_room\n" : version == 5 ? "rooms 1\nroom 1\nend_room\n" : "rooms 1\nroom 1 placement placed 2000 1000\nend_room\n";
        snprintf(text, sizeof text, "sitehelper_project %d\ndomain_id_next 50\nsettings 2400 90 35 600 1200 0 0 maximise\n%swall 2 %s openings 1\nopening 3 window 1200 900 800 1000 0 0 false\n%s%send_project\n",
            version, version < 3 ? "rooms 1\nroom 1 walls 1\n" : "walls 1\n", geometry, room,
            version == 7 ? "room_separators 1\nroom_separator 4 segment 0 0 0 4000\n" : "");
        write_file(path, text);
        assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
        assert(loaded.storey_count == 1 && loaded.storeys[0].id == 50 && loaded.storeys[0].elevation_mm == 0);
        assert(loaded.domain_ids.next == 51);
        assert(sitehelper_project_find_room_by_id(&loaded, 1));
        assert(sitehelper_project_find_wall_by_id(&loaded, 2)->definition.openings[0].id == 3);
        assert(loaded.storeys[0].structure.room_separator_count == (version == 7 ? 1u : 0u));
        if (version == 7) { assert(sitehelper_project_find_room_separator_by_id(&loaded, 4)); }
        valid(&loaded);
        sitehelper_project_destroy(&before); test_clone_project_authoritative(&loaded, &before);
        for (size_t i = 0; i < sizeof bad_watermarks / sizeof *bad_watermarks; i++) {
            char invalid_text[2048];
            snprintf(invalid_text, sizeof invalid_text, "sitehelper_project %d\ndomain_id_next %s\n%s",
                version, bad_watermarks[i], strstr(text, "settings "));
            write_file(path, invalid_text);
            assert(sitehelper_project_load_file(&loaded, path) != SITEHELPER_PERSISTENCE_SUCCESS);
            test_assert_project_authoritative_equal(&before, &loaded);
        }
        /* Fail after parsing all legacy objects; none may escape the candidate. */
        *strstr(text, "end_project") = '\0';
        write_file(path, text);
        assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_MALFORMED_DATA);
        test_assert_project_authoritative_equal(&before, &loaded);
    }
    sitehelper_project_destroy(&before); test_clone_project_authoritative(&loaded, &before);
    write_file(path, "sitehelper_project 7\ndomain_id_next 18446744073709551615\nsettings 2400 90 35 600 1200 0 0 maximise\nwalls 0\nrooms 0\nroom_separators 0\nend_project\n");
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_INVALID_PROJECT);
    test_assert_project_authoritative_equal(&before, &loaded);
    sitehelper_project_destroy(&before); sitehelper_project_destroy(&p); sitehelper_project_destroy(&loaded);
    sitehelper_project_init(&p); sitehelper_project_init(&loaded);
    assert(sitehelper_project_save_file(&p, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.storey_count == 0); valid(&loaded);
    assert(remove(path) == 0); sitehelper_project_destroy(&p); sitehelper_project_destroy(&loaded);
}

static void count_line(void *context, Vec2 a, Vec2 b, Colour colour)
{
    (void)a; (void)b; (void)colour; (*(size_t *)context)++;
}

static void test_editor_isolation(void)
{
    SiteHelperProject p;
    SiteHelperEditor editor;
    sitehelper_project_init(&p); sitehelper_editor_init(&editor);
    assert(editor.current_storey_id == 0);
    DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, 2700);
    DomainId wa = sitehelper_project_add_wall(&p, a, span), wb = sitehelper_project_add_wall(&p, b, span);
    assert(sitehelper_project_add_wall(&p, b, (WallPlanSegment){{0, 1000}, {6000, 1000}}));
    DomainId room = sitehelper_project_add_room(&p, a);
    EditorAction action;
    assert(!sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){1000, 0}, &action));
    assert(sitehelper_editor_set_current_storey(&editor, &p, a));
    assert(sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){1000, 0}, &action));
    assert(editor.current_wall_id == wa);
    editor.current_room_id = room;
    assert(app_current_room(&p, &editor)->id == room && app_current_wall(&p, &editor)->id == wa);
    Wall *wall = sitehelper_project_find_wall_by_id(&p, wa);
    assert(wall_generate(wall, &p.settings));
    sitehelper_editor_select_wall_member_at_position(&editor, wall, (WallLocalPosition){10, 10});
    assert(!editor_selection_is_empty(&editor.selection));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    assert(wall_tool_begin(&editor.wall_tool, (Vec2){100, 100}));
    sitehelper_editor_set_snap_result(&editor, (SnapResult){.type = SNAP_ENDPOINT, .position = {100, 100}});
    editor.opening_placement.has_candidate = 1;
    editor.opening_tool.preview_valid = 1;
    assert(sitehelper_editor_set_current_storey(&editor, &p, b));
    assert(editor.current_room_id == 0 && editor.current_wall_id == 0);
    assert(editor_selection_is_empty(&editor.selection) && !sitehelper_editor_has_wall_preview(&editor));
    assert(!sitehelper_editor_has_snap(&editor) && !editor.opening_placement.has_candidate && !editor.opening_tool.preview_valid);
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_SELECT));
    assert(sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){1000, 0}, &action));
    assert(editor.current_wall_id == wb);
    /* Even manually stale navigation cannot leak into app lookups/rendering. */
    editor.current_wall_id = wa; editor.current_room_id = room;
    assert(app_current_wall_const(&p, &editor) == NULL && app_current_room_const(&p, &editor) == NULL);
    sitehelper_editor_reconcile(&editor, &p);
    assert(editor.current_wall_id == 0 && editor.current_room_id == 0);
    Renderer2D *renderer = renderer2d_create(); assert(renderer);
    size_t lines = 0;
    renderer2d_set_backend(renderer, (RendererBackend){.context = &lines, .draw_line = count_line});
    renderer2d_set_camera(renderer, (Camera2D){.scale = 1});
    renderer2d_set_viewport(renderer, (Vec2){0}, 800, 600);
    WallRenderStyle style = {0};
    app_render_walls(renderer, &p, &editor, &style); assert(lines == 2);
    assert(sitehelper_editor_set_current_storey(&editor, &p, a));
    lines = 0; app_render_walls(renderer, &p, &editor, &style); assert(lines == 1);
    assert(!sitehelper_editor_set_current_storey(&editor, &p, UINT64_MAX));
    assert(editor.current_storey_id == a);
    assert(sitehelper_editor_set_current_storey(&editor, &p, 0));
    lines = 0; app_render_walls(renderer, &p, &editor, &style); assert(lines == 0);
    /* Wall Tool actions capture the active Storey and survive its reallocation. */
    assert(sitehelper_editor_set_current_storey(&editor, &p, b));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    assert(sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){0, 2000}, &action));
    assert(sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){6000, 2000}, &action));
    assert(action.kind == EDITOR_ACTION_COMMAND && action.command.data.wall.storey_id == b);
    assert(sitehelper_project_add_storey(&p, -2700));
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&p, &action.command, &result));
    sitehelper_editor_complete_action(&editor, &action, &result);
    assert(sitehelper_project_find_owning_storey_const(&p, editor.current_wall_id)->id == b);
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(app_current_wall_const(&p, &editor)->id == result.data.add_wall.wall_id);
    assert(sitehelper_editor_set_current_storey(&editor, &p, a));
    editor.current_wall_id = result.data.add_wall.wall_id;
    lines = 0; app_render_walls(renderer, &p, &editor, &style); assert(lines == 0);
    sitehelper_project_destroy(&p); sitehelper_project_init(&p);
    sitehelper_editor_reconcile(&editor, &p);
    assert(editor.current_storey_id == 0 && app_current_wall(&p, &editor) == NULL);
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_PLAN));
    lines = 0; app_render_walls(renderer, &p, &editor, &style); assert(lines == 0);
    assert(!sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){0, 0}, &action));
    renderer2d_destroy(renderer); sitehelper_project_destroy(&p);
}

#ifdef SITEHELPER_TEST_TOPOLOGY
static void test_storey_spatial_isolation(void)
{
    SiteHelperProject p;
    sitehelper_project_init(&p);
    DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, 2700);
    assert(sitehelper_project_add_wall(&p, a, span));
    assert(sitehelper_project_add_wall(&p, b, span)); /* Identical XY never overlaps across Storeys. */
    assert(sitehelper_project_add_wall(&p, a, (WallPlanSegment){{6000, 0}, {6000, 4000}}));
    assert(sitehelper_project_add_room_separator(&p, a, (PlanSegment){{6000, 4000}, {0, 4000}}));
    assert(sitehelper_project_add_room_separator(&p, a, separator_span));
    DomainId ra = sitehelper_project_add_room(&p, a), rb = sitehelper_project_add_room(&p, b);
    assert(sitehelper_project_set_room_location(&p, ra, (PlanPosition){2000, 1000}));
    assert(sitehelper_project_set_room_location(&p, rb, (PlanPosition){2000, 1000}));
    PlanTopology ta = {0}, tb = {0};
    assert(room_region_build_and_resolve(&p, ra, &ta).code == ROOM_REGION_BOUNDED);
    assert(room_region_build_and_resolve(&p, rb, &tb).code == ROOM_REGION_UNBOUNDED);
    assert(ta.edge_count == 4 && tb.edge_count == 1);
    for (size_t i = 0; i < ta.edge_count; i++) { assert(sitehelper_project_find_owning_storey_const(&p, ta.edges[i].source_id)->id == a); }
    WallJunctionSet ja = {0}, jb = {0};
    assert(wall_junctions_build(&ta, &ja) == WALL_JUNCTION_SUCCESS);
    assert(wall_junctions_build(&tb, &jb) == WALL_JUNCTION_SUCCESS);
    assert(ja.junction_count == 1 && ja.junctions[0].kind == WALL_JUNCTION_CORNER && jb.junction_count == 0);
    assert(sitehelper_project_add_wall(&p, b, (WallPlanSegment){{3000, -1000}, {3000, 1000}}));
    assert(plan_topology_build_from_storey(sitehelper_project_find_storey_by_id_const(&p, b), &tb).code == PLAN_TOPOLOGY_SUCCESS);
    assert(wall_junctions_build(&tb, &jb) == WALL_JUNCTION_SUCCESS);
    assert(jb.junction_count == 1 && jb.junctions[0].kind == WALL_JUNCTION_CROSS);
    assert(ta.edge_count == 4 && ja.junction_count == 1); /* Other snapshot unchanged. */
    /* Room ownership resolution must not inspect another Storey's Walls. */
    Wall *saved_walls = p.storeys[0].structure.walls;
    p.storeys[0].structure.walls = NULL;
    assert(room_region_build_and_resolve(&p, rb, &tb).code == ROOM_REGION_UNBOUNDED);
    assert(sitehelper_project_clear_room_location(&p, rb));
    assert(room_region_build_and_resolve(&p, rb, NULL).code == ROOM_REGION_UNPLACED);
    p.storeys[0].structure.walls = saved_walls;
    valid(&p); sitehelper_project_destroy(&p);
    wall_junctions_destroy(&ja); wall_junctions_destroy(&jb); plan_topology_destroy(&ta); plan_topology_destroy(&tb);
}
#endif

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_load_allocation_transactions(void)
{
    const char *path = "storey_allocation_load.tmp";
    SiteHelperProject source;
    sitehelper_project_init(&source);
    DomainId a = sitehelper_project_add_storey(&source, -500);
    DomainId b = sitehelper_project_add_storey(&source, 2700);
    assert(sitehelper_project_add_room(&source, a));
    assert(sitehelper_project_add_wall(&source, a, span));
    DomainId wall = sitehelper_project_add_wall(&source, b, span);
    assert(sitehelper_project_add_room_separator(&source, b, separator_span));
    OpeningCommand opening; DomainId opening_id;
    assert(opening_command_create(wall, OPENING_WINDOW, 1200, 900, 800, 1000, &opening));
    assert(opening_command_execute(&source, &opening, &opening_id));
    for (int legacy = 0; legacy < 2; legacy++) {
        if (legacy) {
            write_file(path, "sitehelper_project 7\ndomain_id_next 50\nsettings 2400 90 35 600 1200 0 0 maximise\nwalls 1\nwall 2 segment 0 0 6000 0 openings 1\nopening 3 window 1200 900 800 1000 0 0 false\nrooms 1\nroom 1 placement unplaced\nend_room\nroom_separators 1\nroom_separator 4 segment 0 0 0 4000\nend_project\n");
        } else { assert(sitehelper_project_save_file(&source, path) == SITEHELPER_PERSISTENCE_SUCCESS); }
        size_t failures = 0;
        for (size_t n = 0; n < 256; n++) {
            SiteHelperProject destination, before;
            test_clone_project_authoritative(&source, &destination);
            test_clone_project_authoritative(&source, &before);
            Storey *storage = destination.storeys;
            failed = 0; fail_after = n;
            SiteHelperPersistenceResult result = sitehelper_project_load_file(&destination, path);
            fail_after = SIZE_MAX;
            if (failed) {
                failures++;
                assert(result != SITEHELPER_PERSISTENCE_SUCCESS);
                assert(destination.storeys == storage);
                test_assert_project_authoritative_equal(&before, &destination);
                assert(sitehelper_project_load_file(&destination, path) == SITEHELPER_PERSISTENCE_SUCCESS);
            } else { assert(result == SITEHELPER_PERSISTENCE_SUCCESS); }
            valid(&destination);
            if (legacy) { assert(destination.storeys[0].id == 50 && destination.domain_ids.next == 51); }
            else { test_assert_project_authoritative_equal(&source, &destination); }
            sitehelper_project_destroy(&before); sitehelper_project_destroy(&destination);
            if (!failed) { break; }
            assert(n < 255);
        }
        assert(failures > 5);
        printf("Storey load allocation failures checked (%s): %zu\n", legacy ? "v7 migration" : "v8", failures);
    }
    assert(remove(path) == 0); sitehelper_project_destroy(&source);
}

static void test_history_allocation_transactions(void)
{
    size_t failures = 0;
    for (int undo_delete = 0; undo_delete < 2; undo_delete++) {
        for (size_t n = 0; n < 128; n++) {
            SiteHelperProject p, before;
            sitehelper_project_init(&p);
            DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, 2700);
            assert(sitehelper_project_add_wall(&p, a, span));
            SiteHelperCommandHistory history;
            sitehelper_command_history_init(&history);
            WallCommand add; SiteHelperCommand command; SiteHelperCommandResult result;
            assert(wall_command_create(b, span, &add));
            assert(sitehelper_command_from_wall(&add, &command));
            DomainId wall = p.domain_ids.next;
            if (undo_delete) {
                assert(sitehelper_command_history_execute(&history, &p, &command, &result));
                DeleteWallCommand deletion;
                assert(delete_wall_command_create(wall, &deletion));
                assert(sitehelper_command_from_delete_wall(&deletion, &command));
                assert(sitehelper_command_history_execute(&history, &p, &command, &result));
                /* Force Storey collection reallocation after undo capture. */
                assert(sitehelper_project_add_storey(&p, -1000));
            }
            test_clone_project_authoritative(&p, &before);
            size_t cursor = history.cursor, count = history.count;
            failed = 0; fail_after = n;
            int success = undo_delete ? sitehelper_command_history_undo(&history, &p) :
                sitehelper_command_history_execute(&history, &p, &command, &result);
            fail_after = SIZE_MAX;
            if (failed) {
                failures++; assert(!success);
                assert(history.cursor == cursor && history.count == count);
                test_assert_project_authoritative_equal(&before, &p);
                assert(undo_delete ? sitehelper_command_history_undo(&history, &p) :
                    sitehelper_command_history_execute(&history, &p, &command, &result));
            } else { assert(success); }
            assert(sitehelper_project_find_owning_storey_const(&p, wall)->id == b);
            valid(&p); sitehelper_project_destroy(&before);
            sitehelper_command_history_destroy(&history); sitehelper_project_destroy(&p);
            if (!failed) { break; }
            assert(n < 127);
        }
    }
    assert(failures > 5);
    printf("Storey history allocation failures checked: %zu\n", failures);
}

static void test_allocation_transactions(void)
{
    size_t failures = 0;
    /* All core creation paths, both growing the Storey array and mutating a
     * non-first Storey's empty collections, preserve project and allocator. */
    for (int operation = 0; operation < 4; operation++) {
        for (size_t fail_at = 0; fail_at < 16; fail_at++) {
            SiteHelperProject p, before;
            sitehelper_project_init(&p);
            assert(sitehelper_project_add_storey(&p, -1000));
            DomainId s = sitehelper_project_add_storey(&p, 0);
            test_clone_project_authoritative(&p, &before);
            Storey *storage = p.storeys;
            failed = 0; fail_after = fail_at;
            DomainId id = operation == 0 ? sitehelper_project_add_storey(&p, 2700) :
                operation == 1 ? sitehelper_project_add_room(&p, s) :
                operation == 2 ? sitehelper_project_add_wall(&p, s, span) :
                sitehelper_project_add_room_separator(&p, s, separator_span);
            fail_after = SIZE_MAX;
            if (failed) {
                failures++; assert(id == 0); assert(p.storeys == storage);
                test_assert_project_authoritative_equal(&before, &p);
            } else { assert(id); valid(&p); }
            sitehelper_project_destroy(&p); sitehelper_project_destroy(&before);
            if (!failed) { break; }
            assert(fail_at < 15);
        }
    }
    SiteHelperProject p;
    sitehelper_project_init(&p);
    fail_after = 0; failed = 0;
    assert(sitehelper_project_add_storey(&p, 0) == 0);
    fail_after = SIZE_MAX;
    assert(failed && p.storeys == NULL && p.domain_ids.next == 1);
    assert(sitehelper_project_add_storey(&p, 0) == 1);
    sitehelper_project_destroy(&p);
    printf("Storey creation allocation failures checked: %zu + empty project\n", failures);
}
#endif

int main(void)
{
    test_lifecycle_and_ownership(); test_global_validation(); test_commands_and_history();
    test_persistence(); test_editor_isolation();
#ifdef SITEHELPER_TEST_TOPOLOGY
    test_storey_spatial_isolation();
#endif
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_transactions(); test_load_allocation_transactions(); test_history_allocation_transactions();
#endif
    puts("Storey model tests passed");
    return 0;
}
