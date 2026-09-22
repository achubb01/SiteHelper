#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "wall_elevation_presentation.h"


static void test_semantic_names_are_stable_presentation_strings(void)
{
    assert(strcmp(wall_elevation_member_role_name(
        WALL_ELEVATION_MEMBER_ROLE_TRIMMER_STUD), "Trimmer stud") == 0);
    assert(strcmp(wall_elevation_member_role_name(
        WALL_ELEVATION_MEMBER_ROLE_HEADER), "Header") == 0);
    assert(strcmp(wall_elevation_member_role_name(
        WALL_ELEVATION_MEMBER_ROLE_UNKNOWN), "Framing member") == 0);
    assert(strcmp(wall_elevation_opening_type_name(OPENING_DOOR), "Door") == 0);
    assert(strcmp(wall_elevation_opening_type_name(OPENING_WINDOW), "Window") == 0);
}

static BuildSettings settings(void)
{
    return (BuildSettings){
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 600,
        .stud_spacing_mode = STUD_SPACING_EVEN,
        .nog_spacing = 1200,
        .opening_width_allowance = 10,
        .opening_height_allowance = 20
    };
}

static void test_semantic_member_stream(void)
{
    Timber studs[] = {
        {.length=2400,.width=35,.depth=90,.position={0,0},.type=TIMBER_STUD,
            .details.stud={.type=STUD_COMMON}},
        {.length=2400,.width=35,.depth=90,.position={600,0},.type=TIMBER_STUD,
            .details.stud={.type=STUD_KING}},
        {.length=2100,.width=35,.depth=90,.position={635,0},.type=TIMBER_STUD,
            .details.stud={.type=STUD_TRIMMER}},
        {.length=600,.width=35,.depth=90,.position={1000,0},.type=TIMBER_STUD,
            .details.stud={.type=STUD_CRIPPLE}}
    };
    Timber nogs[] = {
        {.length=565,.width=35,.depth=90,.position={35,800},.type=TIMBER_NOGGIN,
            .details.noggin={.bay=0}}
    };
    Timber generated[] = {
        {.length=910,.width=35,.depth=90,.position={600,2100},.type=TIMBER_HEADER},
        {.length=840,.width=35,.depth=90,.position={635,600},.type=TIMBER_SILL}
    };
    Wall wall = {
        .framing = {
            .studs=studs,.stud_count=4,
            .nogs=nogs,.nog_count=1,
            .members=generated,.member_count=2,
            .bottomplate={.length=4200,.width=35,.depth=90,.position={0,0},.type=TIMBER_PLATE},
            .topplate={.length=4200,.width=35,.depth=90,.position={0,2400},.type=TIMBER_PLATE}
        }
    };

    const WallElevationMemberRole roles[] = {
        WALL_ELEVATION_MEMBER_ROLE_BOTTOM_PLATE,
        WALL_ELEVATION_MEMBER_ROLE_TOP_PLATE,
        WALL_ELEVATION_MEMBER_ROLE_COMMON_STUD,
        WALL_ELEVATION_MEMBER_ROLE_KING_STUD,
        WALL_ELEVATION_MEMBER_ROLE_TRIMMER_STUD,
        WALL_ELEVATION_MEMBER_ROLE_CRIPPLE_STUD,
        WALL_ELEVATION_MEMBER_ROLE_NOGGIN,
        WALL_ELEVATION_MEMBER_ROLE_HEADER,
        WALL_ELEVATION_MEMBER_ROLE_SILL
    };
    const WallMemberKind kinds[] = {
        WALL_MEMBER_BOTTOM_PLATE,
        WALL_MEMBER_TOP_PLATE,
        WALL_MEMBER_STUD,
        WALL_MEMBER_STUD,
        WALL_MEMBER_STUD,
        WALL_MEMBER_STUD,
        WALL_MEMBER_NOGGIN,
        WALL_MEMBER_GENERATED,
        WALL_MEMBER_GENERATED
    };

    assert(wall_elevation_presentation_member_count(&wall) == 9);
    for (size_t i=0;i<9;i++) {
        WallElevationMember member={0};
        assert(wall_elevation_presentation_member_at(&wall,i,&member));
        assert(member.role == roles[i]);
        assert(member.selection_kind == kinds[i]);
        assert(member.source != NULL);
    }

    WallElevationMember trimmer={0};
    assert(wall_elevation_presentation_member_at(&wall,4,&trimmer));
    assert(trimmer.orientation == WALL_ELEVATION_MEMBER_VERTICAL);
    assert(trimmer.bounds.u == 635 && trimmer.bounds.z == 0);
    assert(trimmer.bounds.width == 35 && trimmer.bounds.height == 2100);

    WallElevationMember header={0};
    assert(wall_elevation_presentation_member_at(&wall,7,&header));
    assert(header.orientation == WALL_ELEVATION_MEMBER_HORIZONTAL);
    assert(header.bounds.u == 600 && header.bounds.z == 2100);
    assert(header.bounds.width == 910 && header.bounds.height == 35);

    assert(!wall_elevation_presentation_member_at(&wall,9,&header));
    assert(wall_elevation_presentation_member_count(NULL) == 0);
}

