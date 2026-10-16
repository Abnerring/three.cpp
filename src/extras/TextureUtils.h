#pragma once
#include "core/JSValue.h"
JSValue contain(const JSValue& texture = {}, const JSValue& aspect = {});
JSValue cover(const JSValue& texture = {}, const JSValue& aspect = {});
JSValue fill(const JSValue& texture = {});
JSValue getByteLength(const JSValue& width = {}, const JSValue& height = {},
                      const JSValue& format = {}, const JSValue& type = {});

// The ordinary function exports and static class retain separate realm-local identities.
JSValue nativeTextureContain();
JSValue nativeTextureCover();
JSValue nativeTextureFill();
JSValue nativeTextureByteLength();
JSValue nativeTextureUtils();

class TextureUtils {
  public:
    TextureUtils() : object_(constructor().construct()) {}
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeTextureUtils(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    static JSValue contain(const JSValue& texture = {}, const JSValue& aspect = {}) { return ::contain(texture, aspect); }
    static JSValue cover(const JSValue& texture = {}, const JSValue& aspect = {}) { return ::cover(texture, aspect); }
    static JSValue fill(const JSValue& texture = {}) { return ::fill(texture); }
    static JSValue getByteLength(const JSValue& width = {}, const JSValue& height = {},
                                 const JSValue& format = {}, const JSValue& type = {}) {
        return ::getByteLength(width, height, format, type);
    }
  private:
    JSValue object_;
};

// Pinned three.js texture byte accounting. Inputs retain JavaScript conversion order.
JSValue nativeTextureGetTextureTypeByteLength(const JSValue& type = {});
JSValue nativeTextureGetByteLength(const JSValue& width = {}, const JSValue& height = {},
                                   const JSValue& format = {}, const JSValue& type = {});
