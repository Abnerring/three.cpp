// Direct port of three.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "renderers/webgl/WebGLLights.h"
#include "core/NativeObject.h"
#include "renderers/shaders/UniformsLib.h"
#include "math/Color.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "constants.h"
#include <cmath>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i) { return i<args.size()?args[i]:JSValue{}; }
void literal(JSValue object,const char* key,JSValue value) {
    JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields) {
    auto result=JSValue::object();for(const auto& [key,value]:fields)literal(result,key,value);return result;
}
JSValue ordinary(JSValue::NativeFunction fn,const Args& captures,const char* name,unsigned arity) {
    return JSValue::constructor(fn,captures,name,arity,JSValue::ConstructorKind::Callable);
}
JSValue vector3() { return nativeVector(std::make_shared<Vector3>()); }
JSValue vector2() { return nativeVector(std::make_shared<Vector2>()); }
JSValue color() { return nativeColor(std::make_shared<Color>()); }
std::string key(const JSValue& value) { return value.toPrimitive(JSValue::PreferredType::String).toString(); }
JSValue add(const JSValue& left,const JSValue& right) {
    const auto a=left.toPrimitive(),b=right.toPrimitive();
    if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String)return a.toString()+b.toString();
    const auto x=a.toNumber();const auto y=b.toNumber();return x+y;
}
JSValue call(const JSValue& self,const char* method,const Args& args={}) { return self.get(method).call(self,args); }
JSValue cacheGet(const JSValue&,const Args& args,const Args& captures) {
    auto lights=captures[0];const auto light=arg(args,0);
    if(!lights.get(key(light.get("id"))).isUndefined())return lights.get(key(light.get("id")));
    JSValue uniforms;const auto type=light.get("type");const bool shadows=captures[1].toBoolean();
    if(shadows) {
        if(type.strictEquals("SunLight")||type.strictEquals("DirectionalLight")||type.strictEquals("SpotLight")||type.strictEquals("PointLight")) {
            uniforms=object({{"shadowIntensity",1},{"shadowBias",0},{"shadowNormalBias",0},{"shadowRadius",1},{"shadowMapSize",vector2()}});
            if(type.strictEquals("PointLight")){literal(uniforms,"shadowCameraNear",1);literal(uniforms,"shadowCameraFar",1000);}
        }
    } else if(type.strictEquals("SunLight")||type.strictEquals("DirectionalLight")) uniforms=object({{"direction",vector3()},{"color",color()}});
    else if(type.strictEquals("SpotLight")) uniforms=object({{"position",vector3()},{"direction",vector3()},{"color",color()},{"distance",0},{"coneCos",0},{"penumbraCos",0},{"decay",0}});
    else if(type.strictEquals("PointLight")) uniforms=object({{"position",vector3()},{"color",color()},{"distance",0},{"decay",0}});
    else if(type.strictEquals("HemisphereLight")) uniforms=object({{"direction",vector3()},{"skyColor",color()},{"groundColor",color()}});
    else if(type.strictEquals("RectAreaLight")) uniforms=object({{"color",color()},{"position",vector3()},{"halfWidth",vector3()},{"halfHeight",vector3()}});
    lights.set(key(light.get("id")),uniforms);return uniforms;
}
JSValue cache(bool shadows) { auto result=JSValue::object();literal(result,"get",ordinary(cacheGet,{JSValue::object(),shadows},"get",1));return result; }
JSValue shadowFirst(const JSValue&,const Args& args,const Args&) {
    const auto a=arg(args,0),b=arg(args,1);
    const auto bs=b.get("castShadow").toBoolean()?2:0;
    const auto as=a.get("castShadow").toBoolean()?2:0;
    const auto bm=b.get("map").toBoolean()?1:0;
    const auto am=a.get("map").toBoolean()?1:0;
    return bs-as+bm-am;
}
JSValue module() {
    // Upstream keeps nextVersion in module scope, so it must outlive every collection.
    // A weak reference renewing itself is not enough: once the collector runs, the module
    // is rebuilt and the counter restarts, which is exactly what the lights parity test
    // caught. Hold it strongly instead (see HANDOFF item 52).
    static thread_local JSValue strong;
    if (strong.isObject()) return strong;
    auto result=JSValue::object(nullptr);literal(result,"nextVersion",0);
    literal(result,"sort",ordinary(shadowFirst,{},"shadowCastingAndTexturingLightsFirst",2));
    strong=result;return result;
}
void copyColor(JSValue uniforms,const char* field,const JSValue& source,const JSValue& intensity) {
    const auto target=uniforms.get(field),copy=target.get("copy");const auto copied=copy.call(target,{source});
    const auto multiply=copied.get("multiplyScalar");multiply.call(copied,{intensity});
}
// Directional/point branches deliberately re-read color/intensity after the
// cache lookup; accessor side effects must not use the earlier loop snapshots.
void liveColor(JSValue uniforms,const char* field,const JSValue& light) {
    const auto target=uniforms.get(field),copy=target.get("copy");const auto source=light.get("color");
    const auto copied=copy.call(target,{source}),multiply=copied.get("multiplyScalar");const auto intensity=light.get("intensity");multiply.call(copied,{intensity});
}
void shadowFields(JSValue uniforms,const JSValue& shadow) {
    uniforms.set("shadowIntensity",shadow.get("intensity"));uniforms.set("shadowBias",shadow.get("bias"));
    uniforms.set("shadowNormalBias",shadow.get("normalBias"));uniforms.set("shadowRadius",shadow.get("radius"));
}
void put(JSValue state,const char* field,const JSValue& index,const JSValue& value) { auto array=state.get(field);array.set(key(index),value); }
JSValue setup(const JSValue&,const Args& args,const Args& captures) {
    auto state=captures[0],global=captures[4];const auto uniformsCache=captures[1],shadowCache=captures[2],extensions=captures[3],lib=captures[5];
    const auto lights=arg(args,0);double r=0,g=0,b=0;
    for(int i=0;i<9;++i) { const auto target=state.get("probe").get(std::to_string(i));call(target,"set",{0,0,0}); }
    int sunLength=0,numSunShadows=0,directionalLength=0,pointLength=0,spotLength=0,rectAreaLength=0,hemiLength=0;
    int numDirectionalShadows=0,numPointShadows=0,numSpotShadows=0,numSpotMaps=0,numSpotShadowsWithMaps=0,numLightProbes=0;
    JSValue numSunShadowCascades=0;
    const auto sort=lights.get("sort");sort.call(lights,{global.get("sort")});
    const auto length=lights.get("length");
    for(double i=0;i<length.toNumber();++i) {
        const auto light=lights.get(JSValue(i).toString()),lightColor=light.get("color"),intensity=light.get("intensity"),distance=light.get("distance");
        JSValue shadowMap=nullptr;
        if(light.get("shadow").toBoolean()&&light.get("shadow").get("map").toBoolean()) {
            if(light.get("shadow").get("map").get("texture").get("format").strictEquals(JSValue(RGFormat)))shadowMap=light.get("shadow").get("map").get("texture");
            else {shadowMap=light.get("shadow").get("map").get("depthTexture");if(!shadowMap.toBoolean())shadowMap=light.get("shadow").get("map").get("texture");}
        }
        if(light.get("isAmbientLight").toBoolean()) {
            const auto red=lightColor.get("r").toNumber();const auto ir=intensity.toNumber();r+=red*ir;
            const auto green=lightColor.get("g").toNumber();const auto ig=intensity.toNumber();g+=green*ig;
            const auto blue=lightColor.get("b").toNumber();const auto ib=intensity.toNumber();b+=blue*ib;
        } else if(light.get("isLightProbe").toBoolean()) {
            for(int j=0;j<9;++j) {const auto target=state.get("probe").get(std::to_string(j)),fn=target.get("addScaledVector");const auto coefficient=light.get("sh").get("coefficients").get(std::to_string(j));fn.call(target,{coefficient,intensity});}
            ++numLightProbes;
        } else if(light.get("isSunLight").toBoolean()) {
            auto uniforms=call(uniformsCache,"get",{light});liveColor(uniforms,"color",light);
            if(light.get("castShadow").toBoolean()) {
                const auto shadow=light.get("shadow");auto shadowUniforms=call(shadowCache,"get",{light});shadowFields(shadowUniforms,shadow);
                const auto mapSize=shadowUniforms.get("shadowMapSize"),copy=mapSize.get("copy"),source=shadow.get("mapSize");
                const auto copied=copy.call(mapSize,{source}),multiply=copied.get("multiply"),extents=call(shadow,"getFrameExtents");multiply.call(copied,{extents});
                put(state,"sunShadow",numSunShadows,shadowUniforms);put(state,"sunShadowMap",numSunShadows,shadowMap);
                const auto cascadeCount=call(shadow,"getViewportCount");
                for(double j=0;j<cascadeCount.toNumber();++j) {
                    const auto matrices=state.get("sunShadowMatrix"),index=add(numSunShadowCascades,j),matrix=call(shadow,"getMatrix",{j});auto target=matrices;target.set(key(index),matrix);
                    const auto cascadeArray=state.get("sunShadowCascade"),cascadeIndex=add(numSunShadowCascades,j),cascade=shadow.get("_cascadeData").get(JSValue(j).toString());auto targetCascade=cascadeArray;targetCascade.set(key(cascadeIndex),cascade);
                }
                numSunShadowCascades=add(numSunShadowCascades,cascadeCount);++numSunShadows;
            }
            put(state,"sun",sunLength,uniforms);++sunLength;
        } else if(light.get("isDirectionalLight").toBoolean()) {
            auto uniforms=call(uniformsCache,"get",{light});liveColor(uniforms,"color",light);
            if(light.get("castShadow").toBoolean()) {
                const auto shadow=light.get("shadow");auto shadowUniforms=call(shadowCache,"get",{light});shadowFields(shadowUniforms,shadow);shadowUniforms.set("shadowMapSize",shadow.get("mapSize"));
                put(state,"directionalShadow",directionalLength,shadowUniforms);put(state,"directionalShadowMap",directionalLength,shadowMap);
                const auto array=state.get("directionalShadowMatrix"),matrix=light.get("shadow").get("matrix");auto target=array;target.set(std::to_string(directionalLength),matrix);++numDirectionalShadows;
            }
            put(state,"directional",directionalLength,uniforms);++directionalLength;
        } else if(light.get("isSpotLight").toBoolean()) {
            auto uniforms=call(uniformsCache,"get",{light});
            const auto position=uniforms.get("position"),set=position.get("setFromMatrixPosition"),matrix=light.get("matrixWorld");set.call(position,{matrix});
            copyColor(uniforms,"color",lightColor,intensity);uniforms.set("distance",distance);
            uniforms.set("coneCos",std::cos(light.get("angle").toNumber()));
            const auto angle=light.get("angle"),penumbra=light.get("penumbra");const auto complement=1-penumbra.toNumber();const auto angleValue=angle.toNumber();uniforms.set("penumbraCos",std::cos(angleValue*complement));
            uniforms.set("decay",light.get("decay"));put(state,"spot",spotLength,uniforms);
            const auto shadow=light.get("shadow");
            if(light.get("map").toBoolean()) {
                const auto mapArray=state.get("spotLightMap"),map=light.get("map");auto target=mapArray;target.set(std::to_string(numSpotMaps),map);++numSpotMaps;
                call(shadow,"updateMatrices",{light});if(light.get("castShadow").toBoolean())++numSpotShadowsWithMaps;
            }
            const auto matrixArray=state.get("spotLightMatrix"),shadowMatrix=shadow.get("matrix");auto target=matrixArray;target.set(std::to_string(spotLength),shadowMatrix);
            if(light.get("castShadow").toBoolean()) {
                auto shadowUniforms=call(shadowCache,"get",{light});shadowFields(shadowUniforms,shadow);shadowUniforms.set("shadowMapSize",shadow.get("mapSize"));
                put(state,"spotShadow",spotLength,shadowUniforms);put(state,"spotShadowMap",spotLength,shadowMap);++numSpotShadows;
            }
            ++spotLength;
        } else if(light.get("isRectAreaLight").toBoolean()) {
            auto uniforms=call(uniformsCache,"get",{light});copyColor(uniforms,"color",lightColor,intensity);
            const auto width=uniforms.get("halfWidth"),widthSet=width.get("set"),widthValue=light.get("width");widthSet.call(width,{widthValue.toNumber()*.5,0.,0.});
            const auto height=uniforms.get("halfHeight"),heightSet=height.get("set"),heightValue=light.get("height");heightSet.call(height,{0.,heightValue.toNumber()*.5,0.});
            put(state,"rectArea",rectAreaLength,uniforms);++rectAreaLength;
        } else if(light.get("isPointLight").toBoolean()) {
            auto uniforms=call(uniformsCache,"get",{light});liveColor(uniforms,"color",light);uniforms.set("distance",light.get("distance"));uniforms.set("decay",light.get("decay"));
            if(light.get("castShadow").toBoolean()) {
                const auto shadow=light.get("shadow");auto shadowUniforms=call(shadowCache,"get",{light});shadowFields(shadowUniforms,shadow);shadowUniforms.set("shadowMapSize",shadow.get("mapSize"));
                shadowUniforms.set("shadowCameraNear",shadow.get("camera").get("near"));shadowUniforms.set("shadowCameraFar",shadow.get("camera").get("far"));
                put(state,"pointShadow",pointLength,shadowUniforms);put(state,"pointShadowMap",pointLength,shadowMap);
                const auto array=state.get("pointShadowMatrix"),matrix=light.get("shadow").get("matrix");auto target=array;target.set(std::to_string(pointLength),matrix);++numPointShadows;
            }
            put(state,"point",pointLength,uniforms);++pointLength;
        } else if(light.get("isHemisphereLight").toBoolean()) {
            auto uniforms=call(uniformsCache,"get",{light});
            const auto sky=uniforms.get("skyColor"),skyCopy=sky.get("copy"),skyColor=light.get("color");const auto copiedSky=skyCopy.call(sky,{skyColor});call(copiedSky,"multiplyScalar",{intensity});
            const auto ground=uniforms.get("groundColor"),groundCopy=ground.get("copy"),groundColor=light.get("groundColor");const auto copiedGround=groundCopy.call(ground,{groundColor});call(copiedGround,"multiplyScalar",{intensity});
            put(state,"hemi",hemiLength,uniforms);++hemiLength;
        }
    }
    if(rectAreaLength>0) {
        const auto has=extensions.get("has"),available=has.call(extensions,{"OES_texture_float_linear"});
        state.set("rectAreaLTC1",lib.get(available.strictEquals(true)?"LTC_FLOAT_1":"LTC_HALF_1"));
        state.set("rectAreaLTC2",lib.get(available.strictEquals(true)?"LTC_FLOAT_2":"LTC_HALF_2"));
    }
    put(state,"ambient",0,r);put(state,"ambient",1,g);put(state,"ambient",2,b);
    auto hash=state.get("hash");
    const std::pair<const char*,int> counts[]={{"sunLength",sunLength},{"directionalLength",directionalLength},{"pointLength",pointLength},{"spotLength",spotLength},{"rectAreaLength",rectAreaLength},{"hemiLength",hemiLength},{"numSunShadows",numSunShadows},{"numDirectionalShadows",numDirectionalShadows},{"numPointShadows",numPointShadows},{"numSpotShadows",numSpotShadows},{"numSpotMaps",numSpotMaps},{"numLightProbes",numLightProbes}};
    bool changed=false;for(const auto& [field,count]:counts)if(!hash.get(field).strictEquals(JSValue(count))){changed=true;break;}
    if(changed) {
        for(const auto& [field,count]:std::initializer_list<std::pair<const char*,int>>{{"sun",sunLength},{"directional",directionalLength},{"spot",spotLength},{"rectArea",rectAreaLength},{"point",pointLength},{"hemi",hemiLength},{"sunShadow",numSunShadows},{"sunShadowMap",numSunShadows}}){auto array=state.get(field);array.set("length",count);}
        for(const auto* field:{"sunShadowMatrix","sunShadowCascade"}){auto array=state.get(field);array.set("length",numSunShadowCascades);}
        for(const auto& [field,count]:std::initializer_list<std::pair<const char*,int>>{{"directionalShadow",numDirectionalShadows},{"directionalShadowMap",numDirectionalShadows},{"directionalShadowMatrix",numDirectionalShadows},{"pointShadow",numPointShadows},{"pointShadowMap",numPointShadows},{"pointShadowMatrix",numPointShadows},{"spotShadow",numSpotShadows},{"spotShadowMap",numSpotShadows},{"spotLightMatrix",numSpotShadows+numSpotMaps-numSpotShadowsWithMaps},{"spotLightMap",numSpotMaps}}){auto array=state.get(field);array.set("length",count);}
        state.set("numSpotLightShadowsWithMaps",numSpotShadowsWithMaps);state.set("numLightProbes",numLightProbes);
        for(const auto& [field,count]:counts)hash.set(field,count);
        const auto next=global.get("nextVersion").toNumber();global.set("nextVersion",next+1);state.set("version",next);
    }
    return {};
}
JSValue setupView(const JSValue&,const Args& args,const Args& captures) {
    const auto state=captures[0],scratch=captures[1],matrix4=captures[2],matrix42=captures[3],lights=arg(args,0),camera=arg(args,1);
    int sun=0,directional=0,point=0,spot=0,rectArea=0,hemi=0;
    const auto viewMatrix=camera.get("matrixWorldInverse"),length=lights.get("length");
    const auto fromPosition=[](const JSValue& vector,const JSValue& light){const auto fn=vector.get("setFromMatrixPosition"),matrix=light.get("matrixWorld");fn.call(vector,{matrix});};
    const auto targetPosition=[&](const JSValue& light){const auto fn=scratch.get("setFromMatrixPosition"),matrix=light.get("target").get("matrixWorld");fn.call(scratch,{matrix});};
    for(double i=0;i<length.toNumber();++i) {
        const auto light=lights.get(JSValue(i).toString());
        if(light.get("isSunLight").toBoolean()) {
            const auto uniforms=state.get("sun").get(std::to_string(sun));fromPosition(uniforms.get("direction"),light);call(uniforms.get("direction"),"transformDirection",{viewMatrix});++sun;
        } else if(light.get("isDirectionalLight").toBoolean()) {
            const auto uniforms=state.get("directional").get(std::to_string(directional));fromPosition(uniforms.get("direction"),light);targetPosition(light);call(uniforms.get("direction"),"sub",{scratch});call(uniforms.get("direction"),"transformDirection",{viewMatrix});++directional;
        } else if(light.get("isSpotLight").toBoolean()) {
            const auto uniforms=state.get("spot").get(std::to_string(spot));fromPosition(uniforms.get("position"),light);call(uniforms.get("position"),"applyMatrix4",{viewMatrix});
            fromPosition(uniforms.get("direction"),light);targetPosition(light);call(uniforms.get("direction"),"sub",{scratch});call(uniforms.get("direction"),"transformDirection",{viewMatrix});++spot;
        } else if(light.get("isRectAreaLight").toBoolean()) {
            const auto uniforms=state.get("rectArea").get(std::to_string(rectArea));fromPosition(uniforms.get("position"),light);call(uniforms.get("position"),"applyMatrix4",{viewMatrix});
            call(matrix42,"identity");const auto copy=matrix4.get("copy"),source=light.get("matrixWorld");copy.call(matrix4,{source});call(matrix4,"premultiply",{viewMatrix});call(matrix42,"extractRotation",{matrix4});
            const auto width=uniforms.get("halfWidth"),widthSet=width.get("set"),widthValue=light.get("width");widthSet.call(width,{widthValue.toNumber()*.5,0.,0.});
            const auto height=uniforms.get("halfHeight"),heightSet=height.get("set"),heightValue=light.get("height");heightSet.call(height,{0.,heightValue.toNumber()*.5,0.});
            call(uniforms.get("halfWidth"),"applyMatrix4",{matrix42});call(uniforms.get("halfHeight"),"applyMatrix4",{matrix42});++rectArea;
        } else if(light.get("isPointLight").toBoolean()) {
            const auto uniforms=state.get("point").get(std::to_string(point));fromPosition(uniforms.get("position"),light);call(uniforms.get("position"),"applyMatrix4",{viewMatrix});++point;
        } else if(light.get("isHemisphereLight").toBoolean()) {
            const auto uniforms=state.get("hemi").get(std::to_string(hemi));fromPosition(uniforms.get("direction"),light);call(uniforms.get("direction"),"transformDirection",{viewMatrix});++hemi;
        }
    }
    return {};
}
}
WebGLLights::WebGLLights(JSValue extensions):object_(JSValue::object()) {
    auto state=::object({{"version",0}}),hash=JSValue::object();
    for(const auto* name:{"sunLength","directionalLength","pointLength","spotLength","rectAreaLength","hemiLength","numSunShadows","numDirectionalShadows","numPointShadows","numSpotShadows","numSpotMaps","numLightProbes"})literal(hash,name,-1);
    literal(state,"hash",hash);auto ambient=JSValue::array();for(int i=0;i<3;++i)literal(ambient,std::to_string(i).c_str(),0);literal(state,"ambient",ambient);
    for(const auto* name:{"probe","sun","sunShadow","sunShadowMap","sunShadowMatrix","sunShadowCascade","directional","directionalShadow","directionalShadowMap","directionalShadowMatrix","spot","spotLightMap","spotShadow","spotShadowMap","spotLightMatrix","rectArea"})literal(state,name,JSValue::array());
    literal(state,"rectAreaLTC1",nullptr);literal(state,"rectAreaLTC2",nullptr);
    for(const auto* name:{"point","pointShadow","pointShadowMap","pointShadowMatrix","hemi"})literal(state,name,JSValue::array());
    literal(state,"numSpotLightShadowsWithMaps",0);literal(state,"numLightProbes",0);
    for(int i=0;i<9;++i)call(state.get("probe"),"push",{vector3()});
    const auto scratch=vector3(),matrix4=nativeMatrix(std::make_shared<Matrix4>()),matrix42=nativeMatrix(std::make_shared<Matrix4>());
    literal(object_,"setup",ordinary(::setup,{state,cache(false),cache(true),extensions,module(),UniformsLib},"setup",1));
    literal(object_,"setupView",ordinary(::setupView,{state,scratch,matrix4,matrix42},"setupView",2));literal(object_,"state",state);
}
JSValue WebGLLights::setup(JSValue lights) {return object_.get("setup").call(object_,{lights});}
JSValue WebGLLights::setupView(JSValue lights,JSValue camera) {return object_.get("setupView").call(object_,{lights,camera});}
