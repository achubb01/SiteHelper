#ifndef ROOF_SOURCE_EDIT_COMMAND_H
#define ROOF_SOURCE_EDIT_COMMAND_H

#include "sitehelper_project.h"

typedef enum {
    ROOF_SOURCE_EDIT_ADD_PORTION_COMPOSED = 0,
    ROOF_SOURCE_EDIT_REMOVE_PORTION,
    ROOF_SOURCE_EDIT_SET_PORTION,
    ROOF_SOURCE_EDIT_SET_COMPOSITION,
    ROOF_SOURCE_EDIT_REMOVE_COMPOSITION,
    ROOF_SOURCE_EDIT_SET_TERMINATION,
    ROOF_SOURCE_EDIT_REMOVE_TERMINATION,
    ROOF_SOURCE_EDIT_KIND_COUNT
} RoofSourceEditKind;

typedef struct {
    PlanPosition *support_vertices;
    size_t support_vertex_count;
    RoofPortionGeneration generation;
    int64_t slope_ppm;
    int reference_z_mm;
    RoofDirection direction;
    RoofSingleSlopeReference single_slope_reference;
} RoofSourceEditOwnedPortion;

typedef struct {
    RoofSourceEditKind kind;
    DomainId roof_id;
    union {
        struct {
            DomainId existing_portion_id;
            RoofCompositionKind composition_kind;
            RoofSourceEditOwnedPortion portion;
        } add_portion;
        struct { DomainId portion_id; } remove_portion;
        struct {
            DomainId portion_id;
            RoofSourceEditOwnedPortion portion;
        } set_portion;
        RoofComposition composition;
        struct {
            DomainId first_portion_id;
            DomainId second_portion_id;
        } remove_composition;
        RoofTermination termination;
        struct {
            DomainId portion_id;
            RoofEnd end;
        } remove_termination;
    } data;
} RoofSourceEditCommand;

int roof_source_edit_command_create_add_portion(DomainId roof_id, DomainId existing_portion_id,
    RoofCompositionKind kind, const RoofPortionSpec *spec, RoofSourceEditCommand *command);
int roof_source_edit_command_create_remove_portion(DomainId roof_id, DomainId portion_id,
    RoofSourceEditCommand *command);
int roof_source_edit_command_create_set_portion(DomainId roof_id, DomainId portion_id,
    const RoofPortionSpec *spec, RoofSourceEditCommand *command);
int roof_source_edit_command_create_set_composition(DomainId roof_id, RoofComposition composition,
    RoofSourceEditCommand *command);
int roof_source_edit_command_create_remove_composition(DomainId roof_id, DomainId first_portion_id,
    DomainId second_portion_id, RoofSourceEditCommand *command);
int roof_source_edit_command_create_set_termination(DomainId roof_id, RoofTermination termination,
    RoofSourceEditCommand *command);
int roof_source_edit_command_create_remove_termination(DomainId roof_id, DomainId portion_id,
    RoofEnd end, RoofSourceEditCommand *command);

int roof_source_edit_command_clone(const RoofSourceEditCommand *source,
    RoofSourceEditCommand *output);
void roof_source_edit_command_destroy(RoofSourceEditCommand *command);

/* First execution may allocate a new portion ID for ADD_PORTION_COMPOSED. */
int roof_source_edit_command_execute(SiteHelperProject *project,
    const RoofSourceEditCommand *command, DomainId *affected_portion_id);
/* Redo never allocates a replacement identity: ADD reuses affected_portion_id. */
int roof_source_edit_command_redo(SiteHelperProject *project,
    const RoofSourceEditCommand *command, DomainId affected_portion_id);

/* Pure authoritative reconstruction used by history to prove the current roof
 * still equals the command's expected post-state before undoing it. */
int roof_source_edit_command_build_expected(const Roof *before,
    const RoofSourceEditCommand *command, DomainId affected_portion_id, Roof *expected);

#endif
