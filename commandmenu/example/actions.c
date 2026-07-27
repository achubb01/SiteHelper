#include <stdio.h>

#include "actions.h"
#include "appcontext.h"

void doubleornuttin(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        fprintf(stderr, "Application context is unavailable\n");
        return;
    }

    app->value *= 2;

    printf("Value: %d\n", app->value);
}

void tripleornuttin(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        fprintf(stderr, "Application context is unavailable\n");
        return;
    }

    app->value *= 3;

    printf("Value: %d\n", app->value);
}

void change_brightness(void *context)
{
    AppContext *app = context;

    if (app == NULL) {
        fprintf(stderr, "Application context is unavailable\n");
        return;
    }

    app->brightness++;

    printf(
        "Brightness: %d\n",
        app->brightness
    );
}