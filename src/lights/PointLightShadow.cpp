#include "lights/PointLightShadow.h"
#include "cameras/PerspectiveCamera.h"
PointLightShadow::PointLightShadow() : LightShadow(std::make_shared<PerspectiveCamera>(90, 1, 0.5, 500)) {}
std::shared_ptr<LightShadow> PointLightShadow::clone() const { auto result = std::make_shared<PointLightShadow>(); result->copy(*this); return result; }

// BEGIN GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "lights/PointLightShadow.h"
#include "geometries/GeometryRuntime.h"
#include "lights/LightShadow.h"
#include "cameras/PerspectiveCamera.h"
namespace PointLightShadowManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class PointLightShadow extends LightShadow {\r\n\r\n\t/**\r\n\t * Constructs a new point light shadow.\r\n\t */\r\n\tconstructor() {\r\n\r\n\t\tsuper( new PerspectiveCamera( 90, 1, 0.5, 500 ) );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isPointLightShadow = true;\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="() {\r\n\r\n\t\tsuper( new PerspectiveCamera( 90, 1, 0.5, 500 ) );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isPointLightShadow = true;\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"PointLightShadow");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);{auto g1=env;auto env=scope(g1);([&]() -> JSValue { const auto g2=home.getPrototype();Args g4;g4.push_back(([&]() -> JSValue { const auto g5=read(env,"PerspectiveCamera");Args g7;g7.push_back(JSValue(90.0));g7.push_back(JSValue(1.0));g7.push_back(JSValue(0.5));g7.push_back(JSValue(500.0)); return g5.construct(g7); }()));const auto instance=g2.construct(g4,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g8=read(env,"$this");const auto g9=JSValue("isPointLightShadow");const auto g10=JSValue(true); return setReference(g8,g9,g10); }());}return read(env,"$this");}
}
JSValue nativePointLightShadowModule(){using namespace PointLightShadowManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"PointLightShadow");auto env=scope();JSValue home;bindImport(env,"LightShadow",nativeLightShadowConstructor());bindImport(env,"PerspectiveCamera",nativePerspectiveCameraConstructor());declare(env,"PointLightShadow",false);auto constructor=JSValue::builtinConstructor(function0,{env},"PointLightShadow",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"LightShadow"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"PointLightShadow",constructor);auto facade=JSValue::object();own(facade,"PointLightShadow",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativePointLightShadowBinding(const std::string& name){return nativePointLightShadowModule().get(name);}
JSValue nativePointLightShadowConstructor(){return nativePointLightShadowBinding("PointLightShadow");}
// END GENERATED LIGHT RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
