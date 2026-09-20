#ifndef DOCUMENT_PLAN_QUERY_H
#define DOCUMENT_PLAN_QUERY_H

#include "document.h"
#include "position.h"

/* Anchor hit query for authored Plan annotations. Notes are overlay objects: the
 * nearest anchor inside tolerance wins; later-authored notes win exact ties. */
DomainId document_plan_find_note_at_position(const DocumentModel *document,
    DomainId storey_id, PlanPoint point, double tolerance_mm);

/* Same anchor-distance rule for persistent Plan symbols. Directional presentation
 * remains derived; stable hit identity is the authored anchor. */
DomainId document_plan_find_symbol_at_position(const DocumentModel *document,
    DomainId storey_id, PlanPoint point, double tolerance_mm);

/* Leader-line/anchor hit query for persistent callouts. The shortest distance
 * to the authored target-label segment wins; later-authored callouts win ties. */
DomainId document_plan_find_callout_at_position(const DocumentModel *document,
    DomainId storey_id, PlanPoint point, double tolerance_mm);

/* Closed-boundary hit query for revision markup. The authoritative polyline,
 * including its implicit closing edge, is selectable; scalloped appearance is
 * presentation-only. Nearest boundary wins, later-authored wins exact ties. */
DomainId document_plan_find_revision_cloud_at_position(const DocumentModel *document,
    DomainId storey_id, PlanPoint point, double tolerance_mm);

#endif
