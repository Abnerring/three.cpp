#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"
#include "textures/CompressedTexture.h"
#include <memory>

// Port of src/loaders/CompressedTextureLoader.js at the pinned revision (see upstream.json).
//
// The base class for the compressed format loaders: FileLoader fetches the bytes under the arraybuffer
// response type, the subclass parse hook turns them into a texData record, and the texture is built from
// it. Two of upstream's three shapes are not representable natively and fail loudly rather than being
// dropped:
//
//  - a cubemap packed into one file (isCubemap), and
//  - the legacy six-url array form,
//
// because both attach a separate mipmap chain to each face, while a native texture keeps one mipmap
// chain for the whole texture. The single-file, single-image form - what the DDS and KTX loaders take -
// maps directly: the base mip sizes the image, the mipmap chain becomes the texture's mipmaps, a single
// mip forces the plain min filter, and the format comes from the texData.
//
// No differential yet: the mapping lives inside load, whose transport is a browser fetch, so comparing
// it needs a fetch stub on the oracle side. The unit cases cover the mapping itself, and this note is
// here rather than an implied claim that parity has been measured.
class CompressedTextureLoader : public Loader {
  public:
    explicit CompressedTextureLoader(JSValue manager={});
    static CompressedTextureLoader fromObject(JSValue value){return CompressedTextureLoader(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
    // Subclass hook; the base refuses, because upstream's parse is abstract.
    virtual JSValue parse(JSValue buffer, JSValue premultiplyAlpha);
    void applyTexData(const std::shared_ptr<CompressedTexture>& texture, JSValue texDatas);
  protected:
    explicit CompressedTextureLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};