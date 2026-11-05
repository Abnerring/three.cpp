#include "catch.hpp"
#include "extras/core/Curve.h"
#include "extras/core/CurvePath.h"
#include "extras/core/Path.h"
#include "extras/core/Shape.h"
#include "extras/curves/Curves.h"
#include "loaders/ObjectLoaderShapes.h"
#include "math/Vector3.h"
#include <array>
#include <exception>
#include <thread>
#include <string_view>

namespace {
using Args=std::vector<JSValue>;
struct Entry {
    const char* name;
    JSValue (*constructor)();
    JSValue (*module)();
    JSValue (*binding)(const std::string&);
    const char* parent;
    double arity;
};
const std::array<Entry,14>& entries(){
    static const std::array<Entry,14> value={{
        {"Curve",nativeCurveConstructor,nativeCurveModule,nativeCurveBinding,nullptr,0},
        {"CurvePath",nativeCurvePathConstructor,nativeCurvePathModule,nativeCurvePathBinding,"Curve",0},
        {"Path",nativePathConstructor,nativePathModule,nativePathBinding,"CurvePath",1},
        {"Shape",nativeShapeConstructor,nativeShapeModule,nativeShapeBinding,"Path",1},
        {"EllipseCurve",nativeEllipseCurveConstructor,nativeEllipseCurveModule,nativeEllipseCurveBinding,"Curve",0},
        {"ArcCurve",nativeArcCurveConstructor,nativeArcCurveModule,nativeArcCurveBinding,"EllipseCurve",6},
        {"LineCurve",nativeLineCurveConstructor,nativeLineCurveModule,nativeLineCurveBinding,"Curve",0},
        {"LineCurve3",nativeLineCurve3Constructor,nativeLineCurve3Module,nativeLineCurve3Binding,"Curve",0},
        {"QuadraticBezierCurve",nativeQuadraticBezierCurveConstructor,nativeQuadraticBezierCurveModule,nativeQuadraticBezierCurveBinding,"Curve",0},
        {"QuadraticBezierCurve3",nativeQuadraticBezierCurve3Constructor,nativeQuadraticBezierCurve3Module,nativeQuadraticBezierCurve3Binding,"Curve",0},
        {"CubicBezierCurve",nativeCubicBezierCurveConstructor,nativeCubicBezierCurveModule,nativeCubicBezierCurveBinding,"Curve",0},
        {"CubicBezierCurve3",nativeCubicBezierCurve3Constructor,nativeCubicBezierCurve3Module,nativeCubicBezierCurve3Binding,"Curve",0},
        {"SplineCurve",nativeSplineCurveConstructor,nativeSplineCurveModule,nativeSplineCurveBinding,"Curve",0},
        {"CatmullRomCurve3",nativeCatmullRomCurve3Constructor,nativeCatmullRomCurve3Module,nativeCatmullRomCurve3Binding,"Curve",0}
    }};
    return value;
}
JSValue parentConstructor(const char* name){for(const auto& entry:entries())if(std::string_view(entry.name)==name)return entry.constructor();throw std::logic_error("Unknown curve parent");}
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
struct Restore {
    JSValue self,key;
    std::optional<JSPropertyDescriptor> descriptor;
    Restore(JSValue object,JSValue property):self(object),key(property),descriptor(object.getOwnPropertyDescriptor(property)){}
    ~Restore(){if(descriptor)self.defineProperty(key,*descriptor);else self.erase(key);}
};
void onFreshThread(void(*body)()){
    std::exception_ptr failure;
    std::thread worker([&]{try{body();}catch(...){failure=std::current_exception();}});
    worker.join();if(failure)std::rethrow_exception(failure);
}
}

TEST_CASE("Canonical curve modules share fourteen stable source constructors", "[extras][curves][runtime]"){
    for(const auto& entry:entries()){
        INFO(entry.name);
        const auto ctor=entry.constructor(),module=entry.module(),proto=ctor.get("prototype");
        REQUIRE(ctor.sameValue(entry.constructor()));
        REQUIRE(module.sameValue(entry.module()));
        REQUIRE(module.getPrototype().isNull());
        REQUIRE(module.ownPropertyNames()==std::vector<std::string>{entry.name});
        REQUIRE(module.get(entry.name).sameValue(ctor));
        REQUIRE(entry.binding(entry.name).sameValue(ctor));
        REQUIRE(entry.binding("unexported").isUndefined());
        REQUIRE(ctor.isConstructible());
        REQUIRE(ctor.get("name").strictEquals(entry.name));
        REQUIRE(ctor.get("length").strictEquals(entry.arity));
        REQUIRE(ctor.nativeFunctionSource().starts_with(std::string("class ")+entry.name));
        REQUIRE_THROWS_AS(ctor.call(),JSTypeError);
        const auto prototypeDescriptor=ctor.getOwnPropertyDescriptor("prototype");
        REQUIRE(prototypeDescriptor);
        REQUIRE(prototypeDescriptor->writable==false);
        REQUIRE(prototypeDescriptor->enumerable==false);
        REQUIRE(prototypeDescriptor->configurable==false);
        const auto constructorDescriptor=proto.getOwnPropertyDescriptor("constructor");
        REQUIRE(constructorDescriptor);
        REQUIRE(constructorDescriptor->value->sameValue(ctor));
        REQUIRE(constructorDescriptor->writable==true);
        REQUIRE(constructorDescriptor->enumerable==false);
        REQUIRE(constructorDescriptor->configurable==true);
        if(entry.parent){
            const auto parent=parentConstructor(entry.parent);
            REQUIRE(ctor.getPrototype().sameValue(parent));
            REQUIRE(proto.getPrototype().sameValue(parent.get("prototype")));
        }
        const auto instance=ctor.construct();
        REQUIRE(instance.getPrototype().sameValue(proto));
        REQUIRE(instance.get("type").strictEquals(entry.name));
    }
}

