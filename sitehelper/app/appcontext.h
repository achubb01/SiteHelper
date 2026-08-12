#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "sitehelper_project.h"
#include "sitehelper_editor.h"

typedef struct
{
    SiteHelperProject project;
    SiteHelperEditor editor;
} AppContext;

#endif