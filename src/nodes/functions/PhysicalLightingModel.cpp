// Native translation of pinned three.js PhysicalLightingModel.js (11738a).
#include "nodes/functions/PhysicalLightingModel.h"
#include "constants.h"
#include "core/JSGlobal.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/core/LightingModel.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/Camera.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/TextureBicubic.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/display/ViewportTextureNode.h"
#include "nodes/functions/BSDF/BRDF_Lambert.h"
#include "nodes/functions/BSDF/BRDF_EON.h"
#include "nodes/functions/BSDF/BRDF_GGX.h"
#include "nodes/functions/BSDF/DFGLUT.h"
#include "nodes/functions/BSDF/EnvironmentBRDF.h"
#include "nodes/functions/BSDF/F_Schlick.h"
#include "nodes/functions/BSDF/Schlick_to_F0.h"
#include "nodes/functions/BSDF/BRDF_Sheen.h"
#include "nodes/functions/BSDF/LTC.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/utils/LoopNode.h"
#include <cmath>

namespace {
using namespace LightingNodeRuntime;
#include "PhysicalLightingModelSources.inc"
JSWeakValue& lexical() { static thread_local JSWeakValue value; return value; }
JSValue withSource(JSValue value, const char* source) { value.setFunctionSourceText(source); return value; }
JSValue nextDestructured(JSIteratorRecord& iterator) {
    if (iterator.done) return {};
    const auto value = stepJSIterator(iterator);
    return value ? *value : JSValue{};
}

JSValue start(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto builder = arg(args, 0);
    if (JSValue(self.get("clearcoat").strictEquals(JSValue(true))).toBoolean()) {
        self.set("clearcoatRadiance", invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("clearcoatRadiance")}));
        self.set("clearcoatSpecularDirect", invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("clearcoatSpecularDirect")}));
        self.set("clearcoatSpecularIndirect", invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("clearcoatSpecularIndirect")}));
    }
    if (JSValue(self.get("sheen").strictEquals(JSValue(true))).toBoolean()) {
        self.set("sheenSpecularDirect", invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("sheenSpecularDirect")}));
        self.set("sheenSpecularIndirect", invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("sheenSpecularIndirect")}));
    }
    if (JSValue(self.get("iridescence").strictEquals(JSValue(true))).toBoolean()) {
        auto dotNVi = invoke(invoke(module.get("normalView"), "dot")({module.get("positionViewDirection")}), "clamp")({});
        auto iridescenceFresnelDielectric = module.get("evalIridescence").call({}, {([&]() { auto object1 = JSValue::object(); own(object1, "outsideIOR", module.get("float").call({}, {JSValue(1.0)})); own(object1, "eta2", module.get("iridescenceIOR")); own(object1, "cosTheta1", dotNVi); own(object1, "thinFilmThickness", module.get("iridescenceThickness")); own(object1, "baseF0", module.get("specularColor")); return object1; }())});
        auto iridescenceFresnelMetallic = module.get("evalIridescence").call({}, {([&]() { auto object2 = JSValue::object(); own(object2, "outsideIOR", module.get("float").call({}, {JSValue(1.0)})); own(object2, "eta2", module.get("iridescenceIOR")); own(object2, "cosTheta1", dotNVi); own(object2, "thinFilmThickness", module.get("iridescenceThickness")); own(object2, "baseF0", module.get("diffuseColor").get("rgb")); return object2; }())});
        self.set("iridescenceFresnel", module.get("mix").call({}, {iridescenceFresnelDielectric, iridescenceFresnelMetallic, module.get("metalness")}));
        self.set("iridescenceF0Dielectric", module.get("Schlick_to_F0").call({}, {([&]() { auto object3 = JSValue::object(); own(object3, "f", iridescenceFresnelDielectric); own(object3, "f90", JSValue(1.0)); own(object3, "dotVH", dotNVi); return object3; }())}));
        self.set("iridescenceF0Metallic", module.get("Schlick_to_F0").call({}, {([&]() { auto object4 = JSValue::object(); own(object4, "f", iridescenceFresnelMetallic); own(object4, "f90", JSValue(1.0)); own(object4, "dotVH", dotNVi); return object4; }())}));
    }
    if (JSValue(self.get("transmission").strictEquals(JSValue(true))).toBoolean()) {
        auto position = module.get("positionWorld");
        auto v = invoke(invoke(module.get("cameraPosition"), "sub")({module.get("positionWorld")}), "normalize")({});
        auto n = module.get("normalWorld");
        auto context = builder.get("context");
        context.set("backdrop", module.get("getIBLVolumeRefraction").call({}, {n, v, module.get("roughness"), module.get("diffuseContribution"), module.get("specularColorBlended"), module.get("specularF90"), position, module.get("modelWorldMatrix"), module.get("cameraViewMatrix"), module.get("cameraProjectionMatrix"), module.get("ior"), module.get("thickness"), module.get("attenuationColor"), module.get("attenuationDistance"), (self.get("dispersion").toBoolean() ? module.get("dispersion") : JSValue(nullptr))}));
        context.set("backdropAlpha", module.get("transmission"));
        invoke(module.get("diffuseColor").get("a"), "mulAssign")({module.get("mix").call({}, {JSValue(1.0), context.get("backdrop").get("a"), module.get("transmission")})});
    }
    auto dotNV = invoke(invoke(module.get("normalView"), "dot")({module.get("positionViewDirection")}), "clamp")({});
    self.set("dfg", invoke(module.get("DFGLUT").call({}, {([&]() { auto object5 = JSValue::object(); own(object5, "roughness", module.get("roughness")); own(object5, "dotNV", dotNV); return object5; }())}), "toConst")({JSValue("dfg")}));
    auto Ess = invoke(self.get("dfg").get("x"), "add")({self.get("dfg").get("y")});
    self.set("multiScatteringCompensation", invoke(invoke(invoke(module.get("specularColorBlended"), "mul")({invoke(invoke(Ess, "reciprocal")({}), "sub")({JSValue(1.0)})}), "add")({JSValue(1.0)}), "toConst")({JSValue("multiScatteringCompensation")}));
    self.set("singleScatteringDielectric", invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("singleScatteringDielectric")}));
    self.set("multiScatteringDielectric", invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("multiScatteringDielectric")}));
    invoke(self, "computeMultiscattering")({self.get("singleScatteringDielectric"), self.get("multiScatteringDielectric"), module.get("specularF90"), module.get("specularColor"), self.get("iridescenceF0Dielectric")});
    if (JSValue(self.get("diffuseRoughness").strictEquals(JSValue(true))).toBoolean()) {
        self.set("eonDirectionalAlbedo", module.get("EON_DirectionalAlbedo").call({}, {([&]() { auto object6 = JSValue::object(); own(object6, "diffuseColor", module.get("diffuseColor").get("rgb")); own(object6, "roughness", module.get("diffuseRoughness")); own(object6, "dotNV", dotNV); return object6; }())}));
    }
    module.get("default").get("prototype").getPrototype().get("start", self).call(self, {builder});
    return JSValue{};
}

