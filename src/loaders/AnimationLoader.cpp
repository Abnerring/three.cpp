#include "loaders/AnimationLoader.h"
#include "animation/AnimationRuntime.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
}
JSValue AnimationLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{initialize(self,arg(args,0));return {};},{},"AnimationLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));auto result=constructor.get("prototype");result.setPrototype(Loader::prototype());
        JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
        descriptor.value=JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue {
            const auto json=arg(args,0);auto animations=JSValue::array();
            for(double i=0;i<json.get("length").toNumber();++i) {
                const auto clip=AnimationClip::parse(json.get(JSValue(i).toString()));animations.get("push").call(animations,{nativeAnimationClip(clip)});
            }
            return animations;
        },{},"parse",1);result.defineProperty("parse",descriptor);
        descriptor.value=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw std::runtime_error("AnimationLoader.load requires the pending native FileLoader transport");},{},"load",4);result.defineProperty("load",descriptor);return result;
    }();return value;
}
AnimationLoader::AnimationLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
AnimationClip::Clips AnimationLoader::parseAnimations(JSValue json) {
    const auto array=Loader::parse(json);AnimationClip::Clips result;
    for(std::uint32_t i=0;i<array.length();++i)result.push_back(animationClipFromNative(array.get(std::to_string(i))));return result;
}
