#include "geometries/ExtrudeGeometry.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/QuadraticBezierCurve3.h"
#include "extras/curves/CatmullRomCurve3.h"
#include <iostream>
namespace {
using E = ExtrudeGeometry;
JsonValue normalize(const JsonValue& value) {
    return std::visit([](const auto& raw) -> JsonValue {
        using T = std::decay_t<decltype(raw)>;
        if constexpr (std::is_same_v<T, double>) {
            if (std::isnan(raw)) return "NaN";
            if (std::isinf(raw)) return raw < 0 ? "-Infinity" : "Infinity";
            if (raw == 0 && std::signbit(raw)) return "-0";
            return raw;
        } else if constexpr (std::is_same_v<T, JsonValue::Array>) {
            JsonValue::Array result; for (const auto& v : raw) result.push_back(normalize(v)); return result;
        } else if constexpr (std::is_same_v<T, JsonValue::Object>) {
            JsonValue::Object result;
            for (const auto& [key, v] : raw) result.emplace(key, key == "uuid" ? JsonValue("uuid") : normalize(v));
            return result;
        } else return raw;
    }, value.storage());
}
JsonValue snapshot(const E& geometry) {
    JsonValue attributes = JsonValue::Object{};
    for (const auto& [name, attribute] : geometry.attributes) attributes[name] = attribute->toJSON();
    JsonValue::Array groups;
    for (const auto& group : geometry.groups) groups.emplace_back(JsonValue::Array{group.start, group.count, group.materialIndex});
    return JsonValue::Object{{"type", geometry.type}, {"attributes", attributes}, {"groups", groups},
        {"index", geometry.index ? geometry.index->toJSON() : JsonValue{}}, {"json", geometry.toJSON().copyJSON()}};
}
std::shared_ptr<Shape> shape(const std::vector<Vector2>& coordinates, const std::string& uuid) {
    auto points = std::make_shared<Shape::Points>();
    for (const auto& v : coordinates) points->push_back(std::make_shared<Vector2>(v));
    auto result = points->empty() ? std::make_shared<Shape>() : std::make_shared<Shape>(points); result->uuid = uuid; return result;
}
std::shared_ptr<Shape> rectangle() { return shape({{0,0},{0,4},{6,4},{6,0}}, "rectangle"); }
std::shared_ptr<Shape> holed(bool clockwise) {
    auto s = shape(clockwise ? std::vector<Vector2>{{0,0},{0,8},{8,8},{8,0}} : std::vector<Vector2>{{0,0},{8,0},{8,8},{0,8}}, "holed");
    auto points = std::make_shared<Shape::Points>();
    for (const auto& p : {Vector2(2,2),Vector2(2,5),Vector2(5,5),Vector2(5,2)}) points->push_back(std::make_shared<Vector2>(p));
    s->holes->push_back(std::make_shared<Path>(points)); return s;
}
E::OptionsPtr options(const JsonValue& value) { return std::make_shared<E::Options>(JSValue::fromJSON(value)); }
class LoggedUV : public E::UVGenerator {
  public:
    std::vector<JsonValue> calls;
    E::OptionsPtr options;
    std::shared_ptr<E::ShapeList> shapes;
    E::UVs generateTopUV(E& g, E::Vertices v, double a, double b, double c) override {
        calls.emplace_back(JsonValue::Array{"top", g.groups.size(), v.length(), a,b,c});
        if (calls.size() == 1) { v.set("0",v.get("0").toNumber()+.25); options->set("depth", 3); shapes->push_back(rectangle()); }
        auto result = E::worldUVGenerator()->generateTopUV(g,v,a,b,c);
        for (auto& uv : result) { uv->x += 10; uv->y -= 4; } return result;
    }
    E::UVs generateSideWallUV(E& g, E::Vertices v, double a, double b, double c, double d) override {
        calls.emplace_back(JsonValue::Array{"side",g.groups.size(),v.length(),a,b,c,d});
        auto result = E::worldUVGenerator()->generateSideWallUV(g,v,a,b,c,d);
        for (auto& uv : result) uv->multiplyScalar(.5); return result;
    }
};
class PopUV : public E::UVGenerator {
    bool popped=false;
  public:
    E::UVs generateTopUV(E& g,E::Vertices v,double a,double b,double c) override {
        if(!popped){popped=true;v.set("length",v.length()-1);}
        return E::worldUVGenerator()->generateTopUV(g,v,a,b,c);
    }
    E::UVs generateSideWallUV(E& g,E::Vertices v,double a,double b,double c,double d) override {
        return E::worldUVGenerator()->generateSideWallUV(g,v,a,b,c,d);
    }
};
class FramePath : public LineCurve3 {
  public:
    JsonValue::Array calls;
    FramePath():LineCurve3(std::make_shared<Vector3>(),std::make_shared<Vector3>(0,0,5)){}
    FrenetFrames computeFrenetFrames(double segments,bool closed=false) override {
        calls.emplace_back(JsonValue::Array{segments,closed});auto result=LineCurve3::computeFrenetFrames(segments,closed);
        for(auto& normal:result.normals)normal.set(1,0,0);
        for(auto& binormal:result.binormals)binormal.set(0,1,0);
        return result;
    }
};
JsonValue uvValues(const E::UVs& values){JsonValue::Array output;for(const auto& v:values)output.emplace_back(JsonValue::Array{v->x,v->y});return output;}
JsonValue regressionCases(){
    JsonValue result=JsonValue::Object{};
    auto o=options(JsonValue::Object{{"bevelEnabled",false}});o->setUVGenerator(std::make_shared<PopUV>());
    result["poppedCoordinate"]=snapshot(E(rectangle(),o));
    auto path=std::make_shared<FramePath>();o=options(JsonValue::Object{{"steps",2}});o->setExtrudePath(path);
    result["customFrames"]=snapshot(E(rectangle(),o));result["frameCalls"]=path->calls;
    auto trace=JSValue::array(),vertices=JSValue::array(12);
    for(int i=0;i<12;++i){JSPropertyDescriptor descriptor;descriptor.enumerable=true;
        descriptor.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures){
            auto log=captures[0];log.set(std::to_string(log.length()),captures[1]);return captures[1];
        },{trace,i});vertices.defineProperty(std::to_string(i),descriptor);}
    E empty(std::make_shared<E::ShapeList>());
    result["topUV"]=uvValues(E::worldUVGenerator()->generateTopUV(empty,vertices,0,1,2));result["topGetters"]=trace.toJSON().value();trace.set("length",0);
    result["sideUV"]=uvValues(E::worldUVGenerator()->generateSideWallUV(empty,vertices,0,1,2,3));result["sideGetters"]=trace.toJSON().value();
    struct TracedShape : Shape {
        JSValue trace;
        explicit TracedShape(JSValue trace):trace(trace){}
        ExtractedPoints extractPoints(double divisions) override {
            trace.set(std::to_string(trace.length()),"extract");return Shape::extractPoints(divisions);
        }
    };
    JsonValue::Array conversions;
    for(const auto name:{"steps","bevelSegments"}){
        trace=JSValue::array();auto value=JSValue::object();
        value.set("valueOf",JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures)->JSValue{
            auto log=captures[0];log.set(std::to_string(log.length()),captures[1]);throw JSTypeError("conversion");
        },{trace,name}));
        o=std::make_shared<E::Options>();o->set(name,value);bool threw=false;
        try{E geometry(std::make_shared<TracedShape>(trace),o);}catch(const JSTypeError&){threw=true;}
        conversions.emplace_back(JsonValue::Array{trace.toJSON().value(),threw});
    }
    result["conversionOrder"]=conversions;
    trace=JSValue::array();auto prototype=JSValue::object();JSPropertyDescriptor inherited;
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
    E assigned(std::make_shared<E::ShapeList>(),std::make_shared<E::Options>(values));
    result["assignedOptions"]=assigned.toJSON().at("options");result["assignmentGetters"]=trace.toJSON().value();
    return result;
}
}
int main() {
    try {
        JsonValue result = JsonValue::Object{};
        E defaults; std::get<std::shared_ptr<Shape>>(defaults.parameters.shapes)->uuid = "default";
        result["default"] = snapshot(defaults);
        JsonValue::Array variants;
        for (const auto& data : std::vector<JsonValue>{JsonValue::Object{},
            JsonValue::Object{{"bevelEnabled",false}}, JsonValue::Object{{"depth",3},{"steps",3}},
            JsonValue::Object{{"bevelThickness",.7}}, JsonValue::Object{{"bevelOffset",-.25},{"bevelSize",.4},{"bevelSegments",2}},
            JsonValue::Object{{"steps",0},{"bevelEnabled",false}}, JsonValue::Object{{"steps",2.5},{"bevelEnabled",false}},
            JsonValue::Object{{"bevelSegments",0}}, JsonValue::Object{{"bevelSegments",1.5}},
            JsonValue::Object{{"bevelSegments",-1}}, JsonValue::Object{{"bevelSegments",NAN}},
            JsonValue::Object{{"bevelSegments",nullptr}}, JsonValue::Object{{"depth",nullptr},{"steps",1}},
            JsonValue::Object{{"steps","2"},{"bevelEnabled",false}},
            JsonValue::Object{{"depth",-2},{"bevelThickness",-.2},{"bevelSize",-.3}},
            JsonValue::Object{{"curveSegments",0},{"bevelEnabled",false}}}) {variants.push_back(snapshot(E(rectangle(),options(data))));}
        result["variants"] = std::move(variants);
        JsonValue::Array contours;
        for (bool clockwise : {false,true}) contours.push_back(snapshot(E(holed(clockwise))));
        for (const auto& coordinates : std::vector<std::vector<Vector2>>{
            {{0,0},{0,4},{3,2},{6,4},{6,0}},
            {{0,0},{0,2},{0,4},{6,4},{6,0}},
            {{0,0},{0,4},{6,4},{6,0},{6,1e-12},{0,0}},
            {{0,0},{0,4},{2,4},{0,4},{6,4},{6,0}}, {}, {{1,1}}, {{0,0},{1,1}}})
            contours.push_back(snapshot(E(shape(coordinates,"special"))));
        auto curved = std::make_shared<Shape>(); curved->uuid="curve";
        curved->moveTo(0,0).quadraticCurveTo(3,7,6,0).lineTo(0,0);
        contours.push_back(snapshot(E(curved, options(JsonValue::Object{{"curveSegments",5},{"bevelSegments",2}}))));
        result["contours"] = std::move(contours);
        auto points=std::make_shared<CatmullRomCurve3::Points>();
        for(const auto& p:{Vector3(0,0,0),Vector3(3,2,1),Vector3(4,-1,3),Vector3(0,0,5)})points->push_back(std::make_shared<Vector3>(p));
        JsonValue::Array paths;
        for(const auto& path:std::vector<std::shared_ptr<Curve<Vector3>>>{
            std::make_shared<LineCurve3>(std::make_shared<Vector3>(0,0,0),std::make_shared<Vector3>(1,2,5)),
            std::make_shared<QuadraticBezierCurve3>(std::make_shared<Vector3>(0,0,0),std::make_shared<Vector3>(4,1,3),std::make_shared<Vector3>(1,5,6)),
            std::make_shared<CatmullRomCurve3>(points,false),std::make_shared<CatmullRomCurve3>(points,true)}) {
            auto o=options(JsonValue::Object{{"steps",4},{"bevelEnabled",true},{"bevelSegments",2}});o->setExtrudePath(path);
            E g(rectangle(),o);paths.push_back(snapshot(g));paths.push_back(snapshot(*E::fromJSON(g.toJSON(),{{"rectangle",std::get<std::shared_ptr<Shape>>(g.parameters.shapes)}})));
        }
        result["paths"] = std::move(paths);
        auto list=std::make_shared<E::ShapeList>(E::ShapeList{rectangle(),holed(false)});
        auto o=options(JsonValue::Object{{"steps",1},{"depth",1},{"bevelEnabled",false}});
        auto uv=std::make_shared<LoggedUV>();uv->options=o;uv->shapes=list;o->setUVGenerator(uv);
        E custom(list,o);result["customUV"]=snapshot(custom);result["uvCalls"]=JsonValue::Array(uv->calls.begin(),uv->calls.end());
        auto clone=std::dynamic_pointer_cast<E>(custom.clone());
        result["identities"]=JsonValue::Array{clone->parameters.options==o,std::get<std::shared_ptr<E::ShapeList>>(clone->parameters.shapes)==list,
            clone->getAttribute("position")!=custom.getAttribute("position"),list->size(),custom.groups.size()};
        result["clone"]=snapshot(*clone);custom.translate(1,2,3);result["transformed"]=snapshot(custom);
        E early;std::get<std::shared_ptr<Shape>>(early.parameters.shapes)->uuid="default";early.setAttribute("position",nullptr);
        early.morphAttributes["position"].push_back(nullptr);result["earlyJSON"]=early.toJSON();
        result["emptyArray"]=snapshot(E(std::make_shared<E::ShapeList>()));
        auto prototype=JSValue::object();prototype.set("depth",3);prototype.set("bevelEnabled",false);
        result["inheritedOptions"]=snapshot(E(rectangle(),std::make_shared<E::Options>(JSValue::object(prototype))));
        result["regressions"]=regressionCases();
        std::cout<<normalize(result).dump()<<'\n';
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