JSValue computeMultiscattering(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto singleScatter = arg(args, 0);
    auto multiScatter = arg(args, 1);
    auto specularF90 = arg(args, 2);
    auto f0 = arg(args, 3);
    auto argument1 = arg(args, 4);
    if (argument1.isUndefined()) argument1 = JSValue(nullptr);
    auto iridescenceF0 = argument1;
    auto fab = self.get("dfg");
    auto Fr = (iridescenceF0.toBoolean() ? invoke(module.get("iridescence"), "mix")({f0, iridescenceF0}) : f0);
    auto FssEss = invoke(invoke(Fr, "mul")({fab.get("x")}), "add")({invoke(specularF90, "mul")({fab.get("y")})});
    auto Ess = invoke(fab.get("x"), "add")({fab.get("y")});
    auto Ems = invoke(Ess, "oneMinus")({});
    auto Favg = invoke(Fr, "add")({invoke(invoke(Fr, "oneMinus")({}), "mul")({JSValue(0.047619)})});
    auto Fms = invoke(invoke(FssEss, "mul")({Favg}), "div")({invoke(invoke(Ems, "mul")({Favg}), "oneMinus")({})});
    invoke(singleScatter, "addAssign")({FssEss});
    invoke(multiScatter, "addAssign")({invoke(Fms, "mul")({Ems})});
    return JSValue{};
}

JSValue direct(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    const auto destructured1 = arg(args, 0);
    auto lightDirection = destructured1.get("lightDirection");
    auto lightColor = destructured1.get("lightColor");
    auto reflectedLight = destructured1.get("reflectedLight");
    auto dotNL = invoke(invoke(module.get("normalView"), "dot")({lightDirection}), "clamp")({});
    auto irradiance = invoke(invoke(dotNL, "mul")({lightColor}), "toVar")({});
    if (JSValue(self.get("sheen").strictEquals(JSValue(true))).toBoolean()) {
        invoke(self.get("sheenSpecularDirect"), "addAssign")({invoke(irradiance, "mul")({module.get("BRDF_Sheen").call({}, {([&]() { auto object2 = JSValue::object(); own(object2, "lightDirection", lightDirection); return object2; }())})})});
        auto sheenAlbedoV = module.get("IBLSheenBRDF").call({}, {([&]() { auto object3 = JSValue::object(); own(object3, "normal", module.get("normalView")); own(object3, "viewDir", module.get("positionViewDirection")); own(object3, "roughness", module.get("sheenRoughness")); return object3; }())});
        auto sheenAlbedoL = module.get("IBLSheenBRDF").call({}, {([&]() { auto object4 = JSValue::object(); own(object4, "normal", module.get("normalView")); own(object4, "viewDir", lightDirection); own(object4, "roughness", module.get("sheenRoughness")); return object4; }())});
        auto sheenEnergyComp = invoke(invoke(invoke(invoke(module.get("sheen").get("r"), "max")({module.get("sheen").get("g")}), "max")({module.get("sheen").get("b")}), "mul")({invoke(sheenAlbedoV, "max")({sheenAlbedoL})}), "oneMinus")({});
        invoke(irradiance, "mulAssign")({sheenEnergyComp});
    }
    if (JSValue(self.get("clearcoat").strictEquals(JSValue(true))).toBoolean()) {
        auto dotNLcc = invoke(invoke(module.get("clearcoatNormalView"), "dot")({lightDirection}), "clamp")({});
        auto ccIrradiance = invoke(dotNLcc, "mul")({lightColor});
        invoke(self.get("clearcoatSpecularDirect"), "addAssign")({invoke(ccIrradiance, "mul")({module.get("BRDF_GGX").call({}, {([&]() { auto object5 = JSValue::object(); own(object5, "lightDirection", lightDirection); own(object5, "f0", module.get("clearcoatF0")); own(object5, "f90", module.get("clearcoatF90")); own(object5, "roughness", module.get("clearcoatRoughness")); own(object5, "normalView", module.get("clearcoatNormalView")); return object5; }())})})});
    }
    auto halfDir = invoke(invoke(lightDirection, "add")({module.get("positionViewDirection")}), "normalize")({});
    auto dotVH = invoke(invoke(module.get("positionViewDirection"), "dot")({halfDir}), "clamp")({});
    auto F = module.get("F_Schlick").call({}, {([&]() { auto object6 = JSValue::object(); own(object6, "f0", module.get("specularColor")); own(object6, "f90", module.get("specularF90")); own(object6, "dotVH", dotVH); return object6; }())});
    auto specularBRDF = module.get("BRDF_GGX").call({}, {([&]() { auto object7 = JSValue::object(); own(object7, "lightDirection", lightDirection); own(object7, "f0", module.get("specularColorBlended")); own(object7, "f90", module.get("specularF90")); own(object7, "roughness", module.get("roughness")); own(object7, "f", self.get("iridescenceFresnel")); own(object7, "USE_IRIDESCENCE", self.get("iridescence")); own(object7, "USE_ANISOTROPY", self.get("anisotropy")); return object7; }())});
    if (JSValue(self.get("retroreflection").strictEquals(JSValue(true))).toBoolean()) {
        auto retroViewDirection = invoke(invoke(module.get("positionViewDirection"), "negate")({}), "reflect")({module.get("normalView")});
        auto retroHalfDir = invoke(invoke(lightDirection, "add")({retroViewDirection}), "normalize")({});
        auto dotRetroVH = invoke(invoke(retroViewDirection, "dot")({retroHalfDir}), "clamp")({});
        auto retroF = module.get("F_Schlick").call({}, {([&]() { auto object8 = JSValue::object(); own(object8, "f0", module.get("specularColor")); own(object8, "f90", module.get("specularF90")); own(object8, "dotVH", dotRetroVH); return object8; }())});
        auto retroSpecularBRDF = module.get("BRDF_GGX").call({}, {([&]() { auto object9 = JSValue::object(); own(object9, "lightDirection", lightDirection); own(object9, "viewDirection", retroViewDirection); own(object9, "f0", module.get("specularColorBlended")); own(object9, "f90", module.get("specularF90")); own(object9, "roughness", module.get("roughness")); own(object9, "f", self.get("iridescenceFresnel")); own(object9, "USE_IRIDESCENCE", self.get("iridescence")); own(object9, "USE_ANISOTROPY", self.get("anisotropy")); return object9; }())});
        F = module.get("mix").call({}, {F, retroF, invoke(module.get("retroreflectivity"), "clamp")({})});
        specularBRDF = module.get("mix").call({}, {specularBRDF, retroSpecularBRDF, invoke(module.get("retroreflectivity"), "clamp")({})});
    }
    auto diffuseBRDF = (self.get("diffuseRoughness").toBoolean() ? invoke(module.get("BRDF_EON").call({}, {([&]() { auto object10 = JSValue::object(); own(object10, "lightDirection", lightDirection); own(object10, "diffuseColor", module.get("diffuseColor").get("rgb")); own(object10, "roughness", module.get("diffuseRoughness")); return object10; }())}), "mul")({invoke(module.get("metalness"), "oneMinus")({})}) : module.get("BRDF_Lambert").call({}, {([&]() { auto object11 = JSValue::object(); own(object11, "diffuseColor", module.get("diffuseContribution")); return object11; }())}));
    invoke(reflectedLight.get("directDiffuse"), "addAssign")({invoke(invoke(irradiance, "mul")({diffuseBRDF}), "mul")({invoke(F, "oneMinus")({})})});
    invoke(reflectedLight.get("directSpecular"), "addAssign")({invoke(invoke(irradiance, "mul")({specularBRDF}), "mul")({self.get("multiScatteringCompensation")})});
    return JSValue{};
}

