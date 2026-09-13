# Linear cutting optimiser

`sitehelper_optimisation` packs required linear pieces into repeatedly available
catalogue lengths. It depends only on standard C headers and the C library. It
has no construction-domain types, IDs, section dimensions, or dependency on
model/project/wall/take-off/cut-list libraries. No upstream model is changed.

A future caller-side adapter can resolve required members into material/stock
classes and then supply `LinearCutRequirement` values. This adapter is deferred:
member purpose and depth/width alone must not determine material compatibility.
The optimiser can serve any linear material without learning its domain.

`linear_cut_optimizer.h` defines requirements (class, opaque reference, length,
quantity), stock options (class, opaque reference, length), and the owned
`LinearCutPlan`. All physical lengths are integer millimetres. Quantities use
`uint64_t`; accumulated lengths use checked `int64_t` arithmetic.

Stock classes are opaque nonzero `uint64_t` equality keys assigned by the caller.
Pieces can share a bar only when their classes match. Class values do not encode
purpose, species, grade, treatment or product semantics here. References are
copied without interpretation; zero, duplicates and all other `uint64_t` values
are allowed. Options have unlimited repeat availability, with multiple lengths
per class supported. They are not inventory records or purchasing quantities.

The deterministic best-fit decreasing heuristic:

1. Expand quantities into pieces using checked counts/allocation sizes.
2. Sort by class ascending, length descending, original requirement index,
   then ordinal within that requirement.
3. Place each piece in the compatible open bar leaving the smallest remainder.
   Equal fits choose the earliest opened bar.
4. If none fits, open the shortest compatible catalogue length accepting the
   piece. Equal lengths choose the earliest input option.
5. If no option fits, fail the whole query with the offending requirement index
   and class; never discard an unfulfillable piece.

**This algorithm is deterministic and produces valid cutting plans, but is a
heuristic and is not guaranteed to find the mathematically minimum-waste
solution.** For example, two 500-mm pieces with 600/1000-mm options and zero kerf
use two 600-mm bars under this rule, even though one 1000-mm bar could suffice.
The algorithm can be replaced without changing construction-domain code.

Nonnegative `kerf_mm` is a packing allowance between adjacent pieces:

```text
consumed = sum(piece lengths) + kerf_mm * (piece count - 1)
stock length = piece length + packing kerf + remainder
```

A single exact-length piece fits with any nonnegative kerf. This is not a full
saw-operation simulation: no trim cuts, machine motion or blade setup are
modelled. Remaining length is reported as remainder; no offcut reuse is implied.

The plan owns flat `stock[]` and `cuts[]` arrays. Bars follow opening order and
each bar owns an index range of cuts in placement order. Each cut copies its
original requirement index/reference/length; each bar copies its option
index/reference/class/length. Per-bar piece/kerf/remainder values and plan-wide
totals obey the length identity above. Counts delimit readable entries; internal
spare allocation storage is not public. Packing order is not machine execution
instructions. The snapshot contains no input pointers and can outlive input
modification/destruction; original indices remain provenance for that snapshot.

Initialize `LinearCutPlan plan = {0}` and call `linear_cut_optimize(...)`.
Success replaces previous output. Every failure, including numeric overflow and
allocation failure, preserves all previous pointers and contents. Check the
returned `LinearCutResult.code`; a retained older snapshot still describes older
inputs. `linear_cut_plan_destroy` frees both arrays and zeros the plan, and is
safe with NULL, zero and repeated calls. Do not shallow-copy into another owner.
Inputs are read-only and must stay valid/unmodified during a call. Output must
be independent of inputs and start zero or be a previous successful result.

NULL arrays require zero counts. Lengths, quantities and classes must be positive.
Pointer/count/kerf checks precede input-size checks, requirement validation and
expansion arithmetic, then catalogue validation. All supplied catalogue entries
are validated even with no requirements. Empty requirements succeed empty;
nonempty valid requirements with an empty catalogue are unsatisfied. Status
codes distinguish argument, requirement, catalogue, unsatisfied, allocation and
numeric failures. Diagnostic indices are `SIZE_MAX` and class is zero when
unavailable. These checks cannot prove arbitrary C pointer validity.

For R requirement rows, S catalogue options, N expanded pieces and B used bars,
time is O(R + S + N log N + N*B + B*S), with O(N) owned/workspace storage.
The simple scan is quadratic in the worst-case number of pieces. Large quantity
expansions can exhaust memory; no arbitrary quantity ceiling or global solver
is added. Future performance work can replace search structures or the heuristic.

Deferred: material/product databases, species/grade/treatment modelling, inventory
quantities, purchasing, prices, supplier/SKU data, offcut inventory/reuse between
runs, a globally optimal/ILP solver, saw-bench machine instructions, labels/barcodes,
GUI and persistence. Plans are derived snapshots with no authoritative identity;
they are not serialized and do not participate in commands/history.

Focused standalone verification (no other SiteHelper library required):

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Isitehelper/optimisation \
  sitehelper/optimisation/linear_cut_optimizer.c \
  sitehelper/optimisation/tests/test_linear_cut_optimizer.c -o /tmp/test-linear-cut
/tmp/test-linear-cut
```

CMake registers `linear_cut_optimizer_tests`; GNU/Clang Linux builds also sweep
allocation failures with linker wrapping and no production allocation hooks.
