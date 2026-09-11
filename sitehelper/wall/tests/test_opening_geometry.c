#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "test_support.h"
#include "wall_query.h"

static BuildSettings settings_for(int mode)
{
    return (BuildSettings){.stud_height=2400,.stud_width=35,.stud_depth=90,
        .stud_spacing=450,.nog_spacing=900,.stud_spacing_mode=(StudSpacingMode)mode,
        .opening_width_allowance=20,.opening_height_allowance=20};
}

static WallOpeningProposal proposal(Opening o)
{
    return (WallOpeningProposal){.type=o.type,.frame_position=o.frame_position,
        .frame_bottom=o.frame_bottom,.width=o.width,.height=o.height,
        .custom_allowance=o.custom_allowance,.width_allowance=o.width_allowance,
        .height_allowance=o.height_allowance};
}

/* No failure injection: a VALID proposal must be insertable AND generatable.
 * Check replacement too, with self-exclusion and unchanged identity/order. */
static void assert_valid_generates(Wall *wall, const BuildSettings *s, Opening o)
{
    WallOpeningProposal p = proposal(o);
    assert(wall_validate_opening(wall,s,&p).code == WALL_OPENING_VALID);
    assert(wall_add_opening_definition(wall,s,&o));
    assert(wall_generate(wall,s));
    assert(wall_apply_opening_definition(wall,s,o.id,&o));
    test_assert_opening_equal(&o,&wall->definition.openings[wall->definition.opening_count-1]);
    Wall copy;
    test_clone_wall_definition(wall,&copy);
    assert(wall_generate(&copy,s));
    test_assert_framing_semantically_equal(&copy.framing,&wall->framing);
    wall_destroy(&copy);
}

static void test_boundary_matrix(void)
{
    for (int mode=STUD_SPACING_EVEN; mode<=STUD_SPACING_MAXIMISE; mode++) {
        BuildSettings s=settings_for(mode);
        Opening cases[] = {
            {.id=1,.type=OPENING_DOOR,.frame_position=105,.width=820,.height=2020},
            {.id=1,.type=OPENING_DOOR,.frame_position=105,.frame_bottom=100,.width=820,.height=2020},
            {.id=1,.type=OPENING_DOOR,.frame_position=105,.width=820,.height=2380}, /* Top exactly wall height. */
            {.id=1,.type=OPENING_DOOR,.frame_position=3055,.width=820,.height=2020}, /* Right legal limit. */
            {.id=1,.type=OPENING_WINDOW,.frame_position=1000,.frame_bottom=700,.width=820,.height=1000},
            {.id=1,.type=OPENING_WINDOW,.frame_position=105,.frame_bottom=35,.width=15,.height=2310}, /* No cripples. */
            {.id=1,.type=OPENING_WINDOW,.frame_position=105,.frame_bottom=36,.width=15,.height=2308}, /* 1 mm above/below. */
            {.id=1,.type=OPENING_WINDOW,.frame_position=1000,.frame_bottom=800,.width=820,.height=1530,
             .custom_allowance=true,.width_allowance=12,.height_allowance=35}, /* Header exactly fits. */
            {.id=1,.type=OPENING_WINDOW,.frame_position=1000,.frame_bottom=700,.width=820,.height=1000,
             .custom_allowance=true,.width_allowance=-10,.height_allowance=-10}
        };
        for (size_t i=0;i<sizeof cases/sizeof *cases;i++) {
            Wall wall={.id=42,.definition.segment.end.x=4000};
            assert_valid_generates(&wall,&s,cases[i]);
            WallOpeningFrameGeometry g;
            assert(wall_opening_frame_geometry(&cases[i],&s,&g));
            assert(wall_find_opening_at_position(&wall,&s,(WallLocalPosition){(int)g.left_u,(int)g.bottom_z})==1);
            assert(wall_find_opening_at_position(&wall,&s,(WallLocalPosition){(int)g.right_u,(int)g.top_z})==1);
            assert(wall_find_opening_at_position(&wall,&s,(WallLocalPosition){(int)g.left_u,(int)g.bottom_z-1})==0);
            assert(wall_find_opening_at_position(&wall,&s,(WallLocalPosition){(int)g.right_u+1,(int)g.top_z})==0);
            size_t trimmers=0, lower=0, upper=0;
            for (size_t j=0;j<wall.framing.stud_count;j++) {
                const Timber *t=&wall.framing.studs[j];
                if (t->details.stud.type==STUD_TRIMMER) {
                    assert(t->position.z==0 && t->length==g.top_z);
                    trimmers++;
                }
                if (t->details.stud.type==STUD_CRIPPLE) {
                    assert(t->length>0);
                    if (t->position.z==0) {
                        assert(t->length==g.bottom_z-s.stud_width);lower++;
                    } else {
                        assert(t->position.z==g.top_z+s.stud_width);
                        assert((int64_t)t->position.z+t->length==s.stud_height);upper++;
                    }
                }
            }
            assert(trimmers==2);
            for (size_t j=0;j<wall.framing.member_count;j++) {
                const Timber *t=&wall.framing.members[j];
                if(t->type==TIMBER_SILL) assert((int64_t)t->position.z+t->width==g.bottom_z);
                if(t->type==TIMBER_HEADER) assert(t->position.z==g.top_z);
            }
            if(cases[i].type==OPENING_WINDOW) {
                assert((lower==0)==(g.bottom_z==s.stud_width));
                assert((upper==0)==(g.top_z+s.stud_width==s.stud_height));
            }
            wall_destroy(&wall);
        }
    }
}

