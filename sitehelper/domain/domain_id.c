#include "domain_id.h"
#include <stdlib.h>

void domain_id_generator_init(
    DomainIdGenerator *generator
)
{
    if (generator == NULL) {
        return;
    }

    generator->next = 1;
}

DomainId domain_id_generate(
    DomainIdGenerator *generator
)
{
    if (generator == NULL) {
        return DOMAIN_ID_INVALID;
    }

    DomainId id =
        generator->next;

    generator->next++;

    return id;
}