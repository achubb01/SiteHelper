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
#include "slab.h"
#include "roof.h"
#include "project_settings_internal.h"

enum
{
    SITEHELPER_PROJECT_FORMAT_VERSION = 22,
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
static int parse_int64_token(const char *token, int64_t *value);
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
static int parse_roof_generation(const char *token, RoofPortionGeneration *generation);
static int parse_roof_single_slope_reference(const char *token, RoofSingleSlopeReference *reference);
static int parse_roof_composition_kind(const char *token, RoofCompositionKind *kind);
static int parse_roof_end(const char *token, RoofEnd *end);

static const char *stud_spacing_mode_token(StudSpacingMode mode);
static const char *opening_type_token(OpeningType type);
static const char *roof_generation_token(RoofPortionGeneration generation);
static const char *roof_single_slope_reference_token(RoofSingleSlopeReference reference);
static const char *roof_composition_kind_token(RoofCompositionKind kind);
static const char *roof_end_token(RoofEnd end);

static int project_contains_id(const SiteHelperProject *project, DomainId id)
{
    return sitehelper_project_contains_domain_id(project, id);
}

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

static int parse_int64_token(const char *token, int64_t *value)
{
    if (token == NULL || value == NULL || token[0] == '\0') { return 0; }
    errno = 0;
    char *end = NULL;
    intmax_t parsed = strtoimax(token, &end, 10);
    if (errno == ERANGE || end == token || *end != '\0' ||
        parsed < INT64_MIN || parsed > INT64_MAX) { return 0; }
    *value = (int64_t)parsed;
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

static int parse_roof_generation(const char *token, RoofPortionGeneration *generation)
{
    if (token == NULL || generation == NULL) { return 0; }
    if (strcmp(token, "opposing_slopes") == 0) { *generation = ROOF_PORTION_OPPOSING_SLOPES; return 1; }
    if (strcmp(token, "all_boundary_slopes") == 0) { *generation = ROOF_PORTION_ALL_BOUNDARY_SLOPES; return 1; }
    if (strcmp(token, "single_slope") == 0) { *generation = ROOF_PORTION_SINGLE_SLOPE; return 1; }
    return 0;
}

static int parse_roof_single_slope_reference(const char *token, RoofSingleSlopeReference *reference)
{
    if (token == NULL || reference == NULL) { return 0; }
    if (strcmp(token, "low_edge") == 0) { *reference = ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE; return 1; }
    if (strcmp(token, "high_edge") == 0) { *reference = ROOF_SINGLE_SLOPE_REFERENCE_HIGH_EDGE; return 1; }
    return 0;
}

static int parse_roof_composition_kind(const char *token, RoofCompositionKind *kind)
{
    if (token == NULL || kind == NULL) { return 0; }
    if (strcmp(token, "intersects") == 0) { *kind = ROOF_COMPOSITION_INTERSECTS; return 1; }
    if (strcmp(token, "abuts") == 0) { *kind = ROOF_COMPOSITION_ABUTS; return 1; }
    return 0;
}

static int parse_roof_end(const char *token, RoofEnd *end)
{
    if (token == NULL || end == NULL) { return 0; }
    if (strcmp(token, "negative_axis") == 0) { *end = ROOF_END_NEGATIVE_AXIS; return 1; }
    if (strcmp(token, "positive_axis") == 0) { *end = ROOF_END_POSITIVE_AXIS; return 1; }
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



static const char *roof_generation_token(RoofPortionGeneration generation)
{
    switch (generation) {
        case ROOF_PORTION_OPPOSING_SLOPES: return "opposing_slopes";
        case ROOF_PORTION_ALL_BOUNDARY_SLOPES: return "all_boundary_slopes";
        case ROOF_PORTION_SINGLE_SLOPE: return "single_slope";
        default: return NULL;
    }
}

static const char *roof_single_slope_reference_token(RoofSingleSlopeReference reference)
{
    switch (reference) {
        case ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE: return "low_edge";
        case ROOF_SINGLE_SLOPE_REFERENCE_HIGH_EDGE: return "high_edge";
        default: return NULL;
    }
}

static const char *roof_composition_kind_token(RoofCompositionKind kind)
{
    switch (kind) {
        case ROOF_COMPOSITION_INTERSECTS: return "intersects";
        case ROOF_COMPOSITION_ABUTS: return "abuts";
        default: return NULL;
    }
}

static const char *roof_end_token(RoofEnd end)
{
    switch (end) {
        case ROOF_END_NEGATIVE_AXIS: return "negative_axis";
        case ROOF_END_POSITIVE_AXIS: return "positive_axis";
        default: return NULL;
    }
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

/* Frozen v1-v9 interpretation. Do not use opening_frame_height/width or
 * wall_opening_frame_geometry here: those APIs describe current model state.
 * Today their sums happen to equal the historical effective sizes, but their
 * contract must be free to evolve without reinterpreting persisted files.
 * Settings are resolved at the load boundary, including v9 Storey overrides. */
static SiteHelperPersistenceResult migrate_legacy_opening(Opening *opening,
    const BuildSettings *settings)
{
    int64_t width = (int64_t)opening->width + (opening->custom_allowance
        ? opening->width_allowance : settings->opening_width_allowance);
    int64_t height = (int64_t)opening->height + (opening->custom_allowance
        ? opening->height_allowance : settings->opening_height_allowance);
    int64_t top = (int64_t)opening->frame_bottom + height;
    if ((opening->type != OPENING_DOOR && opening->type != OPENING_WINDOW) ||
        opening->frame_position < 0 || opening->frame_bottom < 0 ||
        opening->width <= 0 || opening->height <= 0 ||
        width <= 0 || width > INT_MAX || height <= 0 || height > INT_MAX ||
        top > settings->stud_height) {
        return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
    }
    int deduction;
    if (opening->type == OPENING_WINDOW) {
        /* Legacy sill underside = bottom, header underside = bottom + H.
         * Legacy generation required positive lower AND upper cripples. */
        if (opening->frame_bottom == 0 || width < settings->stud_width ||
            top + settings->stud_width >= settings->stud_height) {
            return SITEHELPER_PERSISTENCE_OPENING_MIGRATION_FAILED;
        }
        deduction = settings->stud_width;
    }
    else {
        /* Legacy trimmers ended at H, ignoring B. Noggin exclusion did use B.
         * Keep B and use clear height H-B. The exclusion top changes from B+H
         * to H, but no bay inside the door can support a noggin at/above H:
         * its bordering trimmers end at H. Thus every member stays in place.
         * Doors historically generated no header/sill/cripples. */
        deduction = opening->frame_bottom;
    }
    int64_t clear_height = height - deduction;
    if (clear_height <= 0) {
        return SITEHELPER_PERSISTENCE_OPENING_MIGRATION_FAILED;
    }
    if (opening->type == OPENING_WINDOW) {
        /* Positive clear height proves B + W < B + H <= INT_MAX. */
        opening->frame_bottom += deduction;
    }
    if (opening->height > deduction) {
        opening->height -= deduction;
    }
    else {
        /* A positive legacy allowance can leave positive clear height even
         * when subtracting from nominal height yields zero/negative. Preserve
         * geometry explicitly, using a positive nominal size plus custom
         * allowances; retain effective width when switching off inheritance. */
        opening->height = (int)clear_height;
        opening->height_allowance = 0;
        if (!opening->custom_allowance) {
            opening->width_allowance = settings->opening_width_allowance;
        }
        opening->custom_allowance = true;
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_opening(
    FILE *file,
    SiteHelperProject *project,
    Wall *wall, const BuildSettings *resolved, uintmax_t version
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

    if (version < 10) {
        SiteHelperPersistenceResult result = migrate_legacy_opening(&opening, resolved);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    return wall_add_opening_definition(
        wall,
        resolved,
        &opening
    )
        ? SITEHELPER_PERSISTENCE_SUCCESS
        : SITEHELPER_PERSISTENCE_INVALID_PROJECT;
}

static SiteHelperPersistenceResult parse_wall(
    FILE *file,
    SiteHelperProject *project, BuildStructure *structure,
    uintmax_t version, const BuildSettings *resolved
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
        !build_append_wall(structure, &candidate)) {

        wall_destroy(&candidate);
        return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
    }

    Wall *wall = build_find_wall_by_id(structure, wall_id);
    if (wall == NULL) {
        return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
    }

    for (size_t index = 0; index < opening_count; index++) {
        SiteHelperPersistenceResult result = parse_opening(
            file,
            project,
            wall, resolved, version
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
    FILE *file, const BuildStructure *structure, size_t count)
{
    if (count > structure->wall_count) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    if (count == 0) {
        return SITEHELPER_PERSISTENCE_SUCCESS;
    }
    unsigned char *seen = calloc(structure->wall_count, sizeof *seen);
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
        while (index < structure->wall_count && structure->walls[index].id != id) {
            index++;
        }
        if (index == structure->wall_count || seen[index]) {
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
    SiteHelperProject *project, BuildStructure *structure,
    uintmax_t version, const BuildSettings *resolved
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
    if (!build_add_room(structure, room_id)) {
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
            SiteHelperPersistenceResult result = parse_legacy_wall_references(file, structure, count);
            if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
                return result;
            }
        }
        else {
            /* Versions 1-2 nested physical definitions under rooms. Preserve
             * every identity and promote each wall into the synthesized Storey. */
            for (size_t i = 0; i < count; i++) {
                SiteHelperPersistenceResult result = parse_wall(file, project, structure, version, resolved);
                if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
                    return result;
                }
            }
        }
    }
    return expect_token(file, "end_room");
}

static SiteHelperPersistenceResult parse_room_separators(FILE *file, SiteHelperProject *project, BuildStructure *structure)
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
        if (!build_insert_room_separator(structure, &separator,
                structure->room_separator_count)) {
            return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED;
        }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_structure(FILE *file, SiteHelperProject *project,
    BuildStructure *structure, uintmax_t version, const BuildSettings *resolved)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t room_count;
    SiteHelperPersistenceResult result;
    if (version >= 3) {
        size_t wall_count;
        if (expect_token(file, "walls") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &wall_count)) {

            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }

        for (size_t index = 0; index < wall_count; index++) {
            result = parse_wall(file, project, structure, version, resolved);
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
        result = parse_room(file, project, structure, version, resolved);

        if (result != SITEHELPER_PERSISTENCE_SUCCESS) {
            return result;
        }
    }

    if (version >= 7) {
        result = parse_room_separators(file, project, structure);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_penetrations(FILE *file, Slab *slab)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file,"penetrations") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token,&count)) { return SITEHELPER_PERSISTENCE_MALFORMED_DATA; }
    if (count > SIZE_MAX / sizeof(SlabPenetration)) { return SITEHELPER_PERSISTENCE_INVALID_PROJECT; }
    for (size_t i = 0; i < count; i++) {
        size_t vertex_count;
        if (expect_token(file,"penetration") != SITEHELPER_PERSISTENCE_SUCCESS ||
            expect_token(file,"outline") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token,&vertex_count)) { return SITEHELPER_PERSISTENCE_MALFORMED_DATA; }
        if (vertex_count < 3 || vertex_count > SIZE_MAX / sizeof(PlanPosition)) {
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        PlanPosition *vertices = malloc(vertex_count * sizeof *vertices);
        if (vertices == NULL) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        SiteHelperPersistenceResult result = SITEHELPER_PERSISTENCE_SUCCESS;
        for (size_t j = 0; j < vertex_count; j++) {
            if (expect_token(file,"vertex") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token,&vertices[j].x) ||
                read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token,&vertices[j].y)) {
                result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                break;
            }
        }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS) { result = expect_token(file,"end_penetration"); }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS) {
            SlabCode code = slab_add_penetration(slab,vertices,vertex_count);
            if (code != SLAB_SUCCESS) {
                result = code == SLAB_ALLOCATION_FAILED ? SITEHELPER_PERSISTENCE_ALLOCATION_FAILED : SITEHELPER_PERSISTENCE_INVALID_PROJECT;
            }
        }
        free(vertices);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_regions(FILE *file, Slab *slab)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file,"regions") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token,&count)) { return SITEHELPER_PERSISTENCE_MALFORMED_DATA; }
    if (count > SIZE_MAX / sizeof(SlabRegion)) { return SITEHELPER_PERSISTENCE_INVALID_PROJECT; }
    for (size_t i = 0; i < count; i++) {
        size_t vertex_count;
        int top_level, thickness;
        if (expect_token(file,"region") != SITEHELPER_PERSISTENCE_SUCCESS ||
            expect_token(file,"top_level_offset") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&top_level) ||
            expect_token(file,"thickness") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&thickness) ||
            expect_token(file,"outline") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token,&vertex_count)) { return SITEHELPER_PERSISTENCE_MALFORMED_DATA; }
        if (thickness <= 0 || vertex_count < 3 || vertex_count > SIZE_MAX / sizeof(PlanPosition)) {
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        PlanPosition *vertices = malloc(vertex_count * sizeof *vertices);
        if (vertices == NULL) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        SiteHelperPersistenceResult result = SITEHELPER_PERSISTENCE_SUCCESS;
        for (size_t j = 0; j < vertex_count; j++) {
            if (expect_token(file,"vertex") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token,&vertices[j].x) ||
                read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token,&vertices[j].y)) {
                result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                break;
            }
        }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS) { result = expect_token(file,"end_region"); }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS) {
            SlabCode code = slab_add_region(slab,vertices,vertex_count,top_level,thickness);
            if (code != SLAB_SUCCESS) {
                result = code == SLAB_ALLOCATION_FAILED ? SITEHELPER_PERSISTENCE_ALLOCATION_FAILED : SITEHELPER_PERSISTENCE_INVALID_PROJECT;
            }
        }
        free(vertices);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_edge_rebates(FILE *file, Slab *slab)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file,"edge_rebates") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token,&count)) { return SITEHELPER_PERSISTENCE_MALFORMED_DATA; }
    if (count > SIZE_MAX / sizeof(SlabEdgeRebate)) { return SITEHELPER_PERSISTENCE_INVALID_PROJECT; }
    for (size_t i = 0; i < count; i++) {
        size_t edge_index;
        int start, end, width, depth;
        if (expect_token(file,"edge_rebate") != SITEHELPER_PERSISTENCE_SUCCESS ||
            expect_token(file,"edge") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token,&edge_index) ||
            expect_token(file,"start_offset") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&start) ||
            expect_token(file,"end_offset") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&end) ||
            expect_token(file,"width") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&width) ||
            expect_token(file,"depth") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&depth) ||
            expect_token(file,"end_edge_rebate") != SITEHELPER_PERSISTENCE_SUCCESS) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        SlabCode code = slab_add_edge_rebate(slab,edge_index,start,end,width,depth);
        if (code != SLAB_SUCCESS) {
            return code == SLAB_ALLOCATION_FAILED ? SITEHELPER_PERSISTENCE_ALLOCATION_FAILED :
                SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_slabs(FILE *file, SiteHelperProject *project, Storey *storey, uintmax_t version)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file, "slabs") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &count)) { return SITEHELPER_PERSISTENCE_MALFORMED_DATA; }
    if (count > SIZE_MAX / sizeof(Slab)) { return SITEHELPER_PERSISTENCE_INVALID_PROJECT; }
    for (size_t i = 0; i < count; i++) {
        Slab slab = {0};
        SlabOutline *outline = &slab.definition.outline;
        if (expect_token(file, "slab") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &slab.id) || slab.id == DOMAIN_ID_INVALID ||
            project_contains_id(project, slab.id) ||
            expect_token(file, "top_level_offset") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &slab.definition.top_level_offset_mm) ||
            expect_token(file, "thickness") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &slab.definition.thickness_mm) ||
            expect_token(file, "outline") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &outline->vertex_count)) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (outline->vertex_count < 3 || outline->vertex_count > SIZE_MAX / sizeof *outline->vertices ||
            slab.definition.thickness_mm <= 0) { return SITEHELPER_PERSISTENCE_INVALID_PROJECT; }
        outline->vertex_capacity = outline->vertex_count;
        outline->vertices = malloc(outline->vertex_count * sizeof *outline->vertices);
        if (outline->vertices == NULL) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        SiteHelperPersistenceResult result = SITEHELPER_PERSISTENCE_SUCCESS;
        for (size_t j = 0; j < outline->vertex_count; j++) {
            if (expect_token(file, "vertex") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &outline->vertices[j].x) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &outline->vertices[j].y)) {
                result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                break;
            }
        }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS && version >= 12) { result = parse_penetrations(file,&slab); }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS && version >= 13) { result = parse_regions(file,&slab); }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS && version >= 14) { result = parse_edge_rebates(file,&slab); }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS) { result = expect_token(file, "end_slab"); }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS && slab_validate(&slab) != SLAB_SUCCESS) {
            result = SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
            !sitehelper_project_insert_slab(project, storey->id, &slab)) {
            result = SITEHELPER_PERSISTENCE_ALLOCATION_FAILED;
        }
        slab_destroy(&slab);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_roofs(
    FILE *file,
    SiteHelperProject *project,
    Storey *storey
)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t roof_count;
    if (expect_token(file, "roofs") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &roof_count) || roof_count > SIZE_MAX / sizeof(Roof)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    for (size_t roof_index = 0; roof_index < roof_count; roof_index++) {
        Roof roof = {0};
        size_t portion_count;
        if (expect_token(file, "roof") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &roof.id) || roof.id == DOMAIN_ID_INVALID ||
            project_contains_id(project, roof.id) ||
            expect_token(file, "portions") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &portion_count) || portion_count == 0 ||
            portion_count > SIZE_MAX / sizeof(RoofPortionDefinition)) {
            roof_destroy(&roof);
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }

        SiteHelperPersistenceResult result = SITEHELPER_PERSISTENCE_SUCCESS;
        for (size_t portion_index = 0; portion_index < portion_count; portion_index++) {
            DomainId portion_id;
            RoofPortionGeneration generation;
            int64_t slope_ppm;
            int reference_z_mm;
            RoofDirection direction;
            RoofSingleSlopeReference single_slope_reference;
            size_t vertex_count;

            if (expect_token(file, "portion") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_domain_id_token(token, &portion_id) || portion_id == DOMAIN_ID_INVALID ||
                portion_id == roof.id || project_contains_id(project, portion_id) ||
                expect_token(file, "generation") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_roof_generation(token, &generation) ||
                expect_token(file, "slope_ppm") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int64_token(token, &slope_ppm) ||
                expect_token(file, "reference_z") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &reference_z_mm) ||
                expect_token(file, "direction") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &direction.x) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &direction.y) ||
                expect_token(file, "single_slope_reference") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_roof_single_slope_reference(token, &single_slope_reference) ||
                expect_token(file, "support") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_size_token(token, &vertex_count)) {
                result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                break;
            }
            if (vertex_count < 3 || vertex_count > SIZE_MAX / sizeof(PlanPosition)) {
                result = SITEHELPER_PERSISTENCE_INVALID_PROJECT;
                break;
            }

            PlanPosition *vertices = malloc(vertex_count * sizeof *vertices);
            if (vertices == NULL) {
                result = SITEHELPER_PERSISTENCE_ALLOCATION_FAILED;
                break;
            }
            for (size_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
                if (expect_token(file, "vertex") != SITEHELPER_PERSISTENCE_SUCCESS ||
                    read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                    !parse_int_token(token, &vertices[vertex_index].x) ||
                    read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                    !parse_int_token(token, &vertices[vertex_index].y)) {
                    result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                    break;
                }
            }
            if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
                expect_token(file, "end_portion") != SITEHELPER_PERSISTENCE_SUCCESS) {
                result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            }
            if (result == SITEHELPER_PERSISTENCE_SUCCESS) {
                RoofPortionSpec spec = {
                    vertices, vertex_count, generation, slope_ppm, reference_z_mm,
                    direction, single_slope_reference
                };
                RoofCode code = roof_definition_append_portion(&roof.definition, portion_id, &spec);
                if (code != ROOF_SUCCESS) {
                    result = code == ROOF_ALLOCATION_FAILED
                        ? SITEHELPER_PERSISTENCE_ALLOCATION_FAILED
                        : SITEHELPER_PERSISTENCE_INVALID_PROJECT;
                }
            }
            free(vertices);
            if (result != SITEHELPER_PERSISTENCE_SUCCESS) { break; }
        }

        size_t composition_count = 0;
        if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
            (expect_token(file, "compositions") != SITEHELPER_PERSISTENCE_SUCCESS ||
             read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
             !parse_size_token(token, &composition_count) ||
             composition_count > SIZE_MAX / sizeof(RoofComposition))) {
            result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        for (size_t i = 0; result == SITEHELPER_PERSISTENCE_SUCCESS && i < composition_count; i++) {
            RoofComposition composition;
            if (expect_token(file, "composition") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_domain_id_token(token, &composition.first_portion_id) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_domain_id_token(token, &composition.second_portion_id) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_roof_composition_kind(token, &composition.kind)) {
                result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                break;
            }
            RoofCode code = roof_definition_append_composition(&roof.definition, composition);
            if (code != ROOF_SUCCESS) {
                result = code == ROOF_ALLOCATION_FAILED
                    ? SITEHELPER_PERSISTENCE_ALLOCATION_FAILED
                    : SITEHELPER_PERSISTENCE_INVALID_PROJECT;
            }
        }

        size_t termination_count = 0;
        if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
            (expect_token(file, "terminations") != SITEHELPER_PERSISTENCE_SUCCESS ||
             read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
             !parse_size_token(token, &termination_count) ||
             termination_count > SIZE_MAX / sizeof(RoofTermination))) {
            result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        for (size_t i = 0; result == SITEHELPER_PERSISTENCE_SUCCESS && i < termination_count; i++) {
            RoofTermination termination;
            if (expect_token(file, "termination") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_domain_id_token(token, &termination.portion_id) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_roof_end(token, &termination.end) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &termination.termination_offset_mm)) {
                result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                break;
            }
            RoofCode code = roof_definition_append_termination(&roof.definition, termination);
            if (code != ROOF_SUCCESS) {
                result = code == ROOF_ALLOCATION_FAILED
                    ? SITEHELPER_PERSISTENCE_ALLOCATION_FAILED
                    : SITEHELPER_PERSISTENCE_INVALID_PROJECT;
            }
        }

        if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
            expect_token(file, "end_roof") != SITEHELPER_PERSISTENCE_SUCCESS) {
            result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS && roof_validate(&roof) != ROOF_SUCCESS) {
            result = SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
            !sitehelper_project_insert_roof_at(project, storey->id, &roof, storey->roofs.count)) {
            result = SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        roof_destroy(&roof);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static int hex_value(int character)
{
    if (character >= '0' && character <= '9') { return character - '0'; }
    if (character >= 'a' && character <= 'f') { return character - 'a' + 10; }
    if (character >= 'A' && character <= 'F') { return character - 'A' + 10; }
    return -1;
}

static SiteHelperPersistenceResult parse_annotations(FILE *file, SiteHelperProject *project)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file, "annotations") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &count) || count > SIZE_MAX / sizeof(DocumentAnnotation)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }

    for (size_t i = 0; i < count; i++) {
        DocumentAnnotation annotation = {.kind = DOCUMENT_ANNOTATION_NOTE};
        size_t text_length;
        if (expect_token(file, "annotation") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &annotation.id) || annotation.id == DOMAIN_ID_INVALID ||
            project_contains_id(project, annotation.id) ||
            expect_token(file, "note") != SITEHELPER_PERSISTENCE_SUCCESS ||
            expect_token(file, "storey") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &annotation.anchor.storey_id) ||
            expect_token(file, "position") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &annotation.anchor.position.x) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &annotation.anchor.position.y) ||
            expect_token(file, "target") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &annotation.target_id) ||
            expect_token(file, "text_hex") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &text_length) || text_length == 0 || text_length == SIZE_MAX) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }

        char *text = malloc(text_length + 1);
        if (text == NULL) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        int malformed = 0;
        for (size_t j = 0; j < text_length; j++) {
            int high = hex_value(fgetc(file));
            int low = hex_value(fgetc(file));
            if (high < 0 || low < 0) {
                malformed = 1;
                break;
            }
            unsigned char byte = (unsigned char)((high << 4) | low);
            if (byte == 0) {
                malformed = 1;
                break;
            }
            text[j] = (char)byte;
        }
        if (!malformed) { text[text_length] = '\0'; }
        annotation.text = text;
        if (malformed || expect_token(file, "end_annotation") != SITEHELPER_PERSISTENCE_SUCCESS) {
            free(text);
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (!sitehelper_project_insert_annotation(project, &annotation)) {
            free(text);
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        free(text);
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}


static SiteHelperPersistenceResult parse_dimension_reference(FILE *file,
    DocumentDimensionReference *reference)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    if (read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    if (strcmp(token, "fixed") == 0) {
        reference->kind = DOCUMENT_DIMENSION_FIXED_POINT;
        if (read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &reference->position.x) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &reference->position.y)) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        reference->target_id = DOMAIN_ID_INVALID;
        return SITEHELPER_PERSISTENCE_SUCCESS;
    }
    if (strcmp(token, "wall_start") == 0) reference->kind = DOCUMENT_DIMENSION_WALL_START;
    else if (strcmp(token, "wall_end") == 0) reference->kind = DOCUMENT_DIMENSION_WALL_END;
    else return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    if (read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_domain_id_token(token, &reference->target_id) ||
        reference->target_id == DOMAIN_ID_INVALID) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult parse_dimensions(FILE *file, SiteHelperProject *project)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file, "dimensions") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &count) || count > SIZE_MAX / sizeof(DocumentPlanDimension)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    for (size_t i = 0; i < count; i++) {
        DocumentPlanDimension dimension = {0};
        if (expect_token(file, "dimension") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &dimension.id) || dimension.id == DOMAIN_ID_INVALID ||
            project_contains_id(project, dimension.id) ||
            expect_token(file, "storey") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &dimension.storey_id) ||
            expect_token(file, "offset") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &dimension.offset_mm) ||
            expect_token(file, "first") != SITEHELPER_PERSISTENCE_SUCCESS) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        SiteHelperPersistenceResult result = parse_dimension_reference(file, &dimension.first);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
        if (expect_token(file, "second") != SITEHELPER_PERSISTENCE_SUCCESS) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        result = parse_dimension_reference(file, &dimension.second);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS ||
            expect_token(file, "end_dimension") != SITEHELPER_PERSISTENCE_SUCCESS) {
            return result == SITEHELPER_PERSISTENCE_SUCCESS ? SITEHELPER_PERSISTENCE_MALFORMED_DATA : result;
        }
        if (!sitehelper_project_insert_dimension(project, &dimension)) {
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}


