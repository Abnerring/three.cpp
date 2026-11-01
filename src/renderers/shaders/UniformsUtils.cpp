#include "renderers/shaders/UniformsUtils.h"
#include "math/ColorManagement.h"
#include "utils.h"
#include "core/JSGlobal.h"
#include "core/JSString.h"
#include "core/JSNumber.h"
#include "core/JSBoolean.h"
#include <unordered_set>
namespace {
template<class F>void forIn(const JSValue& object,F body){
    if(object.isNull()||object.isUndefined())return;
    auto receiver=object;
    if(!receiver.isObject())receiver=receiver.toObject();
    // V8 snapshots candidates across the chain. Proxy descriptor checks belong
    // to the per-key filter, after collection and the preceding loop body.
    std::unordered_set<std::string> visited;std::vector<std::string> keys;
    auto level=receiver;
    while(!level.isNull()){
        const bool proxy=level.isProxy();
        for(const auto& key:level.ownPropertyNames()){
            if(visited.contains(key))continue;
            if(proxy){visited.insert(key);keys.push_back(key);}
            else{
                const auto d=level.getOwnPropertyDescriptor(key);
                if(!d)continue;
                visited.insert(key);if(d->enumerable.value_or(false))keys.push_back(key);
            }
        }
        level=level.getPrototype();
    }
    for(const auto& key:keys){
        auto owner=receiver;
        while(!owner.isNull()){
            const auto d=owner.getOwnPropertyDescriptor(key);
            // Ordinary properties remain candidates when their enumerable flag
            // changes. V8 applies the current flag only at a Proxy boundary.
            if(d){if(!owner.isProxy()||d->enumerable.value_or(false))body(key);break;}
            // Integer-indexed exotic lookup stops at an invalid numeric index;
            // it does not fall through to an inherited numeric property.
            if(owner.isHostObject("three.AnimationArray")&&
               (key=="-0"||JSValue(JSValue(key).toNumber()).toString()==key))break;
            owner=owner.getPrototype();
        }
    }
}

bool isThreeObject(const JSValue& property){
 if(!property.toBoolean())return false;
 for(const auto* name:{"isColor","isMatrix3","isMatrix4","isVector2","isVector3","isVector4","isTexture","isQuaternion"})if(property.get(name).toBoolean())return true;
 return false;
}
bool currentArrayIsArray(const JSValue& property){const auto array=nativeGlobalBinding("Array");const auto method=array.get("isArray");return method.call(array,{property}).toBoolean();}
}
JSValue cloneUniforms(const JSValue& input){
 const auto source=input;
 auto dst=JSValue::object();
 forIn(source,[&](const std::string& name){
  dst.set(name,JSValue::object());
  forIn(source.get(name),[&](const std::string& key){
   const auto property=source.get(name).get(key);
   if(isThreeObject(property)){
    if(property.get("isRenderTargetTexture").toBoolean()){
     ThreeUtils::nativeWarnFunction().call({}, {"UniformsUtils: Textures of render targets cannot be cloned via cloneUniforms() or mergeUniforms()."});
     dst.get(name).set(key,nullptr);
    }else{auto target=dst.get(name);const auto clone=property.get("clone");const auto value=clone.call(property);target.set(key,value);}
   }else if(currentArrayIsArray(property)){
    if(isThreeObject(property.get("0"))){
     auto cloned=JSValue::array();const auto length=property.get("length");
     for(double i=0;i<length.toNumber();++i){const auto object=property.get(JSValue(i));const auto method=object.get("clone");const auto value=method.call(object);cloned.set(JSValue(i),value);}
     dst.get(name).set(key,cloned);
    }else{auto target=dst.get(name);const auto slice=property.get("slice");const auto value=slice.call(property);target.set(key,value);}
   }else{dst.get(name).set(key,property);}
  });
 });return dst;
}
JSValue mergeUniforms(const JSValue& input){
 const auto uniforms=input;
 auto merged=JSValue::object();
 for(double i=0;i<uniforms.get("length").toNumber();++i){const auto temporary=cloneUniforms(uniforms.get(JSValue(i)));forIn(temporary,[&](const std::string& key){merged.set(key,temporary.get(key));});}
 return merged;
}
JSValue cloneUniformsGroups(const JSValue& input){
 const auto source=input;
 auto dst=JSValue::array();
 for(double i=0;i<source.get("length").toNumber();++i){const auto push=dst.get("push");const auto group=source.get(JSValue(i));const auto clone=group.get("clone");const auto value=clone.call(group);push.call(dst,{value});}
 return dst;
}
JSValue getUnlitUniformColorSpace(const JSValue& input){
 const auto renderer=input;
 const auto method=renderer.get("getRenderTarget");const auto target=method.call(renderer);
 if(target.isNull())return renderer.get("outputColorSpace");
 if(target.get("isXRRenderTarget").strictEquals(true))return target.get("texture").get("colorSpace");
 return nativeColorManagement().get("workingColorSpace");
}


#include "UniformsUtilsSource.inc"
namespace {
using Args = std::vector<JSValue>;
const JSValue& argument(const Args& args, std::size_t index) {
    static const JSValue undefined;
    return index < args.size() ? args[index] : undefined;
}
JSValue exportedFunction(const JSValue&, const Args& args, const Args& captures) {
    const auto& name = captures[1].string();
    if (name == "cloneUniforms")
        return cloneUniforms(argument(args, 0));
    if (name == "mergeUniforms")
        return mergeUniforms(argument(args, 0));
    if (name == "cloneUniformsGroups")
        return cloneUniformsGroups(argument(args, 0));
    return getUnlitUniformColorSpace(argument(args, 0));
}
JSPropertyDescriptor literal(JSValue value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = std::move(value);
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    return descriptor;
}
JSValue exports() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isObject())
        return *value;
    auto environment = JSValue::object(nullptr);
    for (const auto* name :
         {"cloneUniforms", "mergeUniforms", "cloneUniformsGroups", "getUnlitUniformColorSpace"}) {
        auto fn = JSValue::constructor(exportedFunction, {environment, name}, name, 1,
                                             JSValue::ConstructorKind::Callable);
        fn.setFunctionSourceText(UniformsUtilsSource::source(name));
        environment.defineProperty(name, literal(fn));
    }
    auto legacy = JSValue::object();
    // The source export is a literal: inherited setters do not receive its
    // initial clone/merge fields. Later ordinary assignments are observable.
    legacy.defineProperty("clone", literal(environment.get("cloneUniforms")));
    legacy.defineProperty("merge", literal(environment.get("mergeUniforms")));
    environment.defineProperty("UniformsUtils", literal(legacy));
    environment.retainForHeapLifetime();
    cached = JSWeakValue(environment);
    return environment;
}
} // namespace
JSValue nativeUniformsUtils() {
    return exports().get("UniformsUtils");
}
JSValue nativeCloneUniforms() {
    return exports().get("cloneUniforms");
}
JSValue nativeMergeUniforms() {
    return exports().get("mergeUniforms");
}
JSValue nativeCloneUniformsGroups() {
    return exports().get("cloneUniformsGroups");
}
JSValue nativeGetUnlitUniformColorSpace() {
    return exports().get("getUnlitUniformColorSpace");
}
