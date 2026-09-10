#include "sitehelper_persistence.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wall.h"

enum
{
    SITEHELPER_PROJECT_FORMAT_VERSION = 7,
    PERSISTENCE_TOKEN_CAPACITY = 64
};

typedef enum
{
    PERSISTENCE_TOKEN_EOF,
    PERSISTENCE_TOKEN_OK,
    PERSISTENCE_TOKEN_TOO_LONG
} PersistenceTokenResult;

static PersistenceTokenResult read_token(
    FILE *file,
    char token[PERSISTENCE_TOKEN_CAPACITY]
);

static SiteHelperPersistenceResult expect_token(
    FILE *file,
    const char *expected
);

static SiteHelperPersistenceResult read_required_token(
    FILE *file,
    char token[PERSISTENCE_TOKEN_CAPACITY]
);

static int parse_int_token(const char *token, int *value);
static int parse_size_token(const char *token, size_t *value);
static int parse_domain_id_token(const char *token, DomainId *value);
static int parse_stud_spacing_mode(
    const char *token,
    StudSpacingMode *mode
);
static int parse_opening_type(
    const char *token,
    OpeningType *type
);
static int parse_bool_token(const char *token, bool *value);

static const char *stud_spacing_mode_token(StudSpacingMode mode);
static const char *opening_type_token(OpeningType type);

static int project_contains_id(
    const SiteHelperProject *project,
    DomainId id
);
static SiteHelperPersistenceResult parse_project(
    FILE *file,
    SiteHelperProject *project
);
static SiteHelperPersistenceResult parse_settings(
    FILE *file,
    BuildSettings *settings
);
static SiteHelperPersistenceResult parse_room(
    FILE *file,
    SiteHelperProject *project,
    uintmax_t version
);
static SiteHelperPersistenceResult parse_wall(
    FILE *file,
    SiteHelperProject *project,
    uintmax_t version
);
static SiteHelperPersistenceResult parse_opening(
    FILE *file,
    SiteHelperProject *project,
    Wall *wall
);
static SiteHelperPersistenceResult regenerate_project(
    SiteHelperProject *project
);

static int write_project(
    FILE *file,
    const SiteHelperProject *project
);

static PersistenceTokenResult read_token(
    FILE *file,
    char token[PERSISTENCE_TOKEN_CAPACITY]
)
{
    if (file == NULL || token == NULL) {
        return PERSISTENCE_TOKEN_TOO_LONG;
    }

    int character;

    do {
        character = fgetc(file);
    } while (character != EOF && isspace((unsigned char)character));

    if (character == EOF) {
        return PERSISTENCE_TOKEN_EOF;
    }

    size_t length = 0;
    int too_long = 0;

    while (character != EOF && !isspace((unsigned char)character)) {
        if (length + 1 < PERSISTENCE_TOKEN_CAPACITY) {
            token[length++] = (char)character;
        }
        else {
            too_long = 1;
        }

        character = fgetc(file);
    }

    token[length] = '\0';

    return too_long
        ? PERSISTENCE_TOKEN_TOO_LONG
        : PERSISTENCE_TOKEN_OK;
}

static SiteHelperPersistenceResult read_required_token(
    FILE *file,
    char token[PERSISTENCE_TOKEN_CAPACITY]
)
{
    return read_token(file, token) == PERSISTENCE_TOKEN_OK
        ? SITEHELPER_PERSISTENCE_SUCCESS
        : SITEHELPER_PERSISTENCE_MALFORMED_DATA;
}

static SiteHelperPersistenceResult expect_token(
    FILE *file,
    const char *expected
)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];

    if (read_required_token(file, token) !=
        SITEHELPER_PERSISTENCE_SUCCESS) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    return strcmp(token, expected) == 0
        ? SITEHELPER_PERSISTENCE_SUCCESS
        : SITEHELPER_PERSISTENCE_MALFORMED_DATA;
}

static int parse_int_token(const char *token, int *value)
{
    if (token == NULL || value == NULL || token[0] == '\0') {
        return 0;
    }

    errno = 0;
    char *end = NULL;
    intmax_t parsed = strtoimax(token, &end, 10);

    if (errno == ERANGE || end == token || *end != '\0' ||
        parsed < INT_MIN || parsed > INT_MAX) {

        return 0;
    }

    *value = (int)parsed;
    return 1;
}

