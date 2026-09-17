# Priority 26G3 — Roof Persistence

Priority 26G3 promotes authoritative Roof source state into project persistence
without serializing generated roof geometry or structural-layout snapshots.

## Format version 15

Project format version 15 adds one `roofs` collection inside every Storey after
`slabs` and before `end_storey`.

Versions 1–14 contain no roof records and load with an empty `RoofCollection`.
No roof is inferred from walls, rooms, slabs, Storey elevation or framing
settings.

The v15 roof grammar is intentionally source-oriented:

```text
roofs <count>
roof <roof-id> portions <count>
portion <portion-id> generation <kind> slope_ppm <value> \
    reference_z <mm> direction <x> <y> \
    single_slope_reference <low_edge|high_edge> support <vertex-count>
vertex <x-mm> <y-mm>
...
end_portion
compositions <count>
composition <first-portion-id> <second-portion-id> <intersects|abuts>
...
terminations <count>
termination <portion-id> <negative_axis|positive_axis> <offset-mm>
...
end_roof
```

The ordered Roof collection and ordered portion/relationship arrays are
preserved exactly. Roof and portion `DomainId`s are also preserved exactly.
The project-wide `domain_id_next` watermark remains authoritative and is
validated after parsing with the same global identity rules as live projects.

## Persisted authority

Version 15 stores only data already owned by `RoofDefinition`:

- Roof ID;
- Roof portion IDs;
- ordered support polygon vertices in integer millimetres;
- portion generation intent;
- fixed-point `slope_ppm` using `ROOF_SLOPE_SCALE`;
- explicit vertical reference;
- plan direction;
- single-slope low/high datum ownership;
- explicit composition relationships; and
- explicit primitive terminations.

It does **not** store:

- generated planes or plane polygons;
- ridges, hips, valleys or generic plane seams;
- step/abutment derived interfaces;
- clipped intersection coordinates;
- structural-layout snapshots;
- rafter/truss/member layouts;
- RLW/ULW, tributary areas, reactions or load paths;
- wind pressure regions; or
- standards/compliance results.

## Load boundary

Loading remains transactional. Roof records are parsed into temporary owned
`Roof` values, validated, and inserted into their Storey only when the complete
Roof source state can regenerate successfully.

After the complete candidate project has parsed and passed authoritative
project validation, `regenerate_project()` explicitly regenerates/checks every
Roof again alongside wall framing. Any regeneration failure aborts the load and
leaves the destination project untouched.

The current production Roof does not retain a generated geometry cache, so the
regenerated snapshot is deliberately destroyed after the check. Future cached
derived geometry may replace that detail without changing the persistence
contract.

## Save boundary

The temporary Priority 26G1 `UNSUPPORTED_PROJECT_DATA` save guard is removed.
Valid roof-bearing projects now save as v15. Save still validates authoritative
project state before opening the destination file.

Invalid enum values, malformed source geometry, invalid relationship references,
duplicate global IDs or an invalid DomainId watermark therefore cannot be
written through the supported API.

## Compatibility

- v1–v14: parse through their existing migration paths and receive zero roofs.
- v15: parses all previous authoritative data plus Storey-owned roofs.
- v15 does not attempt to infer roofs for legacy files.
- v15 remains independent of roof covering, structural strategy and engineering
  layers that have not yet become production authority.

This keeps persistence aligned with the central SiteHelper rule: save source
intent, validate it, and regenerate consequences.
