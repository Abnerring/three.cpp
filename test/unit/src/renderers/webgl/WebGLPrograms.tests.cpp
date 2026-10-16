#include "catch.hpp"
#include "parity/WebGLProgramsFixture.h"
#include "core/NativeObject.h"
#include "objects/Mesh.h"
#include "materials/ShaderMaterial.h"
#include "geometries/BoxGeometry.h"
#include "textures/Texture.h"
#include <atomic>
#include <thread>
using namespace ProgramsFixture;
namespace {
struct Context {
    JSValue renderer=dict({{"outputColorSpace","srgb"},{"toneMapping",0},{"shadowMap",dict({{"enabled",false},{"type",0}})}});
    JSValue environments=JSValue::object(),extensions=JSValue::object(),capabilities=dict({{"precision","highp"},{"logarithmicDepthBuffer",false}}),binding=JSValue::object(),clipping=dict({{"numPlanes",0},{"numIntersection",0}});
    JSValue lights=JSValue::object(),scene=dict({{"fog",nullptr},{"environment",nullptr}});
    Context(){
        data(renderer,"getRenderTarget",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return nullptr;}));
        const auto reversed=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return false;});
        data(renderer,"state",dict({{"buffers",dict({{"depth",dict({{"getReversed",reversed}})}})}}));
        data(environments,"get",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return arg(a,0);},{},"get",2));
        data(extensions,"has",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return false;},{},"has",1));
        data(capabilities,"getMaxPrecision",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return arg(a,0);},{},"getMaxPrecision",1));
        for(const auto* key:{"sun","directional","point","spot","spotLightMap","rectArea","hemi","sunShadowMap","directionalShadowMap","pointShadowMap","spotShadowMap"})data(lights,key,JSValue::array());
        data(lights,"numSpotLightShadowsWithMaps",0);data(lights,"numLightProbes",0);
    }
    WebGLPrograms make(){return WebGLPrograms(renderer,environments,extensions,capabilities,binding,clipping);}
    void gpu(){
        auto gl=dict({{"VERTEX_SHADER",35633},{"FRAGMENT_SHADER",35632}}),stats=dict({{"delete",0},{"release",0}});
        data(renderer,"fixtureStats",stats);
        for(const char* method:{"createProgram","createShader","shaderSource","compileShader","attachShader","bindAttribLocation","linkProgram","deleteProgram"})
            data(gl,method,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                if(c[0].strictEquals("createProgram")||c[0].strictEquals("createShader"))return JSValue::object();
                if(c[0].strictEquals("deleteProgram")){auto stats=c[1];data(stats,"delete",stats.get("delete").number()+1);}return {};
            },{method,stats},method));
        data(renderer,"getContext",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{gl}));
        data(binding,"releaseStatesOfProgram",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto s=c[0];data(s,"release",s.get("release").number()+1);return {};},{stats}));
    }
};
}
TEST_CASE("WebGLPrograms selects shaders from authoritative native mesh fields", "[webgl-programs]"){
    Context context;auto programs=context.make();
    auto geometry=std::make_shared<BoxGeometry>();auto material=std::make_shared<MeshBasicMaterial>();
    material->map=std::make_shared<Texture>();material->map->channel=2;
    auto mesh=std::make_shared<Mesh>(geometry,material);
    const auto p=programs.getParameters(nativeMaterial(material),context.lights,JSValue::array(),context.scene,nativeObject(mesh),JSValue::array());
    REQUIRE(p.get("shaderID").strictEquals("basic"));
    REQUIRE(p.get("vertexShader").sameValue(nativeShaderLib().get("basic").get("vertexShader")));
    REQUIRE(p.get("map").strictEquals(true));REQUIRE(p.get("mapUv").strictEquals("uv2"));
    REQUIRE(p.get("vertexUv2s").strictEquals(true));REQUIRE(p.get("vertexUv1s").strictEquals(false));
    REQUIRE(p.get("hasPositionAttribute").strictEquals(true));REQUIRE(p.get("vertexNormals").strictEquals(true));
    REQUIRE(p.get("outputColorSpace").strictEquals("srgb"));
    material->map->channel=3;
    const auto q=programs.getParameters(nativeMaterial(material),context.lights,JSValue::array(),context.scene,nativeObject(mesh),JSValue::array());
    REQUIRE(q.get("mapUv").strictEquals("uv3"));REQUIRE(q.get("vertexUv2s").strictEquals(false));REQUIRE(q.get("vertexUv3s").strictEquals(true));
    REQUIRE_FALSE(programs.getProgramCacheKey(p).strictEquals(programs.getProgramCacheKey(q)));
    const auto uniforms=programs.getUniforms(nativeMaterial(material));
    REQUIRE_FALSE(uniforms.sameValue(nativeShaderLib().get("basic").get("uniforms")));
    REQUIRE(uniforms.get("diffuse").get("value").isObject());
}
TEST_CASE("WebGLPrograms custom shader stages retain native materials until remove or dispose", "[webgl-programs]"){
    const auto realm=JSValue::object();Context context;auto programs=context.make();
    std::weak_ptr<ShaderMaterial> weakNative;JSWeakValue weakMaterial;JSValue parameters;
    {
        auto material=std::make_shared<ShaderMaterial>();material->vertexShader="unique vertex";material->fragmentShader="unique fragment";
        weakNative=material;auto wrapped=nativeMaterial(material);weakMaterial=JSWeakValue(wrapped);
        const auto object=dict({{"geometry",dict({{"attributes",JSValue::object()},{"morphAttributes",JSValue::object()}})}});
        parameters=programs.getParameters(wrapped,context.lights,JSValue::array(),context.scene,object,JSValue::array());
        const auto other=programs.getParameters(wrapped,context.lights,JSValue::array(),context.scene,object,JSValue::array());
        REQUIRE(parameters.get("customVertexShaderID").sameValue(other.get("customVertexShaderID")));
        REQUIRE(parameters.get("customFragmentShaderID").sameValue(other.get("customFragmentShaderID")));
        REQUIRE(programs.getUniforms(wrapped).sameValue(wrapped.get("uniforms")));
    }
    realm.collectGarbage();REQUIRE_FALSE(weakNative.expired());REQUIRE_FALSE(weakMaterial.expired());
    {const auto material=weakMaterial.lock();programs.releaseShaderCache(*material);}
    realm.collectGarbage();REQUIRE(weakMaterial.expired());REQUIRE(weakNative.expired());
    REQUIRE(programs.dispose().isUndefined());
    REQUIRE(programs.programs.value().length()==0);
}
TEST_CASE("WebGLPrograms exported closure and constructor prototype retain shared source environment", "[webgl-programs][gc]"){
    const auto realm=JSValue::object();
    for(const char* name:{"getParameters","getProgramCacheKey","getUniforms","acquireProgram","releaseProgram","releaseShaderCache","dispose"})for(const bool viaPrototype:{false,true}){
        JSValue keeper;std::vector<JSWeakValue> weak;
        {
            Context context;for(const auto& argument:Args{context.renderer,context.environments,context.extensions,context.capabilities,context.binding,context.clipping})weak.emplace_back(argument);
            auto programs=context.make();const auto fn=programs.object().get(name);
            if(viaPrototype){keeper=JSValue::object(fn.get("prototype"));REQUIRE(keeper.get("constructor").sameValue(fn));}
            else keeper=fn;
        }
        realm.collectGarbage();for(const auto& value:weak)REQUIRE_FALSE(value.expired());
        keeper={};realm.collectGarbage();for(const auto& value:weak)REQUIRE(value.expired());
    }
}
TEST_CASE("WebGLPrograms preserves cyclic cache environment collection and thread rejection", "[webgl-programs][gc]"){
    auto realm=JSValue::object();JSWeakValue weak;
    {
        Context context;auto programs=context.make();data(context.renderer,"cache",programs.object());weak=JSWeakValue(programs.object());
    }
    realm.collectGarbage();REQUIRE(weak.expired());
    Context context;auto programs=context.make();auto value=JSValue::object();std::atomic<int> rejected=0;
    std::thread worker([&]{const auto check=[&](auto action){try{action();}catch(const JSTypeError&){++rejected;}};
        check([&]{programs.object();});check([&]{WebGLPrograms copy(programs);});check([&]{WebGLPrograms move(std::move(programs));});
        check([&]{programs.getParameters(value,value,value,value,value,value);});check([&]{programs.getProgramCacheKey(value);});
        check([&]{programs.getUniforms(value);});check([&]{programs.acquireProgram(value,value);});check([&]{programs.releaseProgram(value);});
        check([&]{programs.releaseShaderCache(value);});check([&]{programs.dispose();});
    });worker.join();REQUIRE(rejected==10);REQUIRE(programs.object().isObject());
}
TEST_CASE("WebGLPrograms exposed methods remain ordinary constructible functions", "[webgl-programs]"){
    Context context;auto programs=context.make();
    REQUIRE(nativeWebGLProgramsConstructor().isConstructible());REQUIRE(nativeWebGLProgramsConstructor().get("length").number()==6);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,int>>{{"getParameters",6},{"getProgramCacheKey",1},{"getUniforms",1},{"acquireProgram",2},{"releaseProgram",1},{"releaseShaderCache",1},{"dispose",0}}){
        const auto fn=programs.object().get(name);REQUIRE(fn.isConstructible());REQUIRE(fn.get("name").strictEquals(name));REQUIRE(fn.get("length").number()==arity);
        REQUIRE(fn.get("prototype").get("constructor").sameValue(fn));const auto d=fn.getOwnPropertyDescriptor("prototype");
        REQUIRE(d->writable==true);REQUIRE(d->enumerable==false);REQUIRE(d->configurable==false);
    }
    auto result=programs.object().get("dispose").construct();REQUIRE(result.isObject());
    auto clone=programs;REQUIRE(clone.object().sameValue(programs.object()));auto moved=std::move(clone);REQUIRE(moved.object().sameValue(programs.object()));REQUIRE(clone.object().isUndefined());
}
TEST_CASE("WebGLPrograms lexical state and returned literals ignore inherited write traps", "[webgl-programs]"){
    Context context;auto warm=context.make();Restore restore;auto prototype=JSValue::object().getPrototype();
    const auto trap=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("inherited setter");});
    for(const char* key:{"renderer","layers","customShaders","activeChannels","programsMap","shaderIDs","getParameters","getProgramCacheKey","getUniforms","acquireProgram","releaseProgram","releaseShaderCache","programs","dispose"}){
        restore.save(prototype,key);JSPropertyDescriptor d;d.setter=trap;d.enumerable=false;d.configurable=true;prototype.defineProperty(key,d);
    }
    auto module=context.make();REQUIRE(module.programs.value().type()==JSValue::Type::Array);
    REQUIRE(module.object().hasOwn("getParameters"));REQUIRE(module.object().hasOwn("dispose"));
    REQUIRE(module.getProgramCacheKey(dict({{"shaderID","basic"},{"isRawShaderMaterial",true},{"customProgramCacheKey","x"}})).strictEquals("basic,x"));
    REQUIRE(module.dispose().isUndefined());
}
TEST_CASE("WebGLPrograms retains actual compiled protocols until last release and preserves public array alias", "[webgl-programs][gc]"){
    const auto realm=JSValue::object();Context context;context.gpu();auto module=context.make();
    const auto parameters=dict({{"isRawShaderMaterial",true},{"shaderType","Fixture"},{"shaderName","fixture"},{"defines",JSValue::object()},{"vertexShader","void main() { gl_Position = vec4( 0.0 ); }"},{"fragmentShader","void main() { gl_FragColor = vec4( 1.0 ); }"},{"rendererExtensionParallelShaderCompile",false}});
    JSWeakValue weakProgram,weakHandle;const auto monitor=module.programs.value();
    {
        const auto first=module.acquireProgram(parameters,"key"),second=module.acquireProgram(parameters,"key");
        REQUIRE(first.sameValue(second));REQUIRE(first.get("usedTimes").number()==2);REQUIRE(monitor.length()==1);
        weakProgram=JSWeakValue(first);weakHandle=JSWeakValue(first.get("program"));
    }
    module.programs=JSValue::array();module.dispose();realm.collectGarbage();
    REQUIRE_FALSE(weakProgram.expired());REQUIRE_FALSE(weakHandle.expired());REQUIRE(monitor.length()==1);
    {const auto program=weakProgram.lock();REQUIRE(program.has_value());module.releaseProgram(*program);REQUIRE(program->get("usedTimes").number()==1);}
    realm.collectGarbage();REQUIRE_FALSE(weakProgram.expired());
    {const auto program=weakProgram.lock();module.releaseProgram(*program);REQUIRE(program->get("program").isUndefined());}
    realm.collectGarbage();REQUIRE(weakProgram.expired());REQUIRE(weakHandle.expired());REQUIRE(monitor.length()==0);
    REQUIRE(context.renderer.get("fixtureStats").get("delete").number()==1);REQUIRE(context.renderer.get("fixtureStats").get("release").number()==1);
    REQUIRE(module.programs.value().length()==0);
}
