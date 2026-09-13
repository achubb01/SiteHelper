#include <stdlib.h>
#include "linear_cut_optimizer.h"

typedef struct {
    LinearStockClassId stock_class_id;
    int length_mm;
    size_t requirement_index, ordinal, stock_index;
} Piece;

static LinearCutResult result(LinearCutCode code, size_t requirement_index,
    size_t stock_option_index, LinearStockClassId stock_class_id)
{
    return (LinearCutResult){code, requirement_index, stock_option_index, stock_class_id};
}

void linear_cut_plan_destroy(LinearCutPlan *plan)
{
    if (plan == NULL) { return; }
    free(plan->stock);
    free(plan->cuts);
    *plan = (LinearCutPlan){0};
}

static int piece_compare(const void *pa, const void *pb)
{
    const Piece *a = pa, *b = pb;
    if (a->stock_class_id != b->stock_class_id) {
        return a->stock_class_id < b->stock_class_id ? -1 : 1;
    }
    if (a->length_mm != b->length_mm) { return a->length_mm > b->length_mm ? -1 : 1; }
    if (a->requirement_index != b->requirement_index) {
        return a->requirement_index < b->requirement_index ? -1 : 1;
    }
    return (a->ordinal > b->ordinal) - (a->ordinal < b->ordinal);
}

static size_t best_open_stock(const LinearCutPlan *plan, const Piece *piece, int kerf_mm)
{
    size_t best = SIZE_MAX;
    int64_t consumed = (int64_t)piece->length_mm + kerf_mm;
    for (size_t i = 0; i < plan->stock_count; i++) {
        const LinearPlannedStock *bar = &plan->stock[i];
        if (bar->stock_class_id == piece->stock_class_id && bar->remainder_mm >= consumed &&
            (best == SIZE_MAX || bar->remainder_mm < plan->stock[best].remainder_mm)) {
            best = i;
        }
    }
    return best;
}

static size_t shortest_option(const LinearStockOption *options, size_t count, const Piece *piece)
{
    size_t best = SIZE_MAX;
    for (size_t i = 0; i < count; i++) {
        if (options[i].stock_class_id == piece->stock_class_id && options[i].length_mm >= piece->length_mm &&
            (best == SIZE_MAX || options[i].length_mm < options[best].length_mm)) {
            best = i;
        }
    }
    return best;
}

