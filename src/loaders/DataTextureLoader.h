#pragma once
#include "core/JSValue.h"
#include "loaders/Loader.h"
#include "textures/DataTexture.h"
#include <memory>

// Port of src/loaders/DataTextureLoader.js at the pinned revision (see upstream.json).
//
// This is the base class for the format loaders: it loads the bytes with FileLoader under the
// arraybuffer response type, hands them to parse - which a subclass provides - and applies the returned
// texData record to a DataTexture. The base's own parse has nothing to do and fails loudly, exactly as
// upstream's abstract hook never returns anything useful on its own.
//
// applyTexData is the mapping worth reading: every field falls back to the upstream default, an image
// record wins over width/height/data, mipmaps force the mipmap min filter, a mipmapCount of one forces
// LinearFilter back, and the texture is marked for upload at the end. Assigning mipmaps is not
// implemented natively because a mipmap chain holds texture images rather than records, so that one
// field fails loudly instead of being skipped.
class DataTextureLoader : public Loader {
  public:
    explicit DataTextureLoader(JSValue manager={});
    static DataTextureLoader fromObject(JSValue value){return DataTextureLoader(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
    JSValue createDataTexture(JSValue buffer);
    // Subclass hook. The base refuses, because upstream's parse is abstract.
    virtual JSValue parse(JSValue buffer);
    void applyTexData(const std::shared_ptr<DataTexture>& texture, JSValue texData);
  protected:
    DataTextureLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};