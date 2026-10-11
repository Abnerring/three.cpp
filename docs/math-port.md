# Math port status

The reference checkout is three.js commit `11738a9d55e502fb484cefffcea3a66dc44e9fe9`. Its exported `REVISION` is `187dev`; its package version is `0.186.0`. These labels identify one pinned source tree, not a claim that later releases behave identically.

## Implemented and checked

`HIGH_PRECISION` now means IEEE-754 binary64 (`double`) on every architecture. Vector and matrix mutations return the receiving object. Default equality compares components exactly; an explicitly supplied epsilon remains a C++ convenience extension. Array writers grow their destination and return it.

The existing Vector2, Vector3, Vector4, Matrix3, Matrix4, Euler, Quaternion, Color and MathUtils implementations were compared with the pinned JavaScript. Corrections include homogeneous division, quaternion vector rotation, direction normalization, both projection coordinate systems, reversed depth, all six Euler orders and their singular branches, signed decomposition, singular matrix handling, normal matrices, quaternion shortest-path interpolation and extrapolation, and normalized BufferAttribute reads. Matrix4 determinant now uses the pinned source's shared minors and evaluation order, with cancellation and intermediate overflow/underflow regression cases.

Euler and Quaternion notify mutation callbacks. Their conversion APIs support suppressing callbacks during Object3D synchronization. C++ copy constructors copy components into an independent value; assignment preserves and invokes the destination callback. Euler conversions retain the current order when the caller omits it. The heterogeneous Euler array overload includes the order string.

Hexadecimal and CSS Color inputs use sRGB by default and convert to the linear working space. The port includes the two built-in sRGB spaces, the upstream transfer curves, the full upstream color-name table, supported hexadecimal/rgb/hsl syntax, hexadecimal/CSS output, RGB matrix application and HSL interpolation. MathUtils includes fractional modulo, Mulberry32 seeded random values, proper Euler rotations, and JavaScript rounding/minimum/maximum behavior, including NaN and signed zero.

`test/math/generate-threejs-fixtures.mjs` executes the actual JavaScript modules and rejects a checkout with a different HEAD. It generates `ThreeJsFixtures.h`, which is consumed by `ThreeJsParity.tests.cpp`. Its 83 fixtures cover vector operations, all Euler orders and gimbal branches, quaternion edge cases and flat arrays, projection matrices, decomposition, determinant evaluation, CSS/linear color, proper Euler rotations, seeded random sequences, callback counts and camera projection/unprojection. The generator stores source results rather than computing expected values from the C++ implementation.

The integrated Debug and Release runs each passed 145 CPU test cases and 1,760 assertions, including the determinant and nonfinite `isPowerOfTwo` changes. Passing this suite establishes the exercised cases, not complete math-module equivalence. The full check results are recorded in `docs/VALIDATION.md`.

## Remaining methods in the existing classes

The standard named prototype methods of Vector2, Vector3, Vector4, Matrix3, Matrix4, Euler, Quaternion and Color are present. The Spherical and Cylindrical wrappers delegate to the corresponding coordinate methods. This is a method-name audit, not proof of complete signatures, overloads or every edge case. MathUtils named functions are present, with typed-array operations represented by C++ templates.

## Geometric queries checked against upstream

Plane, Line3, Triangle, Ray and Frustum now implement the named methods in the pinned modules. Ray's triangle intersection uses the current source's watertight projection algorithm; it does not substitute the older intersection algorithm. Local scratch vectors preserve the source's calculation sequence while avoiding shared mutable temporaries. Query methods are callable on const objects. Mutations return the receiver.

Nullable vector results point to the caller's target and return `nullptr` on a miss. The target remains owned by the caller. A degenerate Triangle barycentric calculation zeros its target before returning `nullptr`, as upstream does. Ray's nullable scalar distance to a plane uses `std::optional<double>`.

`test/parity/QueryParityProbe.cpp` emits 718 operation samples, including their inputs. `compare-query.mjs` executes the actual pinned JavaScript for those inputs, checks nonfinite values and signed zero exactly, and compares finite values with tolerance `2e-12 * max(1, abs(actual), abs(expected))`. Cases cover normal, degenerate, NaN/Infinity, boundary and target alias behavior; all Ray queries and backface culling; Triangle interpolation into Vector2/3/4, normalized attributes and the missing trailing W of an itemSize=3 attribute; and perspective/orthographic Frustum extraction for both coordinate systems with and without reversed depth. Eighteen additional samples check Vector2/3/4 normalization, length clamping and length setting with NaN, Infinity, signed zero, underflow and overflow. The script's syntax has been checked.

Debug and Release each passed all 718 query samples and 14,301 comparisons. Vector2/3/4 normalization and length clamping explicitly fall back for zero or NaN, matching JavaScript `length() || 1`. For example, normalizing `[NaN, 5, 6]` preserves its finite components. The Vector3 `normalized()` convenience method uses the same rule.

Frustum has structural overloads for objects that own a bounding sphere or geometry, plus Sprite center/matrixWorld queries. Its erased `Object3D&` overload currently recognizes Mesh, the available geometry-bearing class. Later renderable subclasses need to extend that dispatch or use the structural overload. Ordinary C++ constructor/copy values own their vectors and planes; JavaScript constructor object-reference aliasing is not reproduced by those value constructors. These are explicit representation limits, not claims of universal object parity.

## Remaining semantic and representation differences

- Vector and Color components use C++ getters/setters instead of mutable JavaScript properties. `EulerOrder` uses an enum instead of an arbitrary string. A mutable global `Euler.DEFAULT_ORDER` is not implemented.
- JavaScript iteration over a math object is not represented by a C++ iterator adapter. Returned arrays can be iterated normally.
- Missing/short input arrays, `undefined`, arbitrary nonnumeric values and invalid order strings do not have a uniform JavaScript-compatible representation. Several numeric `fromArray` methods still index the vector directly; others check bounds. Valid-length arrays are covered. The heterogeneous Euler array overload rejects unknown order strings instead of retaining them and warning.
- Unknown CSS inputs preserve the previous color values, but JavaScript warning messages and deprecated Matrix3 warning-once behavior are not reproduced.
- ColorManagement has the built-in sRGB conversions and enable/working-space settings. Its runtime `define`, custom color-space descriptions, XYZ transforms and the complete configuration/query API are not implemented.
- Integer-returning helpers such as `Color.getHex` and integer parameters such as `randInt` cannot express every JavaScript Number edge case. Conversion of NaN or values outside C++ integer ranges is not verified. `seededRandom` accepts an optional 32-bit integer seed rather than an arbitrary JavaScript Number.
- Some C++ convenience methods have no counterpart in current three.js. Examples include approximate equality parameters, Quaternion scalar/component arithmetic and gamma helpers, and Vector4 `setFromMatrixScale`. Their presence is not evidence of upstream parity.

## Remaining math modules

The broader math directory is unfinished. FrustumArray, Interpolant, SphericalHarmonics3 and the `interpolants` directory remain absent. Box2, Cylindrical, Matrix2 and Spherical passed the helper differential suite, which also checks clipping-plane copies (396 values). Box3 and Sphere are additionally exercised by geometry/query checks. This document does not mark the full three.js-to-C++ objective complete.
