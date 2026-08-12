#include <stdlib.h>
#include "sitehelper_editor.h"

void sitehelper_editor_init(
    SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return;
    }

    *editor = (SiteHelperEditor){
        .current_room_id = DOMAIN_ID_INVALID,
        .current_wall_id = DOMAIN_ID_INVALID
    };
}