#include "geometries/Geometries.h"
#include "core/JSModuleObject.h"

JSValue nativeGeometriesModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope guard(initializing, "Geometries module");
    auto module = JSValue::object(nullptr);
#define THREECPP_GEOMETRY(name) module.set(#name, native##name##Constructor());
    THREECPP_GEOMETRY(BoxGeometry)
    THREECPP_GEOMETRY(CapsuleGeometry)
    THREECPP_GEOMETRY(CircleGeometry)
    THREECPP_GEOMETRY(ConeGeometry)
    THREECPP_GEOMETRY(CylinderGeometry)
    THREECPP_GEOMETRY(DodecahedronGeometry)
    THREECPP_GEOMETRY(EdgesGeometry)
    THREECPP_GEOMETRY(ExtrudeGeometry)
    THREECPP_GEOMETRY(IcosahedronGeometry)
    THREECPP_GEOMETRY(LatheGeometry)
    THREECPP_GEOMETRY(OctahedronGeometry)
    THREECPP_GEOMETRY(PlaneGeometry)
    THREECPP_GEOMETRY(PolyhedronGeometry)
    THREECPP_GEOMETRY(RingGeometry)
    THREECPP_GEOMETRY(ShapeGeometry)
    THREECPP_GEOMETRY(SphereGeometry)
    THREECPP_GEOMETRY(TetrahedronGeometry)
    THREECPP_GEOMETRY(TorusGeometry)
    THREECPP_GEOMETRY(TorusKnotGeometry)
    THREECPP_GEOMETRY(TubeGeometry)
    THREECPP_GEOMETRY(WireframeGeometry)
#undef THREECPP_GEOMETRY
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeGeometriesBinding(const std::string& name) {
    return nativeGeometriesModule().get(name);
}

JSValue nativeObjectLoaderGeometries() {
    return nativeGeometriesModule();
}
