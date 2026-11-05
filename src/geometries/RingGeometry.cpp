#include "geometries/RingGeometry.h"
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
RingGeometry::RingGeometry(double innerRadius, double outerRadius, double thetaSegments, double phiSegments, double thetaStart, double thetaLength)
    : parameters{innerRadius, outerRadius, thetaSegments, phiSegments, thetaStart, thetaLength} {
    type = "RingGeometry";
    thetaSegments = MathUtils::max(3, thetaSegments);
    phiSegments = MathUtils::max(1, phiSegments);
    requireTerminating(thetaSegments);
    requireTerminating(phiSegments);
    std::vector<double> indices, vertices, normals, uvs;
    double radius = innerRadius;
    const double radiusStep = (outerRadius - innerRadius) / phiSegments;
    Vector3 vertex;
    for (double j = 0; j <= phiSegments; ++j) {
        for (double i = 0; i <= thetaSegments; ++i) {
            const double segment = thetaStart + i / thetaSegments * thetaLength;
            vertex.x = radius * JSNumberMath::cos(segment);
            vertex.y = radius * JSNumberMath::sin(segment);
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normals.insert(normals.end(), {0, 0, 1});
            uvs.insert(uvs.end(), {(vertex.x / outerRadius + 1) / 2, (vertex.y / outerRadius + 1) / 2});
        }
        radius += radiusStep;
    }
    for (double j = 0; j < phiSegments; ++j) {
        const double thetaSegmentLevel = j * (thetaSegments + 1);
        for (double i = 0; i < thetaSegments; ++i) {
            const double segment = i + thetaSegmentLevel;
            const double a = segment, b = segment + thetaSegments + 1,
                         c = segment + thetaSegments + 2, d = segment + 1;
            indices.insert(indices.end(), {a, b, d, b, c, d});
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
RingGeometry& RingGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const RingGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> RingGeometry::clone() const {
    auto geometry = std::make_shared<RingGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue RingGeometry::toJSON() const {
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
    result["innerRadius"] = parameters.innerRadius;
    result["outerRadius"] = parameters.outerRadius;
    result["thetaSegments"] = parameters.thetaSegments;
    result["phiSegments"] = parameters.phiSegments;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<RingGeometry> RingGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<RingGeometry>(jsonNumber(data, "innerRadius", 0.5), jsonNumber(data, "outerRadius", 1), jsonNumber(data, "thetaSegments", 32), jsonNumber(data, "phiSegments", 1), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/RingGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
namespace RingGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class RingGeometry extends BufferGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new ring geometry.\r\n\t *\r\n\t * @param {number} [innerRadius=0.5] - The inner radius of the ring.\r\n\t * @param {number} [outerRadius=1] - The outer radius of the ring.\r\n\t * @param {number} [thetaSegments=32] - Number of segments. A higher number means the ring will be more round. Minimum is `3`.\r\n\t * @param {number} [phiSegments=1] - Number of segments per ring segment. Minimum is `1`.\r\n\t * @param {number} [thetaStart=0] - Starting angle in radians.\r\n\t * @param {number} [thetaLength=Math.PI*2] - Central angle in radians.\r\n\t */\r\n\tconstructor( innerRadius = 0.5, outerRadius = 1, thetaSegments = 32, phiSegments = 1, thetaStart = 0, thetaLength = Math.PI * 2 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'RingGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tinnerRadius: innerRadius,\r\n\t\t\touterRadius: outerRadius,\r\n\t\t\tthetaSegments: thetaSegments,\r\n\t\t\tphiSegments: phiSegments,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength\r\n\t\t};\r\n\r\n\t\tthetaSegments = Math.max( 3, thetaSegments );\r\n\t\tphiSegments = Math.max( 1, phiSegments );\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// some helper variables\r\n\r\n\t\tlet radius = innerRadius;\r\n\t\tconst radiusStep = ( ( outerRadius - innerRadius ) / phiSegments );\r\n\t\tconst vertex = new Vector3();\r\n\t\tconst uv = new Vector2();\r\n\r\n\t\t// generate vertices, normals and uvs\r\n\r\n\t\tfor ( let j = 0; j <= phiSegments; j ++ ) {\r\n\r\n\t\t\tfor ( let i = 0; i <= thetaSegments; i ++ ) {\r\n\r\n\t\t\t\t// values are generate from the inside of the ring to the outside\r\n\r\n\t\t\t\tconst segment = thetaStart + i / thetaSegments * thetaLength;\r\n\r\n\t\t\t\t// vertex\r\n\r\n\t\t\t\tvertex.x = radius * Math.cos( segment );\r\n\t\t\t\tvertex.y = radius * Math.sin( segment );\r\n\r\n\t\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t\t// normal\r\n\r\n\t\t\t\tnormals.push( 0, 0, 1 );\r\n\r\n\t\t\t\t// uv\r\n\r\n\t\t\t\tuv.x = ( vertex.x / outerRadius + 1 ) / 2;\r\n\t\t\t\tuv.y = ( vertex.y / outerRadius + 1 ) / 2;\r\n\r\n\t\t\t\tuvs.push( uv.x, uv.y );\r\n\r\n\t\t\t}\r\n\r\n\t\t\t// increase the radius for next row of vertices\r\n\r\n\t\t\tradius += radiusStep;\r\n\r\n\t\t}\r\n\r\n\t\t// indices\r\n\r\n\t\tfor ( let j = 0; j < phiSegments; j ++ ) {\r\n\r\n\t\t\tconst thetaSegmentLevel = j * ( thetaSegments + 1 );\r\n\r\n\t\t\tfor ( let i = 0; i < thetaSegments; i ++ ) {\r\n\r\n\t\t\t\tconst segment = i + thetaSegmentLevel;\r\n\r\n\t\t\t\tconst a = segment;\r\n\t\t\t\tconst b = segment + thetaSegments + 1;\r\n\t\t\t\tconst c = segment + thetaSegments + 2;\r\n\t\t\t\tconst d = segment + 1;\r\n\r\n\t\t\t\t// faces\r\n\r\n\t\t\t\tindices.push( a, b, d );\r\n\t\t\t\tindices.push( b, c, d );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}\r\n\r\n\tcopy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {RingGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new RingGeometry( data.innerRadius, data.outerRadius, data.thetaSegments, data.phiSegments, data.thetaStart, data.thetaLength );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( innerRadius = 0.5, outerRadius = 1, thetaSegments = 32, phiSegments = 1, thetaStart = 0, thetaLength = Math.PI * 2 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'RingGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tinnerRadius: innerRadius,\r\n\t\t\touterRadius: outerRadius,\r\n\t\t\tthetaSegments: thetaSegments,\r\n\t\t\tphiSegments: phiSegments,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength\r\n\t\t};\r\n\r\n\t\tthetaSegments = Math.max( 3, thetaSegments );\r\n\t\tphiSegments = Math.max( 1, phiSegments );\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// some helper variables\r\n\r\n\t\tlet radius = innerRadius;\r\n\t\tconst radiusStep = ( ( outerRadius - innerRadius ) / phiSegments );\r\n\t\tconst vertex = new Vector3();\r\n\t\tconst uv = new Vector2();\r\n\r\n\t\t// generate vertices, normals and uvs\r\n\r\n\t\tfor ( let j = 0; j <= phiSegments; j ++ ) {\r\n\r\n\t\t\tfor ( let i = 0; i <= thetaSegments; i ++ ) {\r\n\r\n\t\t\t\t// values are generate from the inside of the ring to the outside\r\n\r\n\t\t\t\tconst segment = thetaStart + i / thetaSegments * thetaLength;\r\n\r\n\t\t\t\t// vertex\r\n\r\n\t\t\t\tvertex.x = radius * Math.cos( segment );\r\n\t\t\t\tvertex.y = radius * Math.sin( segment );\r\n\r\n\t\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t\t// normal\r\n\r\n\t\t\t\tnormals.push( 0, 0, 1 );\r\n\r\n\t\t\t\t// uv\r\n\r\n\t\t\t\tuv.x = ( vertex.x / outerRadius + 1 ) / 2;\r\n\t\t\t\tuv.y = ( vertex.y / outerRadius + 1 ) / 2;\r\n\r\n\t\t\t\tuvs.push( uv.x, uv.y );\r\n\r\n\t\t\t}\r\n\r\n\t\t\t// increase the radius for next row of vertices\r\n\r\n\t\t\tradius += radiusStep;\r\n\r\n\t\t}\r\n\r\n\t\t// indices\r\n\r\n\t\tfor ( let j = 0; j < phiSegments; j ++ ) {\r\n\r\n\t\t\tconst thetaSegmentLevel = j * ( thetaSegments + 1 );\r\n\r\n\t\t\tfor ( let i = 0; i < thetaSegments; i ++ ) {\r\n\r\n\t\t\t\tconst segment = i + thetaSegmentLevel;\r\n\r\n\t\t\t\tconst a = segment;\r\n\t\t\t\tconst b = segment + thetaSegments + 1;\r\n\t\t\t\tconst c = segment + thetaSegments + 2;\r\n\t\t\t\tconst d = segment + 1;\r\n\r\n\t\t\t\t// faces\r\n\r\n\t\t\t\tindices.push( a, b, d );\r\n\t\t\t\tindices.push( b, c, d );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}";
inline constexpr char source2[]="( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source3[]="copy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source4[]="( data ) {\r\n\r\n\t\treturn new RingGeometry( data.innerRadius, data.outerRadius, data.thetaSegments, data.phiSegments, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
inline constexpr char source5[]="fromJSON( data ) {\r\n\r\n\t\treturn new RingGeometry( data.innerRadius, data.outerRadius, data.thetaSegments, data.phiSegments, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function2(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"RingGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"innerRadius",true);declare(env,"outerRadius",true);declare(env,"thetaSegments",true);declare(env,"phiSegments",true);declare(env,"thetaStart",true);declare(env,"thetaLength",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(0.5);initialize(env,"innerRadius",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(1.0);initialize(env,"outerRadius",g2);auto g3=arg(args,2);if(g3.isUndefined())g3=JSValue(32.0);initialize(env,"thetaSegments",g3);auto g4=arg(args,3);if(g4.isUndefined())g4=JSValue(1.0);initialize(env,"phiSegments",g4);auto g5=arg(args,4);if(g5.isUndefined())g5=JSValue(0.0);initialize(env,"thetaStart",g5);auto g6=arg(args,5);if(g6.isUndefined())g6=([&]() -> JSValue { const auto g7=([&]() -> JSValue { const auto g9=read(env,"Math");const auto g10=JSValue("PI"); return g9.get(g10); }());const auto g8=JSValue(2.0); return binary("*",g7,g8); }());initialize(env,"thetaLength",g6);{auto g11=env;auto env=scope(g11);declare(env,"indices",false);declare(env,"vertices",false);declare(env,"normals",false);declare(env,"uvs",false);declare(env,"radius",true);declare(env,"radiusStep",false);declare(env,"vertex",false);declare(env,"uv",false);([&]() -> JSValue { const auto g12=home.getPrototype();Args g14;const auto instance=g12.construct(g14,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g15=read(env,"$this");const auto g16=JSValue("type");const auto g17=JSValue("RingGeometry"); return setReference(g15,g16,g17); }());
([&]() -> JSValue { auto g18=read(env,"$this");const auto g19=JSValue("parameters");const auto g20=([&]() -> JSValue { auto g21=JSValue::object();const auto g22=JSValue("innerRadius");own(g21,g22,read(env,"innerRadius"));const auto g23=JSValue("outerRadius");own(g21,g23,read(env,"outerRadius"));const auto g24=JSValue("thetaSegments");own(g21,g24,read(env,"thetaSegments"));const auto g25=JSValue("phiSegments");own(g21,g25,read(env,"phiSegments"));const auto g26=JSValue("thetaStart");own(g21,g26,read(env,"thetaStart"));const auto g27=JSValue("thetaLength");own(g21,g27,read(env,"thetaLength")); return g21; }()); return setReference(g18,g19,g20); }());
([&]() -> JSValue { const auto g30=([&]() -> JSValue { const auto g32=read(env,"Math");const auto g31=g32.get("max");Args g33;g33.push_back(JSValue(3.0));g33.push_back(read(env,"thetaSegments")); return g31.call(g32,g33); }()); return write(env,"thetaSegments",g30); }());
([&]() -> JSValue { const auto g36=([&]() -> JSValue { const auto g38=read(env,"Math");const auto g37=g38.get("max");Args g39;g39.push_back(JSValue(1.0));g39.push_back(read(env,"phiSegments")); return g37.call(g38,g39); }()); return write(env,"phiSegments",g36); }());
initialize(env,"indices",([&]() -> JSValue { auto g40=JSValue::array(0); return g40; }()));
initialize(env,"vertices",([&]() -> JSValue { auto g41=JSValue::array(0); return g41; }()));
initialize(env,"normals",([&]() -> JSValue { auto g42=JSValue::array(0); return g42; }()));
initialize(env,"uvs",([&]() -> JSValue { auto g43=JSValue::array(0); return g43; }()));
initialize(env,"radius",read(env,"innerRadius"));
initialize(env,"radiusStep",([&]() -> JSValue { const auto g44=([&]() -> JSValue { const auto g46=read(env,"outerRadius");const auto g47=read(env,"innerRadius"); return binary("-",g46,g47); }());const auto g45=read(env,"phiSegments"); return binary("/",g44,g45); }()));
initialize(env,"vertex",([&]() -> JSValue { const auto g48=read(env,"Vector3");Args g50; return g48.construct(g50); }()));
initialize(env,"uv",([&]() -> JSValue { const auto g51=read(env,"Vector2");Args g53; return g51.construct(g53); }()));
{auto g54=env;auto env=scope(g54);declare(env,"j",true);initialize(env,"j",JSValue(0.0));for(;([&]() -> JSValue { const auto g60=read(env,"j");const auto g61=read(env,"phiSegments"); return binary("<=",g60,g61); }()).toBoolean();[&](){auto g55=scope(g54);declare(g55,"j",true);initialize(g55,"j",read(env,"j"));env=g55;([&]() -> JSValue { const auto g56=read(env,"j").toNumber();const JSValue g57(g56+1);write(env,"j",g57); return JSValue(g56); }());}()){{auto g62=env;auto env=scope(g62);{auto g63=env;auto env=scope(g63);declare(env,"i",true);initialize(env,"i",JSValue(0.0));for(;([&]() -> JSValue { const auto g69=read(env,"i");const auto g70=read(env,"thetaSegments"); return binary("<=",g69,g70); }()).toBoolean();[&](){auto g64=scope(g63);declare(g64,"i",true);initialize(g64,"i",read(env,"i"));env=g64;([&]() -> JSValue { const auto g65=read(env,"i").toNumber();const JSValue g66(g65+1);write(env,"i",g66); return JSValue(g65); }());}()){{auto g71=env;auto env=scope(g71);declare(env,"segment",false);initialize(env,"segment",([&]() -> JSValue { const auto g72=read(env,"thetaStart");const auto g73=([&]() -> JSValue { const auto g74=([&]() -> JSValue { const auto g76=read(env,"i");const auto g77=read(env,"thetaSegments"); return binary("/",g76,g77); }());const auto g75=read(env,"thetaLength"); return binary("*",g74,g75); }()); return binary("+",g72,g73); }()));
([&]() -> JSValue { auto g78=read(env,"vertex");const auto g79=JSValue("x");const auto g80=([&]() -> JSValue { const auto g81=read(env,"radius");const auto g82=([&]() -> JSValue { const auto g84=read(env,"Math");const auto g83=g84.get("cos");Args g85;g85.push_back(read(env,"segment")); return g83.call(g84,g85); }()); return binary("*",g81,g82); }()); return setReference(g78,g79,g80); }());
([&]() -> JSValue { auto g86=read(env,"vertex");const auto g87=JSValue("y");const auto g88=([&]() -> JSValue { const auto g89=read(env,"radius");const auto g90=([&]() -> JSValue { const auto g92=read(env,"Math");const auto g91=g92.get("sin");Args g93;g93.push_back(read(env,"segment")); return g91.call(g92,g93); }()); return binary("*",g89,g90); }()); return setReference(g86,g87,g88); }());
([&]() -> JSValue { const auto g95=read(env,"vertices");const auto g94=g95.get("push");Args g96;g96.push_back(([&]() -> JSValue { const auto g97=read(env,"vertex");const auto g98=JSValue("x"); return g97.get(g98); }()));g96.push_back(([&]() -> JSValue { const auto g99=read(env,"vertex");const auto g100=JSValue("y"); return g99.get(g100); }()));g96.push_back(([&]() -> JSValue { const auto g101=read(env,"vertex");const auto g102=JSValue("z"); return g101.get(g102); }())); return g94.call(g95,g96); }());
([&]() -> JSValue { const auto g104=read(env,"normals");const auto g103=g104.get("push");Args g105;g105.push_back(JSValue(0.0));g105.push_back(JSValue(0.0));g105.push_back(JSValue(1.0)); return g103.call(g104,g105); }());
([&]() -> JSValue { auto g106=read(env,"uv");const auto g107=JSValue("x");const auto g108=([&]() -> JSValue { const auto g109=([&]() -> JSValue { const auto g111=([&]() -> JSValue { const auto g113=([&]() -> JSValue { const auto g115=read(env,"vertex");const auto g116=JSValue("x"); return g115.get(g116); }());const auto g114=read(env,"outerRadius"); return binary("/",g113,g114); }());const auto g112=JSValue(1.0); return binary("+",g111,g112); }());const auto g110=JSValue(2.0); return binary("/",g109,g110); }()); return setReference(g106,g107,g108); }());
([&]() -> JSValue { auto g117=read(env,"uv");const auto g118=JSValue("y");const auto g119=([&]() -> JSValue { const auto g120=([&]() -> JSValue { const auto g122=([&]() -> JSValue { const auto g124=([&]() -> JSValue { const auto g126=read(env,"vertex");const auto g127=JSValue("y"); return g126.get(g127); }());const auto g125=read(env,"outerRadius"); return binary("/",g124,g125); }());const auto g123=JSValue(1.0); return binary("+",g122,g123); }());const auto g121=JSValue(2.0); return binary("/",g120,g121); }()); return setReference(g117,g118,g119); }());
([&]() -> JSValue { const auto g129=read(env,"uvs");const auto g128=g129.get("push");Args g130;g130.push_back(([&]() -> JSValue { const auto g131=read(env,"uv");const auto g132=JSValue("x"); return g131.get(g132); }()));g130.push_back(([&]() -> JSValue { const auto g133=read(env,"uv");const auto g134=JSValue("y"); return g133.get(g134); }())); return g128.call(g129,g130); }());}}}
([&]() -> JSValue { const auto g138=read(env,"radius");const auto g139=read(env,"radiusStep");const auto g137=binary("+",g138,g139); return write(env,"radius",g137); }());}}}
{auto g140=env;auto env=scope(g140);declare(env,"j",true);initialize(env,"j",JSValue(0.0));for(;([&]() -> JSValue { const auto g146=read(env,"j");const auto g147=read(env,"phiSegments"); return binary("<",g146,g147); }()).toBoolean();[&](){auto g141=scope(g140);declare(g141,"j",true);initialize(g141,"j",read(env,"j"));env=g141;([&]() -> JSValue { const auto g142=read(env,"j").toNumber();const JSValue g143(g142+1);write(env,"j",g143); return JSValue(g142); }());}()){{auto g148=env;auto env=scope(g148);declare(env,"thetaSegmentLevel",false);initialize(env,"thetaSegmentLevel",([&]() -> JSValue { const auto g149=read(env,"j");const auto g150=([&]() -> JSValue { const auto g151=read(env,"thetaSegments");const auto g152=JSValue(1.0); return binary("+",g151,g152); }()); return binary("*",g149,g150); }()));
{auto g153=env;auto env=scope(g153);declare(env,"i",true);initialize(env,"i",JSValue(0.0));for(;([&]() -> JSValue { const auto g159=read(env,"i");const auto g160=read(env,"thetaSegments"); return binary("<",g159,g160); }()).toBoolean();[&](){auto g154=scope(g153);declare(g154,"i",true);initialize(g154,"i",read(env,"i"));env=g154;([&]() -> JSValue { const auto g155=read(env,"i").toNumber();const JSValue g156(g155+1);write(env,"i",g156); return JSValue(g155); }());}()){{auto g161=env;auto env=scope(g161);declare(env,"segment",false);declare(env,"a",false);declare(env,"b",false);declare(env,"c",false);declare(env,"d",false);initialize(env,"segment",([&]() -> JSValue { const auto g162=read(env,"i");const auto g163=read(env,"thetaSegmentLevel"); return binary("+",g162,g163); }()));
initialize(env,"a",read(env,"segment"));
initialize(env,"b",([&]() -> JSValue { const auto g164=([&]() -> JSValue { const auto g166=read(env,"segment");const auto g167=read(env,"thetaSegments"); return binary("+",g166,g167); }());const auto g165=JSValue(1.0); return binary("+",g164,g165); }()));
initialize(env,"c",([&]() -> JSValue { const auto g168=([&]() -> JSValue { const auto g170=read(env,"segment");const auto g171=read(env,"thetaSegments"); return binary("+",g170,g171); }());const auto g169=JSValue(2.0); return binary("+",g168,g169); }()));
initialize(env,"d",([&]() -> JSValue { const auto g172=read(env,"segment");const auto g173=JSValue(1.0); return binary("+",g172,g173); }()));
([&]() -> JSValue { const auto g175=read(env,"indices");const auto g174=g175.get("push");Args g176;g176.push_back(read(env,"a"));g176.push_back(read(env,"b"));g176.push_back(read(env,"d")); return g174.call(g175,g176); }());
([&]() -> JSValue { const auto g178=read(env,"indices");const auto g177=g178.get("push");Args g179;g179.push_back(read(env,"b"));g179.push_back(read(env,"c"));g179.push_back(read(env,"d")); return g177.call(g178,g179); }());}}}}}}
([&]() -> JSValue { const auto g181=read(env,"$this");const auto g180=g181.get("setIndex");Args g182;g182.push_back(read(env,"indices")); return g180.call(g181,g182); }());
([&]() -> JSValue { const auto g184=read(env,"$this");const auto g183=g184.get("setAttribute");Args g185;g185.push_back(JSValue("position"));g185.push_back(([&]() -> JSValue { const auto g186=read(env,"Float32BufferAttribute");Args g188;g188.push_back(read(env,"vertices"));g188.push_back(JSValue(3.0)); return g186.construct(g188); }())); return g183.call(g184,g185); }());
([&]() -> JSValue { const auto g190=read(env,"$this");const auto g189=g190.get("setAttribute");Args g191;g191.push_back(JSValue("normal"));g191.push_back(([&]() -> JSValue { const auto g192=read(env,"Float32BufferAttribute");Args g194;g194.push_back(read(env,"normals"));g194.push_back(JSValue(3.0)); return g192.construct(g194); }())); return g189.call(g190,g191); }());
([&]() -> JSValue { const auto g196=read(env,"$this");const auto g195=g196.get("setAttribute");Args g197;g197.push_back(JSValue("uv"));g197.push_back(([&]() -> JSValue { const auto g198=read(env,"Float32BufferAttribute");Args g200;g200.push_back(read(env,"uvs"));g200.push_back(JSValue(2.0)); return g198.construct(g200); }())); return g195.call(g196,g197); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"source",true);initialize(env,"source",arg(args,0));{auto g201=env;auto env=scope(g201);([&]() -> JSValue { const auto g203=read(env,"$this");const auto g202=home.getPrototype().get("copy",g203);Args g204;g204.push_back(read(env,"source")); return g202.call(g203,g204); }());
([&]() -> JSValue { auto g205=read(env,"$this");const auto g206=JSValue("parameters");const auto g207=([&]() -> JSValue { const auto g209=read(env,"Object");const auto g208=g209.get("assign");Args g210;g210.push_back(([&]() -> JSValue { auto g211=JSValue::object(); return g211; }()));g210.push_back(([&]() -> JSValue { const auto g212=read(env,"source");const auto g213=JSValue("parameters"); return g212.get(g213); }())); return g208.call(g209,g210); }()); return setReference(g205,g206,g207); }());
return read(env,"$this");}return {};}
JSValue function2(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g215=env;auto env=scope(g215);return ([&]() -> JSValue { const auto g216=read(env,"RingGeometry");Args g218;g218.push_back(([&]() -> JSValue { const auto g219=read(env,"data");const auto g220=JSValue("innerRadius"); return g219.get(g220); }()));g218.push_back(([&]() -> JSValue { const auto g221=read(env,"data");const auto g222=JSValue("outerRadius"); return g221.get(g222); }()));g218.push_back(([&]() -> JSValue { const auto g223=read(env,"data");const auto g224=JSValue("thetaSegments"); return g223.get(g224); }()));g218.push_back(([&]() -> JSValue { const auto g225=read(env,"data");const auto g226=JSValue("phiSegments"); return g225.get(g226); }()));g218.push_back(([&]() -> JSValue { const auto g227=read(env,"data");const auto g228=JSValue("thetaStart"); return g227.get(g228); }()));g218.push_back(([&]() -> JSValue { const auto g229=read(env,"data");const auto g230=JSValue("thetaLength"); return g229.get(g230); }())); return g216.construct(g218); }());}return {};}
}
JSValue nativeRingGeometryModule(){using namespace RingGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"RingGeometry");auto env=scope();JSValue home;bindImport(env,"BufferGeometry",nativeBufferGeometryConstructor());bindImport(env,"Float32BufferAttribute",nativeFloat32BufferAttributeConstructor());bindImport(env,"Vector2",nativeVector2Binding("Vector2"));bindImport(env,"Vector3",nativeVector3Binding("Vector3"));declare(env,"RingGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"RingGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"BufferGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"RingGeometry",constructor);{auto home=prototype;auto function=([&]() -> JSValue { auto g214=JSValue::function(function1,{env,home},"copy",1);g214.setFunctionSourceText(source2); return g214; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("copy",d);}{auto home=constructor;auto function=([&]() -> JSValue { auto g231=JSValue::function(function2,{env,home},"fromJSON",1);g231.setFunctionSourceText(source4); return g231; }());function.setFunctionSourceText(source5);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"RingGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeRingGeometryBinding(const std::string& name){return nativeRingGeometryModule().get(name);}
JSValue nativeRingGeometryConstructor(){return nativeRingGeometryBinding("RingGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
