#ifndef DOMAIN_ID_H
#define DOMAIN_ID_H

#include <stdint.h>

typedef uint64_t DomainId;

#define DOMAIN_ID_INVALID ((DomainId)0)

typedef struct
{
    DomainId next;
} DomainIdGenerator;

void domain_id_generator_init(
    DomainIdGenerator *generator
);

DomainId domain_id_generate(
    DomainIdGenerator *generator
);

#endif