#pragma once
#include "core/JSValue.h"
class LoaderUtils {
  public:
    static JSValue extractUrlBase(JSValue url);
    static JSValue resolveURL(JSValue url,JSValue path);
};
