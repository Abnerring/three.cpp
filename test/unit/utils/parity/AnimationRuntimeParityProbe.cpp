#include "animation/AnimationRuntime.h"
#include "animation/AnimationClip.h"
#include "animation/PropertyBinding.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "core/NativeObject.h"
#include "math/interpolants/LinearInterpolant.h"
#include "math/interpolants/CubicInterpolant.h"
#include <iostream>

namespace {
JSValue call(JSValue self,const char* method,std::vector<JSValue> args={}){return self.get(method).call(self,args);}
JsonValue names(const JSValue& object){JsonValue::Array result;for(const auto& key:object.keys())result.emplace_back(key);return result;}
JsonValue array(const JSValue& value){JsonValue::Array result;for(int i=0;i<value.get("length").toNumber();++i)result.push_back(value.get(std::to_string(i)).toJSON().value_or(JsonValue()));return result;}
void append(JSValue trace,JSValue event){trace.set(std::to_string(trace.length()),event);}
JsonValue interpolantHook(const std::string& kind){
    std::shared_ptr<Interpolant> native;
    if(kind=="settings")native=std::make_shared<CubicInterpolant>(Float32Array({0,1}),Float32Array({2,6}),1);
    else native=std::make_shared<LinearInterpolant>(Float32Array({0,1,2}),Float32Array({2,6,10}),1);
    auto value=nativeInterpolant(native),trace=JSValue::array(),returned=JSValue::array(1);returned.set("0",0);
    const auto fn=JSValue::function([](const JSValue& self,const auto& args,const auto& captures){
        const auto kind=captures[0].string();auto trace=captures[1],result=captures[2];
        append(trace,"call:"+kind+":"+(args.empty()?std::string{}:args[0].toString()));self.collectGarbage();
        if(kind=="interval"){
            auto receiver=self;receiver.set("interpolate_",JSValue::function([](const JSValue& self,const auto& args,const auto& captures){
                append(captures[0],"replacement:"+args[2].toString());self.collectGarbage();auto result=captures[1];result.set("0",77+args[2].toNumber());return result;
            },{trace,result}));return JSValue();
        }
        if(kind=="settings"){auto settings=JSValue::object();settings.set("endingStart",ZeroSlopeEnding);settings.set("endingEnd",ZeroSlopeEnding);return settings;}
        result.set("0",kind=="copy"?100+args[0].toNumber():40+args[2].toNumber());return result;
    },{kind,trace,returned,value});
    const auto name=kind=="copy"?"copySampleValue_":kind=="interval"||kind=="throwInterval"?"intervalChanged_":kind=="settings"?"getSettings_":"interpolate_";
    JSPropertyDescriptor descriptor;descriptor.configurable=true;
    descriptor.getter=JSValue::function([](const JSValue& self,const auto&,const auto& captures){
        append(captures[1],"get:"+captures[0].string());self.collectGarbage();
        if(captures[0].string()=="getterReplace"){
            JSPropertyDescriptor d;d.writable=d.enumerable=d.configurable=true;
            d.value=JSValue::function([](const JSValue& self,const auto& args,const auto& captures){
                append(captures[0],"installed:"+args[2].toString());self.collectGarbage();auto result=captures[1];result.set("0",900+args[2].toNumber());return result;
            },{captures[1],captures[3]});auto receiver=self;receiver.defineProperty("interpolate_",d);
        }
        return captures[2];
    },{kind,trace,kind=="throwInterval"?JSValue(nullptr):fn,returned});value.defineProperty(name,descriptor);
    JsonValue::Array samples,identities;
    const std::vector<double> times=kind=="copy"?std::vector<double>{-1,3}:kind=="settings"?std::vector<double>{.25,.75}:std::vector<double>{.5,.75,1.5};
    for(const auto time:times){try{const auto result=call(value,"evaluate",{time});samples.push_back(array(result));identities.emplace_back(result.sameValue(returned));}
        catch(const JSTypeError&){samples.emplace_back("TypeError");identities.emplace_back(false);}}
    return JsonValue::Object{{"samples",samples},{"identities",identities},{"trace",array(trace)},{"cached",value.get("_cachedIndex").number()}};
}
}
int main(){
    JsonValue output=JsonValue::Object{};
    auto root=JSValue::object();root.set("x",2);
    auto native=std::make_shared<PropertyBinding>(root,".x"),other=std::make_shared<PropertyBinding>(root,".x");
    auto binding=nativePropertyBinding(native),b2=nativePropertyBinding(other);
    output["bindingInitial"]=JsonValue::Object{{"keys",names(binding)},
        {"sharedPrototype",binding.getPrototype().sameValue(b2.getPrototype())},
        {"initialGet",binding.get("getValue").sameValue(binding.get("_getValue_unbound"))},
        {"initialSet",binding.get("setValue").sameValue(binding.get("_setValue_unbound"))}};
    auto buffer=JSValue::array(2);call(binding,"getValue",{buffer,1});
    output["bindingBound"]=JsonValue::Object{{"keys",names(binding)},{"buffer",array(buffer)},
        {"targetIdentity",binding.get("targetObject").sameValue(root)},
        {"getIdentity",binding.get("getValue").sameValue(binding.get("_getValue_direct"))},
        {"propertyName",binding.get("propertyName").toJSON().value()}};
    buffer.set("1",7);call(binding,"setValue",{buffer,1});call(binding,"unbind");
    output["bindingAfter"]=JsonValue::Object{{"x",root.get("x").number()},{"nodeNull",binding.get("node").isNull()},
        {"getIdentity",binding.get("getValue").sameValue(binding.get("_getValue_unbound"))}};
    auto group=JSValue::object();group.set("nCachedObjects_",0);auto list=JSValue::array();list.set("0",binding);
    group.set("subscribe_",JSValue::function([](const JSValue&,const auto&,const auto& c){return c[0];},{list}));
    auto composite=nativePropertyBinding(std::make_shared<PropertyBindingComposite>(group,".x"));
    call(composite,"getValue",{buffer,0});
    output["composite"]=JsonValue::Object{{"keys",names(composite)},
        {"rootUndefined",composite.get("rootNode").isUndefined()},{"pathUndefined",composite.get("path").isUndefined()},{"value",buffer.get("0").number()}};
    auto mixer=nativePropertyMixer(std::make_shared<PropertyMixer>(native,"number",1));
    output["mixerInitial"]=JsonValue::Object{{"keys",names(mixer)},{"cacheUndefined",mixer.get("_cacheIndex").isUndefined()},
        {"mixIdentity",mixer.get("_mixBufferRegion").sameValue(mixer.get("_lerp"))}};
    call(mixer,"saveOriginalState");auto storage=mixer.get("buffer");storage.set("0",11);call(mixer,"accumulate",{0,.5});call(mixer,"apply",{0});
    output["mixerApplied"]=JsonValue::Object{{"x",root.get("x").number()},{"buffer",array(storage)},
        {"bufferIdentity",storage.sameValue(mixer.get("buffer"))},{"weight",mixer.get("cumulativeWeight").number()}};
    call(mixer,"restoreOriginalState");output["restored"]=root.get("x").number();
    auto interp=nativeInterpolant(std::make_shared<LinearInterpolant>(Float32Array({0,1,2}),Float32Array({2,6,14}),1));
    auto settings=JSValue::object();interp.set("settings",settings);auto result=interp.get("resultBuffer");JsonValue::Array samples;
    for(double t:{-.5,0.,.25,1.,1.5,2.,3.})samples.emplace_back(array(call(interp,"evaluate",{t})));
    output["interpolant"]=JsonValue::Object{{"keys",names(interp)},{"samples",samples},{"resultIdentity",result.sameValue(interp.get("resultBuffer"))},
        {"settingsIdentity",call(interp,"getSettings_").sameValue(settings)},{"cached",interp.get("_cachedIndex").number()}};
    auto track=nativeKeyframeTrack(std::make_shared<NumberKeyframeTrack>(".x",Float32Array({0,1}),Float32Array({2,10})));
    auto curve=call(track,"createInterpolant");auto factory=track.get("createInterpolant");call(track,"shift",{2});
    auto tracks=std::make_shared<AnimationClip::Tracks>(AnimationClip::Tracks{keyframeTrackFromNative(track)});
    auto clip=nativeAnimationClip(std::make_shared<AnimationClip>("clip",-1,tracks));
    auto clone=call(clip,"clone");
    output["clipTrack"]=JsonValue::Object{{"duration",clip.get("duration").number()},{"valueType",track.get("ValueTypeName").string()},
        {"shiftedTimes",array(track.get("times"))},{"sharedTimes",curve.get("parameterPositions").sameValue(track.get("times"))},
        {"factoryIdentity",factory.sameValue(track.get("InterpolantFactoryMethodLinear"))},
        {"trackIdentity",clip.get("tracks").get("0").sameValue(track)},
        {"arrayIdentity",clip.get("tracks").sameValue(clip.get("tracks"))},
        {"cloneArrayDistinct",!clone.get("tracks").sameValue(clip.get("tracks"))},
        {"cloneTrackDistinct",!clone.get("tracks").get("0").sameValue(track)}};
    for(const auto* kind:{"interpolate","copy","interval","settings","throwInterval","getterReplace"})output[std::string("hook:")+kind]=interpolantHook(kind);
    std::cout<<output.dump()<<'\n';
}
