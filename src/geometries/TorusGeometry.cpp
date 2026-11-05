#include "geometries/TorusGeometry.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
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
TorusGeometry::TorusGeometry(double radius, double tube, double radialSegments, double tubularSegments, double arc, double thetaStart, double thetaLength)
    : parameters{radius, tube, radialSegments, tubularSegments, arc, thetaStart, thetaLength} {
    type = "TorusGeometry";
    radialSegments = std::floor(radialSegments);
    tubularSegments = std::floor(tubularSegments);
    requireTerminating(radialSegments);
    requireTerminating(tubularSegments);
    std::vector<double> indices, vertices, normals, uvs;
    Vector3 center, vertex, normal;
    for (double j = 0; j <= radialSegments; ++j) {
        const double v = thetaStart + (j / radialSegments) * thetaLength;
        for (double i = 0; i <= tubularSegments; ++i) {
            const double u = i / tubularSegments * arc;
            vertex.set((radius + tube * JSNumberMath::cos(v)) * JSNumberMath::cos(u),
                       (radius + tube * JSNumberMath::cos(v)) * JSNumberMath::sin(u), tube * JSNumberMath::sin(v));
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            center.x = radius * JSNumberMath::cos(u);
            center.y = radius * JSNumberMath::sin(u);
            normal.subVectors(vertex, center).normalize();
            normals.insert(normals.end(), {normal.x, normal.y, normal.z});
            uvs.insert(uvs.end(), {i / tubularSegments, j / radialSegments});
        }
    }
    for (double j = 1; j <= radialSegments; ++j) {
        for (double i = 1; i <= tubularSegments; ++i) {
            const double a = (tubularSegments + 1) * j + i - 1,
                         b = (tubularSegments + 1) * (j - 1) + i - 1,
                         c = (tubularSegments + 1) * (j - 1) + i,
                         d = (tubularSegments + 1) * j + i;
            indices.insert(indices.end(), {a, b, d, b, c, d});
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
TorusGeometry& TorusGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const TorusGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> TorusGeometry::clone() const {
    auto geometry = std::make_shared<TorusGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue TorusGeometry::toJSON() const {
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
    result["tube"] = parameters.tube;
    result["radialSegments"] = parameters.radialSegments;
    result["tubularSegments"] = parameters.tubularSegments;
    result["arc"] = parameters.arc;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<TorusGeometry> TorusGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<TorusGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "tube", 0.4), jsonNumber(data, "radialSegments", 12), jsonNumber(data, "tubularSegments", 48), jsonNumber(data, "arc", std::numbers::pi * 2), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/TorusGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "math/Vector3.h"
namespace TorusGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class TorusGeometry extends BufferGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new torus geometry.\r\n\t *\r\n\t * @param {number} [radius=1] - Radius of the torus, from the center of the torus to the center of the tube.\r\n\t * @param {number} [tube=0.4] - Radius of the tube. Must be smaller than `radius`.\r\n\t * @param {number} [radialSegments=12] - The number of radial segments.\r\n\t * @param {number} [tubularSegments=48] - The number of tubular segments.\r\n\t * @param {number} [arc=Math.PI*2] - Central angle in radians.\r\n\t * @param {number} [thetaStart=0] - Start of the tubular sweep in radians.\r\n\t * @param {number} [thetaLength=Math.PI*2] - Length of the tubular sweep in radians.\r\n\t */\r\n\tconstructor( radius = 1, tube = 0.4, radialSegments = 12, tubularSegments = 48, arc = Math.PI * 2, thetaStart = 0, thetaLength = Math.PI * 2 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'TorusGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\ttube: tube,\r\n\t\t\tradialSegments: radialSegments,\r\n\t\t\ttubularSegments: tubularSegments,\r\n\t\t\tarc: arc,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength,\r\n\t\t};\r\n\r\n\t\tradialSegments = Math.floor( radialSegments );\r\n\t\ttubularSegments = Math.floor( tubularSegments );\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// helper variables\r\n\r\n\t\tconst center = new Vector3();\r\n\t\tconst vertex = new Vector3();\r\n\t\tconst normal = new Vector3();\r\n\r\n\t\t// generate vertices, normals and uvs\r\n\r\n\t\tfor ( let j = 0; j <= radialSegments; j ++ ) {\r\n\r\n\t\t\tconst v = thetaStart + ( j / radialSegments ) * thetaLength;\r\n\r\n\t\t\tfor ( let i = 0; i <= tubularSegments; i ++ ) {\r\n\r\n\t\t\t\tconst u = i / tubularSegments * arc;\r\n\r\n\t\t\t\t// vertex\r\n\r\n\t\t\t\tvertex.x = ( radius + tube * Math.cos( v ) ) * Math.cos( u );\r\n\t\t\t\tvertex.y = ( radius + tube * Math.cos( v ) ) * Math.sin( u );\r\n\t\t\t\tvertex.z = tube * Math.sin( v );\r\n\r\n\t\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t\t// normal\r\n\r\n\t\t\t\tcenter.x = radius * Math.cos( u );\r\n\t\t\t\tcenter.y = radius * Math.sin( u );\r\n\t\t\t\tnormal.subVectors( vertex, center ).normalize();\r\n\r\n\t\t\t\tnormals.push( normal.x, normal.y, normal.z );\r\n\r\n\t\t\t\t// uv\r\n\r\n\t\t\t\tuvs.push( i / tubularSegments );\r\n\t\t\t\tuvs.push( j / radialSegments );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// generate indices\r\n\r\n\t\tfor ( let j = 1; j <= radialSegments; j ++ ) {\r\n\r\n\t\t\tfor ( let i = 1; i <= tubularSegments; i ++ ) {\r\n\r\n\t\t\t\t// indices\r\n\r\n\t\t\t\tconst a = ( tubularSegments + 1 ) * j + i - 1;\r\n\t\t\t\tconst b = ( tubularSegments + 1 ) * ( j - 1 ) + i - 1;\r\n\t\t\t\tconst c = ( tubularSegments + 1 ) * ( j - 1 ) + i;\r\n\t\t\t\tconst d = ( tubularSegments + 1 ) * j + i;\r\n\r\n\t\t\t\t// faces\r\n\r\n\t\t\t\tindices.push( a, b, d );\r\n\t\t\t\tindices.push( b, c, d );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}\r\n\r\n\tcopy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {TorusGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new TorusGeometry( data.radius, data.tube, data.radialSegments, data.tubularSegments, data.arc, data.thetaStart, data.thetaLength );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( radius = 1, tube = 0.4, radialSegments = 12, tubularSegments = 48, arc = Math.PI * 2, thetaStart = 0, thetaLength = Math.PI * 2 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'TorusGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\ttube: tube,\r\n\t\t\tradialSegments: radialSegments,\r\n\t\t\ttubularSegments: tubularSegments,\r\n\t\t\tarc: arc,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength,\r\n\t\t};\r\n\r\n\t\tradialSegments = Math.floor( radialSegments );\r\n\t\ttubularSegments = Math.floor( tubularSegments );\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// helper variables\r\n\r\n\t\tconst center = new Vector3();\r\n\t\tconst vertex = new Vector3();\r\n\t\tconst normal = new Vector3();\r\n\r\n\t\t// generate vertices, normals and uvs\r\n\r\n\t\tfor ( let j = 0; j <= radialSegments; j ++ ) {\r\n\r\n\t\t\tconst v = thetaStart + ( j / radialSegments ) * thetaLength;\r\n\r\n\t\t\tfor ( let i = 0; i <= tubularSegments; i ++ ) {\r\n\r\n\t\t\t\tconst u = i / tubularSegments * arc;\r\n\r\n\t\t\t\t// vertex\r\n\r\n\t\t\t\tvertex.x = ( radius + tube * Math.cos( v ) ) * Math.cos( u );\r\n\t\t\t\tvertex.y = ( radius + tube * Math.cos( v ) ) * Math.sin( u );\r\n\t\t\t\tvertex.z = tube * Math.sin( v );\r\n\r\n\t\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t\t// normal\r\n\r\n\t\t\t\tcenter.x = radius * Math.cos( u );\r\n\t\t\t\tcenter.y = radius * Math.sin( u );\r\n\t\t\t\tnormal.subVectors( vertex, center ).normalize();\r\n\r\n\t\t\t\tnormals.push( normal.x, normal.y, normal.z );\r\n\r\n\t\t\t\t// uv\r\n\r\n\t\t\t\tuvs.push( i / tubularSegments );\r\n\t\t\t\tuvs.push( j / radialSegments );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// generate indices\r\n\r\n\t\tfor ( let j = 1; j <= radialSegments; j ++ ) {\r\n\r\n\t\t\tfor ( let i = 1; i <= tubularSegments; i ++ ) {\r\n\r\n\t\t\t\t// indices\r\n\r\n\t\t\t\tconst a = ( tubularSegments + 1 ) * j + i - 1;\r\n\t\t\t\tconst b = ( tubularSegments + 1 ) * ( j - 1 ) + i - 1;\r\n\t\t\t\tconst c = ( tubularSegments + 1 ) * ( j - 1 ) + i;\r\n\t\t\t\tconst d = ( tubularSegments + 1 ) * j + i;\r\n\r\n\t\t\t\t// faces\r\n\r\n\t\t\t\tindices.push( a, b, d );\r\n\t\t\t\tindices.push( b, c, d );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}";
inline constexpr char source2[]="( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source3[]="copy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source4[]="( data ) {\r\n\r\n\t\treturn new TorusGeometry( data.radius, data.tube, data.radialSegments, data.tubularSegments, data.arc, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
inline constexpr char source5[]="fromJSON( data ) {\r\n\r\n\t\treturn new TorusGeometry( data.radius, data.tube, data.radialSegments, data.tubularSegments, data.arc, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function2(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"TorusGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"radius",true);declare(env,"tube",true);declare(env,"radialSegments",true);declare(env,"tubularSegments",true);declare(env,"arc",true);declare(env,"thetaStart",true);declare(env,"thetaLength",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(1.0);initialize(env,"radius",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(0.4);initialize(env,"tube",g2);auto g3=arg(args,2);if(g3.isUndefined())g3=JSValue(12.0);initialize(env,"radialSegments",g3);auto g4=arg(args,3);if(g4.isUndefined())g4=JSValue(48.0);initialize(env,"tubularSegments",g4);auto g5=arg(args,4);if(g5.isUndefined())g5=([&]() -> JSValue { const auto g6=([&]() -> JSValue { const auto g8=read(env,"Math");const auto g9=JSValue("PI"); return g8.get(g9); }());const auto g7=JSValue(2.0); return binary("*",g6,g7); }());initialize(env,"arc",g5);auto g10=arg(args,5);if(g10.isUndefined())g10=JSValue(0.0);initialize(env,"thetaStart",g10);auto g11=arg(args,6);if(g11.isUndefined())g11=([&]() -> JSValue { const auto g12=([&]() -> JSValue { const auto g14=read(env,"Math");const auto g15=JSValue("PI"); return g14.get(g15); }());const auto g13=JSValue(2.0); return binary("*",g12,g13); }());initialize(env,"thetaLength",g11);{auto g16=env;auto env=scope(g16);declare(env,"indices",false);declare(env,"vertices",false);declare(env,"normals",false);declare(env,"uvs",false);declare(env,"center",false);declare(env,"vertex",false);declare(env,"normal",false);([&]() -> JSValue { const auto g17=home.getPrototype();Args g19;const auto instance=g17.construct(g19,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g20=read(env,"$this");const auto g21=JSValue("type");const auto g22=JSValue("TorusGeometry"); return setReference(g20,g21,g22); }());
([&]() -> JSValue { auto g23=read(env,"$this");const auto g24=JSValue("parameters");const auto g25=([&]() -> JSValue { auto g26=JSValue::object();const auto g27=JSValue("radius");own(g26,g27,read(env,"radius"));const auto g28=JSValue("tube");own(g26,g28,read(env,"tube"));const auto g29=JSValue("radialSegments");own(g26,g29,read(env,"radialSegments"));const auto g30=JSValue("tubularSegments");own(g26,g30,read(env,"tubularSegments"));const auto g31=JSValue("arc");own(g26,g31,read(env,"arc"));const auto g32=JSValue("thetaStart");own(g26,g32,read(env,"thetaStart"));const auto g33=JSValue("thetaLength");own(g26,g33,read(env,"thetaLength")); return g26; }()); return setReference(g23,g24,g25); }());
([&]() -> JSValue { const auto g36=([&]() -> JSValue { const auto g38=read(env,"Math");const auto g37=g38.get("floor");Args g39;g39.push_back(read(env,"radialSegments")); return g37.call(g38,g39); }()); return write(env,"radialSegments",g36); }());
([&]() -> JSValue { const auto g42=([&]() -> JSValue { const auto g44=read(env,"Math");const auto g43=g44.get("floor");Args g45;g45.push_back(read(env,"tubularSegments")); return g43.call(g44,g45); }()); return write(env,"tubularSegments",g42); }());
initialize(env,"indices",([&]() -> JSValue { auto g46=JSValue::array(0); return g46; }()));
initialize(env,"vertices",([&]() -> JSValue { auto g47=JSValue::array(0); return g47; }()));
initialize(env,"normals",([&]() -> JSValue { auto g48=JSValue::array(0); return g48; }()));
initialize(env,"uvs",([&]() -> JSValue { auto g49=JSValue::array(0); return g49; }()));
initialize(env,"center",([&]() -> JSValue { const auto g50=read(env,"Vector3");Args g52; return g50.construct(g52); }()));
initialize(env,"vertex",([&]() -> JSValue { const auto g53=read(env,"Vector3");Args g55; return g53.construct(g55); }()));
initialize(env,"normal",([&]() -> JSValue { const auto g56=read(env,"Vector3");Args g58; return g56.construct(g58); }()));
{auto g59=env;auto env=scope(g59);declare(env,"j",true);initialize(env,"j",JSValue(0.0));for(;([&]() -> JSValue { const auto g65=read(env,"j");const auto g66=read(env,"radialSegments"); return binary("<=",g65,g66); }()).toBoolean();[&](){auto g60=scope(g59);declare(g60,"j",true);initialize(g60,"j",read(env,"j"));env=g60;([&]() -> JSValue { const auto g61=read(env,"j").toNumber();const JSValue g62(g61+1);write(env,"j",g62); return JSValue(g61); }());}()){{auto g67=env;auto env=scope(g67);declare(env,"v",false);initialize(env,"v",([&]() -> JSValue { const auto g68=read(env,"thetaStart");const auto g69=([&]() -> JSValue { const auto g70=([&]() -> JSValue { const auto g72=read(env,"j");const auto g73=read(env,"radialSegments"); return binary("/",g72,g73); }());const auto g71=read(env,"thetaLength"); return binary("*",g70,g71); }()); return binary("+",g68,g69); }()));
{auto g74=env;auto env=scope(g74);declare(env,"i",true);initialize(env,"i",JSValue(0.0));for(;([&]() -> JSValue { const auto g80=read(env,"i");const auto g81=read(env,"tubularSegments"); return binary("<=",g80,g81); }()).toBoolean();[&](){auto g75=scope(g74);declare(g75,"i",true);initialize(g75,"i",read(env,"i"));env=g75;([&]() -> JSValue { const auto g76=read(env,"i").toNumber();const JSValue g77(g76+1);write(env,"i",g77); return JSValue(g76); }());}()){{auto g82=env;auto env=scope(g82);declare(env,"u",false);initialize(env,"u",([&]() -> JSValue { const auto g83=([&]() -> JSValue { const auto g85=read(env,"i");const auto g86=read(env,"tubularSegments"); return binary("/",g85,g86); }());const auto g84=read(env,"arc"); return binary("*",g83,g84); }()));
([&]() -> JSValue { auto g87=read(env,"vertex");const auto g88=JSValue("x");const auto g89=([&]() -> JSValue { const auto g90=([&]() -> JSValue { const auto g92=read(env,"radius");const auto g93=([&]() -> JSValue { const auto g94=read(env,"tube");const auto g95=([&]() -> JSValue { const auto g97=read(env,"Math");const auto g96=g97.get("cos");Args g98;g98.push_back(read(env,"v")); return g96.call(g97,g98); }()); return binary("*",g94,g95); }()); return binary("+",g92,g93); }());const auto g91=([&]() -> JSValue { const auto g100=read(env,"Math");const auto g99=g100.get("cos");Args g101;g101.push_back(read(env,"u")); return g99.call(g100,g101); }()); return binary("*",g90,g91); }()); return setReference(g87,g88,g89); }());
([&]() -> JSValue { auto g102=read(env,"vertex");const auto g103=JSValue("y");const auto g104=([&]() -> JSValue { const auto g105=([&]() -> JSValue { const auto g107=read(env,"radius");const auto g108=([&]() -> JSValue { const auto g109=read(env,"tube");const auto g110=([&]() -> JSValue { const auto g112=read(env,"Math");const auto g111=g112.get("cos");Args g113;g113.push_back(read(env,"v")); return g111.call(g112,g113); }()); return binary("*",g109,g110); }()); return binary("+",g107,g108); }());const auto g106=([&]() -> JSValue { const auto g115=read(env,"Math");const auto g114=g115.get("sin");Args g116;g116.push_back(read(env,"u")); return g114.call(g115,g116); }()); return binary("*",g105,g106); }()); return setReference(g102,g103,g104); }());
([&]() -> JSValue { auto g117=read(env,"vertex");const auto g118=JSValue("z");const auto g119=([&]() -> JSValue { const auto g120=read(env,"tube");const auto g121=([&]() -> JSValue { const auto g123=read(env,"Math");const auto g122=g123.get("sin");Args g124;g124.push_back(read(env,"v")); return g122.call(g123,g124); }()); return binary("*",g120,g121); }()); return setReference(g117,g118,g119); }());
([&]() -> JSValue { const auto g126=read(env,"vertices");const auto g125=g126.get("push");Args g127;g127.push_back(([&]() -> JSValue { const auto g128=read(env,"vertex");const auto g129=JSValue("x"); return g128.get(g129); }()));g127.push_back(([&]() -> JSValue { const auto g130=read(env,"vertex");const auto g131=JSValue("y"); return g130.get(g131); }()));g127.push_back(([&]() -> JSValue { const auto g132=read(env,"vertex");const auto g133=JSValue("z"); return g132.get(g133); }())); return g125.call(g126,g127); }());
([&]() -> JSValue { auto g134=read(env,"center");const auto g135=JSValue("x");const auto g136=([&]() -> JSValue { const auto g137=read(env,"radius");const auto g138=([&]() -> JSValue { const auto g140=read(env,"Math");const auto g139=g140.get("cos");Args g141;g141.push_back(read(env,"u")); return g139.call(g140,g141); }()); return binary("*",g137,g138); }()); return setReference(g134,g135,g136); }());
([&]() -> JSValue { auto g142=read(env,"center");const auto g143=JSValue("y");const auto g144=([&]() -> JSValue { const auto g145=read(env,"radius");const auto g146=([&]() -> JSValue { const auto g148=read(env,"Math");const auto g147=g148.get("sin");Args g149;g149.push_back(read(env,"u")); return g147.call(g148,g149); }()); return binary("*",g145,g146); }()); return setReference(g142,g143,g144); }());
([&]() -> JSValue { const auto g151=([&]() -> JSValue { const auto g154=read(env,"normal");const auto g153=g154.get("subVectors");Args g155;g155.push_back(read(env,"vertex"));g155.push_back(read(env,"center")); return g153.call(g154,g155); }());const auto g150=g151.get("normalize");Args g152; return g150.call(g151,g152); }());
([&]() -> JSValue { const auto g157=read(env,"normals");const auto g156=g157.get("push");Args g158;g158.push_back(([&]() -> JSValue { const auto g159=read(env,"normal");const auto g160=JSValue("x"); return g159.get(g160); }()));g158.push_back(([&]() -> JSValue { const auto g161=read(env,"normal");const auto g162=JSValue("y"); return g161.get(g162); }()));g158.push_back(([&]() -> JSValue { const auto g163=read(env,"normal");const auto g164=JSValue("z"); return g163.get(g164); }())); return g156.call(g157,g158); }());
([&]() -> JSValue { const auto g166=read(env,"uvs");const auto g165=g166.get("push");Args g167;g167.push_back(([&]() -> JSValue { const auto g168=read(env,"i");const auto g169=read(env,"tubularSegments"); return binary("/",g168,g169); }())); return g165.call(g166,g167); }());
([&]() -> JSValue { const auto g171=read(env,"uvs");const auto g170=g171.get("push");Args g172;g172.push_back(([&]() -> JSValue { const auto g173=read(env,"j");const auto g174=read(env,"radialSegments"); return binary("/",g173,g174); }())); return g170.call(g171,g172); }());}}}}}}
{auto g175=env;auto env=scope(g175);declare(env,"j",true);initialize(env,"j",JSValue(1.0));for(;([&]() -> JSValue { const auto g181=read(env,"j");const auto g182=read(env,"radialSegments"); return binary("<=",g181,g182); }()).toBoolean();[&](){auto g176=scope(g175);declare(g176,"j",true);initialize(g176,"j",read(env,"j"));env=g176;([&]() -> JSValue { const auto g177=read(env,"j").toNumber();const JSValue g178(g177+1);write(env,"j",g178); return JSValue(g177); }());}()){{auto g183=env;auto env=scope(g183);{auto g184=env;auto env=scope(g184);declare(env,"i",true);initialize(env,"i",JSValue(1.0));for(;([&]() -> JSValue { const auto g190=read(env,"i");const auto g191=read(env,"tubularSegments"); return binary("<=",g190,g191); }()).toBoolean();[&](){auto g185=scope(g184);declare(g185,"i",true);initialize(g185,"i",read(env,"i"));env=g185;([&]() -> JSValue { const auto g186=read(env,"i").toNumber();const JSValue g187(g186+1);write(env,"i",g187); return JSValue(g186); }());}()){{auto g192=env;auto env=scope(g192);declare(env,"a",false);declare(env,"b",false);declare(env,"c",false);declare(env,"d",false);initialize(env,"a",([&]() -> JSValue { const auto g193=([&]() -> JSValue { const auto g195=([&]() -> JSValue { const auto g197=([&]() -> JSValue { const auto g199=read(env,"tubularSegments");const auto g200=JSValue(1.0); return binary("+",g199,g200); }());const auto g198=read(env,"j"); return binary("*",g197,g198); }());const auto g196=read(env,"i"); return binary("+",g195,g196); }());const auto g194=JSValue(1.0); return binary("-",g193,g194); }()));
initialize(env,"b",([&]() -> JSValue { const auto g201=([&]() -> JSValue { const auto g203=([&]() -> JSValue { const auto g205=([&]() -> JSValue { const auto g207=read(env,"tubularSegments");const auto g208=JSValue(1.0); return binary("+",g207,g208); }());const auto g206=([&]() -> JSValue { const auto g209=read(env,"j");const auto g210=JSValue(1.0); return binary("-",g209,g210); }()); return binary("*",g205,g206); }());const auto g204=read(env,"i"); return binary("+",g203,g204); }());const auto g202=JSValue(1.0); return binary("-",g201,g202); }()));
initialize(env,"c",([&]() -> JSValue { const auto g211=([&]() -> JSValue { const auto g213=([&]() -> JSValue { const auto g215=read(env,"tubularSegments");const auto g216=JSValue(1.0); return binary("+",g215,g216); }());const auto g214=([&]() -> JSValue { const auto g217=read(env,"j");const auto g218=JSValue(1.0); return binary("-",g217,g218); }()); return binary("*",g213,g214); }());const auto g212=read(env,"i"); return binary("+",g211,g212); }()));
initialize(env,"d",([&]() -> JSValue { const auto g219=([&]() -> JSValue { const auto g221=([&]() -> JSValue { const auto g223=read(env,"tubularSegments");const auto g224=JSValue(1.0); return binary("+",g223,g224); }());const auto g222=read(env,"j"); return binary("*",g221,g222); }());const auto g220=read(env,"i"); return binary("+",g219,g220); }()));
([&]() -> JSValue { const auto g226=read(env,"indices");const auto g225=g226.get("push");Args g227;g227.push_back(read(env,"a"));g227.push_back(read(env,"b"));g227.push_back(read(env,"d")); return g225.call(g226,g227); }());
([&]() -> JSValue { const auto g229=read(env,"indices");const auto g228=g229.get("push");Args g230;g230.push_back(read(env,"b"));g230.push_back(read(env,"c"));g230.push_back(read(env,"d")); return g228.call(g229,g230); }());}}}}}}
([&]() -> JSValue { const auto g232=read(env,"$this");const auto g231=g232.get("setIndex");Args g233;g233.push_back(read(env,"indices")); return g231.call(g232,g233); }());
([&]() -> JSValue { const auto g235=read(env,"$this");const auto g234=g235.get("setAttribute");Args g236;g236.push_back(JSValue("position"));g236.push_back(([&]() -> JSValue { const auto g237=read(env,"Float32BufferAttribute");Args g239;g239.push_back(read(env,"vertices"));g239.push_back(JSValue(3.0)); return g237.construct(g239); }())); return g234.call(g235,g236); }());
([&]() -> JSValue { const auto g241=read(env,"$this");const auto g240=g241.get("setAttribute");Args g242;g242.push_back(JSValue("normal"));g242.push_back(([&]() -> JSValue { const auto g243=read(env,"Float32BufferAttribute");Args g245;g245.push_back(read(env,"normals"));g245.push_back(JSValue(3.0)); return g243.construct(g245); }())); return g240.call(g241,g242); }());
([&]() -> JSValue { const auto g247=read(env,"$this");const auto g246=g247.get("setAttribute");Args g248;g248.push_back(JSValue("uv"));g248.push_back(([&]() -> JSValue { const auto g249=read(env,"Float32BufferAttribute");Args g251;g251.push_back(read(env,"uvs"));g251.push_back(JSValue(2.0)); return g249.construct(g251); }())); return g246.call(g247,g248); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"source",true);initialize(env,"source",arg(args,0));{auto g252=env;auto env=scope(g252);([&]() -> JSValue { const auto g254=read(env,"$this");const auto g253=home.getPrototype().get("copy",g254);Args g255;g255.push_back(read(env,"source")); return g253.call(g254,g255); }());
([&]() -> JSValue { auto g256=read(env,"$this");const auto g257=JSValue("parameters");const auto g258=([&]() -> JSValue { const auto g260=read(env,"Object");const auto g259=g260.get("assign");Args g261;g261.push_back(([&]() -> JSValue { auto g262=JSValue::object(); return g262; }()));g261.push_back(([&]() -> JSValue { const auto g263=read(env,"source");const auto g264=JSValue("parameters"); return g263.get(g264); }())); return g259.call(g260,g261); }()); return setReference(g256,g257,g258); }());
return read(env,"$this");}return {};}
JSValue function2(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g266=env;auto env=scope(g266);return ([&]() -> JSValue { const auto g267=read(env,"TorusGeometry");Args g269;g269.push_back(([&]() -> JSValue { const auto g270=read(env,"data");const auto g271=JSValue("radius"); return g270.get(g271); }()));g269.push_back(([&]() -> JSValue { const auto g272=read(env,"data");const auto g273=JSValue("tube"); return g272.get(g273); }()));g269.push_back(([&]() -> JSValue { const auto g274=read(env,"data");const auto g275=JSValue("radialSegments"); return g274.get(g275); }()));g269.push_back(([&]() -> JSValue { const auto g276=read(env,"data");const auto g277=JSValue("tubularSegments"); return g276.get(g277); }()));g269.push_back(([&]() -> JSValue { const auto g278=read(env,"data");const auto g279=JSValue("arc"); return g278.get(g279); }()));g269.push_back(([&]() -> JSValue { const auto g280=read(env,"data");const auto g281=JSValue("thetaStart"); return g280.get(g281); }()));g269.push_back(([&]() -> JSValue { const auto g282=read(env,"data");const auto g283=JSValue("thetaLength"); return g282.get(g283); }())); return g267.construct(g269); }());}return {};}
}
JSValue nativeTorusGeometryModule(){using namespace TorusGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"TorusGeometry");auto env=scope();JSValue home;bindImport(env,"BufferGeometry",nativeBufferGeometryConstructor());bindImport(env,"Float32BufferAttribute",nativeFloat32BufferAttributeConstructor());bindImport(env,"Vector3",nativeVector3Binding("Vector3"));declare(env,"TorusGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"TorusGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"BufferGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"TorusGeometry",constructor);{auto home=prototype;auto function=([&]() -> JSValue { auto g265=JSValue::function(function1,{env,home},"copy",1);g265.setFunctionSourceText(source2); return g265; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("copy",d);}{auto home=constructor;auto function=([&]() -> JSValue { auto g284=JSValue::function(function2,{env,home},"fromJSON",1);g284.setFunctionSourceText(source4); return g284; }());function.setFunctionSourceText(source5);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"TorusGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeTorusGeometryBinding(const std::string& name){return nativeTorusGeometryModule().get(name);}
JSValue nativeTorusGeometryConstructor(){return nativeTorusGeometryBinding("TorusGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
