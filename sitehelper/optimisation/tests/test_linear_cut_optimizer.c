#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linear_cut_optimizer.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int reject_allocation(void)
{
    if (fail_after != SIZE_MAX && fail_after-- == 0) {
        allocation_failed = 1;
        return 1;
    }
    return 0;
}
void *__wrap_malloc(size_t size) { return reject_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return reject_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *p, size_t size) { return reject_allocation() ? NULL : __real_realloc(p, size); }
#endif

/* Independent accounting oracle; makes no assumptions about the heuristic. */
static void verify(const LinearCutPlan *plan, const LinearCutRequirement *requirements,
    size_t requirement_count, const LinearStockOption *options, size_t option_count, int kerf)
{
    uint64_t *quantities = requirement_count ? calloc(requirement_count, sizeof *quantities) : NULL;
    assert(requirement_count == 0 || quantities);
    size_t offset = 0;
    int64_t pieces = 0, stock = 0, allowance = 0, remainder = 0;
    for (size_t i = 0; i < plan->stock_count; i++) {
        const LinearPlannedStock *bar = &plan->stock[i];
        assert(bar->stock_option_index < option_count);
        const LinearStockOption *option = &options[bar->stock_option_index];
        assert(bar->stock_class_id == option->stock_class_id && bar->stock_reference == option->reference);
        assert(bar->stock_length_mm == option->length_mm);
        assert(bar->first_cut == offset && bar->cut_count > 0);
        assert(offset <= plan->cut_count && bar->cut_count <= plan->cut_count - offset);
        int64_t used = 0;
        for (size_t j = 0; j < bar->cut_count; j++) {
            const LinearPlannedCut *cut = &plan->cuts[offset + j];
            assert(cut->requirement_index < requirement_count);
            const LinearCutRequirement *r = &requirements[cut->requirement_index];
            assert(r->stock_class_id == bar->stock_class_id);
            assert(cut->requirement_reference == r->reference && cut->length_mm == r->length_mm);
            quantities[cut->requirement_index]++;
            used += cut->length_mm;
        }
        assert(used == bar->used_piece_length_mm);
        assert((int64_t)kerf * (int64_t)(bar->cut_count - 1) == bar->kerf_mm);
        assert(bar->remainder_mm >= 0 && bar->kerf_mm >= 0);
        assert(used + bar->kerf_mm + bar->remainder_mm == bar->stock_length_mm);
        pieces += used; stock += bar->stock_length_mm;
        allowance += bar->kerf_mm; remainder += bar->remainder_mm;
        offset += bar->cut_count;
    }
    assert(offset == plan->cut_count);
    for (size_t i = 0; i < requirement_count; i++) { assert(quantities[i] == requirements[i].quantity); }
    free(quantities);
    assert(pieces == plan->total_required_length_mm && stock == plan->total_stock_length_mm);
    assert(allowance == plan->total_kerf_mm && remainder == plan->total_remainder_mm);
    assert(stock == pieces + allowance + remainder);
}

static void equivalent(const LinearCutPlan *a, const LinearCutPlan *b)
{
    assert(a->stock_count == b->stock_count && a->cut_count == b->cut_count);
    assert(a->total_required_length_mm == b->total_required_length_mm);
    assert(a->total_stock_length_mm == b->total_stock_length_mm);
    assert(a->total_kerf_mm == b->total_kerf_mm && a->total_remainder_mm == b->total_remainder_mm);
    for (size_t i = 0; i < a->stock_count; i++) {
        const LinearPlannedStock *x = &a->stock[i], *y = &b->stock[i];
        assert(x->stock_option_index == y->stock_option_index && x->stock_reference == y->stock_reference);
        assert(x->stock_class_id == y->stock_class_id && x->stock_length_mm == y->stock_length_mm);
        assert(x->first_cut == y->first_cut && x->cut_count == y->cut_count);
        assert(x->used_piece_length_mm == y->used_piece_length_mm);
        assert(x->kerf_mm == y->kerf_mm && x->remainder_mm == y->remainder_mm);
    }
    for (size_t i = 0; i < a->cut_count; i++) {
        const LinearPlannedCut *x = &a->cuts[i], *y = &b->cuts[i];
        assert(x->requirement_index == y->requirement_index);
        assert(x->requirement_reference == y->requirement_reference && x->length_mm == y->length_mm);
    }
}

static void assert_zero(const LinearCutPlan *plan)
{
    assert(plan->stock == NULL && plan->stock_count == 0 && plan->cuts == NULL && plan->cut_count == 0);
    assert(plan->total_required_length_mm == 0 && plan->total_stock_length_mm == 0);
    assert(plan->total_kerf_mm == 0 && plan->total_remainder_mm == 0);
}

