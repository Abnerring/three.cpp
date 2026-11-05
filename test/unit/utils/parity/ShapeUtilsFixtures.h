#pragma once
#include "extras/ShapeUtils.h"
#include "math/Vector2.h"
#include "core/JSPromise.h"
#include <functional>
namespace ShapeUtilsFixtures {
using Args=std::vector<JSValue>;
inline JSValue object(){return JSValue::object();}
inline JSValue list(const Args& values={}){auto out=JSValue::array();for(const auto& v:values)out.set(std::to_string(out.length()),v);return out;}
inline void push(JSValue a,const JSValue& v){a.set(std::to_string(a.length()),v);}
inline JSValue call(const JSValue& self,const char* name,const Args& a={}){return self.get(name).call(self,a);}
inline void getter(JSValue object,const char* key,JSValue function){JSPropertyDescriptor d;d.getter=function;d.configurable=true;object.defineProperty(key,d);}
inline JSValue outcome(const std::function<JSValue()>& body,const JSValue& reason={}){try{return body();}catch(const JSTypeError&){return "TypeError";}catch(...){return nativeExceptionValue(std::current_exception()).sameValue(reason)?"reason":"other";}}
struct Restore {JSValue object,key;std::optional<JSPropertyDescriptor> previous;Restore(JSValue object,JSValue key):object(object),key(key),previous(object.getOwnPropertyDescriptor(key)){}~Restore(){if(previous)object.defineProperty(key,*previous);else object.erase(key);}};
inline JSValue points(const JSValue& rows,bool alias=false){auto result=JSValue::array();for(unsigned i=0;i<rows.length();++i){const auto row=rows.get(i);auto value=nativeVector2Constructor().construct({row.get("0"),row.get(1)});if(alias&&i>0&&call(value,"equals",{result.get("0")}).toBoolean())value=result.get("0");push(result,value);}return result;}
inline JSValue coordinates(const JSValue& points){auto out=JSValue::array();for(unsigned i=0;i<points.length();++i)push(out,list({points.get(i).get("x"),points.get(i).get("y")}));return out;}
inline JSValue metadata(){
    const auto ctor=nativeShapeUtilsConstructor();auto out=object(),methods=object();out.set("name",ctor.get("name"));out.set("length",ctor.get("length"));out.set("source",call(ctor,"toString"));out.set("instancePrototype",ctor.construct().getPrototype().sameValue(ctor.get("prototype")));out.set("call",outcome([&](){return ctor.call({});}));
    for(const auto* name:{"area","isClockWise","triangulateShape"}){const auto d=ctor.getOwnPropertyDescriptor(name);const auto f=*d->value;auto row=object();row.set("name",f.get("name"));row.set("length",f.get("length"));row.set("source",call(f,"toString"));row.set("flags",list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));methods.set(name,row);}out.set("methods",methods);return out;
}
inline JSValue numeric(const JSValue& request){
    const auto ctor=nativeShapeUtilsConstructor();const bool alias=request.get("alias").toBoolean();auto contour=points(request.get("contour"),alias),holes=JSValue::array();
    for(unsigned i=0;i<request.get("holes").length();++i)push(holes,request.get("shared").toBoolean()?contour:points(request.get("holes").get(i),alias));
    const auto original=contour;auto out=object();out.set("area",call(ctor,"area",{contour}));out.set("clockwise",call(ctor,"isClockWise",{contour}));out.set("faces",call(ctor,"triangulateShape",{contour,holes}));out.set("sameContour",contour.sameValue(original));out.set("contour",coordinates(contour));auto holeRows=JSValue::array();for(unsigned i=0;i<holes.length();++i)push(holeRows,coordinates(holes.get(i)));out.set("holes",holeRows);return out;
}
inline JSValue areaOrder(const std::string& variant){
    const auto ctor=nativeShapeUtilsConstructor();auto trace=list(),reason=object(),contour=object();
    getter(contour,"length",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"length");if(c[1].strictEquals("lengthObject")){auto length=object();length.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"length:valueOf");return 3;},{c[0]}));return length;}return 3;},{trace,variant}));
    for(unsigned i=0;i<3;++i){auto point=object();for(const auto* axis:{"x","y"})getter(point,axis,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        const auto name=std::to_string(static_cast<unsigned>(c[1].toNumber()))+c[2].toString();push(c[0],std::string("get:")+name);if(c[3].strictEquals("throw")&&name=="0y")throw JSThrownValue(c[4]);
        if(c[3].strictEquals("reenter")&&name=="0y"){push(c[0],"reenter");call(c[5],"area",{list()});}
        auto scalar=object();scalar.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],std::string("value:")+c[1].toString());return c[2];},{c[0],name,c[2].strictEquals("x")?c[1]:JSValue(c[1].toNumber()*c[1].toNumber())}));return scalar;
    },{trace,i,axis,variant,reason,ctor}));contour.set(i,point);}
    auto out=object();out.set("result",outcome([&](){return call(ctor,"area",{contour});},reason));out.set("trace",trace);return out;
}
inline JSValue dynamicArea(const std::string& variant){
    auto ctor=nativeShapeUtilsConstructor(),trace=list(),reason=object(),argument=object();Restore restore(ctor,"area");
    getter(ctor,"area",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"get:area",self.sameValue(c[1])}));return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:area",self.sameValue(c[1]),a[0].sameValue(c[2])}));auto scalar=object();scalar.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"valueOf");if(c[1].strictEquals("throw"))throw JSThrownValue(c[2]);return -1;},{c[0],c[3],c[4]}));return scalar;},{c[0],c[1],c[2],c[3],c[4]});},{trace,ctor,argument,variant,reason}));
    auto out=object();out.set("result",outcome([&](){return ctor.get("isClockWise").call(object(),{argument});},reason));out.set("trace",trace);return out;
}
inline JSValue triangulateOrder(const std::string& variant){
    auto ctor=nativeShapeUtilsConstructor(),earcut=nativeShapeUtilsBinding("Earcut"),trace=list(),reason=object(),coordinate=object();Restore restore(earcut,"triangulate");
    coordinate.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSThrownValue("unexpected coordinate conversion");}));
    auto makePoint=[&](const char* name){auto point=object();getter(point,"x",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],std::string("x:")+c[1].toString());return c[2];},{trace,name,coordinate}));getter(point,"y",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],std::string("y:")+c[1].toString());return 1;},{trace,name}));return point;};
    auto a=makePoint("a"),b=makePoint("b"),last=makePoint("last"),contour=list({a,b,last}),holes=list();
    getter(last,"equals",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"get:equals",self.sameValue(c[1])}));return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"equals",self.sameValue(c[1]),a[0].sameValue(c[2])}));if(c[3].strictEquals("equalsThrow"))throw JSThrownValue(c[4]);return c[3].strictEquals("keep")?JSValue(false):JSValue("truthy");},{c[0],c[1],c[2],c[3],c[4]});},{trace,last,a,variant,reason}));
    contour.set("pop",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"pop",self.sameValue(c[1])}));return c[2].call(self);},{trace,contour,contour.get("pop")}));
    holes.set("forEach",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"forEach",self.sameValue(c[1]),a[0].get("name"),a[0].get("length")}));if(c[2].strictEquals("forEachThrow"))throw JSThrownValue(c[3]);return {};},{trace,holes,variant,reason}));
    getter(earcut,"triangulate",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],list({"get:earcut",self.sameValue(c[1])}));return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"earcut",self.sameValue(c[1]),a[0].get("0").sameValue(c[2]),a[0].length(),a[1].length()}));if(c[3].strictEquals("earcutThrow"))throw JSThrownValue(c[4]);auto triangles=list({0,1,2,3});triangles.set("slice",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],list({"slice",a[0],a[1]}));return c[1].call(self,a);},{c[0],triangles.get("slice")}));return triangles;},{c[0],c[1],c[2],c[3],c[4]});},{trace,earcut,coordinate,variant,reason}));
    auto out=object();out.set("result",outcome([&](){return call(ctor,"triangulateShape",{contour,holes});},reason));out.set("length",contour.length());out.set("trace",trace);return out;
}
inline JSValue run(const JSValue& request){
    const auto mode=request.get("mode").toString(),variant=request.get("variant").isUndefined()?std::string{}:request.get("variant").toString();
    if(mode=="metadata")return metadata();if(mode=="numeric")return numeric(request);if(mode=="areaOrder")return areaOrder(variant);if(mode=="dynamicArea")return dynamicArea(variant);if(mode=="triangulateOrder")return triangulateOrder(variant);
    if(mode=="nullish"){const auto ctor=nativeShapeUtilsConstructor();auto out=object();out.set("undefined",outcome([&](){return call(ctor,"area");}));out.set("null",outcome([&](){return call(ctor,"area",{nullptr});}));out.set("empty",call(ctor,"area",{object()}));out.set("missingHoles",outcome([&](){return call(ctor,"triangulateShape",{list()});}));return out;}
    throw JSTypeError("Unknown ShapeUtils scenario");
}
inline JSValue request(const char* mode,const char* variant=""){auto r=object();r.set("mode",mode);r.set("variant",variant);return r;}
}
