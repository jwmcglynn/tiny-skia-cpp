# Function Mapping Tables

Legend: `☐` Not started, `🧩` Stub only, `🟡` Implemented/tested (Rust completeness not yet vetted), `🟢` Rust-completeness vetted (line-by-line against Rust), `✅` Verified parity sign-off (user-requested audit complete), `⏸` Blocked.

### `third_party/tiny-skia/src/scan/path.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `fill_path` | `scan::fillPath` | 🟡 | Empty-path no-op and rectangle fill span smoke tests |
| `fill_path_impl` | `scan::fillPathImpl` | 🟡 | Clipping-disabled and culling paths |
| `conservative_round_to_int` | `conservativeRoundToInt` | 🟡 | Conservative rounding bounds checks |
| `round_down_to_int` | `roundDownToInt` | 🟡 | Biased down-round behavior |
| `round_up_to_int` | `roundUpToInt` | 🟡 | Biased up-round behavior |
| `walk_edges` | `walkEdges` | 🟡 | Horizontal span sequencing checks |
| `remove_edge` | `removeEdge` | 🟡 | Linked-list unlink behavior |
| `backward_insert_edge_based_on_x` | `backwardInsertEdgeBasedOnX` | 🟡 | Ordered insertion with backward scan |
| `insert_edge_after` | `insertEdgeAfter` | 🟡 | Doubly-linked splice behavior |
| `backward_insert_start` | `backwardInsertStart` | 🟡 | Backward start probe invariants |
| `insert_new_edges` | `insertNewEdges` | 🟡 | Y-gated new-edge insertion behavior |

### `third_party/tiny-skia/src/scan/path_aa.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `fill_path` | `scan::path_aa::fillPath` | 🟡 | Empty path no-op and rectangle AA span smoke checks |
| `fill_path_impl` | `scan::path_aa::fillPathImpl` | 🟡 | Fallback-to-non-AA and clipping bounds coverage |
| `rect_overflows_short_shift` | `rectOverflowsShortShift` | 🟡 | Overflow and clamp behavior on large bounds |
| `coverage_to_partial_alpha` | `coverageToPartialAlpha` | 🟡 | Alpha quantization checks at boundaries |

### `third_party/tiny-skia/src/scan/hairline.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `stroke_path` | `scan::strokePath` | 🟡 | Clip handling and span sequence smoke checks |
| `stroke_path_impl` | `scan::strokePathImpl` | 🟡 | Segment dispatch smoke checks |
| `LineCap::Butt` | `LineCap::Butt` | 🟡 | Enum value parity |
| `LineCap::Round` | `LineCap::Round` | 🟡 | Enum value parity |
| `LineCap::Square` | `LineCap::Square` | 🟡 | Enum value parity |
| `hair_line_rgn` | `hairLineRgn` | 🟡 | Horizontal/vertical clipping and traversal |
| `extend_pts` | `extendPts` | 🟡 | Segment-end cap extension coverage |
| `hair_quad` | `hairQuad` | 🟡 | Colinear control points reduce to line-equivalent spans |
| `hair_quad2` | `hairQuad2` | 🟡 | Colinear control-point path uses direct line span sequence |
| `hair_cubic` | `hairCubic` | 🟡 | Colinear cubic reduces to no-split line-equivalent spans |
| `hair_cubic2` | `hairCubic2` | 🟡 | Colinear cubic no-split spans and path-level regression checks |

### `third_party/tiny-skia/src/scan/hairline_aa.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `fill_rect` | `fillRect` | 🟡 | Rectangle intersection fallback and fixed-rect dispatch |
| `fill_fixed_rect` | `fillFixedRect` | 🟡 | Fixed-point conversion to 8-bit span coordinates |
| `fill_dot8` | `fillDot8` | 🟡 | Boundary and fractional-edge coverage |
| `do_scanline` | `doScanline` | 🟡 | 1-pixel, single-edge, and fractional scanline checks |
| `call_hline_blitter` | `callHlineBlitter` | 🟡 | Chunked call sequence and width clamping checks |
| `stroke_path` | `scan::hairline_aa::strokePath` | 🟡 | Clipping/no-clip anti-span behavior |
| `anti_hair_line_rgn` | `antiHairLineRgn` | 🟡 | Line pre-clip and partial-clip sub-rect dispatch checks |
| `do_anti_hairline` | `doAntiHairline` | 🟡 | Dominant-axis and clipping-overlap branch checks |
| `bad_int` | `badInt` | 🟡 | Integer edge sentinel parity |
| `any_bad_ints` | `anyBadInts` | 🟡 | High-bit-flag checks |
| `contribution_64` | `contribution64` | 🟡 | Fractional contribution extraction |
| `HLineAntiHairBlitter` | `HLineAntiHairBlitter` | 🟡 | Lower/upper blend split checks |
| `HorishAntiHairBlitter` | `HorishAntiHairBlitter` | 🟡 | Vertical pair blend split checks |
| `VLineAntiHairBlitter` | `VLineAntiHairBlitter` | 🟡 | Horizontal pair blend split checks |
| `VertishAntiHairBlitter` | `VertishAntiHairBlitter` | 🟡 | Horizontal pair blend split checks |
| `RectClipBlitter` | `RectClipBlitter` | 🟡 | Rect clipping boundaries and anti-run clipping checks |

### `third_party/tiny-skia/src/scan/mod.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `fill_rect` | `scan::fillRect` | 🟡 | Integer-rounding and clip intersection behavior |
| `fill_rect_aa` | `scan::fillRectAa` | 🟡 | Fractional-rect antialias path coverage |

