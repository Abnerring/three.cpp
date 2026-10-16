#include "core/NativeObject.h"
#include "core/JSValue.h"
#include "animation/AnimationUtils.h"
#include "renderers/webgl/WebGLPrograms.h"
#include "geometries/BoxGeometry.h"
#include "objects/Mesh.h"
#include "textures/Texture.h"
#include <iostream>
#include "materials/MeshDepthMaterial.h"
#include "materials/MeshDistanceMaterial.h"
#include "materials/MeshNormalMaterial.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/MeshLambertMaterial.h"
#include "materials/MeshPhongMaterial.h"
#include "materials/MeshToonMaterial.h"
#include "materials/MeshStandardMaterial.h"
#include "materials/MeshPhysicalMaterial.h"
#include "materials/MeshMatcapMaterial.h"
#include "materials/LineBasicMaterial.h"
#include "materials/LineDashedMaterial.h"
#include "materials/PointsMaterial.h"
#include "materials/ShadowMaterial.h"
#include "materials/SpriteMaterial.h"
#include "materials/ShaderMaterial.h"
#include "materials/RawShaderMaterial.h"

using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue();}
void data(JSValue o,const std::string& k,JSValue v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue dict(std::initializer_list<std::pair<const char*,JSValue>> entries){auto r=JSValue::object();for(const auto& [k,v]:entries)data(r,k,v);return r;}
std::shared_ptr<Material> material(const std::string& name){
 if(name=="MeshDepthMaterial")return std::make_shared<MeshDepthMaterial>();
 if(name=="MeshDistanceMaterial")return std::make_shared<MeshDistanceMaterial>();
 if(name=="MeshNormalMaterial")return std::make_shared<MeshNormalMaterial>();
 if(name=="MeshBasicMaterial")return std::make_shared<MeshBasicMaterial>();
 if(name=="MeshLambertMaterial")return std::make_shared<MeshLambertMaterial>();
 if(name=="MeshPhongMaterial")return std::make_shared<MeshPhongMaterial>();
 if(name=="MeshToonMaterial")return std::make_shared<MeshToonMaterial>();
 if(name=="MeshStandardMaterial")return std::make_shared<MeshStandardMaterial>();
 if(name=="MeshPhysicalMaterial")return std::make_shared<MeshPhysicalMaterial>();
 if(name=="MeshMatcapMaterial")return std::make_shared<MeshMatcapMaterial>();
 if(name=="LineBasicMaterial")return std::make_shared<LineBasicMaterial>();
 if(name=="LineDashedMaterial")return std::make_shared<LineDashedMaterial>();
 if(name=="PointsMaterial")return std::make_shared<PointsMaterial>();
 if(name=="ShadowMaterial")return std::make_shared<ShadowMaterial>();
 if(name=="SpriteMaterial")return std::make_shared<SpriteMaterial>();

 if(name=="ShaderMaterial")return std::make_shared<ShaderMaterial>();
 if(name=="RawShaderMaterial")return std::make_shared<RawShaderMaterial>();
 throw JSRangeError("material");
}
JSValue fixture(const JSValue& config){
 auto sources=JSValue::array();auto gl=dict({{"VERTEX_SHADER",35633},{"FRAGMENT_SHADER",35632}});
 for(const char* method:{"createProgram","createShader","shaderSource","compileShader","attachShader","bindAttribLocation","linkProgram","deleteProgram"})
  data(gl,method,JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
   if(c[0].strictEquals("createProgram"))return JSValue::object();
   if(c[0].strictEquals("createShader"))return dict({{"type",arg(a,0)}});
   if(c[0].strictEquals("shaderSource")){const auto s=c[1];data(s,std::to_string(s.length()),dict({{"type",arg(a,0).get("type")},{"source",arg(a,1)}}));}return {};
  },{method,sources},method));
 auto renderer=dict({{"outputColorSpace","srgb"},{"toneMapping",config.get("tone").toBoolean()?JSValue(4):JSValue(0)},{"shadowMap",dict({{"enabled",config.get("shadows").toBoolean()},{"type",1}})}});
 data(renderer,"getContext",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{gl}));
 data(renderer,"getRenderTarget",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return nullptr;}));
 data(renderer,"state",dict({{"buffers",dict({{"depth",dict({{"getReversed",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{config.get("reversed").toBoolean()})}})}})}}));
 auto environments=JSValue::object(),extensions=JSValue::object(),capabilities=dict({{"precision","highp"},{"logarithmicDepthBuffer",config.get("logDepth").toBoolean()}});
 data(environments,"get",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return arg(a,0);}));
 data(extensions,"has",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return false;}));
 data(capabilities,"getMaxPrecision",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return arg(a,0);}));
 auto binding=JSValue::object();data(binding,"releaseStatesOfProgram",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}));
 auto clipping=dict({{"numPlanes",config.get("clip").toBoolean()?JSValue(2):JSValue(0)},{"numIntersection",config.get("clip").toBoolean()?JSValue(1):JSValue(0)}});
 auto lights=JSValue::object();
 for(const char* key:{"sun","directional","point","spot","spotLightMap","rectArea","hemi","sunShadowMap","directionalShadowMap","pointShadowMap","spotShadowMap"})data(lights,key,JSValue::array(config.get("lights").toBoolean()?1:0));
 data(lights,"numSpotLightShadowsWithMaps",config.get("lights").toBoolean()?1:0);data(lights,"numLightProbes",0);
 const auto native=material(config.get("material").toString());native->name="source-fixture";
 auto wrapped=nativeMaterial(native);
 wrapped.set("customProgramCacheKey",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "source-fixture";}));
 const auto props=config.get("props");if(props.isObject())for(const auto& key:props.keys())wrapped.set(key,props.get(key));
 const auto maps=config.get("maps");if(maps.isObject())for(const auto& key:maps.keys()){
  const auto texture=std::make_shared<Texture>();texture->channel=maps.get(key).toNumber();wrapped.set(key,nativeTexture(texture));
 }
 const auto geometry=std::make_shared<BoxGeometry>();const auto mesh=std::make_shared<Mesh>(geometry,native);
 auto scene=dict({{"environment",nullptr},{"fog",config.get("fog").toBoolean()?dict({{"isFogExp2",true}}):JSValue(nullptr)}});
 WebGLPrograms programs(renderer,environments,extensions,capabilities,binding,clipping);
 const auto p=programs.getParameters(wrapped,lights,JSValue::array(config.get("shadows").toBoolean()?1:0),scene,nativeObject(mesh),JSValue::array());
 const auto key=programs.getProgramCacheKey(p);const auto program=programs.acquireProgram(p,key);
 const auto result=dict({{"parameters",p},{"key",key},{"sources",sources}});programs.releaseProgram(program);return result;
}
int main(){try{const auto realm=JSValue::object();std::string line;while(std::getline(std::cin,line)){if(line.empty())continue;const auto spec=Animation_detail::parseJSON(line);const auto result=fixture(spec);std::cout<<result.toJSON()->dump()<<'\n';realm.collectGarbage();}}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
