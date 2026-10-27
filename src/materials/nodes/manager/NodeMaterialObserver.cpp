// Ported from the pinned three.js NodeMaterialObserver.js; see THREEJS-LICENSE.
#include "materials/nodes/manager/NodeMaterialObserver.h"
#include "constants.h"
#include "core/JSWeakMap.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include <set>

namespace {
using namespace UtilityNodeRuntime;

// The captured module record owns the four ephemeron caches and the shared array.
enum Slot { RefreshType, DynamicUsage, Uniforms, LightsCache, MaterialCache, GeometryCache, TextureCache };
JSValue slot(const Args& captures, Slot index) { return captures[1].get(std::to_string(index)); }
std::vector<std::string> forIn(const JSValue& source) {
    std::vector<std::string> keys; std::set<std::string> seen; auto object=source;
    while(!object.isNull()&&!object.isUndefined()) {
        if(!object.isObject())object=JSValue::intrinsicObjectConstructor().call({}, {object});
        for(const auto& key:object.ownPropertyNames()) {
            if(seen.contains(key))continue;
            const auto descriptor=object.getOwnPropertyDescriptor(key); if(!descriptor)continue;
            seen.insert(key); if(descriptor->enumerable.value_or(false))keys.push_back(key);
        }
        object=object.getPrototype();
    }
    return keys;
}
template<class F> void enumerate(const JSValue& object,F callback) {
    const auto keys=forIn(object);
    for(const auto& key:keys)if(object.has(key))callback(key);
}
JSValue record(std::initializer_list<std::pair<const char*,JSValue>> properties) {
    auto result=JSValue::object();for(const auto& [key,value]:properties)own(result,key,value);return result;
}
void increment(JSValue object,const char* key) { const auto value=object.get(key).toNumber();object.set(key,value+1); }
JSValue onDispose(const JSValue&,const Args&,const Args& captures) { increment(captures[0],"_version");return {}; }
JSValue textureSnapshot(const JSValue& self,const JSValue& texture,bool initial) {
    const auto id=texture.get("id");const auto version=initial?JSValue(0):texture.get("version");
    const auto cacheVersion=invoke(self,"getTextureData")({texture}).get("_version");
    return record({{"id",id},{"version",version},{"cacheVersion",cacheVersion}});
}
JSValue nullableVersion(const JSValue& object,const char* name) {
    return !object.get(name).isNull()?object.get(name).get("version"):JSValue(nullptr);
}
JSValue attributeId(const JSValue& attribute) {
    return attribute.get("isInterleavedBufferAttribute").toBoolean()?attribute.get("data").get("uuid"):attribute.get("id");
}
JSValue attributeVersion(const JSValue& attribute) {
    return attribute.get("isInterleavedBufferAttribute").toBoolean()?attribute.get("data").get("version"):attribute.get("version");
}

JSValue equals(const JSValue& self,const Args& args) {
    const auto renderObject=arg(args,0),lightsData=arg(args,1),renderId=arg(args,2);
    const auto object=renderObject.get("object"),material=renderObject.get("material"),geometry=renderObject.get("geometry");
    auto data=invoke(self,"getRenderObjectData")({renderObject});
    if(!invoke(data.get("worldMatrix"),"equals")({object.get("matrixWorld")}).strictEquals(true)) {
        invoke(data.get("worldMatrix"),"copy")({object.get("matrixWorld")});return false;
    }
    auto materialData=invoke(self,"getMaterialData")({renderObject.get("material")});
    if(!materialData.get("_renderId").strictEquals(renderId)) {
        materialData.set("_renderId",renderId);bool changed=false;
        enumerate(materialData,[&](const std::string& property) {
            auto value=materialData.get(property);const auto mtlValue=material.get(property);
            if(property=="_renderId"||property=="_version")return;
            if(value.isNull()||mtlValue.isNull()||mtlValue.isUndefined()) {
                if(!value.strictEquals(mtlValue.isUndefined()?JSValue(nullptr):mtlValue)) {
                    if(mtlValue.isNull()||mtlValue.isUndefined())materialData.set(property,nullptr);
                    else if(mtlValue.get("isTexture").strictEquals(true))materialData.set(property,textureSnapshot(self,mtlValue,false));
                    else if(mtlValue.isObject()&&!mtlValue.isCallable()&&!mtlValue.get("clone").isUndefined())materialData.set(property,invoke(mtlValue,"clone")());
                    else materialData.set(property,mtlValue);
                    changed=true;
                }
            } else if(!value.get("equals").isUndefined()) {
                if(invoke(value,"equals")({mtlValue}).strictEquals(false)) { invoke(value,"copy")({mtlValue});changed=true; }
            } else if(mtlValue.get("isTexture").strictEquals(true)) {
                const auto textureData=invoke(self,"getTextureData")({mtlValue});
                if(!value.get("id").strictEquals(mtlValue.get("id"))||!value.get("version").strictEquals(mtlValue.get("version"))||!value.get("cacheVersion").strictEquals(textureData.get("_version"))) {
                    value.set("id",mtlValue.get("id"));value.set("version",mtlValue.get("version"));value.set("cacheVersion",textureData.get("_version"));changed=true;
                }
            } else if(!value.strictEquals(mtlValue)) { materialData.set(property,mtlValue);changed=true; }
        });
        if(changed)increment(materialData,"_version");
    }
    if(!data.get("materialVersion").strictEquals(materialData.get("_version"))) { data.set("materialVersion",materialData.get("_version"));return false; }
    const auto context=renderObject.get("context"),width=context.get("width"),height=context.get("height");
    if(!data.get("bufferWidth").strictEquals(width)||!data.get("bufferHeight").strictEquals(height)) {data.set("bufferWidth",width);data.set("bufferHeight",height);return false;}
    if(!data.get("geometryId").strictEquals(geometry.get("id"))) {
        data.set("geometryId",geometry.get("id"));data.set("geometryVersion",invoke(self,"getGeometryData")({geometry}).get("_version"));return false;
    }
    auto geometryData=invoke(self,"getGeometryData")({renderObject.get("geometry")});
    if(!geometryData.get("_renderId").strictEquals(renderId)) {
        geometryData.set("_renderId",renderId);bool changed=false;
        const auto attributes=geometry.get("attributes");auto storedAttributes=geometryData.get("attributes");
        std::size_t currentCount=0,storedCount=0;
        enumerate(attributes,[&](const auto&){++currentCount;});
        enumerate(storedAttributes,[&](const std::string& name) {
            ++storedCount;auto stored=storedAttributes.get(name);const auto attribute=attributes.get(name);
            if(attribute.isUndefined()) {if(!storedAttributes.erase(name))throw JSTypeError("Cannot delete property '"+name+"'");changed=true;return;}
            const auto id=attributeId(attribute),version=attributeVersion(attribute);
            if(!stored.get("id").strictEquals(id)||!stored.get("version").strictEquals(version)) {stored.set("id",id);stored.set("version",version);changed=true;}
        });
        if(storedCount!=currentCount) {geometryData.set("attributes",invoke(self,"getAttributesData")({attributes}));changed=true;}
        const auto index=geometry.get("index"),indexId=index.toBoolean()?index.get("id"):JSValue(nullptr),indexVersion=index.toBoolean()?index.get("version"):JSValue(nullptr);
        if(!geometryData.get("indexId").strictEquals(indexId)||!geometryData.get("indexVersion").strictEquals(indexVersion)) {geometryData.set("indexId",indexId);geometryData.set("indexVersion",indexVersion);changed=true;}
        if(!geometryData.get("drawRange").get("start").strictEquals(geometry.get("drawRange").get("start"))||!geometryData.get("drawRange").get("count").strictEquals(geometry.get("drawRange").get("count"))) {
            geometryData.get("drawRange").set("start",geometry.get("drawRange").get("start"));geometryData.get("drawRange").set("count",geometry.get("drawRange").get("count"));changed=true;
        }
        if(changed)increment(geometryData,"_version");
    }
    if(!data.get("geometryVersion").strictEquals(geometryData.get("_version"))) {data.set("geometryVersion",geometryData.get("_version"));return false;}
    if(data.get("morphTargetInfluences").toBoolean()) {
        bool changed=false;
        for(JSValue i=0;less(i,data.get("morphTargetInfluences").get("length"));i=i.toNumber()+1) {
            if(!data.get("morphTargetInfluences").get(i).strictEquals(object.get("morphTargetInfluences").get(i))) {
                data.get("morphTargetInfluences").set(i,object.get("morphTargetInfluences").get(i));changed=true;
            }
        }
        if(changed)return false;
    }
    if(object.get("isInstancedMesh").strictEquals(true)) {
        const auto colorVersion=nullableVersion(object,"instanceColor"),morphVersion=nullableVersion(object,"morphTexture");
        if(!data.get("instanceMatrixVersion").strictEquals(object.get("instanceMatrix").get("version"))||!data.get("instanceColorVersion").strictEquals(colorVersion)||!data.get("morphTextureVersion").strictEquals(morphVersion)) {
            data.set("instanceMatrixVersion",object.get("instanceMatrix").get("version"));data.set("instanceColorVersion",colorVersion);data.set("morphTextureVersion",morphVersion);return false;
        }
    }
    if(object.get("isBatchedMesh").strictEquals(true)) {
        const auto colorVersion=nullableVersion(object,"_colorsTexture");
        if(!data.get("matricesTextureVersion").strictEquals(object.get("_matricesTexture").get("version"))||!data.get("colorsTextureVersion").strictEquals(colorVersion)||!data.get("indirectTextureVersion").strictEquals(object.get("_indirectTexture").get("version"))) {
            data.set("matricesTextureVersion",object.get("_matricesTexture").get("version"));data.set("colorsTextureVersion",colorVersion);data.set("indirectTextureVersion",object.get("_indirectTexture").get("version"));return false;
        }
    }
    if(data.get("lights").toBoolean()) {
        for(JSValue i=0;less(i,lightsData.get("length"));i=i.toNumber()+1) {
            auto light=data.get("lights").get(i);const auto current=lightsData.get(i);
            if(!light.get("map").strictEquals(current.get("map"))||!light.get("cacheVersion").strictEquals(current.get("cacheVersion"))||!light.get("shadowMapWidth").strictEquals(current.get("shadowMapWidth"))||!light.get("shadowMapHeight").strictEquals(current.get("shadowMapHeight"))) {
                light.set("map",current.get("map"));light.set("cacheVersion",current.get("cacheVersion"));light.set("shadowMapWidth",current.get("shadowMapWidth"));light.set("shadowMapHeight",current.get("shadowMapHeight"));return false;
            }
        }
    }
    const auto scene=renderObject.get("scene");
    if(!scene.get("environment").isNull()&&material.get("envMap").isNull()) {
        if(!data.get("environmentIntensity").strictEquals(scene.get("environmentIntensity"))||invoke(data.get("environmentRotation"),"equals")({scene.get("environmentRotation")}).strictEquals(false)) {
            data.set("environmentIntensity",scene.get("environmentIntensity"));invoke(data.get("environmentRotation"),"copy")({scene.get("environmentRotation")});return false;
        }
    }
    if(data.get("center").toBoolean()) {
        if(invoke(data.get("center"),"equals")({object.get("center")}).strictEquals(false)) {invoke(data.get("center"),"copy")({object.get("center")});return false;}
    }
    if(!renderObject.get("bundle").isNull())data.set("version",renderObject.get("bundle").get("version"));
    return true;
}

JSValue method(const JSValue& receiver,const Args& args,const Args& captures) {
    auto self=receiver;const auto name=captures[0].string();const auto input=arg(args,0);
    if(name=="firstInitialization") {
        const auto initialized=invoke(self.get("renderObjects"),"has")({input});
        if(initialized.strictEquals(false)){invoke(self,"getRenderObjectData")({input});return true;}return false;
    }
    if(name=="hasDynamicInstancing") {
        return input.get("isInstancedMesh").strictEquals(true)&&(input.get("instanceMatrix").get("usage").strictEquals(slot(captures,DynamicUsage))||(!input.get("instanceColor").isNull()&&input.get("instanceColor").get("usage").strictEquals(slot(captures,DynamicUsage))));
    }
    if(name=="needsVelocity") {const auto mrt=invoke(input,"getMRT")();return mrt.isNull()?JSValue(false):invoke(mrt,"has")({"velocity"});}
    if(name=="getRenderObjectData") {
        auto data=invoke(self.get("renderObjects"),"get")({input});
        if(data.isUndefined()) {
            const auto geometry=input.get("geometry"),object=input.get("object");
            const auto id=geometry.get("id"),geometryVersion=invoke(self,"getGeometryData")({geometry}).get("_version"),materialVersion=invoke(self,"getMaterialData")({input.get("material")}).get("_version"),matrix=invoke(object.get("matrixWorld"),"clone")();
            data=record({{"geometryId",id},{"geometryVersion",geometryVersion},{"materialVersion",materialVersion},{"worldMatrix",matrix}});
            if(object.get("center").toBoolean())data.set("center",invoke(object.get("center"),"clone")());
            if(object.get("morphTargetInfluences").toBoolean())data.set("morphTargetInfluences",invoke(object.get("morphTargetInfluences"),"slice")());
            if(object.get("isInstancedMesh").strictEquals(true)) {
                data.set("instanceMatrixVersion",object.get("instanceMatrix").get("version"));data.set("instanceColorVersion",nullableVersion(object,"instanceColor"));data.set("morphTextureVersion",nullableVersion(object,"morphTexture"));
            }
            if(object.get("isBatchedMesh").strictEquals(true)) {
                data.set("matricesTextureVersion",object.get("_matricesTexture").get("version"));data.set("colorsTextureVersion",nullableVersion(object,"_colorsTexture"));data.set("indirectTextureVersion",object.get("_indirectTexture").get("version"));
            }
            if(!input.get("bundle").isNull())data.set("version",input.get("bundle").get("version"));
            const auto context=input.get("context"),width=context.get("width"),height=context.get("height");data.set("bufferWidth",width);data.set("bufferHeight",height);
            const auto scene=input.get("scene"),intensity=scene.get("environmentIntensity"),rotation=scene.get("environmentRotation");
            data.set("environmentIntensity",intensity);data.set("environmentRotation",invoke(rotation,"clone")());
            data.set("lights",invoke(self,"getLightsData")({invoke(input.get("lightsNode"),"getBuiltinLights")(),JSValue::array()}));
            invoke(self.get("renderObjects"),"set")({input,data});
        }
        return data;
    }
    if(name=="getAttributesData") {
        auto data=JSValue::object();enumerate(input,[&](const std::string& key){const auto attribute=input.get(key);const auto id=attributeId(attribute),version=attributeVersion(attribute);data.set(key,record({{"id",id},{"version",version}}));});return data;
    }
    if(name=="containsNode") {
        const auto material=input.get("material");
        for(const auto& key:forIn(material))if(material.has(key)&&material.get(key).toBoolean()&&material.get(key).get("isNode").toBoolean())return true;
        return input.get("context").get("modelViewMatrix").toBoolean()||input.get("context").get("modelNormalViewMatrix").toBoolean()||input.get("context").get("getAO").toBoolean()||input.get("context").get("getShadow").toBoolean();
    }
    if(name=="getGeometryData") {
        const auto cache=slot(captures,GeometryCache);auto data=invoke(cache,"get")({input});
        if(data.isUndefined()) {
            const auto attributes=invoke(self,"getAttributesData")({input.get("attributes")});
            const auto indexId=input.get("index").toBoolean()?input.get("index").get("id"):JSValue(nullptr);
            const auto indexVersion=input.get("index").toBoolean()?input.get("index").get("version"):JSValue(nullptr);
            const auto start=input.get("drawRange").get("start"),count=input.get("drawRange").get("count");
            data=record({{"_renderId",-1},{"_version",0},{"attributes",attributes},{"indexId",indexId},{"indexVersion",indexVersion},{"drawRange",record({{"start",start},{"count",count}})}});
            invoke(input,"addEventListener")({"dispose",JSValue::function(onDispose,{data},"",0)});invoke(cache,"set")({input,data});
        }
        return data;
    }
    if(name=="getTextureData") {
        const auto cache=slot(captures,TextureCache);auto data=invoke(cache,"get")({input});
        if(data.isUndefined()) {
            data=record({{"_version",0}});const auto callback=JSValue::function(onDispose,{data},"onDispose",0);
            if(!input.get("renderTarget").isNull())invoke(input.get("renderTarget"),"addEventListener")({"dispose",callback});else invoke(input,"addEventListener")({"dispose",callback});
            invoke(cache,"set")({input,data});
        }
        return data;
    }
    if(name=="getMaterialData") {
        const auto cache=slot(captures,MaterialCache);auto data=invoke(cache,"get")({input});
        if(data.isUndefined()) {
            data=record({{"_renderId",-1},{"_version",0}});
            each(self.get("refreshUniforms"),[&](const JSValue& property) {
                const auto value=input.get(property);if(value.isUndefined())return;
                if(value.isNull())data.set(property,nullptr);
                else if(value.isObject()&&!value.isCallable()&&!value.get("clone").isUndefined()) {
                    if(value.get("isTexture").strictEquals(true))data.set(property,textureSnapshot(self,value,true));else data.set(property,invoke(value,"clone")());
                }else data.set(property,value);
            });
            invoke(cache,"set")({input,data});
        }
        return data;
    }
    if(name=="equals")return equals(self,args);
    if(name=="getLightsData") {
        auto lights=arg(args,1);lights.set("length",0);
        each(input,[&](const JSValue& light) {
            JSValue data=nullptr;
            if(light.get("isSpotLight").strictEquals(true)&&!light.get("map").isNull()) {
                const auto map=light.get("map").get("version"),cacheVersion=invoke(self,"getTextureData")({light.get("map")}).get("_version");data=record({{"map",map},{"cacheVersion",cacheVersion}});
            }
            if(light.get("castShadow").strictEquals(true)&&!light.get("shadow").isUndefined()) {
                if(data.isNull())data=JSValue::object();data.set("shadowMapWidth",light.get("shadow").get("mapSize").get("width"));data.set("shadowMapHeight",light.get("shadow").get("mapSize").get("height"));
            }
            if(!data.isNull())invoke(lights,"push")({data});
        });return lights;
    }
    if(name=="getLights") {
        const auto renderId=arg(args,1),cache=slot(captures,LightsCache);auto cached=invoke(cache,"get")({input});
        if(cached.isUndefined()){cached=record({{"renderId",-1},{"lightsData",JSValue::array()}});invoke(cache,"set")({input,cached});}
        if(cached.get("renderId").strictEquals(renderId))return cached.get("lightsData");
        cached.set("renderId",renderId);invoke(self,"getLightsData")({invoke(input,"getBuiltinLights")(),cached.get("lightsData")});return cached.get("lightsData");
    }
    if(name=="needsRefresh") {
        const auto frame=arg(args,1);
        if(self.get("hasNode").toBoolean()||self.get("hasAnimation").toBoolean()||invoke(self,"hasDynamicInstancing")({input.get("object")}).toBoolean()||invoke(self,"firstInitialization")({input}).toBoolean()||invoke(self,"needsVelocity")({frame.get("renderer")}).toBoolean())return slot(captures,RefreshType).get("FULL");
        const auto renderId=frame.get("renderId");auto refreshType=slot(captures,RefreshType).get("NONE");
        if(!self.get("renderId").strictEquals(renderId)){self.set("renderId",renderId);refreshType=slot(captures,RefreshType).get("SHARED");}
        const bool isStatic=input.get("object").get("static").strictEquals(true);
        const bool isBundle=!input.get("bundle").isNull()&&input.get("bundle").get("static").strictEquals(true)&&invoke(self,"getRenderObjectData")({input}).get("version").strictEquals(input.get("bundle").get("version"));
        if(isStatic||isBundle)return refreshType;
        const auto lights=invoke(self,"getLights")({input.get("lightsNode"),renderId});if(invoke(self,"equals")({input,lights,renderId}).strictEquals(false))refreshType=slot(captures,RefreshType).get("FULL");return refreshType;
    }
    return {};
}
}

JSValue nativeNodeMaterialObserverConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto refreshType=nativeConstantsBinding("RenderObjectRefreshType"),dynamicUsage=nativeConstantsBinding("DynamicDrawUsage");
    auto state=JSValue::array();state.set("0",refreshType);state.set("1",dynamicUsage);auto uniforms=JSValue::array();
    for(const auto* key:{"alphaMap","alphaTest","anisotropy","anisotropyMap","anisotropyRotation","aoMap","aoMapIntensity","attenuationColor","attenuationDistance","bumpMap","bumpScale","clearcoat","clearcoatMap","clearcoatNormalMap","clearcoatNormalScale","clearcoatRoughness","color","dashOffset","dashSize","diffuseRoughness","diffuseRoughnessMap","dispersion","displacementBias","displacementMap","displacementScale","emissive","emissiveIntensity","emissiveMap","envMap","envMapIntensity","envMapRotation","gapSize","gradientMap","ior","iridescence","iridescenceIOR","iridescenceMap","iridescenceThickness","iridescenceThicknessMap","lightMap","lightMapIntensity","linewidth","map","matcap","metalness","metalnessMap","normalMap","normalScale","opacity","reflectivity","retroreflectivity","rotation","roughness","roughnessMap","scale","sheen","sheenColor","sheenColorMap","sheenRoughness","sheenRoughnessMap","shininess","size","specular","specularColor","specularColorMap","specularIntensity","specularIntensityMap","specularMap","steps","thickness","transmission","transmissionMap","wireframe"})invoke(uniforms,"push")({key});
    state.set("2",uniforms);for(int i=3;i<=6;++i)state.set(std::to_string(i),createNativeWeakMap());
    auto constructor=JSValue::constructor(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue {
        auto self=receiver;const auto builder=arg(args,0);self.set("renderObjects",createNativeWeakMap());self.set("hasNode",invoke(self,"containsNode")({builder}));self.set("hasAnimation",builder.get("object").get("isSkinnedMesh").strictEquals(true));self.set("refreshUniforms",captures[0].get("2"));self.set("renderId",0);return self;
    },{state},"NodeMaterialObserver",1);
    auto prototype=constructor.get("prototype");
    for(const auto& [name,length]:std::initializer_list<std::pair<const char*,unsigned>>{{"firstInitialization",1},{"hasDynamicInstancing",1},{"needsVelocity",1},{"getRenderObjectData",1},{"getAttributesData",1},{"containsNode",1},{"getGeometryData",1},{"getTextureData",1},{"getMaterialData",1},{"equals",3},{"getLightsData",2},{"getLights",2},{"needsRefresh",2}}) {
        JSPropertyDescriptor d;d.value=JSValue::function(method,{name,state},name,length);d.writable=d.configurable=true;prototype.defineProperty(name,d);
    }
    constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
JSValue nativeNodeMaterialObserverModule() {
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    auto module=JSValue::object();UtilityNodeRuntime::own(module,"default",nativeNodeMaterialObserverConstructor());module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeNodeMaterialObserverBinding(const std::string& name) {if(name=="default")return nativeNodeMaterialObserverConstructor();return {};}
