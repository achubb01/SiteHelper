#ifndef CUT_LIST_H
#define CUT_LIST_H

#include <stdint.h>
#include "sitehelper_project.h"

/* Required pieces, not available stock or individually identified members. */
typedef struct {
    DomainId source_wall_id;
    TimberType type;
    /* Same convention as FramingTakeoff: canonical STUD_COMMON means not
     * applicable for non-studs and does not distinguish their requirements. */
    StudType stud_type;
    int length_mm, depth_mm, width_mm;
    uint64_t quantity;
} RequiredMember;

/* Exclusively owned values, with no identity or borrowed source pointers.
 * Stored Storey order -> stored Wall order -> Priority 21 item order (type,
 * stud subtype where applicable, length, depth, width). Identical requirements
 * aggregate only within a Wall. This is source order, not a cutting sequence.
 * Do not shallow-copy this structure into another owner. */
typedef struct {
    RequiredMember *members;
    size_t member_count;
} CutList;

typedef enum {
    CUT_LIST_SUCCESS = 0,
    CUT_LIST_INVALID_ARGUMENT,
    CUT_LIST_INVALID_SOURCE, /* Collection metadata or zero source Wall ID. */
    CUT_LIST_INVALID_FRAMING,
    CUT_LIST_ALLOCATION_FAILED,
    CUT_LIST_NUMERIC_OVERFLOW
} CutListCode;

typedef struct {
    CutListCode code;
    DomainId wall_id; /* Offending Wall when known; zero otherwise. */
} CutListResult;

/* Read-only conversion of a separate wall-level FramingTakeoff for each Wall.
 * No construction rules or framing validation are duplicated here. Empty
 * Storey/Project succeeds; an identified Wall without usable framing fails.
 * Wall IDs must be nonzero and globally unique (uniqueness is a model contract,
 * not revalidated here). Input collections must have valid disjoint allocations
 * and must not be mutated concurrently. This is not whole-project validation
 * or a check that committed framing is fresh relative to definitions/settings.
 *
 * Output must be zero-initialized or a previous successful result, independent
 * of input. Success replaces it; every failure leaves it completely unchanged.
 * Check status: a retained older snapshot still describes older construction.
 * Owned values can outlive source regeneration or destruction. Not serialized. */
CutListResult cut_list_build_wall(const Wall *wall, CutList *output);
CutListResult cut_list_build_storey(const Storey *storey, CutList *output);
CutListResult cut_list_build_project(const SiteHelperProject *project, CutList *output);

/* Frees owned storage and zeros the result. NULL/zero/repeated calls safe.
 * Result pointers expire on successful rebuild or destruction. */
void cut_list_destroy(CutList *list);

#endif
