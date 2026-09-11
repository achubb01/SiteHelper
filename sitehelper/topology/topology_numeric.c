#include <assert.h>
#include <limits.h>
#include "topology_numeric_internal.h"

_Static_assert(INT_MAX <= INT32_MAX && INT_MIN >= INT32_MIN,
    "Topology exact arithmetic supports signed 32-bit authoritative coordinates");
static PlanTopologyMagnitude pack(TopologyUInt value){return(PlanTopologyMagnitude){(uint64_t)value,(uint64_t)(value>>64)};}
static TopologyUInt unpack(PlanTopologyMagnitude value){return((TopologyUInt)value.hi<<64)|value.lo;}
PlanTopologyRational topology_rational(TopologyInt n,TopologyUInt d){assert(d!=0);TopologyUInt m=n<0?0-(TopologyUInt)n:(TopologyUInt)n,a=m,b=d;while(b){TopologyUInt r=a%b;a=b;b=r;}return(PlanTopologyRational){pack(m/a),pack(d/a),n<0};}
int topology_rational_compare(PlanTopologyRational a,PlanTopologyRational b){if(a.negative!=b.negative)return a.negative?-1:1;TopologyUInt an=unpack(a.numerator),ad=unpack(a.denominator),bn=unpack(b.numerator),bd=unpack(b.denominator);int dir=a.negative?-1:1;for(;;){TopologyUInt aq=an/ad,bq=bn/bd;if(aq!=bq)return dir*(aq<bq?-1:1);TopologyUInt ar=an%ad,br=bn%bd;if(!ar||!br)return ar==br?0:dir*(ar?1:-1);an=ad;ad=ar;bn=bd;bd=br;dir=-dir;}}
int topology_vertex_compare(PlanTopologyVertex a,PlanTopologyVertex b){int c=topology_rational_compare(a.x,b.x);return c?c:topology_rational_compare(a.y,b.y);} PlanTopologyVertex topology_integer_point(PlanPosition p){return(PlanTopologyVertex){topology_rational(p.x,1),topology_rational(p.y,1)};}
int topology_checked_add(TopologyInt a,TopologyInt b,TopologyInt*out){return!__builtin_add_overflow(a,b,out);} int topology_checked_multiply(TopologyInt a,TopologyInt b,TopologyInt*out){return!__builtin_mul_overflow(a,b,out);} TopologyInt topology_cross(int64_t ax,int64_t ay,int64_t bx,int64_t by){return(TopologyInt)ax*by-(TopologyInt)ay*bx;}