static SiteHelperPersistenceResult parse_symbols(FILE *file, SiteHelperProject *project, int version)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file, "symbols") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &count) || count > SIZE_MAX / sizeof(DocumentPlanSymbol)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    for (size_t i = 0; i < count; i++) {
        DocumentPlanSymbol symbol = {0};
        if (expect_token(file, "symbol") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &symbol.id) || symbol.id == DOMAIN_ID_INVALID ||
            project_contains_id(project, symbol.id) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (strcmp(token, "point_marker") == 0) {
            symbol.kind=DOCUMENT_PLAN_SYMBOL_POINT_MARKER;
        } else if (version >= 20 && strcmp(token, "view_direction") == 0) {
            symbol.kind=DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION;
        } else {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (expect_token(file, "storey") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &symbol.storey_id) ||
            expect_token(file, "position") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &symbol.anchor.x) ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token, &symbol.anchor.y)) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (symbol.kind == DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION) {
            if (expect_token(file, "direction") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &symbol.direction.dx) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &symbol.direction.dy)) {
                return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            }
        }
        if (expect_token(file, "end_symbol") != SITEHELPER_PERSISTENCE_SUCCESS) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (!sitehelper_project_insert_symbol(project, &symbol)) {
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}


static SiteHelperPersistenceResult parse_callouts(FILE *file, SiteHelperProject *project)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file,"callouts") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token,&count) || count > SIZE_MAX/sizeof(DocumentPlanCallout)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    for (size_t i=0;i<count;i++) {
        DocumentPlanCallout callout={0};
        size_t text_length;
        if (expect_token(file,"callout") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token,&callout.id) || callout.id == DOMAIN_ID_INVALID ||
            project_contains_id(project,callout.id) ||
            expect_token(file,"storey") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token,&callout.storey_id) ||
            expect_token(file,"target") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&callout.target.x) ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&callout.target.y) ||
            expect_token(file,"label") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&callout.label_anchor.x) ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_int_token(token,&callout.label_anchor.y) ||
            expect_token(file,"text_hex") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file,token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token,&text_length) || text_length == 0 || text_length == SIZE_MAX) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        char *text=malloc(text_length+1);
        if (text == NULL) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        int malformed=0;
        for (size_t j=0;j<text_length;j++) {
            int high=hex_value(fgetc(file));
            int low=hex_value(fgetc(file));
            if (high < 0 || low < 0) { malformed=1; break; }
            unsigned char byte=(unsigned char)((high<<4)|low);
            if (byte == 0) { malformed=1; break; }
            text[j]=(char)byte;
        }
        if (!malformed) { text[text_length]='\0'; }
        callout.text=text;
        if (malformed || expect_token(file,"end_callout") != SITEHELPER_PERSISTENCE_SUCCESS) {
            free(text); return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (!sitehelper_project_insert_callout(project,&callout)) {
            free(text); return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        free(text);
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}


static SiteHelperPersistenceResult parse_revisions(FILE *file, SiteHelperProject *project)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file, "revisions") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &count) || count > SIZE_MAX / sizeof(DocumentRevision)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    for (size_t i = 0; i < count; i++) {
        DocumentRevision revision = {0};
        size_t identifier_length, description_length;
        if (expect_token(file, "revision") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &revision.id) || revision.id == DOMAIN_ID_INVALID ||
            project_contains_id(project, revision.id) ||
            expect_token(file, "identifier_hex") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &identifier_length) || identifier_length == 0 ||
            identifier_length == SIZE_MAX) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        revision.identifier = malloc(identifier_length + 1);
        if (revision.identifier == NULL) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        int malformed = 0;
        for (size_t j = 0; j < identifier_length; j++) {
            int high = hex_value(fgetc(file));
            int low = hex_value(fgetc(file));
            if (high < 0 || low < 0) { malformed = 1; break; }
            unsigned char byte = (unsigned char)((high << 4) | low);
            if (byte == 0) { malformed = 1; break; }
            revision.identifier[j] = (char)byte;
        }
        if (!malformed) { revision.identifier[identifier_length] = '\0'; }
        if (malformed || expect_token(file, "description_hex") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &description_length) || description_length == SIZE_MAX) {
            document_revision_destroy(&revision);
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        revision.description = malloc(description_length + 1);
        if (revision.description == NULL) {
            document_revision_destroy(&revision);
            return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED;
        }
        for (size_t j = 0; j < description_length; j++) {
            int high = hex_value(fgetc(file));
            int low = hex_value(fgetc(file));
            if (high < 0 || low < 0) { malformed = 1; break; }
            unsigned char byte = (unsigned char)((high << 4) | low);
            if (byte == 0) { malformed = 1; break; }
            revision.description[j] = (char)byte;
        }
        if (!malformed) { revision.description[description_length] = '\0'; }
        if (malformed || expect_token(file, "end_revision") != SITEHELPER_PERSISTENCE_SUCCESS) {
            document_revision_destroy(&revision);
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (!sitehelper_project_insert_revision(project, &revision)) {
            document_revision_destroy(&revision);
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        document_revision_destroy(&revision);
    }
    return SITEHELPER_PERSISTENCE_SUCCESS;
}


static SiteHelperPersistenceResult parse_revision_clouds(
    FILE *file, SiteHelperProject *project, int version)
{
    char token[PERSISTENCE_TOKEN_CAPACITY];
    size_t count;
    if (expect_token(file, "revision_clouds") != SITEHELPER_PERSISTENCE_SUCCESS ||
        read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
        !parse_size_token(token, &count) ||
        count > SIZE_MAX / sizeof(DocumentPlanRevisionCloud)) {
        return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
    }
    for (size_t i = 0; i < count; i++) {
        DocumentPlanRevisionCloud cloud = {0};
        size_t vertex_count;
        if (expect_token(file, "revision_cloud") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &cloud.id) ||
            cloud.id == DOMAIN_ID_INVALID || project_contains_id(project, cloud.id) ||
            expect_token(file, "storey") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_domain_id_token(token, &cloud.storey_id)) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (version >= 22) {
            if (expect_token(file, "revision") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_domain_id_token(token, &cloud.revision_id)) {
                return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            }
        }
        if (expect_token(file, "vertices") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &vertex_count) || vertex_count < 3 ||
            vertex_count > SIZE_MAX / sizeof *cloud.vertices) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        cloud.vertices = malloc(vertex_count * sizeof *cloud.vertices);
        if (cloud.vertices == NULL) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        cloud.vertex_count = vertex_count;
        SiteHelperPersistenceResult result = SITEHELPER_PERSISTENCE_SUCCESS;
        for (size_t v = 0; v < vertex_count; v++) {
            if (expect_token(file, "vertex") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &cloud.vertices[v].x) ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &cloud.vertices[v].y)) {
                result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                break;
            }
        }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
            expect_token(file, "end_revision_cloud") != SITEHELPER_PERSISTENCE_SUCCESS) {
            result = SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
        if (result == SITEHELPER_PERSISTENCE_SUCCESS &&
            !sitehelper_project_insert_revision_cloud(project, &cloud)) {
            result = SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        free(cloud.vertices);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
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

    size_t storey_count = 1;
    if (version >= 8) {
        if (expect_token(file, "storeys") != SITEHELPER_PERSISTENCE_SUCCESS ||
            read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
            !parse_size_token(token, &storey_count)) { return SITEHELPER_PERSISTENCE_MALFORMED_DATA; }
    } else {
        /* Parser-only placeholder: no legacy identity is assigned or remapped.
         * Its ID is filled after all legacy definitions have been parsed. */
        project->storeys = calloc(1, sizeof *project->storeys);
        if (project->storeys == NULL) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        project->storey_count = project->storey_capacity = 1;
    }
    for (size_t i = 0; i < storey_count; i++) {
        if (version >= 8) {
            DomainId id;
            int elevation;
            if (expect_token(file, "storey") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_domain_id_token(token, &id) || id == DOMAIN_ID_INVALID || project_contains_id(project, id) ||
                expect_token(file, "elevation") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                !parse_int_token(token, &elevation)) { return SITEHELPER_PERSISTENCE_MALFORMED_DATA; }
            if (!sitehelper_project_insert_storey(project, id, elevation)) { return SITEHELPER_PERSISTENCE_ALLOCATION_FAILED; }
        }
        Storey *storey = &project->storeys[i];
        if (version >= 9) {
            if (expect_token(file, "stud_height") != SITEHELPER_PERSISTENCE_SUCCESS ||
                read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS) {
                return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            }
            if (strcmp(token, "override") == 0) {
                storey->settings.has_stud_height_override = true;
                if (read_required_token(file, token) != SITEHELPER_PERSISTENCE_SUCCESS ||
                    !parse_int_token(token, &storey->settings.stud_height)) {
                    return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
                }
            } else if (strcmp(token, "inherit") != 0) {
                return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
            }
        }
        BuildSettings resolved;
        /* Resolve staged parser values through the same rule. Legacy Storeys
         * still have a temporary zero ID here, and inherit without an override. */
        if (!project_resolve_build_settings(&project->settings, &storey->settings, &resolved)) {
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        result = parse_structure(file, project, &storey->structure, version, &resolved);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
        if (version >= 11) {
            result = parse_slabs(file, project, storey, version);
            if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
        }
        if (version >= 15) {
            result = parse_roofs(file, project, storey);
            if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
        }
        if (version >= 8 && expect_token(file, "end_storey") != SITEHELPER_PERSISTENCE_SUCCESS) {
            return SITEHELPER_PERSISTENCE_MALFORMED_DATA;
        }
    }
    if (version >= 16) {
        result = parse_annotations(file, project);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    if (version >= 17) {
        result = parse_dimensions(file, project);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    if (version >= 18) {
        result = parse_symbols(file, project, version);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    if (version >= 19) {
        result = parse_callouts(file, project);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    if (version >= 22) {
        result = parse_revisions(file, project);
        if (result != SITEHELPER_PERSISTENCE_SUCCESS) { return result; }
    }
    if (version >= 21) {
        result = parse_revision_clouds(file, project, version);
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
    if (version < 8) {
        DomainIdGenerator ids = project->domain_ids;
        DomainId storey_id = domain_id_generate(&ids);
        if (storey_id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
            project_contains_id(project, storey_id)) { return SITEHELPER_PERSISTENCE_INVALID_PROJECT; }
        project->storeys[0].id = storey_id;
        project->domain_ids = ids;
        /* Validation below rejects legacy IDs above the original watermark:
         * they are >= the advanced watermark. No IDs are stolen or wrapped. */
    }

    return SITEHELPER_PERSISTENCE_SUCCESS;
}

static SiteHelperPersistenceResult regenerate_project(
    SiteHelperProject *project
)
{
    if (project == NULL) {
        return SITEHELPER_PERSISTENCE_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < project->storey_count; i++) {
        BuildStructure *structure = &project->storeys[i].structure;
        BuildSettings resolved;
        if (!sitehelper_project_resolve_storey_build_settings(project, project->storeys[i].id, &resolved)) {
            return SITEHELPER_PERSISTENCE_INVALID_PROJECT;
        }
        for (size_t wall_index = 0;
             wall_index < structure->wall_count;
             wall_index++) {

            if (!wall_generate(
                    &structure->walls[wall_index],
                    &resolved)) {

                return SITEHELPER_PERSISTENCE_REGENERATION_FAILED;
            }
        }
        for (size_t roof_index = 0; roof_index < project->storeys[i].roofs.count; roof_index++) {
            RoofPrototypeGeometry geometry = {0};
            RoofCode code = roof_build_derived_geometry(
                &project->storeys[i].roofs.items[roof_index], &geometry);
            roof_prototype_geometry_destroy(&geometry);
            if (code != ROOF_SUCCESS) {
                return SITEHELPER_PERSISTENCE_REGENERATION_FAILED;
            }
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
            "storeys %zu\n",
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
            project->storey_count) < 0) {

        return 0;
    }

    for (size_t i = 0; i < project->storey_count; i++) {
        const Storey *storey = &project->storeys[i];
        const BuildStructure *structure = &storey->structure;
        if (fprintf(file, "storey %" PRIu64 " elevation %d\nstud_height ",
                (uint64_t)storey->id, storey->elevation_mm) < 0) { return 0; }
        if (storey->settings.has_stud_height_override) {
            if (fprintf(file, "override %d\n", storey->settings.stud_height) < 0) { return 0; }
        } else if (fputs("inherit\n", file) == EOF) { return 0; }
        if (fprintf(file, "walls %zu\n", structure->wall_count) < 0) { return 0; }
        for (size_t wall_index = 0;
             wall_index < structure->wall_count;
             wall_index++) {

                const Wall *wall = &structure->walls[wall_index];

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

        if (fprintf(file, "rooms %zu\n", structure->room_count) < 0) {
            return 0;
        }

        for (size_t room_index = 0;
             room_index < structure->room_count;
             room_index++) {

            const Room *room = &structure->rooms[room_index];
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

        if (fprintf(file, "room_separators %zu\n", structure->room_separator_count) < 0) { return 0; }
        for (size_t i = 0; i < structure->room_separator_count; i++) {
            const RoomSeparator *separator = &structure->room_separators[i];
            if (fprintf(file, "room_separator %" PRIu64 " segment %d %d %d %d\n",
                    (uint64_t)separator->id, separator->segment.start.x, separator->segment.start.y,
                    separator->segment.end.x, separator->segment.end.y) < 0) { return 0; }
        }
        if (fprintf(file, "slabs %zu\n", storey->slabs.count) < 0) { return 0; }
        for (size_t i = 0; i < storey->slabs.count; i++) {
            const Slab *slab = &storey->slabs.items[i];
            const SlabDefinition *d = &slab->definition;
            if (fprintf(file, "slab %" PRIu64 " top_level_offset %d thickness %d outline %zu\n",
                    (uint64_t)slab->id, d->top_level_offset_mm, d->thickness_mm, d->outline.vertex_count) < 0) { return 0; }
            for (size_t j = 0; j < d->outline.vertex_count; j++) {
                if (fprintf(file, "vertex %d %d\n", d->outline.vertices[j].x, d->outline.vertices[j].y) < 0) { return 0; }
            }
            if (fprintf(file,"penetrations %zu\n",d->penetrations.count) < 0) { return 0; }
            for (size_t j = 0; j < d->penetrations.count; j++) {
                const SlabOutline *o = &d->penetrations.items[j].outline;
                if (fprintf(file,"penetration outline %zu\n",o->vertex_count) < 0) { return 0; }
                for (size_t k = 0; k < o->vertex_count; k++) {
                    if (fprintf(file,"vertex %d %d\n",o->vertices[k].x,o->vertices[k].y) < 0) { return 0; }
                }
                if (fputs("end_penetration\n",file) == EOF) { return 0; }
            }
            if (fprintf(file,"regions %zu\n",d->regions.count) < 0) { return 0; }
            for (size_t j = 0; j < d->regions.count; j++) {
                const SlabRegion *r = &d->regions.items[j];
                if (fprintf(file,"region top_level_offset %d thickness %d outline %zu\n",
                        r->top_level_offset_mm,r->thickness_mm,r->outline.vertex_count) < 0) { return 0; }
                for (size_t k = 0; k < r->outline.vertex_count; k++) {
                    if (fprintf(file,"vertex %d %d\n",r->outline.vertices[k].x,r->outline.vertices[k].y) < 0) { return 0; }
                }
                if (fputs("end_region\n",file) == EOF) { return 0; }
            }
            if (fprintf(file,"edge_rebates %zu\n",d->edge_rebates.count) < 0) { return 0; }
            for (size_t j = 0; j < d->edge_rebates.count; j++) {
                const SlabEdgeRebate *r=&d->edge_rebates.items[j];
                if (fprintf(file,"edge_rebate edge %zu start_offset %d end_offset %d width %d depth %d\n"
                        "end_edge_rebate\n",r->edge_index,r->start_offset_mm,r->end_offset_mm,
                        r->width_mm,r->depth_mm) < 0) { return 0; }
            }
            if (fputs("end_slab\n", file) == EOF) { return 0; }
        }
        if (fprintf(file, "roofs %zu\n", storey->roofs.count) < 0) { return 0; }
        for (size_t roof_index = 0; roof_index < storey->roofs.count; roof_index++) {
            const Roof *roof = &storey->roofs.items[roof_index];
            if (fprintf(file, "roof %" PRIu64 " portions %zu\n",
                    (uint64_t)roof->id, roof->definition.portion_count) < 0) { return 0; }
            for (size_t portion_index = 0;
                 portion_index < roof->definition.portion_count;
                 portion_index++) {
                const RoofPortionDefinition *portion = &roof->definition.portions[portion_index];
                const char *generation = roof_generation_token(portion->generation);
                const char *reference = roof_single_slope_reference_token(
                    portion->single_slope_reference);
                if (generation == NULL || reference == NULL ||
                    fprintf(file,
                        "portion %" PRIu64 " generation %s slope_ppm %" PRId64
                        " reference_z %d direction %d %d single_slope_reference %s support %zu\n",
                        (uint64_t)portion->id, generation, portion->slope_ppm,
                        portion->reference_z_mm, portion->direction.x, portion->direction.y,
                        reference, portion->support_vertex_count) < 0) { return 0; }
                for (size_t vertex_index = 0;
                     vertex_index < portion->support_vertex_count;
                     vertex_index++) {
                    if (fprintf(file, "vertex %d %d\n",
                            portion->support_vertices[vertex_index].x,
                            portion->support_vertices[vertex_index].y) < 0) { return 0; }
                }
                if (fputs("end_portion\n", file) == EOF) { return 0; }
            }
            if (fprintf(file, "compositions %zu\n", roof->definition.composition_count) < 0) {
                return 0;
            }
            for (size_t composition_index = 0;
                 composition_index < roof->definition.composition_count;
                 composition_index++) {
                const RoofComposition *composition =
                    &roof->definition.compositions[composition_index];
                const char *kind = roof_composition_kind_token(composition->kind);
                if (kind == NULL || fprintf(file, "composition %" PRIu64 " %" PRIu64 " %s\n",
                        (uint64_t)composition->first_portion_id,
                        (uint64_t)composition->second_portion_id, kind) < 0) { return 0; }
            }
            if (fprintf(file, "terminations %zu\n", roof->definition.termination_count) < 0) {
                return 0;
            }
            for (size_t termination_index = 0;
                 termination_index < roof->definition.termination_count;
                 termination_index++) {
                const RoofTermination *termination =
                    &roof->definition.terminations[termination_index];
                const char *end = roof_end_token(termination->end);
                if (end == NULL || fprintf(file, "termination %" PRIu64 " %s %d\n",
                        (uint64_t)termination->portion_id, end,
                        termination->termination_offset_mm) < 0) { return 0; }
            }
            if (fputs("end_roof\n", file) == EOF) { return 0; }
        }
        if (fputs("end_storey\n", file) == EOF) { return 0; }
    }
    if (fprintf(file, "annotations %zu\n", project->document.annotation_count) < 0) {
        return 0;
    }
    for (size_t i = 0; i < project->document.annotation_count; i++) {
        const DocumentAnnotation *annotation = &project->document.annotations[i];
        size_t text_length = strlen(annotation->text);
        if (fprintf(file,
                "annotation %" PRIu64 " note storey %" PRIu64
                " position %d %d target %" PRIu64 " text_hex %zu\n",
                (uint64_t)annotation->id, (uint64_t)annotation->anchor.storey_id,
                annotation->anchor.position.x, annotation->anchor.position.y,
                (uint64_t)annotation->target_id, text_length) < 0) {
            return 0;
        }
        for (size_t j = 0; j < text_length; j++) {
            if (fprintf(file, "%02x", (unsigned char)annotation->text[j]) < 0) {
                return 0;
            }
        }
        if (fputs("\nend_annotation\n", file) == EOF) { return 0; }
    }
    if (fprintf(file, "dimensions %zu\n", project->document.dimension_count) < 0) { return 0; }
    for (size_t i = 0; i < project->document.dimension_count; i++) {
        const DocumentPlanDimension *dimension = &project->document.dimensions[i];
        if (fprintf(file, "dimension %" PRIu64 " storey %" PRIu64 " offset %d\n",
                (uint64_t)dimension->id, (uint64_t)dimension->storey_id,
                dimension->offset_mm) < 0) { return 0; }
        const DocumentDimensionReference refs[2] = {dimension->first, dimension->second};
        const char *labels[2] = {"first", "second"};
        for (size_t r = 0; r < 2; r++) {
            if (refs[r].kind == DOCUMENT_DIMENSION_FIXED_POINT) {
                if (fprintf(file, "%s fixed %d %d\n", labels[r],
                        refs[r].position.x, refs[r].position.y) < 0) { return 0; }
            } else {
                const char *kind = refs[r].kind == DOCUMENT_DIMENSION_WALL_START
                    ? "wall_start" : refs[r].kind == DOCUMENT_DIMENSION_WALL_END
                    ? "wall_end" : NULL;
                if (kind == NULL || fprintf(file, "%s %s %" PRIu64 "\n", labels[r], kind,
                        (uint64_t)refs[r].target_id) < 0) { return 0; }
            }
        }
        if (fputs("end_dimension\n", file) == EOF) { return 0; }
    }
    if (fprintf(file, "symbols %zu\n", project->document.symbol_count) < 0) { return 0; }
    for (size_t i = 0; i < project->document.symbol_count; i++) {
        const DocumentPlanSymbol *symbol = &project->document.symbols[i];
        if (symbol->kind == DOCUMENT_PLAN_SYMBOL_POINT_MARKER) {
            if (fprintf(file, "symbol %" PRIu64 " point_marker storey %" PRIu64
                    " position %d %d\nend_symbol\n",
                    (uint64_t)symbol->id, (uint64_t)symbol->storey_id,
                    symbol->anchor.x, symbol->anchor.y) < 0) { return 0; }
        } else if (symbol->kind == DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION) {
            if (fprintf(file, "symbol %" PRIu64 " view_direction storey %" PRIu64
                    " position %d %d direction %d %d\nend_symbol\n",
                    (uint64_t)symbol->id, (uint64_t)symbol->storey_id,
                    symbol->anchor.x, symbol->anchor.y,
                    symbol->direction.dx, symbol->direction.dy) < 0) { return 0; }
        } else {
            return 0;
        }
    }
    if (fprintf(file,"callouts %zu\n",project->document.callout_count) < 0) { return 0; }
    for (size_t i=0;i<project->document.callout_count;i++) {
        const DocumentPlanCallout *callout=&project->document.callouts[i];
        size_t text_length=strlen(callout->text);
        if (fprintf(file,"callout %" PRIu64 " storey %" PRIu64
                " target %d %d label %d %d text_hex %zu\n",
                (uint64_t)callout->id,(uint64_t)callout->storey_id,
                callout->target.x,callout->target.y,
                callout->label_anchor.x,callout->label_anchor.y,text_length) < 0) { return 0; }
        for (size_t j=0;j<text_length;j++) {
            if (fprintf(file,"%02x",(unsigned char)callout->text[j]) < 0) { return 0; }
        }
        if (fputs("\nend_callout\n",file) == EOF) { return 0; }
    }
    if (fprintf(file, "revisions %zu\n", project->document.revision_count) < 0) {
        return 0;
    }
    for (size_t i = 0; i < project->document.revision_count; i++) {
        const DocumentRevision *revision = &project->document.revisions[i];
        size_t identifier_length = strlen(revision->identifier);
        size_t description_length = strlen(revision->description);
        if (fprintf(file, "revision %" PRIu64 " identifier_hex %zu\n",
                (uint64_t)revision->id, identifier_length) < 0) { return 0; }
        for (size_t j = 0; j < identifier_length; j++) {
            if (fprintf(file, "%02x", (unsigned char)revision->identifier[j]) < 0) {
                return 0;
            }
        }
        if (fprintf(file, "\ndescription_hex %zu\n", description_length) < 0) { return 0; }
        for (size_t j = 0; j < description_length; j++) {
            if (fprintf(file, "%02x", (unsigned char)revision->description[j]) < 0) {
                return 0;
            }
        }
        if (fputs("\nend_revision\n", file) == EOF) { return 0; }
    }
    if (fprintf(file, "revision_clouds %zu\n",
            project->document.revision_cloud_count) < 0) {
        return 0;
    }
    for (size_t i = 0; i < project->document.revision_cloud_count; i++) {
        const DocumentPlanRevisionCloud *cloud = &project->document.revision_clouds[i];
        if (fprintf(file, "revision_cloud %" PRIu64 " storey %" PRIu64
                " revision %" PRIu64 " vertices %zu\n",
                (uint64_t)cloud->id, (uint64_t)cloud->storey_id,
                (uint64_t)cloud->revision_id, cloud->vertex_count) < 0) {
            return 0;
        }
        for (size_t v = 0; v < cloud->vertex_count; v++) {
            if (fprintf(file, "vertex %d %d\n",
                    cloud->vertices[v].x, cloud->vertices[v].y) < 0) {
                return 0;
            }
        }
        if (fputs("end_revision_cloud\n", file) == EOF) { return 0; }
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
