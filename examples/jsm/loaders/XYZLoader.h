#pragma once
#include "core/BufferGeometry.h"
#include "core/NativeObject.h"
#include "loaders/Loader.h"
#include <memory>
#include <string>

// Port of examples/jsm/loaders/XYZLoader.js at the pinned revision (see upstream.json).
//
// XYZ is the simplest point cloud format: each line is either "x y z" or "x y z r g b". Every
// behaviour below mirrors the pinned parse: lines are trimmed, a leading '#' is a comment, values
// are split on runs of whitespace, only three and six value lines produce output, the colours are
// divided by 255 and converted from sRGB, and the colour attribute is only created when at least
// one colour was read. Numbers go through JS parseFloat semantics rather than C++ stream
// extraction, which disagrees on leading garbage, on "Infinity" and on hex-looking text.
class XYZLoader : public Loader {
  public:
    explicit XYZLoader(JSValue manager={});
    static XYZLoader fromObject(JSValue value){return XYZLoader(std::move(value),Existing{});}
    static JSValue prototype();
    std::shared_ptr<BufferGeometry> parseText(const std::string& text);
    JSValue parse(JSValue text);
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    XYZLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};