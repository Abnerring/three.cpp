#include "loaders/ObjectLoaderShapes.h"
#include "loaders/ObjectLoaderShapesSources.inc"
#include "core/JSModuleObject.h"
#include "core/NativeObject.h"
#include "extras/curves/Curves.h"
#include "math/MathUtils.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <string_view>
#include <tuple>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i) {return i<a.size()?a[i]:JSValue{};}
JSValue defaulted(const Args& a,std::size_t i,const JSValue& value) {const auto v=arg(a,i);return v.isUndefined()?value:v;}
struct Invocation {JSValue object,method;JSValue operator()(const Args& args={})const{return method.call(object,args);}};
Invocation invoke(const JSValue& self,const char* key) {return {self,self.get(key)};}
void own(JSValue self,const char* key,const JSValue& value) {JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;self.defineProperty(key,d);}
JSValue at(const JSValue& value,double i) {return value.get(JSValue(i));}
JSWeakValue& lexical() {static thread_local JSWeakValue weak;return weak;}
JSValue classes() {nativeObjectLoaderCurveConstructors();return *lexical().lock();}
JSValue vectorConstructor(bool spatial) {return spatial?nativeVector3Constructor():nativeVector2Constructor();}

template<class Vector> std::shared_ptr<Vector> vectorValue(const JSValue& value) {
    auto result=std::make_shared<Vector>();result->x=value.get("x").toNumber();result->y=value.get("y").toNumber();
    if constexpr(std::is_same_v<Vector,Vector3>)result->z=value.get("z").toNumber();
    return result;
}
template<class Vector> auto pointsValue(const JSValue& values) {
    auto result=std::make_shared<std::vector<std::shared_ptr<Vector>>>();
    for(double i=0;i<values.get("length").toNumber();++i)result->push_back(vectorValue<Vector>(at(values,i)));
    return result;
}
std::shared_ptr<Curve<Vector2>> curve2(const JSValue& value) {
    const auto type=value.get("type");std::shared_ptr<Curve<Vector2>> result;
    if(type.strictEquals("LineCurve"))result=std::make_shared<LineCurve>(vectorValue<Vector2>(value.get("v1")),vectorValue<Vector2>(value.get("v2")));
    else if(type.strictEquals("QuadraticBezierCurve"))result=std::make_shared<QuadraticBezierCurve>(vectorValue<Vector2>(value.get("v0")),vectorValue<Vector2>(value.get("v1")),vectorValue<Vector2>(value.get("v2")));
    else if(type.strictEquals("CubicBezierCurve"))result=std::make_shared<CubicBezierCurve>(vectorValue<Vector2>(value.get("v0")),vectorValue<Vector2>(value.get("v1")),vectorValue<Vector2>(value.get("v2")),vectorValue<Vector2>(value.get("v3")));
    else if(type.strictEquals("SplineCurve"))result=std::make_shared<SplineCurve>(pointsValue<Vector2>(value.get("points")));
    else if(type.strictEquals("EllipseCurve") || type.strictEquals("ArcCurve")) {
        auto ellipse=std::make_shared<EllipseCurve>();
        ellipse->aX=value.get("aX").toNumber();ellipse->aY=value.get("aY").toNumber();
        ellipse->xRadius=value.get("xRadius").toNumber();ellipse->yRadius=value.get("yRadius").toNumber();
        ellipse->aStartAngle=value.get("aStartAngle").toNumber();ellipse->aEndAngle=value.get("aEndAngle").toNumber();
        ellipse->aClockwise=value.get("aClockwise").toBoolean();ellipse->aRotation=value.get("aRotation").toNumber();result=ellipse;
    } else if(type.strictEquals("Path") || type.strictEquals("Shape") || type.strictEquals("CurvePath")) {
        auto path=std::make_shared<Path>();path->type=type.toString();path->autoClose=value.get("autoClose").toBoolean();
        const auto curves=value.get("curves");
        for(double i=0;i<curves.get("length").toNumber();++i)path->curves->push_back(curve2(at(curves,i)));
        if(!type.strictEquals("CurvePath"))path->currentPoint=vectorValue<Vector2>(value.get("currentPoint"));
        result=path;
    } else throw JSTypeError("ObjectLoader: unsupported planar curve type "+type.toString());
    result->type=type.toString();result->arcLengthDivisions=value.get("arcLengthDivisions").toNumber();return result;
}
JSValue fromJSON(const JSValue& receiver,const Args& a,const Args& c) {
    auto self=receiver;const auto json=arg(a,0), name=c[0];
    if(!name.strictEquals("Curve")) {
        const auto base=c[1].getPrototype();
        base.get("fromJSON",self).call(self,{json});
    }
    if(name.strictEquals("Curve"))self.set("arcLengthDivisions",json.get("arcLengthDivisions"));
    else if(name.strictEquals("CurvePath")) {
        self.set("autoClose",json.get("autoClose"));self.set("curves",JSValue::array());
        const auto length=json.get("curves").get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto curve=at(json.get("curves"),i);
            invoke(self.get("curves"),"push")({invoke(nativeObjectLoaderCurveConstructors().get(curve.get("type")).construct(),"fromJSON")({curve})});
        }
    } else if(name.strictEquals("Path"))invoke(self.get("currentPoint"),"fromArray")({json.get("currentPoint")});
    else if(name.strictEquals("Shape")) {
        self.set("uuid",json.get("uuid"));self.set("holes",JSValue::array());
        const auto length=json.get("holes").get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto hole=at(json.get("holes"),i);
            invoke(self.get("holes"),"push")({invoke(classes().get("Path").construct(),"fromJSON")({hole})});
        }
    } else if(name.strictEquals("EllipseCurve")) {
        for(const auto* key:{"aX","aY","xRadius","yRadius","aStartAngle","aEndAngle","aClockwise","aRotation"})self.set(key,json.get(key));
    } else if(name.strictEquals("SplineCurve") || name.strictEquals("CatmullRomCurve3")) {
        self.set("points",JSValue::array());const auto length=json.get("points").get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto point=at(json.get("points"),i);
            invoke(self.get("points"),"push")({invoke(vectorConstructor(name.strictEquals("CatmullRomCurve3")).construct(),"fromArray")({point})});
        }
        if(name.strictEquals("CatmullRomCurve3"))for(const auto* key:{"closed","curveType","tension"})self.set(key,json.get(key));
    } else {
        const auto count=name.strictEquals("CubicBezierCurve")||name.strictEquals("CubicBezierCurve3")?4:3;
        const auto start=name.strictEquals("LineCurve")||name.strictEquals("LineCurve3")?1:0;
        for(int i=start;i<count;++i) {const auto key="v"+std::to_string(i);invoke(self.get(key),"fromArray")({json.get(key)});}
    }
    return self;
}
JSValue toJSON(const JSValue& self,const Args&,const Args& captures) {
    const auto name=captures[0];JSValue data;
    if(name.strictEquals("Curve")) {
        data=JSValue::object();auto metadata=JSValue::object();
        own(metadata,"version",4.7);own(metadata,"type","Curve");own(metadata,"generator","Curve.toJSON");own(data,"metadata",metadata);
        data.set("arcLengthDivisions",self.get("arcLengthDivisions"));data.set("type",self.get("type"));return data;
    }
    const auto base=captures[1].getPrototype();data=base.get("toJSON",self).call(self);
    if(name.strictEquals("CurvePath")) {
        data.set("autoClose",self.get("autoClose"));data.set("curves",JSValue::array());
        const auto length=self.get("curves").get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto curve=at(self.get("curves"),i);
            invoke(data.get("curves"),"push")({invoke(curve,"toJSON")()});
        }
    } else if(name.strictEquals("Path"))data.set("currentPoint",invoke(self.get("currentPoint"),"toArray")());
    else if(name.strictEquals("Shape")) {
        data.set("uuid",self.get("uuid"));data.set("holes",JSValue::array());
        const auto length=self.get("holes").get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto hole=at(self.get("holes"),i);
            invoke(data.get("holes"),"push")({invoke(hole,"toJSON")()});
        }
    } else if(name.strictEquals("EllipseCurve")) {
        for(const auto* key:{"aX","aY","xRadius","yRadius","aStartAngle","aEndAngle","aClockwise","aRotation"})data.set(key,self.get(key));
    } else if(name.strictEquals("SplineCurve") || name.strictEquals("CatmullRomCurve3")) {
        data.set("points",JSValue::array());const auto length=self.get("points").get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto point=at(self.get("points"),i);
            invoke(data.get("points"),"push")({invoke(point,"toArray")()});
        }
        if(name.strictEquals("CatmullRomCurve3"))for(const auto* key:{"closed","curveType","tension"})data.set(key,self.get(key));
    } else {
        const auto count=name.strictEquals("CubicBezierCurve")||name.strictEquals("CubicBezierCurve3")?4:3;
        const auto start=name.strictEquals("LineCurve")||name.strictEquals("LineCurve3")?1:0;
        for(int i=start;i<count;++i) {const auto key="v"+std::to_string(i);data.set(key,invoke(self.get(key),"toArray")());}
    }
    return data;
}
JSValue construct(const JSValue& newTarget,const Args& a,const Args& c) {
    const auto name=c[0].string();auto self=JSValue::object(newTarget.get("prototype"));
    if(name=="Curve") {self.set("type","Curve");self.set("arcLengthDivisions",200);self.set("needsUpdate",false);self.set("cacheArcLengths",nullptr);return self;}
    Args baseArgs;
    if(name=="ArcCurve")baseArgs={arg(a,0),arg(a,1),arg(a,2),arg(a,2),arg(a,3),arg(a,4),arg(a,5)};
    if(name=="Shape")baseArgs={arg(a,0)};
    self=c[1].get("constructor").getPrototype().construct(baseArgs,newTarget);
    if(name=="CurvePath") {self.set("type",name);self.set("curves",JSValue::array());self.set("autoClose",false);}
    else if(name=="Path") {self.set("type",name);self.set("currentPoint",nativeVector2Constructor().construct());if(arg(a,0).toBoolean())invoke(self,"setFromPoints")({arg(a,0)});}
    else if(name=="Shape") {self.set("uuid",nativeMathUtilsGenerateUUID().call());self.set("type",name);self.set("holes",JSValue::array());}
    else {
        self.set("is"+name,true);self.set("type",name);
        if(name=="EllipseCurve") {
            const double defaults[]={0,0,1,1,0,6.28318530717958647692,0,0};
            const char* keys[]={"aX","aY","xRadius","yRadius","aStartAngle","aEndAngle","aClockwise","aRotation"};
            for(std::size_t i=0;i<8;++i)self.set(keys[i],defaulted(a,i,i==6?JSValue(false):JSValue(defaults[i])));
        } else if(name=="SplineCurve" || name=="CatmullRomCurve3") {
            self.set("points",defaulted(a,0,JSValue::array()));
            if(name=="CatmullRomCurve3") {self.set("closed",defaulted(a,1,false));self.set("curveType",defaulted(a,2,"centripetal"));self.set("tension",defaulted(a,3,0.5));}
        } else if(name!="ArcCurve") {
            const bool spatial=name.ends_with('3');const int start=name=="LineCurve"||name=="LineCurve3"?1:0;
            const int count=name=="CubicBezierCurve"||name=="CubicBezierCurve3"?4:3;
            for(int i=start;i<count;++i) {const auto v=arg(a,i-start);self.set("v"+std::to_string(i),v.isUndefined()?vectorConstructor(spatial).construct():v);}
        }
    }
    return self;
}
JSValue sample(const JSValue& self,const Args& a,const Args& c) {
    const auto method=c[0].string();
    if(method=="getPoints" || method=="getSpacedPoints") {
        const bool path=c[1].strictEquals("CurvePath");
        const auto divisions=defaulted(a,0,path?JSValue(method=="getPoints"?12:40):JSValue(5));
        auto result=JSValue::array();
        if(path && method=="getPoints") {
            JSValue last;const auto curves=self.get("curves");
            for(double i=0;i<curves.get("length").toNumber();++i) {
                const auto curve=at(curves,i);JSValue resolution;
                if(curve.get("isEllipseCurve").toBoolean())resolution=divisions.toNumber()*2;
                else if(curve.get("isLineCurve").toBoolean() || curve.get("isLineCurve3").toBoolean())resolution=1;
                else if(curve.get("isSplineCurve").toBoolean()) {const auto left=divisions.toNumber();resolution=left*curve.get("points").get("length").toNumber();}
                else resolution=divisions;
                const auto points=invoke(curve,"getPoints")({resolution});
                for(double j=0;j<points.get("length").toNumber();++j) {
                    const auto point=at(points,j);if(last.toBoolean() && invoke(last,"equals")({point}).toBoolean())continue;
                    invoke(result,"push")({point});last=point;
                }
            }
            if(self.get("autoClose").toBoolean() && result.get("length").toNumber()>1 && !invoke(at(result,result.get("length").toNumber()-1),"equals")({at(result,0)}).toBoolean())invoke(result,"push")({at(result,0)});
        } else {
            for(double i=0;i<=divisions.toNumber();++i)invoke(result,"push")({invoke(self,path||method=="getPoints"?"getPoint":"getPointAt")({i/divisions.toNumber()})});
            if(path && self.get("autoClose").toBoolean())invoke(result,"push")({at(result,0)});
        }
        return result;
    }
    const auto type=self.get("type");const bool spatial=type.type()==JSValue::Type::String && type.string().ends_with('3');
    const auto output=[&](auto points) {auto result=JSValue::array();for(const auto& p:points)invoke(result,"push")({nativeVector(p)});return result;};
    if(method=="getLength")return spatial?objectLoaderCurve3(self)->getLength():curve2(self)->getLength();
    if(method=="getPoint" || method=="getPointAt" || method=="getTangent" || method=="getTangentAt") {
        const auto t=arg(a,0).toNumber();const auto target=arg(a,1).isUndefined()?vectorConstructor(spatial).construct():arg(a,1);
        if(spatial) {
            const auto curve=objectLoaderCurve3(self);Vector3 point;
            if(method=="getPoint")curve->getPoint(t,point);else if(method=="getPointAt")curve->getPointAt(t,point);else if(method=="getTangent")curve->getTangent(t,point);else curve->getTangentAt(t,point);
            return invoke(target,"set")({point.x,point.y,point.z});
        }
        const auto curve=curve2(self);Vector2 point;
        if(method=="getPoint")curve->getPoint(t,point);else if(method=="getPointAt")curve->getPointAt(t,point);else if(method=="getTangent")curve->getTangent(t,point);else curve->getTangentAt(t,point);
        return invoke(target,"set")({point.x,point.y});
    }
    const auto curve=objectLoaderCurve3(self);const auto frames=curve->computeFrenetFrames(arg(a,0).toNumber(),arg(a,1).toBoolean());
    const auto vectors=[&](const auto& values) {auto result=JSValue::array();for(const auto& v:values)invoke(result,"push")({nativeVector(std::make_shared<Vector3>(v))});return result;};
    auto result=JSValue::object();own(result,"tangents",vectors(frames.tangents));own(result,"normals",vectors(frames.normals));own(result,"binormals",vectors(frames.binormals));return result;
}
} // namespace

