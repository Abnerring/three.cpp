# Core, scene and camera port

Reference: three.js r187dev, commit `11738a9d55e502fb484cefffcea3a66dc44e9fe9`.

The scene graph keeps the upstream transform order and update flags. Rotation and quaternion changes synchronize through callbacks. Local transforms, world transforms, parented `lookAt`, world-preserving `attach`, pivot transforms, lifecycle events, recursive clone/copy and traversal follow the reference source. Cameras include physical lens parameters, view offsets, WebGL/WebGPU coordinate systems and reversed depth. Camera view matrices exclude scale, matching r187dev.

Scene backgrounds currently accept `Color`. Linear and exponential fog, background/environment rotation and intensity, and override material cloning are represented. Environment textures clone through their virtual texture method. JSON output includes the scene graph, cameras, fog, morph state, animation clips, shape definitions and deduplicated geometry/material/texture/image resources in upstream discovery order.

## C++ ownership and API adaptations

- `children` owns descendants with `std::shared_ptr<Object3D>`. `parent` is a non-owning pointer. Destroying a parent clears surviving children's parent pointers. Change the graph through `add`, `remove`, `clear` and `attach`; direct edits to these public fields can break graph invariants.
- Keep a `shared_ptr` while calling a method on a child that might detach itself. This supplies the caller lifetime that JavaScript's object references provide.
- `Object3D` copy/move constructors and assignments are disabled. Use `copy(source, recursive)` or virtual `clone(recursive)` so quaternion/Euler callbacks remain attached to their own object.
- Event listeners use shared callback handles. Save the handle returned by `addEventListener` for identity checks and removal. Registering the same handle twice does not duplicate it.
- Null children, self parenting and ancestor cycles throw `std::invalid_argument`. The upstream implementation logs self-parenting errors; C++ additionally rejects cycles to protect ownership and recursive graph operations.
- `static` is named `isStatic` because `static` is a C++ keyword. JSON retains the upstream `static` property name.
- Vector/Color components are public numeric fields (`position.x`, `color.r`). Euler/Quaternion components are accessor properties whose assignments trigger the upstream callback; bulk methods retain their original callback counts. Camera properties remain public fields.
- `getObjectByProperty` and `getObjectsByProperty` query scalar properties declared by the ported object/scene/camera classes. Arbitrary JavaScript dynamic properties and object identity queries are not yet represented.
- Empty render/shadow callbacks correspond to upstream's default no-op hooks. Vulkan supplies the renderer argument. Geometry/material/group pointers are null for scene-level render hooks.
- The virtual `toJSON(Object3DJSONContext* meta = nullptr)` mirrors upstream's explicit `meta` argument. A root call owns its context; descendants share it, and geometry/material serializers run once per UUID in that document. Custom subclasses override this contextual overload and pass `meta` to their base. `toJSON()` retains the upstream document shape. The compatibility `toJSON(bool root)` wrapper starts an independent document; `false` omits document metadata and retains resource arrays.

## Remaining modules and behavior

Texture backgrounds and environment lighting remain pending. CPU environment serialization skips render-target textures, matching the upstream condition. Animation clip resources are cached by UUID; animation playback, property binding and mixing are separate work. Raycaster and Mesh intersections cover geometry bounds, indexed/non-indexed triangles, material sides/groups, draw ranges, morph positions, UV/normal/barycentric output, layer filtering and recursive traversal. Line, Points, Sprite, LOD and instanced/skinned object dispatch remain pending.

Managed cameras and objects referenced by intersection results retain shared ownership. Stack-owned camera/object pointers remain borrowed and require the caller to keep them alive. Raycast's bool result uses true for upstream's undefined result and false to stop recursive propagation. Custom subclasses can override raycast, getVertexPosition and _computeIntersections.

BufferGeometry groups retain stable object identity through SharedVector. Mutating groups[i].count changes an already queued group; clearing the container does not destroy groups retained by the renderer. Geometry copy/clone and toNonIndexed create independent groups. SharedVector exposes the array operations used here and does not offer every std::vector method.

`lookAt` and `attach` inherit the reference source's limitations for non-uniform parent scales. Do not use these methods to infer support that upstream itself does not provide.

The Vulkan backend currently implements basic material, fog and 2D DataTexture map/alphaMap rendering. Lighting, other material models and shadow passes remain pending; representing their future properties in the scene graph alone will not establish rendering parity. Texture/image cache entries retain their source identity, version and upload state; earlier draws retain immutable images across later uploads and disposal within the same frame.

`JSValue` provides a separate runtime graph for undefined/null, binary64 values, UTF-16 string identity, object aliases, ordered keys, sparse arrays, native functions with explicit managed captures and receivers, prototypes, descriptors/accessors and basic coercion. GC traces prototypes, captures, accessors and conversion hooks; reentrant operations retain object identity and descriptor snapshots. Typed traces preserve identity and types rather than flattening them to ordinary JSON. Symbol/BigInt/Proxy, construction, complete boxed built-ins, cross-thread references and integration of all engine properties remain pending. The managed primitive conversion hook is an internal adapter, not Symbol support or a JavaScript bridge. `JsonValue` remains a serialization helper with upstream key order and lone-surrogate escaping. Material.setValues(userData) and direct geometry/object/material serialization retain userData aliases; copy/clone use the upstream class's sharing/deep-clone rules.

`BufferAttribute.array` now holds typed views over shared ArrayBuffer bytes. Direct construction preserves the supplied view; typed subclasses receiving arrays, copy and clone allocate independent bytes. An ArrayBuffer constructor creates a distinct view sharing the supplied bytes. subarray preserves the buffer but creates a distinct view; slice copies bytes. Same-type set copies raw bytes with overlap support, including NaN payloads; cross-type set snapshots values before conversion. Clamped writes saturate and use ties-to-even. itemSize/count and component indices retain double semantics, including nonintegral counts. Float16BufferAttribute stores raw Uint16 encodings and converts through DataUtils in its component accessors; its generic getComponent/setComponent retain raw storage semantics. Runtime object graph integration, detached/resizable buffers, DataView, native Float16Array and BigInt storage remain pending.

## Verification

`test/unit/src/core`, `test/unit/src/cameras` and `test/unit/src/scenes` verify graph ownership, events and listener mutation, callback synchronization, transforms, matrix update controls, parented camera directions, camera scale exclusion, projections, view offsets, cloning and JSON resource reuse.

`test/unit/utils/parity/CoreParityProbe.cpp` and `compare-core.mjs` compare C++ results directly with the pinned JavaScript source. The comparison covers pivot/parented lookAt/attach transformations, both camera projections across coordinate/depth modes, lens/view bounds, and nested scenes with shared geometry/material resources. It verifies this implemented scope; it does not establish completion of all three.js modules.
