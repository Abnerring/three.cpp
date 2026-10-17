#pragma once
#include "core/BufferGeometry.h"
#include "core/NativeObject.h"
#include "loaders/Loader.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Port of examples/jsm/loaders/STLLoader.js at the pinned revision (see upstream.json).
//
// The parse dispatches on the same test the pinned source uses: a face count read from offset 80
// predicts the exact length of a binary file, and failing that the first five offsets are checked
// for the bytes of "solid". The binary path is implemented here, including the Magics header
// colour and the packed per face colour; because both are integers over 31 or 255 followed by an
// sRGB conversion, the values are compared as float32 bit patterns in the differential.
//
// The ASCII path is not implemented natively yet and fails loudly rather than returning an empty
// geometry. Upstream also writes to the console when a face carries the wrong number of normals or
// vertices; there is no console here, so that diagnostic is not reproduced.
class STLLoader : public Loader {
  public:
    explicit STLLoader(JSValue manager={});
    static STLLoader fromObject(JSValue value){return STLLoader(std::move(value),Existing{});}
    static JSValue prototype();
    static bool isBinary(const std::vector<std::uint8_t>& data);
    std::shared_ptr<BufferGeometry> parseBytes(const std::vector<std::uint8_t>& data,bool* hasColorsOut=nullptr,double* alphaOut=nullptr,
        std::vector<std::string>* groupNamesOut=nullptr);
    JSValue parse(JSValue data);
    JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
  private:
    STLLoader(JSValue value,Existing):Loader(std::move(value),Existing{}) {}
};
