#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#include "wall_plan_transform.h"

_Static_assert(_Generic((PlanPoint){0}, PlanPosition: 0, default: 1),
    "Calculated plan points must remain distinct from persistent positions");

static void assert_near(double actual, double expected)
{
    assert(isfinite(actual));
    assert(fabs(actual - expected) <= 1e-9 * fmax(1.0, fabs(expected)));
}

static void assert_mapping(WallPlanSegment segment, double u, PlanPoint expected)
{
    PlanPoint point;
    assert(wall_plan_segment_u_to_plan(segment, u, &point));
    assert_near(point.x, expected.x);
    assert_near(point.y, expected.y);
    double projected;
    assert(wall_plan_segment_plan_to_u(segment, expected, &projected));
    assert_near(projected, u);
}

static void test_endpoints_and_round_trips(void)
{
    const struct {
        WallPlanSegment segment;
        int length;
    } cases[] = {
        {{{1000, 2000}, {5000, 2000}}, 4000},
        {{{5000, 2000}, {1000, 2000}}, 4000},
        {{{1000, 2000}, {1000, 6000}}, 4000},
        {{{1000, 6000}, {1000, 2000}}, 4000},
        {{{1000, 2000}, {4600, 6800}}, 6000},
        {{{4600, 6800}, {1000, 2000}}, 6000},
        {{{0, 0}, {1000, 1000}}, 1414},
        {{{1000, 1000}, {0, 0}}, 1414},
        {{{-5000, -3000}, {-4000, -2000}}, 1414},
        {{{-1800, 2400}, {1800, -2400}}, 6000},
        {{{0, 0}, {2, 2}}, 3},
        {{{0, 0}, {1, 0}}, 1},
        {{{INT_MIN, INT_MAX}, {INT_MIN + 6000, INT_MAX}}, 6000},
        {{{0, 0}, {INT_MAX, 0}}, INT_MAX},
        {{{INT_MAX, 0}, {0, 0}}, INT_MAX}
    };
    const double fractions[] = {-0.25, 0.0, 0.125, 0.3, 0.5, 0.9, 1.0, 1.25};
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        WallPlanSegment segment = cases[i].segment;
        int length = cases[i].length;
        assert(wall_plan_segment_length_mm(segment) == length);
        PlanPoint point;
        assert(wall_plan_segment_u_to_plan(segment, 0.0, &point));
        assert(point.x == segment.start.x && point.y == segment.start.y);
        assert(wall_plan_segment_u_to_plan(segment, length, &point));
        assert(point.x == segment.end.x && point.y == segment.end.y);

        double u;
        assert(wall_plan_segment_plan_to_u(segment,
            (PlanPoint){segment.start.x, segment.start.y}, &u));
        assert_near(u, 0.0);
        assert(wall_plan_segment_plan_to_u(segment,
            (PlanPoint){segment.end.x, segment.end.y}, &u));
        assert_near(u, length);

        for (size_t j = 0; j < sizeof fractions / sizeof fractions[0]; j++) {
            double original = fractions[j] * length;
            assert(wall_plan_segment_u_to_plan(segment, original, &point));
            assert(wall_plan_segment_plan_to_u(segment, point, &u));
            assert_near(u, original);
        }
    }
}

static void test_known_locations(void)
{
    WallPlanSegment horizontal = {{1000, 2000}, {5000, 2000}};
    assert_mapping(horizontal, 1000, (PlanPoint){2000, 2000});
    assert_mapping(horizontal, 0.25, (PlanPoint){1000.25, 2000});
    assert_mapping(horizontal, -1000, (PlanPoint){0, 2000});
    assert_mapping(horizontal, 5000, (PlanPoint){6000, 2000});
    assert_mapping((WallPlanSegment){{5000, 2000}, {1000, 2000}},
        1000, (PlanPoint){4000, 2000});
    assert_mapping((WallPlanSegment){{1000, 2000}, {1000, 6000}},
        1000, (PlanPoint){1000, 3000});
    assert_mapping((WallPlanSegment){{1000, 6000}, {1000, 2000}},
        1000, (PlanPoint){1000, 5000});

    WallPlanSegment diagonal = {{1000, 2000}, {4600, 6800}};
    assert_mapping(diagonal, 1500, (PlanPoint){1900, 3200});
    assert_mapping(diagonal, 3000, (PlanPoint){2800, 4400});
    assert_mapping(diagonal, -1500, (PlanPoint){100, 800});
    assert_mapping(diagonal, 7500, (PlanPoint){5500, 8000});
    assert_mapping((WallPlanSegment){{-1800, 2400}, {1800, -2400}},
        1500, (PlanPoint){-900, 1200});

    WallPlanSegment rounded = {{0, 0}, {1000, 1000}};
    assert_mapping(rounded, 707, (PlanPoint){500, 500});
    assert_mapping(rounded, 353.5, (PlanPoint){250, 250});
    assert_mapping(rounded, 1, (PlanPoint){1000.0 / 1414.0, 1000.0 / 1414.0});
    assert_mapping(rounded, 2121, (PlanPoint){1500, 1500});
    assert_mapping((WallPlanSegment){{-5000, -3000}, {-4000, -2000}},
        707, (PlanPoint){-4500, -2500});
}

