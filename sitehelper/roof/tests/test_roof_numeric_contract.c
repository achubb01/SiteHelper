#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define ROOF_SLOPE_SCALE INT64_C(1000000)
#define DEG_TO_RAD 0.017453292519943295769236907684886

typedef struct
{
    int64_t numerator;
    int64_t denominator;
} Rational64;

typedef struct
{
    int64_t gradient_x;
    int64_t gradient_y;
    int64_t offset;
} ScaledPlane;

typedef struct
{
    int64_t a;
    int64_t b;
    int64_t c;
} PlanLine;

static uint64_t magnitude_i64(int64_t value)
{
    return value < 0 ? UINT64_C(0) - (uint64_t)value : (uint64_t)value;
}

static uint64_t gcd_u64(uint64_t a, uint64_t b)
{
    while (b != 0)
    {
        uint64_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

static Rational64 rational64(int64_t numerator, int64_t denominator)
{
    assert(denominator != 0);

    if (denominator < 0)
    {
        numerator = -numerator;
        denominator = -denominator;
    }

    if (numerator == 0)
    {
        return (Rational64){0, 1};
    }

    uint64_t divisor = gcd_u64(magnitude_i64(numerator), (uint64_t)denominator);
    return (Rational64){numerator / (int64_t)divisor,
                        denominator / (int64_t)divisor};
}

static long double rational64_approximate(Rational64 value)
{
    return (long double)value.numerator / (long double)value.denominator;
}

static void assert_rational(Rational64 actual, int64_t numerator, int64_t denominator)
{
    Rational64 expected = rational64(numerator, denominator);
    assert(actual.numerator == expected.numerator);
    assert(actual.denominator == expected.denominator);
}

static int64_t slope_from_degrees(double degrees)
{
    return (int64_t)llround(tan(degrees * DEG_TO_RAD) * (double)ROOF_SLOPE_SCALE);
}

static double degrees_from_slope(int64_t slope)
{
    return atan((double)slope / (double)ROOF_SLOPE_SCALE) / DEG_TO_RAD;
}

/* Integer affine roof plane:
 *
 *   SCALE * Z = gradient_x * X + gradient_y * Y + offset
 *
 * X/Y/Z are millimetres. gradient_x/gradient_y are vertical rise per one
 * million horizontal run. The prototype intentionally keeps this local to the
 * test: 26C chooses the numeric contract, not a public RoofDefinition API. */
static ScaledPlane plane_from_reference(int x, int y, int z,
                                        int64_t gradient_x,
                                        int64_t gradient_y)
{
    int64_t offset = ROOF_SLOPE_SCALE * (int64_t)z
                   - gradient_x * (int64_t)x
                   - gradient_y * (int64_t)y;
    return (ScaledPlane){gradient_x, gradient_y, offset};
}

static Rational64 plane_z_at_integer(ScaledPlane plane, int x, int y)
{
    int64_t numerator = plane.gradient_x * (int64_t)x
                      + plane.gradient_y * (int64_t)y
                      + plane.offset;
    return rational64(numerator, ROOF_SLOPE_SCALE);
}

/* Projected plan line on which two roof planes have equal Z. */
static PlanLine plane_intersection_line(ScaledPlane lhs, ScaledPlane rhs)
{
    return (PlanLine){
        lhs.gradient_x - rhs.gradient_x,
        lhs.gradient_y - rhs.gradient_y,
        rhs.offset - lhs.offset
    };
}

static Rational64 line_y_at_x(PlanLine line, int x)
{
    assert(line.b != 0);
    return rational64(line.c - line.a * (int64_t)x, line.b);
}

static Rational64 plane_z_at_rational_y(ScaledPlane plane, int x, Rational64 y)
{
    /* Fixture-sized exact evaluation. Production exact arithmetic must use the
     * project's checked portable wide-integer path rather than relying on these
     * 64-bit intermediate bounds. */
    int64_t numerator = (plane.gradient_x * (int64_t)x + plane.offset)
                      * y.denominator
                      + plane.gradient_y * y.numerator;
    int64_t denominator = y.denominator * ROOF_SLOPE_SCALE;
    return rational64(numerator, denominator);
}

static void test_common_degree_inputs_canonicalize_to_expected_slopes(void)
{
    const struct
    {
        double degrees;
        int64_t slope;
    } cases[] = {
        {5.0, 87489},
        {7.5, 131652},
        {10.0, 176327},
        {15.0, 267949},
        {20.0, 363970},
        {22.5, 414214},
        {25.0, 466308},
        {30.0, 577350},
        {35.0, 700208},
        {45.0, 1000000}
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i)
    {
        int64_t slope = slope_from_degrees(cases[i].degrees);
        assert(slope == cases[i].slope);
        assert(fabs(degrees_from_slope(slope) - cases[i].degrees) < 0.00005);

        double exact = tan(cases[i].degrees * DEG_TO_RAD);
        double canonical = (double)slope / (double)ROOF_SLOPE_SCALE;
        assert(fabs(canonical - exact) * 16000.0 < 0.0081);
    }
}

static void test_a0_gable_plane_intersection_and_height(void)
{
    const int64_t slope_22_5 = 414214;

    ScaledPlane south = plane_from_reference(0, 0, 0, 0, slope_22_5);
    ScaledPlane north = plane_from_reference(0, 8000, 0, 0, -slope_22_5);
    PlanLine ridge = plane_intersection_line(south, north);

    Rational64 y = line_y_at_x(ridge, 6000);
    assert_rational(y, 4000, 1);

    Rational64 z = plane_z_at_integer(south, 6000, 4000);
    assert_rational(z, 207107, 125); /* 1656.856 mm */
    assert(fabsl(rational64_approximate(z)
                 - 4000.0L * tanl(22.5L * (long double)DEG_TO_RAD))
           < 0.002L);
}

static void test_c1_unequal_gable_valley_is_exact_rational(void)
{
    const int64_t slope_22_5 = 414214;
    const int64_t slope_30 = 577350;

    ScaledPlane main_north = plane_from_reference(0, 8000, 0, 0, -slope_22_5);
    ScaledPlane wing_west = plane_from_reference(4000, 0, 0, slope_30, 0);

    PlanLine valley = plane_intersection_line(main_north, wing_west);
    Rational64 junction_y = line_y_at_x(valley, 6000);

    assert_rational(junction_y, INT64_C(1079506000), INT64_C(207107));
    assert(fabsl(rational64_approximate(junction_y) - 5212.310544790856L)
           < 1e-9L);

    Rational64 main_z = plane_z_at_rational_y(main_north, 6000, junction_y);
    Rational64 wing_z = plane_z_at_rational_y(wing_west, 6000, junction_y);
    assert(main_z.numerator == wing_z.numerator);
    assert(main_z.denominator == wing_z.denominator);
    assert_rational(main_z, 11547, 10); /* 1154.7 mm */

    /* Plane order changes line signs but not the geometric answer. */
    PlanLine reversed = plane_intersection_line(wing_west, main_north);
    Rational64 reversed_y = line_y_at_x(reversed, 6000);
    assert(reversed_y.numerator == junction_y.numerator);
    assert(reversed_y.denominator == junction_y.denominator);

    /* 26B explicitly requires a non-integer derived junction. */
    assert(junction_y.denominator != 1);
}

static void test_f1_overlap_intersection_is_exact_and_not_rounded(void)
{
    const int64_t slope_22_5 = 414214;
    const int64_t slope_10 = 176327;

    ScaledPlane main_north = plane_from_reference(0, 8000, 0, 0, -slope_22_5);
    ScaledPlane overlap = plane_from_reference(0, 7000, 250, 0, -slope_10);

    PlanLine seam = plane_intersection_line(main_north, overlap);
    Rational64 seam_y_at_left = line_y_at_x(seam, 3000);
    Rational64 seam_y_at_right = line_y_at_x(seam, 9000);

    assert_rational(seam_y_at_left, INT64_C(1829423000), INT64_C(237887));
    assert(seam_y_at_left.numerator == seam_y_at_right.numerator);
    assert(seam_y_at_left.denominator == seam_y_at_right.denominator);
    assert(seam_y_at_left.denominator != 1);
    assert(fabsl(rational64_approximate(seam_y_at_left) - 7690.302538600260L)
           < 1e-9L);

    Rational64 main_z = plane_z_at_rational_y(main_north, 3000, seam_y_at_left);
    Rational64 overlap_z = plane_z_at_rational_y(overlap, 3000, seam_y_at_left);
    assert(main_z.numerator == overlap_z.numerator);
    assert(main_z.denominator == overlap_z.denominator);
    assert_rational(main_z, INT64_C(15258194011), INT64_C(118943500));
}

int main(void)
{
    test_common_degree_inputs_canonicalize_to_expected_slopes();
    test_a0_gable_plane_intersection_and_height();
    test_c1_unequal_gable_valley_is_exact_rational();
    test_f1_overlap_intersection_is_exact_and_not_rounded();
    puts("roof numeric contract prototype: ok");
    return 0;
}
