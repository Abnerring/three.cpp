// Direct port of three.js InterleavedBufferAttribute.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See repository LICENSE.
#include "core/InterleavedBufferAttribute.h"
#include "core/JSDataSnapshot.h"
#include "core/BufferAttribute.h"
#include "math/MathUtils.h"
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
using Interpolant_detail::add;
using Interpolant_detail::multiply;
JSValue arg(const Args& args,std::size_t i) {return i<args.size()?args[i]:JSValue{};}
JSValue call(JSValue receiver,const char* name,Args args={}) {const auto method=receiver.get(name);return method.call(receiver,args);}
std::string key(const JSValue& value) {return value.toPrimitive(JSValue::PreferredType::String).toString();}
JSValue normalize(JSValue value,JSValue array,bool inverse) {
    const auto constructor=array.get("constructor");
    if(constructor.sameValue(NativeArray_detail::constructor(6))) return value;
    constexpr double maxima[]{127,255,32767,65535,2147483647,4294967295,0,0,255};
    for(std::size_t i:{0u,1u,2u,3u,4u,5u,8u}) if(constructor.sameValue(NativeArray_detail::constructor(i))) {
        const auto n=value.toNumber();
        if(!inverse) return MathUtils::round(n*maxima[i]);
        const auto divided=n/maxima[i];return (i==0||i==2||i==4)?MathUtils::max(divided,-1):divided;
    }
    throw std::runtime_error("THREE.MathUtils: Invalid component type.");
}
JSValue indexOf(JSValue self,JSValue index) {
    const auto stride=self.get("data").get("stride");const auto product=multiply(index,stride);
    const auto offset=self.get("offset");return add(product,offset);
}
struct Vector {JSValue x=0,y=0,z=0;};
thread_local Vector vector;
// A const C++ argument is borrowed only during the synchronous call. Saved JS
// matrix/element handles read the final snapshot after return, including unwind.
template<class Matrix> struct MatrixBorrow {
    const Matrix* borrowed;
    Matrix snapshot;
    explicit MatrixBorrow(const Matrix& matrix):borrowed(&matrix),snapshot(matrix) {}
    void finish() noexcept {if(borrowed) {snapshot=*borrowed;borrowed=nullptr;}}
};
template<class Matrix> struct MatrixBorrowScope {
    std::shared_ptr<MatrixBorrow<Matrix>> state;
    JSValue object;
    explicit MatrixBorrowScope(const Matrix& matrix):state(std::make_shared<MatrixBorrow<Matrix>>(matrix)) {
        const auto host=JSValue::hostObject(state,state.get(),"three.BorrowedAttributeMatrix");
        auto elements=JSValue::array(static_cast<std::uint32_t>(matrix.elements().size()));
        for(std::size_t i=0;i<matrix.elements().size();++i) {
            JSPropertyDescriptor descriptor;descriptor.enumerable=descriptor.configurable=true;
            descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue {
                const auto state=std::static_pointer_cast<MatrixBorrow<Matrix>>(captures[0].hostPayload("three.BorrowedAttributeMatrix"));
                return (state->borrowed?*state->borrowed:state->snapshot).elements()[static_cast<std::size_t>(captures[1].toNumber())];
            },{host,JSValue(i)});
            descriptor.setter=JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue {
                const auto state=std::static_pointer_cast<MatrixBorrow<Matrix>>(captures[0].hostPayload("three.BorrowedAttributeMatrix"));
                if(state->borrowed) throw JSTypeError("Cannot write a borrowed const Matrix argument");
                state->snapshot.elements()[static_cast<std::size_t>(captures[1].toNumber())]=arg(args,0).toNumber();return {};
            },{host,JSValue(i)});
            elements.defineProperty(std::to_string(i),descriptor);
        }
        object=nativeMatrix(std::shared_ptr<Matrix>(state,&state->snapshot));object.setHostSlot("elements",elements);
    }
    ~MatrixBorrowScope() {state->finish();}
};
void normalizeVector() {
    const auto xx=multiply(vector.x,vector.x),yy=multiply(vector.y,vector.y),zz=multiply(vector.z,vector.z);
    const auto length=std::sqrt(xx+yy+zz),scale=1/((length==0||std::isnan(length))?1:length);
    vector.x=multiply(vector.x,scale);vector.y=multiply(vector.y,scale);vector.z=multiply(vector.z,scale);
}
void transform(JSValue matrix,bool normal,bool direction) {
    const auto x=vector.x,y=vector.y,z=vector.z,e=matrix.get("elements");
    const auto product=[&](int i,JSValue v){const auto value=e.get(std::to_string(i));return multiply(value,v);};
    const auto sum=[&](int a,int b,int c){const auto first=product(a,x),second=product(b,y),third=product(c,z);return first+second+third;};
    if(normal) {
        vector.x=sum(0,3,6);vector.y=sum(1,4,7);vector.z=sum(2,5,8);normalizeVector();return;
    }
    if(direction) {vector.x=sum(0,4,8);vector.y=sum(1,5,9);vector.z=sum(2,6,10);normalizeVector();return;}
    const auto projected=sum(3,7,11);const auto denominator=add(projected,e.get("15"));const auto w=1/denominator.toNumber();
    const auto row=[&](int a,int b,int c,int d){const auto value=sum(a,b,c);const auto translated=add(value,e.get(std::to_string(d)));return multiply(translated,w);};
    vector.x=row(0,4,8,12);vector.y=row(1,5,9,13);vector.z=row(2,6,10,14);
}
JSValue uninterleave(JSValue self) {
    auto array=JSValue::array();
    for(double i=0;i<self.get("count").toNumber();++i) {
        const auto index=indexOf(self,i);
        for(double j=0;j<self.get("itemSize").toNumber();++j) {
            const auto push=array.get("push");const auto source=self.get("data").get("array");const auto position=add(index,j);
            const auto value=source.get(key(position));push.call(array,{value});
        }
    }
    return array;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures) {
    auto self=receiver;const auto& name=captures[0].string();
    if(name=="count"||name=="array") return self.get("data").get(name);
    if(name=="needsUpdate") {auto data=self.get("data");data.set("needsUpdate",arg(args,0));return {};}
    if(name=="getComponent"||name=="getX"||name=="getY"||name=="getZ"||name=="getW") {
        const auto array=name=="getComponent"?self.get("array"):self.get("data").get("array");
        auto index=indexOf(self,arg(args,0));
        if(name=="getComponent") index=add(index,arg(args,1));
        else if(name!="getX") index=add(index,name=="getY"?1:name=="getZ"?2:3);
        auto value=array.get(key(index));if(self.get("normalized").toBoolean()) value=normalize(value,self.get("array"),true);return value;
    }
    if(name=="setComponent"||name=="setX"||name=="setY"||name=="setZ"||name=="setW") {
        auto value=arg(args,name=="setComponent"?2:1);
        if(self.get("normalized").toBoolean()) value=normalize(value,self.get("array"),false);
        auto array=self.get("data").get("array");auto index=indexOf(self,arg(args,0));
        if(name=="setComponent") index=add(index,arg(args,1));
        else if(name!="setX") index=add(index,name=="setY"?1:name=="setZ"?2:3);
        array.set(key(index),value);return self;
    }
    if(name=="setXY"||name=="setXYZ"||name=="setXYZW") {
        const auto index=indexOf(self,arg(args,0));const auto length=name=="setXY"?2:name=="setXYZ"?3:4;
        Args values;for(int i=0;i<length;++i) values.push_back(arg(args,i+1));
        if(self.get("normalized").toBoolean()) for(auto& value:values) value=normalize(value,self.get("array"),false);
        for(int i=0;i<length;++i) {auto array=self.get("data").get("array");const auto position=add(index,i);array.set(key(position),values[i]);}
        return self;
    }
    if(name=="applyMatrix4"||name=="applyNormalMatrix"||name=="transformDirection") {
        const auto count=name=="applyMatrix4"?self.get("data").get("count"):self.get("count");
        for(double i=0;i<count.toNumber();++i) {
            vector.x=call(self,"getX",{i});vector.y=call(self,"getY",{i});vector.z=call(self,"getZ",{i});
            transform(arg(args,0),name=="applyNormalMatrix",name=="transformDirection");
            const auto setter=self.get("setXYZ");const auto x=vector.x,y=vector.y,z=vector.z;setter.call(self,{i,x,y,z});
        }
        return self;
    }
    if(name=="clone"||name=="toJSON") {
        auto metadata=arg(args,0);
        if(metadata.isUndefined()) {
            std::clog<<"InterleavedBufferAttribute."<<name<<"(): "<<(name=="clone"?"Cloning":"Serializing")<<" an interleaved buffer attribute will de-interleave buffer data.\n";
            const auto array=uninterleave(self);
            if(name=="clone") {
                const auto constructor=self.get("array").get("constructor");const auto values=constructor.construct({array});
                const auto size=self.get("itemSize"),normalized=self.get("normalized");const auto storage=animationArray(values);
                const auto result=std::visit([&](const auto& input)->std::shared_ptr<BufferAttribute> {
                    if constexpr(std::is_same_v<std::decay_t<decltype(input)>,JSValue>) throw JSTypeError("BufferAttribute array must be a TypedArray");
                    else return std::make_shared<BufferAttribute>(input,size.toNumber(),normalized.toBoolean());
                },storage.storage());auto wrapper=nativeAttribute(result);wrapper.setHostSlot("rawItemSize",size);wrapper.set("normalized",normalized);return wrapper;
            }
            auto result=JSValue::object();result.set("itemSize",self.get("itemSize"));result.set("type",self.get("array").get("constructor").get("name"));
            result.set("array",array);result.set("normalized",self.get("normalized"));return result;
        }
        if(metadata.get("interleavedBuffers").isUndefined()) metadata.set("interleavedBuffers",JSValue::object());
        const auto cache=metadata.get("interleavedBuffers");const auto uuid=self.get("data").get("uuid");
        if(cache.get(key(uuid)).isUndefined()) {
            auto destination=metadata.get("interleavedBuffers");const auto property=self.get("data").get("uuid");const auto propertyKey=key(property);
            const auto data=self.get("data"),callback=data.get(name);destination.set(propertyKey,callback.call(data,{metadata}));
        }
        if(name=="clone") {
            const auto cache=metadata.get("interleavedBuffers");const auto uuid=self.get("data").get("uuid");const auto data=cache.get(key(uuid));
            const auto size=self.get("itemSize"),offset=self.get("offset"),normalized=self.get("normalized");
            return InterleavedBufferAttribute(data,size,offset,normalized).object();
        }
        auto result=JSValue::object();result.set("isInterleavedBufferAttribute",true);result.set("itemSize",self.get("itemSize"));
        result.set("data",self.get("data").get("uuid"));result.set("offset",self.get("offset"));result.set("normalized",self.get("normalized"));return result;
    }
    throw JSTypeError("Unknown InterleavedBufferAttribute method");
}
}
void InterleavedBufferAttribute::initialize(JSValue self,JSValue data,JSValue itemSize,JSValue offset,JSValue normalized) {
    if(normalized.isUndefined()) normalized=false;
    self.set("isInterleavedBufferAttribute",true);self.set("name","");self.set("data",data);self.set("itemSize",itemSize);self.set("offset",offset);self.set("normalized",normalized);
}
JSValue InterleavedBufferAttribute::constructor() {
    static thread_local JSWeakValue weak;
    if(const auto value=weak.lock();value&&value->isConstructible())return *value;
        const auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue {
            initialize(self,arg(args,0),arg(args,1),arg(args,2),arg(args,3));return {};
        },{},"InterleavedBufferAttribute",3);auto result=constructor.get("prototype");
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"getComponent",2},{"setComponent",3},{"getX",1},{"getY",1},{"getZ",1},{"getW",1},
            {"setX",2},{"setY",2},{"setZ",2},{"setW",2},{"setXY",3},{"setXYZ",4},{"setXYZW",5},{"applyMatrix4",1},{"applyNormalMatrix",1},{"transformDirection",1},{"clone",1},{"toJSON",1}}) {
            JSPropertyDescriptor d;d.value=JSValue::function(method,{name},name,arity);d.writable=d.configurable=true;result.defineProperty(name,d);
        }
        for(const char* name:{"count","array","needsUpdate"}) {
            JSPropertyDescriptor d;const auto function=JSValue::function(method,{name},std::string(name==std::string("needsUpdate")?"set ":"get ")+name,name==std::string("needsUpdate")?1:0);
            if(name==std::string("needsUpdate")) d.setter=function;else d.getter=function;d.configurable=true;result.defineProperty(name,d);
        }
        constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
