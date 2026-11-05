#include "geometries/PlaneGeometry.h"
#include <cmath>
PlaneGeometry::PlaneGeometry(double width, double height, double widthSegments,
                             double heightSegments)
    : parameters{width, height, widthSegments, heightSegments} {
    type = "PlaneGeometry";
    const double gx = std::floor(widthSegments), gy = std::floor(heightSegments);
    if (gy == INFINITY || (gy + 1 > 0 && gx == INFINITY))
        throw JSRangeError("PlaneGeometry infinite subdivision does not terminate");
    const double sw = width / gx, sh = height / gy;
    std::vector<double> vertices, normals, uvs;
    std::vector<std::uint32_t> indices;
    for (double iy = 0; iy < gy + 1; ++iy)
        for (double ix = 0; ix < gx + 1; ++ix) {
            vertices.insert(vertices.end(), {ix * sw - width / 2, -(iy * sh - height / 2), 0});
            normals.insert(normals.end(), {0, 0, 1});
            uvs.insert(uvs.end(), {ix / gx, 1 - iy / gy});
        }
    for (double iy = 0; iy < gy; ++iy)
        for (double ix = 0; ix < gx; ++ix) {
            const auto a = static_cast<std::uint32_t>(ix + (gx + 1) * iy),
                       b = static_cast<std::uint32_t>(ix + (gx + 1) * (iy + 1)),
                       c = static_cast<std::uint32_t>(ix + 1 + (gx + 1) * (iy + 1)),
                       d = static_cast<std::uint32_t>(ix + 1 + (gx + 1) * iy);
            indices.insert(indices.end(), {a, b, d, b, c, d});
        }
    setIndex(indices);
    setAttribute("position", std::make_shared<Float32BufferAttribute>(std::move(vertices), 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(std::move(normals), 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(std::move(uvs), 2));
}
BufferGeometry& PlaneGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (auto p = dynamic_cast<const PlaneGeometry*>(&source))
        parameters = p->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> PlaneGeometry::clone() const {
    auto result = std::make_shared<PlaneGeometry>();
    result->copy(*this);
    return result;
}
JsonValue PlaneGeometry::toJSON() const {
    if (_transformed) {
        auto result = BufferGeometry::toJSON();
        result["type"] = "BufferGeometry";
        return result;
    }
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    result["width"] = parameters.width;
    result["height"] = parameters.height;
    result["widthSegments"] = parameters.widthSegments;
    result["heightSegments"] = parameters.heightSegments;
    return result;
}
std::shared_ptr<PlaneGeometry> PlaneGeometry::fromJSON(const JsonValue& json) {
    const auto number = [&](const char* key) { return json.contains(key) ? JSValue::fromJSON(json.at(key)).toNumber() : 1.; };
    return std::make_shared<PlaneGeometry>(number("width"), number("height"), number("widthSegments"), number("heightSegments"));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/PlaneGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
namespace PlaneGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class PlaneGeometry extends BufferGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new plane geometry.\r\n\t *\r\n\t * @param {number} [width=1] - The width along the X axis.\r\n\t * @param {number} [height=1] - The height along the Y axis\r\n\t * @param {number} [widthSegments=1] - The number of segments along the X axis.\r\n\t * @param {number} [heightSegments=1] - The number of segments along the Y axis.\r\n\t */\r\n\tconstructor( width = 1, height = 1, widthSegments = 1, heightSegments = 1 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'PlaneGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\twidth: width,\r\n\t\t\theight: height,\r\n\t\t\twidthSegments: widthSegments,\r\n\t\t\theightSegments: heightSegments\r\n\t\t};\r\n\r\n\t\tconst width_half = width / 2;\r\n\t\tconst height_half = height / 2;\r\n\r\n\t\tconst gridX = Math.floor( widthSegments );\r\n\t\tconst gridY = Math.floor( heightSegments );\r\n\r\n\t\tconst gridX1 = gridX + 1;\r\n\t\tconst gridY1 = gridY + 1;\r\n\r\n\t\tconst segment_width = width / gridX;\r\n\t\tconst segment_height = height / gridY;\r\n\r\n\t\t//\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\tfor ( let iy = 0; iy < gridY1; iy ++ ) {\r\n\r\n\t\t\tconst y = iy * segment_height - height_half;\r\n\r\n\t\t\tfor ( let ix = 0; ix < gridX1; ix ++ ) {\r\n\r\n\t\t\t\tconst x = ix * segment_width - width_half;\r\n\r\n\t\t\t\tvertices.push( x, - y, 0 );\r\n\r\n\t\t\t\tnormals.push( 0, 0, 1 );\r\n\r\n\t\t\t\tuvs.push( ix / gridX );\r\n\t\t\t\tuvs.push( 1 - ( iy / gridY ) );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\tfor ( let iy = 0; iy < gridY; iy ++ ) {\r\n\r\n\t\t\tfor ( let ix = 0; ix < gridX; ix ++ ) {\r\n\r\n\t\t\t\tconst a = ix + gridX1 * iy;\r\n\t\t\t\tconst b = ix + gridX1 * ( iy + 1 );\r\n\t\t\t\tconst c = ( ix + 1 ) + gridX1 * ( iy + 1 );\r\n\t\t\t\tconst d = ( ix + 1 ) + gridX1 * iy;\r\n\r\n\t\t\t\tindices.push( a, b, d );\r\n\t\t\t\tindices.push( b, c, d );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}\r\n\r\n\tcopy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {PlaneGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new PlaneGeometry( data.width, data.height, data.widthSegments, data.heightSegments );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( width = 1, height = 1, widthSegments = 1, heightSegments = 1 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\tthis.type = 'PlaneGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\twidth: width,\r\n\t\t\theight: height,\r\n\t\t\twidthSegments: widthSegments,\r\n\t\t\theightSegments: heightSegments\r\n\t\t};\r\n\r\n\t\tconst width_half = width / 2;\r\n\t\tconst height_half = height / 2;\r\n\r\n\t\tconst gridX = Math.floor( widthSegments );\r\n\t\tconst gridY = Math.floor( heightSegments );\r\n\r\n\t\tconst gridX1 = gridX + 1;\r\n\t\tconst gridY1 = gridY + 1;\r\n\r\n\t\tconst segment_width = width / gridX;\r\n\t\tconst segment_height = height / gridY;\r\n\r\n\t\t//\r\n\r\n\t\tconst indices = [];\r\n\t\tconst vertices = [];\r\n\t\tconst normals = [];\r\n\t\tconst uvs = [];\r\n\r\n\t\tfor ( let iy = 0; iy < gridY1; iy ++ ) {\r\n\r\n\t\t\tconst y = iy * segment_height - height_half;\r\n\r\n\t\t\tfor ( let ix = 0; ix < gridX1; ix ++ ) {\r\n\r\n\t\t\t\tconst x = ix * segment_width - width_half;\r\n\r\n\t\t\t\tvertices.push( x, - y, 0 );\r\n\r\n\t\t\t\tnormals.push( 0, 0, 1 );\r\n\r\n\t\t\t\tuvs.push( ix / gridX );\r\n\t\t\t\tuvs.push( 1 - ( iy / gridY ) );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\tfor ( let iy = 0; iy < gridY; iy ++ ) {\r\n\r\n\t\t\tfor ( let ix = 0; ix < gridX; ix ++ ) {\r\n\r\n\t\t\t\tconst a = ix + gridX1 * iy;\r\n\t\t\t\tconst b = ix + gridX1 * ( iy + 1 );\r\n\t\t\t\tconst c = ( ix + 1 ) + gridX1 * ( iy + 1 );\r\n\t\t\t\tconst d = ( ix + 1 ) + gridX1 * iy;\r\n\r\n\t\t\t\tindices.push( a, b, d );\r\n\t\t\t\tindices.push( b, c, d );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\tthis.setIndex( indices );\r\n\t\tthis.setAttribute( 'position', new Float32BufferAttribute( vertices, 3 ) );\r\n\t\tthis.setAttribute( 'normal', new Float32BufferAttribute( normals, 3 ) );\r\n\t\tthis.setAttribute( 'uv', new Float32BufferAttribute( uvs, 2 ) );\r\n\r\n\t}";
inline constexpr char source2[]="( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source3[]="copy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.parameters = Object.assign( {}, source.parameters );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr char source4[]="( data ) {\r\n\r\n\t\treturn new PlaneGeometry( data.width, data.height, data.widthSegments, data.heightSegments );\r\n\r\n\t}";
inline constexpr char source5[]="fromJSON( data ) {\r\n\r\n\t\treturn new PlaneGeometry( data.width, data.height, data.widthSegments, data.heightSegments );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function2(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"PlaneGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"width",true);declare(env,"height",true);declare(env,"widthSegments",true);declare(env,"heightSegments",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(1.0);initialize(env,"width",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(1.0);initialize(env,"height",g2);auto g3=arg(args,2);if(g3.isUndefined())g3=JSValue(1.0);initialize(env,"widthSegments",g3);auto g4=arg(args,3);if(g4.isUndefined())g4=JSValue(1.0);initialize(env,"heightSegments",g4);{auto g5=env;auto env=scope(g5);declare(env,"width_half",false);declare(env,"height_half",false);declare(env,"gridX",false);declare(env,"gridY",false);declare(env,"gridX1",false);declare(env,"gridY1",false);declare(env,"segment_width",false);declare(env,"segment_height",false);declare(env,"indices",false);declare(env,"vertices",false);declare(env,"normals",false);declare(env,"uvs",false);([&]() -> JSValue { const auto g6=home.getPrototype();Args g8;const auto instance=g6.construct(g8,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g9=read(env,"$this");const auto g10=JSValue("type");const auto g11=JSValue("PlaneGeometry"); return setReference(g9,g10,g11); }());
([&]() -> JSValue { auto g12=read(env,"$this");const auto g13=JSValue("parameters");const auto g14=([&]() -> JSValue { auto g15=JSValue::object();const auto g16=JSValue("width");own(g15,g16,read(env,"width"));const auto g17=JSValue("height");own(g15,g17,read(env,"height"));const auto g18=JSValue("widthSegments");own(g15,g18,read(env,"widthSegments"));const auto g19=JSValue("heightSegments");own(g15,g19,read(env,"heightSegments")); return g15; }()); return setReference(g12,g13,g14); }());
initialize(env,"width_half",([&]() -> JSValue { const auto g20=read(env,"width");const auto g21=JSValue(2.0); return binary("/",g20,g21); }()));
initialize(env,"height_half",([&]() -> JSValue { const auto g22=read(env,"height");const auto g23=JSValue(2.0); return binary("/",g22,g23); }()));
initialize(env,"gridX",([&]() -> JSValue { const auto g25=read(env,"Math");const auto g24=g25.get("floor");Args g26;g26.push_back(read(env,"widthSegments")); return g24.call(g25,g26); }()));
initialize(env,"gridY",([&]() -> JSValue { const auto g28=read(env,"Math");const auto g27=g28.get("floor");Args g29;g29.push_back(read(env,"heightSegments")); return g27.call(g28,g29); }()));
initialize(env,"gridX1",([&]() -> JSValue { const auto g30=read(env,"gridX");const auto g31=JSValue(1.0); return binary("+",g30,g31); }()));
initialize(env,"gridY1",([&]() -> JSValue { const auto g32=read(env,"gridY");const auto g33=JSValue(1.0); return binary("+",g32,g33); }()));
initialize(env,"segment_width",([&]() -> JSValue { const auto g34=read(env,"width");const auto g35=read(env,"gridX"); return binary("/",g34,g35); }()));
initialize(env,"segment_height",([&]() -> JSValue { const auto g36=read(env,"height");const auto g37=read(env,"gridY"); return binary("/",g36,g37); }()));
initialize(env,"indices",([&]() -> JSValue { auto g38=JSValue::array(0); return g38; }()));
initialize(env,"vertices",([&]() -> JSValue { auto g39=JSValue::array(0); return g39; }()));
initialize(env,"normals",([&]() -> JSValue { auto g40=JSValue::array(0); return g40; }()));
initialize(env,"uvs",([&]() -> JSValue { auto g41=JSValue::array(0); return g41; }()));
{auto g42=env;auto env=scope(g42);declare(env,"iy",true);initialize(env,"iy",JSValue(0.0));for(;([&]() -> JSValue { const auto g48=read(env,"iy");const auto g49=read(env,"gridY1"); return binary("<",g48,g49); }()).toBoolean();[&](){auto g43=scope(g42);declare(g43,"iy",true);initialize(g43,"iy",read(env,"iy"));env=g43;([&]() -> JSValue { const auto g44=read(env,"iy").toNumber();const JSValue g45(g44+1);write(env,"iy",g45); return JSValue(g44); }());}()){{auto g50=env;auto env=scope(g50);declare(env,"y",false);initialize(env,"y",([&]() -> JSValue { const auto g51=([&]() -> JSValue { const auto g53=read(env,"iy");const auto g54=read(env,"segment_height"); return binary("*",g53,g54); }());const auto g52=read(env,"height_half"); return binary("-",g51,g52); }()));
{auto g55=env;auto env=scope(g55);declare(env,"ix",true);initialize(env,"ix",JSValue(0.0));for(;([&]() -> JSValue { const auto g61=read(env,"ix");const auto g62=read(env,"gridX1"); return binary("<",g61,g62); }()).toBoolean();[&](){auto g56=scope(g55);declare(g56,"ix",true);initialize(g56,"ix",read(env,"ix"));env=g56;([&]() -> JSValue { const auto g57=read(env,"ix").toNumber();const JSValue g58(g57+1);write(env,"ix",g58); return JSValue(g57); }());}()){{auto g63=env;auto env=scope(g63);declare(env,"x",false);initialize(env,"x",([&]() -> JSValue { const auto g64=([&]() -> JSValue { const auto g66=read(env,"ix");const auto g67=read(env,"segment_width"); return binary("*",g66,g67); }());const auto g65=read(env,"width_half"); return binary("-",g64,g65); }()));
([&]() -> JSValue { const auto g69=read(env,"vertices");const auto g68=g69.get("push");Args g70;g70.push_back(read(env,"x"));g70.push_back(JSValue(-read(env,"y").toNumber()));g70.push_back(JSValue(0.0)); return g68.call(g69,g70); }());
([&]() -> JSValue { const auto g73=read(env,"normals");const auto g72=g73.get("push");Args g74;g74.push_back(JSValue(0.0));g74.push_back(JSValue(0.0));g74.push_back(JSValue(1.0)); return g72.call(g73,g74); }());
([&]() -> JSValue { const auto g76=read(env,"uvs");const auto g75=g76.get("push");Args g77;g77.push_back(([&]() -> JSValue { const auto g78=read(env,"ix");const auto g79=read(env,"gridX"); return binary("/",g78,g79); }())); return g75.call(g76,g77); }());
([&]() -> JSValue { const auto g81=read(env,"uvs");const auto g80=g81.get("push");Args g82;g82.push_back(([&]() -> JSValue { const auto g83=JSValue(1.0);const auto g84=([&]() -> JSValue { const auto g85=read(env,"iy");const auto g86=read(env,"gridY"); return binary("/",g85,g86); }()); return binary("-",g83,g84); }())); return g80.call(g81,g82); }());}}}}}}
{auto g87=env;auto env=scope(g87);declare(env,"iy",true);initialize(env,"iy",JSValue(0.0));for(;([&]() -> JSValue { const auto g93=read(env,"iy");const auto g94=read(env,"gridY"); return binary("<",g93,g94); }()).toBoolean();[&](){auto g88=scope(g87);declare(g88,"iy",true);initialize(g88,"iy",read(env,"iy"));env=g88;([&]() -> JSValue { const auto g89=read(env,"iy").toNumber();const JSValue g90(g89+1);write(env,"iy",g90); return JSValue(g89); }());}()){{auto g95=env;auto env=scope(g95);{auto g96=env;auto env=scope(g96);declare(env,"ix",true);initialize(env,"ix",JSValue(0.0));for(;([&]() -> JSValue { const auto g102=read(env,"ix");const auto g103=read(env,"gridX"); return binary("<",g102,g103); }()).toBoolean();[&](){auto g97=scope(g96);declare(g97,"ix",true);initialize(g97,"ix",read(env,"ix"));env=g97;([&]() -> JSValue { const auto g98=read(env,"ix").toNumber();const JSValue g99(g98+1);write(env,"ix",g99); return JSValue(g98); }());}()){{auto g104=env;auto env=scope(g104);declare(env,"a",false);declare(env,"b",false);declare(env,"c",false);declare(env,"d",false);initialize(env,"a",([&]() -> JSValue { const auto g105=read(env,"ix");const auto g106=([&]() -> JSValue { const auto g107=read(env,"gridX1");const auto g108=read(env,"iy"); return binary("*",g107,g108); }()); return binary("+",g105,g106); }()));
initialize(env,"b",([&]() -> JSValue { const auto g109=read(env,"ix");const auto g110=([&]() -> JSValue { const auto g111=read(env,"gridX1");const auto g112=([&]() -> JSValue { const auto g113=read(env,"iy");const auto g114=JSValue(1.0); return binary("+",g113,g114); }()); return binary("*",g111,g112); }()); return binary("+",g109,g110); }()));
initialize(env,"c",([&]() -> JSValue { const auto g115=([&]() -> JSValue { const auto g117=read(env,"ix");const auto g118=JSValue(1.0); return binary("+",g117,g118); }());const auto g116=([&]() -> JSValue { const auto g119=read(env,"gridX1");const auto g120=([&]() -> JSValue { const auto g121=read(env,"iy");const auto g122=JSValue(1.0); return binary("+",g121,g122); }()); return binary("*",g119,g120); }()); return binary("+",g115,g116); }()));
initialize(env,"d",([&]() -> JSValue { const auto g123=([&]() -> JSValue { const auto g125=read(env,"ix");const auto g126=JSValue(1.0); return binary("+",g125,g126); }());const auto g124=([&]() -> JSValue { const auto g127=read(env,"gridX1");const auto g128=read(env,"iy"); return binary("*",g127,g128); }()); return binary("+",g123,g124); }()));
([&]() -> JSValue { const auto g130=read(env,"indices");const auto g129=g130.get("push");Args g131;g131.push_back(read(env,"a"));g131.push_back(read(env,"b"));g131.push_back(read(env,"d")); return g129.call(g130,g131); }());
([&]() -> JSValue { const auto g133=read(env,"indices");const auto g132=g133.get("push");Args g134;g134.push_back(read(env,"b"));g134.push_back(read(env,"c"));g134.push_back(read(env,"d")); return g132.call(g133,g134); }());}}}}}}
([&]() -> JSValue { const auto g136=read(env,"$this");const auto g135=g136.get("setIndex");Args g137;g137.push_back(read(env,"indices")); return g135.call(g136,g137); }());
([&]() -> JSValue { const auto g139=read(env,"$this");const auto g138=g139.get("setAttribute");Args g140;g140.push_back(JSValue("position"));g140.push_back(([&]() -> JSValue { const auto g141=read(env,"Float32BufferAttribute");Args g143;g143.push_back(read(env,"vertices"));g143.push_back(JSValue(3.0)); return g141.construct(g143); }())); return g138.call(g139,g140); }());
([&]() -> JSValue { const auto g145=read(env,"$this");const auto g144=g145.get("setAttribute");Args g146;g146.push_back(JSValue("normal"));g146.push_back(([&]() -> JSValue { const auto g147=read(env,"Float32BufferAttribute");Args g149;g149.push_back(read(env,"normals"));g149.push_back(JSValue(3.0)); return g147.construct(g149); }())); return g144.call(g145,g146); }());
([&]() -> JSValue { const auto g151=read(env,"$this");const auto g150=g151.get("setAttribute");Args g152;g152.push_back(JSValue("uv"));g152.push_back(([&]() -> JSValue { const auto g153=read(env,"Float32BufferAttribute");Args g155;g155.push_back(read(env,"uvs"));g155.push_back(JSValue(2.0)); return g153.construct(g155); }())); return g150.call(g151,g152); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"source",true);initialize(env,"source",arg(args,0));{auto g156=env;auto env=scope(g156);([&]() -> JSValue { const auto g158=read(env,"$this");const auto g157=home.getPrototype().get("copy",g158);Args g159;g159.push_back(read(env,"source")); return g157.call(g158,g159); }());
([&]() -> JSValue { auto g160=read(env,"$this");const auto g161=JSValue("parameters");const auto g162=([&]() -> JSValue { const auto g164=read(env,"Object");const auto g163=g164.get("assign");Args g165;g165.push_back(([&]() -> JSValue { auto g166=JSValue::object(); return g166; }()));g165.push_back(([&]() -> JSValue { const auto g167=read(env,"source");const auto g168=JSValue("parameters"); return g167.get(g168); }())); return g163.call(g164,g165); }()); return setReference(g160,g161,g162); }());
return read(env,"$this");}return {};}
JSValue function2(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g170=env;auto env=scope(g170);return ([&]() -> JSValue { const auto g171=read(env,"PlaneGeometry");Args g173;g173.push_back(([&]() -> JSValue { const auto g174=read(env,"data");const auto g175=JSValue("width"); return g174.get(g175); }()));g173.push_back(([&]() -> JSValue { const auto g176=read(env,"data");const auto g177=JSValue("height"); return g176.get(g177); }()));g173.push_back(([&]() -> JSValue { const auto g178=read(env,"data");const auto g179=JSValue("widthSegments"); return g178.get(g179); }()));g173.push_back(([&]() -> JSValue { const auto g180=read(env,"data");const auto g181=JSValue("heightSegments"); return g180.get(g181); }())); return g171.construct(g173); }());}return {};}
}
JSValue nativePlaneGeometryModule(){using namespace PlaneGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"PlaneGeometry");auto env=scope();JSValue home;bindImport(env,"BufferGeometry",nativeBufferGeometryConstructor());bindImport(env,"Float32BufferAttribute",nativeFloat32BufferAttributeConstructor());declare(env,"PlaneGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"PlaneGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"BufferGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"PlaneGeometry",constructor);{auto home=prototype;auto function=([&]() -> JSValue { auto g169=JSValue::function(function1,{env,home},"copy",1);g169.setFunctionSourceText(source2); return g169; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("copy",d);}{auto home=constructor;auto function=([&]() -> JSValue { auto g182=JSValue::function(function2,{env,home},"fromJSON",1);g182.setFunctionSourceText(source4); return g182; }());function.setFunctionSourceText(source5);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"PlaneGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativePlaneGeometryBinding(const std::string& name){return nativePlaneGeometryModule().get(name);}
JSValue nativePlaneGeometryConstructor(){return nativePlaneGeometryBinding("PlaneGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
