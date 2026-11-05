#include "lights/DirectionalLight.h"
#include <numbers>
#include <stdexcept>
DirectionalLight::DirectionalLight(const Color& color, double intensity) : Light(color, intensity) { type = "DirectionalLight"; position.copy(Object3D::DEFAULT_UP); updateMatrix(); }
void DirectionalLight::registerNode(const JSValue& nodeClass) { registerNodeFor("DirectionalLight", nodeClass); }
JSValue DirectionalLight::getLightNode() const { auto node = registeredNodeFor("DirectionalLight"); return node ? *node : Light::getLightNode(); }
DirectionalLight& DirectionalLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const DirectionalLight*>(&source);
    if (!light) throw std::invalid_argument("DirectionalLight::copy requires a DirectionalLight");
    (void)recursive; Light::copy(source);
    target = light->target->clone();
    shadow = std::dynamic_pointer_cast<DirectionalLightShadow>(light->shadow->clone());
    return *this;
}
std::shared_ptr<Object3D> DirectionalLight::clone(bool recursive) const { auto result = std::make_shared<DirectionalLight>(); result->copy(*this, recursive); return result; }
JsonValue DirectionalLight::toJSON(Object3DJSONContext* meta) const {
    auto data = Light::toJSON(meta); auto& object = data["object"];
    object["shadow"] = shadow->toJSON(); object["target"] = target->uuid;
    return data;
}
std::optional<JsonValue> DirectionalLight::propertyValue(const std::string& property) const {
    if (property == "isDirectionalLight") return isDirectionalLight;

    return Light::propertyValue(property);
}
void DirectionalLight::dispose() { Object3D::dispose(); const auto retained = shadow; if (!retained) throw JSTypeError("Cannot read properties of null (reading 'dispose')"); retained->dispose(); }

