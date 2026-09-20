#include "plan_callout_command.h"

#include <stdlib.h>
#include <string.h>

static char *copy_text(const char *text)
{
    if (text == NULL || text[0] == '\0') { return NULL; }
    size_t length=strlen(text);
    char *copy=malloc(length+1);
    if (copy != NULL) { memcpy(copy,text,length+1); }
    return copy;
}

static int callout_matches(const DocumentPlanCallout *callout, DomainId id,
    DomainId storey_id, PlanPosition target, PlanPosition label_anchor, const char *text)
{
    return callout != NULL && callout->id == id && callout->storey_id == storey_id &&
        callout->target.x == target.x && callout->target.y == target.y &&
        callout->label_anchor.x == label_anchor.x && callout->label_anchor.y == label_anchor.y &&
        callout->text != NULL && text != NULL && strcmp(callout->text,text) == 0;
}

int create_plan_callout_command_create(DomainId storey_id, PlanPosition target,
    PlanPosition label_anchor, const char *text, CreatePlanCalloutCommand *command)
{
    if (command == NULL || storey_id == DOMAIN_ID_INVALID || text == NULL || text[0] == '\0') {
        return 0;
    }
    DocumentPlanCallout candidate={.id=1,.storey_id=storey_id,.target=target,
        .label_anchor=label_anchor,.text=(char *)text};
    if (!document_plan_callout_is_locally_valid(&candidate)) { return 0; }
    char *owned=copy_text(text);
    if (owned == NULL) { return 0; }
    create_plan_callout_command_destroy(command);
    *command=(CreatePlanCalloutCommand){storey_id,target,label_anchor,owned};
    return 1;
}

int create_plan_callout_command_clone(const CreatePlanCalloutCommand *source,
    CreatePlanCalloutCommand *output)
{
    if (source == NULL || output == NULL || source == output) { return 0; }
    CreatePlanCalloutCommand candidate={0};
    if (!create_plan_callout_command_create(source->storey_id,source->target,
            source->label_anchor,source->text,&candidate)) { return 0; }
    create_plan_callout_command_destroy(output);
    *output=candidate;
    return 1;
}

void create_plan_callout_command_destroy(CreatePlanCalloutCommand *command)
{
    if (command == NULL) { return; }
    free(command->text);
    *command=(CreatePlanCalloutCommand){0};
}

int create_plan_callout_command_execute(SiteHelperProject *project,
    const CreatePlanCalloutCommand *command, DomainId *callout_id)
{
    if (callout_id == NULL) { return 0; }
    *callout_id=DOMAIN_ID_INVALID;
    if (project == NULL || command == NULL || command->text == NULL) { return 0; }
    DomainId id=sitehelper_project_add_plan_callout(project,command->storey_id,
        command->target,command->label_anchor,command->text);
    if (id == DOMAIN_ID_INVALID) { return 0; }
    *callout_id=id;
    return 1;
}

int create_plan_callout_command_redo(SiteHelperProject *project,
    const CreatePlanCalloutCommand *command, DomainId callout_id)
{
    if (project == NULL || command == NULL || callout_id == DOMAIN_ID_INVALID ||
        command->text == NULL) { return 0; }
    DocumentPlanCallout callout={.id=callout_id,.storey_id=command->storey_id,
        .target=command->target,.label_anchor=command->label_anchor,.text=command->text};
    return sitehelper_project_insert_callout(project,&callout);
}

int create_plan_callout_command_undo(SiteHelperProject *project,
    const CreatePlanCalloutCommand *command, DomainId callout_id)
{
    return callout_matches(sitehelper_project_find_callout_by_id_const(project,callout_id),
        callout_id,command->storey_id,command->target,command->label_anchor,command->text) &&
        sitehelper_project_remove_callout_by_id(project,callout_id);
}

int edit_plan_callout_command_create(DomainId callout_id, DomainId storey_id,
    PlanPosition target, PlanPosition label_anchor, const char *text,
    EditPlanCalloutCommand *command)
{
    if (command == NULL || callout_id == DOMAIN_ID_INVALID || storey_id == DOMAIN_ID_INVALID ||
        text == NULL || text[0] == '\0') { return 0; }
    DocumentPlanCallout candidate={.id=callout_id,.storey_id=storey_id,.target=target,
        .label_anchor=label_anchor,.text=(char *)text};
    if (!document_plan_callout_is_locally_valid(&candidate)) { return 0; }
    char *owned=copy_text(text);
    if (owned == NULL) { return 0; }
    edit_plan_callout_command_destroy(command);
    *command=(EditPlanCalloutCommand){callout_id,storey_id,target,label_anchor,owned};
    return 1;
}

int edit_plan_callout_command_clone(const EditPlanCalloutCommand *source,
    EditPlanCalloutCommand *output)
{
    if (source == NULL || output == NULL || source == output) { return 0; }
    EditPlanCalloutCommand candidate={0};
    if (!edit_plan_callout_command_create(source->callout_id,source->storey_id,
            source->target,source->label_anchor,source->text,&candidate)) { return 0; }
    edit_plan_callout_command_destroy(output);
    *output=candidate;
    return 1;
}

void edit_plan_callout_command_destroy(EditPlanCalloutCommand *command)
{
    if (command == NULL) { return; }
    free(command->text);
    *command=(EditPlanCalloutCommand){0};
}

int edit_plan_callout_command_execute(SiteHelperProject *project,
    const EditPlanCalloutCommand *command)
{
    return project != NULL && command != NULL && command->text != NULL &&
        sitehelper_project_update_plan_callout(project,command->callout_id,
            command->storey_id,command->target,command->label_anchor,command->text);
}

int delete_plan_callout_command_create(DomainId callout_id, DeletePlanCalloutCommand *command)
{
    if (command == NULL || callout_id == DOMAIN_ID_INVALID) { return 0; }
    *command=(DeletePlanCalloutCommand){callout_id};
    return 1;
}

int delete_plan_callout_command_execute(SiteHelperProject *project,
    const DeletePlanCalloutCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_remove_callout_by_id(project,command->callout_id);
}
