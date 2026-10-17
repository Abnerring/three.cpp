#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"

// Port of src/loaders/ImageLoader.js at the pinned revision (see upstream.json).
//
// Upstream builds a DOM Image and sets its src; there is no DOM here. That host boundary is reduced
// to one injectable module-level decoder, documented in docs/HANDOFF.md item 22: it receives
// {url, crossOrigin} and returns {width, height, data} where data is RGBA8 bytes, or throws. An
// image record is the JS object {width, height, data, complete:true}, which is what the cache holds
// and what callbacks receive.
//
// Everything else transcribes the pinned loader: the `image:` cache key, the hit path that either
// calls back immediately from a complete record or queues the callbacks against one still loading,
// the duplicate-request map keyed by url, the itemStart/itemEnd/itemError ordering, and the rule
// that crossOrigin is only set for urls that are not data URIs. onProgress is unsupported upstream
// and is not consulted here either.
//
// The SVG path through document.createElementNS is not implemented and fails loudly.
class ImageLoader : public Loader {
  public:
    explicit ImageLoader(JSValue manager={});
    static ImageLoader fromObject(JSValue value){return ImageLoader(std::move(value),Existing{});}
    static JSValue prototype();
    static void setDecoder(JSValue decoder);
    static JSValue decoder();
    // Returns the image record. Synchronous, like the other loaders in this port: an injected decoder
    // answers immediately where the browser would fire a load event later.
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    ImageLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};
