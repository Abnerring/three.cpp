#include "geometries/SphereGeometry.h"
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
SphereGeometry::SphereGeometry(double radius, double widthSegments, double heightSegments, double phiStart, double phiLength, double thetaStart, double thetaLength)
    : parameters{radius, widthSegments, heightSegments, phiStart, phiLength, thetaStart, thetaLength} {
    type = "SphereGeometry";
    widthSegments = MathUtils::max(3, std::floor(widthSegments));
    heightSegments = MathUtils::max(2, std::floor(heightSegments));
    requireTerminating(widthSegments);
    requireTerminating(heightSegments);
    const double thetaEnd = MathUtils::min(thetaStart + thetaLength, pi);
    double index = 0;
    std::vector<std::vector<double>> grid;
    std::vector<double> indices, vertices, normals, uvs;
    Vector3 vertex, normal;
    for (double iy = 0; iy <= heightSegments; ++iy) {
        std::vector<double> row;
        const double v = iy / heightSegments;
        const double theta = thetaStart + v * thetaLength;
        const double y = radius * JSNumberMath::cos(theta);
        const double ringRadius = std::sqrt(radius * radius - y * y);
        double uOffset = 0;
        if (iy == 0 && thetaStart == 0) uOffset = 0.5 / widthSegments;
        else if (iy == heightSegments && thetaEnd == pi) uOffset = -0.5 / widthSegments;
        for (double ix = 0; ix <= widthSegments; ++ix) {
            const double u = ix / widthSegments;
            const double phi = phiStart + u * phiLength;
            vertex.set(-ringRadius * JSNumberMath::cos(phi), y, ringRadius * JSNumberMath::sin(phi));
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normal.copy(vertex).normalize();
            normals.insert(normals.end(), {normal.x, normal.y, normal.z});
            uvs.insert(uvs.end(), {u + uOffset, 1 - v});
            row.push_back(index++);
        }
        grid.push_back(std::move(row));
    }
    for (std::size_t iy = 0; double(iy) < heightSegments; ++iy) {
        for (std::size_t ix = 0; double(ix) < widthSegments; ++ix) {
            const double a = grid[iy][ix + 1], b = grid[iy][ix],
                         c = grid[iy + 1][ix], d = grid[iy + 1][ix + 1];
            if (iy != 0 || thetaStart > 0) indices.insert(indices.end(), {a, b, d});
            if (double(iy) != heightSegments - 1 || thetaEnd < pi) indices.insert(indices.end(), {b, c, d});
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
SphereGeometry& SphereGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const SphereGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> SphereGeometry::clone() const {
    auto geometry = std::make_shared<SphereGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue SphereGeometry::toJSON() const {
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
    result["widthSegments"] = parameters.widthSegments;
    result["heightSegments"] = parameters.heightSegments;
    result["phiStart"] = parameters.phiStart;
    result["phiLength"] = parameters.phiLength;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<SphereGeometry> SphereGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<SphereGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "widthSegments", 32), jsonNumber(data, "heightSegments", 16), jsonNumber(data, "phiStart", 0), jsonNumber(data, "phiLength", std::numbers::pi * 2), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/SphereGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "math/Vector3.h"
namespace SphereGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class SphereGeometry extends BufferGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new sphere geometry.\r\n\t *\r\n\t * @param {number} [radius=1] - The sphere radius.\r\n\t * @param {number} [widthSegments=32] - The number of horizontal segments. Minimum value is `3`.\r\n\t * @param {number} [heightSegments=16] - The number of vertical segments. Minimum value is `2`.\r\n\t * @param {number} [phiStart=0] - The horizontal starting angle in radians.\r\n\t * @param {number} [phiLength=Math.PI*2] - The horizontal sweep angle size.\r\n\t * @param {number} [thetaStart=0] - The vertical starting angle in radians.\r\n\t * @param {number} [thetaLength=Math.PI] - The vertical sweep angle size.\r\n\t */\r\n\tconstructor( radius = 1, widthSegments = 32, heightSegments = 16, phiStart = 0, phiLength = Math.PI * 2, thetaStart = 0, thetaLength = Math.PI ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'SphereGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\twidthSegments: widthSegments,\r\n\t\t\theightSegments: heightSegments,\r\n\t\t\tphiStart: phiStart,\r\n\t\t\tphiLength: phiLength,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength\r\n\t\t};\r\n\r\n\t\twidthSegments = Math.max( 3, Math.floor( widthSegments ) );\r\n\t\theightSegments = Math.max( 2, Math.floor( heightSegments ) );\r\n\r\n\t\tconst thetaEnd = Math.min( thetaStart + thetaLength, Math.PI );\r\n\r\n\t\tlet index = 0;\r\n\t\tconst grid = [];\r\n\r\n\t\tconst vertex = new Vector3();\r\n\t\tconst normal = new Vector3();\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// generate vertices, normals and uvs\r\n\r\n\t\tfor ( let iy = 0; iy <= heightSegments; iy ++ ) {\r\n\r\n\t\t\tconst verticesRow = [];\r\n\r\n\t\t\tconst v = iy / heightSegments;\r\n\t\t\tconst theta = thetaStart + v * thetaLength;\r\n\r\n\t\t\tconst y = radius * Math.cos( theta );\r\n\t\t\tconst ringRadius = Math.sqrt( radius * radius - y * y );\r\n\r\n\t\t\t// special case for the poles\r\n\r\n\t\t\tlet uOffset = 0;\r\n\r\n\t\t\tif ( iy === 0 && thetaStart === 0 ) {\r\n\r\n\t\t\t\tuOffset = 0.5 / widthSegments;\r\n\r\n\t\t\t} else if ( iy === heightSegments && thetaEnd === Math.PI ) {\r\n\r\n\t\t\t\tuOffset = - 0.5 / widthSegments;\r\n\r\n\t\t\t}\r\n\r\n\t\t\tfor ( let ix = 0; ix <= widthSegments; ix ++ ) {\r\n\r\n\t\t\t\tconst u = ix / widthSegments;\r\n\t\t\t\tconst phi = phiStart + u * phiLength;\r\n\r\n\t\t\t\t// vertex\r\n\r\n\t\t\t\tvertex.x = - ringRadius * Math.cos( phi );\r\n\t\t\t\tvertex.y = y;\r\n\t\t\t\tvertex.z = ringRadius * Math.sin( phi );\r\n\r\n\t\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t\t// normal\r\n\r\n\t\t\t\tnormal.copy( vertex ).normalize();\r\n\t\t\t\tnormals.push( normal.x, normal.y, normal.z );\r\n\r\n\t\t\t\t// uv\r\n\r\n\t\t\t\tuvs.push( u + uOffset, 1 - v );\r\n\r\n\t\t\t\tverticesRow.push( index ++ );\r\n\r\n\t\t\t}\r\n\r\n\t\t\tgrid.push( verticesRow );\r\n\r\n\t\t}\r\n\r\n\t\t// indices\r\n\r\n\t\tfor ( let iy = 0; iy < heightSegments; iy ++ ) {\r\n\r\n\t\t\tfor ( let ix = 0; ix < widthSegments; ix ++ ) {\r\n\r\n\t\t\t\tconst a = grid[ iy ][ ix + 1 ];\r\n\t\t\t\tconst b = grid[ iy ][ ix ];\r\n\t\t\t\tconst c = grid[ iy + 1 ][ ix ];\r\n\t\t\t\tconst d = grid[ iy + 1 ][ ix + 1 ];\r\n\r\n\t\t\t\tif ( iy !== 0 || thetaStart > 0 ) indices.push( a, b, d );\r\n\t\t\t\tif ( iy !== heightSegments - 1 || thetaEnd < Math.PI ) indices.push( b, c, d );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}\r\n\r\n\tcopy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {SphereGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new SphereGeometry( data.radius, data.widthSegments, data.heightSegments, data.phiStart, data.phiLength, data.thetaStart, data.thetaLength );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( radius = 1, widthSegments = 32, heightSegments = 16, phiStart = 0, phiLength = Math.PI * 2, thetaStart = 0, thetaLength = Math.PI ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'SphereGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\twidthSegments: widthSegments,\r\n\t\t\theightSegments: heightSegments,\r\n\t\t\tphiStart: phiStart,\r\n\t\t\tphiLength: phiLength,\r\n\t\t\tthetaStart: thetaStart,\r\n\t\t\tthetaLength: thetaLength\r\n\t\t};\r\n\r\n\t\twidthSegments = Math.max( 3, Math.floor( widthSegments ) );\r\n\t\theightSegments = Math.max( 2, Math.floor( heightSegments ) );\r\n\r\n\t\tconst thetaEnd = Math.min( thetaStart + thetaLength, Math.PI );\r\n\r\n\t\tlet index = 0;\r\n\t\tconst grid = [];\r\n\r\n\t\tconst vertex = new Vector3();\r\n\t\tconst normal = new Vector3();\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// generate vertices, normals and uvs\r\n\r\n\t\tfor ( let iy = 0; iy <= heightSegments; iy ++ ) {\r\n\r\n\t\t\tconst verticesRow = [];\r\n\r\n\t\t\tconst v = iy / heightSegments;\r\n\t\t\tconst theta = thetaStart + v * thetaLength;\r\n\r\n\t\t\tconst y = radius * Math.cos( theta );\r\n\t\t\tconst ringRadius = Math.sqrt( radius * radius - y * y );\r\n\r\n\t\t\t// special case for the poles\r\n\r\n\t\t\tlet uOffset = 0;\r\n\r\n\t\t\tif ( iy === 0 && thetaStart === 0 ) {\r\n\r\n\t\t\t\tuOffset = 0.5 / widthSegments;\r\n\r\n\t\t\t} else if ( iy === heightSegments && thetaEnd === Math.PI ) {\r\n\r\n\t\t\t\tuOffset = - 0.5 / widthSegments;\r\n\r\n\t\t\t}\r\n\r\n\t\t\tfor ( let ix = 0; ix <= widthSegments; ix ++ ) {\r\n\r\n\t\t\t\tconst u = ix / widthSegments;\r\n\t\t\t\tconst phi = phiStart + u * phiLength;\r\n\r\n\t\t\t\t// vertex\r\n\r\n\t\t\t\tvertex.x = - ringRadius * Math.cos( phi );\r\n\t\t\t\tvertex.y = y;\r\n\t\t\t\tvertex.z = ringRadius * Math.sin( phi );\r\n\r\n\t\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t\t// normal\r\n\r\n\t\t\t\tnormal.copy( vertex ).normalize();\r\n\t\t\t\tnormals.push( normal.x, normal.y, normal.z );\r\n\r\n\t\t\t\t// uv\r\n\r\n\t\t\t\tuvs.push( u + uOffset, 1 - v );\r\n\r\n\t\t\t\tverticesRow.push( index ++ );\r\n\r\n\t\t\t}\r\n\r\n\t\t\tgrid.push( verticesRow );\r\n\r\n\t\t}\r\n\r\n\t\t// indices\r\n\r\n\t\tfor ( let iy = 0; iy < heightSegments; iy ++ ) {\r\n\r\n\t\t\tfor ( let ix = 0; ix < widthSegments; ix ++ ) {\r\n\r\n\t\t\t\tconst a = grid[ iy ][ ix + 1 ];\r\n\t\t\t\tconst b = grid[ iy ][ ix ];\r\n\t\t\t\tconst c = grid[ iy + 1 ][ ix ];\r\n\t\t\t\tconst d = grid[ iy + 1 ][ ix + 1 ];\r\n\r\n\t\t\t\tif ( iy !== 0 || thetaStart > 0 ) indices.push( a, b, d );\r\n\t\t\t\tif ( iy !== heightSegments - 1 || thetaEnd < Math.PI ) indices.push( b, c, d );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}";
inline constexpr char source2[]="( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source3[]="copy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source4[]="( data ) {\r\n\r\n\t\treturn new SphereGeometry( data.radius, data.widthSegments, data.heightSegments, data.phiStart, data.phiLength, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
inline constexpr char source5[]="fromJSON( data ) {\r\n\r\n\t\treturn new SphereGeometry( data.radius, data.widthSegments, data.heightSegments, data.phiStart, data.phiLength, data.thetaStart, data.thetaLength );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function2(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"SphereGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"radius",true);declare(env,"widthSegments",true);declare(env,"heightSegments",true);declare(env,"phiStart",true);declare(env,"phiLength",true);declare(env,"thetaStart",true);declare(env,"thetaLength",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(1.0);initialize(env,"radius",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(32.0);initialize(env,"widthSegments",g2);auto g3=arg(args,2);if(g3.isUndefined())g3=JSValue(16.0);initialize(env,"heightSegments",g3);auto g4=arg(args,3);if(g4.isUndefined())g4=JSValue(0.0);initialize(env,"phiStart",g4);auto g5=arg(args,4);if(g5.isUndefined())g5=([&]() -> JSValue { const auto g6=([&]() -> JSValue { const auto g8=read(env,"Math");const auto g9=JSValue("PI"); return g8.get(g9); }());const auto g7=JSValue(2.0); return binary("*",g6,g7); }());initialize(env,"phiLength",g5);auto g10=arg(args,5);if(g10.isUndefined())g10=JSValue(0.0);initialize(env,"thetaStart",g10);auto g11=arg(args,6);if(g11.isUndefined())g11=([&]() -> JSValue { const auto g12=read(env,"Math");const auto g13=JSValue("PI"); return g12.get(g13); }());initialize(env,"thetaLength",g11);{auto g14=env;auto env=scope(g14);declare(env,"thetaEnd",false);declare(env,"index",true);declare(env,"grid",false);declare(env,"vertex",false);declare(env,"normal",false);declare(env,"indices",false);declare(env,"vertices",false);declare(env,"normals",false);declare(env,"uvs",false);([&]() -> JSValue { const auto g15=home.getPrototype();Args g17;const auto instance=g15.construct(g17,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g18=read(env,"$this");const auto g19=JSValue("type");const auto g20=JSValue("SphereGeometry"); return setReference(g18,g19,g20); }());
([&]() -> JSValue { auto g21=read(env,"$this");const auto g22=JSValue("parameters");const auto g23=([&]() -> JSValue { auto g24=JSValue::object();const auto g25=JSValue("radius");own(g24,g25,read(env,"radius"));const auto g26=JSValue("widthSegments");own(g24,g26,read(env,"widthSegments"));const auto g27=JSValue("heightSegments");own(g24,g27,read(env,"heightSegments"));const auto g28=JSValue("phiStart");own(g24,g28,read(env,"phiStart"));const auto g29=JSValue("phiLength");own(g24,g29,read(env,"phiLength"));const auto g30=JSValue("thetaStart");own(g24,g30,read(env,"thetaStart"));const auto g31=JSValue("thetaLength");own(g24,g31,read(env,"thetaLength")); return g24; }()); return setReference(g21,g22,g23); }());
([&]() -> JSValue { const auto g34=([&]() -> JSValue { const auto g36=read(env,"Math");const auto g35=g36.get("max");Args g37;g37.push_back(JSValue(3.0));g37.push_back(([&]() -> JSValue { const auto g39=read(env,"Math");const auto g38=g39.get("floor");Args g40;g40.push_back(read(env,"widthSegments")); return g38.call(g39,g40); }())); return g35.call(g36,g37); }()); return write(env,"widthSegments",g34); }());
([&]() -> JSValue { const auto g43=([&]() -> JSValue { const auto g45=read(env,"Math");const auto g44=g45.get("max");Args g46;g46.push_back(JSValue(2.0));g46.push_back(([&]() -> JSValue { const auto g48=read(env,"Math");const auto g47=g48.get("floor");Args g49;g49.push_back(read(env,"heightSegments")); return g47.call(g48,g49); }())); return g44.call(g45,g46); }()); return write(env,"heightSegments",g43); }());
initialize(env,"thetaEnd",([&]() -> JSValue { const auto g51=read(env,"Math");const auto g50=g51.get("min");Args g52;g52.push_back(([&]() -> JSValue { const auto g53=read(env,"thetaStart");const auto g54=read(env,"thetaLength"); return binary("+",g53,g54); }()));g52.push_back(([&]() -> JSValue { const auto g55=read(env,"Math");const auto g56=JSValue("PI"); return g55.get(g56); }())); return g50.call(g51,g52); }()));
initialize(env,"index",JSValue(0.0));
initialize(env,"grid",([&]() -> JSValue { auto g57=JSValue::array(0); return g57; }()));
initialize(env,"vertex",([&]() -> JSValue { const auto g58=read(env,"Vector3");Args g60; return g58.construct(g60); }()));
initialize(env,"normal",([&]() -> JSValue { const auto g61=read(env,"Vector3");Args g63; return g61.construct(g63); }()));
initialize(env,"indices",([&]() -> JSValue { auto g64=JSValue::array(0); return g64; }()));
initialize(env,"vertices",([&]() -> JSValue { auto g65=JSValue::array(0); return g65; }()));
initialize(env,"normals",([&]() -> JSValue { auto g66=JSValue::array(0); return g66; }()));
initialize(env,"uvs",([&]() -> JSValue { auto g67=JSValue::array(0); return g67; }()));
{auto g68=env;auto env=scope(g68);declare(env,"iy",true);initialize(env,"iy",JSValue(0.0));for(;([&]() -> JSValue { const auto g74=read(env,"iy");const auto g75=read(env,"heightSegments"); return binary("<=",g74,g75); }()).toBoolean();[&](){auto g69=scope(g68);declare(g69,"iy",true);initialize(g69,"iy",read(env,"iy"));env=g69;([&]() -> JSValue { const auto g70=read(env,"iy").toNumber();const JSValue g71(g70+1);write(env,"iy",g71); return JSValue(g70); }());}()){{auto g76=env;auto env=scope(g76);declare(env,"verticesRow",false);declare(env,"v",false);declare(env,"theta",false);declare(env,"y",false);declare(env,"ringRadius",false);declare(env,"uOffset",true);initialize(env,"verticesRow",([&]() -> JSValue { auto g77=JSValue::array(0); return g77; }()));
initialize(env,"v",([&]() -> JSValue { const auto g78=read(env,"iy");const auto g79=read(env,"heightSegments"); return binary("/",g78,g79); }()));
initialize(env,"theta",([&]() -> JSValue { const auto g80=read(env,"thetaStart");const auto g81=([&]() -> JSValue { const auto g82=read(env,"v");const auto g83=read(env,"thetaLength"); return binary("*",g82,g83); }()); return binary("+",g80,g81); }()));
initialize(env,"y",([&]() -> JSValue { const auto g84=read(env,"radius");const auto g85=([&]() -> JSValue { const auto g87=read(env,"Math");const auto g86=g87.get("cos");Args g88;g88.push_back(read(env,"theta")); return g86.call(g87,g88); }()); return binary("*",g84,g85); }()));
initialize(env,"ringRadius",([&]() -> JSValue { const auto g90=read(env,"Math");const auto g89=g90.get("sqrt");Args g91;g91.push_back(([&]() -> JSValue { const auto g92=([&]() -> JSValue { const auto g94=read(env,"radius");const auto g95=read(env,"radius"); return binary("*",g94,g95); }());const auto g93=([&]() -> JSValue { const auto g96=read(env,"y");const auto g97=read(env,"y"); return binary("*",g96,g97); }()); return binary("-",g92,g93); }())); return g89.call(g90,g91); }()));
initialize(env,"uOffset",JSValue(0.0));
if(([&]() -> JSValue { const auto g98=([&]() -> JSValue { const auto g99=read(env,"iy");const auto g100=JSValue(0.0); return binary("===",g99,g100); }());if(!g98.toBoolean())return g98; return ([&]() -> JSValue { const auto g101=read(env,"thetaStart");const auto g102=JSValue(0.0); return binary("===",g101,g102); }()); }()).toBoolean()){{auto g103=env;auto env=scope(g103);([&]() -> JSValue { const auto g106=([&]() -> JSValue { const auto g107=JSValue(0.5);const auto g108=read(env,"widthSegments"); return binary("/",g107,g108); }()); return write(env,"uOffset",g106); }());}}else{if(([&]() -> JSValue { const auto g109=([&]() -> JSValue { const auto g110=read(env,"iy");const auto g111=read(env,"heightSegments"); return binary("===",g110,g111); }());if(!g109.toBoolean())return g109; return ([&]() -> JSValue { const auto g112=read(env,"thetaEnd");const auto g113=([&]() -> JSValue { const auto g114=read(env,"Math");const auto g115=JSValue("PI"); return g114.get(g115); }()); return binary("===",g112,g113); }()); }()).toBoolean()){{auto g116=env;auto env=scope(g116);([&]() -> JSValue { const auto g119=([&]() -> JSValue { const auto g120=JSValue(-JSValue(0.5).toNumber());const auto g121=read(env,"widthSegments"); return binary("/",g120,g121); }()); return write(env,"uOffset",g119); }());}}}
{auto g123=env;auto env=scope(g123);declare(env,"ix",true);initialize(env,"ix",JSValue(0.0));for(;([&]() -> JSValue { const auto g129=read(env,"ix");const auto g130=read(env,"widthSegments"); return binary("<=",g129,g130); }()).toBoolean();[&](){auto g124=scope(g123);declare(g124,"ix",true);initialize(g124,"ix",read(env,"ix"));env=g124;([&]() -> JSValue { const auto g125=read(env,"ix").toNumber();const JSValue g126(g125+1);write(env,"ix",g126); return JSValue(g125); }());}()){{auto g131=env;auto env=scope(g131);declare(env,"u",false);declare(env,"phi",false);initialize(env,"u",([&]() -> JSValue { const auto g132=read(env,"ix");const auto g133=read(env,"widthSegments"); return binary("/",g132,g133); }()));
initialize(env,"phi",([&]() -> JSValue { const auto g134=read(env,"phiStart");const auto g135=([&]() -> JSValue { const auto g136=read(env,"u");const auto g137=read(env,"phiLength"); return binary("*",g136,g137); }()); return binary("+",g134,g135); }()));
([&]() -> JSValue { auto g138=read(env,"vertex");const auto g139=JSValue("x");const auto g140=([&]() -> JSValue { const auto g141=JSValue(-read(env,"ringRadius").toNumber());const auto g142=([&]() -> JSValue { const auto g145=read(env,"Math");const auto g144=g145.get("cos");Args g146;g146.push_back(read(env,"phi")); return g144.call(g145,g146); }()); return binary("*",g141,g142); }()); return setReference(g138,g139,g140); }());
([&]() -> JSValue { auto g147=read(env,"vertex");const auto g148=JSValue("y");const auto g149=read(env,"y"); return setReference(g147,g148,g149); }());
([&]() -> JSValue { auto g150=read(env,"vertex");const auto g151=JSValue("z");const auto g152=([&]() -> JSValue { const auto g153=read(env,"ringRadius");const auto g154=([&]() -> JSValue { const auto g156=read(env,"Math");const auto g155=g156.get("sin");Args g157;g157.push_back(read(env,"phi")); return g155.call(g156,g157); }()); return binary("*",g153,g154); }()); return setReference(g150,g151,g152); }());
([&]() -> JSValue { const auto g159=read(env,"vertices");const auto g158=g159.get("push");Args g160;g160.push_back(([&]() -> JSValue { const auto g161=read(env,"vertex");const auto g162=JSValue("x"); return g161.get(g162); }()));g160.push_back(([&]() -> JSValue { const auto g163=read(env,"vertex");const auto g164=JSValue("y"); return g163.get(g164); }()));g160.push_back(([&]() -> JSValue { const auto g165=read(env,"vertex");const auto g166=JSValue("z"); return g165.get(g166); }())); return g158.call(g159,g160); }());
([&]() -> JSValue { const auto g168=([&]() -> JSValue { const auto g171=read(env,"normal");const auto g170=g171.get("copy");Args g172;g172.push_back(read(env,"vertex")); return g170.call(g171,g172); }());const auto g167=g168.get("normalize");Args g169; return g167.call(g168,g169); }());
([&]() -> JSValue { const auto g174=read(env,"normals");const auto g173=g174.get("push");Args g175;g175.push_back(([&]() -> JSValue { const auto g176=read(env,"normal");const auto g177=JSValue("x"); return g176.get(g177); }()));g175.push_back(([&]() -> JSValue { const auto g178=read(env,"normal");const auto g179=JSValue("y"); return g178.get(g179); }()));g175.push_back(([&]() -> JSValue { const auto g180=read(env,"normal");const auto g181=JSValue("z"); return g180.get(g181); }())); return g173.call(g174,g175); }());
([&]() -> JSValue { const auto g183=read(env,"uvs");const auto g182=g183.get("push");Args g184;g184.push_back(([&]() -> JSValue { const auto g185=read(env,"u");const auto g186=read(env,"uOffset"); return binary("+",g185,g186); }()));g184.push_back(([&]() -> JSValue { const auto g187=JSValue(1.0);const auto g188=read(env,"v"); return binary("-",g187,g188); }())); return g182.call(g183,g184); }());
([&]() -> JSValue { const auto g190=read(env,"verticesRow");const auto g189=g190.get("push");Args g191;g191.push_back(([&]() -> JSValue { const auto g192=read(env,"index").toNumber();const JSValue g193(g192+1);write(env,"index",g193); return JSValue(g192); }())); return g189.call(g190,g191); }());}}}
([&]() -> JSValue { const auto g197=read(env,"grid");const auto g196=g197.get("push");Args g198;g198.push_back(read(env,"verticesRow")); return g196.call(g197,g198); }());}}}
{auto g199=env;auto env=scope(g199);declare(env,"iy",true);initialize(env,"iy",JSValue(0.0));for(;([&]() -> JSValue { const auto g205=read(env,"iy");const auto g206=read(env,"heightSegments"); return binary("<",g205,g206); }()).toBoolean();[&](){auto g200=scope(g199);declare(g200,"iy",true);initialize(g200,"iy",read(env,"iy"));env=g200;([&]() -> JSValue { const auto g201=read(env,"iy").toNumber();const JSValue g202(g201+1);write(env,"iy",g202); return JSValue(g201); }());}()){{auto g207=env;auto env=scope(g207);{auto g208=env;auto env=scope(g208);declare(env,"ix",true);initialize(env,"ix",JSValue(0.0));for(;([&]() -> JSValue { const auto g214=read(env,"ix");const auto g215=read(env,"widthSegments"); return binary("<",g214,g215); }()).toBoolean();[&](){auto g209=scope(g208);declare(g209,"ix",true);initialize(g209,"ix",read(env,"ix"));env=g209;([&]() -> JSValue { const auto g210=read(env,"ix").toNumber();const JSValue g211(g210+1);write(env,"ix",g211); return JSValue(g210); }());}()){{auto g216=env;auto env=scope(g216);declare(env,"a",false);declare(env,"b",false);declare(env,"c",false);declare(env,"d",false);initialize(env,"a",([&]() -> JSValue { const auto g217=([&]() -> JSValue { const auto g219=read(env,"grid");const auto g220=read(env,"iy"); return g219.get(g220); }());const auto g218=([&]() -> JSValue { const auto g221=read(env,"ix");const auto g222=JSValue(1.0); return binary("+",g221,g222); }()); return g217.get(g218); }()));
initialize(env,"b",([&]() -> JSValue { const auto g223=([&]() -> JSValue { const auto g225=read(env,"grid");const auto g226=read(env,"iy"); return g225.get(g226); }());const auto g224=read(env,"ix"); return g223.get(g224); }()));
initialize(env,"c",([&]() -> JSValue { const auto g227=([&]() -> JSValue { const auto g229=read(env,"grid");const auto g230=([&]() -> JSValue { const auto g231=read(env,"iy");const auto g232=JSValue(1.0); return binary("+",g231,g232); }()); return g229.get(g230); }());const auto g228=read(env,"ix"); return g227.get(g228); }()));
initialize(env,"d",([&]() -> JSValue { const auto g233=([&]() -> JSValue { const auto g235=read(env,"grid");const auto g236=([&]() -> JSValue { const auto g237=read(env,"iy");const auto g238=JSValue(1.0); return binary("+",g237,g238); }()); return g235.get(g236); }());const auto g234=([&]() -> JSValue { const auto g239=read(env,"ix");const auto g240=JSValue(1.0); return binary("+",g239,g240); }()); return g233.get(g234); }()));
if(([&]() -> JSValue { const auto g241=([&]() -> JSValue { const auto g242=read(env,"iy");const auto g243=JSValue(0.0); return binary("!==",g242,g243); }());if(g241.toBoolean())return g241; return ([&]() -> JSValue { const auto g244=read(env,"thetaStart");const auto g245=JSValue(0.0); return binary(">",g244,g245); }()); }()).toBoolean()){([&]() -> JSValue { const auto g247=read(env,"indices");const auto g246=g247.get("push");Args g248;g248.push_back(read(env,"a"));g248.push_back(read(env,"b"));g248.push_back(read(env,"d")); return g246.call(g247,g248); }());}
if(([&]() -> JSValue { const auto g249=([&]() -> JSValue { const auto g250=read(env,"iy");const auto g251=([&]() -> JSValue { const auto g252=read(env,"heightSegments");const auto g253=JSValue(1.0); return binary("-",g252,g253); }()); return binary("!==",g250,g251); }());if(g249.toBoolean())return g249; return ([&]() -> JSValue { const auto g254=read(env,"thetaEnd");const auto g255=([&]() -> JSValue { const auto g256=read(env,"Math");const auto g257=JSValue("PI"); return g256.get(g257); }()); return binary("<",g254,g255); }()); }()).toBoolean()){([&]() -> JSValue { const auto g259=read(env,"indices");const auto g258=g259.get("push");Args g260;g260.push_back(read(env,"b"));g260.push_back(read(env,"c"));g260.push_back(read(env,"d")); return g258.call(g259,g260); }());}}}}}}}
([&]() -> JSValue { const auto g262=read(env,"$this");const auto g261=g262.get("setIndex");Args g263;g263.push_back(read(env,"indices")); return g261.call(g262,g263); }());
([&]() -> JSValue { const auto g265=read(env,"$this");const auto g264=g265.get("setAttribute");Args g266;g266.push_back(JSValue("position"));g266.push_back(([&]() -> JSValue { const auto g267=read(env,"Float32BufferAttribute");Args g269;g269.push_back(read(env,"vertices"));g269.push_back(JSValue(3.0)); return g267.construct(g269); }())); return g264.call(g265,g266); }());
([&]() -> JSValue { const auto g271=read(env,"$this");const auto g270=g271.get("setAttribute");Args g272;g272.push_back(JSValue("normal"));g272.push_back(([&]() -> JSValue { const auto g273=read(env,"Float32BufferAttribute");Args g275;g275.push_back(read(env,"normals"));g275.push_back(JSValue(3.0)); return g273.construct(g275); }())); return g270.call(g271,g272); }());
([&]() -> JSValue { const auto g277=read(env,"$this");const auto g276=g277.get("setAttribute");Args g278;g278.push_back(JSValue("uv"));g278.push_back(([&]() -> JSValue { const auto g279=read(env,"Float32BufferAttribute");Args g281;g281.push_back(read(env,"uvs"));g281.push_back(JSValue(2.0)); return g279.construct(g281); }())); return g276.call(g277,g278); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"source",true);initialize(env,"source",arg(args,0));{auto g282=env;auto env=scope(g282);([&]() -> JSValue { const auto g284=read(env,"$this");const auto g283=home.getPrototype().get("copy",g284);Args g285;g285.push_back(read(env,"source")); return g283.call(g284,g285); }());
([&]() -> JSValue { auto g286=read(env,"$this");const auto g287=JSValue("parameters");const auto g288=([&]() -> JSValue { const auto g290=read(env,"Object");const auto g289=g290.get("assign");Args g291;g291.push_back(([&]() -> JSValue { auto g292=JSValue::object(); return g292; }()));g291.push_back(([&]() -> JSValue { const auto g293=read(env,"source");const auto g294=JSValue("parameters"); return g293.get(g294); }())); return g289.call(g290,g291); }()); return setReference(g286,g287,g288); }());
return read(env,"$this");}return {};}
JSValue function2(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g296=env;auto env=scope(g296);return ([&]() -> JSValue { const auto g297=read(env,"SphereGeometry");Args g299;g299.push_back(([&]() -> JSValue { const auto g300=read(env,"data");const auto g301=JSValue("radius"); return g300.get(g301); }()));g299.push_back(([&]() -> JSValue { const auto g302=read(env,"data");const auto g303=JSValue("widthSegments"); return g302.get(g303); }()));g299.push_back(([&]() -> JSValue { const auto g304=read(env,"data");const auto g305=JSValue("heightSegments"); return g304.get(g305); }()));g299.push_back(([&]() -> JSValue { const auto g306=read(env,"data");const auto g307=JSValue("phiStart"); return g306.get(g307); }()));g299.push_back(([&]() -> JSValue { const auto g308=read(env,"data");const auto g309=JSValue("phiLength"); return g308.get(g309); }()));g299.push_back(([&]() -> JSValue { const auto g310=read(env,"data");const auto g311=JSValue("thetaStart"); return g310.get(g311); }()));g299.push_back(([&]() -> JSValue { const auto g312=read(env,"data");const auto g313=JSValue("thetaLength"); return g312.get(g313); }())); return g297.construct(g299); }());}return {};}
}
JSValue nativeSphereGeometryModule(){using namespace SphereGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"SphereGeometry");auto env=scope();JSValue home;bindImport(env,"BufferGeometry",nativeBufferGeometryConstructor());bindImport(env,"Float32BufferAttribute",nativeFloat32BufferAttributeConstructor());bindImport(env,"Vector3",nativeVector3Binding("Vector3"));declare(env,"SphereGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"SphereGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"BufferGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"SphereGeometry",constructor);{auto home=prototype;auto function=([&]() -> JSValue { auto g295=JSValue::function(function1,{env,home},"copy",1);g295.setFunctionSourceText(source2); return g295; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("copy",d);}{auto home=constructor;auto function=([&]() -> JSValue { auto g314=JSValue::function(function2,{env,home},"fromJSON",1);g314.setFunctionSourceText(source4); return g314; }());function.setFunctionSourceText(source5);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"SphereGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeSphereGeometryBinding(const std::string& name){return nativeSphereGeometryModule().get(name);}
JSValue nativeSphereGeometryConstructor(){return nativeSphereGeometryBinding("SphereGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