static void test_invalid_geometry_rejected_before_generation(void)
{
    BuildSettings s=settings_for(STUD_SPACING_MAXIMISE);
    Opening base={.id=1,.type=OPENING_WINDOW,.frame_position=1000,.frame_bottom=700,.width=820,.height=1000};
    Opening invalid[9];
    for(size_t i=0;i<9;i++) invalid[i]=base;
    invalid[0].frame_bottom=0;
    invalid[1].frame_bottom=34;
    invalid[2].width=14; /* Effective 34 < one 35 mm support. */
    invalid[3].frame_bottom=1380; /* Clear top at wall height, no header room. */
    invalid[4].frame_bottom=1346; /* Header exceeds height by 1 mm. */
    invalid[5].width=INT_MAX;
    invalid[6].frame_position=INT_MAX;
    invalid[7].height=0;
    invalid[8].frame_bottom=INT_MAX;
    WallOpeningValidationCode codes[]={WALL_OPENING_TOO_CLOSE_TO_BASELINE,WALL_OPENING_TOO_CLOSE_TO_BASELINE,
        WALL_OPENING_INVALID_DIMENSIONS,WALL_OPENING_INVALID_HEIGHT,WALL_OPENING_INVALID_HEIGHT,
        WALL_OPENING_INVALID_DIMENSIONS,WALL_OPENING_TOO_CLOSE_TO_RIGHT_END,
        WALL_OPENING_INVALID_DIMENSIONS,WALL_OPENING_INVALID_HEIGHT};
    Wall wall={.id=42,.definition.segment.end.x=4000};
    assert_valid_generates(&wall,&s,base);
    for(size_t i=0;i<9;i++) {
        Wall empty={.definition.segment=wall.definition.segment};
        WallOpeningProposal p=proposal(invalid[i]);
        assert(wall_validate_opening(&empty,&s,&p).code==codes[i]);
        Wall before=wall;
        assert(!wall_apply_opening_definition(&wall,&s,1,&invalid[i]));
        test_assert_opening_equal(&base,&wall.definition.openings[0]);
        assert(before.framing.studs==wall.framing.studs && before.definition.openings==wall.definition.openings);
        /* Direct regeneration must enforce the very same validator. */
        Wall borrowed={.definition={.segment=wall.definition.segment,.openings=&invalid[i],.opening_count=1}};
        assert(!wall_generate(&borrowed,&s));
        assert(borrowed.framing.studs==NULL);
    }
    WallOpeningProposal p=proposal(base);
    s.stud_spacing_mode=(StudSpacingMode)99;
    assert(wall_validate_opening(&wall,&s,&p).code==WALL_OPENING_INVALID_ARGUMENT);
    wall_destroy(&wall);
}

