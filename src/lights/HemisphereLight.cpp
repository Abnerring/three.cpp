#include "lights/HemisphereLight.h"
#include <numbers>
#include <stdexcept>
HemisphereLight::HemisphereLight(const Color& skyColor, const Color& groundColor, double intensity) : Light(skyColor, intensity), groundColor(groundColor) { type = "HemisphereLight"; position.copy(Object3D::DEFAULT_UP); updateMatrix(); }
void HemisphereLight::registerNode(const JSValue& nodeClass) { registerNodeFor("HemisphereLight", nodeClass); }
JSValue HemisphereLight::getLightNode() const { auto node = registeredNodeFor("HemisphereLight"); return node ? *node : Light::getLightNode(); }
HemisphereLight& HemisphereLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const HemisphereLight*>(&source);
    if (!light) throw std::invalid_argument("HemisphereLight::copy requires a HemisphereLight");
    Light::copy(source, recursive);
    groundColor.copy(light->groundColor);
    return *this;
}
std::shared_ptr<Object3D> HemisphereLight::clone(bool recursive) const { auto result = std::make_shared<HemisphereLight>(); result->copy(*this, recursive); return result; }
JsonValue HemisphereLight::toJSON(Object3DJSONContext* meta) const {
    auto data = Light::toJSON(meta); auto& object = data["object"];
    object["groundColor"] = groundColor.getHex();
    return data;
}
std::optional<JsonValue> HemisphereLight::propertyValue(const std::string& property) const {
    if (property == "isHemisphereLight") return isHemisphereLight;

    return Light::propertyValue(property);
}