JSValue InterleavedBufferAttribute::prototype() { return constructor().get("prototype"); }
InterleavedBufferAttribute::InterleavedBufferAttribute(JSValue data,JSValue itemSize,JSValue offset,JSValue normalized)
    :object_(constructor().construct({data,itemSize,offset,normalized})) {}
JSValue InterleavedBufferAttribute::getComponent(JSValue index,JSValue component) const {return call(object_,"getComponent",{index,component});}
JSValue InterleavedBufferAttribute::setComponent(JSValue index,JSValue component,JSValue value) {return call(object_,"setComponent",{index,component,value});}
#define THREE_INTERLEAVED_GET(Name) double InterleavedBufferAttribute::Name(double index) const {return call(object_,#Name,{index}).toNumber();}
THREE_INTERLEAVED_GET(getX) THREE_INTERLEAVED_GET(getY) THREE_INTERLEAVED_GET(getZ) THREE_INTERLEAVED_GET(getW)
#undef THREE_INTERLEAVED_GET
#define THREE_INTERLEAVED_SET(Name) InterleavedBufferAttribute& InterleavedBufferAttribute::Name(double index,double value) {call(object_,#Name,{index,value});return *this;}
THREE_INTERLEAVED_SET(setX) THREE_INTERLEAVED_SET(setY) THREE_INTERLEAVED_SET(setZ) THREE_INTERLEAVED_SET(setW)
#undef THREE_INTERLEAVED_SET
InterleavedBufferAttribute& InterleavedBufferAttribute::setXY(double i,double x,double y) {call(object_,"setXY",{i,x,y});return *this;}
InterleavedBufferAttribute& InterleavedBufferAttribute::setXYZ(double i,double x,double y,double z) {call(object_,"setXYZ",{i,x,y,z});return *this;}
InterleavedBufferAttribute& InterleavedBufferAttribute::setXYZW(double i,double x,double y,double z,double w) {call(object_,"setXYZW",{i,x,y,z,w});return *this;}
InterleavedBufferAttribute& InterleavedBufferAttribute::applyMatrix4(const Matrix4& matrix) {MatrixBorrowScope borrowed(matrix);call(object_,"applyMatrix4",{borrowed.object});return *this;}
InterleavedBufferAttribute& InterleavedBufferAttribute::applyNormalMatrix(const Matrix3& matrix) {MatrixBorrowScope borrowed(matrix);call(object_,"applyNormalMatrix",{borrowed.object});return *this;}
InterleavedBufferAttribute& InterleavedBufferAttribute::transformDirection(const Matrix4& matrix) {MatrixBorrowScope borrowed(matrix);call(object_,"transformDirection",{borrowed.object});return *this;}
JSValue InterleavedBufferAttribute::clone(JSValue metadata) const {return call(object_,"clone",{metadata});}
JSValue InterleavedBufferAttribute::toJSON(JSValue metadata) const {return call(object_,"toJSON",{metadata});}
JsonValue InterleavedBufferAttribute::toJSON() const {return *snapshotData(serializeAttribute());}
