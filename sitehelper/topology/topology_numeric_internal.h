#ifndef TOPOLOGY_NUMERIC_INTERNAL_H
#define TOPOLOGY_NUMERIC_INTERNAL_H

#include "plan_topology.h"

/* Private compiler extension. Public exact values use portable 64-bit limbs. */
__extension__ typedef __int128 TopologyInt;
__extension__ typedef unsigned __int128 TopologyUInt;

PlanTopologyRational topology_rational(TopologyInt numerator, TopologyUInt denominator);
int topology_rational_compare(PlanTopologyRational a, PlanTopologyRational b);
int topology_vertex_compare(PlanTopologyVertex a, PlanTopologyVertex b);
PlanTopologyVertex topology_integer_point(PlanPosition point);
int topology_checked_add(TopologyInt a, TopologyInt b, TopologyInt *out);
int topology_checked_multiply(TopologyInt a, TopologyInt b, TopologyInt *out);
TopologyInt topology_cross(int64_t ax, int64_t ay, int64_t bx, int64_t by);

#endif