static void test_off_axis_projection(void)
{
    const struct {
        WallPlanSegment segment;
        PlanPoint point;
        double u;
    } cases[] = {
        {{{1000, 2000}, {5000, 2000}}, {2500, 2500}, 1500},
        {{{5000, 2000}, {1000, 2000}}, {2500, 1500}, 2500},
        {{{1000, 2000}, {1000, 6000}}, {-500, 3500}, 1500},
        /* Midpoint plus a perpendicular offset (-400, 300). */
        {{{1000, 2000}, {4600, 6800}}, {2400, 4700}, 3000},
        {{{0, 0}, {1000, 1000}}, {400, 600}, 707},
        {{{1000, 2000}, {5000, 2000}}, {0, 2500}, -1000},
        {{{1000, 2000}, {5000, 2000}}, {6000, 1500}, 5000}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        double u;
        assert(wall_plan_segment_plan_to_u(cases[i].segment, cases[i].point, &u));
        assert_near(u, cases[i].u);
    }
}

static void test_invalid_inputs(void)
{
    const WallPlanSegment invalid[] = {
        {{0, 0}, {0, 0}},
        {{12, 34}, {12, 34}},
        {{-1, 0}, {INT_MAX, 0}},
        {{INT_MIN, INT_MIN}, {INT_MAX, INT_MAX}},
        {{0, 0}, {INT_MAX, INT_MAX}}
    };
    PlanPoint point = {123, 456};
    double u = 789;
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        assert(wall_plan_segment_length_mm(invalid[i]) == 0);
        assert(!wall_plan_segment_u_to_plan(invalid[i], 0, &point));
        assert(!wall_plan_segment_plan_to_u(invalid[i], (PlanPoint){0, 0}, &u));
        assert(point.x == 123 && point.y == 456 && u == 789);
    }
    WallPlanSegment valid = {{1000, 2000}, {5000, 2000}};
    assert(!wall_plan_segment_u_to_plan(valid, 0, NULL));
    assert(!wall_plan_segment_plan_to_u(valid, point, NULL));
    const double nonfinite[] = {NAN, INFINITY, -INFINITY};
    for (size_t i = 0; i < sizeof nonfinite / sizeof nonfinite[0]; i++) {
        assert(!wall_plan_segment_u_to_plan(valid, nonfinite[i], &point));
        assert(!wall_plan_segment_plan_to_u(valid, (PlanPoint){nonfinite[i], 0}, &u));
        assert(!wall_plan_segment_plan_to_u(valid, (PlanPoint){0, nonfinite[i]}, &u));
        assert(point.x == 123 && point.y == 456 && u == 789);
    }
    /* Finite inputs must not silently publish overflowing calculations. */
    assert(!wall_plan_segment_plan_to_u(valid, (PlanPoint){DBL_MAX, 0}, &u));
    assert(u == 789);
    assert(wall_plan_segment_u_to_plan(
        (WallPlanSegment){{0, 0}, {1, 1}}, DBL_MAX, &point));
    assert(point.x == DBL_MAX && point.y == DBL_MAX);
    /* The inverse can exceed the double range even for finite plan points. */
    assert(!wall_plan_segment_plan_to_u(
        (WallPlanSegment){{0, 0}, {2, 2}}, (PlanPoint){DBL_MAX, DBL_MAX}, &u));
    assert(u == 789);
}

int main(void)
{
    test_endpoints_and_round_trips();
    test_known_locations();
    test_off_axis_projection();
    test_invalid_inputs();
    puts("wall plan transform tests passed");
    return 0;
}