static void test_empty_single_exact_and_kerf(void)
{
    LinearCutPlan plan = {0};
    assert(linear_cut_optimize(NULL, 0, NULL, 0, 0, &plan).code == LINEAR_CUT_SUCCESS);
    assert_zero(&plan);
    LinearCutRequirement r = {1, 0, 500, 1};
    LinearStockOption s = {1, UINT64_MAX, 1000};
    assert(linear_cut_optimize(&r, 1, &s, 1, 3, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, &r, 1, &s, 1, 3);
    assert(plan.stock_count == 1 && plan.cut_count == 1 && plan.total_remainder_mm == 500);
    assert(plan.total_kerf_mm == 0);
    r.quantity = 2;
    assert(linear_cut_optimize(&r, 1, &s, 1, 0, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, &r, 1, &s, 1, 0);
    assert(plan.stock_count == 1 && plan.stock[0].cut_count == 2 && plan.total_remainder_mm == 0);
    assert(linear_cut_optimize(&r, 1, &s, 1, 1, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, &r, 1, &s, 1, 1);
    assert(plan.stock_count == 2 && plan.total_kerf_mm == 0);
    r.length_mm = 498; r.quantity = 5;
    assert(linear_cut_optimize(&r, 1, &s, 1, 4, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, &r, 1, &s, 1, 4);
    assert(plan.stock_count == 3 && plan.cut_count == 5 && plan.total_kerf_mm == 8);
    r.length_mm = INT_MAX; s.length_mm = INT_MAX; r.quantity = 2;
    assert(linear_cut_optimize(&r, 1, &s, 1, INT_MAX, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, &r, 1, &s, 1, INT_MAX);
    assert(plan.total_required_length_mm == 2 * (int64_t)INT_MAX && plan.total_remainder_mm == 0);
    r.length_mm = 1;
    assert(linear_cut_optimize(&r, 1, &s, 1, INT_MAX, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, &r, 1, &s, 1, INT_MAX);
    assert(plan.stock_count == 2); /* length + kerf must not overflow an int. */
    assert(linear_cut_optimize(NULL, 0, &s, 1, 4, &plan).code == LINEAR_CUT_SUCCESS);
    assert_zero(&plan);
    linear_cut_plan_destroy(&plan); linear_cut_plan_destroy(&plan); linear_cut_plan_destroy(NULL);
    assert_zero(&plan);
}

static void test_best_fit_and_catalogue_choices(void)
{
    LinearCutRequirement requirements[] = {{1, 81, 800, 1}, {1, 61, 600, 1}, {1, 41, 40, 1}};
    LinearStockOption options[] = {{1, 10, 1000}, {1, 11, 650}, {1, 12, 650}};
    LinearCutPlan plan = {0};
    assert(linear_cut_optimize(requirements, 3, options, 3, 0, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, requirements, 3, options, 3, 0);
    assert(plan.stock_count == 2);
    assert(plan.stock[0].stock_option_index == 0 && plan.stock[0].cut_count == 1);
    assert(plan.stock[1].stock_option_index == 1 && plan.stock[1].cut_count == 2);
    assert(plan.stock[1].remainder_mm == 10); /* Best fit chooses the second bar. */
    assert(plan.cuts[2].requirement_reference == 41);

    LinearCutRequirement repeated = {1, 0, 500, 2};
    LinearStockOption lengths[] = {{1, 0, 1000}, {1, 0, 600}};
    assert(linear_cut_optimize(&repeated, 1, lengths, 2, 0, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, &repeated, 1, lengths, 2, 0);
    assert(plan.stock_count == 2 && plan.total_remainder_mm == 200);
    /* The specified shortest-new-bar heuristic is intentionally nonoptimal:
     * one 1000-mm bar could instead fit both pieces with zero remainder. */
    linear_cut_plan_destroy(&plan);
}

static void test_ties_classes_and_opaque_references(void)
{
    LinearCutRequirement r[] = {{9, 0, 400, 1}, {2, 900, 600, 2},
        {2, 100, 400, 1}, {2, 900, 400, 1}, {9, UINT64_MAX, 400, 1}};
    LinearStockOption s[] = {{9, 0, 1000}, {2, UINT64_MAX, 1000}, {2, 1, 1000}};
    LinearCutRequirement original_r[5]; LinearStockOption original_s[3];
    memcpy(original_r, r, sizeof r); memcpy(original_s, s, sizeof s);
    LinearCutPlan plan = {0}, again = {0};
    assert(linear_cut_optimize(r, 5, s, 3, 0, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, r, 5, s, 3, 0);
    assert(plan.stock_count == 3);
    assert(plan.stock[0].stock_class_id == 2 && plan.stock[1].stock_class_id == 2);
    assert(plan.stock[2].stock_class_id == 9);
    assert(plan.stock[0].stock_option_index == 1 && plan.stock[1].stock_option_index == 1);
    assert(plan.cuts[0].requirement_index == 1 && plan.cuts[1].requirement_index == 2);
    assert(plan.cuts[2].requirement_index == 1 && plan.cuts[3].requirement_index == 3);
    assert(plan.cuts[4].requirement_index == 0 && plan.cuts[5].requirement_index == 4);
    assert(linear_cut_optimize(r, 5, s, 3, 0, &again).code == LINEAR_CUT_SUCCESS);
    equivalent(&plan, &again);
    assert(memcmp(original_r, r, sizeof r) == 0 && memcmp(original_s, s, sizeof s) == 0);

    /* References can encode any caller purpose/provenance. Changing every
     * reference must not affect grouping, catalogue selection or cut order. */
    for (size_t i = 0; i < 5; i++) { r[i].reference = 17 + i; }
    for (size_t i = 0; i < 3; i++) { s[i].reference = 33 + i; }
    assert(linear_cut_optimize(r, 5, s, 3, 0, &again).code == LINEAR_CUT_SUCCESS);
    verify(&again, r, 5, s, 3, 0);
    assert(plan.stock_count == again.stock_count && plan.cut_count == again.cut_count);
    for (size_t i = 0; i < plan.stock_count; i++) {
        assert(plan.stock[i].stock_option_index == again.stock[i].stock_option_index);
        assert(plan.stock[i].first_cut == again.stock[i].first_cut && plan.stock[i].cut_count == again.stock[i].cut_count);
    }
    for (size_t i = 0; i < plan.cut_count; i++) {
        assert(plan.cuts[i].requirement_index == again.cuts[i].requirement_index);
    }
    linear_cut_plan_destroy(&again); linear_cut_plan_destroy(&plan);
}

typedef struct {
    LinearCutPlan before;
    LinearPlannedStock *stock_copy;
    LinearPlannedCut *cuts_copy;
} SnapshotGuard;

static SnapshotGuard guard(const LinearCutPlan *plan)
{
    SnapshotGuard g = {.before = *plan};
    if (plan->stock_count) {
        g.stock_copy = malloc(plan->stock_count * sizeof *g.stock_copy); assert(g.stock_copy);
        memcpy(g.stock_copy, plan->stock, plan->stock_count * sizeof *g.stock_copy);
    }
    if (plan->cut_count) {
        g.cuts_copy = malloc(plan->cut_count * sizeof *g.cuts_copy); assert(g.cuts_copy);
        memcpy(g.cuts_copy, plan->cuts, plan->cut_count * sizeof *g.cuts_copy);
    }
    return g;
}

static void unchanged(const SnapshotGuard *g, const LinearCutPlan *plan)
{
    assert(memcmp(&g->before, plan, sizeof *plan) == 0);
    if (plan->stock_count) { assert(memcmp(g->stock_copy, plan->stock, plan->stock_count * sizeof *plan->stock) == 0); }
    if (plan->cut_count) { assert(memcmp(g->cuts_copy, plan->cuts, plan->cut_count * sizeof *plan->cuts) == 0); }
}

static void guard_destroy(SnapshotGuard *g) { free(g->stock_copy); free(g->cuts_copy); }

static void test_snapshot_lifetime(void)
{
    LinearCutRequirement *r = malloc(sizeof *r);
    LinearStockOption *s = malloc(sizeof *s);
    assert(r && s);
    *r = (LinearCutRequirement){77, 900, 300, 3};
    *s = (LinearStockOption){77, 123, 1000};
    LinearCutPlan plan = {0};
    assert(linear_cut_optimize(r, 1, s, 1, 3, &plan).code == LINEAR_CUT_SUCCESS);
    verify(&plan, r, 1, s, 1, 3);
    SnapshotGuard g = guard(&plan);
    *r = (LinearCutRequirement){0}; *s = (LinearStockOption){0};
    unchanged(&g, &plan);
    free(r); free(s);
    unchanged(&g, &plan);
    assert(plan.cuts[0].requirement_reference == 900 && plan.stock[0].stock_reference == 123);
    assert(linear_cut_optimize(NULL, 0, NULL, 0, 0, &plan).code == LINEAR_CUT_SUCCESS);
    assert_zero(&plan);
    guard_destroy(&g);
    linear_cut_plan_destroy(&plan);
}

static void test_invalid_and_overflow_transactions(void)
{
    LinearCutRequirement valid_r = {1, 55, 400, 2};
    LinearStockOption valid_s = {1, 66, 1000};
    for (int populated = 0; populated < 2; populated++) {
        LinearCutPlan plan = {0};
        if (populated) { assert(linear_cut_optimize(&valid_r, 1, &valid_s, 1, 3, &plan).code == LINEAR_CUT_SUCCESS); }
        SnapshotGuard g = guard(&plan);
        for (int fault = 0; fault < 18; fault++) {
            LinearCutRequirement r = valid_r; LinearStockOption s = valid_s;
            const LinearCutRequirement *rp = &r; const LinearStockOption *sp = &s;
            size_t rc = 1, sc = 1;
            int kerf = 3;
            LinearCutCode expected = LINEAR_CUT_INVALID_ARGUMENT;
            switch (fault) {
            case 0: rp = NULL; break;
            case 1: sp = NULL; break;
            case 2: kerf = -1; break;
            case 3: r.length_mm = 0; expected = LINEAR_CUT_INVALID_REQUIREMENT; break;
            case 4: r.length_mm = -1; expected = LINEAR_CUT_INVALID_REQUIREMENT; break;
            case 5: r.quantity = 0; expected = LINEAR_CUT_INVALID_REQUIREMENT; break;
            case 6: r.stock_class_id = 0; expected = LINEAR_CUT_INVALID_REQUIREMENT; break;
            case 7: s.length_mm = 0; expected = LINEAR_CUT_INVALID_STOCK_CATALOGUE; break;
            case 8: s.length_mm = -1; expected = LINEAR_CUT_INVALID_STOCK_CATALOGUE; break;
            case 9: s.stock_class_id = 0; expected = LINEAR_CUT_INVALID_STOCK_CATALOGUE; break;
            case 10: sc = 0; expected = LINEAR_CUT_UNSATISFIED_REQUIREMENT; break;
            case 11: s.stock_class_id = 2; expected = LINEAR_CUT_UNSATISFIED_REQUIREMENT; break;
            case 12: s.length_mm = 399; expected = LINEAR_CUT_UNSATISFIED_REQUIREMENT; break;
            case 13: r.length_mm = INT_MAX; r.quantity = (uint64_t)INT64_MAX / INT_MAX + 1;
                expected = LINEAR_CUT_NUMERIC_OVERFLOW; break;
            case 14: r.length_mm = 1; r.quantity = UINT64_MAX;
                expected = LINEAR_CUT_NUMERIC_OVERFLOW; break;
            case 15: r.length_mm = 1; r.quantity = SIZE_MAX / sizeof(LinearPlannedStock) + 1;
                expected = LINEAR_CUT_NUMERIC_OVERFLOW; break;
            case 16: rc = SIZE_MAX; expected = LINEAR_CUT_NUMERIC_OVERFLOW; break;
            case 17: sc = SIZE_MAX; expected = LINEAR_CUT_NUMERIC_OVERFLOW; break;
            }
            LinearCutResult status = linear_cut_optimize(rp, rc, sp, sc, kerf, &plan);
            assert(status.code == expected);
            if (expected == LINEAR_CUT_INVALID_REQUIREMENT || expected == LINEAR_CUT_UNSATISFIED_REQUIREMENT ||
                (fault >= 13 && fault <= 15)) {
                assert(status.requirement_index == 0 && status.stock_class_id == r.stock_class_id);
            }
            if (expected == LINEAR_CUT_INVALID_STOCK_CATALOGUE) {
                assert(status.stock_option_index == 0 && status.stock_class_id == s.stock_class_id);
            }
            unchanged(&g, &plan);
        }
        assert(linear_cut_optimize(&valid_r, 1, &valid_s, 1, 0, NULL).code == LINEAR_CUT_INVALID_ARGUMENT);
        LinearStockOption bad = {0};
        assert(linear_cut_optimize(NULL, 0, &bad, 1, 0, &plan).code == LINEAR_CUT_INVALID_STOCK_CATALOGUE);
        unchanged(&g, &plan);
        /* Overflow in the sum of valid individual required-length products. */
        LinearCutRequirement huge[] = {{1, 0, INT_MAX, (uint64_t)INT64_MAX / INT_MAX}, {1, 0, INT_MAX, 1}};
        LinearCutResult status = linear_cut_optimize(huge, 2, &valid_s, 1, 0, &plan);
        assert(status.code == LINEAR_CUT_NUMERIC_OVERFLOW);
        /* On 32-bit targets the expansion-size limit can reject the first row. */
        if (SIZE_MAX > UINT32_MAX) { assert(status.requirement_index == 1); }
        unchanged(&g, &plan);
        /* Failure after an earlier compatible class has already been packed. */
        LinearCutRequirement partial[] = {{1, 17, 400, 1}, {2, 19, 300, 1}};
        status = linear_cut_optimize(partial, 2, &valid_s, 1, 3, &plan);
        assert(status.code == LINEAR_CUT_UNSATISFIED_REQUIREMENT && status.requirement_index == 1 && status.stock_class_id == 2);
        unchanged(&g, &plan);
        assert(linear_cut_optimize(&valid_r, 1, &valid_s, 1, 3, &plan).code == LINEAR_CUT_SUCCESS);
        verify(&plan, &valid_r, 1, &valid_s, 1, 3);
        guard_destroy(&g); linear_cut_plan_destroy(&plan);
    }
}

static uint32_t next_random(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

static void test_varied_accounting(void)
{
    uint32_t seed = 23;
    LinearStockOption options[] = {{1, 0, 600}, {1, 1, 1200}, {2, 0, 850}, {2, 2, 1200}, {3, 0, 1200}};
    for (size_t trial = 0; trial < 200; trial++) {
        LinearCutRequirement requirements[12];
        for (size_t i = 0; i < 12; i++) {
            requirements[i] = (LinearCutRequirement){1 + next_random(&seed) % 3,
                next_random(&seed), 1 + (int)(next_random(&seed) % 1200), 1 + next_random(&seed) % 5};
        }
        int kerf = (int)(next_random(&seed) % 11);
        LinearCutPlan plan = {0}, again = {0};
        assert(linear_cut_optimize(requirements, 12, options, 5, kerf, &plan).code == LINEAR_CUT_SUCCESS);
        verify(&plan, requirements, 12, options, 5, kerf);
        assert(linear_cut_optimize(requirements, 12, options, 5, kerf, &again).code == LINEAR_CUT_SUCCESS);
        equivalent(&plan, &again);
        linear_cut_plan_destroy(&plan); linear_cut_plan_destroy(&again);
    }
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    LinearCutRequirement r[] = {{2, 99, 550, 4}, {1, 88, 400, 3}, {2, 77, 200, 8}};
    LinearStockOption s[] = {{1, 10, 1000}, {2, 20, 1200}};
    LinearCutRequirement original_r[3]; LinearStockOption original_s[2];
    memcpy(original_r, r, sizeof r); memcpy(original_s, s, sizeof s);
    size_t failures = 0;
    for (int populated = 0; populated < 2; populated++) {
        for (size_t n = 0; ; n++) {
            LinearCutPlan plan = {0};
            if (populated) { assert(linear_cut_optimize(&r[1], 1, s, 2, 0, &plan).code == LINEAR_CUT_SUCCESS); }
            SnapshotGuard g = guard(&plan);
            fail_after = n; allocation_failed = 0;
            LinearCutResult status = linear_cut_optimize(r, 3, s, 2, 3, &plan);
            fail_after = SIZE_MAX;
            if (allocation_failed) {
                failures++;
                assert(status.code == LINEAR_CUT_ALLOCATION_FAILED);
                unchanged(&g, &plan);
                assert(linear_cut_optimize(r, 3, s, 2, 3, &plan).code == LINEAR_CUT_SUCCESS);
            } else { assert(status.code == LINEAR_CUT_SUCCESS); }
            verify(&plan, r, 3, s, 2, 3);
            assert(memcmp(original_r, r, sizeof r) == 0 && memcmp(original_s, s, sizeof s) == 0);
            guard_destroy(&g); linear_cut_plan_destroy(&plan);
            if (!allocation_failed) { break; }
            assert(n < 32);
        }
    }
    assert(failures >= 6);
    printf("linear optimiser allocation failures checked: %zu\n", failures);
}
#endif

int main(void)
{
    test_empty_single_exact_and_kerf();
    test_best_fit_and_catalogue_choices();
    test_ties_classes_and_opaque_references();
    test_snapshot_lifetime();
    test_invalid_and_overflow_transactions();
    test_varied_accounting();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    puts("linear cut optimiser tests passed");
    return 0;
}
