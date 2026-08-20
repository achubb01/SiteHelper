#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "sitehelper_project.h"
#include "sitehelper_editor.h"
#include "command_history.h"

typedef struct
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
} AppContext;

#endif