static int parse_size_token(const char *token, size_t *value)
{
    if (token == NULL || value == NULL || token[0] == '\0' ||
        token[0] == '-') {

        return 0;
    }

    errno = 0;
    char *end = NULL;
    uintmax_t parsed = strtoumax(token, &end, 10);

    if (errno == ERANGE || end == token || *end != '\0' ||
        parsed > SIZE_MAX) {

        return 0;
    }

    *value = (size_t)parsed;
    return 1;
}

static int parse_domain_id_token(const char *token, DomainId *value)
{
    if (token == NULL || value == NULL || token[0] == '\0' ||
        token[0] == '-') {

        return 0;
    }

    errno = 0;
    char *end = NULL;
    uintmax_t parsed = strtoumax(token, &end, 10);

    if (errno == ERANGE || end == token || *end != '\0' ||
        parsed > UINT64_MAX) {

        return 0;
    }

    *value = (DomainId)parsed;
    return 1;
}

static int parse_stud_spacing_mode(
    const char *token,
    StudSpacingMode *mode
)
{
    if (token == NULL || mode == NULL) {
        return 0;
    }

    if (strcmp(token, "even") == 0) {
        *mode = STUD_SPACING_EVEN;
        return 1;
    }

    if (strcmp(token, "maximise") == 0) {
        *mode = STUD_SPACING_MAXIMISE;
        return 1;
    }

    return 0;
}

static int parse_opening_type(
    const char *token,
    OpeningType *type
)
{
    if (token == NULL || type == NULL) {
        return 0;
    }

    if (strcmp(token, "door") == 0) {
        *type = OPENING_DOOR;
        return 1;
    }

    if (strcmp(token, "window") == 0) {
        *type = OPENING_WINDOW;
        return 1;
    }

    return 0;
}

static int parse_bool_token(const char *token, bool *value)
{
    if (token == NULL || value == NULL) {
        return 0;
    }

    if (strcmp(token, "false") == 0) {
        *value = false;
        return 1;
    }

    if (strcmp(token, "true") == 0) {
        *value = true;
        return 1;
    }

    return 0;
}

static const char *stud_spacing_mode_token(StudSpacingMode mode)
{
    switch (mode) {
        case STUD_SPACING_EVEN:
            return "even";

        case STUD_SPACING_MAXIMISE:
            return "maximise";

        default:
            return NULL;
    }
}

static const char *opening_type_token(OpeningType type)
{
    switch (type) {
        case OPENING_DOOR:
            return "door";

        case OPENING_WINDOW:
            return "window";

        default:
            return NULL;
    }
}

static int project_contains_id(
    const SiteHelperProject *project,
    DomainId id
)
{
    return project != NULL && build_contains_domain_id(&project->structure, id);
}

static SiteHelperPersistenceResult parse_settings(
    FILE *file,
    BuildSettings *settings
)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];

    if (expect_token(file, "settings") != SITEHELPER_PERSISTENCE_SUCCESS) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    int *integer_fields[] = {
        &settings->stud_height,
        &settings->stud_depth,
        &settings->stud_width,
        &settings->stud_spacing,
        &settings->nog_spacing,
        &settings->opening_width_allowance,
        &settings->opening_height_allowance
    };

    for (size_t index = 0;
         index < sizeof integer_fields / sizeof *integer_fields;
         index++) {

        if (read_required_token(file, token) !=
                SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, integer_fields[index])) {

            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
    }

    if (read_required_token(file, token) !=
            SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_stud_spacing_mode(token, &settings->stud_spacing_mode)) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    return build_settings_valid(settings)
        ? SITEHELPER_PERSISTENCE_SUCCESS
        : SITEHELPER_PERSISTENCE_INVALID_PROJECT;
}

static SiteHelperPersistenceResult parse_opening(
    FILE *file,
    SiteHelperProject *project,
    Wall *wall
)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    Opening opening = {0};

    if (expect_token(file, "opening") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_domain_id_token(token, &opening.id) ||
        opening.id == DOMAIN_ID_INVALID ||
        project_contains_id(project, opening.id) ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_opening_type(token, &opening.type)) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    int *integer_fields[] = {
        &opening.frame_position,
        &opening.frame_bottom,
        &opening.width,
        &opening.height,
        &opening.width_allowance,
        &opening.height_allowance
    };

    for (size_t index = 0;
         index < sizeof integer_fields / sizeof *integer_fields;
         index++) {

        if (read_required_token(file, token) !=
                SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, integer_fields[index])) {

            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
    }

    if (read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_bool_token(token, &opening.custom_allowance)) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    return wall_add_opening_definition(
        wall,
        &project->settings,
        &opening
    )
        ? SITEHELPER_PERSISTENCE_SUCCESS
        : SITEHELPER_PERSISTENCE_INVALID_PROJECT;
}

