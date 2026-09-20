#include "plan_note_command.h"

#include <stdlib.h>
#include <string.h>

static char *copy_text(const char *text)
{
    if (text == NULL || text[0] == '\0') { return NULL; }
    size_t length = strlen(text);
    char *copy = malloc(length + 1);
    if (copy != NULL) { memcpy(copy, text, length + 1); }
    return copy;
}

int create_plan_note_command_create(DomainId storey_id, PlanPosition position,
    DomainId target_id, const char *text, CreatePlanNoteCommand *command)
{
    if (command == NULL || storey_id == DOMAIN_ID_INVALID || text == NULL ||
        text[0] == '\0') { return 0; }
    char *owned = copy_text(text);
    if (owned == NULL) { return 0; }
    create_plan_note_command_destroy(command);
    *command = (CreatePlanNoteCommand){storey_id, position, target_id, owned};
    return 1;
}

int create_plan_note_command_clone(const CreatePlanNoteCommand *source,
    CreatePlanNoteCommand *output)
{
    if (source == NULL || output == NULL || source == output) { return 0; }
    CreatePlanNoteCommand candidate = {0};
    if (!create_plan_note_command_create(source->storey_id, source->position,
            source->target_id, source->text, &candidate)) { return 0; }
    create_plan_note_command_destroy(output);
    *output = candidate;
    return 1;
}

void create_plan_note_command_destroy(CreatePlanNoteCommand *command)
{
    if (command == NULL) { return; }
    free(command->text);
    *command = (CreatePlanNoteCommand){0};
}

static int annotation_matches_create(const DocumentAnnotation *annotation,
    const CreatePlanNoteCommand *command, DomainId id)
{
    return annotation != NULL && command != NULL && annotation->id == id &&
        annotation->kind == DOCUMENT_ANNOTATION_NOTE &&
        annotation->anchor.storey_id == command->storey_id &&
        annotation->anchor.position.x == command->position.x &&
        annotation->anchor.position.y == command->position.y &&
        annotation->target_id == command->target_id && annotation->text != NULL &&
        command->text != NULL && strcmp(annotation->text, command->text) == 0;
}

int create_plan_note_command_execute(SiteHelperProject *project,
    const CreatePlanNoteCommand *command, DomainId *annotation_id)
{
    if (annotation_id == NULL) { return 0; }
    *annotation_id = DOMAIN_ID_INVALID;
    if (project == NULL || command == NULL || command->text == NULL) { return 0; }
    DomainId id = sitehelper_project_add_plan_note(project, command->storey_id,
        command->position, command->target_id, command->text);
    if (id == DOMAIN_ID_INVALID) { return 0; }
    *annotation_id = id;
    return 1;
}

int create_plan_note_command_redo(SiteHelperProject *project,
    const CreatePlanNoteCommand *command, DomainId annotation_id)
{
    if (project == NULL || command == NULL || annotation_id == DOMAIN_ID_INVALID ||
        command->text == NULL) { return 0; }
    DocumentAnnotation annotation = {
        .id = annotation_id, .kind = DOCUMENT_ANNOTATION_NOTE,
        .anchor = {.storey_id = command->storey_id, .position = command->position},
        .target_id = command->target_id, .text = command->text
    };
    return sitehelper_project_insert_annotation(project, &annotation);
}

int create_plan_note_command_undo(SiteHelperProject *project,
    const CreatePlanNoteCommand *command, DomainId annotation_id)
{
    const DocumentAnnotation *annotation = sitehelper_project_find_annotation_by_id_const(
        project, annotation_id);
    return annotation_matches_create(annotation, command, annotation_id) &&
        sitehelper_project_remove_annotation_by_id(project, annotation_id);
}

int edit_plan_note_command_create(DomainId annotation_id, DomainId storey_id,
    PlanPosition position, DomainId target_id, const char *text,
    EditPlanNoteCommand *command)
{
    if (command == NULL || annotation_id == DOMAIN_ID_INVALID ||
        storey_id == DOMAIN_ID_INVALID || text == NULL || text[0] == '\0') { return 0; }
    char *owned = copy_text(text);
    if (owned == NULL) { return 0; }
    edit_plan_note_command_destroy(command);
    *command = (EditPlanNoteCommand){annotation_id, storey_id, position, target_id, owned};
    return 1;
}

int edit_plan_note_command_clone(const EditPlanNoteCommand *source,
    EditPlanNoteCommand *output)
{
    if (source == NULL || output == NULL || source == output) { return 0; }
    EditPlanNoteCommand candidate = {0};
    if (!edit_plan_note_command_create(source->annotation_id, source->storey_id,
            source->position, source->target_id, source->text, &candidate)) { return 0; }
    edit_plan_note_command_destroy(output);
    *output = candidate;
    return 1;
}

void edit_plan_note_command_destroy(EditPlanNoteCommand *command)
{
    if (command == NULL) { return; }
    free(command->text);
    *command = (EditPlanNoteCommand){0};
}

int edit_plan_note_command_execute(SiteHelperProject *project,
    const EditPlanNoteCommand *command)
{
    return project != NULL && command != NULL && command->text != NULL &&
        sitehelper_project_update_plan_note(project, command->annotation_id,
            command->storey_id, command->position, command->target_id, command->text);
}

int delete_plan_note_command_create(DomainId annotation_id,
    DeletePlanNoteCommand *command)
{
    if (annotation_id == DOMAIN_ID_INVALID || command == NULL) { return 0; }
    *command = (DeletePlanNoteCommand){annotation_id};
    return 1;
}

int delete_plan_note_command_execute(SiteHelperProject *project,
    const DeletePlanNoteCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_remove_annotation_by_id(project, command->annotation_id);
}
