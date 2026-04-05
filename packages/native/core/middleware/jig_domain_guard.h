#ifndef JIG_DOMAIN_GUARD_H
#define JIG_DOMAIN_GUARD_H

#include "../jig_middleware.h"

/*
 * Create a domain guard middleware.
 * For methods ending in .enable or .disable, checks that the domain
 * (part before first dot) is in the supported domains list.
 *
 * supported_domains: array of domain name strings (borrowed, must outlive middleware)
 * domain_count: number of entries
 *
 * The returned middleware's ctx points to an internal struct;
 * free with jig_domain_guard_destroy when done.
 */
jig_middleware jig_domain_guard_create(const char **supported_domains,
                                       int domain_count);

void jig_domain_guard_destroy(jig_middleware *mw);

#endif /* JIG_DOMAIN_GUARD_H */
