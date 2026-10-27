#include "nodes/utils/MaxMipLevelNode.h"

#include <cmath>
#include <limits>
#include "math/JSNumberMath.h"

double MaxMipLevel::level(double width, double height) {
    // The larger dimension, then the base two logarithm of it.
    if(std::isnan(width)||std::isnan(height))return std::numeric_limits<double>::quiet_NaN();
    const double largest=height>width||(height==0&&width==0&&!std::signbit(height))?height:width;
    return JSNumberMath::log2(largest);
}

bool MaxMipLevel::usable(bool hasWidth) {
    // Against the absence of a width rather than for its truth, so nought passes.
    return hasWidth;
}

double MaxMipLevel::levelWithZeroWidth(double height) {
    // With a width of nought the larger dimension is the height, so the level follows it.
    return level(0,height);
}

MaxMipLevel::ImageSource MaxMipLevel::imageSource(bool hasImages, bool hasFirstImage,
                                                  bool firstImageHasImage) {
    // The first of the three places the source can find an image in.
    if (hasImages) {
        if (hasFirstImage && firstImageHasImage) return ImageSource::FirstImageOrItself;
        return ImageSource::FirstImage;
    }
    return ImageSource::TextureImage;
}
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue update(const JSValue& receiver,const Args&,const Args&){
    auto self=receiver;const auto texture=self.get("texture"),images=texture.get("images");JSValue image;
    if(images.toBoolean()&&greater(images.get("length"),0)){
        const auto first=images.get("0");const auto candidate=first.toBoolean()?images.get("0").get("image"):first;image=candidate.toBoolean()?candidate:images.get("0");
    }else image=texture.get("image");
    if(image.toBoolean()&&!image.get("width").isUndefined()){
        const auto width=image.get("width"),height=image.get("height");const auto x=width.toNumber(),y=height.toNumber();self.set("value",MaxMipLevel::level(x,y));
    }return {};
}
}
JSValue nativeMaxMipLevelNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MaxMipLevelNode module");
    const auto base=nativeUniformNodeConstructor(),types=nativeNodeConstantsBinding("NodeUpdateType"),tsl=nativeTSLBase();auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{auto self=superConstruct(target,c,{0});self.set("_textureNode",arg(args,0));self.set("updateType",c[1].get("FRAME"));return self;},{holder,types},"MaxMipLevelNode",1);
    holder.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.configurable=true;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "MaxMipLevelNode";},{},"get type",0);ctor.defineProperty("type",type);
    for(const auto* key:{"textureNode","texture"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{const auto node=self.get("_textureNode");return c[0].strictEquals("texture")?node.get("value"):node;},{key},std::string("get ")+key,0);prototype.defineProperty(key,d);}
    JSPropertyDescriptor d;d.value=JSValue::function(update,{},"update",0);d.writable=d.configurable=true;prototype.defineProperty("update",d);
    const auto proxy=nativeTSLBaseBinding("nodeProxy").call({}, {ctor});const auto factory=invoke(proxy,"setParameterLength")({1});
    auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);own(bindings,"maxMipLevel",factory);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"default",ctor);own(module,"maxMipLevel",factory);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeMaxMipLevelNodeBinding(const std::string& name){const auto keeper=nativeMaxMipLevelNodeModule();return lexical().lock()->get(name);}
JSValue nativeMaxMipLevelNodeConstructor(){return nativeMaxMipLevelNodeBinding("default");}
MaxMipLevelNode::MaxMipLevelNode(const JSValue& textureNode):object_(constructor().construct({textureNode})){}
JSValue MaxMipLevelNode::update()const{return object_.get("update").call(object_);}
