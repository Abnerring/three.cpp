#include "parity/WebGLShaderCacheFixture.h"
#include <string>
using namespace ShaderCacheFixture;
int main(int argc,char** argv){
    const std::string mode=argc>1?argv[1]:"retain";const auto realm=JSValue::object();JSValue retained;
    JSWeakValue cacheWeak,materialWeak,stageWeak,setWeak,shaderMapWeak,materialMapWeak,cacheConstructorWeak,stageConstructorWeak,payloadWeak,stagePayloadWeak;
    double initialId=0;
    {
        auto cache=WebGLShaderCache().object(),material=object(),stage=call(cache,"_getShaderStage",{"code"});initialId=stage.get("id").number();
        call(cache,"update",{material,stage,stage});auto set=call(cache,"_getShaderCacheForMaterial",{material});
        cacheWeak=JSWeakValue(cache);materialWeak=JSWeakValue(material);stageWeak=JSWeakValue(stage);setWeak=JSWeakValue(set);
        shaderMapWeak=JSWeakValue(cache.get("shaderCache"));materialMapWeak=JSWeakValue(cache.get("materialCache"));
        if(mode=="retain"||mode=="remove"||mode=="dispose")retained=cache;
        if(mode=="remove")call(cache,"remove",{material});if(mode=="dispose")call(cache,"dispose");
        if(mode=="cycle")stage.set("code",cache);
        if(mode.rfind("detached:",0)==0){
            const auto constructor=nativeWebGLShaderCacheConstructor(),stageConstructor=stage.get("constructor");
            auto prototype=constructor.get("prototype"),stagePrototype=stageConstructor.get("prototype"),payload=object(),stagePayload=object();
            prop(prototype,"payload",payload);prop(stagePrototype,"payload",stagePayload);
            cacheConstructorWeak=JSWeakValue(constructor);stageConstructorWeak=JSWeakValue(stageConstructor);payloadWeak=JSWeakValue(payload);stagePayloadWeak=JSWeakValue(stagePayload);
            retained=prototype.get(mode.substr(9));
            for(const char* name:{"constructor","update","remove","getVertexShaderStage","getFragmentShaderStage","dispose","_getShaderCacheForMaterial","_getShaderStage"})prototype.erase(name);
            stagePrototype.erase("constructor");
        }
    }
    realm.collectGarbage();
    O result{{"mode",mode},{"cache",!cacheWeak.expired()},{"material",!materialWeak.expired()},{"stage",!stageWeak.expired()},{"set",!setWeak.expired()},{"shaderMap",!shaderMapWeak.expired()},{"materialMap",!materialMapWeak.expired()}};
    if(mode.rfind("detached:",0)==0){
        result["cacheConstructor"]=!cacheConstructorWeak.expired();result["stageConstructor"]=!stageConstructorWeak.expired();result["payload"]=!payloadWeak.expired();result["stagePayload"]=!stagePayloadWeak.expired();
        result["detached"]=retained.isCallable();const auto stageConstructor=stageConstructorWeak.lock();
        result["nextId"]=stageConstructor?JsonValue(stageConstructor->construct({"later"}).get("id").number()-initialId):JsonValue("expired");
    }
    emit(result);
}
