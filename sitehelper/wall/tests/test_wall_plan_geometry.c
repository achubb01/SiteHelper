#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "wall_plan_geometry.h"
#include "wall_query.h"

static int near(double a,double b){return fabs(a-b)<1e-6;}

static Wall wall_with(WallPlanSegment segment,int thickness,WallPlanAlignment alignment)
{
    Wall wall={0};
    assert(wall_set_plan_segment(&wall,segment));
    assert(wall_set_plan_specification(&wall,(WallPlanSpecification){thickness,alignment}));
    return wall;
}

static void test_horizontal_vertical_and_alignment(void)
{
    Wall wall=wall_with((WallPlanSegment){{0,0},{1000,0}},90,WALL_PLAN_ALIGNMENT_CENTER);
    WallPlanGeometry g; assert(wall_plan_geometry_build(&wall.definition,&g));
    assert(near(g.left_start.x,0)&&near(g.left_start.y,45));
    assert(near(g.right_start.x,0)&&near(g.right_start.y,-45));
    WallPlanSegment before=wall.definition.segment;
    assert(wall_set_plan_specification(&wall,(WallPlanSpecification){140,WALL_PLAN_ALIGNMENT_CENTER}));
    assert(wall.definition.segment.start.x==before.start.x && wall.definition.segment.start.y==before.start.y);
    assert(wall.definition.segment.end.x==before.end.x && wall.definition.segment.end.y==before.end.y);
    assert(wall_plan_geometry_build(&wall.definition,&g)); assert(near(g.left_start.y,70));

    wall=wall_with((WallPlanSegment){{10,20},{10,1020}},100,WALL_PLAN_ALIGNMENT_LEFT_FACE);
    assert(wall_plan_geometry_build(&wall.definition,&g));
    assert(near(g.left_start.x,10)&&near(g.right_start.x,110));
}

static void test_diagonal_and_body_hit(void)
{
    Wall wall=wall_with((WallPlanSegment){{0,0},{300,400}},100,WALL_PLAN_ALIGNMENT_CENTER);
    WallPlanGeometry g; assert(wall_plan_geometry_build(&wall.definition,&g));
    assert(near(hypot(g.left_start.x,g.left_start.y),50.0));
    assert(wall_plan_geometry_distance(&g,(PlanPoint){150,200})==0.0);
    assert(wall_plan_geometry_distance(&g,(PlanPoint){210,155})>0.0);

    Wall walls[2]={
        wall_with((WallPlanSegment){{0,0},{1000,0}},90,WALL_PLAN_ALIGNMENT_CENTER),
        wall_with((WallPlanSegment){{500,-500},{500,500}},90,WALL_PLAN_ALIGNMENT_CENTER)
    };
    BuildStructure structure={.walls=walls,.wall_count=2,.wall_capacity=2};
    walls[0].id=10;walls[1].id=20;
    assert(wall_plan_find_wall_at_position(&structure,(PlanPoint){500,0},0)==20);
    assert(wall_plan_find_wall_at_position(&structure,(PlanPoint){500,40},0)==20);
    assert(wall_plan_find_wall_at_position(&structure,(PlanPoint){250,40},0)==10);
}

static void test_l_and_t_junction_bodies_meet(void)
{
    Wall horizontal=wall_with((WallPlanSegment){{0,0},{1000,0}},90,WALL_PLAN_ALIGNMENT_CENTER);
    Wall l_leg=wall_with((WallPlanSegment){{1000,0},{1000,800}},90,WALL_PLAN_ALIGNMENT_CENTER);
    Wall t_leg=wall_with((WallPlanSegment){{500,0},{500,800}},90,WALL_PLAN_ALIGNMENT_CENTER);
    WallPlanGeometry a,b,c;
    assert(wall_plan_geometry_build(&horizontal.definition,&a));
    assert(wall_plan_geometry_build(&l_leg.definition,&b));
    assert(wall_plan_geometry_build(&t_leg.definition,&c));
    assert(wall_plan_geometry_distance(&a,(PlanPoint){1000,0})==0.0);
    assert(wall_plan_geometry_distance(&b,(PlanPoint){1000,0})==0.0);
    assert(wall_plan_geometry_distance(&a,(PlanPoint){500,0})==0.0);
    assert(wall_plan_geometry_distance(&c,(PlanPoint){500,0})==0.0);
}

static void test_nominal_default(void)
{
    Wall wall={0}; assert(wall_set_plan_segment(&wall,(WallPlanSegment){{0,0},{1000,0}}));
    assert(wall.definition.plan_specification.thickness_mm==WALL_DEFAULT_NOMINAL_THICKNESS_MM);
    assert(wall.definition.plan_specification.alignment==WALL_PLAN_ALIGNMENT_CENTER);
}

int main(void)
{
    test_horizontal_vertical_and_alignment();
    test_diagonal_and_body_hit();
    test_l_and_t_junction_bodies_meet();
    test_nominal_default();
    puts("wall plan geometry tests passed");
    return 0;
}