static void test_multiple_openings_and_overlap(void)
{
    BuildSettings s=settings_for(STUD_SPACING_EVEN);
    Wall wall={.id=42,.definition.segment.end.x=6000};
    Opening cases[]={
        {.id=3,.type=OPENING_WINDOW,.frame_position=4300,.frame_bottom=35,.width=820,.height=1000},
        {.id=1,.type=OPENING_DOOR,.frame_position=105,.width=820,.height=2020},
        {.id=2,.type=OPENING_WINDOW,.frame_position=2200,.frame_bottom=700,.width=820,.height=1000}};
    for(size_t i=0;i<3;i++) assert_valid_generates(&wall,&s,cases[i]);
    Opening overlap=cases[1];overlap.id=4;overlap.frame_position=2200;
    WallOpeningProposal p=proposal(overlap);
    WallOpeningValidation v=wall_validate_opening(&wall,&s,&p);
    assert(v.code==WALL_OPENING_OVERLAPS_OPENING && v.conflicting_opening_id==2);
    for(size_t i=0;i<3;i++) assert(wall.definition.openings[i].id==cases[i].id);
    wall_destroy(&wall);
}

static void test_large_representable_geometry(void)
{
    BuildSettings s=settings_for(STUD_SPACING_EVEN);
    s.stud_height=INT_MAX-100;s.stud_spacing=INT_MAX;s.nog_spacing=INT_MAX;
    Wall wall={.id=42,.definition.segment.end.x=INT_MAX};
    Opening o={.id=1,.type=OPENING_WINDOW,.frame_position=INT_MAX-2000,
        .frame_bottom=INT_MAX-2000,.width=800,.height=1000};
    assert_valid_generates(&wall,&s,o);
    wall_destroy(&wall);
    /* Multiplication overflow previously occurred in even-position generation. */
    s=settings_for(STUD_SPACING_EVEN);s.stud_spacing=50000;
    wall=(Wall){.id=42,.definition.segment.end.x=50000000};
    o=(Opening){.id=1,.type=OPENING_WINDOW,.frame_position=1000,.frame_bottom=700,.width=200000,.height=1000};
    assert_valid_generates(&wall,&s,o);
    wall_destroy(&wall);
}

static void test_validation_generation_domain_matrix(void)
{
    size_t valid=0,rejected=0;
    for(int mode=STUD_SPACING_EVEN;mode<=STUD_SPACING_MAXIMISE;mode++)
    for(int member=3;member<=7;member+=4)
    for(int spacing=23;spacing<=250;spacing+=227)
    for(int allowance=-1;allowance<=4;allowance++)
    for(int type=OPENING_DOOR;type<=OPENING_WINDOW;type++) {
        BuildSettings s=settings_for(mode);
        s.stud_width=member;s.stud_height=120;s.stud_spacing=spacing;s.nog_spacing=47;
        s.opening_width_allowance=allowance;s.opening_height_allowance=allowance;
        int widths[]={1,member-1,member,member+1,60,1000,INT_MAX};
        int bottoms[]={0,member-1,member,member+1,60,120,INT_MAX};
        for(size_t wi=0;wi<sizeof widths/sizeof *widths;wi++)
        for(size_t bi=0;bi<sizeof bottoms/sizeof *bottoms;bi++) {
            int heights[]={1,member-1,50,120-bottoms[bi]-member-1,
                120-bottoms[bi]-member,120-bottoms[bi]-member+1,INT_MAX};
            for(size_t hi=0;hi<sizeof heights/sizeof *heights;hi++) {
                Opening o={.id=1,.type=(OpeningType)type,.frame_position=3*member,
                    .frame_bottom=bottoms[bi],.width=widths[wi],.height=heights[hi]};
                Wall wall={.id=42,.definition.segment.end.x=600};
                WallOpeningProposal p=proposal(o);
                WallOpeningValidation v=wall_validate_opening(&wall,&s,&p);
                /* Borrow a definition to also check direct generation on every
                 * rejected case, without insertion masking disagreements. */
                wall.definition.openings=&o;wall.definition.opening_count=1;
                assert(wall_generate(&wall,&s)==(v.code==WALL_OPENING_VALID));
                if(v.code==WALL_OPENING_VALID) valid++;else rejected++;
                wall_framing_destroy(&wall.framing);
            }
        }
    }
    assert(valid>1000 && rejected>1000);
    printf("validation/generation matrix: %zu valid, %zu rejected\n",valid,rejected);
}

int main(void)
{
    test_boundary_matrix();test_invalid_geometry_rejected_before_generation();
    test_multiple_openings_and_overlap();test_large_representable_geometry();
    test_validation_generation_domain_matrix();
    puts("opening geometry invariant tests passed");
    return 0;
}
