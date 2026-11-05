#include "geometries/ConeGeometry.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr double pi = std::numbers::pi;
void requireTerminating(double segments) {
    if (segments == std::numeric_limits<double>::infinity())
        throw JSRangeError("Infinite geometry subdivisions do not terminate");
}
double jsonNumber(const JsonValue& data, const char* key, double fallback) {
    return data.contains(key) ? JSValue::fromJSON(data.at(key)).toNumber() : fallback;
}
bool jsonBoolean(const JsonValue& data, const char* key, bool fallback) {
    return data.contains(key) ? JSValue::fromJSON(data.at(key)).toBoolean() : fallback;
}
void finish(BufferGeometry& geometry, const std::vector<double>& indices,
            const std::vector<double>& vertices, const std::vector<double>& normals,
            const std::vector<double>& uvs, bool uvBeforeNormal = false) {
    if (std::any_of(indices.begin(), indices.end(), [](double i) { return i >= 65535; }))
        geometry.setIndex(std::make_shared<Uint32BufferAttribute>(indices, 1));
    else
        geometry.setIndex(std::make_shared<Uint16BufferAttribute>(indices, 1));
    geometry.setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
    if (uvBeforeNormal)
        geometry.setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
    geometry.setAttribute("normal", std::make_shared<Float32BufferAttribute>(normals, 3));
    if (!uvBeforeNormal)
        geometry.setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
}
} // namespace
ConeGeometry::ConeGeometry(double radius, double height, double radialSegments, double heightSegments, bool openEnded, double thetaStart, double thetaLength)
    : CylinderGeometry(0, radius, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength), parameters{radius, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength} {
    type = "ConeGeometry";

}
ConeGeometry& ConeGeometry::copy(const BufferGeometry& source) {
    CylinderGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const ConeGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> ConeGeometry::clone() const {
    auto geometry = std::make_shared<ConeGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue ConeGeometry::toJSON() const {
    if (_transformed) {
        auto result = BufferGeometry::toJSON();
        result["type"] = "BufferGeometry";
        return result;
    }
    // Parameterized geometries return before visiting vertex storage upstream.
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    result["radius"] = parameters.radius;
    result["height"] = parameters.height;
    result["radialSegments"] = parameters.radialSegments;
    result["heightSegments"] = parameters.heightSegments;
    result["openEnded"] = parameters.openEnded;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<ConeGeometry> ConeGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<ConeGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "height", 1), jsonNumber(data, "radialSegments", 32), jsonNumber(data, "heightSegments", 1), jsonBoolean(data, "openEnded", false), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/ConeGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "geometries/CylinderGeometry.h"
namespace ConeGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class ConeGeometry extends CylinderGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new cone geometry.\r\n\t *\r\n\t * @param {number} [radius=1] - Radius of the cone base.\r\n\t * @param {number} [height=1] - Height of the cone.\r\n\t * @param {number} [radialSegments=32] - Number of segmented faces around the circumference of the cone.\r\n\t * @param {number} [heightSegments=1] - Number of rows of faces along the height of the cone.\r\n\t * @param {boolean} [openEnded=false] - Whether the base of the cone is open or capped.\r\n\t * @param {number} [thetaStart=0] - Start angle for first segment, in radians.\r\n\t * @param {number} [thetaLength=Math.PI*2] - The central angle, often called theta, of the circular sector, in radians.\r\n\t * The default value results in a complete cone.\r\n\t */\r\n\tconstructor( radius = 1, height = 1, radialSegments = 32, heightSegments = 1, openEnded = false, thetaStart = 0, thetaLength = Math.PI * 2 ) {\r\n\r\n\t\tsuper( 0, radius, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength );\r\n\r\n\t\tthis.type = 'ConeGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\theight: height,\r\n\t\t\tradialSegments: radialSegments,\r\n\t\t\theightSegments: heightSegments,\r\n\t\t\topenEnded: openEnded,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength\r\n\t\t};\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {ConeGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new ConeGeometry( data.radius, data.height, data.radialSegments, data.heightSegments, data.openEnded, data.thetaStart, data.thetaLength );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( radius = 1, height = 1, radialSegments = 32, heightSegments = 1, openEnded = false, thetaStart = 0, thetaLength = Math.PI * 2 ) {\r\n\r\n\t\tsuper( 0, radius, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength );\r\n\r\n\t\tthis.type = 'ConeGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\theight: height,\r\n\t\t\tradialSegments: radialSegments,\r\n\t\t\theightSegments: heightSegments,\r\n\t\t\topenEnded: openEnded,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength\r\n\t\t};\r\n\r\n\t}";
inline constexpr char source2[]="( data ) {\r\n\r\n\t\treturn new ConeGeometry( data.radius, data.height, data.radialSegments, data.heightSegments, data.openEnded, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
inline constexpr char source3[]="fromJSON( data ) {\r\n\r\n\t\treturn new ConeGeometry( data.radius, data.height, data.radialSegments, data.heightSegments, data.openEnded, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"ConeGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"radius",true);declare(env,"height",true);declare(env,"radialSegments",true);declare(env,"heightSegments",true);declare(env,"openEnded",true);declare(env,"thetaStart",true);declare(env,"thetaLength",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(1.0);initialize(env,"radius",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(1.0);initialize(env,"height",g2);auto g3=arg(args,2);if(g3.isUndefined())g3=JSValue(32.0);initialize(env,"radialSegments",g3);auto g4=arg(args,3);if(g4.isUndefined())g4=JSValue(1.0);initialize(env,"heightSegments",g4);auto g5=arg(args,4);if(g5.isUndefined())g5=JSValue(false);initialize(env,"openEnded",g5);auto g6=arg(args,5);if(g6.isUndefined())g6=JSValue(0.0);initialize(env,"thetaStart",g6);auto g7=arg(args,6);if(g7.isUndefined())g7=([&]() -> JSValue { const auto g8=([&]() -> JSValue { const auto g10=read(env,"Math");const auto g11=JSValue("PI"); return g10.get(g11); }());const auto g9=JSValue(2.0); return binary("*",g8,g9); }());initialize(env,"thetaLength",g7);{auto g12=env;auto env=scope(g12);([&]() -> JSValue { const auto g13=home.getPrototype();Args g15;g15.push_back(JSValue(0.0));g15.push_back(read(env,"radius"));g15.push_back(read(env,"height"));g15.push_back(read(env,"radialSegments"));g15.push_back(read(env,"heightSegments"));g15.push_back(read(env,"openEnded"));g15.push_back(read(env,"thetaStart"));g15.push_back(read(env,"thetaLength"));const auto instance=g13.construct(g15,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g16=read(env,"$this");const auto g17=JSValue("type");const auto g18=JSValue("ConeGeometry"); return setReference(g16,g17,g18); }());
([&]() -> JSValue { auto g19=read(env,"$this");const auto g20=JSValue("parameters");const auto g21=([&]() -> JSValue { auto g22=JSValue::object();const auto g23=JSValue("radius");own(g22,g23,read(env,"radius"));const auto g24=JSValue("height");own(g22,g24,read(env,"height"));const auto g25=JSValue("radialSegments");own(g22,g25,read(env,"radialSegments"));const auto g26=JSValue("heightSegments");own(g22,g26,read(env,"heightSegments"));const auto g27=JSValue("openEnded");own(g22,g27,read(env,"openEnded"));const auto g28=JSValue("thetaStart");own(g22,g28,read(env,"thetaStart"));const auto g29=JSValue("thetaLength");own(g22,g29,read(env,"thetaLength")); return g22; }()); return setReference(g19,g20,g21); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g30=env;auto env=scope(g30);return ([&]() -> JSValue { const auto g31=read(env,"ConeGeometry");Args g33;g33.push_back(([&]() -> JSValue { const auto g34=read(env,"data");const auto g35=JSValue("radius"); return g34.get(g35); }()));g33.push_back(([&]() -> JSValue { const auto g36=read(env,"data");const auto g37=JSValue("height"); return g36.get(g37); }()));g33.push_back(([&]() -> JSValue { const auto g38=read(env,"data");const auto g39=JSValue("radialSegments"); return g38.get(g39); }()));g33.push_back(([&]() -> JSValue { const auto g40=read(env,"data");const auto g41=JSValue("heightSegments"); return g40.get(g41); }()));g33.push_back(([&]() -> JSValue { const auto g42=read(env,"data");const auto g43=JSValue("openEnded"); return g42.get(g43); }()));g33.push_back(([&]() -> JSValue { const auto g44=read(env,"data");const auto g45=JSValue("thetaStart"); return g44.get(g45); }()));g33.push_back(([&]() -> JSValue { const auto g46=read(env,"data");const auto g47=JSValue("thetaLength"); return g46.get(g47); }())); return g31.construct(g33); }());}return {};}
}
JSValue nativeConeGeometryModule(){using namespace ConeGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"ConeGeometry");auto env=scope();JSValue home;bindImport(env,"CylinderGeometry",nativeCylinderGeometryConstructor());declare(env,"ConeGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"ConeGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"CylinderGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"ConeGeometry",constructor);{auto home=constructor;auto function=([&]() -> JSValue { auto g48=JSValue::function(function1,{env,home},"fromJSON",1);g48.setFunctionSourceText(source2); return g48; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"ConeGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeConeGeometryBinding(const std::string& name){return nativeConeGeometryModule().get(name);}
JSValue nativeConeGeometryConstructor(){return nativeConeGeometryBinding("ConeGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
