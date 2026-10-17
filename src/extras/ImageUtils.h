#pragma once
#include "core/JSValue.h"

// Port of src/extras/ImageUtils.js at the pinned revision (see upstream.json).
//
// Upstream guards two of its branches on DOM types: getDataURL falls back to image.src when
// HTMLCanvasElement does not exist, and sRGBToLinear only takes the canvas path when the argument is an
// HTMLImageElement, HTMLCanvasElement or ImageBitmap. None of those exist natively, so the branches
// upstream already defines for that situation are the only reachable ones - which is why this port
// matches the pinned behaviour rather than approximating it.
//
// getDataURL therefore returns image.src unchanged, which is also the data: shortcut upstream takes
// first. sRGBToLinear converts image.data element by element: Uint8Array and Uint8ClampedArray are
// floored after scaling by 255, every other array type is converted as floating point data, and an
// argument without data is returned unchanged. Upstream warns to the console in that last case; there is
// no console here, so that is recorded rather than reproduced.
class ImageUtils {
  public:
    static JSValue object();
    static JSValue getDataURL(JSValue image, JSValue type = JSValue("image/png"));
    static JSValue sRGBToLinear(JSValue image);
};