static void test_member_hit_preserves_existing_precedence(void)
{
    Timber studs[] = {
        {.length=2400,.width=35,.position={0,0},.type=TIMBER_STUD,
            .details.stud={.type=STUD_COMMON}}
    };
    Wall wall = {
        .framing = {
            .studs=studs,.stud_count=1,
            .bottomplate={.length=4200,.width=35,.position={0,0},.type=TIMBER_PLATE},
            .topplate={.length=4200,.width=35,.position={0,2400},.type=TIMBER_PLATE}
        }
    };

    WallElevationMember hit={0};
    assert(wall_elevation_presentation_find_member(&wall,(WallLocalPosition){10,10},&hit));
    assert(hit.role == WALL_ELEVATION_MEMBER_ROLE_BOTTOM_PLATE);
    assert(hit.selection_kind == WALL_MEMBER_BOTTOM_PLATE);

    assert(wall_elevation_presentation_find_member(&wall,(WallLocalPosition){10,100},&hit));
    assert(hit.role == WALL_ELEVATION_MEMBER_ROLE_COMMON_STUD);
    assert(hit.source == &studs[0]);

    assert(!wall_elevation_presentation_find_member(&wall,(WallLocalPosition){5000,5000},&hit));
}

static void test_opening_and_wall_bounds_are_view_semantics(void)
{
    BuildSettings build = settings();
    Opening openings[] = {
        {
            .id=77,.type=OPENING_WINDOW,
            .frame_position=1000,.frame_bottom=700,
            .width=800,.height=1000,
            .custom_allowance=false
        }
    };
    Wall wall = {
        .definition = {
            .segment={{0,0},{4200,0}},
            .openings=openings,.opening_count=1
        }
    };

    WallElevationRect bounds={0};
    assert(wall_elevation_presentation_bounds(&wall,&build,&bounds));
    assert(bounds.u == 0 && bounds.z == 0);
    assert(bounds.width == 4200);
    assert(bounds.height == 2435);

    WallElevationOpening opening={0};
    assert(wall_elevation_presentation_opening_count(&wall) == 1);
    assert(wall_elevation_presentation_opening_at(&wall,&build,0,&opening));
    assert(opening.opening_id == 77);
    assert(opening.type == OPENING_WINDOW);
    assert(opening.clear_bounds.u == 1000);
    assert(opening.clear_bounds.z == 700);
    assert(opening.clear_bounds.width == 810);
    assert(opening.clear_bounds.height == 1020);

    WallElevationOpening hit={0};
    assert(wall_elevation_presentation_find_opening(
        &wall,&build,(WallLocalPosition){1810,1720},&hit));
    assert(hit.opening_id == 77);
    assert(!wall_elevation_presentation_find_opening(
        &wall,&build,(WallLocalPosition){1811,1721},&hit));
}

int main(void)
{
    test_semantic_names_are_stable_presentation_strings();
    test_semantic_member_stream();
    test_member_hit_preserves_existing_precedence();
    test_opening_and_wall_bounds_are_view_semantics();
    printf("wall elevation presentation tests passed\n");
    return 0;
}
