#include <assert.h>
#include <stdio.h>

#include "domain_id.h"

static void test_generator_starts_at_one(void)
{
    DomainIdGenerator generator;

    domain_id_generator_init(
        &generator
    );

    DomainId id =
        domain_id_generate(
            &generator
        );

    assert(id == 1);
}

static void test_generator_produces_unique_ids(void)
{
    DomainIdGenerator generator;

    domain_id_generator_init(
        &generator
    );

    DomainId first =
        domain_id_generate(
            &generator
        );

    DomainId second =
        domain_id_generate(
            &generator
        );

    assert(first != second);
}

static void test_generator_produces_monotonic_ids(void)
{
    DomainIdGenerator generator;

    domain_id_generator_init(
        &generator
    );

    DomainId first =
        domain_id_generate(
            &generator
        );

    DomainId second =
        domain_id_generate(
            &generator
        );

    assert(second > first);
}

static void test_null_generator_returns_invalid_id(void)
{
    DomainId id =
        domain_id_generate(
            NULL
        );

    assert(
        id == DOMAIN_ID_INVALID
    );
}

int main(void)
{
    test_generator_starts_at_one();
    test_generator_produces_unique_ids();
    test_generator_produces_monotonic_ids();
    test_null_generator_returns_invalid_id();

    printf(
        "All domain ID tests passed.\n"
    );

    return 0;
}