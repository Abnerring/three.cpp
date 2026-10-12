#pragma once
#include "core/JSValue.h"
#include <functional>
#include <memory>
#include <vector>

class Object3D;

// Optional cross-cast interface for types supplied by addons. The core library
// depends on this interface, never on the addon class or its constructors.
class NativeObjectExtension {
  public:
    using ObjectVisitor = std::function<void(const std::shared_ptr<Object3D>&)>;
    virtual ~NativeObjectExtension() = default;
    virtual void initializeNativeWrapper(JSValue wrapper) = 0;
    // Pass each actual owning shared_ptr field by reference. The collector uses
    // its address to count/deduplicate slots; copies and temporaries are invalid.
    // GC callbacks must not allocate wrappers, invoke JS, or mutate native data.
    virtual void visitNativeObjectReferences(const ObjectVisitor& visitor) const = 0;
    // Return addresses of actual JSValue fields, without copying their values.
    virtual std::vector<const JSValue*> nativeJSReferences() const = 0;
};
