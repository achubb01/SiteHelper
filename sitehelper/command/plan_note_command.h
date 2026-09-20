#ifndef PLAN_NOTE_COMMAND_H
#define PLAN_NOTE_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId storey_id;
    PlanPosition position;
    DomainId target_id;
    char *text;
} CreatePlanNoteCommand;

typedef struct {
    DomainId annotation_id;
    DomainId storey_id;
    PlanPosition position;
    DomainId target_id;
    char *text;
} EditPlanNoteCommand;

typedef struct {
    DomainId annotation_id;
} DeletePlanNoteCommand;

int create_plan_note_command_create(DomainId storey_id, PlanPosition position,
    DomainId target_id, const char *text, CreatePlanNoteCommand *command);
int create_plan_note_command_clone(const CreatePlanNoteCommand *source,
    CreatePlanNoteCommand *output);
void create_plan_note_command_destroy(CreatePlanNoteCommand *command);
int create_plan_note_command_execute(SiteHelperProject *project,
    const CreatePlanNoteCommand *command, DomainId *annotation_id);
int create_plan_note_command_redo(SiteHelperProject *project,
    const CreatePlanNoteCommand *command, DomainId annotation_id);
int create_plan_note_command_undo(SiteHelperProject *project,
    const CreatePlanNoteCommand *command, DomainId annotation_id);

int edit_plan_note_command_create(DomainId annotation_id, DomainId storey_id,
    PlanPosition position, DomainId target_id, const char *text,
    EditPlanNoteCommand *command);
int edit_plan_note_command_clone(const EditPlanNoteCommand *source,
    EditPlanNoteCommand *output);
void edit_plan_note_command_destroy(EditPlanNoteCommand *command);
int edit_plan_note_command_execute(SiteHelperProject *project,
    const EditPlanNoteCommand *command);

int delete_plan_note_command_create(DomainId annotation_id,
    DeletePlanNoteCommand *command);
int delete_plan_note_command_execute(SiteHelperProject *project,
    const DeletePlanNoteCommand *command);

#endif
