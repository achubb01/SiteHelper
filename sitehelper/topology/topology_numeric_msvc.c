#include <assert.h>
#include <limits.h>
#include <intrin.h>
#include "topology_numeric_internal.h"

_Static_assert(INT_MAX <= INT32_MAX && INT_MIN >= INT32_MIN,
    "Topology exact arithmetic supports signed 32-bit authoritative coordinates");

#if !defined(_M_X64)
#error "SiteHelper exact topology requires the MSVC x64 target"
#endif

static TopologyUInt add(TopologyUInt a,TopologyUInt b){TopologyUInt r={a.lo+b.lo,a.hi+b.hi};if(r.lo<a.lo)r.hi++;return r;}
static TopologyUInt sub(TopologyUInt a,TopologyUInt b){TopologyUInt r={a.lo-b.lo,a.hi-b.hi};if(a.lo<b.lo)r.hi--;return r;}
static TopologyUInt neg(TopologyUInt a){return add((TopologyUInt){~a.lo,~a.hi},(TopologyUInt){1,0});}
static int cmp(TopologyUInt a,TopologyUInt b){return a.hi!=b.hi?(a.hi<b.hi?-1:1):(a.lo<b.lo?-1:a.lo!=b.lo);}
static void divmod(TopologyUInt n,TopologyUInt d,TopologyUInt*q,TopologyUInt*r){assert(!topology_uint_is_zero(d));*q=(TopologyUInt){0,0};*r=(TopologyUInt){0,0};for(int i=127;i>=0;i--){uint64_t old=r->lo;r->lo<<=1;r->hi=(r->hi<<1)|(old>>63);if(i>=64)r->lo|=(n.hi>>(i-64))&1;else r->lo|=(n.lo>>i)&1;if(cmp(*r,d)>=0){*r=sub(*r,d);if(i<64)q->lo|=UINT64_C(1)<<i;else q->hi|=UINT64_C(1)<<(i-64);}}}
TopologyInt topology_int_from_i64(int64_t v){return(TopologyInt){(uint64_t)v,v<0?-1:0};}
TopologyUInt topology_uint_from_u64(uint64_t v){return(TopologyUInt){v,0};}
int topology_uint_is_zero(TopologyUInt v){return!(v.lo||v.hi);}int topology_int_is_negative(TopologyInt v){return v.hi<0;}int topology_int_is_zero(TopologyInt v){return!(v.lo||v.hi);}
TopologyUInt topology_int_as_uint(TopologyInt v){return(TopologyUInt){v.lo,(uint64_t)v.hi};}TopologyInt topology_uint_as_int(TopologyUInt v){return(TopologyInt){v.lo,(int64_t)v.hi};}TopologyInt topology_int_negate(TopologyInt v){return topology_uint_as_int(neg(topology_int_as_uint(v)));}TopologyUInt topology_int_magnitude(TopologyInt v){TopologyUInt u=topology_int_as_uint(v);return topology_int_is_negative(v)?neg(u):u;}
int topology_uint_compare(TopologyUInt a,TopologyUInt b){return cmp(a,b);}int topology_int_compare(TopologyInt a,TopologyInt b){return a.hi!=b.hi?(a.hi<b.hi?-1:1):(a.lo<b.lo?-1:a.lo!=b.lo);}
TopologyInt topology_int_add(TopologyInt a,TopologyInt b){return topology_uint_as_int(add(topology_int_as_uint(a),topology_int_as_uint(b)));}TopologyInt topology_int_subtract(TopologyInt a,TopologyInt b){return topology_uint_as_int(sub(topology_int_as_uint(a),topology_int_as_uint(b)));}TopologyUInt topology_uint_subtract(TopologyUInt a,TopologyUInt b){return sub(a,b);}TopologyUInt topology_uint_add(TopologyUInt a,TopologyUInt b){return add(a,b);}TopologyUInt topology_uint_shift_left(TopologyUInt a,unsigned n){if(n>=128)return(TopologyUInt){0,0};if(n>=64)return(TopologyUInt){0,a.lo<<(n-64)};return n?(TopologyUInt){a.lo<<n,(a.hi<<n)|(a.lo>>(64-n))}:a;}TopologyUInt topology_uint_complement(TopologyUInt a){return(TopologyUInt){~a.lo,~a.hi};}
TopologyUInt topology_uint_divide(TopologyUInt n,TopologyUInt d){TopologyUInt q,r;divmod(n,d,&q,&r);return q;}TopologyUInt topology_uint_remainder(TopologyUInt n,TopologyUInt d){TopologyUInt q,r;divmod(n,d,&q,&r);return r;}
static PlanTopologyMagnitude pack(TopologyUInt v){return(PlanTopologyMagnitude){v.lo,v.hi};}static TopologyUInt unpack(PlanTopologyMagnitude v){return(TopologyUInt){v.lo,v.hi};}
PlanTopologyRational topology_rational(TopologyInt n,TopologyUInt d){TopologyUInt m=topology_int_magnitude(n),a=m,b=d;while(!topology_uint_is_zero(b)){TopologyUInt r=topology_uint_remainder(a,b);a=b;b=r;}return(PlanTopologyRational){pack(topology_uint_divide(m,a)),pack(topology_uint_divide(d,a)),topology_int_is_negative(n)};}
int topology_rational_compare(PlanTopologyRational a,PlanTopologyRational b){if(a.negative!=b.negative)return a.negative?-1:1;TopologyUInt an=unpack(a.numerator),ad=unpack(a.denominator),bn=unpack(b.numerator),bd=unpack(b.denominator);int dir=a.negative?-1:1;for(;;){int c=cmp(topology_uint_divide(an,ad),topology_uint_divide(bn,bd));if(c)return dir*c;TopologyUInt ar=topology_uint_remainder(an,ad),br=topology_uint_remainder(bn,bd);if(topology_uint_is_zero(ar)||topology_uint_is_zero(br))return topology_uint_is_zero(ar)&&topology_uint_is_zero(br)?0:dir*(topology_uint_is_zero(ar)?-1:1);an=ad;ad=ar;bn=bd;bd=br;dir=-dir;}}
int topology_vertex_compare(PlanTopologyVertex a,PlanTopologyVertex b){int c=topology_rational_compare(a.x,b.x);return c?c:topology_rational_compare(a.y,b.y);}PlanTopologyVertex topology_integer_point(PlanPosition p){return(PlanTopologyVertex){topology_rational(topology_int_from_i64(p.x),topology_uint_from_u64(1)),topology_rational(topology_int_from_i64(p.y),topology_uint_from_u64(1))};}
int topology_checked_add(TopologyInt a,TopologyInt b,TopologyInt*out){TopologyInt r=topology_int_add(a,b);*out=r;return!(topology_int_is_negative(a)==topology_int_is_negative(b)&&topology_int_is_negative(r)!=topology_int_is_negative(a));}
int topology_checked_multiply(TopologyInt a,TopologyInt b,TopologyInt*out){TopologyUInt x=topology_int_magnitude(a),y=topology_int_magnitude(b),r={0,0};uint64_t high; r.lo=_umul128(x.lo,y.lo,&high);r.hi=high;int overflow=x.hi&&y.hi;if(x.hi){uint64_t h;uint64_t l=_umul128(x.hi,y.lo,&h);if(h||l>UINT64_MAX-r.hi)overflow=1;r.hi+=l;}if(y.hi){uint64_t h;uint64_t l=_umul128(x.lo,y.hi,&h);if(h||l>UINT64_MAX-r.hi)overflow=1;r.hi+=l;}int negative=topology_int_is_negative(a)!=topology_int_is_negative(b);if((!negative&&(r.hi&UINT64_C(0x8000000000000000)))||(negative&&(r.hi>UINT64_C(0x8000000000000000)||(r.hi==UINT64_C(0x8000000000000000)&&r.lo))))overflow=1;*out=topology_uint_as_int(r);if(negative)*out=topology_int_negate(*out);return !overflow;}
TopologyInt topology_cross(int64_t ax,int64_t ay,int64_t bx,int64_t by){TopologyInt x,y;topology_checked_multiply(topology_int_from_i64(ax),topology_int_from_i64(by),&x);topology_checked_multiply(topology_int_from_i64(ay),topology_int_from_i64(bx),&y);return topology_int_subtract(x,y);}
