#include "geometries/CapsuleGeometry.h"
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
CapsuleGeometry::CapsuleGeometry(double radius, double height, double capSegments, double radialSegments, double heightSegments)
    : parameters{radius, height, capSegments, radialSegments, heightSegments} {
    type = "CapsuleGeometry";
    height = MathUtils::max(0, height);
    capSegments = MathUtils::max(1, std::floor(capSegments));
    radialSegments = MathUtils::max(3, std::floor(radialSegments));
    heightSegments = MathUtils::max(1, std::floor(heightSegments));
    requireTerminating(capSegments);
    requireTerminating(radialSegments);
    requireTerminating(heightSegments);
    std::vector<double> indices, vertices, normals, uvs;
    const double halfHeight = height / 2;
    const double capArcLength = (pi / 2) * radius;
    const double cylinderPartLength = height;
    const double totalArcLength = 2 * capArcLength + cylinderPartLength;
    const double numVerticalSegments = capSegments * 2 + heightSegments;
    const double verticesPerRow = radialSegments + 1;
    Vector3 normal, vertex;
    for (double iy = 0; iy <= numVerticalSegments; ++iy) {
        double currentArcLength = 0, profileY = 0, profileRadius = 0, normalYComponent = 0;
        if (iy <= capSegments) {
            const double progress = iy / capSegments;
            const double angle = (progress * pi) / 2;
            profileY = -halfHeight - radius * JSNumberMath::cos(angle);
            profileRadius = radius * JSNumberMath::sin(angle);
            normalYComponent = -radius * JSNumberMath::cos(angle);
            currentArcLength = progress * capArcLength;
        } else if (iy <= capSegments + heightSegments) {
            const double progress = (iy - capSegments) / heightSegments;
            profileY = -halfHeight + progress * height;
            profileRadius = radius;
            normalYComponent = 0;
            currentArcLength = capArcLength + progress * cylinderPartLength;
        } else {
            const double progress = (iy - capSegments - heightSegments) / capSegments;
            const double angle = (progress * pi) / 2;
            profileY = halfHeight + radius * JSNumberMath::sin(angle);
            profileRadius = radius * JSNumberMath::cos(angle);
            normalYComponent = radius * JSNumberMath::sin(angle);
            currentArcLength = capArcLength + cylinderPartLength + progress * capArcLength;
        }
        const double v = MathUtils::max(0, MathUtils::min(1, currentArcLength / totalArcLength));
        double uOffset = 0;
        if (iy == 0) uOffset = 0.5 / radialSegments;
        else if (iy == numVerticalSegments) uOffset = -0.5 / radialSegments;
        for (double ix = 0; ix <= radialSegments; ++ix) {
            const double u = ix / radialSegments;
            const double theta = u * pi * 2;
            const double sinTheta = JSNumberMath::sin(theta), cosTheta = JSNumberMath::cos(theta);
            vertex.set(-profileRadius * cosTheta, profileY, profileRadius * sinTheta);
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normal.set(-profileRadius * cosTheta, normalYComponent, profileRadius * sinTheta).normalize();
            normals.insert(normals.end(), {normal.x, normal.y, normal.z});
            uvs.insert(uvs.end(), {u + uOffset, v});
        }
        if (iy > 0) {
            const double prevIndexRow = (iy - 1) * verticesPerRow;
            for (double ix = 0; ix < radialSegments; ++ix) {
                const double i1 = prevIndexRow + ix, i2 = prevIndexRow + ix + 1,
                             i3 = iy * verticesPerRow + ix, i4 = iy * verticesPerRow + ix + 1;
                indices.insert(indices.end(), {i1, i2, i3, i2, i4, i3});
            }
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
CapsuleGeometry& CapsuleGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const CapsuleGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> CapsuleGeometry::clone() const {
    auto geometry = std::make_shared<CapsuleGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue CapsuleGeometry::toJSON() const {
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
    result["capSegments"] = parameters.capSegments;
    result["radialSegments"] = parameters.radialSegments;
    result["heightSegments"] = parameters.heightSegments;
    return result;
}
std::shared_ptr<CapsuleGeometry> CapsuleGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<CapsuleGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "height", 1), jsonNumber(data, "capSegments", 4), jsonNumber(data, "radialSegments", 8), jsonNumber(data, "heightSegments", 1));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/CapsuleGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "math/Vector3.h"
namespace CapsuleGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class CapsuleGeometry extends BufferGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new capsule geometry.\r\n\t *\r\n\t * @param {number} [radius=1] - Radius of the capsule.\r\n\t * @param {number} [height=1] - Height of the middle section.\r\n\t * @param {number} [capSegments=4] - Number of curve segments used to build each cap.\r\n\t * @param {number} [radialSegments=8] - Number of segmented faces around the circumference of the capsule. Must be an integer >= 3.\r\n\t * @param {number} [heightSegments=1] - Number of rows of faces along the height of the middle section. Must be an integer >= 1.\r\n\t */\r\n\tconstructor( radius = 1, height = 1, capSegments = 4, radialSegments = 8, heightSegments = 1 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'CapsuleGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\theight: height,\r\n\t\t\tcapSegments: capSegments,\r\n\t\t\tradialSegments: radialSegments,\r\n\t\t\theightSegments: heightSegments,\r\n\t\t};\r\n\r\n\t\theight = Math.max( 0, height );\r\n\t\tcapSegments = Math.max( 1, Math.floor( capSegments ) );\r\n\t\tradialSegments = Math.max( 3, Math.floor( radialSegments ) );\r\n\t\theightSegments = Math.max( 1, Math.floor( heightSegments ) );\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// helper variables\r\n\r\n\t\tconst halfHeight = height / 2;\r\n\t\tconst capArcLength = ( Math.PI / 2 ) * radius;\r\n\t\tconst cylinderPartLength = height;\r\n\t\tconst totalArcLength = 2 * capArcLength + cylinderPartLength;\r\n\r\n\t\tconst numVerticalSegments = capSegments * 2 + heightSegments;\r\n\t\tconst verticesPerRow = radialSegments + 1;\r\n\r\n\t\tconst normal = new Vector3();\r\n\t\tconst vertex = new Vector3();\r\n\r\n\t\t// generate vertices, normals, and uvs\r\n\r\n\t\tfor ( let iy = 0; iy <= numVerticalSegments; iy ++ ) {\r\n\r\n\t\t\tlet currentArcLength = 0;\r\n\t\t\tlet profileY = 0;\r\n\t\t\tlet profileRadius = 0;\r\n\t\t\tlet normalYComponent = 0;\r\n\r\n\t\t\tif ( iy <= capSegments ) {\r\n\r\n\t\t\t\t// bottom cap\r\n\t\t\t\tconst segmentProgress = iy / capSegments;\r\n\t\t\t\tconst angle = ( segmentProgress * Math.PI ) / 2;\r\n\t\t\t\tprofileY = - halfHeight - radius * Math.cos( angle );\r\n\t\t\t\tprofileRadius = radius * Math.sin( angle );\r\n\t\t\t\tnormalYComponent = - radius * Math.cos( angle );\r\n\t\t\t\tcurrentArcLength = segmentProgress * capArcLength;\r\n\r\n\t\t\t} else if ( iy <= capSegments + heightSegments ) {\r\n\r\n\t\t\t\t// middle section\r\n\t\t\t\tconst segmentProgress = ( iy - capSegments ) / heightSegments;\r\n\t\t\t\tprofileY = - halfHeight + segmentProgress * height;\r\n\t\t\t\tprofileRadius = radius;\r\n\t\t\t\tnormalYComponent = 0;\r\n\t\t\t\tcurrentArcLength = capArcLength + segmentProgress * cylinderPartLength;\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\t// top cap\r\n\t\t\t\tconst segmentProgress =\r\n\t\t\t\t\t( iy - capSegments - heightSegments ) / capSegments;\r\n\t\t\t\tconst angle = ( segmentProgress * Math.PI ) / 2;\r\n\t\t\t\tprofileY = halfHeight + radius * Math.sin( angle );\r\n\t\t\t\tprofileRadius = radius * Math.cos( angle );\r\n\t\t\t\tnormalYComponent = radius * Math.sin( angle );\r\n\t\t\t\tcurrentArcLength =\r\n\t\t\t\t\tcapArcLength + cylinderPartLength + segmentProgress * capArcLength;\r\n\r\n\t\t\t}\r\n\r\n\t\t\tconst v = Math.max( 0, Math.min( 1, currentArcLength / totalArcLength ) );\r\n\r\n\r\n\t\t\t// special case for the poles\r\n\r\n\t\t\tlet uOffset = 0;\r\n\r\n\t\t\tif ( iy === 0 ) {\r\n\r\n\t\t\t\tuOffset = 0.5 / radialSegments;\r\n\r\n\t\t\t} else if ( iy === numVerticalSegments ) {\r\n\r\n\t\t\t\tuOffset = - 0.5 / radialSegments;\r\n\r\n\t\t\t}\r\n\r\n\t\t\tfor ( let ix = 0; ix <= radialSegments; ix ++ ) {\r\n\r\n\t\t\t\tconst u = ix / radialSegments;\r\n\t\t\t\tconst theta = u * Math.PI * 2;\r\n\r\n\t\t\t\tconst sinTheta = Math.sin( theta );\r\n\t\t\t\tconst cosTheta = Math.cos( theta );\r\n\r\n\t\t\t\t// vertex\r\n\r\n\t\t\t\tvertex.x = - profileRadius * cosTheta;\r\n\t\t\t\tvertex.y = profileY;\r\n\t\t\t\tvertex.z = profileRadius * sinTheta;\r\n\t\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t\t// normal\r\n\r\n\t\t\t\tnormal.set(\r\n\t\t\t\t\t- profileRadius * cosTheta,\r\n\t\t\t\t\tnormalYComponent,\r\n\t\t\t\t\tprofileRadius * sinTheta\r\n\t\t\t\t);\r\n\t\t\t\tnormal.normalize();\r\n\t\t\t\tnormals.push( normal.x, normal.y, normal.z );\r\n\r\n\t\t\t\t// uv\r\n\r\n\t\t\t\tuvs.push( u + uOffset, v );\r\n\r\n\t\t\t}\r\n\r\n\t\t\tif ( iy > 0 ) {\r\n\r\n\t\t\t\tconst prevIndexRow = ( iy - 1 ) * verticesPerRow;\r\n\t\t\t\tfor ( let ix = 0; ix < radialSegments; ix ++ ) {\r\n\r\n\t\t\t\t\tconst i1 = prevIndexRow + ix;\r\n\t\t\t\t\tconst i2 = prevIndexRow + ix + 1;\r\n\t\t\t\t\tconst i3 = iy * verticesPerRow + ix;\r\n\t\t\t\t\tconst i4 = iy * verticesPerRow + ix + 1;\r\n\r\n\t\t\t\t\tindices.push( i1, i2, i3 );\r\n\t\t\t\t\tindices.push( i2, i4, i3 );\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}\r\n\r\n\tcopy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {CapsuleGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new CapsuleGeometry( data.radius, data.height, data.capSegments, data.radialSegments, data.heightSegments );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( radius = 1, height = 1, capSegments = 4, radialSegments = 8, heightSegments = 1 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'CapsuleGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\theight: height,\r\n\t\t\tcapSegments: capSegments,\r\n\t\t\tradialSegments: radialSegments,\r\n\t\t\theightSegments: heightSegments,\r\n\t\t};\r\n\r\n\t\theight = Math.max( 0, height );\r\n\t\tcapSegments = Math.max( 1, Math.floor( capSegments ) );\r\n\t\tradialSegments = Math.max( 3, Math.floor( radialSegments ) );\r\n\t\theightSegments = Math.max( 1, Math.floor( heightSegments ) );\r\n\r\n\t\t// buffers\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\t// helper variables\r\n\r\n\t\tconst halfHeight = height / 2;\r\n\t\tconst capArcLength = ( Math.PI / 2 ) * radius;\r\n\t\tconst cylinderPartLength = height;\r\n\t\tconst totalArcLength = 2 * capArcLength + cylinderPartLength;\r\n\r\n\t\tconst numVerticalSegments = capSegments * 2 + heightSegments;\r\n\t\tconst verticesPerRow = radialSegments + 1;\r\n\r\n\t\tconst normal = new Vector3();\r\n\t\tconst vertex = new Vector3();\r\n\r\n\t\t// generate vertices, normals, and uvs\r\n\r\n\t\tfor ( let iy = 0; iy <= numVerticalSegments; iy ++ ) {\r\n\r\n\t\t\tlet currentArcLength = 0;\r\n\t\t\tlet profileY = 0;\r\n\t\t\tlet profileRadius = 0;\r\n\t\t\tlet normalYComponent = 0;\r\n\r\n\t\t\tif ( iy <= capSegments ) {\r\n\r\n\t\t\t\t// bottom cap\r\n\t\t\t\tconst segmentProgress = iy / capSegments;\r\n\t\t\t\tconst angle = ( segmentProgress * Math.PI ) / 2;\r\n\t\t\t\tprofileY = - halfHeight - radius * Math.cos( angle );\r\n\t\t\t\tprofileRadius = radius * Math.sin( angle );\r\n\t\t\t\tnormalYComponent = - radius * Math.cos( angle );\r\n\t\t\t\tcurrentArcLength = segmentProgress * capArcLength;\r\n\r\n\t\t\t} else if ( iy <= capSegments + heightSegments ) {\r\n\r\n\t\t\t\t// middle section\r\n\t\t\t\tconst segmentProgress = ( iy - capSegments ) / heightSegments;\r\n\t\t\t\tprofileY = - halfHeight + segmentProgress * height;\r\n\t\t\t\tprofileRadius = radius;\r\n\t\t\t\tnormalYComponent = 0;\r\n\t\t\t\tcurrentArcLength = capArcLength + segmentProgress * cylinderPartLength;\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\t// top cap\r\n\t\t\t\tconst segmentProgress =\r\n\t\t\t\t\t( iy - capSegments - heightSegments ) / capSegments;\r\n\t\t\t\tconst angle = ( segmentProgress * Math.PI ) / 2;\r\n\t\t\t\tprofileY = halfHeight + radius * Math.sin( angle );\r\n\t\t\t\tprofileRadius = radius * Math.cos( angle );\r\n\t\t\t\tnormalYComponent = radius * Math.sin( angle );\r\n\t\t\t\tcurrentArcLength =\r\n\t\t\t\t\tcapArcLength + cylinderPartLength + segmentProgress * capArcLength;\r\n\r\n\t\t\t}\r\n\r\n\t\t\tconst v = Math.max( 0, Math.min( 1, currentArcLength / totalArcLength ) );\r\n\r\n\r\n\t\t\t// special case for the poles\r\n\r\n\t\t\tlet uOffset = 0;\r\n\r\n\t\t\tif ( iy === 0 ) {\r\n\r\n\t\t\t\tuOffset = 0.5 / radialSegments;\r\n\r\n\t\t\t} else if ( iy === numVerticalSegments ) {\r\n\r\n\t\t\t\tuOffset = - 0.5 / radialSegments;\r\n\r\n\t\t\t}\r\n\r\n\t\t\tfor ( let ix = 0; ix <= radialSegments; ix ++ ) {\r\n\r\n\t\t\t\tconst u = ix / radialSegments;\r\n\t\t\t\tconst theta = u * Math.PI * 2;\r\n\r\n\t\t\t\tconst sinTheta = Math.sin( theta );\r\n\t\t\t\tconst cosTheta = Math.cos( theta );\r\n\r\n\t\t\t\t// vertex\r\n\r\n\t\t\t\tvertex.x = - profileRadius * cosTheta;\r\n\t\t\t\tvertex.y = profileY;\r\n\t\t\t\tvertex.z = profileRadius * sinTheta;\r\n\t\t\t\tvertices.push( vertex.x, vertex.y, vertex.z );\r\n\r\n\t\t\t\t// normal\r\n\r\n\t\t\t\tnormal.set(\r\n\t\t\t\t\t- profileRadius * cosTheta,\r\n\t\t\t\t\tnormalYComponent,\r\n\t\t\t\t\tprofileRadius * sinTheta\r\n\t\t\t\t);\r\n\t\t\t\tnormal.normalize();\r\n\t\t\t\tnormals.push( normal.x, normal.y, normal.z );\r\n\r\n\t\t\t\t// uv\r\n\r\n\t\t\t\tuvs.push( u + uOffset, v );\r\n\r\n\t\t\t}\r\n\r\n\t\t\tif ( iy > 0 ) {\r\n\r\n\t\t\t\tconst prevIndexRow = ( iy - 1 ) * verticesPerRow;\r\n\t\t\t\tfor ( let ix = 0; ix < radialSegments; ix ++ ) {\r\n\r\n\t\t\t\t\tconst i1 = prevIndexRow + ix;\r\n\t\t\t\t\tconst i2 = prevIndexRow + ix + 1;\r\n\t\t\t\t\tconst i3 = iy * verticesPerRow + ix;\r\n\t\t\t\t\tconst i4 = iy * verticesPerRow + ix + 1;\r\n\r\n\t\t\t\t\tindices.push( i1, i2, i3 );\r\n\t\t\t\t\tindices.push( i2, i4, i3 );\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// build geometry\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}";
inline constexpr char source2[]="( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source3[]="copy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source4[]="( data ) {\r\n\r\n\t\treturn new CapsuleGeometry( data.radius, data.height, data.capSegments, data.radialSegments, data.heightSegments );\r\n\r\n\t}";
inline constexpr char source5[]="fromJSON( data ) {\r\n\r\n\t\treturn new CapsuleGeometry( data.radius, data.height, data.capSegments, data.radialSegments, data.heightSegments );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function2(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"CapsuleGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"radius",true);declare(env,"height",true);declare(env,"capSegments",true);declare(env,"radialSegments",true);declare(env,"heightSegments",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(1.0);initialize(env,"radius",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(1.0);initialize(env,"height",g2);auto g3=arg(args,2);if(g3.isUndefined())g3=JSValue(4.0);initialize(env,"capSegments",g3);auto g4=arg(args,3);if(g4.isUndefined())g4=JSValue(8.0);initialize(env,"radialSegments",g4);auto g5=arg(args,4);if(g5.isUndefined())g5=JSValue(1.0);initialize(env,"heightSegments",g5);{auto g6=env;auto env=scope(g6);declare(env,"indices",false);declare(env,"vertices",false);declare(env,"normals",false);declare(env,"uvs",false);declare(env,"halfHeight",false);declare(env,"capArcLength",false);declare(env,"cylinderPartLength",false);declare(env,"totalArcLength",false);declare(env,"numVerticalSegments",false);declare(env,"verticesPerRow",false);declare(env,"normal",false);declare(env,"vertex",false);([&]() -> JSValue { const auto g7=home.getPrototype();Args g9;const auto instance=g7.construct(g9,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g10=read(env,"$this");const auto g11=JSValue("type");const auto g12=JSValue("CapsuleGeometry"); return setReference(g10,g11,g12); }());
([&]() -> JSValue { auto g13=read(env,"$this");const auto g14=JSValue("parameters");const auto g15=([&]() -> JSValue { auto g16=JSValue::object();const auto g17=JSValue("radius");own(g16,g17,read(env,"radius"));const auto g18=JSValue("height");own(g16,g18,read(env,"height"));const auto g19=JSValue("capSegments");own(g16,g19,read(env,"capSegments"));const auto g20=JSValue("radialSegments");own(g16,g20,read(env,"radialSegments"));const auto g21=JSValue("heightSegments");own(g16,g21,read(env,"heightSegments")); return g16; }()); return setReference(g13,g14,g15); }());
([&]() -> JSValue { const auto g24=([&]() -> JSValue { const auto g26=read(env,"Math");const auto g25=g26.get("max");Args g27;g27.push_back(JSValue(0.0));g27.push_back(read(env,"height")); return g25.call(g26,g27); }()); return write(env,"height",g24); }());
([&]() -> JSValue { const auto g30=([&]() -> JSValue { const auto g32=read(env,"Math");const auto g31=g32.get("max");Args g33;g33.push_back(JSValue(1.0));g33.push_back(([&]() -> JSValue { const auto g35=read(env,"Math");const auto g34=g35.get("floor");Args g36;g36.push_back(read(env,"capSegments")); return g34.call(g35,g36); }())); return g31.call(g32,g33); }()); return write(env,"capSegments",g30); }());
([&]() -> JSValue { const auto g39=([&]() -> JSValue { const auto g41=read(env,"Math");const auto g40=g41.get("max");Args g42;g42.push_back(JSValue(3.0));g42.push_back(([&]() -> JSValue { const auto g44=read(env,"Math");const auto g43=g44.get("floor");Args g45;g45.push_back(read(env,"radialSegments")); return g43.call(g44,g45); }())); return g40.call(g41,g42); }()); return write(env,"radialSegments",g39); }());
([&]() -> JSValue { const auto g48=([&]() -> JSValue { const auto g50=read(env,"Math");const auto g49=g50.get("max");Args g51;g51.push_back(JSValue(1.0));g51.push_back(([&]() -> JSValue { const auto g53=read(env,"Math");const auto g52=g53.get("floor");Args g54;g54.push_back(read(env,"heightSegments")); return g52.call(g53,g54); }())); return g49.call(g50,g51); }()); return write(env,"heightSegments",g48); }());
initialize(env,"indices",([&]() -> JSValue { auto g55=JSValue::array(0); return g55; }()));
initialize(env,"vertices",([&]() -> JSValue { auto g56=JSValue::array(0); return g56; }()));
initialize(env,"normals",([&]() -> JSValue { auto g57=JSValue::array(0); return g57; }()));
initialize(env,"uvs",([&]() -> JSValue { auto g58=JSValue::array(0); return g58; }()));
initialize(env,"halfHeight",([&]() -> JSValue { const auto g59=read(env,"height");const auto g60=JSValue(2.0); return binary("/",g59,g60); }()));
initialize(env,"capArcLength",([&]() -> JSValue { const auto g61=([&]() -> JSValue { const auto g63=([&]() -> JSValue { const auto g65=read(env,"Math");const auto g66=JSValue("PI"); return g65.get(g66); }());const auto g64=JSValue(2.0); return binary("/",g63,g64); }());const auto g62=read(env,"radius"); return binary("*",g61,g62); }()));
initialize(env,"cylinderPartLength",read(env,"height"));
initialize(env,"totalArcLength",([&]() -> JSValue { const auto g67=([&]() -> JSValue { const auto g69=JSValue(2.0);const auto g70=read(env,"capArcLength"); return binary("*",g69,g70); }());const auto g68=read(env,"cylinderPartLength"); return binary("+",g67,g68); }()));
initialize(env,"numVerticalSegments",([&]() -> JSValue { const auto g71=([&]() -> JSValue { const auto g73=read(env,"capSegments");const auto g74=JSValue(2.0); return binary("*",g73,g74); }());const auto g72=read(env,"heightSegments"); return binary("+",g71,g72); }()));
initialize(env,"verticesPerRow",([&]() -> JSValue { const auto g75=read(env,"radialSegments");const auto g76=JSValue(1.0); return binary("+",g75,g76); }()));
initialize(env,"normal",([&]() -> JSValue { const auto g77=read(env,"Vector3");Args g79; return g77.construct(g79); }()));
initialize(env,"vertex",([&]() -> JSValue { const auto g80=read(env,"Vector3");Args g82; return g80.construct(g82); }()));
{auto g83=env;auto env=scope(g83);declare(env,"iy",true);initialize(env,"iy",JSValue(0.0));for(;([&]() -> JSValue { const auto g89=read(env,"iy");const auto g90=read(env,"numVerticalSegments"); return binary("<=",g89,g90); }()).toBoolean();[&](){auto g84=scope(g83);declare(g84,"iy",true);initialize(g84,"iy",read(env,"iy"));env=g84;([&]() -> JSValue { const auto g85=read(env,"iy").toNumber();const JSValue g86(g85+1);write(env,"iy",g86); return JSValue(g85); }());}()){{auto g91=env;auto env=scope(g91);declare(env,"currentArcLength",true);declare(env,"profileY",true);declare(env,"profileRadius",true);declare(env,"normalYComponent",true);declare(env,"v",false);declare(env,"uOffset",true);initialize(env,"currentArcLength",JSValue(0.0));
initialize(env,"profileY",JSValue(0.0));
initialize(env,"profileRadius",JSValue(0.0));
initialize(env,"normalYComponent",JSValue(0.0));
if(([&]() -> JSValue { const auto g92=read(env,"iy");const auto g93=read(env,"capSegments"); return binary("<=",g92,g93); }()).toBoolean()){{auto g94=env;auto env=scope(g94);declare(env,"segmentProgress",false);declare(env,"angle",false);initialize(env,"segmentProgress",([&]() -> JSValue { const auto g95=read(env,"iy");const auto g96=read(env,"capSegments"); return binary("/",g95,g96); }()));
initialize(env,"angle",([&]() -> JSValue { const auto g97=([&]() -> JSValue { const auto g99=read(env,"segmentProgress");const auto g100=([&]() -> JSValue { const auto g101=read(env,"Math");const auto g102=JSValue("PI"); return g101.get(g102); }()); return binary("*",g99,g100); }());const auto g98=JSValue(2.0); return binary("/",g97,g98); }()));
([&]() -> JSValue { const auto g105=([&]() -> JSValue { const auto g106=JSValue(-read(env,"halfHeight").toNumber());const auto g107=([&]() -> JSValue { const auto g109=read(env,"radius");const auto g110=([&]() -> JSValue { const auto g112=read(env,"Math");const auto g111=g112.get("cos");Args g113;g113.push_back(read(env,"angle")); return g111.call(g112,g113); }()); return binary("*",g109,g110); }()); return binary("-",g106,g107); }()); return write(env,"profileY",g105); }());
([&]() -> JSValue { const auto g116=([&]() -> JSValue { const auto g117=read(env,"radius");const auto g118=([&]() -> JSValue { const auto g120=read(env,"Math");const auto g119=g120.get("sin");Args g121;g121.push_back(read(env,"angle")); return g119.call(g120,g121); }()); return binary("*",g117,g118); }()); return write(env,"profileRadius",g116); }());
([&]() -> JSValue { const auto g124=([&]() -> JSValue { const auto g125=JSValue(-read(env,"radius").toNumber());const auto g126=([&]() -> JSValue { const auto g129=read(env,"Math");const auto g128=g129.get("cos");Args g130;g130.push_back(read(env,"angle")); return g128.call(g129,g130); }()); return binary("*",g125,g126); }()); return write(env,"normalYComponent",g124); }());
([&]() -> JSValue { const auto g133=([&]() -> JSValue { const auto g134=read(env,"segmentProgress");const auto g135=read(env,"capArcLength"); return binary("*",g134,g135); }()); return write(env,"currentArcLength",g133); }());}}else{if(([&]() -> JSValue { const auto g136=read(env,"iy");const auto g137=([&]() -> JSValue { const auto g138=read(env,"capSegments");const auto g139=read(env,"heightSegments"); return binary("+",g138,g139); }()); return binary("<=",g136,g137); }()).toBoolean()){{auto g140=env;auto env=scope(g140);declare(env,"segmentProgress",false);initialize(env,"segmentProgress",([&]() -> JSValue { const auto g141=([&]() -> JSValue { const auto g143=read(env,"iy");const auto g144=read(env,"capSegments"); return binary("-",g143,g144); }());const auto g142=read(env,"heightSegments"); return binary("/",g141,g142); }()));
([&]() -> JSValue { const auto g147=([&]() -> JSValue { const auto g148=JSValue(-read(env,"halfHeight").toNumber());const auto g149=([&]() -> JSValue { const auto g151=read(env,"segmentProgress");const auto g152=read(env,"height"); return binary("*",g151,g152); }()); return binary("+",g148,g149); }()); return write(env,"profileY",g147); }());
([&]() -> JSValue { const auto g155=read(env,"radius"); return write(env,"profileRadius",g155); }());
([&]() -> JSValue { const auto g158=JSValue(0.0); return write(env,"normalYComponent",g158); }());
([&]() -> JSValue { const auto g161=([&]() -> JSValue { const auto g162=read(env,"capArcLength");const auto g163=([&]() -> JSValue { const auto g164=read(env,"segmentProgress");const auto g165=read(env,"cylinderPartLength"); return binary("*",g164,g165); }()); return binary("+",g162,g163); }()); return write(env,"currentArcLength",g161); }());}}else{{auto g166=env;auto env=scope(g166);declare(env,"segmentProgress",false);declare(env,"angle",false);initialize(env,"segmentProgress",([&]() -> JSValue { const auto g167=([&]() -> JSValue { const auto g169=([&]() -> JSValue { const auto g171=read(env,"iy");const auto g172=read(env,"capSegments"); return binary("-",g171,g172); }());const auto g170=read(env,"heightSegments"); return binary("-",g169,g170); }());const auto g168=read(env,"capSegments"); return binary("/",g167,g168); }()));
initialize(env,"angle",([&]() -> JSValue { const auto g173=([&]() -> JSValue { const auto g175=read(env,"segmentProgress");const auto g176=([&]() -> JSValue { const auto g177=read(env,"Math");const auto g178=JSValue("PI"); return g177.get(g178); }()); return binary("*",g175,g176); }());const auto g174=JSValue(2.0); return binary("/",g173,g174); }()));
([&]() -> JSValue { const auto g181=([&]() -> JSValue { const auto g182=read(env,"halfHeight");const auto g183=([&]() -> JSValue { const auto g184=read(env,"radius");const auto g185=([&]() -> JSValue { const auto g187=read(env,"Math");const auto g186=g187.get("sin");Args g188;g188.push_back(read(env,"angle")); return g186.call(g187,g188); }()); return binary("*",g184,g185); }()); return binary("+",g182,g183); }()); return write(env,"profileY",g181); }());
([&]() -> JSValue { const auto g191=([&]() -> JSValue { const auto g192=read(env,"radius");const auto g193=([&]() -> JSValue { const auto g195=read(env,"Math");const auto g194=g195.get("cos");Args g196;g196.push_back(read(env,"angle")); return g194.call(g195,g196); }()); return binary("*",g192,g193); }()); return write(env,"profileRadius",g191); }());
([&]() -> JSValue { const auto g199=([&]() -> JSValue { const auto g200=read(env,"radius");const auto g201=([&]() -> JSValue { const auto g203=read(env,"Math");const auto g202=g203.get("sin");Args g204;g204.push_back(read(env,"angle")); return g202.call(g203,g204); }()); return binary("*",g200,g201); }()); return write(env,"normalYComponent",g199); }());
([&]() -> JSValue { const auto g207=([&]() -> JSValue { const auto g208=([&]() -> JSValue { const auto g210=read(env,"capArcLength");const auto g211=read(env,"cylinderPartLength"); return binary("+",g210,g211); }());const auto g209=([&]() -> JSValue { const auto g212=read(env,"segmentProgress");const auto g213=read(env,"capArcLength"); return binary("*",g212,g213); }()); return binary("+",g208,g209); }()); return write(env,"currentArcLength",g207); }());}}}
initialize(env,"v",([&]() -> JSValue { const auto g215=read(env,"Math");const auto g214=g215.get("max");Args g216;g216.push_back(JSValue(0.0));g216.push_back(([&]() -> JSValue { const auto g218=read(env,"Math");const auto g217=g218.get("min");Args g219;g219.push_back(JSValue(1.0));g219.push_back(([&]() -> JSValue { const auto g220=read(env,"currentArcLength");const auto g221=read(env,"totalArcLength"); return binary("/",g220,g221); }())); return g217.call(g218,g219); }())); return g214.call(g215,g216); }()));
initialize(env,"uOffset",JSValue(0.0));
if(([&]() -> JSValue { const auto g222=read(env,"iy");const auto g223=JSValue(0.0); return binary("===",g222,g223); }()).toBoolean()){{auto g224=env;auto env=scope(g224);([&]() -> JSValue { const auto g227=([&]() -> JSValue { const auto g228=JSValue(0.5);const auto g229=read(env,"radialSegments"); return binary("/",g228,g229); }()); return write(env,"uOffset",g227); }());}}else{if(([&]() -> JSValue { const auto g230=read(env,"iy");const auto g231=read(env,"numVerticalSegments"); return binary("===",g230,g231); }()).toBoolean()){{auto g232=env;auto env=scope(g232);([&]() -> JSValue { const auto g235=([&]() -> JSValue { const auto g236=JSValue(-JSValue(0.5).toNumber());const auto g237=read(env,"radialSegments"); return binary("/",g236,g237); }()); return write(env,"uOffset",g235); }());}}}
{auto g239=env;auto env=scope(g239);declare(env,"ix",true);initialize(env,"ix",JSValue(0.0));for(;([&]() -> JSValue { const auto g245=read(env,"ix");const auto g246=read(env,"radialSegments"); return binary("<=",g245,g246); }()).toBoolean();[&](){auto g240=scope(g239);declare(g240,"ix",true);initialize(g240,"ix",read(env,"ix"));env=g240;([&]() -> JSValue { const auto g241=read(env,"ix").toNumber();const JSValue g242(g241+1);write(env,"ix",g242); return JSValue(g241); }());}()){{auto g247=env;auto env=scope(g247);declare(env,"u",false);declare(env,"theta",false);declare(env,"sinTheta",false);declare(env,"cosTheta",false);initialize(env,"u",([&]() -> JSValue { const auto g248=read(env,"ix");const auto g249=read(env,"radialSegments"); return binary("/",g248,g249); }()));
initialize(env,"theta",([&]() -> JSValue { const auto g250=([&]() -> JSValue { const auto g252=read(env,"u");const auto g253=([&]() -> JSValue { const auto g254=read(env,"Math");const auto g255=JSValue("PI"); return g254.get(g255); }()); return binary("*",g252,g253); }());const auto g251=JSValue(2.0); return binary("*",g250,g251); }()));
initialize(env,"sinTheta",([&]() -> JSValue { const auto g257=read(env,"Math");const auto g256=g257.get("sin");Args g258;g258.push_back(read(env,"theta")); return g256.call(g257,g258); }()));
initialize(env,"cosTheta",([&]() -> JSValue { const auto g260=read(env,"Math");const auto g259=g260.get("cos");Args g261;g261.push_back(read(env,"theta")); return g259.call(g260,g261); }()));
([&]() -> JSValue { auto g262=read(env,"vertex");const auto g263=JSValue("x");const auto g264=([&]() -> JSValue { const auto g265=JSValue(-read(env,"profileRadius").toNumber());const auto g266=read(env,"cosTheta"); return binary("*",g265,g266); }()); return setReference(g262,g263,g264); }());
([&]() -> JSValue { auto g268=read(env,"vertex");const auto g269=JSValue("y");const auto g270=read(env,"profileY"); return setReference(g268,g269,g270); }());
([&]() -> JSValue { auto g271=read(env,"vertex");const auto g272=JSValue("z");const auto g273=([&]() -> JSValue { const auto g274=read(env,"profileRadius");const auto g275=read(env,"sinTheta"); return binary("*",g274,g275); }()); return setReference(g271,g272,g273); }());
([&]() -> JSValue { const auto g277=read(env,"vertices");const auto g276=g277.get("push");Args g278;g278.push_back(([&]() -> JSValue { const auto g279=read(env,"vertex");const auto g280=JSValue("x"); return g279.get(g280); }()));g278.push_back(([&]() -> JSValue { const auto g281=read(env,"vertex");const auto g282=JSValue("y"); return g281.get(g282); }()));g278.push_back(([&]() -> JSValue { const auto g283=read(env,"vertex");const auto g284=JSValue("z"); return g283.get(g284); }())); return g276.call(g277,g278); }());
([&]() -> JSValue { const auto g286=read(env,"normal");const auto g285=g286.get("set");Args g287;g287.push_back(([&]() -> JSValue { const auto g288=JSValue(-read(env,"profileRadius").toNumber());const auto g289=read(env,"cosTheta"); return binary("*",g288,g289); }()));g287.push_back(read(env,"normalYComponent"));g287.push_back(([&]() -> JSValue { const auto g291=read(env,"profileRadius");const auto g292=read(env,"sinTheta"); return binary("*",g291,g292); }())); return g285.call(g286,g287); }());
([&]() -> JSValue { const auto g294=read(env,"normal");const auto g293=g294.get("normalize");Args g295; return g293.call(g294,g295); }());
([&]() -> JSValue { const auto g297=read(env,"normals");const auto g296=g297.get("push");Args g298;g298.push_back(([&]() -> JSValue { const auto g299=read(env,"normal");const auto g300=JSValue("x"); return g299.get(g300); }()));g298.push_back(([&]() -> JSValue { const auto g301=read(env,"normal");const auto g302=JSValue("y"); return g301.get(g302); }()));g298.push_back(([&]() -> JSValue { const auto g303=read(env,"normal");const auto g304=JSValue("z"); return g303.get(g304); }())); return g296.call(g297,g298); }());
([&]() -> JSValue { const auto g306=read(env,"uvs");const auto g305=g306.get("push");Args g307;g307.push_back(([&]() -> JSValue { const auto g308=read(env,"u");const auto g309=read(env,"uOffset"); return binary("+",g308,g309); }()));g307.push_back(read(env,"v")); return g305.call(g306,g307); }());}}}
if(([&]() -> JSValue { const auto g310=read(env,"iy");const auto g311=JSValue(0.0); return binary(">",g310,g311); }()).toBoolean()){{auto g312=env;auto env=scope(g312);declare(env,"prevIndexRow",false);initialize(env,"prevIndexRow",([&]() -> JSValue { const auto g313=([&]() -> JSValue { const auto g315=read(env,"iy");const auto g316=JSValue(1.0); return binary("-",g315,g316); }());const auto g314=read(env,"verticesPerRow"); return binary("*",g313,g314); }()));
{auto g317=env;auto env=scope(g317);declare(env,"ix",true);initialize(env,"ix",JSValue(0.0));for(;([&]() -> JSValue { const auto g323=read(env,"ix");const auto g324=read(env,"radialSegments"); return binary("<",g323,g324); }()).toBoolean();[&](){auto g318=scope(g317);declare(g318,"ix",true);initialize(g318,"ix",read(env,"ix"));env=g318;([&]() -> JSValue { const auto g319=read(env,"ix").toNumber();const JSValue g320(g319+1);write(env,"ix",g320); return JSValue(g319); }());}()){{auto g325=env;auto env=scope(g325);declare(env,"i1",false);declare(env,"i2",false);declare(env,"i3",false);declare(env,"i4",false);initialize(env,"i1",([&]() -> JSValue { const auto g326=read(env,"prevIndexRow");const auto g327=read(env,"ix"); return binary("+",g326,g327); }()));
initialize(env,"i2",([&]() -> JSValue { const auto g328=([&]() -> JSValue { const auto g330=read(env,"prevIndexRow");const auto g331=read(env,"ix"); return binary("+",g330,g331); }());const auto g329=JSValue(1.0); return binary("+",g328,g329); }()));
initialize(env,"i3",([&]() -> JSValue { const auto g332=([&]() -> JSValue { const auto g334=read(env,"iy");const auto g335=read(env,"verticesPerRow"); return binary("*",g334,g335); }());const auto g333=read(env,"ix"); return binary("+",g332,g333); }()));
initialize(env,"i4",([&]() -> JSValue { const auto g336=([&]() -> JSValue { const auto g338=([&]() -> JSValue { const auto g340=read(env,"iy");const auto g341=read(env,"verticesPerRow"); return binary("*",g340,g341); }());const auto g339=read(env,"ix"); return binary("+",g338,g339); }());const auto g337=JSValue(1.0); return binary("+",g336,g337); }()));
([&]() -> JSValue { const auto g343=read(env,"indices");const auto g342=g343.get("push");Args g344;g344.push_back(read(env,"i1"));g344.push_back(read(env,"i2"));g344.push_back(read(env,"i3")); return g342.call(g343,g344); }());
([&]() -> JSValue { const auto g346=read(env,"indices");const auto g345=g346.get("push");Args g347;g347.push_back(read(env,"i2"));g347.push_back(read(env,"i4"));g347.push_back(read(env,"i3")); return g345.call(g346,g347); }());}}}}}}}}
([&]() -> JSValue { const auto g349=read(env,"$this");const auto g348=g349.get("setIndex");Args g350;g350.push_back(read(env,"indices")); return g348.call(g349,g350); }());
([&]() -> JSValue { const auto g352=read(env,"$this");const auto g351=g352.get("setAttribute");Args g353;g353.push_back(JSValue("position"));g353.push_back(([&]() -> JSValue { const auto g354=read(env,"Float32BufferAttribute");Args g356;g356.push_back(read(env,"vertices"));g356.push_back(JSValue(3.0)); return g354.construct(g356); }())); return g351.call(g352,g353); }());
([&]() -> JSValue { const auto g358=read(env,"$this");const auto g357=g358.get("setAttribute");Args g359;g359.push_back(JSValue("normal"));g359.push_back(([&]() -> JSValue { const auto g360=read(env,"Float32BufferAttribute");Args g362;g362.push_back(read(env,"normals"));g362.push_back(JSValue(3.0)); return g360.construct(g362); }())); return g357.call(g358,g359); }());
([&]() -> JSValue { const auto g364=read(env,"$this");const auto g363=g364.get("setAttribute");Args g365;g365.push_back(JSValue("uv"));g365.push_back(([&]() -> JSValue { const auto g366=read(env,"Float32BufferAttribute");Args g368;g368.push_back(read(env,"uvs"));g368.push_back(JSValue(2.0)); return g366.construct(g368); }())); return g363.call(g364,g365); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"source",true);initialize(env,"source",arg(args,0));{auto g369=env;auto env=scope(g369);([&]() -> JSValue { const auto g371=read(env,"$this");const auto g370=home.getPrototype().get("copy",g371);Args g372;g372.push_back(read(env,"source")); return g370.call(g371,g372); }());
([&]() -> JSValue { auto g373=read(env,"$this");const auto g374=JSValue("parameters");const auto g375=([&]() -> JSValue { const auto g377=read(env,"Object");const auto g376=g377.get("assign");Args g378;g378.push_back(([&]() -> JSValue { auto g379=JSValue::object(); return g379; }()));g378.push_back(([&]() -> JSValue { const auto g380=read(env,"source");const auto g381=JSValue("parameters"); return g380.get(g381); }())); return g376.call(g377,g378); }()); return setReference(g373,g374,g375); }());
return read(env,"$this");}return {};}
JSValue function2(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g383=env;auto env=scope(g383);return ([&]() -> JSValue { const auto g384=read(env,"CapsuleGeometry");Args g386;g386.push_back(([&]() -> JSValue { const auto g387=read(env,"data");const auto g388=JSValue("radius"); return g387.get(g388); }()));g386.push_back(([&]() -> JSValue { const auto g389=read(env,"data");const auto g390=JSValue("height"); return g389.get(g390); }()));g386.push_back(([&]() -> JSValue { const auto g391=read(env,"data");const auto g392=JSValue("capSegments"); return g391.get(g392); }()));g386.push_back(([&]() -> JSValue { const auto g393=read(env,"data");const auto g394=JSValue("radialSegments"); return g393.get(g394); }()));g386.push_back(([&]() -> JSValue { const auto g395=read(env,"data");const auto g396=JSValue("heightSegments"); return g395.get(g396); }())); return g384.construct(g386); }());}return {};}
}
JSValue nativeCapsuleGeometryModule(){using namespace CapsuleGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"CapsuleGeometry");auto env=scope();JSValue home;bindImport(env,"BufferGeometry",nativeBufferGeometryConstructor());bindImport(env,"Float32BufferAttribute",nativeFloat32BufferAttributeConstructor());bindImport(env,"Vector3",nativeVector3Binding("Vector3"));declare(env,"CapsuleGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"CapsuleGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"BufferGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"CapsuleGeometry",constructor);{auto home=prototype;auto function=([&]() -> JSValue { auto g382=JSValue::function(function1,{env,home},"copy",1);g382.setFunctionSourceText(source2); return g382; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("copy",d);}{auto home=constructor;auto function=([&]() -> JSValue { auto g397=JSValue::function(function2,{env,home},"fromJSON",1);g397.setFunctionSourceText(source4); return g397; }());function.setFunctionSourceText(source5);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"CapsuleGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeCapsuleGeometryBinding(const std::string& name){return nativeCapsuleGeometryModule().get(name);}
JSValue nativeCapsuleGeometryConstructor(){return nativeCapsuleGeometryBinding("CapsuleGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