JSValue directRectArea(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    const auto destructured1 = arg(args, 0);
    auto lightColor = destructured1.get("lightColor");
    auto lightPosition = destructured1.get("lightPosition");
    auto halfWidth = destructured1.get("halfWidth");
    auto halfHeight = destructured1.get("halfHeight");
    auto reflectedLight = destructured1.get("reflectedLight");
    auto ltc_1 = destructured1.get("ltc_1");
    auto ltc_2 = destructured1.get("ltc_2");
    auto p0 = invoke(invoke(lightPosition, "add")({halfWidth}), "sub")({halfHeight});
    auto p1 = invoke(invoke(lightPosition, "sub")({halfWidth}), "sub")({halfHeight});
    auto p2 = invoke(invoke(lightPosition, "sub")({halfWidth}), "add")({halfHeight});
    auto p3 = invoke(invoke(lightPosition, "add")({halfWidth}), "add")({halfHeight});
    auto N = module.get("normalView");
    auto V = module.get("positionViewDirection");
    auto P = invoke(module.get("positionView"), "toVar")({});
    auto uv = module.get("LTC_Uv").call({}, {([&]() { auto object2 = JSValue::object(); own(object2, "N", N); own(object2, "V", V); own(object2, "roughness", module.get("roughness")); return object2; }())});
    auto t1 = invoke(invoke(ltc_1, "sample")({uv}), "toVar")({});
    auto t2 = invoke(invoke(ltc_2, "sample")({uv}), "toVar")({});
    auto mInv = invoke(module.get("mat3").call({}, {module.get("vec3").call({}, {t1.get("x"), JSValue(0.0), t1.get("y")}), module.get("vec3").call({}, {JSValue(0.0), JSValue(1.0), JSValue(0.0)}), module.get("vec3").call({}, {t1.get("z"), JSValue(0.0), t1.get("w")})}), "toVar")({});
    auto fresnel = invoke(invoke(invoke(module.get("specularColorBlended"), "mul")({t2.get("x")}), "add")({invoke(invoke(module.get("specularF90"), "sub")({module.get("specularColorBlended")}), "mul")({t2.get("y")})}), "toVar")({});
    invoke(reflectedLight.get("directSpecular"), "addAssign")({invoke(invoke(lightColor, "mul")({fresnel}), "mul")({module.get("LTC_Evaluate").call({}, {([&]() { auto object3 = JSValue::object(); own(object3, "N", N); own(object3, "V", V); own(object3, "P", P); own(object3, "mInv", mInv); own(object3, "p0", p0); own(object3, "p1", p1); own(object3, "p2", p2); own(object3, "p3", p3); return object3; }())})})});
    invoke(reflectedLight.get("directDiffuse"), "addAssign")({invoke(invoke(lightColor, "mul")({module.get("diffuseContribution")}), "mul")({module.get("LTC_Evaluate").call({}, {([&]() { auto object4 = JSValue::object(); own(object4, "N", N); own(object4, "V", V); own(object4, "P", P); own(object4, "mInv", module.get("mat3").call({}, {JSValue(1.0), JSValue(0.0), JSValue(0.0), JSValue(0.0), JSValue(1.0), JSValue(0.0), JSValue(0.0), JSValue(0.0), JSValue(1.0)})); own(object4, "p0", p0); own(object4, "p1", p1); own(object4, "p2", p2); own(object4, "p3", p3); return object4; }())})})});
    if (JSValue(self.get("clearcoat").strictEquals(JSValue(true))).toBoolean()) {
        auto Ncc = module.get("clearcoatNormalView");
        auto uvClearcoat = module.get("LTC_Uv").call({}, {([&]() { auto object5 = JSValue::object(); own(object5, "N", Ncc); own(object5, "V", V); own(object5, "roughness", module.get("clearcoatRoughness")); return object5; }())});
        auto t1Clearcoat = invoke(ltc_1, "sample")({uvClearcoat});
        auto t2Clearcoat = invoke(ltc_2, "sample")({uvClearcoat});
        auto mInvClearcoat = module.get("mat3").call({}, {module.get("vec3").call({}, {t1Clearcoat.get("x"), JSValue(0.0), t1Clearcoat.get("y")}), module.get("vec3").call({}, {JSValue(0.0), JSValue(1.0), JSValue(0.0)}), module.get("vec3").call({}, {t1Clearcoat.get("z"), JSValue(0.0), t1Clearcoat.get("w")})});
        auto fresnelClearcoat = invoke(invoke(module.get("clearcoatF0"), "mul")({t2Clearcoat.get("x")}), "add")({invoke(invoke(module.get("clearcoatF90"), "sub")({module.get("clearcoatF0")}), "mul")({t2Clearcoat.get("y")})});
        invoke(self.get("clearcoatSpecularDirect"), "addAssign")({invoke(invoke(lightColor, "mul")({fresnelClearcoat}), "mul")({module.get("LTC_Evaluate").call({}, {([&]() { auto object6 = JSValue::object(); own(object6, "N", Ncc); own(object6, "V", V); own(object6, "P", P); own(object6, "mInv", mInvClearcoat); own(object6, "p0", p0); own(object6, "p1", p1); own(object6, "p2", p2); own(object6, "p3", p3); return object6; }())})})});
    }
    return JSValue{};
}

JSValue indirect(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto builder = arg(args, 0);
    invoke(self, "indirectDiffuse")({builder});
    invoke(self, "indirectSpecular")({builder});
    invoke(self, "ambientOcclusion")({builder});
    return JSValue{};
}

JSValue indirectDiffuse(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto builder = arg(args, 0);
    const auto destructured1 = builder.get("context");
    auto irradiance = destructured1.get("irradiance");
    auto reflectedLight = destructured1.get("reflectedLight");
    auto singleScattering = self.get("singleScatteringDielectric");
    auto multiScattering = self.get("multiScatteringDielectric");
    auto diffuseBRDF = (self.get("diffuseRoughness").toBoolean() ? invoke(self.get("eonDirectionalAlbedo"), "mul")({invoke(module.get("metalness"), "oneMinus")({}), JSValue(JSValue(1.0).toNumber() / nativeGlobalBinding("Math").get("PI").toNumber())}) : module.get("BRDF_Lambert").call({}, {([&]() { auto object2 = JSValue::object(); own(object2, "diffuseColor", module.get("diffuseContribution")); return object2; }())}));
    auto diffuse = invoke(invoke(invoke(irradiance, "mul")({diffuseBRDF}), "mul")({invoke(invoke(singleScattering, "add")({multiScattering}), "oneMinus")({})}), "toVar")({});
    if (JSValue(self.get("sheen").strictEquals(JSValue(true))).toBoolean()) {
        auto sheenAlbedo = module.get("IBLSheenBRDF").call({}, {([&]() { auto object3 = JSValue::object(); own(object3, "normal", module.get("normalView")); own(object3, "viewDir", module.get("positionViewDirection")); own(object3, "roughness", module.get("sheenRoughness")); return object3; }())});
        invoke(self.get("sheenSpecularIndirect"), "addAssign")({invoke(irradiance, "mul")({module.get("sheen"), sheenAlbedo, JSValue(JSValue(1.0).toNumber() / nativeGlobalBinding("Math").get("PI").toNumber())})});
        auto sheenEnergyComp = invoke(invoke(invoke(invoke(module.get("sheen").get("r"), "max")({module.get("sheen").get("g")}), "max")({module.get("sheen").get("b")}), "mul")({sheenAlbedo}), "oneMinus")({});
        invoke(diffuse, "mulAssign")({sheenEnergyComp});
    }
    invoke(reflectedLight.get("indirectDiffuse"), "addAssign")({diffuse});
    return JSValue{};
}

