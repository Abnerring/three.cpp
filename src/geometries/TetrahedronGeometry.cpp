#include "geometries/TetrahedronGeometry.h"
namespace {
PolyhedronGeometry::Numbers vertices() {  return {
			1, 1, 1, 	- 1, - 1, 1, 	- 1, 1, - 1, 	1, - 1, - 1
		}; }
PolyhedronGeometry::Numbers indices() { return {
			2, 1, 0, 	0, 3, 2,	1, 3, 0,	2, 3, 1
		}; }
}
TetrahedronGeometry::TetrahedronGeometry(double radius, double detail)
    : PolyhedronGeometry(vertices(), indices(), radius, detail) {
    type = "TetrahedronGeometry";
    parameters.vertices.reset(); parameters.indices.reset();
}
std::shared_ptr<BufferGeometry> TetrahedronGeometry::clone() const {
    auto result = std::make_shared<TetrahedronGeometry>(); result->copy(*this); return result;
}
std::shared_ptr<TetrahedronGeometry> TetrahedronGeometry::fromJSON(const JsonValue& data) {
    const auto number = [&](const char* key, double fallback) {
        return data.contains(key) ? (data.at(key).isNull() ? 0 : data.at(key).get<double>()) : fallback;
    };
    return std::make_shared<TetrahedronGeometry>(number("radius", 1), number("detail", 0));
}

