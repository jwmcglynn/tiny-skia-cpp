# Function Mapping Tables

Legend: `☐` Not started, `🧩` Stub only, `🟡` Implemented/tested (Rust completeness not yet vetted), `🟢` Rust-completeness vetted (line-by-line against Rust), `✅` Verified parity sign-off (user-requested audit complete), `⏸` Blocked.

### `third_party/tiny-skia/src/path64/cubic64.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Cubic64Pair` | `Cubic64Pair` | 🟡 | Struct layout coverage |
| `Cubic64::new` | `Cubic64::create` | 🟡 | Point copy semantics |
| `Cubic64::as_f64_slice` | `Cubic64::asF64Slice` | 🟡 | Flattened coordinate order |
| `Cubic64::point_at_t` | `Cubic64::pointAtT` | 🟡 | Endpoint fast-path and midpoint checks |
| `Cubic64::search_roots` | `Cubic64::searchRoots` | 🟡 | Segmented binary-search behavior |
| `find_inflections` | `Cubic64::findInflections` | 🟡 | Subdivision invariants |
| `Cubic64::chop_at` | `Cubic64::chopAt` | 🟡 | Midpoint split control points |
| `coefficients` | `coefficients` | 🟡 | Coefficient transform parity |
| `roots_valid_t` | `rootsValidT` | 🟡 | Endpoint clamp and dedupe behavior |
| `roots_real` | `rootsReal` | 🟡 | Real-root regime parity |
| `find_extrema` | `findExtrema` | 🟡 | Derivative-division parity |
| `interp_cubic_coords_x` | `interpCubicCoordsX` | 🟡 | Coord decomposition identity |
| `interp_cubic_coords_y` | `interpCubicCoordsY` | 🟡 | Coord decomposition identity |

### `third_party/tiny-skia/src/path64/line_cubic_intersections.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `horizontal_intersect` | `horizontalIntersect` | 🟡 | Root set and fallback behavior |
| `vertical_intersect` | `verticalIntersect` | 🟡 | Root set and fallback behavior |

### `third_party/tiny-skia/src/path64/mod.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `DBL_EPSILON_ERR` | `kDblEpsilonErr` | 🟡 | Constant value equivalence |
| `FLT_EPSILON_HALF` | `kFloatEpsilonHalf` | 🟡 | Constant value equivalence |
| `FLT_EPSILON_CUBED` | `kFloatEpsilonCubed` | 🟡 | Constant value equivalence |
| `FLT_EPSILON_INVERSE` | `kFloatEpsilonInverse` | 🟡 | Constant value equivalence |
| `Scalar64::bound` | `bound` | 🟡 | Clamp boundary behavior and NaN/inf expectations |
| `Scalar64::between` | `between` | 🟡 | Range test ordering invariants |
| `Scalar64::precisely_zero` | `preciselyZero` | 🟡 | Zero threshold parity |
| `Scalar64::approximately_zero_or_more` | `approximatelyZeroOrMore` | 🟡 | Boundary threshold parity |
| `Scalar64::approximately_one_or_less` | `approximatelyOneOrLess` | 🟡 | Boundary threshold parity |
| `Scalar64::approximately_zero` | `approximatelyZero` | 🟡 | Magnitude threshold parity |
| `Scalar64::approximately_zero_inverse` | `approximatelyZeroInverse` | 🟡 | Magnitude threshold parity |
| `Scalar64::approximately_zero_cubed` | `approximatelyZeroCubed` | 🟡 | Magnitude threshold parity |
| `Scalar64::approximately_zero_half` | `approximatelyZeroHalf` | 🟡 | Magnitude threshold parity |
| `Scalar64::approximately_zero_when_compared_to` | `approximatelyZeroWhenComparedTo` | 🟡 | Relative threshold parity |
| `Scalar64::approximately_equal` | `approximatelyEqual` | 🟡 | Signed and mirrored comparisons |
| `Scalar64::approximately_equal_half` | `approximatelyEqualHalf` | 🟡 | Signed threshold parity |
| `Scalar64::almost_dequal_ulps` | `almostDequalUlps` | 🟡 | ULP-like bound parity |
| `cube_root` | `cubeRoot` | 🟡 | Positive/negative/zero parity |
| `cbrt_5d` | `cbrt5d` | 🟡 | Bit-level seed parity smoke |
| `halley_cbrt3d` | `halleyCbrt3d` | 🟡 | Root convergence parity |
| `cbrta_halleyd` | `cbrtaHalleyd` | 🟡 | Iteration formula parity |
| `interp` | `interp` | 🟡 | Linear interpolation identity |

### `third_party/tiny-skia/src/path64/point64.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Point64::from_xy` | `Point64::fromXy` | 🟡 | `x/y` initialization identity checks |
| `Point64::from_point` | `Point64::fromPoint` | 🟡 | Float round-trip with `Point` |
| `Point64::zero` | `Point64::zero` | 🟡 | Zero initialization checks |
| `Point64::to_point` | `Point64::toPoint` | 🟡 | Float conversion identity checks |
| `Point64::axis_coord` | `Point64::axisCoord` | 🟡 | X/Y axis extraction checks |

### `third_party/tiny-skia/src/path64/quad64.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `push_valid_ts` | `pushValidTs` | 🟡 | Dedupe, range filtering, and clamping assertions |
| `roots_valid_t` | `rootsValidT` | 🟡 | Unit interval root filtering checks |
| `roots_real` | `rootsReal` | 🟡 | Monic and mirrored root sets checks |

Add one section per file as soon as implementation begins.

