#ifndef CREATE_ROOF_COMMAND_H
#define CREATE_ROOF_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId storey_id;
    PlanPosition *support_vertices;
    size_t support_vertex_count;
    RoofPortionGeneration generation;
    int64_t slope_ppm;
    int reference_z_mm;
    RoofDirection direction;
    RoofSingleSlopeReference single_slope_reference;
} CreateRoofCommand;

/* Owns a deep copy of the initial portion support polygon. Output is zero or a
 * previous valid command; failure leaves it unchanged and success replaces it. */
int create_roof_command_create(DomainId storey_id, const RoofPortionSpec *initial_portion,
    CreateRoofCommand *command);
int create_roof_command_clone(const CreateRoofCommand *source, CreateRoofCommand *output);
void create_roof_command_destroy(CreateRoofCommand *command);
int create_roof_command_execute(SiteHelperProject *project, const CreateRoofCommand *command,
    DomainId *roof_id, DomainId *portion_id);
int create_roof_command_undo(SiteHelperProject *project, const CreateRoofCommand *command,
    DomainId roof_id, DomainId portion_id);
int create_roof_command_redo(SiteHelperProject *project, const CreateRoofCommand *command,
    DomainId roof_id, DomainId portion_id);

#endif
