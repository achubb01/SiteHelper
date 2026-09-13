#ifndef LINEAR_CUT_OPTIMIZER_H
#define LINEAR_CUT_OPTIMIZER_H

#include <stddef.h>
#include <stdint.h>

/* Opaque compatibility key supplied by the caller. Equality alone determines
 * compatibility; dimensions, purpose and provenance do not define a class. */
typedef uint64_t LinearStockClassId;
#define LINEAR_STOCK_CLASS_INVALID ((LinearStockClassId)0)

typedef struct {
    LinearStockClassId stock_class_id;
    uint64_t reference; /* Opaque provenance; zero and duplicates are valid. */
    int length_mm;
    uint64_t quantity;
} LinearCutRequirement;

typedef struct {
    LinearStockClassId stock_class_id;
    uint64_t reference; /* Opaque catalogue reference; never interpreted. */
    int length_mm; /* This option can be used repeatedly; no inventory limit. */
} LinearStockOption;

typedef struct {
    size_t requirement_index; /* Original input index, not a persistent ID. */
    uint64_t requirement_reference;
    int length_mm;
} LinearPlannedCut;

typedef struct {
    size_t stock_option_index;
    uint64_t stock_reference;
    LinearStockClassId stock_class_id;
    int stock_length_mm;
    size_t first_cut, cut_count; /* Contiguous range in LinearCutPlan.cuts. */
    int used_piece_length_mm;
    int kerf_mm; /* Total packing allowance on this bar, not per adjacency. */
    int remainder_mm;
} LinearPlannedStock;

/* Exclusively owned derived snapshot. No borrowed input pointers or identity.
 * Stock follows bar-opening order; cuts within each bar follow placement order.
 * These are packing assignments, not physical saw-operation instructions.
 * For each bar: stock_length_mm = used_piece_length_mm + kerf_mm + remainder_mm.
 * Totals obey the same identity. Do not shallow-copy into another owner. */
typedef struct {
    LinearPlannedStock *stock;
    size_t stock_count;
    LinearPlannedCut *cuts;
    size_t cut_count;
    int64_t total_required_length_mm;
    int64_t total_stock_length_mm;
    int64_t total_kerf_mm;
    int64_t total_remainder_mm;
} LinearCutPlan;

typedef enum {
    LINEAR_CUT_SUCCESS = 0,
    LINEAR_CUT_INVALID_ARGUMENT,
    LINEAR_CUT_INVALID_REQUIREMENT,
    LINEAR_CUT_INVALID_STOCK_CATALOGUE,
    LINEAR_CUT_UNSATISFIED_REQUIREMENT,
    LINEAR_CUT_ALLOCATION_FAILED,
    LINEAR_CUT_NUMERIC_OVERFLOW
} LinearCutCode;

typedef struct {
    LinearCutCode code;
    size_t requirement_index; /* SIZE_MAX when not applicable/known. */
    size_t stock_option_index; /* SIZE_MAX when not applicable/known. */
    LinearStockClassId stock_class_id; /* Zero when not applicable/known. */
} LinearCutResult;

/* Deterministic best-fit decreasing heuristic; NOT guaranteed minimum waste.
 * Expand quantities, order by class ascending, length descending, original
 * requirement index then piece ordinal. Best fitting open compatible bar wins;
 * equal remainders choose the earliest opened bar. Otherwise open the shortest
 * compatible option that fits, breaking ties by original catalogue index.
 *
 * Classes, lengths and quantities must be positive; kerf_mm must be nonnegative.
 * Consumed length = sum(piece lengths) + kerf_mm * (piece count - 1).
 * One exact-length piece therefore fits even with positive kerf. This allowance
 * excludes trim cuts, machine motion and blade setup.
 *
 * NULL input arrays are valid only with zero respective counts. All supplied
 * entries are validated, including the catalogue for empty requirements.
 * Empty requirements succeed with an empty plan; an empty catalogue with
 * nonempty valid requirements is unsatisfied. Inputs are never mutated and
 * must remain valid/unmodified during the call. References are only copied.
 *
 * Output must be zero-initialized or a previous successful independent result.
 * Success replaces it; ANY failure preserves all previous pointers/contents.
 * The owned plan can outlive modification/destruction of input arrays.
 * Counts, allocation sizes and integer-millimetre totals use checked arithmetic.
 * Metadata checks cannot verify actual allocation sizes or pointer validity. */
LinearCutResult linear_cut_optimize(const LinearCutRequirement *requirements,
    size_t requirement_count, const LinearStockOption *options, size_t option_count,
    int kerf_mm, LinearCutPlan *output);

/* Free both arrays and zero the plan. NULL/zero/repeated calls are safe. */
void linear_cut_plan_destroy(LinearCutPlan *plan);

#endif
