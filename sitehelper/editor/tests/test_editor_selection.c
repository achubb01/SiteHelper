#include <assert.h>
#include <stdio.h>

#include "editor_selection.h"

static void assert_empty(const EditorSelection *selection)
{
    assert(editor_selection_is_empty(selection));
    assert(selection->kind == EDITOR_SELECTION_NONE);
    assert(selection->scope == EDITOR_SELECTION_SCOPE_NONE);
    assert(selection->wall_id == DOMAIN_ID_INVALID && selection->opening_id == DOMAIN_ID_INVALID);
    assert(selection->slab_id == DOMAIN_ID_INVALID && selection->slab_feature_index == SIZE_MAX);
    assert(wall_selection_is_empty(&selection->wall_member));
    assert(selection->wall_member.timber.length == 0);
    assert(!editor_selection_matches_scope(selection, EDITOR_SELECTION_SCOPE_NONE));
    assert(!editor_selection_matches_scope(selection, EDITOR_SELECTION_SCOPE_PLAN));
}

static Timber make_test_stud(void)
{
    return (Timber){
        .length = 2400,
        .depth = 90,
        .width = 90,

        .position = {
            .u = 100,
            .z = 0
        },

        .type = TIMBER_STUD,

        .details.stud = {
            .type = STUD_COMMON
        }
    };
}

static void test_selection_initialises_empty(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );
    assert_empty(&selection);

    assert(
        editor_selection_is_empty(
            &selection
        )
    );
}

static void test_selection_can_store_wall_member(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    Timber stud =
        make_test_stud();

    editor_selection_set_wall_member(
        &selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
        10,
        WALL_MEMBER_STUD,
        &stud
    );
    assert(selection.kind == EDITOR_SELECTION_WALL_MEMBER);
    assert(selection.scope == EDITOR_SELECTION_SCOPE_WALL_ELEVATION);
    editor_selection_set_wall_member(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
        10, WALL_MEMBER_STUD, &selection.wall_member.timber);
    assert(selection.wall_member.timber.length == stud.length);
    assert(editor_selection_get_wall_member(&selection, EDITOR_SELECTION_SCOPE_PLAN, 10) == NULL);

    assert(
        !editor_selection_is_empty(
            &selection
        )
    );

    assert(
        editor_selection_get_wall_member(
            &selection,
            EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
            10
        ) != NULL
    );
}

static void test_wall_member_selection_is_scoped_to_wall(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    Timber stud =
        make_test_stud();

    editor_selection_set_wall_member(
        &selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
        10,
        WALL_MEMBER_STUD,
        &stud
    );

    assert(
        editor_selection_get_wall_member(
            &selection,
            EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
            11
        ) == NULL
    );
}

static void test_selection_can_be_cleared(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    Timber stud =
        make_test_stud();

    editor_selection_set_wall_member(
        &selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
        10,
        WALL_MEMBER_STUD,
        &stud
    );

    editor_selection_clear(
        &selection
    );
    assert_empty(&selection);

    assert(
        editor_selection_is_empty(
            &selection
        )
    );
}

static void test_invalid_wall_clears_selection(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    Timber stud =
        make_test_stud();

    editor_selection_set_wall_member(
        &selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
        DOMAIN_ID_INVALID,
        WALL_MEMBER_STUD,
        &stud
    );

    assert(
        editor_selection_is_empty(
            &selection
        )
    );
}

