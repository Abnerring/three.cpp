#include "loaders/AnimationLoader.h"
#include "animation/AnimationClip.h"
#include "animation/AnimationRuntime.h"
#include "core/JsonValue.h"
#include "loaders/FileLoader.h"
#include <memory>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
}
std::vector<std::shared_ptr<AnimationClip>> AnimationLoader::parseAnimations(JSValue json) {
    // Reading an array goes through a string index: an integer one selects the name overload of get
    // and is read as a null pointer, which is the trap that cost several rounds on STLLoader.
    const auto length=json.get("length").toNumber();
    auto clips=std::make_shared<std::vector<std::shared_ptr<AnimationClip>>>();
    clips->reserve(static_cast<std::size_t>(length<0?0:length));
    for(double index=0;index<length;index+=1)
        clips->push_back(AnimationClip::parse(json.get(std::to_string(static_cast<long long>(index)))));
    return *clips;
}

// parse is the JS door over the same loop, so the two entry points cannot drift apart.
JSValue AnimationLoader::parse(JSValue json) {
    return nativeAnimationClips(std::make_shared<std::vector<std::shared_ptr<AnimationClip>>>(parseAnimations(json)));
}
AnimationLoader::AnimationLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue AnimationLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"AnimationLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4},{"parse",1}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
                AnimationLoader loader=AnimationLoader::fromObject(receiver);
                if(captures[0].string()=="parse")return loader.parse(arg(args,0));
                return loader.load(arg(args,0),arg(args,1),arg(args,2),arg(args,3));
            },{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}
namespace {
// The completion callback has to be a plain function, so the callbacks for the load in flight wait
// in a thread local slot; the injected transport answers synchronously.
struct Pending {
    JSValue onLoad{},onError{},manager{},url{};
};
Pending& pending(){static thread_local Pending value;return value;}
JSValue loadedCallback(const JSValue&,const Args& args,const Args&){
    auto& slot=pending();
    try {
        AnimationLoader parser;
        const auto clips=parser.parse(JSValue::fromJSON(args.empty()?std::string{}:args[0].toString()));
        if(!slot.onLoad.isUndefined())slot.onLoad.call(JSValue{}, {clips});
    } catch(const std::exception& error) {
        // Upstream logs when no onError is supplied; there is no console natively.
        if(!slot.onError.isUndefined())slot.onError.call(JSValue{}, {JSValue(error.what())});
        if(!slot.manager.isUndefined())slot.manager.get("itemError").call(slot.manager, {slot.url});
    }
    return {};
}
}
JSValue AnimationLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    auto& slot=pending();
    slot.onLoad=onLoad;slot.onError=onError;slot.url=url;
    slot.manager=object_.get("manager");
    FileLoader fileLoader(slot.manager);
    const auto call=[&](const char* name,const JSValue& value){const auto method=fileLoader.object().get(name);if(!method.isUndefined())method.call(fileLoader.object(),{value});};
    call("setPath",object_.get("path"));
    call("setRequestHeader",object_.get("requestHeader"));
    call("setWithCredentials",object_.get("withCredentials"));
    const auto result=fileLoader.load(url,JSValue::function(loadedCallback,{}, "onLoad", 1),onProgress,onError);
    slot= Pending{};
    return result;
}