std::shared_ptr<Curve<Vector3>> objectLoaderCurve3(const JSValue& value) {
    const auto type=value.get("type");std::shared_ptr<Curve<Vector3>> result;
    if(type.strictEquals("LineCurve3"))result=std::make_shared<LineCurve3>(vectorValue<Vector3>(value.get("v1")),vectorValue<Vector3>(value.get("v2")));
    else if(type.strictEquals("QuadraticBezierCurve3"))result=std::make_shared<QuadraticBezierCurve3>(vectorValue<Vector3>(value.get("v0")),vectorValue<Vector3>(value.get("v1")),vectorValue<Vector3>(value.get("v2")));
    else if(type.strictEquals("CubicBezierCurve3"))result=std::make_shared<CubicBezierCurve3>(vectorValue<Vector3>(value.get("v0")),vectorValue<Vector3>(value.get("v1")),vectorValue<Vector3>(value.get("v2")),vectorValue<Vector3>(value.get("v3")));
    else if(type.strictEquals("CatmullRomCurve3")) {
        const auto points=pointsValue<Vector3>(value.get("points"));const auto closed=value.get("closed").toBoolean();const auto curveType=value.get("curveType").toString();const auto tension=value.get("tension").toNumber();
        result=std::make_shared<CatmullRomCurve3>(points,closed,curveType,tension);
    } else throw JSTypeError("ObjectLoader: unsupported spatial curve type "+type.toString());
    result->arcLengthDivisions=value.get("arcLengthDivisions").toNumber();return result;
}
std::shared_ptr<Shape> objectLoaderShape(const JSValue& value) {
    auto result=std::make_shared<Shape>();result->uuid=value.get("uuid").toString();result->arcLengthDivisions=value.get("arcLengthDivisions").toNumber();
    result->autoClose=value.get("autoClose").toBoolean();result->currentPoint=vectorValue<Vector2>(value.get("currentPoint"));
    const auto curves=value.get("curves");for(double i=0;i<curves.get("length").toNumber();++i)result->curves->push_back(curve2(at(curves,i)));
    const auto holes=value.get("holes");for(double i=0;i<holes.get("length").toNumber();++i)result->holes->push_back(std::dynamic_pointer_cast<Path>(curve2(at(holes,i))));
    return result;
}
JSValue nativeObjectLoaderCurveConstructors() {
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ObjectLoader shape dependencies");
    auto module=JSValue::object(nullptr);
    const auto add=[&](const char* name,const char* parent,unsigned arity,const char* classSource,const char* fromSource) {
        auto holder=JSValue::object(nullptr);auto ctor=JSValue::builtinConstructor(construct,{name,holder},name,arity);holder.set("constructor",ctor);
        if(parent) {ctor.setPrototype(module.get(parent));ctor.get("prototype").setPrototype(module.get(parent).get("prototype"));}
        ctor.setFunctionSourceText(classSource);own(module,name,ctor);
        const std::pair<const char*,const char*> jsonSources[]={
            {"Curve",ObjectLoaderShapesSource::Curve::toJSONSource},{"CurvePath",ObjectLoaderShapesSource::CurvePath::toJSONSource},
            {"Path",ObjectLoaderShapesSource::Path::toJSONSource},{"Shape",ObjectLoaderShapesSource::Shape::toJSONSource},
            {"EllipseCurve",ObjectLoaderShapesSource::EllipseCurve::toJSONSource},{"LineCurve",ObjectLoaderShapesSource::LineCurve::toJSONSource},
            {"LineCurve3",ObjectLoaderShapesSource::LineCurve3::toJSONSource},{"CubicBezierCurve",ObjectLoaderShapesSource::CubicBezierCurve::toJSONSource},
            {"CubicBezierCurve3",ObjectLoaderShapesSource::CubicBezierCurve3::toJSONSource},{"QuadraticBezierCurve",ObjectLoaderShapesSource::QuadraticBezierCurve::toJSONSource},
            {"QuadraticBezierCurve3",ObjectLoaderShapesSource::QuadraticBezierCurve3::toJSONSource},{"SplineCurve",ObjectLoaderShapesSource::SplineCurve::toJSONSource},
            {"CatmullRomCurve3",ObjectLoaderShapesSource::CatmullRomCurve3::toJSONSource}};
        const auto proto=ctor.get("prototype");
        for(const auto& [className,source]:jsonSources)if(std::string_view(name)==className) {
            auto fn=JSValue::function(toJSON,{name,proto},"toJSON",0);fn.setFunctionSourceText(source);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;auto target=proto;target.defineProperty("toJSON",d);break;
        }
        if(fromSource) {auto fn=JSValue::function(fromJSON,{name,proto},"fromJSON",1);fn.setFunctionSourceText(fromSource);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;ctor.get("prototype").defineProperty("fromJSON",d);}
    };
    add("Curve",nullptr,0,ObjectLoaderShapesSource::Curve::classSource,ObjectLoaderShapesSource::Curve::fromJSONSource);
    add("CurvePath","Curve",0,ObjectLoaderShapesSource::CurvePath::classSource,ObjectLoaderShapesSource::CurvePath::fromJSONSource);
    add("Path","CurvePath",1,ObjectLoaderShapesSource::Path::classSource,ObjectLoaderShapesSource::Path::fromJSONSource);
    add("Shape","Path",1,ObjectLoaderShapesSource::Shape::classSource,ObjectLoaderShapesSource::Shape::fromJSONSource);
    add("EllipseCurve","Curve",0,ObjectLoaderShapesSource::EllipseCurve::classSource,ObjectLoaderShapesSource::EllipseCurve::fromJSONSource);
    add("ArcCurve","EllipseCurve",6,ObjectLoaderShapesSource::ArcCurve::classSource,nullptr);
    add("LineCurve","Curve",0,ObjectLoaderShapesSource::LineCurve::classSource,ObjectLoaderShapesSource::LineCurve::fromJSONSource);
    add("LineCurve3","Curve",0,ObjectLoaderShapesSource::LineCurve3::classSource,ObjectLoaderShapesSource::LineCurve3::fromJSONSource);
    add("QuadraticBezierCurve","Curve",0,ObjectLoaderShapesSource::QuadraticBezierCurve::classSource,ObjectLoaderShapesSource::QuadraticBezierCurve::fromJSONSource);
    add("QuadraticBezierCurve3","Curve",0,ObjectLoaderShapesSource::QuadraticBezierCurve3::classSource,ObjectLoaderShapesSource::QuadraticBezierCurve3::fromJSONSource);
    add("CubicBezierCurve","Curve",0,ObjectLoaderShapesSource::CubicBezierCurve::classSource,ObjectLoaderShapesSource::CubicBezierCurve::fromJSONSource);
    add("CubicBezierCurve3","Curve",0,ObjectLoaderShapesSource::CubicBezierCurve3::classSource,ObjectLoaderShapesSource::CubicBezierCurve3::fromJSONSource);
    add("SplineCurve","Curve",0,ObjectLoaderShapesSource::SplineCurve::classSource,ObjectLoaderShapesSource::SplineCurve::fromJSONSource);
    add("CatmullRomCurve3","Curve",0,ObjectLoaderShapesSource::CatmullRomCurve3::classSource,ObjectLoaderShapesSource::CatmullRomCurve3::fromJSONSource);
    const auto installSample=[&](const char* cls,const char* name,unsigned arity,const char* source) {
        auto fn=JSValue::function(sample,{name,cls},name,arity);fn.setFunctionSourceText(source);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;module.get(cls).get("prototype").defineProperty(name,d);
    };
    installSample("Curve","getPoints",0,ObjectLoaderShapesSource::Curve::getPointsSource);installSample("Curve","getSpacedPoints",0,ObjectLoaderShapesSource::Curve::getSpacedPointsSource);
    installSample("Curve","getPoint",2,ObjectLoaderShapesSource::Curve::getPointSource);installSample("Curve","getPointAt",2,ObjectLoaderShapesSource::Curve::getPointAtSource);
    installSample("Curve","getTangent",1,ObjectLoaderShapesSource::Curve::getTangentSource);installSample("Curve","getTangentAt",1,ObjectLoaderShapesSource::Curve::getTangentAtSource);
    installSample("Curve","getLength",0,ObjectLoaderShapesSource::Curve::getLengthSource);installSample("Curve","computeFrenetFrames",2,ObjectLoaderShapesSource::Curve::computeFrenetFramesSource);
    installSample("CurvePath","getPoints",0,ObjectLoaderShapesSource::CurvePath::getPointsSource);installSample("CurvePath","getSpacedPoints",0,ObjectLoaderShapesSource::CurvePath::getSpacedPointsSource);
    auto shapeProto=module.get("Shape").get("prototype");
    const auto pathMethod=[&](const char* name,unsigned arity,JSValue::NativeFunction body,const char* source) {auto fn=JSValue::function(body,{},name,arity);fn.setFunctionSourceText(source);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;module.get("Path").get("prototype").defineProperty(name,d);};
    pathMethod("moveTo",2,+[](const JSValue& self,const Args& a,const Args&)->JSValue {invoke(self.get("currentPoint"),"set")({arg(a,0),arg(a,1)});return self;},ObjectLoaderShapesSource::Path::moveToSource);
    pathMethod("lineTo",2,+[](const JSValue& self,const Args& a,const Args&)->JSValue {
        const auto first=invoke(self.get("currentPoint"),"clone")({});const auto second=nativeVector2Constructor().construct({arg(a,0),arg(a,1)});
        const auto curve=classes().get("LineCurve").construct({first,second});invoke(self.get("curves"),"push")({curve});invoke(self.get("currentPoint"),"set")({arg(a,0),arg(a,1)});return self;
    },ObjectLoaderShapesSource::Path::lineToSource);
    pathMethod("setFromPoints",1,+[](const JSValue& self,const Args& a,const Args&)->JSValue {
        const auto points=arg(a,0);invoke(self,"moveTo")({at(points,0).get("x"),at(points,0).get("y")});const auto length=points.get("length");
        for(double i=1;i<length.toNumber();++i)invoke(self,"lineTo")({at(points,i).get("x"),at(points,i).get("y")});return self;
    },ObjectLoaderShapesSource::Path::setFromPointsSource);
    auto holes=JSValue::function(+[](const JSValue& self,const Args& a,const Args&)->JSValue {
        auto result=JSValue::array();const auto length=self.get("holes").get("length");
        for(double i=0;i<length.toNumber();++i)result.set(JSValue(i),invoke(at(self.get("holes"),i),"getPoints")({arg(a,0)}));return result;
    },{},"getPointsHoles",1);holes.setFunctionSourceText(ObjectLoaderShapesSource::Shape::getPointsHolesSource);
    JSPropertyDescriptor d;d.value=holes;d.writable=d.configurable=true;shapeProto.defineProperty("getPointsHoles",d);
    auto extract=JSValue::function(+[](const JSValue& self,const Args& a,const Args&)->JSValue {auto result=JSValue::object();own(result,"shape",invoke(self,"getPoints")({arg(a,0)}));own(result,"holes",invoke(self,"getPointsHoles")({arg(a,0)}));return result;},{},"extractPoints",1);
    extract.setFunctionSourceText(ObjectLoaderShapesSource::Shape::extractPointsSource);d.value=extract;shapeProto.defineProperty("extractPoints",d);
    module.retainForHeapLifetime();lexical()=JSWeakValue(module);
    auto exports=JSValue::object(nullptr);
    for(const auto* name:{"ArcCurve","CatmullRomCurve3","CubicBezierCurve","CubicBezierCurve3","EllipseCurve","LineCurve","LineCurve3","QuadraticBezierCurve","QuadraticBezierCurve3","SplineCurve"})own(exports,name,module.get(name));
    exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;
}
JSValue nativeObjectLoaderShapeConstructor() {return classes().get("Shape");}
