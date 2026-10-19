#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"

// Port of src/loaders/ImageBitmapLoader.js at the pinned revision.
//
// Upstream fetches the url and hands the blob to createImageBitmap, behind a promise and an abort controller.
// There is no fetch here, so that host boundary is reduced to one injectable decoder, the same shape the image
// loader already uses: it receives {url, options, crossOrigin} and returns a bitmap record, or throws. Because
// the decoder answers immediately the loader is synchronous, where upstream resolves a promise.
//
// The parts that are not host bound are transcribed: the image-bitmap: cache key, the path and manager
// resolution, the itemStart/itemEnd/itemError ordering, the options record with premultiplyAlpha defaulting to
// none, and setOptions replacing it wholesale. The promise branch for a cached in-flight request has no
// counterpart once the decoder is synchronous, and is noted rather than faked.
class ImageBitmapLoader : public Loader {
  public:
    explicit ImageBitmapLoader(JSValue manager={});
    static ImageBitmapLoader fromObject(JSValue value){return ImageBitmapLoader(std::move(value),Existing{});}
    static JSValue prototype();
    static void setDecoder(JSValue decoder);
    static JSValue decoder();
    JSValue setOptions(JSValue options={});
    // Returns the bitmap record. Synchronous, like the other loaders in this port.
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    ImageBitmapLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};