JSValue indirectSpecular(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto builder = arg(args, 0);
    const auto destructured1 = builder.get("context");
    auto radiance = destructured1.get("radiance");
    auto iblIrradiance = destructured1.get("iblIrradiance");
    auto reflectedLight = destructured1.get("reflectedLight");
    if (JSValue(self.get("sheen").strictEquals(JSValue(true))).toBoolean()) {
        invoke(self.get("sheenSpecularIndirect"), "addAssign")({invoke(iblIrradiance, "mul")({module.get("sheen"), module.get("IBLSheenBRDF").call({}, {([&]() { auto object2 = JSValue::object(); own(object2, "normal", module.get("normalView")); own(object2, "viewDir", module.get("positionViewDirection")); own(object2, "roughness", module.get("sheenRoughness")); return object2; }())}), JSValue(JSValue(1.0).toNumber() / nativeGlobalBinding("Math").get("PI").toNumber())})});
    }
    if (JSValue(self.get("clearcoat").strictEquals(JSValue(true))).toBoolean()) {
        auto dotNVcc = invoke(invoke(module.get("clearcoatNormalView"), "dot")({module.get("positionViewDirection")}), "clamp")({});
        auto clearcoatEnv = module.get("EnvironmentBRDF").call({}, {([&]() { auto object3 = JSValue::object(); own(object3, "dotNV", dotNVcc); own(object3, "specularColor", module.get("clearcoatF0")); own(object3, "specularF90", module.get("clearcoatF90")); own(object3, "roughness", module.get("clearcoatRoughness")); return object3; }())});
        invoke(self.get("clearcoatSpecularIndirect"), "addAssign")({invoke(self.get("clearcoatRadiance"), "mul")({clearcoatEnv})});
    }
    auto singleScatteringDielectric = self.get("singleScatteringDielectric");
    auto multiScatteringDielectric = self.get("multiScatteringDielectric");
    auto singleScatteringMetallic = invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("singleScatteringMetallic")});
    auto multiScatteringMetallic = invoke(module.get("vec3").call({}, {}), "toVar")({JSValue("multiScatteringMetallic")});
    invoke(self, "computeMultiscattering")({singleScatteringMetallic, multiScatteringMetallic, module.get("specularF90"), module.get("diffuseColor").get("rgb"), self.get("iridescenceF0Metallic")});
    auto singleScattering = module.get("mix").call({}, {singleScatteringDielectric, singleScatteringMetallic, module.get("metalness")});
    auto multiScattering = module.get("mix").call({}, {multiScatteringDielectric, multiScatteringMetallic, module.get("metalness")});
    auto totalScatteringDielectric = invoke(singleScatteringDielectric, "add")({multiScatteringDielectric});
    auto diffuseAlbedo = (self.get("diffuseRoughness").toBoolean() ? invoke(self.get("eonDirectionalAlbedo"), "mul")({invoke(module.get("metalness"), "oneMinus")({})}) : module.get("diffuseContribution"));
    auto diffuse = invoke(diffuseAlbedo, "mul")({invoke(totalScatteringDielectric, "oneMinus")({})});
    auto cosineWeightedIrradiance = invoke(iblIrradiance, "mul")({JSValue(JSValue(1.0).toNumber() / nativeGlobalBinding("Math").get("PI").toNumber())});
    auto indirectSpecular = invoke(invoke(invoke(radiance, "mul")({singleScattering}), "add")({invoke(multiScattering, "mul")({cosineWeightedIrradiance})}), "toVar")({});
    auto indirectDiffuse = invoke(invoke(diffuse, "mul")({cosineWeightedIrradiance}), "toVar")({});
    if (JSValue(self.get("sheen").strictEquals(JSValue(true))).toBoolean()) {
        auto sheenAlbedo = module.get("IBLSheenBRDF").call({}, {([&]() { auto object4 = JSValue::object(); own(object4, "normal", module.get("normalView")); own(object4, "viewDir", module.get("positionViewDirection")); own(object4, "roughness", module.get("sheenRoughness")); return object4; }())});
        auto sheenEnergyComp = invoke(invoke(invoke(invoke(module.get("sheen").get("r"), "max")({module.get("sheen").get("g")}), "max")({module.get("sheen").get("b")}), "mul")({sheenAlbedo}), "oneMinus")({});
        invoke(indirectSpecular, "mulAssign")({sheenEnergyComp});
        invoke(indirectDiffuse, "mulAssign")({sheenEnergyComp});
    }
    invoke(reflectedLight.get("indirectSpecular"), "addAssign")({indirectSpecular});
    invoke(reflectedLight.get("indirectDiffuse"), "addAssign")({indirectDiffuse});
    return JSValue{};
}

JSValue ambientOcclusion(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto builder = arg(args, 0);
    const auto destructured1 = builder.get("context");
    auto ambientOcclusion = destructured1.get("ambientOcclusion");
    auto reflectedLight = destructured1.get("reflectedLight");
    auto dotNV = invoke(invoke(module.get("normalView"), "dot")({module.get("positionViewDirection")}), "clamp")({});
    auto aoNV = invoke(dotNV, "add")({ambientOcclusion});
    auto aoExp = invoke(invoke(invoke(invoke(module.get("roughness"), "mul")({JSValue(-JSValue(16.0).toNumber())}), "oneMinus")({}), "negate")({}), "exp2")({});
    auto aoNode = invoke(invoke(ambientOcclusion, "sub")({invoke(invoke(aoNV, "pow")({aoExp}), "oneMinus")({})}), "clamp")({});
    if (JSValue(self.get("clearcoat").strictEquals(JSValue(true))).toBoolean()) {
        invoke(self.get("clearcoatSpecularIndirect"), "mulAssign")({ambientOcclusion});
    }
    if (JSValue(self.get("sheen").strictEquals(JSValue(true))).toBoolean()) {
        invoke(self.get("sheenSpecularIndirect"), "mulAssign")({ambientOcclusion});
    }
    invoke(reflectedLight.get("indirectDiffuse"), "mulAssign")({ambientOcclusion});
    invoke(reflectedLight.get("indirectSpecular"), "mulAssign")({aoNode});
    return JSValue{};
}

JSValue finish(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    const auto destructured1 = arg(args, 0);
    auto context = destructured1.get("context");
    const auto destructured2 = context;
    auto outgoingLight = destructured2.get("outgoingLight");
    if (JSValue(self.get("clearcoat").strictEquals(JSValue(true))).toBoolean()) {
        auto dotNVcc = invoke(invoke(module.get("clearcoatNormalView"), "dot")({module.get("positionViewDirection")}), "clamp")({});
        auto Fcc = module.get("F_Schlick").call({}, {([&]() { auto object3 = JSValue::object(); own(object3, "dotVH", dotNVcc); own(object3, "f0", module.get("clearcoatF0")); own(object3, "f90", module.get("clearcoatF90")); return object3; }())});
        auto clearcoatLight = invoke(invoke(outgoingLight, "mul")({invoke(invoke(module.get("clearcoat"), "mul")({Fcc}), "oneMinus")({})}), "add")({invoke(invoke(self.get("clearcoatSpecularDirect"), "add")({self.get("clearcoatSpecularIndirect")}), "mul")({module.get("clearcoat")})});
        invoke(outgoingLight, "assign")({clearcoatLight});
    }
    if (JSValue(self.get("sheen").strictEquals(JSValue(true))).toBoolean()) {
        auto sheenLight = invoke(outgoingLight, "add")({self.get("sheenSpecularDirect"), self.get("sheenSpecularIndirect")});
        invoke(outgoingLight, "assign")({sheenLight});
    }
    return JSValue{};
}

} // namespace

JSValue nativePhysicalLightingModelModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PhysicalLightingModel module");
    auto module = JSValue::object(nullptr);
    own(module, "BRDF_Lambert", nativeBRDFLambert());
    own(module, "BRDF_EON", nativeBRDFEON());
    own(module, "EON_DirectionalAlbedo", nativeBRDFEONBinding("EON_DirectionalAlbedo"));
    own(module, "BRDF_GGX", nativeBRDFGGX());
    own(module, "DFGLUT", nativeDFGLUT());
    own(module, "EnvironmentBRDF", nativeEnvironmentBRDF());
    own(module, "F_Schlick", nativeFSchlick());
    own(module, "Schlick_to_F0", nativeSchlickToF0());
    own(module, "BRDF_Sheen", nativeBRDFSheen());
    own(module, "LTC_Evaluate", nativeLTCBinding("LTC_Evaluate"));
    own(module, "LTC_Uv", nativeLTCBinding("LTC_Uv"));
    own(module, "LightingModel", nativeLightingModelConstructor());
    own(module, "diffuseColor", nativePropertyNodeBinding("diffuseColor"));
    own(module, "diffuseContribution", nativePropertyNodeBinding("diffuseContribution"));
    own(module, "diffuseRoughness", nativePropertyNodeBinding("diffuseRoughness"));
    own(module, "specularColor", nativePropertyNodeBinding("specularColor"));
    own(module, "specularColorBlended", nativePropertyNodeBinding("specularColorBlended"));
    own(module, "specularF90", nativePropertyNodeBinding("specularF90"));
    own(module, "roughness", nativePropertyNodeBinding("roughness"));
    own(module, "metalness", nativePropertyNodeBinding("metalness"));
    own(module, "clearcoat", nativePropertyNodeBinding("clearcoat"));
    own(module, "clearcoatRoughness", nativePropertyNodeBinding("clearcoatRoughness"));
    own(module, "sheen", nativePropertyNodeBinding("sheen"));
    own(module, "sheenRoughness", nativePropertyNodeBinding("sheenRoughness"));
    own(module, "iridescence", nativePropertyNodeBinding("iridescence"));
    own(module, "iridescenceIOR", nativePropertyNodeBinding("iridescenceIOR"));
    own(module, "iridescenceThickness", nativePropertyNodeBinding("iridescenceThickness"));
    own(module, "ior", nativePropertyNodeBinding("ior"));
    own(module, "thickness", nativePropertyNodeBinding("thickness"));
    own(module, "transmission", nativePropertyNodeBinding("transmission"));
    own(module, "attenuationDistance", nativePropertyNodeBinding("attenuationDistance"));
    own(module, "attenuationColor", nativePropertyNodeBinding("attenuationColor"));
    own(module, "dispersion", nativePropertyNodeBinding("dispersion"));
    own(module, "retroreflectivity", nativePropertyNodeBinding("retroreflectivity"));
    own(module, "normalView", nativeNormalBinding("normalView"));
    own(module, "clearcoatNormalView", nativeNormalBinding("clearcoatNormalView"));
    own(module, "normalWorld", nativeNormalBinding("normalWorld"));
    own(module, "positionViewDirection", nativePositionBinding("positionViewDirection"));
    own(module, "positionView", nativePositionBinding("positionView"));
    own(module, "positionWorld", nativePositionBinding("positionWorld"));
    own(module, "Fn", nativeTSLBaseBinding("Fn"));
    own(module, "float", nativeTSLBaseBinding("float"));
    own(module, "vec2", nativeTSLBaseBinding("vec2"));
    own(module, "vec3", nativeTSLBaseBinding("vec3"));
    own(module, "vec4", nativeTSLBaseBinding("vec4"));
    own(module, "mat3", nativeTSLBaseBinding("mat3"));
    own(module, "If", nativeTSLBaseBinding("If"));
    own(module, "mix", nativeMathNodeBinding("mix"));
    own(module, "normalize", nativeMathNodeBinding("normalize"));
    own(module, "refract", nativeMathNodeBinding("refract"));
    own(module, "length", nativeMathNodeBinding("length"));
    own(module, "clamp", nativeMathNodeBinding("clamp"));
    own(module, "log2", nativeMathNodeBinding("log2"));
    own(module, "log", nativeMathNodeBinding("log"));
    own(module, "exp", nativeMathNodeBinding("exp"));
    own(module, "smoothstep", nativeMathNodeBinding("smoothstep"));
    own(module, "div", nativeOperatorNodeBinding("div"));
    own(module, "cameraPosition", nativeCameraBinding("cameraPosition"));
    own(module, "cameraProjectionMatrix", nativeCameraBinding("cameraProjectionMatrix"));
    own(module, "cameraViewMatrix", nativeCameraBinding("cameraViewMatrix"));
    own(module, "cameraViewport", nativeCameraBinding("cameraViewport"));
    own(module, "modelWorldMatrix", nativeModelNodeBinding("modelWorldMatrix"));
    own(module, "screenSize", nativeScreenNodeBinding("screenSize"));
    own(module, "viewportMipTexture", nativeViewportTextureNodeBinding("viewportMipTexture"));
    own(module, "viewportOpaqueMipTexture", nativeViewportTextureNodeBinding("viewportOpaqueMipTexture"));
    own(module, "textureBicubicLevel", nativeTextureBicubicBinding("textureBicubicLevel"));
    own(module, "Loop", nativeLoopNodeBinding("Loop"));
    own(module, "BackSide", JSValue(BackSide));
    own(module, "getVolumeTransmissionRay", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto n = nextDestructured(iterator1);
        auto v = nextDestructured(iterator1);
        auto thickness = nextDestructured(iterator1);
        auto ior = nextDestructured(iterator1);
        auto modelMatrix = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        auto refractionVector = module.get("vec3").call({}, {module.get("refract").call({}, {invoke(v, "negate")({}), module.get("normalize").call({}, {n}), module.get("div").call({}, {JSValue(1.0), ior})})});
        auto modelScale = module.get("vec3").call({}, {module.get("length").call({}, {modelMatrix.get(JSValue(0.0)).get("xyz")}), module.get("length").call({}, {modelMatrix.get(JSValue(1.0)).get("xyz")}), module.get("length").call({}, {modelMatrix.get(JSValue(2.0)).get("xyz")})});
        return invoke(module.get("normalize").call({}, {refractionVector}), "mul")({invoke(thickness, "mul")({modelScale})});
        return JSValue{};
    }, {module}, "", 1), callback1Source), ([&]() { auto object1 = JSValue::object(); own(object1, "n", JSValue("vec3")); own(object1, "v", JSValue("vec3")); own(object1, "thickness", JSValue("float")); own(object1, "ior", JSValue("float")); own(object1, "modelMatrix", JSValue("mat4")); own(object1, "return", JSValue("vec3")); return object1; }())}));
    own(module, "applyIorToRoughness", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto roughness = nextDestructured(iterator1);
        auto ior = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        return invoke(roughness, "mul")({module.get("clamp").call({}, {invoke(invoke(ior, "mul")({JSValue(2.0)}), "sub")({JSValue(2.0)}), JSValue(0.0), JSValue(1.0)})});
        return JSValue{};
    }, {module}, "", 1), callback2Source), ([&]() { auto object1 = JSValue::object(); own(object1, "roughness", JSValue("float")); own(object1, "ior", JSValue("float")); own(object1, "return", JSValue("float")); return object1; }())}));
    own(module, "viewportBackSideTexture", module.get("viewportMipTexture").call({}, {}));
    own(module, "viewportFrontSideTexture", module.get("viewportOpaqueMipTexture").call({}, {}));
    own(module, "getTransmissionSample", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto fragCoord = nextDestructured(iterator1);
        auto roughness = nextDestructured(iterator1);
        auto ior = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        const auto destructured2 = arg(args, 1);
        auto material = destructured2.get("material");
        auto vTexture = (JSValue(material.get("side").strictEquals(module.get("BackSide"))).toBoolean() ? module.get("viewportBackSideTexture") : module.get("viewportFrontSideTexture"));
        auto transmissionSample = invoke(vTexture, "sample")({invoke(invoke(invoke(fragCoord, "mul")({module.get("cameraViewport").get("zw")}), "add")({module.get("cameraViewport").get("xy")}), "div")({module.get("screenSize")})});
        auto lod = invoke(module.get("log2").call({}, {module.get("cameraViewport").get("z")}), "mul")({module.get("applyIorToRoughness").call({}, {roughness, ior})});
        return module.get("textureBicubicLevel").call({}, {transmissionSample, lod});
        return JSValue{};
    }, {module}, "", 2), callback3Source)}));
    own(module, "volumeAttenuation", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto transmissionDistance = nextDestructured(iterator1);
        auto attenuationColor = nextDestructured(iterator1);
        auto attenuationDistance = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        module.get("If").call({}, {invoke(attenuationDistance, "notEqual")({JSValue(0.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto transmissionDistance = captures.at(1);
            auto attenuationColor = captures.at(2);
            auto attenuationDistance = captures.at(3);
            auto attenuationCoefficient = invoke(invoke(module.get("log").call({}, {attenuationColor}), "negate")({}), "div")({attenuationDistance});
            auto transmittance = module.get("exp").call({}, {invoke(invoke(attenuationCoefficient, "negate")({}), "mul")({transmissionDistance})});
            return transmittance;
            return JSValue{};
        }, {module, transmissionDistance, attenuationColor, attenuationDistance}, "", 0), callback5Source)});
        return module.get("vec3").call({}, {JSValue(1.0)});
        return JSValue{};
    }, {module}, "", 1), callback4Source), ([&]() { auto object1 = JSValue::object(); own(object1, "transmissionDistance", JSValue("float")); own(object1, "attenuationColor", JSValue("vec3")); own(object1, "attenuationDistance", JSValue("float")); own(object1, "return", JSValue("vec3")); return object1; }())}));
    own(module, "getIBLVolumeRefraction", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto iterator1 = getJSIterator(arg(args, 0));
        auto n = nextDestructured(iterator1);
        auto v = nextDestructured(iterator1);
        auto roughness = nextDestructured(iterator1);
        auto diffuseColor = nextDestructured(iterator1);
        auto specularColor = nextDestructured(iterator1);
        auto specularF90 = nextDestructured(iterator1);
        auto position = nextDestructured(iterator1);
        auto modelMatrix = nextDestructured(iterator1);
        auto viewMatrix = nextDestructured(iterator1);
        auto projMatrix = nextDestructured(iterator1);
        auto ior = nextDestructured(iterator1);
        auto thickness = nextDestructured(iterator1);
        auto attenuationColor = nextDestructured(iterator1);
        auto attenuationDistance = nextDestructured(iterator1);
        auto dispersion = nextDestructured(iterator1);
        if (!iterator1.done) closeJSIterator(iterator1);
        JSValue transmittedLight;
        JSValue transmittance;
        if (dispersion.toBoolean()) {
            transmittedLight = invoke(module.get("vec4").call({}, {}), "toVar")({});
            transmittance = invoke(module.get("vec3").call({}, {}), "toVar")({});
            auto halfSpread = invoke(invoke(ior, "sub")({JSValue(1.0)}), "mul")({invoke(dispersion, "mul")({JSValue(0.025)})});
            auto iors = module.get("vec3").call({}, {invoke(ior, "sub")({halfSpread}), ior, invoke(ior, "add")({halfSpread})});
            module.get("Loop").call({}, {([&]() { auto object2 = JSValue::object(); own(object2, "start", JSValue(0.0)); own(object2, "end", JSValue(3.0)); return object2; }()), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto n = captures.at(1);
                auto v = captures.at(2);
                auto roughness = captures.at(3);
                auto diffuseColor = captures.at(4);
                auto position = captures.at(5);
                auto modelMatrix = captures.at(6);
                auto viewMatrix = captures.at(7);
                auto projMatrix = captures.at(8);
                auto thickness = captures.at(9);
                auto attenuationColor = captures.at(10);
                auto attenuationDistance = captures.at(11);
                auto transmittedLight = captures.at(12);
                auto transmittance = captures.at(13);
                auto iors = captures.at(14);
                const auto destructured1 = arg(args, 0);
                auto i = destructured1.get("i");
                auto ior = invoke(iors, "element")({i});
                auto transmissionRay = module.get("getVolumeTransmissionRay").call({}, {n, v, thickness, ior, modelMatrix});
                auto refractedRayExit = invoke(position, "add")({transmissionRay});
                auto ndcPos = invoke(projMatrix, "mul")({invoke(viewMatrix, "mul")({module.get("vec4").call({}, {refractedRayExit, JSValue(1.0)})})});
                auto refractionCoords = invoke(module.get("vec2").call({}, {invoke(ndcPos.get("xy"), "div")({ndcPos.get("w")})}), "toVar")({});
                invoke(refractionCoords, "addAssign")({JSValue(1.0)});
                invoke(refractionCoords, "divAssign")({JSValue(2.0)});
                invoke(refractionCoords, "assign")({module.get("vec2").call({}, {refractionCoords.get("x"), invoke(refractionCoords.get("y"), "oneMinus")({})})});
                auto transmissionSample = module.get("getTransmissionSample").call({}, {refractionCoords, roughness, ior});
                invoke(invoke(transmittedLight, "element")({i}), "assign")({invoke(transmissionSample, "element")({i})});
                invoke(transmittedLight.get("a"), "addAssign")({transmissionSample.get("a")});
                invoke(invoke(transmittance, "element")({i}), "assign")({invoke(invoke(diffuseColor, "element")({i}), "mul")({invoke(module.get("volumeAttenuation").call({}, {module.get("length").call({}, {transmissionRay}), attenuationColor, attenuationDistance}), "element")({i})})});
                return JSValue{};
            }, {module, n, v, roughness, diffuseColor, position, modelMatrix, viewMatrix, projMatrix, thickness, attenuationColor, attenuationDistance, transmittedLight, transmittance, iors}, "", 1), callback7Source)});
            invoke(transmittedLight.get("a"), "divAssign")({JSValue(3.0)});
        } else {
            auto transmissionRay = module.get("getVolumeTransmissionRay").call({}, {n, v, thickness, ior, modelMatrix});
            auto refractedRayExit = invoke(position, "add")({transmissionRay});
            auto ndcPos = invoke(projMatrix, "mul")({invoke(viewMatrix, "mul")({module.get("vec4").call({}, {refractedRayExit, JSValue(1.0)})})});
            auto refractionCoords = invoke(module.get("vec2").call({}, {invoke(ndcPos.get("xy"), "div")({ndcPos.get("w")})}), "toVar")({});
            invoke(refractionCoords, "addAssign")({JSValue(1.0)});
            invoke(refractionCoords, "divAssign")({JSValue(2.0)});
            invoke(refractionCoords, "assign")({module.get("vec2").call({}, {refractionCoords.get("x"), invoke(refractionCoords.get("y"), "oneMinus")({})})});
            transmittedLight = module.get("getTransmissionSample").call({}, {refractionCoords, roughness, ior});
            transmittance = invoke(diffuseColor, "mul")({module.get("volumeAttenuation").call({}, {module.get("length").call({}, {transmissionRay}), attenuationColor, attenuationDistance})});
        }
        auto attenuatedColor = invoke(transmittance.get("rgb"), "mul")({transmittedLight.get("rgb")});
        auto dotNV = invoke(invoke(n, "dot")({v}), "clamp")({});
        auto F = module.get("vec3").call({}, {module.get("EnvironmentBRDF").call({}, {([&]() { auto object3 = JSValue::object(); own(object3, "dotNV", dotNV); own(object3, "specularColor", specularColor); own(object3, "specularF90", specularF90); own(object3, "roughness", roughness); return object3; }())})});
        auto transmittanceFactor = invoke(invoke(transmittance.get("r"), "add")({transmittance.get("g"), transmittance.get("b")}), "div")({JSValue(3.0)});
        return module.get("vec4").call({}, {invoke(invoke(F, "oneMinus")({}), "mul")({attenuatedColor}), invoke(invoke(invoke(transmittedLight.get("a"), "oneMinus")({}), "mul")({transmittanceFactor}), "oneMinus")({})});
        return JSValue{};
    }, {module}, "", 1), callback6Source)}));
    own(module, "XYZ_TO_REC709", module.get("mat3").call({}, {JSValue(3.2404542), JSValue(-JSValue(1.5371385).toNumber()), JSValue(-JSValue(0.4985314).toNumber()), JSValue(-JSValue(0.969266).toNumber()), JSValue(1.8760108), JSValue(0.041556), JSValue(0.0556434), JSValue(-JSValue(0.2040259).toNumber()), JSValue(1.0572252)}));
    own(module, "Fresnel0ToIor", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto fresnel0 = arg(args, 0);
        auto sqrtF0 = invoke(fresnel0, "sqrt")({});
        return invoke(invoke(module.get("vec3").call({}, {JSValue(1.0)}), "add")({sqrtF0}), "div")({invoke(module.get("vec3").call({}, {JSValue(1.0)}), "sub")({sqrtF0})});
        return JSValue{};
    }, {module}, "Fresnel0ToIor", 1), callback8Source));
    own(module, "IorToFresnel0", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto transmittedIor = arg(args, 0);
        auto incidentIor = arg(args, 1);
        return invoke(invoke(invoke(transmittedIor, "sub")({incidentIor}), "div")({invoke(transmittedIor, "add")({incidentIor})}), "pow2")({});
        return JSValue{};
    }, {module}, "IorToFresnel0", 2), callback9Source));
    own(module, "evalSensitivity", withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        auto OPD = arg(args, 0);
        auto shift = arg(args, 1);
        auto phase = invoke(OPD, "mul")({JSValue(JSValue(JSValue(2.0).toNumber() * nativeGlobalBinding("Math").get("PI").toNumber()).toNumber() * JSValue(1e-9).toNumber())});
        auto val = module.get("vec3").call({}, {JSValue(5.4856e-13), JSValue(4.4201e-13), JSValue(5.2481e-13)});
        auto pos = module.get("vec3").call({}, {JSValue(1681000.0), JSValue(1795300.0), JSValue(2208400.0)});
        auto VAR = module.get("vec3").call({}, {JSValue(4327800000.0), JSValue(9304600000.0), JSValue(6612100000.0)});
        auto x = invoke(invoke(module.get("float").call({}, {JSValue(JSValue(9.747e-14).toNumber() * invoke(nativeGlobalBinding("Math"), "sqrt")({JSValue(JSValue(JSValue(2.0).toNumber() * nativeGlobalBinding("Math").get("PI").toNumber()).toNumber() * JSValue(4528200000.0).toNumber())}).toNumber())}), "mul")({invoke(invoke(invoke(phase, "mul")({JSValue(2239900.0)}), "add")({shift.get("x")}), "cos")({})}), "mul")({invoke(invoke(invoke(phase, "pow2")({}), "mul")({JSValue(-JSValue(4528200000.0).toNumber())}), "exp")({})});
        auto xyz = invoke(invoke(invoke(val, "mul")({invoke(invoke(VAR, "mul")({JSValue(JSValue(2.0).toNumber() * nativeGlobalBinding("Math").get("PI").toNumber())}), "sqrt")({})}), "mul")({invoke(invoke(invoke(pos, "mul")({phase}), "add")({shift}), "cos")({})}), "mul")({invoke(invoke(invoke(invoke(phase, "pow2")({}), "negate")({}), "mul")({VAR}), "exp")({})});
        xyz = invoke(module.get("vec3").call({}, {invoke(xyz.get("x"), "add")({x}), xyz.get("y"), xyz.get("z")}), "div")({JSValue(1.0685e-7)});
        auto rgb = invoke(module.get("XYZ_TO_REC709"), "mul")({xyz});
        return rgb;
        return JSValue{};
    }, {module}, "evalSensitivity", 2), callback10Source));
    own(module, "evalIridescence", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        const auto destructured1 = arg(args, 0);
        auto outsideIOR = destructured1.get("outsideIOR");
        auto eta2 = destructured1.get("eta2");
        auto cosTheta1 = destructured1.get("cosTheta1");
        auto thinFilmThickness = destructured1.get("thinFilmThickness");
        auto baseF0 = destructured1.get("baseF0");
        auto iridescenceIOR = module.get("mix").call({}, {outsideIOR, eta2, module.get("smoothstep").call({}, {JSValue(0.0), JSValue(0.03), thinFilmThickness})});
        auto sinTheta2Sq = invoke(invoke(invoke(outsideIOR, "div")({iridescenceIOR}), "pow2")({}), "mul")({invoke(invoke(cosTheta1, "pow2")({}), "oneMinus")({})});
        auto cosTheta2Sq = invoke(sinTheta2Sq, "oneMinus")({});
        module.get("If").call({}, {invoke(cosTheta2Sq, "lessThan")({JSValue(0.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            return module.get("vec3").call({}, {JSValue(1.0)});
            return JSValue{};
        }, {module}, "", 0), callback12Source)});
        auto cosTheta2 = invoke(cosTheta2Sq, "sqrt")({});
        auto R0 = module.get("IorToFresnel0").call({}, {iridescenceIOR, outsideIOR});
        auto R12 = module.get("F_Schlick").call({}, {([&]() { auto object2 = JSValue::object(); own(object2, "f0", R0); own(object2, "f90", JSValue(1.0)); own(object2, "dotVH", cosTheta1); return object2; }())});
        auto T121 = invoke(R12, "oneMinus")({});
        auto phi12 = invoke(invoke(iridescenceIOR, "lessThan")({outsideIOR}), "select")({nativeGlobalBinding("Math").get("PI"), JSValue(0.0)});
        auto phi21 = invoke(module.get("float").call({}, {nativeGlobalBinding("Math").get("PI")}), "sub")({phi12});
        auto baseIOR = module.get("Fresnel0ToIor").call({}, {invoke(baseF0, "clamp")({JSValue(0.0), JSValue(0.9999)})});
        auto R1 = module.get("IorToFresnel0").call({}, {baseIOR, invoke(iridescenceIOR, "toVec3")({})});
        auto R23 = module.get("F_Schlick").call({}, {([&]() { auto object3 = JSValue::object(); own(object3, "f0", R1); own(object3, "f90", JSValue(1.0)); own(object3, "dotVH", cosTheta2); return object3; }())});
        auto phi23 = module.get("vec3").call({}, {invoke(invoke(baseIOR.get("x"), "lessThan")({iridescenceIOR}), "select")({nativeGlobalBinding("Math").get("PI"), JSValue(0.0)}), invoke(invoke(baseIOR.get("y"), "lessThan")({iridescenceIOR}), "select")({nativeGlobalBinding("Math").get("PI"), JSValue(0.0)}), invoke(invoke(baseIOR.get("z"), "lessThan")({iridescenceIOR}), "select")({nativeGlobalBinding("Math").get("PI"), JSValue(0.0)})});
        auto OPD = invoke(iridescenceIOR, "mul")({thinFilmThickness, cosTheta2, JSValue(2.0)});
        auto phi = invoke(module.get("vec3").call({}, {phi21}), "add")({phi23});
        auto R123 = invoke(invoke(R12, "mul")({R23}), "clamp")({JSValue(0.00001), JSValue(0.9999)});
        auto r123 = invoke(R123, "sqrt")({});
        auto Rs = invoke(invoke(invoke(T121, "pow2")({}), "mul")({R23}), "div")({invoke(module.get("vec3").call({}, {JSValue(1.0)}), "sub")({R123})});
        auto C0 = invoke(R12, "add")({Rs});
        auto I = invoke(C0, "toVar")({});
        auto Cm = invoke(invoke(Rs, "sub")({T121}), "toVar")({});
        module.get("Loop").call({}, {([&]() { auto object4 = JSValue::object(); own(object4, "start", JSValue(1.0)); own(object4, "end", JSValue(2.0)); own(object4, "condition", JSValue("<=")); own(object4, "name", JSValue("m")); return object4; }()), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto OPD = captures.at(1);
            auto phi = captures.at(2);
            auto r123 = captures.at(3);
            auto I = captures.at(4);
            auto Cm = captures.at(5);
            const auto destructured1 = arg(args, 0);
            auto m = destructured1.get("m");
            invoke(Cm, "mulAssign")({r123});
            auto Sm = invoke(module.get("evalSensitivity").call({}, {invoke(module.get("float").call({}, {m}), "mul")({OPD}), invoke(module.get("float").call({}, {m}), "mul")({phi})}), "mul")({JSValue(2.0)});
            invoke(I, "addAssign")({invoke(Cm, "mul")({Sm})});
            return JSValue{};
        }, {module, OPD, phi, r123, I, Cm}, "", 1), callback13Source)});
        return invoke(I, "max")({module.get("vec3").call({}, {JSValue(0.0)})});
        return JSValue{};
    }, {module}, "", 1), callback11Source), ([&]() { auto object1 = JSValue::object(); own(object1, "outsideIOR", JSValue("float")); own(object1, "eta2", JSValue("float")); own(object1, "cosTheta1", JSValue("float")); own(object1, "thinFilmThickness", JSValue("float")); own(object1, "baseF0", JSValue("vec3")); own(object1, "return", JSValue("vec3")); return object1; }())}));
    own(module, "IBLSheenBRDF", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        const auto destructured1 = arg(args, 0);
        auto normal = destructured1.get("normal");
        auto viewDir = destructured1.get("viewDir");
        auto roughness = destructured1.get("roughness");
        auto dotNV = invoke(invoke(normal, "dot")({viewDir}), "saturate")({});
        auto r2 = invoke(roughness, "mul")({roughness});
        auto rInv = invoke(invoke(roughness, "add")({JSValue(0.1)}), "reciprocal")({});
        auto a = invoke(invoke(invoke(module.get("float").call({}, {JSValue(-JSValue(1.9362).toNumber())}), "add")({invoke(roughness, "mul")({JSValue(1.0678)})}), "add")({invoke(r2, "mul")({JSValue(0.4573)})}), "sub")({invoke(rInv, "mul")({JSValue(0.8469)})});
        auto b = invoke(invoke(invoke(module.get("float").call({}, {JSValue(-JSValue(0.6014).toNumber())}), "add")({invoke(roughness, "mul")({JSValue(0.5538)})}), "sub")({invoke(r2, "mul")({JSValue(0.467)})}), "sub")({invoke(rInv, "mul")({JSValue(0.1255)})});
        auto DG = invoke(invoke(invoke(a, "mul")({dotNV}), "add")({b}), "exp")({});
        return invoke(DG, "saturate")({});
        return JSValue{};
    }, {module}, "", 1), callback14Source), ([&]() { auto object1 = JSValue::object(); own(object1, "normal", JSValue("vec3")); own(object1, "viewDir", JSValue("vec3")); own(object1, "roughness", JSValue("float")); own(object1, "return", JSValue("float")); return object1; }())}));
    own(module, "clearcoatF0", module.get("vec3").call({}, {JSValue(0.04)}));
    own(module, "clearcoatF90", module.get("float").call({}, {JSValue(1.0)}));
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(+[](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto self = captures.at(0).get("constructor").getPrototype().construct({}, newTarget);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = JSValue(false);
        auto clearcoat = argument1;
        auto argument2 = arg(args, 1);
        if (argument2.isUndefined()) argument2 = JSValue(false);
        auto sheen = argument2;
        auto argument3 = arg(args, 2);
        if (argument3.isUndefined()) argument3 = JSValue(false);
        auto iridescence = argument3;
        auto argument4 = arg(args, 3);
        if (argument4.isUndefined()) argument4 = JSValue(false);
        auto anisotropy = argument4;
        auto argument5 = arg(args, 4);
        if (argument5.isUndefined()) argument5 = JSValue(false);
        auto transmission = argument5;
        auto argument6 = arg(args, 5);
        if (argument6.isUndefined()) argument6 = JSValue(false);
        auto dispersion = argument6;
        auto argument7 = arg(args, 6);
        if (argument7.isUndefined()) argument7 = JSValue(false);
        auto retroreflection = argument7;
        auto argument8 = arg(args, 7);
        if (argument8.isUndefined()) argument8 = JSValue(false);
        auto diffuseRoughness = argument8;
        self.set("clearcoat", clearcoat);
        self.set("sheen", sheen);
        self.set("iridescence", iridescence);
        self.set("anisotropy", anisotropy);
        self.set("transmission", transmission);
        self.set("dispersion", dispersion);
        self.set("retroreflection", retroreflection);
        self.set("diffuseRoughness", diffuseRoughness);
        self.set("clearcoatRadiance", JSValue(nullptr));
        self.set("clearcoatSpecularDirect", JSValue(nullptr));
        self.set("clearcoatSpecularIndirect", JSValue(nullptr));
        self.set("sheenSpecularDirect", JSValue(nullptr));
        self.set("sheenSpecularIndirect", JSValue(nullptr));
        self.set("iridescenceFresnel", JSValue(nullptr));
        self.set("iridescenceF0Dielectric", JSValue(nullptr));
        self.set("iridescenceF0Metallic", JSValue(nullptr));
        self.set("dfg", JSValue(nullptr));
        self.set("eonDirectionalAlbedo", JSValue(nullptr));
        self.set("multiScatteringCompensation", JSValue(nullptr));
        self.set("singleScatteringDielectric", JSValue(nullptr));
        self.set("multiScatteringDielectric", JSValue(nullptr));
        return self;
    }, {holder}, "PhysicalLightingModel", 0);
    holder.set("constructor", ctor);
    const auto base = module.get("LightingModel");
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    ctor.setFunctionSourceText(classSource);
    method(prototype, "start", 1, start, {module}, startSource);
    method(prototype, "computeMultiscattering", 4, computeMultiscattering, {module}, computeMultiscatteringSource);
    method(prototype, "direct", 1, direct, {module}, directSource);
    method(prototype, "directRectArea", 1, directRectArea, {module}, directRectAreaSource);
    method(prototype, "indirect", 1, indirect, {module}, indirectSource);
    method(prototype, "indirectDiffuse", 1, indirectDiffuse, {module}, indirectDiffuseSource);
    method(prototype, "indirectSpecular", 1, indirectSpecular, {module}, indirectSpecularSource);
    method(prototype, "ambientOcclusion", 1, ambientOcclusion, {module}, ambientOcclusionSource);
    method(prototype, "finish", 1, finish, {module}, finishSource);
    own(module, "default", ctor);
    module.retainForHeapLifetime();
    lexical() = JSWeakValue(module);
    auto exports = JSValue::object();
    own(exports, "default", ctor);
    exports.retainForHeapLifetime();
    weak = JSWeakValue(exports);
    return exports;
}

JSValue nativePhysicalLightingModelBinding(const std::string& name) {
    nativePhysicalLightingModelModule();
    return name == "default" ? lexical().lock()->get(name) : JSValue{};
}
JSValue nativePhysicalLightingModelConstructor() { return nativePhysicalLightingModelBinding("default"); }
JSValue nativePhysicalLightingModelHelper(const std::string& name) {
    nativePhysicalLightingModelModule();
    return lexical().lock()->get(name);
}
