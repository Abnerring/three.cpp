#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"

// Port of src/loaders/BufferGeometryLoader.js at the pinned revision (see upstream.json).
//
// Implemented: index, plain attributes, morph attributes, morphTargetsRelative, groups (including the
// legacy drawcalls and offsets keys), boundingSphere, name and userData, all read through a type
// name that selects the typed attribute class.
//
// Not implemented yet, and failing loudly rather than being skipped: interleaved buffer attributes,
// instanced buffer attributes and instanced geometry, and the arrayBuffers table used by interleaved
// buffers. Each of those is a distinct capability with its own storage, and silently ignoring one
// would produce a geometry that looks parsed but is not. Upstream logs to the console when no onError
// is supplied; there is no console natively, so that branch is recorded rather than reproduced.
class BufferGeometryLoader : public Loader {
  public:
    explicit BufferGeometryLoader(JSValue manager={});
    static BufferGeometryLoader fromObject(JSValue value){return BufferGeometryLoader(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue parse(JSValue json);
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    BufferGeometryLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};