LinearCutResult linear_cut_optimize(const LinearCutRequirement *requirements,
    size_t requirement_count, const LinearStockOption *options, size_t option_count,
    int kerf_mm, LinearCutPlan *output)
{
    if (output == NULL || kerf_mm < 0 || (requirement_count != 0 && requirements == NULL) ||
        (option_count != 0 && options == NULL)) {
        return result(LINEAR_CUT_INVALID_ARGUMENT, SIZE_MAX, SIZE_MAX, 0);
    }
    if (requirement_count > SIZE_MAX / sizeof *requirements || option_count > SIZE_MAX / sizeof *options) {
        return result(LINEAR_CUT_NUMERIC_OVERFLOW, SIZE_MAX, SIZE_MAX, 0);
    }
    size_t count = 0;
    int64_t required_length = 0;
    /* Bound every array before expansion, including worst-case one bar/piece. */
    size_t maximum = SIZE_MAX / sizeof(Piece);
    if (maximum > SIZE_MAX / sizeof(LinearPlannedStock)) { maximum = SIZE_MAX / sizeof(LinearPlannedStock); }
    if (maximum > SIZE_MAX / sizeof(LinearPlannedCut)) { maximum = SIZE_MAX / sizeof(LinearPlannedCut); }
    for (size_t i = 0; i < requirement_count; i++) {
        const LinearCutRequirement *r = &requirements[i];
        if (r->stock_class_id == LINEAR_STOCK_CLASS_INVALID || r->length_mm <= 0 || r->quantity == 0) {
            return result(LINEAR_CUT_INVALID_REQUIREMENT, i, SIZE_MAX, r->stock_class_id);
        }
        if (r->quantity > (uint64_t)((INT64_MAX - required_length) / r->length_mm) ||
            r->quantity > maximum - count) {
            return result(LINEAR_CUT_NUMERIC_OVERFLOW, i, SIZE_MAX, r->stock_class_id);
        }
        count += (size_t)r->quantity;
        required_length += (int64_t)r->quantity * r->length_mm;
    }
    for (size_t i = 0; i < option_count; i++) {
        if (options[i].stock_class_id == LINEAR_STOCK_CLASS_INVALID || options[i].length_mm <= 0) {
            return result(LINEAR_CUT_INVALID_STOCK_CATALOGUE, SIZE_MAX, i, options[i].stock_class_id);
        }
    }

    LinearCutResult status = result(LINEAR_CUT_SUCCESS, SIZE_MAX, SIZE_MAX, 0);
    LinearCutPlan candidate = {.total_required_length_mm = required_length};
    Piece *pieces = NULL;
    if (count != 0) {
        pieces = malloc(count * sizeof *pieces);
        if (pieces == NULL) { goto allocation_failed; }
        candidate.stock = malloc(count * sizeof *candidate.stock);
        if (candidate.stock == NULL) { goto allocation_failed; }
        candidate.cuts = malloc(count * sizeof *candidate.cuts);
        if (candidate.cuts == NULL) { goto allocation_failed; }
    }
    size_t offset = 0;
    for (size_t i = 0; i < requirement_count; i++) {
        for (size_t ordinal = 0; ordinal < (size_t)requirements[i].quantity; ordinal++) {
            pieces[offset++] = (Piece){requirements[i].stock_class_id, requirements[i].length_mm,
                i, ordinal, SIZE_MAX};
        }
    }
    if (count > 1) { qsort(pieces, count, sizeof *pieces, piece_compare); }
    for (size_t i = 0; i < count; i++) {
        Piece *piece = &pieces[i];
        size_t selected = best_open_stock(&candidate, piece, kerf_mm);
        int allowance = kerf_mm;
        if (selected == SIZE_MAX) {
            size_t option = shortest_option(options, option_count, piece);
            if (option == SIZE_MAX) {
                status = result(LINEAR_CUT_UNSATISFIED_REQUIREMENT, piece->requirement_index,
                    SIZE_MAX, piece->stock_class_id);
                goto cleanup;
            }
            const LinearStockOption *source = &options[option];
            if (candidate.total_stock_length_mm > INT64_MAX - source->length_mm) {
                status = result(LINEAR_CUT_NUMERIC_OVERFLOW, piece->requirement_index,
                    option, piece->stock_class_id);
                goto cleanup;
            }
            candidate.total_stock_length_mm += source->length_mm;
            selected = candidate.stock_count++;
            candidate.stock[selected] = (LinearPlannedStock){
                .stock_option_index = option, .stock_reference = source->reference,
                .stock_class_id = source->stock_class_id, .stock_length_mm = source->length_mm,
                .remainder_mm = source->length_mm
            };
            allowance = 0; /* No adjacency before the first piece. */
        }
        LinearPlannedStock *bar = &candidate.stock[selected];
        /* The fit check bounds each nonnegative sum below stock_length_mm,
         * so these int operations cannot overflow, even at INT_MAX lengths. */
        bar->used_piece_length_mm += piece->length_mm;
        bar->kerf_mm += allowance;
        bar->remainder_mm -= piece->length_mm;
        bar->remainder_mm -= allowance;
        bar->cut_count++;
        piece->stock_index = selected;
    }

    /* Prefix ranges are bounded by count. Reuse cut_count as a fill cursor,
     * then restore it while gathering cuts in their original placement order. */
    offset = 0;
    for (size_t i = 0; i < candidate.stock_count; i++) {
        LinearPlannedStock *bar = &candidate.stock[i];
        bar->first_cut = offset;
        offset += bar->cut_count;
        bar->cut_count = 0;
        /* Each component is nonnegative and bounded by the already checked
         * total stock length. Summing components therefore cannot overflow. */
        candidate.total_kerf_mm += bar->kerf_mm;
        candidate.total_remainder_mm += bar->remainder_mm;
    }
    for (size_t i = 0; i < count; i++) {
        const Piece *piece = &pieces[i];
        LinearPlannedStock *bar = &candidate.stock[piece->stock_index];
        candidate.cuts[bar->first_cut + bar->cut_count++] = (LinearPlannedCut){
            piece->requirement_index, requirements[piece->requirement_index].reference, piece->length_mm
        };
    }
    candidate.cut_count = count;
    linear_cut_plan_destroy(output);
    *output = candidate;
    candidate = (LinearCutPlan){0};
    goto cleanup;

allocation_failed:
    status = result(LINEAR_CUT_ALLOCATION_FAILED, SIZE_MAX, SIZE_MAX, 0);
cleanup:
    free(pieces);
    linear_cut_plan_destroy(&candidate);
    return status;
}
