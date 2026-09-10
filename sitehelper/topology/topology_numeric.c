#include <assert.h>
#include <limits.h>
#include "topology_numeric_internal.h"

_Static_assert(INT_MAX <= INT32_MAX && INT_MIN >= INT32_MIN,
    "Topology exact arithmetic supports signed 32-bit authoritative coordinates");

static PlanTopologyMagnitude pack(TopologyUInt value)
{
    return (PlanTopologyMagnitude){(uint64_t)value, (uint64_t)(value >> 64)};
}

static TopologyUInt unpack(PlanTopologyMagnitude value)
{
    return ((TopologyUInt)value.hi << 64) | value.lo;
}

PlanTopologyRational topology_rational(TopologyInt numerator, TopologyUInt denominator)
{
    assert(denominator != 0);
    /* Unsigned subtraction also handles the minimum signed 128-bit value. */
    TopologyUInt magnitude = numerator < 0 ? 0 - (TopologyUInt)numerator : (TopologyUInt)numerator;
    TopologyUInt a = magnitude, b = denominator;
    while (b != 0) { TopologyUInt remainder = a % b; a = b; b = remainder; }
    return (PlanTopologyRational){pack(magnitude / a), pack(denominator / a), numerator < 0};
}

/* Continued-fraction comparison never forms numerator * other_denominator.
 * Such products could exceed 128 bits for valid 32-bit segment intersections. */
int topology_rational_compare(PlanTopologyRational a, PlanTopologyRational b)
{
    if (a.negative != b.negative) { return a.negative ? -1 : 1; }
    TopologyUInt an = unpack(a.numerator), ad = unpack(a.denominator);
    TopologyUInt bn = unpack(b.numerator), bd = unpack(b.denominator);
    int direction = a.negative ? -1 : 1;
    assert(ad != 0 && bd != 0);
    for (;;) {
        TopologyUInt aq = an / ad, bq = bn / bd;
        if (aq != bq) { return direction * (aq < bq ? -1 : 1); }
        TopologyUInt ar = an % ad, br = bn % bd;
        if (ar == 0 || br == 0) {
            return ar == br ? 0 : direction * (ar == 0 ? -1 : 1);
        }
        an = ad; ad = ar; bn = bd; bd = br;
        direction = -direction;
    }
}

int topology_vertex_compare(PlanTopologyVertex a, PlanTopologyVertex b)
{
    int x = topology_rational_compare(a.x, b.x);
    return x != 0 ? x : topology_rational_compare(a.y, b.y);
}

PlanTopologyVertex topology_integer_point(PlanPosition point)
{
    return (PlanTopologyVertex){topology_rational(point.x, 1), topology_rational(point.y, 1)};
}

int topology_checked_add(TopologyInt a, TopologyInt b, TopologyInt *out)
{
    return !__builtin_add_overflow(a, b, out);
}

int topology_checked_multiply(TopologyInt a, TopologyInt b, TopologyInt *out)
{
    return !__builtin_mul_overflow(a, b, out);
}

TopologyInt topology_cross(int64_t ax, int64_t ay, int64_t bx, int64_t by)
{
    /* Callers supply differences of int32 coordinates, at most 2^32-1.
     * Each product is <2^64 and the signed difference is <2^65. */
    return (TopologyInt)ax * by - (TopologyInt)ay * bx;
}