// BEGIN GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "lights/DirectionalLight.h"
#include "geometries/GeometryRuntime.h"
#include "lights/Light.h"
#include "lights/DirectionalLightShadow.h"
#include "core/Object3D.h"
namespace DirectionalLightManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class DirectionalLight extends Light {\r\n\r\n\t/**\r\n\t * Constructs a new directional light.\r\n\t *\r\n\t * @param {(number|Color|string)} [color=0xffffff] - The light's color.\r\n\t * @param {number} [intensity=1] - The light's strength/intensity.\r\n\t */\r\n\tconstructor( color, intensity ) {\r\n\r\n\t\tsuper( color, intensity );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isDirectionalLight = true;\r\n\r\n\t\tthis.type = 'DirectionalLight';\r\n\r\n\t\tthis.position.copy( Object3D.DEFAULT_UP );\r\n\t\tthis.updateMatrix();\r\n\r\n\t\t/**\r\n\t\t * The directional light points from its position to the\r\n\t\t * target's position.\r\n\t\t *\r\n\t\t * For the target's position to be changed to anything other\r\n\t\t * than the default, it must be added to the scene.\r\n\t\t *\r\n\t\t * It is also possible to set the target to be another 3D object\r\n\t\t * in the scene. The light will now track the target object.\r\n\t\t *\r\n\t\t * @type {Object3D}\r\n\t\t */\r\n\t\tthis.target = new Object3D();\r\n\r\n\t\t/**\r\n\t\t * This property holds the light's shadow configuration.\r\n\t\t *\r\n\t\t * @type {DirectionalLightShadow}\r\n\t\t */\r\n\t\tthis.shadow = new DirectionalLightShadow();\r\n\r\n\t}\r\n\r\n\tdispose() {\r\n\r\n\t\tsuper.dispose();\r\n\r\n\t\tthis.shadow.dispose();\r\n\r\n\t}\r\n\r\n\tcopy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.target = source.target.clone();\r\n\t\tthis.shadow = source.shadow.clone();\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\ttoJSON( meta ) {\r\n\r\n\t\tconst data = super.toJSON( meta );\r\n\r\n\t\tdata.object.shadow = this.shadow.toJSON();\r\n\t\tdata.object.target = this.target.uuid;\r\n\r\n\t\treturn data;\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( color, intensity ) {\r\n\r\n\t\tsuper( color, intensity );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isDirectionalLight = true;\r\n\r\n\t\tthis.type = 'DirectionalLight';\r\n\r\n\t\tthis.position.copy( Object3D.DEFAULT_UP );\r\n\t\tthis.updateMatrix();\r\n\r\n\t\t/**\r\n\t\t * The directional light points from its position to the\r\n\t\t * target's position.\r\n\t\t *\r\n\t\t * For the target's position to be changed to anything other\r\n\t\t * than the default, it must be added to the scene.\r\n\t\t *\r\n\t\t * It is also possible to set the target to be another 3D object\r\n\t\t * in the scene. The light will now track the target object.\r\n\t\t *\r\n\t\t * @type {Object3D}\r\n\t\t */\r\n\t\tthis.target = new Object3D();\r\n\r\n\t\t/**\r\n\t\t * This property holds the light's shadow configuration.\r\n\t\t *\r\n\t\t * @type {DirectionalLightShadow}\r\n\t\t */\r\n\t\tthis.shadow = new DirectionalLightShadow();\r\n\r\n\t}";
inline constexpr char source2[]="() {\r\n\r\n\t\tsuper.dispose();\r\n\r\n\t\tthis.shadow.dispose();\r\n\r\n\t}";
inline constexpr char source3[]="dispose() {\r\n\r\n\t\tsuper.dispose();\r\n\r\n\t\tthis.shadow.dispose();\r\n\r\n\t}";
inline constexpr char source4[]="( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.target = source.target.clone();\r\n\t\tthis.shadow = source.shadow.clone();\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source5[]="copy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.target = source.target.clone();\r\n\t\tthis.shadow = source.shadow.clone();\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source6[]="( meta ) {\r\n\r\n\t\tconst data = super.toJSON( meta );\r\n\r\n\t\tdata.object.shadow = this.shadow.toJSON();\r\n\t\tdata.object.target = this.target.uuid;\r\n\r\n\t\treturn data;\r\n\r\n\t}";
inline constexpr char source7[]="toJSON( meta ) {\r\n\r\n\t\tconst data = super.toJSON( meta );\r\n\r\n\t\tdata.object.shadow = this.shadow.toJSON();\r\n\t\tdata.object.target = this.target.uuid;\r\n\r\n\t\treturn data;\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function2(const JSValue&,const Args&,const Args&);
JSValue function3(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"DirectionalLight");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"color",true);declare(env,"intensity",true);initialize(env,"color",arg(args,0));initialize(env,"intensity",arg(args,1));{auto g1=env;auto env=scope(g1);([&]() -> JSValue { const auto g2=home.getPrototype();Args g4;g4.push_back(read(env,"color"));g4.push_back(read(env,"intensity"));const auto instance=g2.construct(g4,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g5=read(env,"$this");const auto g6=JSValue("isDirectionalLight");const auto g7=JSValue(true); return setReference(g5,g6,g7); }());
([&]() -> JSValue { auto g8=read(env,"$this");const auto g9=JSValue("type");const auto g10=JSValue("DirectionalLight"); return setReference(g8,g9,g10); }());
([&]() -> JSValue { const auto g12=([&]() -> JSValue { const auto g14=read(env,"$this");const auto g15=JSValue("position"); return g14.get(g15); }());const auto g11=g12.get("copy");Args g13;g13.push_back(([&]() -> JSValue { const auto g16=read(env,"Object3D");const auto g17=JSValue("DEFAULT_UP"); return g16.get(g17); }())); return g11.call(g12,g13); }());
([&]() -> JSValue { const auto g19=read(env,"$this");const auto g18=g19.get("updateMatrix");Args g20; return g18.call(g19,g20); }());
([&]() -> JSValue { auto g21=read(env,"$this");const auto g22=JSValue("target");const auto g23=([&]() -> JSValue { const auto g24=read(env,"Object3D");Args g26; return g24.construct(g26); }()); return setReference(g21,g22,g23); }());
([&]() -> JSValue { auto g27=read(env,"$this");const auto g28=JSValue("shadow");const auto g29=([&]() -> JSValue { const auto g30=read(env,"DirectionalLightShadow");Args g32; return g30.construct(g32); }()); return setReference(g27,g28,g29); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);{auto g33=env;auto env=scope(g33);([&]() -> JSValue { const auto g35=read(env,"$this");const auto g34=home.getPrototype().get("dispose",g35);Args g36; return g34.call(g35,g36); }());
([&]() -> JSValue { const auto g38=([&]() -> JSValue { const auto g40=read(env,"$this");const auto g41=JSValue("shadow"); return g40.get(g41); }());const auto g37=g38.get("dispose");Args g39; return g37.call(g38,g39); }());}return {};}
JSValue function2(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"source",true);initialize(env,"source",arg(args,0));{auto g43=env;auto env=scope(g43);([&]() -> JSValue { const auto g45=read(env,"$this");const auto g44=home.getPrototype().get("copy",g45);Args g46;g46.push_back(read(env,"source")); return g44.call(g45,g46); }());
([&]() -> JSValue { auto g47=read(env,"$this");const auto g48=JSValue("target");const auto g49=([&]() -> JSValue { const auto g51=([&]() -> JSValue { const auto g53=read(env,"source");const auto g54=JSValue("target"); return g53.get(g54); }());const auto g50=g51.get("clone");Args g52; return g50.call(g51,g52); }()); return setReference(g47,g48,g49); }());
([&]() -> JSValue { auto g55=read(env,"$this");const auto g56=JSValue("shadow");const auto g57=([&]() -> JSValue { const auto g59=([&]() -> JSValue { const auto g61=read(env,"source");const auto g62=JSValue("shadow"); return g61.get(g62); }());const auto g58=g59.get("clone");Args g60; return g58.call(g59,g60); }()); return setReference(g55,g56,g57); }());
return read(env,"$this");}return {};}
JSValue function3(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"meta",true);initialize(env,"meta",arg(args,0));{auto g64=env;auto env=scope(g64);declare(env,"data",false);initialize(env,"data",([&]() -> JSValue { const auto g66=read(env,"$this");const auto g65=home.getPrototype().get("toJSON",g66);Args g67;g67.push_back(read(env,"meta")); return g65.call(g66,g67); }()));
([&]() -> JSValue { auto g68=([&]() -> JSValue { const auto g71=read(env,"data");const auto g72=JSValue("object"); return g71.get(g72); }());const auto g69=JSValue("shadow");const auto g70=([&]() -> JSValue { const auto g74=([&]() -> JSValue { const auto g76=read(env,"$this");const auto g77=JSValue("shadow"); return g76.get(g77); }());const auto g73=g74.get("toJSON");Args g75; return g73.call(g74,g75); }()); return setReference(g68,g69,g70); }());
([&]() -> JSValue { auto g78=([&]() -> JSValue { const auto g81=read(env,"data");const auto g82=JSValue("object"); return g81.get(g82); }());const auto g79=JSValue("target");const auto g80=([&]() -> JSValue { const auto g83=([&]() -> JSValue { const auto g85=read(env,"$this");const auto g86=JSValue("target"); return g85.get(g86); }());const auto g84=JSValue("uuid"); return g83.get(g84); }()); return setReference(g78,g79,g80); }());
return read(env,"data");}return {};}
}
JSValue nativeDirectionalLightModule(){using namespace DirectionalLightManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"DirectionalLight");auto env=scope();JSValue home;bindImport(env,"Light",nativeLightConstructor());bindImport(env,"DirectionalLightShadow",nativeDirectionalLightShadowConstructor());bindImport(env,"Object3D",nativeObject3DConstructor());declare(env,"DirectionalLight",false);auto constructor=JSValue::builtinConstructor(function0,{env},"DirectionalLight",2);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"Light"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"DirectionalLight",constructor);{auto home=prototype;auto function=([&]() -> JSValue { auto g42=JSValue::function(function1,{env,home},"dispose",0);g42.setFunctionSourceText(source2); return g42; }());function.setFunctionSourceText(source3);auto d=home.getOwnPropertyDescriptor("dispose").value_or(JSPropertyDescriptor{});d.value=function;d.writable=true;d.configurable=true;home.defineProperty("dispose",d);}{auto home=prototype;auto function=([&]() -> JSValue { auto g63=JSValue::function(function2,{env,home},"copy",1);g63.setFunctionSourceText(source4); return g63; }());function.setFunctionSourceText(source5);auto d=home.getOwnPropertyDescriptor("copy").value_or(JSPropertyDescriptor{});d.value=function;d.writable=true;d.configurable=true;home.defineProperty("copy",d);}{auto home=prototype;auto function=([&]() -> JSValue { auto g87=JSValue::function(function3,{env,home},"toJSON",1);g87.setFunctionSourceText(source6); return g87; }());function.setFunctionSourceText(source7);auto d=home.getOwnPropertyDescriptor("toJSON").value_or(JSPropertyDescriptor{});d.value=function;d.writable=true;d.configurable=true;home.defineProperty("toJSON",d);}auto facade=JSValue::object();own(facade,"DirectionalLight",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeDirectionalLightBinding(const std::string& name){return nativeDirectionalLightModule().get(name);}
JSValue nativeDirectionalLightConstructor(){return nativeDirectionalLightBinding("DirectionalLight");}
// END GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
