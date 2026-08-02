#include <assert.h>
#include <stdio.h>

#include "grid_render.h"

static void test_grid_uses_100_at_close_zoom(void)
{
    double spacing =
        grid_choose_spacing(
            1.0,
            16.0
        );

    assert(spacing == 100.0);
}

static void test_grid_uses_500_when_100_is_too_dense(void)
{
    double spacing =
        grid_choose_spacing(
            0.12,
            16.0
        );

    assert(spacing == 500.0);
}

static void test_grid_uses_1000_when_500_is_too_dense(void)
{
    double spacing =
        grid_choose_spacing(
            0.02,
            16.0
        );

    assert(spacing == 1000.0);
}

static void test_grid_uses_larger_spacing_when_zoomed_far_out(void)
{
    double spacing =
        grid_choose_spacing(
            0.001,
            16.0
        );

    assert(spacing == 25000.0);
}

static void test_grid_spacing_meets_minimum_screen_spacing(void)
{
    const double scale = 0.007;
    const double minimum = 16.0;

    double spacing =
        grid_choose_spacing(
            scale,
            minimum
        );

    assert(
        spacing * scale >= minimum
    );
}

static void test_invalid_scale_returns_default_spacing(void)
{
    assert(
        grid_choose_spacing(
            0.0,
            16.0
        ) == 100.0
    );

    assert(
        grid_choose_spacing(
            -1.0,
            16.0
        ) == 100.0
    );
}

static void test_invalid_minimum_returns_default_spacing(void)
{
    assert(
        grid_choose_spacing(
            1.0,
            0.0
        ) == 100.0
    );

    assert(
        grid_choose_spacing(
            1.0,
            -10.0
        ) == 100.0
    );
}

int main(void)
{
    test_grid_uses_100_at_close_zoom();
    test_grid_uses_500_when_100_is_too_dense();
    test_grid_uses_1000_when_500_is_too_dense();
    test_grid_uses_larger_spacing_when_zoomed_far_out();
    test_grid_spacing_meets_minimum_screen_spacing();
    test_invalid_scale_returns_default_spacing();
    test_invalid_minimum_returns_default_spacing();

    printf("All grid render tests passed.\n");

    return 0;
}