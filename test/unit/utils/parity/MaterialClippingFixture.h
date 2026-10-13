#pragma once
#include "core/NativeObject.h"
#include "materials/Material.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Plane.h"
#include "objects/ClippingGroup.h"
#include "parity/WebGLClippingFixture.h"
namespace MaterialClippingFixture {
using ClippingFixture::A;
using ClippingFixture::Args;
using ClippingFixture::array;
using ClippingFixture::encode;
using ClippingFixture::invoke;
using ClippingFixture::O;
using ClippingFixture::object;
using ClippingFixture::outcome;
using ClippingFixture::prop;
inline std::shared_ptr<Plane> makePlane(unsigned n) {
    return std::make_shared<Plane>(n % 2 ? Vector3(0, 1, 0) : Vector3(1, 0, 0),
                                   static_cast<double>(n + 2));
}
inline JsonValue planes(JSValue value) {
    if (value.isNull())
        return nullptr;
    A result;
    for (unsigned i = 0; i < value.get("length").toNumber(); ++i) {
        auto p = value.get(std::to_string(i));
        if (p.isNull()) {
            result.push_back(nullptr);
            continue;
        }
        auto n = p.get("normal");
        result.emplace_back(
            O{{"normal", A{n.get("x").number(), n.get("y").number(), n.get("z").number()}},
              {"constant", p.get("constant").number()}});
    }
    return result;
}
struct Fixture {
    std::shared_ptr<Material> source = std::make_shared<MeshBasicMaterial>(),
                              target = std::make_shared<MeshBasicMaterial>();
    std::shared_ptr<Plane> p = makePlane(0), q = makePlane(1), r = makePlane(2);
    JSValue s, t, original, targetOriginal;
    Fixture() {
        source->name = "source";
        target->name = "target";
        source->clippingPlanes = std::vector{p, q};
        target->clippingPlanes = std::vector{r};
        s = nativeMaterial(source);
        t = nativeMaterial(target);
        original = s.get("clippingPlanes");
        targetOriginal = t.get("clippingPlanes");
    }
    JsonValue snapshot() {
        auto current = s.get("clippingPlanes"), other = t.get("clippingPlanes");
        return O{{"source", planes(current)},
                 {"target", planes(other)},
                 {"old", planes(original)},
                 {"targetOld", planes(targetOriginal)},
                 {"sameOld", current.sameValue(original)},
                 {"sameTargetOld", other.sameValue(targetOriginal)},
                 {"sameFields", current.sameValue(other)},
                 {"names", A{source->name, target->name}}};
    }
};
inline JsonValue scenario(unsigned mode) {
    Fixture f;
    JSValue cloned;
    JsonValue json = nullptr;
    auto result = outcome([&]() -> JSValue {
        switch (mode) {
        case 0:
            break;
        case 1:
            f.target->clippingPlanes = f.source->clippingPlanes;
            f.t.get("clippingPlanes").get("0").set("constant", 12);
            break;
        case 2:
            f.s.set("clippingPlanes", f.targetOriginal);
            f.source->clippingPlanes->push_back(f.q);
            break;
        case 3:
            f.source->clippingPlanes->push_back(f.r);
            break;
        case 4:
            invoke(f.original, "push", {nativePlane(f.r)});
            break;
        case 5:
            invoke(f.original, "shift");
            break;
        case 6:
            f.source->clippingPlanes = std::vector{f.r};
            f.original.get("0").set("constant", 8);
            break;
        case 7:
            f.source->clippingPlanes.reset();
            invoke(f.original, "push", {nativePlane(f.r)});
            break;
        case 8: {
            auto alias = *f.source->clippingPlanes;
            f.source->clippingPlanes = std::vector{f.r};
            alias.push_back(f.r);
            break;
        }
        case 9:
            cloned = invoke(f.s, "clone");
            break;
        case 10:
            f.target->copy(*f.source);
            break;
        case 11:
            f.source->copy(*f.source);
            break;
        case 12:
            json = f.source->toJSON().at("clippingPlanes");
            break;
        case 13:
            f.source->fromJSON(O{{"clippingPlanes", A{f.r->toJSON()}}});
            break;
        case 14:
            f.source->clippingPlanes.reset();
            cloned = invoke(f.s, "clone");
            break;
        case 15:
            f.source->clippingPlanes.emplace();
            cloned = invoke(f.s, "clone");
            break;
        case 16:
            (*f.source->clippingPlanes)[1] = nullptr;
            f.target->copy(*f.source);
            break;
        case 17: {
            auto group = std::make_shared<ClippingGroup>();
            group->clippingPlanes = *f.source->clippingPlanes;
            f.t.set("clippingPlanes", nativeObject(group).get("clippingPlanes"));
            group->clippingPlanes.push_back(f.r);
            break;
        }
        case 18:
            f.s.set("clippingPlanes", nullptr);
            break;
        case 19:
            invoke(f.s, "setValues", {object({{"clippingPlanes", f.targetOriginal}})});
            break;
        case 20:
            prop(f.s, "clippingPlanes", f.targetOriginal);
            break;
        case 21:
            invoke(f.original, "pop");
            invoke(f.original, "unshift", {nativePlane(f.r)});
            break;
        case 22:
            f.source->clippingPlanes->clear();
            break;
        case 24:
            invoke(f.original, "splice", {0, 1});
            break;
        case 23:
            cloned = invoke(f.s, "clone");
            cloned.get("clippingPlanes").get("0").set("constant", 27);
            break;
        }
        return {};
    });
    JsonValue clone = nullptr;
    if (cloned.isObject()) {
        auto entries = cloned.get("clippingPlanes");
        clone = O{{"values", planes(entries)},
                  {"sameArray", entries.sameValue(f.s.get("clippingPlanes"))},
                  {"sameFirst", !entries.isNull() && entries.get("length").number() > 0 &&
                                    entries.get("0").sameValue(f.original.get("0"))}};
    }
    return O{{"mode", mode},
             {"result", result},
             {"snapshot", f.snapshot()},
             {"clone", clone},
             {"json", json},
             {"repeatIdentity", f.s.get("clippingPlanes").sameValue(f.s.get("clippingPlanes"))}};
}
} // namespace MaterialClippingFixture
