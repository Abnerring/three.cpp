#include "nodes/display/FrontFacingNode.h"

FrontFacing::Answer FrontFacing::answer(bool fragmentStage, Side side) {
    // Facing is a fragment stage question, so anything else says true without asking.
    if (!fragmentStage) return Answer::True;
    // A material drawn only on the back is never front facing.
    if (side == Side::Back) return Answer::False;
    return Answer::AskBuilder;
}

double FrontFacing::faceDirection(bool frontFacing) {
    // Twice the flag less one: true gives one and false gives minus one.
    const double flag = frontFacing ? 1.0 : 0.0;
    return flag * 2.0 - 1.0;
}

double FrontFacing::backSideFactor(Side side, bool frontFacing) {
    // Sided to the front leaves the vector alone; sided to the back always turns it over; sided
    // both ways turns it over exactly where the facing number is minus one.
    if (side == Side::Front) return 1.0;
    if (side == Side::Back) return -1.0;
    return faceDirection(frontFacing);
}

std::string FrontFacing::deprecatedRenameWarning() {
    return "TSL: \"directionToFaceDirection()\" has been renamed to \"negateOnBackSide()\".";
}
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "constants.h"
#include "utils.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue&,const Args& args,const Args& c){
    if(c[0].strictEquals("isCacheable"))return false;const auto builder=arg(args,0);
    if(!builder.get("shaderStage").strictEquals("fragment"))return "true";
    const auto material=builder.get("material");if(material.get("side").strictEquals(BackSide))return "false";
    return invoke(builder,"getFrontFacing")();
}
}
JSValue nativeFrontFacingNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"FrontFacingNode module");
    const auto base=nativeNodeConstructor(),tsl=nativeTSLBase();auto bindings=JSValue::object(nullptr);
    const auto ctor=makeConstructor("FrontFacingNode",0,+[](const JSValue& target,const Args&,const Args& c)->JSValue{auto self=superConstruct(target,c,{"bool"});self.set("isFrontFacingNode",true);return self;},{{"isCacheable",0},{"generate",1}},method);
    own(bindings,"default",ctor);const auto frontFacing=nativeTSLBaseBinding("nodeImmutable").call({}, {ctor});own(bindings,"frontFacing",frontFacing);
    const auto value=nativeTSLBaseBinding("float").call({}, {frontFacing});const auto multiplied=invoke(value,"mul")({2.0});own(bindings,"faceDirection",invoke(multiplied,"sub")({1.0}));
    const auto body=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        auto vector=TSLFunctionRuntime::first(arg(args,0));const auto material=arg(args,1).get("material"),side=material.get("side");
        if(side.strictEquals(BackSide))vector=invoke(vector,"mul")({-1.0});else if(side.strictEquals(DoubleSide))vector=invoke(vector,"mul")({c[0].get("faceDirection")});return vector;
    },{bindings},"",2);
    own(bindings,"negateOnBackSide",nativeTSLBaseBinding("Fn").call({}, {body}));
    own(bindings,"directionToFaceDirection",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        ThreeUtils::warnOnce({FrontFacing::deprecatedRenameWarning()});return c[0].get("negateOnBackSide").call({}, {arg(args,0)});
    },{bindings},"directionToFaceDirection",1));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);auto module=JSValue::object();for(const auto* key:{"default","directionToFaceDirection","faceDirection","frontFacing","negateOnBackSide"})own(module,key,bindings.get(key));module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeFrontFacingNodeBinding(const std::string& name){const auto keeper=nativeFrontFacingNodeModule();return lexical().lock()->get(name);}
JSValue nativeFrontFacingNodeConstructor(){return nativeFrontFacingNodeBinding("default");}
FrontFacingNode::FrontFacingNode():object_(constructor().construct()){}
JSValue FrontFacingNode::generate(const JSValue& builder)const{return object_.get("generate").call(object_,{builder});}