static SiteHelperPersistenceResult parse_wall(
    FILE *file,
    SiteHelperProject *project,
    uintmax_t version
)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    DomainId wall_id;
    WallPlanSegment segment = {0};
    size_t opening_count;

    if (expect_token(file, "wall") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_domain_id_token(token, &wall_id) ||
        wall_id == DOMAIN_ID_INVALID || project_contains_id(project, wall_id)) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    if (version >= 4) {
        if (expect_token(file, "segment") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &segment.start.x) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &segment.start.y) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &segment.end.x) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &segment.end.y)) {

            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
    } else {
        if (version >= 2) {
            if (expect_token(file, "origin") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &segment.start.x) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &segment.start.y)) {
                return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            }
        }
        int length;
        if (expect_token(file, "length") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &length)) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        /* Versions 1-3 represented horizontal walls. Check before adding. */
        if (length <= 0 || segment.start.x > INT_MAX - length) {
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        segment.end = (PlanPosition){
            .x = segment.start.x + length, .y = segment.start.y
        };
    }

    if (
        expect_token(file, "openings") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &opening_count)) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    Wall candidate = { .id = wall_id };
    if (!wall_set_plan_segment(&candidate, segment) ||
        !build_append_wall(&project->structure, &candidate)) {

        wall_destroy(&candidate);
        return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
    }

    Wall *wall = build_find_wall_by_id(&project->structure, wall_id);
    if (wall == NULL) {
        return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
    }

    for (size_t index = 0; index < opening_count; index++) {
        SiteHelperPersistenceResult result = parse_opening(
            file,
            project,
            wall
        );

        if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
            return result;
        }
    }

    return SITEHELPER_PERSISTENCE_SUCCESS;
}

/* Versions 3-4 carried unordered per-room wall references. Validate their
 * original syntax, targets and uniqueness, then discard them. This temporary
 * bitmap is parser state; it never becomes a Room/domain relationship. */
static SiteHelperPersistenceResult parse_legacy_wall_references(
    FILE *file, const SiteHelperProject *project, size_t count)
{
    if (count > project->structure.wall_count) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    if (count == 0) {
        return SITEHELPER_PERSISTENCE_SUCCESS;
    }
    unsigned char *seen = calloc(project->structure.wall_count, sizeof *seen);
    if (seen == NULL) {
        return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED;
    }
    SiteHelperPersistenceResult result = SITEHELPER_PERSISTENCE_SUCCESS;
    for (size_t i = 0; i < count; i++) {
        char token[PERSISTENCE_TOKEN_CAPACITY];
        DomainId id;
        if (expect_token(file, "wall_ref") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &id) || id == DOMAIN_ID_INVALID) {
            result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            break;
        }
        size_t index = 0;
        while (index < project->structure.wall_count && project->structure.walls[index].id != id) {
            index++;
        }
        if (index == project->structure.wall_count || seen[index]) {
            result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            break;
        }
        seen[index] = 1;
    }
    free(seen);
    return result;
}

static SiteHelperPersistenceResult parse_room(
    FILE *file,
    SiteHelperProject *project,
    uintmax_t version
)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    DomainId room_id;
    if (expect_token(file, "room") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_domain_id_token(token, &room_id) || room_id == DOMAIN_ID_INVALID ||
        project_contains_id(project, room_id)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    if (!build_add_room(&project->structure, room_id)) {
        return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
    }
    if (version >= 6) {
        if (expect_token(file, "placement") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (strcmp(token, "placed") == 0) {
            PlanPosition location;
            if (read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &location.x) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &location.y)) {
                return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            }
            if (!sitehelper_project_set_room_location(project, room_id, location)) {
                return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
            }
        }
        else if (strcmp(token, "unplaced") != 0) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
    }
    /* Versions 1-5 never specified semantic placement. Creation leaves these
     * rooms unplaced; legacy physical geometry must not synthesize a point. */
    if (version < 5) {
        size_t count;
        if (expect_token(file, version >= 3 ? "wall_refs" : "walls") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &count)) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (version >= 3) {
            SiteHelperPersistenceResult result = parse_legacy_wall_references(file, project, count);
            if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
                return result;
            }
        }
        else {
            /* Versions 1-2 nested physical definitions under rooms. Preserve
             * every identity and promote each wall into global project storage. */
            for (size_t i = 0; i < count; i++) {
                SiteHelperPersistenceResult result = parse_wall(file, project, version);
                if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
                    return result;
                }
            }
        }
    }
    return expect_token(file, "end_room");
}