TEST_CASE("Canonical curve imports ignore mutable constructor and facade properties", "[extras][curves][runtime]"){
    for(const auto& entry:entries()){
        INFO(entry.name);
        const auto ctor=entry.constructor();auto module=entry.module(),proto=ctor.get("prototype");
        Restore moduleProperty(module,entry.name),constructorProperty(proto,"constructor");
        JSPropertyDescriptor poisoned;
        poisoned.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("Mutable facade read");});
        poisoned.configurable=true;
        module.defineProperty(entry.name,poisoned);
        proto.defineProperty("constructor",poisoned);
        REQUIRE(entry.constructor().sameValue(ctor));
        REQUIRE(entry.binding(entry.name).sameValue(ctor));
        REQUIRE(entry.module().sameValue(module));
        const auto instance=ctor.construct();
        REQUIRE(instance.get("type").strictEquals(entry.name));
        REQUIRE(instance.getPrototype().sameValue(proto));
    }
}

TEST_CASE("Curves namespace contains precisely the ten upstream exports", "[extras][curves][runtime]"){
    const auto module=nativeCurvesModule();
    const std::vector<std::string> expected={"ArcCurve","CatmullRomCurve3","CubicBezierCurve","CubicBezierCurve3","EllipseCurve","LineCurve","LineCurve3","QuadraticBezierCurve","QuadraticBezierCurve3","SplineCurve"};
    REQUIRE(module.getPrototype().isNull());
    REQUIRE(module.ownPropertyNames()==expected);
    REQUIRE(nativeCurvesModule().sameValue(module));
    REQUIRE(nativeObjectLoaderCurveConstructors().sameValue(module));
    REQUIRE(nativeObjectLoaderShapeConstructor().sameValue(nativeShapeConstructor()));
    for(const auto& name:expected){
        REQUIRE(module.get(name).sameValue(nativeCurvesBinding(name)));
        REQUIRE(module.get(name).sameValue(parentConstructor(name.c_str())));
    }
    for(const auto* name:{"Curve","CurvePath","Path","Shape","unknown"}){
        REQUIRE_FALSE(module.hasOwn(name));
        REQUIRE(nativeCurvesBinding(name).isUndefined());
    }
}

TEST_CASE("Curve modules retain identities during GC and release every retired heap", "[extras][curves][runtime][gc]"){
    onFreshThread([]{
        REQUIRE_FALSE(JSValue::hasActiveHeap());
        std::vector<JSWeakValue> previous;
        for(int round=0;round<3;++round){
            for(const auto& weak:previous)REQUIRE(weak.expired());
            REQUIRE_FALSE(JSValue::hasActiveHeap());
            previous.clear();
            {
                auto keeper=JSValue::object();
                for(const auto& entry:entries()){
                    auto ctor=entry.constructor();
                    REQUIRE_FALSE(ctor.hasOwn("realmMarker"));
                    ctor.set("realmMarker",round);
                    previous.emplace_back(ctor);
                    previous.emplace_back(entry.module());
                    previous.emplace_back(ctor.get("prototype").get("getPoint"));
                }
                previous.emplace_back(nativeCurvesModule());
                auto points=JSValue::array();
                points.set("0",nativeVector3Constructor().construct({1,2,3}));
                points.set("1",nativeVector3Constructor().construct({4,5,6}));
                {
                    const auto unknown=nativeCatmullRomCurve3Constructor().construct({points,false,"unknown"});
                    const auto initial=call(unknown,"getPoint",{0.25});
                    REQUIRE(initial.get("x").strictEquals(0));
                    REQUIRE(initial.get("y").strictEquals(0));
                    REQUIRE(initial.get("z").strictEquals(0));
                    const auto known=nativeCatmullRomCurve3Constructor().construct({points,false,"catmullrom",0.5});
                    REQUIRE(call(known,"getPoint",{0.5}).get("x").strictEquals(2.5));
                }
                JSWeakValue instance;
                {auto value=nativeLineCurveConstructor().construct();instance=JSWeakValue(value);}
                keeper.collectGarbage();
                REQUIRE(instance.expired());
                for(const auto& weak:previous)REQUIRE_FALSE(weak.expired());
                for(const auto& entry:entries())REQUIRE(entry.constructor().get("realmMarker").strictEquals(round));
            }
            for(const auto& weak:previous)REQUIRE(weak.expired());
            REQUIRE_FALSE(JSValue::hasActiveHeap());
        }
    });
}
