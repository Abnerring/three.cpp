#include "geometries/OctahedronGeometry.h"
namespace {
PolyhedronGeometry::Numbers vertices() {  return {
			1, 0, 0, 	- 1, 0, 0,	0, 1, 0,
			0, - 1, 0, 	0, 0, 1,	0, 0, - 1
		}; }
PolyhedronGeometry::Numbers indices() { return {
			0, 2, 4,	0, 4, 3,	0, 3, 5,
			0, 5, 2,	1, 2, 5,	1, 5, 3,
			1, 3, 4,	1, 4, 2
		}; }
}
OctahedronGeometry::OctahedronGeometry(double radius, double detail)
    : PolyhedronGeometry(vertices(), indices(), radius, detail) {
    type = "OctahedronGeometry";
    parameters.vertices.reset(); parameters.indices.reset();
}
std::shared_ptr<BufferGeometry> OctahedronGeometry::clone() const {
    auto result = std::make_shared<OctahedronGeometry>(); result->copy(*this); return result;
}
std::shared_ptr<OctahedronGeometry> OctahedronGeometry::fromJSON(const JsonValue& data) {
    const auto number = [&](const char* key, double fallback) {
        return data.contains(key) ? (data.at(key).isNull() ? 0 : data.at(key).get<double>()) : fallback;
    };
    return std::make_shared<OctahedronGeometry>(number("radius", 1), number("detail", 0));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/OctahedronGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "geometries/PolyhedronGeometry.h"
namespace OctahedronGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class OctahedronGeometry extends PolyhedronGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new octahedron geometry.\r\n\t *\r\n\t * @param {number} [radius=1] - Radius of the octahedron.\r\n\t * @param {number} [detail=0] - Setting this to a value greater than `0` adds vertices making it no longer a octahedron.\r\n\t */\r\n\tconstructor( radius = 1, detail = 0 ) {\r\n\r\n\t\tconst vertices = [\r\n\t\t\t1, 0, 0, \t- 1, 0, 0,\t0, 1, 0,\r\n\t\t\t0, - 1, 0, \t0, 0, 1,\t0, 0, - 1\r\n\t\t];\r\n\r\n\t\tconst indices = [\r\n\t\t\t0, 2, 4,\t0, 4, 3,\t0, 3, 5,\r\n\t\t\t0, 5, 2,\t1, 2, 5,\t1, 5, 3,\r\n\t\t\t1, 3, 4,\t1, 4, 2\r\n\t\t];\r\n\r\n\t\tsuper( vertices, indices, radius, detail );\r\n\r\n\t\tthis.type = 'OctahedronGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\tdetail: detail\r\n\t\t};\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {OctahedronGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new OctahedronGeometry( data.radius, data.detail );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( radius = 1, detail = 0 ) {\r\n\r\n\t\tconst vertices = [\r\n\t\t\t1, 0, 0, \t- 1, 0, 0,\t0, 1, 0,\r\n\t\t\t0, - 1, 0, \t0, 0, 1,\t0, 0, - 1\r\n\t\t];\r\n\r\n\t\tconst indices = [\r\n\t\t\t0, 2, 4,\t0, 4, 3,\t0, 3, 5,\r\n\t\t\t0, 5, 2,\t1, 2, 5,\t1, 5, 3,\r\n\t\t\t1, 3, 4,\t1, 4, 2\r\n\t\t];\r\n\r\n\t\tsuper( vertices, indices, radius, detail );\r\n\r\n\t\tthis.type = 'OctahedronGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\tdetail: detail\r\n\t\t};\r\n\r\n\t}";
inline constexpr char source2[]="( data ) {\r\n\r\n\t\treturn new OctahedronGeometry( data.radius, data.detail );\r\n\r\n\t}";
inline constexpr char source3[]="fromJSON( data ) {\r\n\r\n\t\treturn new OctahedronGeometry( data.radius, data.detail );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"OctahedronGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"radius",true);declare(env,"detail",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(1.0);initialize(env,"radius",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(0.0);initialize(env,"detail",g2);{auto g3=env;auto env=scope(g3);declare(env,"vertices",false);declare(env,"indices",false);initialize(env,"vertices",([&]() -> JSValue { auto g4=JSValue::array(18);own(g4,JSValue(0.0),JSValue(1.0));own(g4,JSValue(1.0),JSValue(0.0));own(g4,JSValue(2.0),JSValue(0.0));own(g4,JSValue(3.0),JSValue(-JSValue(1.0).toNumber()));own(g4,JSValue(4.0),JSValue(0.0));own(g4,JSValue(5.0),JSValue(0.0));own(g4,JSValue(6.0),JSValue(0.0));own(g4,JSValue(7.0),JSValue(1.0));own(g4,JSValue(8.0),JSValue(0.0));own(g4,JSValue(9.0),JSValue(0.0));own(g4,JSValue(10.0),JSValue(-JSValue(1.0).toNumber()));own(g4,JSValue(11.0),JSValue(0.0));own(g4,JSValue(12.0),JSValue(0.0));own(g4,JSValue(13.0),JSValue(0.0));own(g4,JSValue(14.0),JSValue(1.0));own(g4,JSValue(15.0),JSValue(0.0));own(g4,JSValue(16.0),JSValue(0.0));own(g4,JSValue(17.0),JSValue(-JSValue(1.0).toNumber())); return g4; }()));
initialize(env,"indices",([&]() -> JSValue { auto g8=JSValue::array(24);own(g8,JSValue(0.0),JSValue(0.0));own(g8,JSValue(1.0),JSValue(2.0));own(g8,JSValue(2.0),JSValue(4.0));own(g8,JSValue(3.0),JSValue(0.0));own(g8,JSValue(4.0),JSValue(4.0));own(g8,JSValue(5.0),JSValue(3.0));own(g8,JSValue(6.0),JSValue(0.0));own(g8,JSValue(7.0),JSValue(3.0));own(g8,JSValue(8.0),JSValue(5.0));own(g8,JSValue(9.0),JSValue(0.0));own(g8,JSValue(10.0),JSValue(5.0));own(g8,JSValue(11.0),JSValue(2.0));own(g8,JSValue(12.0),JSValue(1.0));own(g8,JSValue(13.0),JSValue(2.0));own(g8,JSValue(14.0),JSValue(5.0));own(g8,JSValue(15.0),JSValue(1.0));own(g8,JSValue(16.0),JSValue(5.0));own(g8,JSValue(17.0),JSValue(3.0));own(g8,JSValue(18.0),JSValue(1.0));own(g8,JSValue(19.0),JSValue(3.0));own(g8,JSValue(20.0),JSValue(4.0));own(g8,JSValue(21.0),JSValue(1.0));own(g8,JSValue(22.0),JSValue(4.0));own(g8,JSValue(23.0),JSValue(2.0)); return g8; }()));
([&]() -> JSValue { const auto g9=home.getPrototype();Args g11;g11.push_back(read(env,"vertices"));g11.push_back(read(env,"indices"));g11.push_back(read(env,"radius"));g11.push_back(read(env,"detail"));const auto instance=g9.construct(g11,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g12=read(env,"$this");const auto g13=JSValue("type");const auto g14=JSValue("OctahedronGeometry"); return setReference(g12,g13,g14); }());
([&]() -> JSValue { auto g15=read(env,"$this");const auto g16=JSValue("parameters");const auto g17=([&]() -> JSValue { auto g18=JSValue::object();const auto g19=JSValue("radius");own(g18,g19,read(env,"radius"));const auto g20=JSValue("detail");own(g18,g20,read(env,"detail")); return g18; }()); return setReference(g15,g16,g17); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g21=env;auto env=scope(g21);return ([&]() -> JSValue { const auto g22=read(env,"OctahedronGeometry");Args g24;g24.push_back(([&]() -> JSValue { const auto g25=read(env,"data");const auto g26=JSValue("radius"); return g25.get(g26); }()));g24.push_back(([&]() -> JSValue { const auto g27=read(env,"data");const auto g28=JSValue("detail"); return g27.get(g28); }())); return g22.construct(g24); }());}return {};}
}
JSValue nativeOctahedronGeometryModule(){using namespace OctahedronGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"OctahedronGeometry");auto env=scope();JSValue home;bindImport(env,"PolyhedronGeometry",nativePolyhedronGeometryConstructor());declare(env,"OctahedronGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"OctahedronGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"PolyhedronGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"OctahedronGeometry",constructor);{auto home=constructor;auto function=([&]() -> JSValue { auto g29=JSValue::function(function1,{env,home},"fromJSON",1);g29.setFunctionSourceText(source2); return g29; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"OctahedronGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeOctahedronGeometryBinding(const std::string& name){return nativeOctahedronGeometryModule().get(name);}
JSValue nativeOctahedronGeometryConstructor(){return nativeOctahedronGeometryBinding("OctahedronGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
