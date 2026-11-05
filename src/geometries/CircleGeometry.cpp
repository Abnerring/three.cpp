#include "geometries/CircleGeometry.h"
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
CircleGeometry::CircleGeometry(double radius, double segments, double thetaStart, double thetaLength)
    : parameters{radius, segments, thetaStart, thetaLength} {
    type = "CircleGeometry";
    segments = MathUtils::max(3, segments);
    requireTerminating(segments);
    std::vector<double> indices, vertices{0, 0, 0}, normals{0, 0, 1}, uvs{0.5, 0.5};
    Vector3 vertex;
    for (double s = 0; s <= segments; ++s) {
        const double segment = thetaStart + s / segments * thetaLength;
        vertex.x = radius * JSNumberMath::cos(segment);
        vertex.y = radius * JSNumberMath::sin(segment);
        vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
        normals.insert(normals.end(), {0, 0, 1});
        uvs.insert(uvs.end(), {(vertex.x / radius + 1) / 2, (vertex.y / radius + 1) / 2});
    }
    for (double i = 1; i <= segments; ++i)
        indices.insert(indices.end(), {i, i + 1, 0});
    finish(*this, indices, vertices, normals, uvs);
}
CircleGeometry& CircleGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const CircleGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> CircleGeometry::clone() const {
    auto geometry = std::make_shared<CircleGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue CircleGeometry::toJSON() const {
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
    result["segments"] = parameters.segments;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<CircleGeometry> CircleGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<CircleGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "segments", 32), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/CircleGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "math/Vector3.h"
#include "math/Vector2.h"
namespace CircleGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class CircleGeometry extends BufferGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new circle geometry.\r\n\t *\r\n\t * @param {number} [radius=1] - Radius of the circle.\r\n\t * @param {number} [segments=32] - Number of segments (triangles), minimum = `3`.\r\n\t * @param {number} [thetaStart=0] - Start angle for first segment in radians.\r\n\t * @param {number} [thetaLength=Math.PI*2] - The central angle, often called theta,\r\n\t * of the circular sector in radians. The default value results in a complete circle.\r\n\t */\r\n\tconstructor( radius = 1, segments = 32, thetaStart = 0, thetaLength = Math.PI * 2 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'CircleGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\tsegments: segments,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength\r\n\t\t};\r\n\r\n\t\tsegments = Math.max( 3, segments );\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// helper variables\r\n\r\n\t\tconst vertex = new Vector3();\r\n\t\tconst uv = new Vector2();\r\n\r\n\t\t// center point\r\n\r\n\t\tvertices.push( 0, 0, 0 );\r\n\t\tnormals.push( 0, 0, 1 );\r\n\t\tuvs.push( 0.5, 0.5 );\r\n\r\n\t\tfor ( let s = 0, i = 3; s <= segments; s ++, i += 3 ) {\r\n\r\n\t\t\tconst segment = thetaStart + s / segments * thetaLength;\r\n\r\n\t\t\t// vertex\r\n\r\n\t\t\tvertex.x = radius * Math.cos( segment );\r\n\t\t\tvertex.y = radius * Math.sin( segment );\r\n\r\n\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t// normal\r\n\r\n\t\t\tnormals.push( 0, 0, 1 );\r\n\r\n\t\t\t// uvs\r\n\r\n\t\t\tuv.x = ( vertices[ i ] / radius + 1 ) / 2;\r\n\t\t\tuv.y = ( vertices[ i + 1 ] / radius + 1 ) / 2;\r\n\r\n\t\t\tuvs.push( uv.x, uv.y );\r\n\r\n\t\t}\r\n\r\n\t\t// indices\r\n\r\n\t\tfor ( let i = 1; i <= segments; i ++ ) {\r\n\r\n\t\t\tindices.push( i, i + 1, 0 );\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}\r\n\r\n\tcopy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {CircleGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new CircleGeometry( data.radius, data.segments, data.thetaStart, data.thetaLength );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( radius = 1, segments = 32, thetaStart = 0, thetaLength = Math.PI * 2 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'CircleGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\tsegments: segments,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength\r\n\t\t};\r\n\r\n\t\tsegments = Math.max( 3, segments );\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// helper variables\r\n\r\n\t\tconst vertex = new Vector3();\r\n\t\tconst uv = new Vector2();\r\n\r\n\t\t// center point\r\n\r\n\t\tvertices.push( 0, 0, 0 );\r\n\t\tnormals.push( 0, 0, 1 );\r\n\t\tuvs.push( 0.5, 0.5 );\r\n\r\n\t\tfor ( let s = 0, i = 3; s <= segments; s ++, i += 3 ) {\r\n\r\n\t\t\tconst segment = thetaStart + s / segments * thetaLength;\r\n\r\n\t\t\t// vertex\r\n\r\n\t\t\tvertex.x = radius * Math.cos( segment );\r\n\t\t\tvertex.y = radius * Math.sin( segment );\r\n\r\n\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t// normal\r\n\r\n\t\t\tnormals.push( 0, 0, 1 );\r\n\r\n\t\t\t// uvs\r\n\r\n\t\t\tuv.x = ( vertices[ i ] / radius + 1 ) / 2;\r\n\t\t\tuv.y = ( vertices[ i + 1 ] / radius + 1 ) / 2;\r\n\r\n\t\t\tuvs.push( uv.x, uv.y );\r\n\r\n\t\t}\r\n\r\n\t\t// indices\r\n\r\n\t\tfor ( let i = 1; i <= segments; i ++ ) {\r\n\r\n\t\t\tindices.push( i, i + 1, 0 );\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}";
inline constexpr char source2[]="( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source3[]="copy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source4[]="( data ) {\r\n\r\n\t\treturn new CircleGeometry( data.radius, data.segments, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
inline constexpr char source5[]="fromJSON( data ) {\r\n\r\n\t\treturn new CircleGeometry( data.radius, data.segments, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function2(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"CircleGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"radius",true);declare(env,"segments",true);declare(env,"thetaStart",true);declare(env,"thetaLength",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(1.0);initialize(env,"radius",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(32.0);initialize(env,"segments",g2);auto g3=arg(args,2);if(g3.isUndefined())g3=JSValue(0.0);initialize(env,"thetaStart",g3);auto g4=arg(args,3);if(g4.isUndefined())g4=([&]() -> JSValue { const auto g5=([&]() -> JSValue { const auto g7=read(env,"Math");const auto g8=JSValue("PI"); return g7.get(g8); }());const auto g6=JSValue(2.0); return binary("*",g5,g6); }());initialize(env,"thetaLength",g4);{auto g9=env;auto env=scope(g9);declare(env,"indices",false);declare(env,"vertices",false);declare(env,"normals",false);declare(env,"uvs",false);declare(env,"vertex",false);declare(env,"uv",false);([&]() -> JSValue { const auto g10=home.getPrototype();Args g12;const auto instance=g10.construct(g12,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g13=read(env,"$this");const auto g14=JSValue("type");const auto g15=JSValue("CircleGeometry"); return setReference(g13,g14,g15); }());
([&]() -> JSValue { auto g16=read(env,"$this");const auto g17=JSValue("parameters");const auto g18=([&]() -> JSValue { auto g19=JSValue::object();const auto g20=JSValue("radius");own(g19,g20,read(env,"radius"));const auto g21=JSValue("segments");own(g19,g21,read(env,"segments"));const auto g22=JSValue("thetaStart");own(g19,g22,read(env,"thetaStart"));const auto g23=JSValue("thetaLength");own(g19,g23,read(env,"thetaLength")); return g19; }()); return setReference(g16,g17,g18); }());
([&]() -> JSValue { const auto g26=([&]() -> JSValue { const auto g28=read(env,"Math");const auto g27=g28.get("max");Args g29;g29.push_back(JSValue(3.0));g29.push_back(read(env,"segments")); return g27.call(g28,g29); }()); return write(env,"segments",g26); }());
initialize(env,"indices",([&]() -> JSValue { auto g30=JSValue::array(0); return g30; }()));
initialize(env,"vertices",([&]() -> JSValue { auto g31=JSValue::array(0); return g31; }()));
initialize(env,"normals",([&]() -> JSValue { auto g32=JSValue::array(0); return g32; }()));
initialize(env,"uvs",([&]() -> JSValue { auto g33=JSValue::array(0); return g33; }()));
initialize(env,"vertex",([&]() -> JSValue { const auto g34=read(env,"Vector3");Args g36; return g34.construct(g36); }()));
initialize(env,"uv",([&]() -> JSValue { const auto g37=read(env,"Vector2");Args g39; return g37.construct(g39); }()));
([&]() -> JSValue { const auto g41=read(env,"vertices");const auto g40=g41.get("push");Args g42;g42.push_back(JSValue(0.0));g42.push_back(JSValue(0.0));g42.push_back(JSValue(0.0)); return g40.call(g41,g42); }());
([&]() -> JSValue { const auto g44=read(env,"normals");const auto g43=g44.get("push");Args g45;g45.push_back(JSValue(0.0));g45.push_back(JSValue(0.0));g45.push_back(JSValue(1.0)); return g43.call(g44,g45); }());
([&]() -> JSValue { const auto g47=read(env,"uvs");const auto g46=g47.get("push");Args g48;g48.push_back(JSValue(0.5));g48.push_back(JSValue(0.5)); return g46.call(g47,g48); }());
{auto g49=env;auto env=scope(g49);declare(env,"s",true);declare(env,"i",true);initialize(env,"s",JSValue(0.0));initialize(env,"i",JSValue(3.0));for(;([&]() -> JSValue { const auto g60=read(env,"s");const auto g61=read(env,"segments"); return binary("<=",g60,g61); }()).toBoolean();[&](){auto g50=scope(g49);declare(g50,"s",true);initialize(g50,"s",read(env,"s"));declare(g50,"i",true);initialize(g50,"i",read(env,"i"));env=g50;([&]() -> JSValue { ([&]() -> JSValue { const auto g51=read(env,"s").toNumber();const JSValue g52(g51+1);write(env,"s",g52); return JSValue(g51); }()); return ([&]() -> JSValue { const auto g58=read(env,"i");const auto g59=JSValue(3.0);const auto g57=binary("+",g58,g59); return write(env,"i",g57); }()); }());}()){{auto g62=env;auto env=scope(g62);declare(env,"segment",false);initialize(env,"segment",([&]() -> JSValue { const auto g63=read(env,"thetaStart");const auto g64=([&]() -> JSValue { const auto g65=([&]() -> JSValue { const auto g67=read(env,"s");const auto g68=read(env,"segments"); return binary("/",g67,g68); }());const auto g66=read(env,"thetaLength"); return binary("*",g65,g66); }()); return binary("+",g63,g64); }()));
([&]() -> JSValue { auto g69=read(env,"vertex");const auto g70=JSValue("x");const auto g71=([&]() -> JSValue { const auto g72=read(env,"radius");const auto g73=([&]() -> JSValue { const auto g75=read(env,"Math");const auto g74=g75.get("cos");Args g76;g76.push_back(read(env,"segment")); return g74.call(g75,g76); }()); return binary("*",g72,g73); }()); return setReference(g69,g70,g71); }());
([&]() -> JSValue { auto g77=read(env,"vertex");const auto g78=JSValue("y");const auto g79=([&]() -> JSValue { const auto g80=read(env,"radius");const auto g81=([&]() -> JSValue { const auto g83=read(env,"Math");const auto g82=g83.get("sin");Args g84;g84.push_back(read(env,"segment")); return g82.call(g83,g84); }()); return binary("*",g80,g81); }()); return setReference(g77,g78,g79); }());
([&]() -> JSValue { const auto g86=read(env,"vertices");const auto g85=g86.get("push");Args g87;g87.push_back(([&]() -> JSValue { const auto g88=read(env,"vertex");const auto g89=JSValue("x"); return g88.get(g89); }()));g87.push_back(([&]() -> JSValue { const auto g90=read(env,"vertex");const auto g91=JSValue("y"); return g90.get(g91); }()));g87.push_back(([&]() -> JSValue { const auto g92=read(env,"vertex");const auto g93=JSValue("z"); return g92.get(g93); }())); return g85.call(g86,g87); }());
([&]() -> JSValue { const auto g95=read(env,"normals");const auto g94=g95.get("push");Args g96;g96.push_back(JSValue(0.0));g96.push_back(JSValue(0.0));g96.push_back(JSValue(1.0)); return g94.call(g95,g96); }());
([&]() -> JSValue { auto g97=read(env,"uv");const auto g98=JSValue("x");const auto g99=([&]() -> JSValue { const auto g100=([&]() -> JSValue { const auto g102=([&]() -> JSValue { const auto g104=([&]() -> JSValue { const auto g106=read(env,"vertices");const auto g107=read(env,"i"); return g106.get(g107); }());const auto g105=read(env,"radius"); return binary("/",g104,g105); }());const auto g103=JSValue(1.0); return binary("+",g102,g103); }());const auto g101=JSValue(2.0); return binary("/",g100,g101); }()); return setReference(g97,g98,g99); }());
([&]() -> JSValue { auto g108=read(env,"uv");const auto g109=JSValue("y");const auto g110=([&]() -> JSValue { const auto g111=([&]() -> JSValue { const auto g113=([&]() -> JSValue { const auto g115=([&]() -> JSValue { const auto g117=read(env,"vertices");const auto g118=([&]() -> JSValue { const auto g119=read(env,"i");const auto g120=JSValue(1.0); return binary("+",g119,g120); }()); return g117.get(g118); }());const auto g116=read(env,"radius"); return binary("/",g115,g116); }());const auto g114=JSValue(1.0); return binary("+",g113,g114); }());const auto g112=JSValue(2.0); return binary("/",g111,g112); }()); return setReference(g108,g109,g110); }());
([&]() -> JSValue { const auto g122=read(env,"uvs");const auto g121=g122.get("push");Args g123;g123.push_back(([&]() -> JSValue { const auto g124=read(env,"uv");const auto g125=JSValue("x"); return g124.get(g125); }()));g123.push_back(([&]() -> JSValue { const auto g126=read(env,"uv");const auto g127=JSValue("y"); return g126.get(g127); }())); return g121.call(g122,g123); }());}}}
{auto g128=env;auto env=scope(g128);declare(env,"i",true);initialize(env,"i",JSValue(1.0));for(;([&]() -> JSValue { const auto g134=read(env,"i");const auto g135=read(env,"segments"); return binary("<=",g134,g135); }()).toBoolean();[&](){auto g129=scope(g128);declare(g129,"i",true);initialize(g129,"i",read(env,"i"));env=g129;([&]() -> JSValue { const auto g130=read(env,"i").toNumber();const JSValue g131(g130+1);write(env,"i",g131); return JSValue(g130); }());}()){{auto g136=env;auto env=scope(g136);([&]() -> JSValue { const auto g138=read(env,"indices");const auto g137=g138.get("push");Args g139;g139.push_back(read(env,"i"));g139.push_back(([&]() -> JSValue { const auto g140=read(env,"i");const auto g141=JSValue(1.0); return binary("+",g140,g141); }()));g139.push_back(JSValue(0.0)); return g137.call(g138,g139); }());}}}
([&]() -> JSValue { const auto g143=read(env,"$this");const auto g142=g143.get("setIndex");Args g144;g144.push_back(read(env,"indices")); return g142.call(g143,g144); }());
([&]() -> JSValue { const auto g146=read(env,"$this");const auto g145=g146.get("setAttribute");Args g147;g147.push_back(JSValue("position"));g147.push_back(([&]() -> JSValue { const auto g148=read(env,"Float32BufferAttribute");Args g150;g150.push_back(read(env,"vertices"));g150.push_back(JSValue(3.0)); return g148.construct(g150); }())); return g145.call(g146,g147); }());
([&]() -> JSValue { const auto g152=read(env,"$this");const auto g151=g152.get("setAttribute");Args g153;g153.push_back(JSValue("normal"));g153.push_back(([&]() -> JSValue { const auto g154=read(env,"Float32BufferAttribute");Args g156;g156.push_back(read(env,"normals"));g156.push_back(JSValue(3.0)); return g154.construct(g156); }())); return g151.call(g152,g153); }());
([&]() -> JSValue { const auto g158=read(env,"$this");const auto g157=g158.get("setAttribute");Args g159;g159.push_back(JSValue("uv"));g159.push_back(([&]() -> JSValue { const auto g160=read(env,"Float32BufferAttribute");Args g162;g162.push_back(read(env,"uvs"));g162.push_back(JSValue(2.0)); return g160.construct(g162); }())); return g157.call(g158,g159); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"source",true);initialize(env,"source",arg(args,0));{auto g163=env;auto env=scope(g163);([&]() -> JSValue { const auto g165=read(env,"$this");const auto g164=home.getPrototype().get("copy",g165);Args g166;g166.push_back(read(env,"source")); return g164.call(g165,g166); }());
([&]() -> JSValue { auto g167=read(env,"$this");const auto g168=JSValue("parameters");const auto g169=([&]() -> JSValue { const auto g171=read(env,"Object");const auto g170=g171.get("assign");Args g172;g172.push_back(([&]() -> JSValue { auto g173=JSValue::object(); return g173; }()));g172.push_back(([&]() -> JSValue { const auto g174=read(env,"source");const auto g175=JSValue("parameters"); return g174.get(g175); }())); return g170.call(g171,g172); }()); return setReference(g167,g168,g169); }());
return read(env,"$this");}return {};}
JSValue function2(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g177=env;auto env=scope(g177);return ([&]() -> JSValue { const auto g178=read(env,"CircleGeometry");Args g180;g180.push_back(([&]() -> JSValue { const auto g181=read(env,"data");const auto g182=JSValue("radius"); return g181.get(g182); }()));g180.push_back(([&]() -> JSValue { const auto g183=read(env,"data");const auto g184=JSValue("segments"); return g183.get(g184); }()));g180.push_back(([&]() -> JSValue { const auto g185=read(env,"data");const auto g186=JSValue("thetaStart"); return g185.get(g186); }()));g180.push_back(([&]() -> JSValue { const auto g187=read(env,"data");const auto g188=JSValue("thetaLength"); return g187.get(g188); }())); return g178.construct(g180); }());}return {};}
}
JSValue nativeCircleGeometryModule(){using namespace CircleGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"CircleGeometry");auto env=scope();JSValue home;bindImport(env,"BufferGeometry",nativeBufferGeometryConstructor());bindImport(env,"Float32BufferAttribute",nativeFloat32BufferAttributeConstructor());bindImport(env,"Vector3",nativeVector3Binding("Vector3"));bindImport(env,"Vector2",nativeVector2Binding("Vector2"));declare(env,"CircleGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"CircleGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"BufferGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"CircleGeometry",constructor);{auto home=prototype;auto function=([&]() -> JSValue { auto g176=JSValue::function(function1,{env,home},"copy",1);g176.setFunctionSourceText(source2); return g176; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("copy",d);}{auto home=constructor;auto function=([&]() -> JSValue { auto g189=JSValue::function(function2,{env,home},"fromJSON",1);g189.setFunctionSourceText(source4); return g189; }());function.setFunctionSourceText(source5);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"CircleGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeCircleGeometryBinding(const std::string& name){return nativeCircleGeometryModule().get(name);}
JSValue nativeCircleGeometryConstructor(){return nativeCircleGeometryBinding("CircleGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
