#include "core/InterleavedBufferAttribute.h"
#include "core/BufferGeometry.h"
#include "objects/Mesh.h"
#include "objects/SkinnedMesh.h"
#include "core/Raycaster.h"
#include <iostream>
#include <cmath>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue list(std::initializer_list<JSValue> input) {auto value=JSValue::array();for(const auto& v:input)value.get("push").call(value,{v});return value;}
void push(JSValue target,JSValue value) {target.get("push").call(target,{value});}
JSValue call(JSValue target,const char* name,Args args={}) {return target.get(name).call(target,args);}
JSValue number(JSValue value) {
    if(value.isUndefined()) return "#undefined";if(value.type()!=JSValue::Type::Number)return value;
    const auto n=value.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n<0?"#-Infinity":"#Infinity";if(n==0&&std::signbit(n))return "#-0";return value;
}
JSValue values(JSValue array) {auto result=JSValue::array();for(double i=0;i<array.get("length").toNumber();++i)push(result,number(array.get(JSValue(i).toString())));return result;}
JSValue keys(JSValue value) {auto result=JSValue::array();for(auto& key:value.keys())push(result,key);return result;}
JSValue vector(const Vector3& v) {return list({number(v.x),number(v.y),number(v.z)});}
JSValue bounds(BufferGeometry& geometry) {
    geometry.computeBoundingBox();geometry.computeBoundingSphere();auto result=JSValue::object();
    result.set("box",list({vector(geometry.boundingBox->min),vector(geometry.boundingBox->max)}));result.set("sphere",list({vector(geometry.boundingSphere->center),number(geometry.boundingSphere->radius)}));return result;
}
template<class F> JSValue attempt(F&& callback) {try{return callback();}catch(const JSRangeError&){return "RangeError";}catch(const JSTypeError&){return "TypeError";}catch(const std::exception&){return "Error";}}
JSValue make(int kind) {auto array=NativeArray_detail::constructor(kind).construct({24});for(int i=0;i<24;++i)array.set(std::to_string(i),(i-8)*3.25);array.get("buffer").set("_uuid","storage");return array;}
JSValue attributeState(JSValue attribute) {
    auto result=JSValue::object();result.set("keys",keys(attribute));
    for(const char* key:{"isBufferAttribute","isInterleavedBufferAttribute","name","count","itemSize","offset","normalized","needsUpdate"})result.set(key,number(attribute.get(key)));
    auto components=JSValue::array();for(double index:{0.,1.,2.,.5,-.5}) {
        auto row=JSValue::array();for(const char* name:{"getX","getY","getZ","getW"})push(row,attempt([&]{return number(call(attribute,name,{index}));}));push(components,row);
    }
    result.set("components",components);result.set("values",values(attribute.get("array")));return result;
}
JSValue basicCase(int kind,bool normalized) {
    const auto array=make(kind);InterleavedBuffer data(array,6);data.uuid="buffer";InterleavedBufferAttribute attr(data,3,1,normalized);attr.name="position";
    auto result=JSValue::object();result.set("initial",attributeState(attr));result.set("identity",list({attr.data.strictEquals(data),attr.array.strictEquals(array)}));
    const std::pair<const char*,Args> writes[]={{"setX",{.5,.3}},{"setY",{1,-.7}},{"setZ",{0,.25}},{"setW",{2,1.5}},
        {"setComponent",{1,1,.125}},{"setXY",{0,.1,.2}},{"setXYZ",{1,-.2,.3,.4}},{"setXYZW",{2,.5,.6,.7,.8}}};
    auto statuses=JSValue::array();for(const auto& [name,args]:writes)push(statuses,attempt([&]{return call(attr,name,args).sameValue(attr);}));result.set("writes",statuses);
    result.set("after",attributeState(attr));for(JSValue value:{JSValue(true),JSValue(1),JSValue(false),JSValue(true)})attr.needsUpdate=value;
    result.set("version",data.version.value());result.set("deinterleavedJSON",attr.toJSON(JSValue{}));
    auto flat=attr.clone();result.set("flat",list({flat.get("isBufferAttribute"),number(flat.get("isInterleavedBufferAttribute")),values(flat.get("array")),flat.get("name"),flat.get("normalized"),flat.get("count")}));
    auto meta=JSValue::object();result.set("json",attr.toJSON(meta));result.set("meta",meta);
    auto cloneMeta=JSValue::object();auto copied=attr.clone(cloneMeta);result.set("clone",attributeState(copied));
    result.set("cloneIdentity",list({!copied.get("data").sameValue(data),!copied.get("array").sameValue(array),copied.get("data").get("version").strictEquals(0)}));
    return result;
}
JSValue transformCase(int kind,int operation) {
    InterleavedBuffer data(make(kind),6);InterleavedBufferAttribute attr(data,3,1,kind!=6&&kind!=7);
    auto matrix=std::make_shared<Matrix4>();matrix->makeRotationY(.35);matrix->scale(Vector3(1.2,.8,1.1));matrix->setPosition(2,-3,4);
    if(operation==0)attr.applyMatrix4(*matrix);
    else if(operation==1){Matrix3 normal;normal.getNormalMatrix(*matrix);attr.applyNormalMatrix(normal);}
    else attr.transformDirection(*matrix);
    auto result=JSValue::object();result.set("data",values(data.array));result.set("version",data.version.value());return result;
}
JSValue geometryCase(bool indexed,bool relative) {
    auto array=nativeArray(Float32Array({-1,-1,0, 0,0,1, 0,0, 1,-1,0, 0,0,1, 1,0, 0,1,0, 0,0,1, .5f,1}));
    array.get("buffer").set("_uuid","storage");InterleavedBuffer data(array,8);data.uuid="buffer";
    auto position=std::make_shared<InterleavedBufferAttribute>(data,3,0),normal=std::make_shared<InterleavedBufferAttribute>(data,3,3),uv=std::make_shared<InterleavedBufferAttribute>(data,2,6);
    auto geometry=std::make_shared<BufferGeometry>();geometry->uuid="geometry";geometry->setAttribute("position",position).setAttribute("normal",normal).setAttribute("uv",uv);
    if(indexed)geometry->setIndex(std::vector<std::uint32_t>{2,0,1});
    InterleavedBuffer morph(nativeArray(Float32Array({.1f,.2f,.3f,9,.4f,.5f,.6f,9,.7f,.8f,.9f,9})),4);morph.uuid="morph";morph.array.get("buffer").set("_uuid","morph-storage");
    geometry->morphAttributes["position"]={std::make_shared<InterleavedBufferAttribute>(morph,3,0)};geometry->morphTargetsRelative=relative;
    auto result=JSValue::object();result.set("before",bounds(*geometry));result.set("json",JSValue::fromJSON(geometry->toJSON()));
    auto clone=geometry->clone();auto cp=std::dynamic_pointer_cast<InterleavedBufferAttribute>(clone->getAttribute("position")),cn=std::dynamic_pointer_cast<InterleavedBufferAttribute>(clone->getAttribute("normal"));
    result.set("cloneIdentity",list({cp->data.strictEquals(cn->data),!cp->data.strictEquals(data),!cp->array.strictEquals(array),clone->getAttribute("position")!=position}));
    cp->setX(0,42);result.set("cloneData",list({cp->getX(0),position->getX(0),cn->array.get("0")}));
    Matrix4 matrix;matrix.makeRotationZ(.2);matrix.setPosition(1,2,3);geometry->applyMatrix4(matrix);
    result.set("after",bounds(*geometry));result.set("data",values(array));result.set("version",data.version.value());
    auto expanded=geometry->toNonIndexed();result.set("expandedSame",expanded==geometry);
    if(indexed){result.set("expanded",JSValue::fromJSON(expanded->getAttribute("position")->toJSON()));result.set("expandedUV",JSValue::fromJSON(expanded->getAttribute("uv")->toJSON()));}
    geometry->computeVertexNormals();result.set("normals",values(array));result.set("normalVersion",data.version.value());
    Mesh mesh(geometry);mesh.updateMatrixWorld(true);Raycaster ray(Vector3(1,2,10),Vector3(0,0,-1));std::vector<Intersection> hits;mesh.raycast(ray,hits);
    auto intersections=JSValue::array();for(const auto& hit:hits){auto value=JSValue::object();value.set("distance",hit.distance);value.set("point",vector(hit.point));if(hit.uv)value.set("uv",list({hit.uv->x,hit.uv->y}));push(intersections,value);}result.set("hits",intersections);return result;
}
void getter(JSValue object,const char* name,JSValue log,JSValue value) {
    JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c){push(c[0],c[1]);return c[2];},{log,name,value});object.defineProperty(name,d);
}
JSValue orderCase() {
    auto result=JSValue::object();
    for(const char* name:{"getX","getY","getComponent","setX","setY","setComponent","setXY","setXYZ"}) {
        auto log=JSValue::array(),data=JSValue::object(),array=make(1);getter(data,"array",log,array);getter(data,"stride",log,6);
        InterleavedBufferAttribute attribute(data,3,1,true);getter(attribute.object(),"data",log,data);getter(attribute.object(),"offset",log,1);getter(attribute.object(),"normalized",log,true);
        auto value=attempt([&]{auto output=call(attribute,name,{1,2,.3,.4});return output.sameValue(attribute)?JSValue("self"):number(output);});
        result.set(name,list({value,log,values(array)}));
    }
    for(const char* operation:{"clone","toJSON"}) {
        auto log=JSValue::array();InterleavedBuffer data(make(6),6);data.uuid="buffer";InterleavedBufferAttribute value(data,3,1);
        auto meta=JSValue::object(),cache=JSValue::object();getter(meta,"interleavedBuffers",log,cache);getter(value.object(),"data",log,data);
        getter(value.object(),"itemSize",log,3);getter(value.object(),"offset",log,1);getter(value.object(),"normalized",log,false);
        call(value,operation,{meta});result.set(operation,log);
    }
    return result;
}
JSValue edgeCase() {
    auto result=JSValue::object();InterleavedBuffer data(make(6),6);InterleavedBufferAttribute attribute(data,3,"1");
    result.set("stringOffset",list({number(call(attribute,"getX",{0})),number(call(attribute,"getY",{0})),number(call(attribute,"getComponent",{0,"2"}))}));
    call(attribute,"setXY",{0,7,8});result.set("stringWrites",values(data.array));
    attribute.offset=1;data.count=1.5;result.set("changedCount",attribute.count.value());data.array=nativeArray(Float32Array({1,2,3,4}));result.set("changedArray",values(attribute.array));
    result.set("readonly",attempt([&]{attribute.count=9;return JSValue("ok");}));
    InterleavedBufferAttribute absent;result.set("missingCount",attempt([&]{return absent.count.value();}));result.set("missingArray",attempt([&]{return absent.array.value();}));
    result.set("nullMeta",attempt([&]{return attribute.clone(nullptr);}));
    InterleavedBufferAttribute regular(InterleavedBuffer(make(6),6),3,1);regular.array.value().set("constructor",JSValue::object());regular.normalized=true;
    result.set("customCtorNormalization",attempt([&]{return call(regular,"getX",{0});}));
    auto object=JSValue::object();object.set("offset",9);InterleavedBufferAttribute raw(InterleavedBuffer(make(6),6),3,1,object);
    const auto clone=raw.clone();result.set("normalizedIdentity",clone.get("normalized").sameValue(object));
    raw.itemSize="3";const auto stringClone=raw.clone();result.set("rawItemSize",list({stringClone.get("itemSize"),call(stringClone,"toJSON").get("itemSize"),stringClone.get("count")}));return result;
}
JSValue transformOrderCase(int operation,bool reentrant) {
    auto log=JSValue::array(),count=JSValue::object(),state=JSValue::object();state.set("coercions",0);state.set("entered",false);
    count.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args& c){auto state=c[1];const auto n=state.get("coercions").toNumber()+1;state.set("coercions",n);push(c[0],"count:"+JSValue(n).toString());return JSValue(n<=2?2:0);},{log,state}));
    InterleavedBuffer data(nativeArray(Float32Array({1,2,3,4,5,6})),3);data.count=count;
    InterleavedBufferAttribute attribute(data,3,0);InterleavedBufferAttribute nested(InterleavedBuffer(nativeArray(Float32Array({11,12,13})),3),3,0);
    auto matrix=JSValue::object(),elements=JSValue::array();for(double n:{1.,0.,0.,0.,1.,0.,0.,0.,1.})push(elements,n);
    if(operation!=1) {elements=JSValue::array();for(double n:{1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.,10.,20.,30.,1.})push(elements,n);}
    getter(matrix,"elements",log,elements);
    const auto name=operation==0?"applyMatrix4":operation==1?"applyNormalMatrix":"transformDirection";
    for(const char* component:{"getX","getY","getZ"}) {const auto original=attribute.object().get(component);
        attribute.object().set(component,JSValue::function([](const JSValue& self,const Args& args,const Args& c){push(c[0],c[1].string()+":"+args[0].toString());
            if(c[1].string()=="getY"&&c[6].toBoolean()&&!c[2].get("entered").toBoolean()) {auto state=c[2];state.set("entered",true);call(c[3],c[5].string().c_str(),{c[4]});}
            return c[7].call(self,args);},{log,component,state,nested.object(),matrix,name,reentrant,original}));}
    const auto setter=attribute.object().get("setXYZ");attribute.object().set("setXYZ",JSValue::function([](const JSValue& self,const Args& args,const Args& c){push(c[0],list({"set",args[0],args[1],args[2],args[3]}));return c[1].call(self,args);},{log,setter}));
    call(attribute,name,{matrix});return list({log,values(data.array),values(nested.array)});
}
JSValue gcCase() {
    auto anchor=JSValue::object();auto mesh=std::make_shared<Mesh>();std::weak_ptr<Mesh> weak=mesh;
    {InterleavedBuffer data(make(6),6);auto attribute=std::make_shared<InterleavedBufferAttribute>(data,3,1);mesh->geometry->setAttribute("position",attribute);
      const auto owner=nativeObject(mesh);attribute->object().set("owner",owner);data.array.value().set("owner",owner);data.array.get("buffer").set("owner",owner);}
    anchor.collectGarbage();auto result=JSValue::object();result.set("retained",!weak.expired());
    {auto attribute=std::dynamic_pointer_cast<InterleavedBufferAttribute>(mesh->geometry->getAttribute("position"));result.set("identity",attribute->getProperty("owner").sameValue(nativeObject(mesh)));}
    mesh.reset();anchor.collectGarbage();result.set("released",weak.expired());return result;
}
}
int main(){try{auto result=JSValue::object();
    for(int kind=0;kind<9;++kind)for(bool normalized:{false,true})result.set("basic:"+std::to_string(kind)+":"+std::to_string(normalized),basicCase(kind,normalized));
    for(int kind:{0,1,2,6,7})for(int operation=0;operation<3;++operation)result.set("transform:"+std::to_string(kind)+":"+std::to_string(operation),transformCase(kind,operation));
    for(bool indexed:{false,true})for(bool relative:{false,true})result.set("geometry:"+std::to_string(indexed)+":"+std::to_string(relative),geometryCase(indexed,relative));
    for(int operation=0;operation<3;++operation)for(bool reentrant:{false,true})result.set("transformOrder:"+std::to_string(operation)+":"+std::to_string(reentrant),transformOrderCase(operation,reentrant));
    result.set("order",orderCase());result.set("edges",edgeCase());result.set("gc",gcCase());std::cout<<result.toJSON()->dump()<<'\n';
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