static void test_kinds_scopes_and_invalid_setters(void)
{
    EditorSelection selection;
    Timber stud = make_test_stud();
    editor_selection_set_wall(&selection, EDITOR_SELECTION_SCOPE_PLAN, 42);
    assert(selection.kind == EDITOR_SELECTION_WALL && selection.scope == EDITOR_SELECTION_SCOPE_PLAN);
    assert(selection.wall_id == 42 && selection.opening_id == DOMAIN_ID_INVALID);
    editor_selection_set_opening(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 42, 43);
    assert(selection.kind == EDITOR_SELECTION_OPENING && selection.scope == EDITOR_SELECTION_SCOPE_WALL_ELEVATION);
    assert(selection.wall_id == 42 && selection.opening_id == 43);
    assert(wall_selection_is_empty(&selection.wall_member));
    /* Container orthogonality: these combinations are representable, even
     * though today's hit-test paths do not create them. */
    editor_selection_set_opening(&selection, EDITOR_SELECTION_SCOPE_PLAN, 42, 43);
    assert(editor_selection_matches_scope(&selection, EDITOR_SELECTION_SCOPE_PLAN));
    editor_selection_set_wall_member(&selection, EDITOR_SELECTION_SCOPE_PLAN, 42, WALL_MEMBER_STUD, &stud);
    assert(editor_selection_get_wall_member(&selection, EDITOR_SELECTION_SCOPE_PLAN, 42));
    assert(!editor_selection_get_wall_member(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 42));
    editor_selection_set_wall(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 42);
    assert(selection.kind == EDITOR_SELECTION_WALL && selection.scope == EDITOR_SELECTION_SCOPE_WALL_ELEVATION);
    assert(selection.wall_member.timber.length == 0);

    EditorSelectionScope invalid[] = {EDITOR_SELECTION_SCOPE_NONE, (EditorSelectionScope)-1, (EditorSelectionScope)99};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        editor_selection_set_wall(&selection, EDITOR_SELECTION_SCOPE_PLAN, 42);
        editor_selection_set_wall(&selection, invalid[i], 42); assert_empty(&selection);
        editor_selection_set_opening(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 42, 43);
        editor_selection_set_opening(&selection, invalid[i], 42, 43); assert_empty(&selection);
        editor_selection_set_wall_member(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 42, WALL_MEMBER_STUD, &stud);
        editor_selection_set_wall_member(&selection, invalid[i], 42, WALL_MEMBER_STUD, &stud); assert_empty(&selection);
    }
    editor_selection_set_wall(&selection, EDITOR_SELECTION_SCOPE_PLAN, DOMAIN_ID_INVALID); assert_empty(&selection);
    editor_selection_set_opening(&selection, EDITOR_SELECTION_SCOPE_PLAN, 42, DOMAIN_ID_INVALID); assert_empty(&selection);
    editor_selection_set_opening(&selection, EDITOR_SELECTION_SCOPE_PLAN, DOMAIN_ID_INVALID, 43); assert_empty(&selection);
    editor_selection_set_wall_member(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 0, WALL_MEMBER_STUD, &stud); assert_empty(&selection);
    editor_selection_set_wall_member(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 42, WALL_MEMBER_NONE, &stud); assert_empty(&selection);
    editor_selection_set_wall_member(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 42, (WallMemberKind)99, &stud); assert_empty(&selection);
    editor_selection_set_wall_member(&selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, 42, WALL_MEMBER_STUD, NULL); assert_empty(&selection);
    editor_selection_set_wall(NULL, EDITOR_SELECTION_SCOPE_PLAN, 42);
    editor_selection_set_opening(NULL, EDITOR_SELECTION_SCOPE_PLAN, 42, 43);
    editor_selection_set_wall_member(NULL, EDITOR_SELECTION_SCOPE_PLAN, 42, WALL_MEMBER_STUD, &stud);
    editor_selection_set_slab(NULL, EDITOR_SELECTION_SCOPE_PLAN, 42);
    editor_selection_set_slab_feature(NULL, EDITOR_SELECTION_SCOPE_PLAN, 42,
        EDITOR_SELECTION_SLAB_REGION, 0);
    assert(!editor_selection_matches_scope(NULL, EDITOR_SELECTION_SCOPE_PLAN));
}

static void test_slab_selection_is_value_only(void)
{
    EditorSelection selection;
    editor_selection_set_slab(&selection,EDITOR_SELECTION_SCOPE_PLAN,70);
    assert(selection.kind==EDITOR_SELECTION_SLAB&&selection.slab_id==70);
    assert(selection.slab_feature_index==SIZE_MAX&&selection.wall_id==DOMAIN_ID_INVALID);
    editor_selection_set_slab_feature(&selection,EDITOR_SELECTION_SCOPE_PLAN,70,
        EDITOR_SELECTION_SLAB_PENETRATION,0);
    assert(selection.kind==EDITOR_SELECTION_SLAB_PENETRATION&&selection.slab_id==70);
    assert(selection.slab_feature_index==0);
    editor_selection_set_slab_feature(&selection,EDITOR_SELECTION_SCOPE_PLAN,70,
        EDITOR_SELECTION_SLAB_REGION,1);
    assert(selection.kind==EDITOR_SELECTION_SLAB_REGION&&selection.slab_feature_index==1);
    editor_selection_set_slab_feature(&selection,EDITOR_SELECTION_SCOPE_PLAN,70,
        EDITOR_SELECTION_SLAB_EDGE_REBATE,2);
    assert(selection.kind==EDITOR_SELECTION_SLAB_EDGE_REBATE&&selection.slab_feature_index==2);
    editor_selection_set_slab(&selection,EDITOR_SELECTION_SCOPE_WALL_ELEVATION,70);
    assert_empty(&selection);
    editor_selection_set_slab_feature(&selection,EDITOR_SELECTION_SCOPE_PLAN,70,
        EDITOR_SELECTION_SLAB,0);
    assert_empty(&selection);
    editor_selection_set_slab_feature(&selection,EDITOR_SELECTION_SCOPE_PLAN,70,
        EDITOR_SELECTION_SLAB_REGION,SIZE_MAX);
    assert_empty(&selection);
}

int main(void)
{
    test_kinds_scopes_and_invalid_setters();
    test_selection_initialises_empty();
    test_selection_can_store_wall_member();
    test_wall_member_selection_is_scoped_to_wall();
    test_selection_can_be_cleared();
    test_invalid_wall_clears_selection();
    test_slab_selection_is_value_only();

    printf(
        "All editor selection tests passed.\n"
    );

    return 0;
}
