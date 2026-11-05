#include "lights/DirectionalLightShadow.h"
#include "cameras/OrthographicCamera.h"
DirectionalLightShadow::DirectionalLightShadow() : LightShadow(std::make_shared<OrthographicCamera>(-5, 5, 5, -5, 0.5, 500)) {}
std::shared_ptr<LightShadow> DirectionalLightShadow::clone() const { auto result = std::make_shared<DirectionalLightShadow>(); result->copy(*this); return result; }

// BEGIN GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "lights/DirectionalLightShadow.h"
#include "geometries/GeometryRuntime.h"
#include "lights/LightShadow.h"
#include "cameras/OrthographicCamera.h"
namespace DirectionalLightShadowManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class DirectionalLightShadow extends LightShadow {\r\n\r\n\t/**\r\n\t * Constructs a new directional light shadow.\r\n\t */\r\n\tconstructor() {\r\n\r\n\t\tsuper( new OrthographicCamera( - 5, 5, 5, - 5, 0.5, 500 ) );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isDirectionalLightShadow = true;\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="() {\r\n\r\n\t\tsuper( new OrthographicCamera( - 5, 5, 5, - 5, 0.5, 500 ) );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isDirectionalLightShadow = true;\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"DirectionalLightShadow");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);{auto g1=env;auto env=scope(g1);([&]() -> JSValue { const auto g2=home.getPrototype();Args g4;g4.push_back(([&]() -> JSValue { const auto g5=read(env,"OrthographicCamera");Args g7;g7.push_back(JSValue(-JSValue(5.0).toNumber()));g7.push_back(JSValue(5.0));g7.push_back(JSValue(5.0));g7.push_back(JSValue(-JSValue(5.0).toNumber()));g7.push_back(JSValue(0.5));g7.push_back(JSValue(500.0)); return g5.construct(g7); }()));const auto instance=g2.construct(g4,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g10=read(env,"$this");const auto g11=JSValue("isDirectionalLightShadow");const auto g12=JSValue(true); return setReference(g10,g11,g12); }());}return read(env,"$this");}
}
JSValue nativeDirectionalLightShadowModule(){using namespace DirectionalLightShadowManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"DirectionalLightShadow");auto env=scope();JSValue home;bindImport(env,"LightShadow",nativeLightShadowConstructor());bindImport(env,"OrthographicCamera",nativeOrthographicCameraConstructor());declare(env,"DirectionalLightShadow",false);auto constructor=JSValue::builtinConstructor(function0,{env},"DirectionalLightShadow",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"LightShadow"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"DirectionalLightShadow",constructor);auto facade=JSValue::object();own(facade,"DirectionalLightShadow",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeDirectionalLightShadowBinding(const std::string& name){return nativeDirectionalLightShadowModule().get(name);}
JSValue nativeDirectionalLightShadowConstructor(){return nativeDirectionalLightShadowBinding("DirectionalLightShadow");}
// END GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
