#include "math/FrustumArray.h"
#include "cameras/ArrayCamera.h"
#include "objects/Sprite.h"

namespace {
thread_local Matrix4 projScreenMatrix;
std::shared_ptr<Frustum> frustumAt(const std::vector<std::shared_ptr<Frustum>>& frustums, std::size_t index) {
    if (index >= frustums.size() || !frustums[index])
        throw JSTypeError("Cannot read properties of null or undefined (reading frustum method)");
    return frustums[index];
}
}
FrustumArray& FrustumArray::setFromArrayCamera(const ArrayCamera& cameraArray) {
    const auto cameras = cameraArray.cameras;
    for (std::size_t i = 0; i < cameras.size(); ++i) {
        const auto camera = cameras[i];
        if (!camera) throw JSTypeError("Cannot read properties of null or undefined (reading 'projectionMatrix')");
        projScreenMatrix.multiplyMatrices(camera->projectionMatrix, camera->matrixWorldInverse);
        if (_frustums.size() <= i) _frustums.resize(i + 1);
        if (!_frustums[i]) _frustums[i] = std::make_shared<Frustum>();
        _frustums[i]->setFromProjectionMatrix(projScreenMatrix, camera->coordinateSystem, camera->reversedDepth);
    }
    _count = cameras.size();
    return *this;
}
bool FrustumArray::intersectsObject(Object3D& object) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->intersectsObject(object)) return true;
    return false;
}
bool FrustumArray::intersectsSprite(const Sprite& sprite) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->intersectsSprite(sprite)) return true;
    return false;
}
bool FrustumArray::intersectsSphere(const Sphere& sphere) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->intersectsSphere(sphere)) return true;
    return false;
}
bool FrustumArray::intersectsBox(const Box3& box) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->intersectsBox(box)) return true;
    return false;
}
bool FrustumArray::containsPoint(const Vector3& point) const {
    for (std::size_t i = 0; i < _count; ++i) if (frustumAt(_frustums,i)->containsPoint(point)) return true;
    return false;
}
FrustumArray& FrustumArray::copy(const FrustumArray& source) {
    coordinateSystem = source.coordinateSystem;
    for (std::size_t i = 0; i < source._count; ++i) {
        if (_frustums.size() <= i) _frustums.resize(i + 1);
        if (!_frustums[i]) _frustums[i] = std::make_shared<Frustum>();
        _frustums[i]->copy(*frustumAt(source._frustums,i));
    }
    _count = source._count;
    return *this;
}
FrustumArray FrustumArray::clone() const { return FrustumArray().copy(*this); }

// Managed source implementation; typed camera/resource bridges are separate callers.
#include "core/JSModuleObject.h"
#include "core/JSStringEncoding.h"
#include "math/Matrix4.h"
#include "math/FrustumArraySource.inc"
#include <string_view>
namespace FrustumArrayManaged {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned index){return index<args.size()?args[index]:JSValue{};}
void own(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
struct Call {JSValue receiver,callee;JSValue operator()(const Args& args={})const{return callee.call(receiver,args);}};
Call invoke(const JSValue& object,const char* key){return {object,object.get(key)};}
bool less(const JSValue& left,const JSValue& right){const auto a=left.toPrimitive(JSValue::PreferredType::Number),b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String)return jsUTF16(a.string())<jsUTF16(b.string());const auto x=a.toNumber(),y=b.toNumber();return x<y;}
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();const auto state=captures[1],value=arg(args,0);
    if(name=="clone"){const auto result=state.get("FrustumArray").construct();return invoke(result,"copy")({self});}
    if(name=="setFromArrayCamera"){
        const auto cameras=value.get("cameras"),frustums=self.get("_frustums"),matrix=state.get("_projScreenMatrix");
        for(double i=0;less(i,cameras.get("length"));++i){
            const auto camera=cameras.get(JSValue(i));invoke(matrix,"multiplyMatrices")({camera.get("projectionMatrix"),camera.get("matrixWorldInverse")});
            if(frustums.get(JSValue(i)).isUndefined()){auto array=frustums;array.set(JSValue(i),state.get("Frustum").construct());}
            invoke(frustums.get(JSValue(i)),"setFromProjectionMatrix")({matrix,camera.get("coordinateSystem"),camera.get("reversedDepth")});
        }
        self.set("_count",cameras.get("length"));return self;
    }
    if(name=="copy"){
        self.set("coordinateSystem",value.get("coordinateSystem"));const auto frustums=self.get("_frustums"),sourceFrustums=value.get("_frustums");
        for(double i=0;less(i,value.get("_count"));++i){
            if(frustums.get(JSValue(i)).isUndefined()){auto array=frustums;array.set(JSValue(i),state.get("Frustum").construct());}
            invoke(frustums.get(JSValue(i)),"copy")({sourceFrustums.get(JSValue(i))});
        }
        self.set("_count",value.get("_count"));return self;
    }
    if(name=="intersectsObject"||name=="intersectsSprite"||name=="intersectsSphere"||name=="intersectsBox"||name=="containsPoint"){
        const auto frustums=self.get("_frustums");for(double i=0;less(i,self.get("_count"));++i)if(invoke(frustums.get(JSValue(i)),name.c_str())({value}).toBoolean())return true;return false;
    }
    throw std::logic_error("Unknown managed FrustumArray method: "+name);
}
}
JSValue nativeFrustumArrayModule(){
    using namespace FrustumArrayManaged;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"FrustumArray module");
    const auto frustum=nativeFrustumConstructor(),matrix=nativeMatrix4Constructor();auto state=JSValue::object(nullptr);own(state,"Frustum",frustum);own(state,"_projScreenMatrix",matrix.construct());
    auto constructor=JSValue::constructor(+[](const JSValue& receiver,const Args&,const Args&)->JSValue{
        auto self=receiver;self.set("coordinateSystem",WebGLCoordinateSystem);self.set("_frustums",JSValue::array());self.set("_count",0);return {};
    },{},"FrustumArray",0);constructor.setFunctionSourceText(FrustumArraySource::classSource);own(state,"FrustumArray",constructor);
    for(const auto& spec:FrustumArraySource::methods){if(std::string_view(spec.name)=="constructor")continue;auto function=JSValue::function(method,{spec.name,state},spec.name,spec.arity);function.setFunctionSourceText(spec.source);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;constructor.get("prototype").defineProperty(spec.name,d);}
    auto bindings=JSValue::object(nullptr);own(bindings,"FrustumArray",constructor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"FrustumArray",constructor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeFrustumArrayBinding(const std::string& name){const auto keeper=nativeFrustumArrayModule();return FrustumArrayManaged::lexical().lock()->get(name);}
JSValue nativeFrustumArrayConstructor(){return nativeFrustumArrayBinding("FrustumArray");}
