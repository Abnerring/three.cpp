#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"

// Port of src/loaders/CubeTextureLoader.js at the pinned revision (see upstream.json).
//
// load walks the url array through ImageLoader and fills the cube texture's six faces, then calls the
// caller's onLoad once all six have arrived. The loop runs over urls.length while the callback tests
// loaded === 6, exactly as upstream, so an array that is not six long never calls back; and the order
// is the caller's, documented upstream as pos-x, neg-x, pos-y, neg-y, pos-z, neg-z.
//
// The faces are written through CubeTexture::images rather than through the JS property: that property
// is a native ImageProperty over a TextureImage whose Faces are runtime objects, so assigning a plain
// record to it is refused. Each face is built as a TextureImage carrying the record's width, height and
// pixel data, which is what the rest of the renderer reads.
//
// The texture is returned synchronously and its colour space is srgb. onProgress is unsupported
// upstream and is passed through untouched.
class CubeTextureLoader : public Loader {
  public:
    explicit CubeTextureLoader(JSValue manager={});
    static CubeTextureLoader fromObject(JSValue value){return CubeTextureLoader(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue load(JSValue urls={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    CubeTextureLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};