// BEGIN GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "lights/HemisphereLight.h"
#include "geometries/GeometryRuntime.h"
#include "lights/Light.h"
#include "math/Color.h"
#include "core/Object3D.h"
namespace HemisphereLightManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class HemisphereLight extends Light {\r\n\r\n\t/**\r\n\t * Constructs a new hemisphere light.\r\n\t *\r\n\t * @param {(number|Color|string)} [skyColor=0xffffff] - The light's sky color.\r\n\t * @param {(number|Color|string)} [groundColor=0xffffff] - The light's ground color.\r\n\t * @param {number} [intensity=1] - The light's strength/intensity.\r\n\t */\r\n\tconstructor( skyColor, groundColor, intensity ) {\r\n\r\n\t\tsuper( skyColor, intensity );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isHemisphereLight = true;\r\n\r\n\t\tthis.type = 'HemisphereLight';\r\n\r\n\t\tthis.position.copy( Object3D.DEFAULT_UP );\r\n\t\tthis.updateMatrix();\r\n\r\n\t\t/**\r\n\t\t * The light's ground color.\r\n\t\t *\r\n\t\t * @type {Color}\r\n\t\t */\r\n\t\tthis.groundColor = new Color( groundColor );\r\n\r\n\t}\r\n\r\n\tcopy( source, recursive ) {\r\n\r\n\t\tsuper.copy( source, recursive );\r\n\r\n\t\tthis.groundColor.copy( source.groundColor );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\ttoJSON( meta ) {\r\n\r\n\t\tconst data = super.toJSON( meta );\r\n\r\n\t\tdata.object.groundColor = this.groundColor.getHex();\r\n\r\n\t\treturn data;\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( skyColor, groundColor, intensity ) {\r\n\r\n\t\tsuper( skyColor, intensity );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isHemisphereLight = true;\r\n\r\n\t\tthis.type = 'HemisphereLight';\r\n\r\n\t\tthis.position.copy( Object3D.DEFAULT_UP );\r\n\t\tthis.updateMatrix();\r\n\r\n\t\t/**\r\n\t\t * The light's ground color.\r\n\t\t *\r\n\t\t * @type {Color}\r\n\t\t */\r\n\t\tthis.groundColor = new Color( groundColor );\r\n\r\n\t}";
inline constexpr char source2[]="( source, recursive ) {\r\n\r\n\t\tsuper.copy( source, recursive );\r\n\r\n\t\tthis.groundColor.copy( source.groundColor );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source3[]="copy( source, recursive ) {\r\n\r\n\t\tsuper.copy( source, recursive );\r\n\r\n\t\tthis.groundColor.copy( source.groundColor );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source4[]="( meta ) {\r\n\r\n\t\tconst data = super.toJSON( meta );\r\n\r\n\t\tdata.object.groundColor = this.groundColor.getHex();\r\n\r\n\t\treturn data;\r\n\r\n\t}";
inline constexpr char source5[]="toJSON( meta ) {\r\n\r\n\t\tconst data = super.toJSON( meta );\r\n\r\n\t\tdata.object.groundColor = this.groundColor.getHex();\r\n\r\n\t\treturn data;\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function2(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"HemisphereLight");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"skyColor",true);declare(env,"groundColor",true);declare(env,"intensity",true);initialize(env,"skyColor",arg(args,0));initialize(env,"groundColor",arg(args,1));initialize(env,"intensity",arg(args,2));{auto g1=env;auto env=scope(g1);([&]() -> JSValue { const auto g2=home.getPrototype();Args g4;g4.push_back(read(env,"skyColor"));g4.push_back(read(env,"intensity"));const auto instance=g2.construct(g4,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g5=read(env,"$this");const auto g6=JSValue("isHemisphereLight");const auto g7=JSValue(true); return setReference(g5,g6,g7); }());
([&]() -> JSValue { auto g8=read(env,"$this");const auto g9=JSValue("type");const auto g10=JSValue("HemisphereLight"); return setReference(g8,g9,g10); }());
([&]() -> JSValue { const auto g12=([&]() -> JSValue { const auto g14=read(env,"$this");const auto g15=JSValue("position"); return g14.get(g15); }());const auto g11=g12.get("copy");Args g13;g13.push_back(([&]() -> JSValue { const auto g16=read(env,"Object3D");const auto g17=JSValue("DEFAULT_UP"); return g16.get(g17); }())); return g11.call(g12,g13); }());
([&]() -> JSValue { const auto g19=read(env,"$this");const auto g18=g19.get("updateMatrix");Args g20; return g18.call(g19,g20); }());
([&]() -> JSValue { auto g21=read(env,"$this");const auto g22=JSValue("groundColor");const auto g23=([&]() -> JSValue { const auto g24=read(env,"Color");Args g26;g26.push_back(read(env,"groundColor")); return g24.construct(g26); }()); return setReference(g21,g22,g23); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"source",true);declare(env,"recursive",true);initialize(env,"source",arg(args,0));initialize(env,"recursive",arg(args,1));{auto g27=env;auto env=scope(g27);([&]() -> JSValue { const auto g29=read(env,"$this");const auto g28=home.getPrototype().get("copy",g29);Args g30;g30.push_back(read(env,"source"));g30.push_back(read(env,"recursive")); return g28.call(g29,g30); }());
([&]() -> JSValue { const auto g32=([&]() -> JSValue { const auto g34=read(env,"$this");const auto g35=JSValue("groundColor"); return g34.get(g35); }());const auto g31=g32.get("copy");Args g33;g33.push_back(([&]() -> JSValue { const auto g36=read(env,"source");const auto g37=JSValue("groundColor"); return g36.get(g37); }())); return g31.call(g32,g33); }());
return read(env,"$this");}return {};}
JSValue function2(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"meta",true);initialize(env,"meta",arg(args,0));{auto g39=env;auto env=scope(g39);declare(env,"data",false);initialize(env,"data",([&]() -> JSValue { const auto g41=read(env,"$this");const auto g40=home.getPrototype().get("toJSON",g41);Args g42;g42.push_back(read(env,"meta")); return g40.call(g41,g42); }()));
([&]() -> JSValue { auto g43=([&]() -> JSValue { const auto g46=read(env,"data");const auto g47=JSValue("object"); return g46.get(g47); }());const auto g44=JSValue("groundColor");const auto g45=([&]() -> JSValue { const auto g49=([&]() -> JSValue { const auto g51=read(env,"$this");const auto g52=JSValue("groundColor"); return g51.get(g52); }());const auto g48=g49.get("getHex");Args g50; return g48.call(g49,g50); }()); return setReference(g43,g44,g45); }());
return read(env,"data");}return {};}
}
JSValue nativeHemisphereLightModule(){using namespace HemisphereLightManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"HemisphereLight");auto env=scope();JSValue home;bindImport(env,"Light",nativeLightConstructor());bindImport(env,"Color",nativeColorConstructor());bindImport(env,"Object3D",nativeObject3DConstructor());declare(env,"HemisphereLight",false);auto constructor=JSValue::builtinConstructor(function0,{env},"HemisphereLight",3);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"Light"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"HemisphereLight",constructor);{auto home=prototype;auto function=([&]() -> JSValue { auto g38=JSValue::function(function1,{env,home},"copy",2);g38.setFunctionSourceText(source2); return g38; }());function.setFunctionSourceText(source3);auto d=home.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});d.value=function;d.writable=true;d.configurable=true;home.defineProperty("copy",d);}{auto home=prototype;auto function=([&]() -> JSValue { auto g53=JSValue::function(function2,{env,home},"toJSON",1);g53.setFunctionSourceText(source4); return g53; }());function.setFunctionSourceText(source5);auto d=home.getOwnPropertyDescriptor("toJSON").value_or(JSPropertyDescriptor{});d.value=function;d.writable=true;d.configurable=true;home.defineProperty("toJSON",d);}auto facade=JSValue::object();own(facade,"HemisphereLight",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeHemisphereLightBinding(const std::string& name){return nativeHemisphereLightModule().get(name);}
JSValue nativeHemisphereLightConstructor(){return nativeHemisphereLightBinding("HemisphereLight");}
// END GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