static SiteHelperPersistenceResult parse_room_separators(FILE *file, SiteHelperProject *project)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file, "room_separators") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &count)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    for (size_t i = 0; i < count; i++) {
        RoomSeparator separator;
        if (expect_token(file, "room_separator") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &separator.id) || separator.id == DOMAIN_ID_INVALID ||
            project_contains_id(project, separator.id) ||
            expect_token(file, "segment") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &separator.segment.start.x) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &separator.segment.start.y) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &separator.segment.end.x) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &separator.segment.end.y)) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (!plan_segment_valid(separator.segment)) {
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        if (!build_insert_room_separator(&project->structure, &separator,
                project->structure.room_separator_count)) {
            return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED;
        }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_project(
    FILE *file,
    SiteHelperProject *project
)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    DomainId next;
    size_t room_count;

    if (expect_token(file, "sitehelper_project") !=
            SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    uintmax_t version;
    errno = 0;
    char *end = NULL;
    version = strtoumax(token, &end, 10);

    if (token[0] == '-' || errno == ERANGE || end == token ||
        *end != '\0') {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    if (version < 1 || version > SITEHELPER_PROJECT_FORMAT_VERSION) {
        return SITEHELPER_PERSISTENCE_UNSUPPORTED_VERSION;
    }

    if (expect_token(file, "domain_id_next") !=
            SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_domain_id_token(token, &next) ||
        next == DOMAIN_ID_INVALID) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    SiteHelperPersistenceResult result = parse_settings(
        file,
        &project->settings
    );

    if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
        return result;
    }

    if (version >= 3) {
        size_t wall_count;
        if (expect_token(file, "walls") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &wall_count)) {

            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }

        for (size_t index = 0; index < wall_count; index++) {
            result = parse_wall(file, project, version);
            if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
                return result;
            }
        }
    }

    if (expect_token(file, "rooms") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &room_count)) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    for (size_t index = 0; index < room_count; index++) {
        result = parse_room(file, project, version);

        if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
            return result;
        }
    }

    if (version >= 7) {
        result = parse_room_separators(file, project);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    if (expect_token(file, "end_project") !=
        SITEHELPER_PERSISTENCE_SUCCESS) {

        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    if (read_token(file, token) != PERSISTENCE_TOKEN_EOF) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    project->domain_ids.next = next;

    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult regenerate_project(
    SiteHelperProject *project
)
{
    if (project == NULL) {
        return SITEHELPER_PERSISTENCE_INVALID_ARGUMENT;
    }

    for (size_t wall_index = 0;
         wall_index < project->structure.wall_count;
         wall_index++) {

        if (!wall_generate(
                &project->structure.walls[wall_index],
                &project->settings)) {

            return SITEHELPER_PERSISTENCE_REGENERATION_FAILED;
        }
    }

    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static int write_project(
    FILE *file,
    const SiteHelperProject *project
)
{
    if (fprintf(file,
            "sitehelper_project %d\n"
            "domain_id_next %" PRIu64 "\n"
            "settings %d %d %d %d %d %d %d %s\n"
            "walls %zu\n",
            SITEHELPER_PROJECT_FORMAT_VERSION,
            (uint64_t)project->domain_ids.next,
            project->settings.stud_height,
            project->settings.stud_depth,
            project->settings.stud_width,
            project->settings.stud_spacing,
            project->settings.nog_spacing,
            project->settings.opening_width_allowance,
            project->settings.opening_height_allowance,
            stud_spacing_mode_token(project->settings.stud_spacing_mode),
            project->structure.wall_count) < 0) {

        return 0;
    }

    for (size_t wall_index = 0;
         wall_index < project->structure.wall_count;
         wall_index++) {

            const Wall *wall = &project->structure.walls[wall_index];

            if (fprintf(file,
                    "wall %" PRIu64 " segment %d %d %d %d openings %zu\n",
                    (uint64_t)wall->id,
                    wall->definition.segment.start.x,
                    wall->definition.segment.start.y,
                    wall->definition.segment.end.x,
                    wall->definition.segment.end.y,
                    wall->definition.opening_count) < 0) {

                return 0;
            }

            for (size_t opening_index = 0;
                 opening_index < wall->definition.opening_count;
                 opening_index++) {

                const Opening *opening =
                    &wall->definition.openings[opening_index];

                if (fprintf(file,
                        "opening %" PRIu64 " %s %d %d %d %d %d %d %s\n",
                        (uint64_t)opening->id,
                        opening_type_token(opening->type),
                        opening->frame_position,
                        opening->frame_bottom,
                        opening->width,
                        opening->height,
                        opening->width_allowance,
                        opening->height_allowance,
                        opening->custom_allowance ? "true" : "false") < 0) {

                    return 0;
                }
            }
    }

    if (fprintf(file, "rooms %zu\n", project->structure.room_count) < 0) {
        return 0;
    }

    for (size_t room_index = 0;
         room_index < project->structure.room_count;
         room_index++) {

        const Room *room = &project->structure.rooms[room_index];
        if (fprintf(file, "room %" PRIu64 " placement ", (uint64_t)room->id) < 0) {
            return 0;
        }
        if (room->has_location) {
            if (fprintf(file, "placed %d %d\n", room->location.x, room->location.y) < 0) {
                return 0;
            }
        }
        else if (fputs("unplaced\n", file) == EOF) {
            return 0;
        }
        if (fputs("end_room\n", file) == EOF) {
            return 0;
        }
    }

    if (fprintf(file, "room_separators %zu\n", project->structure.room_separator_count) < 0) { return 0; }
    for (size_t i = 0; i < project->structure.room_separator_count; i++) {
        const RoomSeparator *separator = &project->structure.room_separators[i];
        if (fprintf(file, "room_separator %" PRIu64 " segment %d %d %d %d\n",
                (uint64_t)separator->id, separator->segment.start.x, separator->segment.start.y,
                separator->segment.end.x, separator->segment.end.y) < 0) { return 0; }
    }
    return fputs("end_project\n", file) != EOF;
}

SiteHelperPersistenceResult sitehelper_project_save_file(
    const SiteHelperProject *project,
    const char *path
)
{
    if (project == NULL || path == NULL || path[0] == '\0') {
        return SITEHELPER_PERSISTENCE_INVALID_ARGUMENT;
    }

    if (sitehelper_project_validate(project).code != SITEHELPER_PROJECT_VALID) {
        return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
    }

    FILE *file = fopen(path, "w");

    if (file == NULL) {
        return SITEHELPER_PERSISTENCE_IO_ERROR;
    }

    int written = write_project(file, project);
    int close_result = fclose(file);

    return written && close_result == 0
        ? SITEHELPER_PERSISTENCE_SUCCESS
        : SITEHELPER_PERSISTENCE_IO_ERROR;
}

SiteHelperPersistenceResult sitehelper_project_load_file(
    SiteHelperProject *destination,
    const char *path
)
{
    if (destination == NULL || path == NULL || path[0] == '\0') {
        return SITEHELPER_PERSISTENCE_INVALID_ARGUMENT;
    }

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        return SITEHELPER_PERSISTENCE_IO_ERROR;
    }

    SiteHelperProject candidate;
    sitehelper_project_init(&candidate);

    SiteHelperPersistenceResult result = parse_project(file, &candidate);

    if (ferror(file)) {
        result = SITEHELPER_PERSISTENCE_IO_ERROR;
    }

    if (fclose(file) != 0 && result == SITEHELPER_PERSISTENCE_SUCCESS) {
        result = SITEHELPER_PERSISTENCE_IO_ERROR;
    }

    if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
        sitehelper_project_validate(&candidate).code != SITEHELPER_PROJECT_VALID) {
        result = SITEHELPER_PERSISTENCE_INVALID_PROJECT;
    }
    if (result == SITEHELPER_PERSISTENCE_SUCCESS) {
        result = regenerate_project(&candidate);
    }

    if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
        sitehelper_project_destroy(&candidate);
        return result;
    }

    SiteHelperProject previous = *destination;
    *destination = candidate;
    sitehelper_project_destroy(&previous);

    return SITEHELPER_PERSISTENCE_SUCCESS;
}
