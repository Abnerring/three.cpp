#pragma once
#include "core/BufferGeometry.h"
#include "core/JSValue.h"
#include <memory>
#include "loaders/Loader.h"
JSValue nativeBufferGeometryLoaderConstructor();

// Source-managed BufferGeometryLoader.js at the pinned revision. parse() and
// load() preserve runtime values and resource identities. parseGeometry() is
// the separate legacy typed renderer adapter.
class BufferGeometryLoader : public Loader {
  public:
    explicit BufferGeometryLoader(JSValue manager={});
    static BufferGeometryLoader fromObject(JSValue value){return BufferGeometryLoader(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue parse(JSValue json);
    std::shared_ptr<BufferGeometry> parseGeometry(JSValue json);
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    BufferGeometryLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};
