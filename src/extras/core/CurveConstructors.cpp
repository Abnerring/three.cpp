// Generated native curve constructors and lexical class identities.
#include "extras/core/Curve.h"
#include "extras/core/CurvePath.h"
#include "extras/core/Path.h"
#include "extras/core/Shape.h"
#include "extras/curves/EllipseCurve.h"
#include "extras/curves/ArcCurve.h"
#include "extras/curves/LineCurve.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/QuadraticBezierCurve.h"
#include "extras/curves/QuadraticBezierCurve3.h"
#include "extras/curves/CubicBezierCurve.h"
#include "extras/curves/CubicBezierCurve3.h"
#include "extras/curves/SplineCurve.h"
#include "extras/curves/CatmullRomCurve3.h"
#include "extras/curves/Curves.h"
#include "extras/core/CurveConstructors.h"
#include "extras/core/CurveConstructorsSources.inc"
#include "extras/core/CurveRuntime.h"
#include "extras/core/CurvePathRuntime.h"
#include "extras/curves/CurvesRuntime.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
void own(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = std::move(value);
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(name, descriptor);
}
JSValue construct(const JSValue& newTarget, const Args& args, const Args& captures) {
    const auto constructor = captures[0].getInternalSlot("constructor");
    return constructor.getInternalSlot("nativeCurveConstructor").construct(args, newTarget);
}
JSValue classModule(const char* name) {
    auto constructor = nativeCurveClassTable().get(name);
    auto module = constructor.getInternalSlot("nativeCurveModule");
    if (module.isUndefined()) {
        module = JSValue::object(nullptr);
        own(module, name, constructor);
        constructor.setInternalSlot("nativeCurveModule", module);
    }
    return module;
}
}
JSValue nativeCurveClassTable() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope guard(initializing, "Curve classes");
    auto table = JSValue::object(nullptr);
    const auto add = [&](const char* name, const char* parent, unsigned arity, const char* source) {
        auto holder = JSValue::object(nullptr);
        auto constructor = JSValue::builtinConstructor(construct, {holder}, name, arity);
        holder.setInternalSlot("constructor", constructor);
        if (parent) {
            constructor.setPrototype(table.get(parent));
            constructor.get("prototype").setPrototype(table.get(parent).get("prototype"));
        }
        constructor.setFunctionSourceText(source);
        own(table, name, constructor);
    };
    add("Curve", nullptr, 0, CurveConstructorsSource::Curve);
    add("CurvePath", "Curve", 0, CurveConstructorsSource::CurvePath);
    add("Path", "CurvePath", 1, CurveConstructorsSource::Path);
    add("Shape", "Path", 1, CurveConstructorsSource::Shape);
    add("EllipseCurve", "Curve", 0, CurveConstructorsSource::EllipseCurve);
    add("ArcCurve", "EllipseCurve", 6, CurveConstructorsSource::ArcCurve);
    add("LineCurve", "Curve", 0, CurveConstructorsSource::LineCurve);
    add("LineCurve3", "Curve", 0, CurveConstructorsSource::LineCurve3);
    add("QuadraticBezierCurve", "Curve", 0, CurveConstructorsSource::QuadraticBezierCurve);
    add("QuadraticBezierCurve3", "Curve", 0, CurveConstructorsSource::QuadraticBezierCurve3);
    add("CubicBezierCurve", "Curve", 0, CurveConstructorsSource::CubicBezierCurve);
    add("CubicBezierCurve3", "Curve", 0, CurveConstructorsSource::CubicBezierCurve3);
    add("SplineCurve", "Curve", 0, CurveConstructorsSource::SplineCurve);
    add("CatmullRomCurve3", "Curve", 0, CurveConstructorsSource::CatmullRomCurve3);
    installNativeCurveNumericMethods(table.get("Curve"), table);
    installNativeCurveSubclassMethods(table);
    installNativeCurvePathMethods(table);
    installNativePathMethods(table);
    installNativeShapeMethods(table);
    table.retainForHeapLifetime();
    weak = JSWeakValue(table);
    return table;
}
JSValue nativeCurvesModule() {
    auto table = nativeCurveClassTable();
    auto module = table.getInternalSlot("nativeCurvesModule");
    if (module.isUndefined()) {
        module = JSValue::object(nullptr);
        own(module, "ArcCurve", table.get("ArcCurve"));
        own(module, "CatmullRomCurve3", table.get("CatmullRomCurve3"));
        own(module, "CubicBezierCurve", table.get("CubicBezierCurve"));
        own(module, "CubicBezierCurve3", table.get("CubicBezierCurve3"));
        own(module, "EllipseCurve", table.get("EllipseCurve"));
        own(module, "LineCurve", table.get("LineCurve"));
        own(module, "LineCurve3", table.get("LineCurve3"));
        own(module, "QuadraticBezierCurve", table.get("QuadraticBezierCurve"));
        own(module, "QuadraticBezierCurve3", table.get("QuadraticBezierCurve3"));
        own(module, "SplineCurve", table.get("SplineCurve"));
        table.setInternalSlot("nativeCurvesModule", module);
    }
    return module;
}
JSValue nativeCurvesBinding(const std::string& name) {
    const auto table = nativeCurveClassTable();
    for (const auto* key : {"ArcCurve", "CatmullRomCurve3", "CubicBezierCurve", "CubicBezierCurve3", "EllipseCurve", "LineCurve", "LineCurve3", "QuadraticBezierCurve", "QuadraticBezierCurve3", "SplineCurve"})
        if (name == key) return table.get(name);
    return {};
}
JSValue nativeCurveConstructor() { return nativeCurveClassTable().get("Curve"); }
JSValue nativeCurveModule() { return classModule("Curve"); }
JSValue nativeCurveBinding(const std::string& name) { return name == "Curve" ? nativeCurveConstructor() : JSValue{}; }
JSValue nativeCurvePathConstructor() { return nativeCurveClassTable().get("CurvePath"); }
JSValue nativeCurvePathModule() { return classModule("CurvePath"); }
JSValue nativeCurvePathBinding(const std::string& name) { return name == "CurvePath" ? nativeCurvePathConstructor() : JSValue{}; }
JSValue nativePathConstructor() { return nativeCurveClassTable().get("Path"); }
JSValue nativePathModule() { return classModule("Path"); }
JSValue nativePathBinding(const std::string& name) { return name == "Path" ? nativePathConstructor() : JSValue{}; }
JSValue nativeShapeConstructor() { return nativeCurveClassTable().get("Shape"); }
JSValue nativeShapeModule() { return classModule("Shape"); }
JSValue nativeShapeBinding(const std::string& name) { return name == "Shape" ? nativeShapeConstructor() : JSValue{}; }
JSValue nativeEllipseCurveConstructor() { return nativeCurveClassTable().get("EllipseCurve"); }
JSValue nativeEllipseCurveModule() { return classModule("EllipseCurve"); }
JSValue nativeEllipseCurveBinding(const std::string& name) { return name == "EllipseCurve" ? nativeEllipseCurveConstructor() : JSValue{}; }
JSValue nativeArcCurveConstructor() { return nativeCurveClassTable().get("ArcCurve"); }
JSValue nativeArcCurveModule() { return classModule("ArcCurve"); }
JSValue nativeArcCurveBinding(const std::string& name) { return name == "ArcCurve" ? nativeArcCurveConstructor() : JSValue{}; }
JSValue nativeLineCurveConstructor() { return nativeCurveClassTable().get("LineCurve"); }
JSValue nativeLineCurveModule() { return classModule("LineCurve"); }
JSValue nativeLineCurveBinding(const std::string& name) { return name == "LineCurve" ? nativeLineCurveConstructor() : JSValue{}; }
JSValue nativeLineCurve3Constructor() { return nativeCurveClassTable().get("LineCurve3"); }
JSValue nativeLineCurve3Module() { return classModule("LineCurve3"); }
JSValue nativeLineCurve3Binding(const std::string& name) { return name == "LineCurve3" ? nativeLineCurve3Constructor() : JSValue{}; }
JSValue nativeQuadraticBezierCurveConstructor() { return nativeCurveClassTable().get("QuadraticBezierCurve"); }
JSValue nativeQuadraticBezierCurveModule() { return classModule("QuadraticBezierCurve"); }
JSValue nativeQuadraticBezierCurveBinding(const std::string& name) { return name == "QuadraticBezierCurve" ? nativeQuadraticBezierCurveConstructor() : JSValue{}; }
JSValue nativeQuadraticBezierCurve3Constructor() { return nativeCurveClassTable().get("QuadraticBezierCurve3"); }
JSValue nativeQuadraticBezierCurve3Module() { return classModule("QuadraticBezierCurve3"); }
JSValue nativeQuadraticBezierCurve3Binding(const std::string& name) { return name == "QuadraticBezierCurve3" ? nativeQuadraticBezierCurve3Constructor() : JSValue{}; }
JSValue nativeCubicBezierCurveConstructor() { return nativeCurveClassTable().get("CubicBezierCurve"); }
JSValue nativeCubicBezierCurveModule() { return classModule("CubicBezierCurve"); }
JSValue nativeCubicBezierCurveBinding(const std::string& name) { return name == "CubicBezierCurve" ? nativeCubicBezierCurveConstructor() : JSValue{}; }
JSValue nativeCubicBezierCurve3Constructor() { return nativeCurveClassTable().get("CubicBezierCurve3"); }
JSValue nativeCubicBezierCurve3Module() { return classModule("CubicBezierCurve3"); }
JSValue nativeCubicBezierCurve3Binding(const std::string& name) { return name == "CubicBezierCurve3" ? nativeCubicBezierCurve3Constructor() : JSValue{}; }
JSValue nativeSplineCurveConstructor() { return nativeCurveClassTable().get("SplineCurve"); }
JSValue nativeSplineCurveModule() { return classModule("SplineCurve"); }
JSValue nativeSplineCurveBinding(const std::string& name) { return name == "SplineCurve" ? nativeSplineCurveConstructor() : JSValue{}; }
JSValue nativeCatmullRomCurve3Constructor() { return nativeCurveClassTable().get("CatmullRomCurve3"); }
JSValue nativeCatmullRomCurve3Module() { return classModule("CatmullRomCurve3"); }
JSValue nativeCatmullRomCurve3Binding(const std::string& name) { return name == "CatmullRomCurve3" ? nativeCatmullRomCurve3Constructor() : JSValue{}; }
