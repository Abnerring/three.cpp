#include "catch.hpp"
#include "geometries/ExtrudeGeometry.h"
#include "extras/curves/LineCurve3.h"

namespace {
using E = ExtrudeGeometry;
E::OptionsPtr extrudeOptions(JsonValue value) { return std::make_shared<E::Options>(JSValue::fromJSON(value)); }
std::shared_ptr<Shape> extrudeSquare(bool hole = false) {
    auto shape = std::make_shared<Shape>();
    shape->moveTo(0,0).lineTo(8,0).lineTo(8,8).lineTo(0,8).lineTo(0,0);
    if (hole) {
        auto path = std::make_shared<Path>();
        path->moveTo(2,2).lineTo(2,5).lineTo(5,5).lineTo(5,2).lineTo(2,2);
        shape->holes->push_back(path);
    }
    return shape;
}
}
TEST_CASE("Extrude default geometry builds bevel layers, caps and side groups", "[extrude-geometries]") {
    E geometry;
    REQUIRE_FALSE(geometry.index);
    REQUIRE(geometry.getAttribute("position")->getCount() == 180);
    REQUIRE(geometry.getAttribute("normal")->getCount() == 180);
    REQUIRE(geometry.getAttribute("uv")->getCount() == 180);
    REQUIRE(geometry.groups.size() == 2);
    REQUIRE(geometry.groups[0].start == 0); REQUIRE(geometry.groups[0].count == 12);
    REQUIRE(geometry.groups[0].materialIndex == 0);
    REQUIRE(geometry.groups[1].start == 12); REQUIRE(geometry.groups[1].count == 168);
    REQUIRE(geometry.groups[1].materialIndex == 1);
    geometry.computeBoundingBox();
    REQUIRE(geometry.boundingBox->min.x == Approx(-.6)); REQUIRE(geometry.boundingBox->max.x == Approx(.6));
    REQUIRE(geometry.boundingBox->min.z == Approx(-.2)); REQUIRE(geometry.boundingBox->max.z == Approx(1.2));
}
TEST_CASE("Extrude caps preserve holes and surface orientation", "[extrude-geometries]") {
    E geometry(extrudeSquare(true),extrudeOptions(JsonValue::Object{{"bevelEnabled",false},{"depth",3},{"steps",2}}));
    auto positions=geometry.getAttribute("position"), normals=geometry.getAttribute("normal");
    REQUIRE(geometry.groups[0].count == 48); REQUIRE(geometry.groups[1].count == 96);
    double lower=0,upper=0;
    for(std::size_t i=0;i<geometry.groups[0].count;i+=3) {
        Vector3 a,b,c;a.fromBufferAttribute(*positions,i);b.fromBufferAttribute(*positions,i+1);c.fromBufferAttribute(*positions,i+2);
        const auto area=b.sub(a).cross(c.sub(a)).length()/2;
        if(a.z==0){lower+=area;REQUIRE(normals->getZ(i)==-1);}else{upper+=area;REQUIRE(normals->getZ(i)==1);}
    }
    REQUIRE(lower==55);REQUIRE(upper==55);
}
TEST_CASE("Extrude path disables bevel and preserves path identity through clone", "[extrude-geometries]") {
    auto path=std::make_shared<LineCurve3>(std::make_shared<Vector3>(),std::make_shared<Vector3>(0,0,5));
    auto options=extrudeOptions(JsonValue::Object{{"steps",3},{"bevelEnabled",true},{"bevelThickness",10}});
    options->setExtrudePath(path);
    E geometry(extrudeSquare(),options);geometry.computeBoundingBox();
    REQUIRE(geometry.boundingBox->min.z==0);REQUIRE(geometry.boundingBox->max.z==5);
    REQUIRE(geometry.getAttribute("position")->getCount()==84);
    auto clone=std::dynamic_pointer_cast<E>(geometry.clone());
    REQUIRE(clone->parameters.options==options);REQUIRE(clone->parameters.options->extrudePath==path);
    REQUIRE(clone->getAttribute("position")!=geometry.getAttribute("position"));
    auto original=std::get<std::shared_ptr<Shape>>(geometry.parameters.shapes);
    auto restored=E::fromJSON(geometry.toJSON(),{{original->uuid,original}});
    REQUIRE(restored->getAttribute("position")->toJSON()==geometry.getAttribute("position")->toJSON());
    REQUIRE(restored->parameters.options->extrudePath!=path);
    REQUIRE(std::get<std::shared_ptr<E::ShapeList>>(restored->parameters.shapes)->at(0)==original);
}
TEST_CASE("Extrude UV callbacks observe shared growing vertices before attributes exist", "[extrude-geometries]") {
    struct Generator : E::UVGenerator {
        int top=0,side=0;JSValue vertices;
        E::UVs generateTopUV(E& geometry,E::Vertices array,double a,double b,double c) override {
            REQUIRE_FALSE(geometry.hasAttribute("position"));
            if(vertices.isUndefined())vertices=array;else REQUIRE(vertices.strictEquals(array));
            ++top;return E::worldUVGenerator()->generateTopUV(geometry,array,a,b,c);
        }
        E::UVs generateSideWallUV(E& geometry,E::Vertices array,double a,double b,double c,double d) override {
            REQUIRE(vertices.strictEquals(array));REQUIRE(geometry.groups.size()==1);
            ++side;return E::worldUVGenerator()->generateSideWallUV(geometry,array,a,b,c,d);
        }
    };
    auto generator=std::make_shared<Generator>();
    auto options=extrudeOptions(JsonValue::Object{{"bevelEnabled",false}});options->setUVGenerator(generator);
    E geometry(E::defaultShape(),options);
    REQUIRE(generator->top==4);REQUIRE(generator->side==4);
    REQUIRE(generator->vertices.length()==geometry.getAttribute("position")->getCount()*3);
    generator->vertices.set("0",999);
    REQUIRE(geometry.getAttribute("position")->getX(0)!=999);
    REQUIRE(geometry.toJSON().at("options").at("UVGenerator").isObject());
    const auto persisted=geometry.toJSON().copyJSON();
    REQUIRE_THROWS_AS(E::fromJSON(persisted,{{std::get<std::shared_ptr<Shape>>(geometry.parameters.shapes)->uuid,
        std::get<std::shared_ptr<Shape>>(geometry.parameters.shapes)}}),JSTypeError);
}
TEST_CASE("Extrude serialization keeps original options and skips invalid unused attributes", "[extrude-geometries]") {
    auto options=extrudeOptions(JsonValue::Object{{"depth",2},{"bevelEnabled",false}});
    auto shapes=std::make_shared<E::ShapeList>(E::ShapeList{extrudeSquare()});
    E geometry(shapes,options);auto clone=std::dynamic_pointer_cast<E>(geometry.clone());
    options->set("depth",7);shapes->push_back(extrudeSquare());
    REQUIRE(clone->parameters.options==options);REQUIRE(std::get<std::shared_ptr<E::ShapeList>>(clone->parameters.shapes)==shapes);
    REQUIRE(geometry.toJSON().at("options").at("depth").get<double>()==7);
    REQUIRE(geometry.toJSON().at("shapes").size()==2);REQUIRE(geometry.groups.size()==2);
    geometry.setAttribute("position",nullptr);geometry.morphAttributes["position"].push_back(nullptr);
    REQUIRE_FALSE(geometry.toJSON().contains("data"));
    clone->translate(1,2,3);REQUIRE(clone->toJSON().at("type").get<std::string>()=="BufferGeometry");
    REQUIRE(clone->toJSON().contains("data"));REQUIRE(clone->toJSON().contains("options"));
    auto nullPath=extrudeOptions(JsonValue::Object{});nullPath->setExtrudePath(nullptr);
    E withNullPath(E::defaultShape(),nullPath);REQUIRE_THROWS_AS(withNullPath.toJSON(),JSTypeError);
}
TEST_CASE("Extrude merges adjacent duplicate vertices and permits empty shapes", "[extrude-geometries]") {
    auto standard=extrudeSquare();auto duplicate=std::make_shared<Shape>();
    duplicate->moveTo(0,0).lineTo(8,0).lineTo(8,1e-12).lineTo(8,8).lineTo(0,8).lineTo(0,0);
    E a(standard),b(duplicate);
    const auto first=a.getAttribute("position"),second=b.getAttribute("position");
    REQUIRE(first->getCount()==second->getCount());
    for(double i=0;i<first->getCount();++i) {
        REQUIRE(first->getX(i)==Approx(second->getX(i)).margin(1e-9));
        REQUIRE(first->getY(i)==Approx(second->getY(i)).margin(1e-9));
        REQUIRE(first->getZ(i)==Approx(second->getZ(i)).margin(1e-9));
    }
    E empty(std::make_shared<Shape>());REQUIRE(empty.getAttribute("position")->getCount()==0);REQUIRE(empty.groups.size()==2);
    E emptyArray(std::make_shared<E::ShapeList>());REQUIRE(emptyArray.groups.empty());REQUIRE(emptyArray.getAttribute("normal")->getCount()==0);
}
TEST_CASE("Extrude reads inherited options and preserves Vector2 undefined defaults", "[extrude-geometries]") {
    auto prototype=JSValue::object();prototype.set("depth",3);prototype.set("bevelEnabled",false);
    auto options=std::make_shared<E::Options>(JSValue::object(prototype));
    E geometry(E::defaultShape(),options);geometry.computeBoundingBox();
    REQUIRE(geometry.boundingBox->max.z==3);REQUIRE(geometry.getAttribute("position")->getCount()==36);
    REQUIRE(geometry.toJSON().at("options").size()==0);
    auto vertices=JSValue::array(9);vertices.set("1",NAN);vertices.set("3",4);vertices.set("4",5);
    const auto uvs=E::worldUVGenerator()->generateTopUV(geometry,vertices,0,1,2);
    REQUIRE(uvs[0]->x==0);REQUIRE(std::isnan(uvs[0]->y));
    REQUIRE(uvs[1]->x==4);REQUIRE(uvs[1]->y==5);
    REQUIRE(uvs[2]->x==0);REQUIRE(uvs[2]->y==0);
}
TEST_CASE("Extrude retains fractional groups after a UV callback pops a coordinate", "[extrude-geometries]") {
    struct PopUV : E::UVGenerator {
        bool popped=false;
        E::UVs generateTopUV(E& g,E::Vertices v,double a,double b,double c) override {
            if(!popped){popped=true;v.set("length",v.length()-1);}
            return E::worldUVGenerator()->generateTopUV(g,v,a,b,c);
        }
        E::UVs generateSideWallUV(E& g,E::Vertices v,double a,double b,double c,double d) override {
            return E::worldUVGenerator()->generateSideWallUV(g,v,a,b,c,d);
        }
    };
    auto options=extrudeOptions(JsonValue::Object{{"bevelEnabled",false}});options->setUVGenerator(std::make_shared<PopUV>());
    E geometry(extrudeSquare(),options);
    REQUIRE(geometry.groups[0].count==35.0/3);
    REQUIRE(geometry.groups[1].start==35.0/3);
    REQUIRE(geometry.groups[1].count==24);
    REQUIRE(geometry.getAttribute("position")->getCount()==107.0/3);
}
TEST_CASE("Extrude dispatches a derived curve Frenet frame override", "[extrude-geometries]") {
    struct FramePath : LineCurve3 {
        int calls=0;double segments=-1;bool wasClosed=true;
        FramePath():LineCurve3(std::make_shared<Vector3>(),std::make_shared<Vector3>(0,0,5)){}
        FrenetFrames computeFrenetFrames(double n,bool closed=false) override {
            ++calls;segments=n;wasClosed=closed;auto result=LineCurve3::computeFrenetFrames(n,closed);
            for(auto& normal:result.normals)normal.set(1,0,0);
            for(auto& binormal:result.binormals)binormal.set(0,1,0);
            return result;
        }
    };
    auto path=std::make_shared<FramePath>();auto options=extrudeOptions(JsonValue::Object{{"steps",2}});options->setExtrudePath(path);
    E geometry(extrudeSquare(),options);geometry.computeBoundingBox();
    REQUIRE(path->calls==1);REQUIRE(path->segments==2);REQUIRE_FALSE(path->wasClosed);
    REQUIRE(geometry.boundingBox->min.x==0);REQUIRE(geometry.boundingBox->max.x==8);
    REQUIRE(geometry.boundingBox->min.y==0);REQUIRE(geometry.boundingBox->max.y==8);
    REQUIRE(geometry.boundingBox->max.z==5);
}
TEST_CASE("Extrude world UV generators snapshot vertex getters in upstream order", "[extrude-geometries]") {
    auto trace=JSValue::array();auto vertices=JSValue::array(12);
    for(int i=0;i<12;++i){JSPropertyDescriptor descriptor;descriptor.enumerable=true;
        descriptor.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures){
            auto log=captures[0];log.set(std::to_string(log.length()),captures[1]);return captures[1];
        },{trace,i});vertices.defineProperty(std::to_string(i),descriptor);}
    E geometry(std::make_shared<E::ShapeList>());
    const auto top=E::worldUVGenerator()->generateTopUV(geometry,vertices,0,1,2);
    REQUIRE(trace.toJSON().value()==JsonValue(JsonValue::Array{0,1,3,4,6,7}));
    REQUIRE(top[2]->x==6);REQUIRE(top[2]->y==7);
    trace.set("length",0);
    const auto side=E::worldUVGenerator()->generateSideWallUV(geometry,vertices,0,1,2,3);
    REQUIRE(trace.toJSON().value()==JsonValue(JsonValue::Array{0,1,2,3,4,5,6,7,8,9,10,11}));
    REQUIRE(side[3]->x==10);REQUIRE(side[3]->y==-10);
}
TEST_CASE("Extrude defers subdivision conversion until after extracting the shape", "[extrude-geometries]") {
    struct TracedShape : Shape {
        JSValue trace;
        explicit TracedShape(JSValue trace):trace(trace){}
        ExtractedPoints extractPoints(double divisions) override {
            trace.set(std::to_string(trace.length()),"extract");return Shape::extractPoints(divisions);
        }
    };
    for(const auto name:{"steps","bevelSegments"}){
        auto trace=JSValue::array();auto value=JSValue::object();
        value.set("valueOf",JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures)->JSValue{
            auto log=captures[0];log.set(std::to_string(log.length()),captures[1]);throw JSTypeError("conversion");
        },{trace,name}));
        auto options=std::make_shared<E::Options>();options->set(name,value);
        REQUIRE_THROWS_AS(E(std::make_shared<TracedShape>(trace),options),JSTypeError);
        REQUIRE(trace.toJSON().value()==JsonValue(JsonValue::Array{"extract",name}));
    }
}
TEST_CASE("Extrude options assignment rechecks descriptors after earlier getters", "[extrude-geometries]") {
    auto trace=JSValue::array();auto prototype=JSValue::object();JSPropertyDescriptor inherited;
    inherited.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&)->JSValue{
        throw JSTypeError("deleted own property must not read its prototype");
    });prototype.defineProperty("b",inherited);
    auto values=JSValue::object(prototype);JSPropertyDescriptor first;first.enumerable=true;
    first.getter=JSValue::function([](const JSValue& receiver,const std::vector<JSValue>&,const std::vector<JSValue>& captures)->JSValue{
        auto values=receiver,log=captures[0];log.set(std::to_string(log.length()),"a");values.erase("b");
        JSPropertyDescriptor visible;visible.enumerable=true;values.defineProperty("c",visible);return 1;
    },{trace});values.defineProperty("a",first);
    JSPropertyDescriptor removed;removed.value=2;removed.enumerable=true;removed.configurable=true;values.defineProperty("b",removed);
    JSPropertyDescriptor hidden;hidden.value=3;hidden.enumerable=false;hidden.configurable=true;values.defineProperty("c",hidden);
    E geometry(std::make_shared<E::ShapeList>(),std::make_shared<E::Options>(values));
    REQUIRE(geometry.toJSON().at("options")==JsonValue(JsonValue::Object{{"a",1},{"c",3}}));
    REQUIRE(trace.toJSON().value()==JsonValue(JsonValue::Array{"a"}));
}