// BEGIN GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4456)
#endif
#include "geometries/TetrahedronGeometry.h"
#include "geometries/GeometryRuntime.h"
#include "geometries/PolyhedronGeometry.h"
namespace TetrahedronGeometryManagedGenerated{using namespace GeometryRuntime;
inline constexpr char source0[]="class TetrahedronGeometry extends PolyhedronGeometry {\r\n\r\n\t/**\r\n\t * Constructs a new tetrahedron geometry.\r\n\t *\r\n\t * @param {number} [radius=1] - Radius of the tetrahedron.\r\n\t * @param {number} [detail=0] - Setting this to a value greater than `0` adds vertices making it no longer a tetrahedron.\r\n\t */\r\n\tconstructor( radius = 1, detail = 0 ) {\r\n\r\n\t\tconst vertices = [\r\n\t\t\t1, 1, 1, \t- 1, - 1, 1, \t- 1, 1, - 1, \t1, - 1, - 1\r\n\t\t];\r\n\r\n\t\tconst indices = [\r\n\t\t\t2, 1, 0, \t0, 3, 2,\t1, 3, 0,\t2, 3, 1\r\n\t\t];\r\n\r\n\t\tsuper( vertices, indices, radius, detail );\r\n\r\n\t\tthis.type = 'TetrahedronGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\tdetail: detail\r\n\t\t};\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Factory method for creating an instance of this class from the given\r\n\t * JSON object.\r\n\t *\r\n\t * @param {Object} data - A JSON object representing the serialized geometry.\r\n\t * @return {TetrahedronGeometry} A new instance.\r\n\t */\r\n\tstatic fromJSON( data ) {\r\n\r\n\t\treturn new TetrahedronGeometry( data.radius, data.detail );\r\n\r\n\t}\r\n\r\n}";
inline constexpr char source1[]="( radius = 1, detail = 0 ) {\r\n\r\n\t\tconst vertices = [\r\n\t\t\t1, 1, 1, \t- 1, - 1, 1, \t- 1, 1, - 1, \t1, - 1, - 1\r\n\t\t];\r\n\r\n\t\tconst indices = [\r\n\t\t\t2, 1, 0, \t0, 3, 2,\t1, 3, 0,\t2, 3, 1\r\n\t\t];\r\n\r\n\t\tsuper( vertices, indices, radius, detail );\r\n\r\n\t\tthis.type = 'TetrahedronGeometry';\r\n\r\n\t\t/**\r\n\t\t * Holds the constructor parameters that have been\r\n\t\t * used to generate the geometry. Any modification\r\n\t\t * after instantiation does not change the geometry.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.parameters = {\r\n\t\t\tradius: radius,\r\n\t\t\tdetail: detail\r\n\t\t};\r\n\r\n\t}";
inline constexpr char source2[]="( data ) {\r\n\r\n\t\treturn new TetrahedronGeometry( data.radius, data.detail );\r\n\r\n\t}";
inline constexpr char source3[]="fromJSON( data ) {\r\n\r\n\t\treturn new TetrahedronGeometry( data.radius, data.detail );\r\n\r\n\t}";
JSValue function0(const JSValue&,const Args&,const Args&);
JSValue function1(const JSValue&,const Args&,const Args&);
JSValue function0(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=read(env,"TetrahedronGeometry");declare(env,"$this",false);declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);declare(env,"radius",true);declare(env,"detail",true);auto g1=arg(args,0);if(g1.isUndefined())g1=JSValue(1.0);initialize(env,"radius",g1);auto g2=arg(args,1);if(g2.isUndefined())g2=JSValue(0.0);initialize(env,"detail",g2);{auto g3=env;auto env=scope(g3);declare(env,"vertices",false);declare(env,"indices",false);initialize(env,"vertices",([&]() -> JSValue { auto g4=JSValue::array(12);own(g4,JSValue(0.0),JSValue(1.0));own(g4,JSValue(1.0),JSValue(1.0));own(g4,JSValue(2.0),JSValue(1.0));own(g4,JSValue(3.0),JSValue(-JSValue(1.0).toNumber()));own(g4,JSValue(4.0),JSValue(-JSValue(1.0).toNumber()));own(g4,JSValue(5.0),JSValue(1.0));own(g4,JSValue(6.0),JSValue(-JSValue(1.0).toNumber()));own(g4,JSValue(7.0),JSValue(1.0));own(g4,JSValue(8.0),JSValue(-JSValue(1.0).toNumber()));own(g4,JSValue(9.0),JSValue(1.0));own(g4,JSValue(10.0),JSValue(-JSValue(1.0).toNumber()));own(g4,JSValue(11.0),JSValue(-JSValue(1.0).toNumber())); return g4; }()));
initialize(env,"indices",([&]() -> JSValue { auto g11=JSValue::array(12);own(g11,JSValue(0.0),JSValue(2.0));own(g11,JSValue(1.0),JSValue(1.0));own(g11,JSValue(2.0),JSValue(0.0));own(g11,JSValue(3.0),JSValue(0.0));own(g11,JSValue(4.0),JSValue(3.0));own(g11,JSValue(5.0),JSValue(2.0));own(g11,JSValue(6.0),JSValue(1.0));own(g11,JSValue(7.0),JSValue(3.0));own(g11,JSValue(8.0),JSValue(0.0));own(g11,JSValue(9.0),JSValue(2.0));own(g11,JSValue(10.0),JSValue(3.0));own(g11,JSValue(11.0),JSValue(1.0)); return g11; }()));
([&]() -> JSValue { const auto g12=home.getPrototype();Args g14;g14.push_back(read(env,"vertices"));g14.push_back(read(env,"indices"));g14.push_back(read(env,"radius"));g14.push_back(read(env,"detail"));const auto instance=g12.construct(g14,read(env,"$newTarget"));initialize(env,"$this",instance); return instance; }());
([&]() -> JSValue { auto g15=read(env,"$this");const auto g16=JSValue("type");const auto g17=JSValue("TetrahedronGeometry"); return setReference(g15,g16,g17); }());
([&]() -> JSValue { auto g18=read(env,"$this");const auto g19=JSValue("parameters");const auto g20=([&]() -> JSValue { auto g21=JSValue::object();const auto g22=JSValue("radius");own(g21,g22,read(env,"radius"));const auto g23=JSValue("detail");own(g21,g23,read(env,"detail")); return g21; }()); return setReference(g18,g19,g20); }());}return read(env,"$this");}
JSValue function1(const JSValue& receiver,const Args& args,const Args& captures){auto env=scope(captures[0]);auto home=captures[1];declare(env,"$this",false);initialize(env,"$this",receiver);declare(env,"data",true);initialize(env,"data",arg(args,0));{auto g24=env;auto env=scope(g24);return ([&]() -> JSValue { const auto g25=read(env,"TetrahedronGeometry");Args g27;g27.push_back(([&]() -> JSValue { const auto g28=read(env,"data");const auto g29=JSValue("radius"); return g28.get(g29); }()));g27.push_back(([&]() -> JSValue { const auto g30=read(env,"data");const auto g31=JSValue("detail"); return g30.get(g31); }())); return g25.construct(g27); }());}return {};}
}
JSValue nativeTetrahedronGeometryModule(){using namespace TetrahedronGeometryManagedGenerated;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"TetrahedronGeometry");auto env=scope();JSValue home;bindImport(env,"PolyhedronGeometry",nativePolyhedronGeometryConstructor());declare(env,"TetrahedronGeometry",false);auto constructor=JSValue::builtinConstructor(function0,{env},"TetrahedronGeometry",0);constructor.setFunctionSourceText(source0);constructor.setPrototype(read(env,"PolyhedronGeometry"));auto prototype=constructor.get("prototype");prototype.setPrototype(constructor.getPrototype().get("prototype"));home=constructor;initialize(env,"TetrahedronGeometry",constructor);{auto home=constructor;auto function=([&]() -> JSValue { auto g32=JSValue::function(function1,{env,home},"fromJSON",1);g32.setFunctionSourceText(source2); return g32; }());function.setFunctionSourceText(source3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}auto facade=JSValue::object();own(facade,"TetrahedronGeometry",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeTetrahedronGeometryBinding(const std::string& name){return nativeTetrahedronGeometryModule().get(name);}
JSValue nativeTetrahedronGeometryConstructor(){return nativeTetrahedronGeometryBinding("TetrahedronGeometry");}
// END GENERATED GEOMETRY RUNTIME
#ifdef _MSC_VER
#pragma warning(pop)
#endif
