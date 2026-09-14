# Slab subordinate feature commands

Penetrations, replacement regions and edge rebates remain subordinate slab-owned
geometry. They receive no `DomainId`. Commands address one feature through its
parent Slab `DomainId`, feature kind and current collection index. That index is
an ephemeral source-order position, not persistent identity and is not serialized.

Polygon ADD commands own deep copies of their ordered vertices. History clones
that storage before execution. ADD appends one validated feature, undo verifies
the exact feature at the recorded position before removing it, and redo uses the
slab domain's validated indexed insertion. No ADD allocates a domain identity.

DELETE history entries capture an independent complete snapshot before mutation.
Undo restores the snapshot at its original collection index; redo verifies the
restored value at that index before removing it again. Collection counts and exact
values are checked so external mutation cannot make an older command operate on a
different feature. Failed execute, undo or redo leaves both project state and the
history cursor unchanged.

Plan Delete input translates the existing subordinate editor selection into the
matching DELETE command. Successful completion clears the ephemeral selection so
an item shifted into the same index is never mistaken for the deleted feature.
Commands contain no editor state, and undo does not restore selection.

Feature drawing, reshape/move operations and property editing are deferred.
