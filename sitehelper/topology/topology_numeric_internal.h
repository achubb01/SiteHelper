#ifndef TOPOLOGY_NUMERIC_INTERNAL_H
#define TOPOLOGY_NUMERIC_INTERNAL_H

#include "plan_topology.h"

/* GCC/Clang retain their existing native exact path. MSVC uses the portable
 * two-limb implementation in topology_numeric_msvc.c. */
#if !defined(_MSC_VER)
__extension__ typedef __int128 TopologyInt;
__extension__ typedef unsigned __int128 TopologyUInt;
#else
typedef struct { uint64_t lo, hi; } TopologyUInt;
typedef struct { uint64_t lo; int64_t hi; } TopologyInt;
#endif

PlanTopologyRational topology_rational(TopologyInt numerator, TopologyUInt denominator);
int topology_rational_compare(PlanTopologyRational a, PlanTopologyRational b);
int topology_vertex_compare(PlanTopologyVertex a, PlanTopologyVertex b);
PlanTopologyVertex topology_integer_point(PlanPosition point);
int topology_checked_add(TopologyInt a, TopologyInt b, TopologyInt *out);
int topology_checked_multiply(TopologyInt a, TopologyInt b, TopologyInt *out);
TopologyInt topology_cross(int64_t ax, int64_t ay, int64_t bx, int64_t by);
#if defined(_MSC_VER)
TopologyInt topology_int_from_i64(int64_t value);
TopologyUInt topology_uint_from_u64(uint64_t value);
TopologyUInt topology_int_magnitude(TopologyInt value);
int topology_int_compare(TopologyInt a, TopologyInt b);
int topology_uint_compare(TopologyUInt a, TopologyUInt b);
int topology_int_is_zero(TopologyInt value);
int topology_int_is_negative(TopologyInt value);
int topology_uint_is_zero(TopologyUInt value);
TopologyInt topology_int_negate(TopologyInt value);
TopologyUInt topology_int_as_uint(TopologyInt value);
TopologyInt topology_uint_as_int(TopologyUInt value);
TopologyInt topology_int_add(TopologyInt a, TopologyInt b);
TopologyInt topology_int_subtract(TopologyInt a, TopologyInt b);
TopologyUInt topology_uint_subtract(TopologyUInt a, TopologyUInt b);
TopologyUInt topology_uint_add(TopologyUInt a, TopologyUInt b);
TopologyUInt topology_uint_shift_left(TopologyUInt a, unsigned bits);
TopologyUInt topology_uint_complement(TopologyUInt a);
TopologyUInt topology_uint_divide(TopologyUInt n, TopologyUInt d);
TopologyUInt topology_uint_remainder(TopologyUInt n, TopologyUInt d);
#endif

#if !defined(_MSC_VER)
static inline TopologyInt topology_int_from_i64(int64_t v) { return (TopologyInt)v; }
static inline TopologyUInt topology_uint_from_u64(uint64_t v) { return (TopologyUInt)v; }
static inline TopologyUInt topology_int_magnitude(TopologyInt v) { return v < 0 ? 0 - (TopologyUInt)v : (TopologyUInt)v; }
static inline int topology_int_compare(TopologyInt a, TopologyInt b) { return a < b ? -1 : a != b; }
static inline int topology_uint_compare(TopologyUInt a, TopologyUInt b) { return a < b ? -1 : a != b; }
static inline int topology_int_is_zero(TopologyInt v) { return v == 0; }
static inline int topology_int_is_negative(TopologyInt v) { return v < 0; }
static inline int topology_uint_is_zero(TopologyUInt v) { return v == 0; }
static inline TopologyInt topology_int_negate(TopologyInt v) { return -v; }
static inline TopologyUInt topology_int_as_uint(TopologyInt v) { return (TopologyUInt)v; }
static inline TopologyInt topology_uint_as_int(TopologyUInt v) { return (TopologyInt)v; }
static inline TopologyInt topology_int_add(TopologyInt a, TopologyInt b) { return a + b; }
static inline TopologyInt topology_int_subtract(TopologyInt a, TopologyInt b) { return a - b; }
static inline TopologyUInt topology_uint_subtract(TopologyUInt a, TopologyUInt b) { return a - b; }
static inline TopologyUInt topology_uint_add(TopologyUInt a, TopologyUInt b) { return a + b; }
static inline TopologyUInt topology_uint_shift_left(TopologyUInt a, unsigned n) { return a << n; }
static inline TopologyUInt topology_uint_complement(TopologyUInt a) { return ~a; }
static inline TopologyUInt topology_uint_divide(TopologyUInt a, TopologyUInt b) { return a / b; }
static inline TopologyUInt topology_uint_remainder(TopologyUInt a, TopologyUInt b) { return a % b; }
#endif

#endif
