#pragma once
#include "loaders/Loader.h"
#include "core/BufferGeometry.h"
class BufferGeometryLoader : public Loader {
  public:
    explicit BufferGeometryLoader(JSValue manager={});
    static JSValue prototype();
    using Loader::parse;
    std::shared_ptr<BufferGeometry> parseGeometry(JSValue json);
    std::shared_ptr<BufferGeometry> parse(const JsonValue& json){return parseGeometry(JSValue::fromJSON(json));}
};
