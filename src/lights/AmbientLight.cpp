#include "lights/AmbientLight.h"
#include <numbers>
#include <stdexcept>
AmbientLight::AmbientLight(const Color& color, double intensity) : Light(color, intensity) { type = "AmbientLight";  }
void AmbientLight::registerNode(const JSValue& nodeClass) { registerNodeFor("AmbientLight", nodeClass); }
JSValue AmbientLight::getLightNode() const { auto node = registeredNodeFor("AmbientLight"); return node ? *node : Light::getLightNode(); }
AmbientLight& AmbientLight::copy(const Object3D& source, bool recursive) {
    Light::copy(source, recursive);

    return *this;
}
std::shared_ptr<Object3D> AmbientLight::clone(bool recursive) const { auto result = std::make_shared<AmbientLight>(); result->copy(*this, recursive); return result; }
JsonValue AmbientLight::toJSON(Object3DJSONContext* meta) const {
    return Light::toJSON(meta);
}
std::optional<JsonValue> AmbientLight::propertyValue(const std::string& property) const {
    if (property == "isAmbientLight") return isAmbientLight;

    return Light::propertyValue(property);
}

// BEGIN GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "lights/AmbientLight.h"
#include "geometries/GeometryRuntime.h"
#include "lights/Light.h"
namespace AmbientLightManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class AmbientLight extends Light {\r\n\r\n\t/**\r\n\t * Constructs a new ambient light.\r\n\t *\r\n\t * @param {(number|Color|string)} [color=0xffffff] - The light's color.\r\n\t * @param {number} [intensity=1] - The light's strength/intensity.\r\n\t */\r\n\tconstructor( color, intensity ) {\r\n\r\n\t\tsuper( color, intensity );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isAmbientLight = true;\r\n\r\n\t\tthis.type = 'AmbientLight';\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( color, intensity ) {\r\n\r\n\t\tsuper( color, intensity );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isAmbientLight = true;\r\n\r\n\t\tthis.type = 'AmbientLight';\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"AmbientLight");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"color",true);declare(env,"intensity",true);initialize(env,"color",arg(args,0));initialize(env,"intensity",arg(args,1));{auto g1=env;auto env=scope(g1);([&]() -> JSValue { const auto g2=home.getPrototype();Args g4;g4.push_back(read(env,"color"));g4.push_back(read(env,"intensity"));const auto instance=g2.construct(g4,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g5=read(env,"$this");const auto g6=JSValue("isAmbientLight");const auto g7=JSValue(true); return setReference(g5,g6,g7); }());
([&]() -> JSValue { auto g8=read(env,"$this");const auto g9=JSValue("type");const auto g10=JSValue("AmbientLight"); return setReference(g8,g9,g10); }());}return read(env,"$this");}
}
JSValue nativeAmbientLightModule(){using namespace AmbientLightManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"AmbientLight");auto env=scope();JSValue home;bindImport(env,"Light",nativeLightConstructor());declare(env,"AmbientLight",false);auto constructor=JSValue::builtinConstructor(function0,{env},"AmbientLight",2);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"Light"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"AmbientLight",constructor);auto facade=JSValue::object();own(facade,"AmbientLight",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeAmbientLightBinding(const std::string& name){return nativeAmbientLightModule().get(name);}
JSValue nativeAmbientLightConstructor(){return nativeAmbientLightBinding("AmbientLight");}
// END GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
