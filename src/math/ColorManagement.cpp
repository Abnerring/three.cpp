// Ported from three.js ColorManagement.js; see THREEJS-LICENSE.
#include "math/ColorManagement.h"
#include "math/Color.h"
#include "math/Matrix3.h"
#include "core/NativeObject.h"
#include "core/JSModuleObject.h"
#include "constants.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue();}
JSPropertyDescriptor data(JSValue value){JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;return d;}
void literal(JSValue object,const char* key,JSValue value){object.defineProperty(key,data(std::move(value)));}
JSValue lookup(const JSValue& self,const JSValue& space){auto spaces=self.get("spaces");return spaces.get(space);}
double transfer(const JSValue& value,bool toSRGB) {
    const auto comparison=value.toNumber();
    if(toSRGB){if(comparison<.0031308)return value.toNumber()*12.92;return 1.055*std::pow(value.toNumber(),.41666)-.055;}
    if(comparison<.04045)return value.toNumber()*.0773993808;
    return std::pow(value.toNumber()*.9478672986+.0521327014,2.4);
}
JSValue convert(const JSValue& self,const Args& args,const Args&) {
    auto color=arg(args,0);const auto source=arg(args,1),target=arg(args,2);
    if(self.get("enabled").strictEquals(false)||source.strictEquals(target)||!source.toBoolean()||!target.toBoolean())return color;
    if(lookup(self,source).get("transfer").strictEquals(SRGBTransfer))
        for(const auto* field:{"r","g","b"})color.set(field,transfer(color.get(field),false));
    const auto fromPrimaries=lookup(self,source).get("primaries");
    const auto toPrimaries=lookup(self,target).get("primaries");
    if(!fromPrimaries.strictEquals(toPrimaries)) {
        auto method=color.get("applyMatrix3");const auto matrix=lookup(self,source).get("toXYZ");method.call(color,{matrix});
        method=color.get("applyMatrix3");const auto second=lookup(self,target).get("fromXYZ");method.call(color,{second});
    }
    if(lookup(self,target).get("transfer").strictEquals(SRGBTransfer))
        for(const auto* field:{"r","g","b"})color.set(field,transfer(color.get(field),true));
    return color;
}
JSValue workingToColorSpace(const JSValue& self,const Args& args,const Args&) {
    const auto fn=self.get("convert"),working=self.get("workingColorSpace");return fn.call(self,{arg(args,0),working,arg(args,1)});
}
JSValue colorSpaceToWorking(const JSValue& self,const Args& args,const Args&) {
    const auto fn=self.get("convert"),working=self.get("workingColorSpace");return fn.call(self,{arg(args,0),arg(args,1),working});
}
JSValue getPrimaries(const JSValue& self,const Args& args,const Args&){return lookup(self,arg(args,0)).get("primaries");}
JSValue getTransfer(const JSValue& self,const Args& args,const Args&){const auto space=arg(args,0);if(space.strictEquals(NoColorSpace))return LinearTransfer;return lookup(self,space).get("transfer");}
JSValue getToneMappingMode(const JSValue& self,const Args& args,const Args&){const auto mode=lookup(self,arg(args,0)).get("outputColorSpaceConfig").get("toneMappingMode");return mode.toBoolean()?mode:JSValue("standard");}
JSValue getLuminanceCoefficients(const JSValue& self,const Args& args,const Args&){
    const auto target=arg(args,0);auto space=arg(args,1);if(space.isUndefined())space=self.get("workingColorSpace");
    const auto fn=target.get("fromArray");const auto coefficients=lookup(self,space).get("luminanceCoefficients");return fn.call(target,{coefficients});
}
JSValue define(const JSValue& self,const Args& args,const Args&){
    const auto object=JSValue::intrinsicObjectConstructor(),fn=object.get("assign"),spaces=self.get("spaces");fn.call(object,{spaces,arg(args,0)});return {};
}
JSValue getMatrix(const JSValue& self,const Args& args,const Args&){
    const auto target=arg(args,0),copy=target.get("copy"),toXYZ=lookup(self,arg(args,1)).get("toXYZ");
    const auto receiver=copy.call(target,{toXYZ}),multiply=receiver.get("multiply"),fromXYZ=lookup(self,arg(args,2)).get("fromXYZ");
    return multiply.call(receiver,{fromXYZ});
}
JSValue getDrawingBufferColorSpace(const JSValue& self,const Args& args,const Args&){return lookup(self,arg(args,0)).get("outputColorSpaceConfig").get("drawingBufferColorSpace");}
JSValue getUnpackColorSpace(const JSValue& self,const Args& args,const Args&){auto space=arg(args,0);if(space.isUndefined())space=self.get("workingColorSpace");return lookup(self,space).get("workingColorSpaceConfig").get("unpackColorSpace");}
void method(JSValue object,const char* key,JSValue::NativeFunction fn,unsigned arity){literal(object,key,JSValue::constructor(fn,{},key,arity,JSValue::ConstructorKind::Callable));}
JSValue array(std::initializer_list<double> values){auto result=JSValue::array();unsigned i=0;for(auto value:values)result.defineProperty(std::to_string(i++),data(value));return result;}
JSValue build() {
    auto object=JSValue::object();literal(object,"enabled",true);literal(object,"workingColorSpace",LinearSRGBColorSpace);literal(object,"spaces",JSValue::object());
    method(object,"convert",convert,3);method(object,"workingToColorSpace",workingToColorSpace,2);method(object,"colorSpaceToWorking",colorSpaceToWorking,2);
    method(object,"getPrimaries",getPrimaries,1);method(object,"getTransfer",getTransfer,1);method(object,"getToneMappingMode",getToneMappingMode,1);
    method(object,"getLuminanceCoefficients",getLuminanceCoefficients,1);method(object,"define",define,1);method(object,"_getMatrix",getMatrix,3);
    method(object,"_getDrawingBufferColorSpace",getDrawingBufferColorSpace,1);method(object,"_getUnpackColorSpace",getUnpackColorSpace,0);
    auto makeMatrix=[](const Args& values){auto matrix=JSValue::object(nativeMatrix3Prototype());matrix.set("elements",array({1,0,0,0,1,0,0,0,1}));const auto set=matrix.get("set");set.call(matrix,values);return matrix;};
    auto toXYZ=makeMatrix({.4123908,.3575843,.1804808,.2126390,.7151687,.0721923,.0193308,.1191948,.9505322});
    auto fromXYZ=makeMatrix({3.2409699,-1.5373832,-.4986108,-.9692436,1.8759675,.0415551,.0556301,-.2039770,1.0569715});
    // These two are module-level source globals, independent of replacement spaces.
    toXYZ.retainForHeapLifetime();fromXYZ.retainForHeapLifetime();
    const auto primaries=array({.640,.330,.300,.600,.150,.060}),coefficients=array({.2126,.7152,.0722}),whitePoint=array({.3127,.3290});
    auto spaces=JSValue::object();
    for(const auto* name:{"srgb-linear","srgb"}) {
        auto space=JSValue::object();literal(space,"primaries",primaries);literal(space,"whitePoint",whitePoint);
        literal(space,"transfer",std::string(name)=="srgb"?SRGBTransfer:LinearTransfer);literal(space,"toXYZ",toXYZ);literal(space,"fromXYZ",fromXYZ);literal(space,"luminanceCoefficients",coefficients);
        if(std::string(name)=="srgb-linear"){auto config=JSValue::object();literal(config,"unpackColorSpace",SRGBColorSpace);literal(space,"workingColorSpaceConfig",config);}
        auto config=JSValue::object();literal(config,"drawingBufferColorSpace",SRGBColorSpace);literal(space,"outputColorSpaceConfig",config);literal(spaces,name,space);
    }
    const auto fn=object.get("define");fn.call(object,{spaces});return object;
}
struct BorrowedColor {Color* color;};
std::shared_ptr<BorrowedColor> borrowed(const JSValue& self){auto value=std::static_pointer_cast<BorrowedColor>(self.hostPayload("three.BorrowedColor"));if(!value->color)throw JSTypeError("Borrowed Color view has retired");return value;}
JSValue read(const JSValue& self,const Args&,const Args& captures){const auto color=borrowed(self)->color;const auto key=captures[0].string();return key=="r"?color->r:key=="g"?color->g:color->b;}
JSValue write(const JSValue& self,const Args& args,const Args& captures){auto color=borrowed(self)->color;const auto value=arg(args,0).toNumber();const auto key=captures[0].string();(key=="r"?color->r:key=="g"?color->g:color->b)=value;return {};}
class ColorView {
    std::shared_ptr<BorrowedColor> state_;
  public:
    JSValue object;
    explicit ColorView(Color& color):state_(std::make_shared<BorrowedColor>(&color)),object(JSValue::hostObject(state_,state_.get(),"three.BorrowedColor")) {
        object.setPrototype(nativeColorPrototype());literal(object,"isColor",true);
        for(const auto* field:{"r","g","b"}){JSPropertyDescriptor d;d.getter=JSValue::function(read,{field},std::string("get ")+field,0);d.setter=JSValue::function(write,{field},std::string("set ")+field,1);d.enumerable=d.configurable=true;object.defineProperty(field,d);}
    }
    ~ColorView(){state_->color=nullptr;}
};
Color& nativeCall(Color& color,const char* name,const Args& args){auto management=nativeColorManagement();const auto fn=management.get(name);ColorView view(color);Args parameters{view.object};parameters.insert(parameters.end(),args.begin(),args.end());fn.call(management,parameters);return color;}
}
JSValue nativeColorManagement(){static thread_local JSWeakValue cached;if(auto value=cached.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"ColorManagement");auto value=build();value.retainForHeapLifetime();cached=JSWeakValue(value);return value;}
Color& ColorManagement::convert(Color& color,const std::string& source,const std::string& target){return nativeCall(color,"convert",{source,target});}
Color& ColorManagement::colorSpaceToWorking(Color& color,const std::string& source){return nativeCall(color,"colorSpaceToWorking",{source});}
Color& ColorManagement::workingToColorSpace(Color& color,const std::string& target){return nativeCall(color,"workingToColorSpace",{target});}
