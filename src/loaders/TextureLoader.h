#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"

// Port of src/loaders/TextureLoader.js at the pinned revision (see upstream.json).
//
// The whole of load is a thin wrapper over ImageLoader: create a Texture, hand it the same manager,
// crossOrigin and path, and when the image record arrives set it as the texture's image and mark the
// texture for upload before calling back. The texture is returned synchronously, so a caller can use
// it before the image exists - which is what the pinned loader documents.
//
// onProgress is unsupported upstream and is passed through untouched.
class TextureLoader : public Loader {
  public:
    explicit TextureLoader(JSValue manager={});
    static TextureLoader fromObject(JSValue value){return TextureLoader(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    TextureLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};
