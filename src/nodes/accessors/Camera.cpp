#include "nodes/accessors/Camera.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/accessors/BuiltinNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "math/Vector3.h"
#include <algorithm>
#include <array>
#include <string_view>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical() {static thread_local JSWeakValue weak;return weak;}

const char* cameraFunctionSource(std::string_view key) {
    if (key == "near") return "( { camera } ) => camera.near";
    if (key == "far") return "( { camera } ) => camera.far";
    if (key == "cameraProjectionMatrix") return "( { camera } ) => {\n\n\tlet cameraProjectionMatrix;\n\n\tif ( camera.isArrayCamera && camera.cameras.length > 0 ) {\n\n\t\tconst matrices = [];\n\n\t\tfor ( const subCamera of camera.cameras ) {\n\n\t\t\tmatrices.push( subCamera.projectionMatrix );\n\n\t\t}\n\n\t\tif ( _cameraProjectionMatrixArray === null ) {\n\n\t\t\t_cameraProjectionMatrixArray = uniformArray( matrices ).setGroup( renderGroup ).setName( 'cameraProjectionMatrices' );\n\n\t\t} else {\n\n\t\t\t_cameraProjectionMatrixArray.array = matrices;\n\n\t\t}\n\n\t\tcameraProjectionMatrix = _cameraProjectionMatrixArray.element( camera.isMultiViewCamera ? builtin( 'gl_ViewID_OVR' ) : cameraIndex );\n\n\t} else {\n\n\t\tif ( _cameraProjectionMatrixBase === null ) {\n\n\t\t\t_cameraProjectionMatrixBase = uniform( camera.projectionMatrix ).setName( 'cameraProjectionMatrix' ).setGroup( renderGroup ).onRenderUpdate( ( { camera } ) => camera.projectionMatrix );\n\n\t\t}\n\n\t\tcameraProjectionMatrix = _cameraProjectionMatrixBase;\n\n\t}\n\n\treturn cameraProjectionMatrix;\n\n}";
    if (key == "projectionMatrix") return "( { camera } ) => camera.projectionMatrix";
    if (key == "cameraProjectionMatrixInverse") return "( { camera } ) => {\n\n\tlet cameraProjectionMatrixInverse;\n\n\tif ( camera.isArrayCamera && camera.cameras.length > 0 ) {\n\n\t\tconst matrices = [];\n\n\t\tfor ( const subCamera of camera.cameras ) {\n\n\t\t\tmatrices.push( subCamera.projectionMatrixInverse );\n\n\t\t}\n\n\t\tif ( _cameraProjectionMatrixInverseArray === null ) {\n\n\t\t\t_cameraProjectionMatrixInverseArray = uniformArray( matrices ).setGroup( renderGroup ).setName( 'cameraProjectionMatricesInverse' );\n\n\t\t} else {\n\n\t\t\t_cameraProjectionMatrixInverseArray.array = matrices;\n\n\t\t}\n\n\t\tcameraProjectionMatrixInverse = _cameraProjectionMatrixInverseArray.element( camera.isMultiViewCamera ? builtin( 'gl_ViewID_OVR' ) : cameraIndex );\n\n\t} else {\n\n\t\tif ( _cameraProjectionMatrixInverseBase === null ) {\n\n\t\t\t_cameraProjectionMatrixInverseBase = uniform( camera.projectionMatrixInverse ).setName( 'cameraProjectionMatrixInverse' ).setGroup( renderGroup ).onRenderUpdate( ( { camera } ) => camera.projectionMatrixInverse );\n\n\t\t}\n\n\t\tcameraProjectionMatrixInverse = _cameraProjectionMatrixInverseBase;\n\n\t}\n\n\treturn cameraProjectionMatrixInverse;\n\n}";
    if (key == "projectionMatrixInverse") return "( { camera } ) => camera.projectionMatrixInverse";
    if (key == "cameraViewMatrix") return "( { camera } ) => {\n\n\tlet cameraViewMatrix;\n\n\tif ( camera.isArrayCamera && camera.cameras.length > 0 ) {\n\n\t\tconst matrices = [];\n\n\t\tfor ( const subCamera of camera.cameras ) {\n\n\t\t\tmatrices.push( subCamera.matrixWorldInverse );\n\n\t\t}\n\n\t\tif ( _cameraViewMatrixArray === null ) {\n\n\t\t\t_cameraViewMatrixArray = uniformArray( matrices ).setGroup( renderGroup ).setName( 'cameraViewMatrices' );\n\n\t\t} else {\n\n\t\t\t_cameraViewMatrixArray.array = matrices;\n\n\t\t}\n\n\t\tcameraViewMatrix = _cameraViewMatrixArray.element( camera.isMultiViewCamera ? builtin( 'gl_ViewID_OVR' ) : cameraIndex );\n\n\t} else {\n\n\t\tif ( _cameraViewMatrixBase === null ) {\n\n\t\t\t_cameraViewMatrixBase = uniform( camera.matrixWorldInverse ).setName( 'cameraViewMatrix' ).setGroup( renderGroup ).onRenderUpdate( ( { camera } ) => camera.matrixWorldInverse );\n\n\t\t}\n\n\t\tcameraViewMatrix = _cameraViewMatrixBase;\n\n\t}\n\n\treturn cameraViewMatrix;\n\n}";
    if (key == "matrixWorldInverse") return "( { camera } ) => camera.matrixWorldInverse";
    if (key == "cameraWorldMatrix") return "( { camera } ) => {\n\n\tlet cameraWorldMatrix;\n\n\tif ( camera.isArrayCamera && camera.cameras.length > 0 ) {\n\n\t\tconst matrices = [];\n\n\t\tfor ( const subCamera of camera.cameras ) {\n\n\t\t\tmatrices.push( subCamera.matrixWorld );\n\n\t\t}\n\n\t\tif ( _cameraWorldMatrixArray === null ) {\n\n\t\t\t_cameraWorldMatrixArray = uniformArray( matrices ).setGroup( renderGroup ).setName( 'cameraWorldMatrices' );\n\n\t\t} else {\n\n\t\t\t_cameraWorldMatrixArray.array = matrices;\n\n\t\t}\n\n\t\tcameraWorldMatrix = _cameraWorldMatrixArray.element( camera.isMultiViewCamera ? builtin( 'gl_ViewID_OVR' ) : cameraIndex );\n\n\t} else {\n\n\t\tif ( _cameraWorldMatrixBase === null ) {\n\n\t\t\t_cameraWorldMatrixBase = uniform( camera.matrixWorld ).setName( 'cameraWorldMatrix' ).setGroup( renderGroup ).onRenderUpdate( ( { camera } ) => camera.matrixWorld );\n\n\t\t}\n\n\t\tcameraWorldMatrix = _cameraWorldMatrixBase;\n\n\t}\n\n\treturn cameraWorldMatrix;\n\n}";
    if (key == "matrixWorld") return "( { camera } ) => camera.matrixWorld";
    if (key == "cameraNormalMatrix") return "( { camera } ) => {\n\n\tlet cameraNormalMatrix;\n\n\tif ( camera.isArrayCamera && camera.cameras.length > 0 ) {\n\n\t\tconst matrices = [];\n\n\t\tfor ( const subCamera of camera.cameras ) {\n\n\t\t\tmatrices.push( subCamera.normalMatrix );\n\n\t\t}\n\n\t\tif ( _cameraNormalMatrixArray === null ) {\n\n\t\t\t_cameraNormalMatrixArray = uniformArray( matrices ).setGroup( renderGroup ).setName( 'cameraNormalMatrices' );\n\n\t\t} else {\n\n\t\t\t_cameraNormalMatrixArray.array = matrices;\n\n\t\t}\n\n\t\tcameraNormalMatrix = _cameraNormalMatrixArray.element( camera.isMultiViewCamera ? builtin( 'gl_ViewID_OVR' ) : cameraIndex );\n\n\t} else {\n\n\t\tif ( _cameraNormalMatrixBase === null ) {\n\n\t\t\t_cameraNormalMatrixBase = uniform( camera.normalMatrix ).setName( 'cameraNormalMatrix' ).setGroup( renderGroup ).onRenderUpdate( ( { camera } ) => camera.normalMatrix );\n\n\t\t}\n\n\t\tcameraNormalMatrix = _cameraNormalMatrixBase;\n\n\t}\n\n\treturn cameraNormalMatrix;\n\n}";
    if (key == "normalMatrix") return "( { camera } ) => camera.normalMatrix";
    if (key == "cameraPosition") return "( { camera } ) => {\n\n\tlet cameraPosition;\n\n\tif ( camera.isArrayCamera && camera.cameras.length > 0 ) {\n\n\t\tconst positions = [];\n\n\t\tfor ( let i = 0, l = camera.cameras.length; i < l; i ++ ) {\n\n\t\t\tpositions.push( new Vector3() );\n\n\t\t}\n\n\t\tif ( _cameraPositionArray === null ) {\n\n\t\t\t_cameraPositionArray = uniformArray( positions ).setGroup( renderGroup ).setName( 'cameraPositions' ).onRenderUpdate( ( { camera }, self ) => {\n\n\t\t\t\tconst subCameras = camera.cameras;\n\t\t\t\tconst array = self.array;\n\n\t\t\t\tfor ( let i = 0, l = subCameras.length; i < l; i ++ ) {\n\n\t\t\t\t\tarray[ i ].setFromMatrixPosition( subCameras[ i ].matrixWorld );\n\n\t\t\t\t}\n\n\t\t\t} );\n\n\t\t} else {\n\n\t\t\t_cameraPositionArray.array = positions;\n\n\t\t}\n\n\t\tcameraPosition = _cameraPositionArray.element( camera.isMultiViewCamera ? builtin( 'gl_ViewID_OVR' ) : cameraIndex );\n\n\t} else {\n\n\t\tif ( _cameraPositionBase === null ) {\n\n\t\t\t_cameraPositionBase = uniform( new Vector3() ).setName( 'cameraPosition' ).setGroup( renderGroup ).onRenderUpdate( ( { camera }, self ) => self.value.setFromMatrixPosition( camera.matrixWorld ) );\n\n\t\t}\n\n\t\tcameraPosition = _cameraPositionBase;\n\n\t}\n\n\treturn cameraPosition;\n\n}";
    if (key == "positionArray") return "( { camera }, self ) => {\n\n\t\t\t\tconst subCameras = camera.cameras;\n\t\t\t\tconst array = self.array;\n\n\t\t\t\tfor ( let i = 0, l = subCameras.length; i < l; i ++ ) {\n\n\t\t\t\t\tarray[ i ].setFromMatrixPosition( subCameras[ i ].matrixWorld );\n\n\t\t\t\t}\n\n\t\t\t}";
    if (key == "positionBase") return "( { camera }, self ) => self.value.setFromMatrixPosition( camera.matrixWorld )";
    if (key == "cameraViewport") return "( { camera } ) => {\n\n\tlet cameraViewport;\n\n\tif ( camera.isArrayCamera && camera.cameras.length > 0 ) {\n\n\t\tconst viewports = [];\n\n\t\tfor ( const subCamera of camera.cameras ) {\n\n\t\t\tviewports.push( subCamera.viewport );\n\n\t\t}\n\n\t\tif ( _cameraViewportArray === null ) {\n\n\t\t\t_cameraViewportArray = uniformArray( viewports, 'vec4' ).setGroup( renderGroup ).setName( 'cameraViewports' );\n\n\t\t} else {\n\n\t\t\t_cameraViewportArray.array = viewports;\n\n\t\t}\n\n\t\tcameraViewport = _cameraViewportArray.element( cameraIndex );\n\n\t} else {\n\n\t\tif ( _cameraViewportBase === null ) {\n\n\t\t\t// Fallback for single camera\n\t\t\t_cameraViewportBase = vec4( 0, 0, screenSize.x, screenSize.y ).toConst( 'cameraViewport' );\n\n\t\t}\n\n\t\tcameraViewport = _cameraViewportBase;\n\n\t}\n\n\treturn cameraViewport;\n\n}";
    return "";
}
JSValue imports() {
    static thread_local JSWeakValue weak;if(const auto table=weak.lock();table&&table->isObject())return *table;
    const auto uniform=nativeUniformNodeConstructor(),render=nativeUniformGroupNodeBinding("renderGroup"),shared=nativeUniformGroupNodeBinding("sharedUniformGroup"),vector=nativeVector3Constructor(),tsl=nativeTSLBase(),array=nativeUniformArrayNodeModule(),builtin=nativeBuiltinNodeModule(),screen=nativeScreenNodeModule();
    auto table=JSValue::object(nullptr); own(table,"uniform",nativeUniformFactory());own(table,"renderGroup",render);own(table,"sharedUniformGroup",shared);own(table,"Vector3",vector);
    own(table,"Fn",nativeTSLBaseBinding("Fn"));own(table,"vec4",nativeTSLBaseBinding("vec4"));own(table,"uniformArray",nativeUniformArrayNodeBinding("uniformArray"));own(table,"builtin",nativeBuiltinNodeBinding("builtin"));own(table,"screenSize",nativeScreenNodeBinding("screenSize"));
    for(const auto* name:{"cameraProjectionMatrix","cameraProjectionMatrixInverse","cameraViewMatrix","cameraWorldMatrix","cameraNormalMatrix","cameraPosition","cameraViewport"}) {
        own(table,(std::string(name)+"Base").c_str(),nullptr);own(table,(std::string(name)+"Array").c_str(),nullptr);
    }
    table.retainForHeapLifetime();weak=JSWeakValue(table);return table;
}
JSValue update(const JSValue&,const Args& args,const Args& captures) {
    const auto camera=arg(args,0).get("camera");const auto kind=captures[0];
    if(kind.strictEquals("positionBase")) return invoke(arg(args,1).get("value"),"setFromMatrixPosition")({camera.get("matrixWorld")});
    if(kind.strictEquals("positionArray")) {
        const auto subCameras=camera.get("cameras"),array=arg(args,1).get("array"),length=subCameras.get("length");
        for(double i=0;less(i,length);++i) invoke(array.get(JSValue(i)),"setFromMatrixPosition")({subCameras.get(JSValue(i)).get("matrixWorld")});
        return {};
    }
    return camera.get(kind.toPropertyKey());
}
JSValue callback(const char* kind,unsigned arity=1) {
    auto function=JSValue::function(update,{kind},"",arity);
    function.setFunctionSourceText(cameraFunctionSource(kind));
    return function;
}
JSValue body(const JSValue&,const Args& args,const Args& captures) {
    const auto camera=arg(args,0).get("camera"),name=captures[1],property=captures[2],plural=captures[3];auto table=captures[0];
    const auto baseKey=name.string()+"Base",arrayKey=name.string()+"Array";
    const bool position=name.strictEquals("cameraPosition"),viewport=name.strictEquals("cameraViewport");
    if(camera.get("isArrayCamera").toBoolean()&&greater(camera.get("cameras").get("length"),0)) {
        auto values=JSValue::array({});
        if(position) {
            const auto length=camera.get("cameras").get("length");
            for(double i=0;less(i,length);++i) invoke(values,"push")({table.get("Vector3").construct()});
        } else each(camera.get("cameras"),[&](const JSValue& subCamera){invoke(values,"push")({subCamera.get(property.toPropertyKey())});});
        auto array=table.get(arrayKey);
        if(array.isNull()) {
            const auto uniform=viewport?table.get("uniformArray").call({}, {values,"vec4"}):table.get("uniformArray").call({}, {values});
            array=invoke(invoke(uniform,"setGroup")({table.get("renderGroup")}),"setName")({plural});
            if(position) array=invoke(array,"onRenderUpdate")({callback("positionArray",2)});
            table.set(arrayKey,array);
        } else array.set("array",values);
        const auto element=invoke(array,"element");
        const auto index=!viewport&&camera.get("isMultiViewCamera").toBoolean()?table.get("builtin").call({}, {"gl_ViewID_OVR"}):table.get("cameraIndex");
        return element({index});
    }
    auto base=table.get(baseKey);
    if(base.isNull()) {
        if(viewport) {
            const auto size=table.get("screenSize"),x=size.get("x"),y=size.get("y");
            base=invoke(table.get("vec4").call({}, {0,0,x,y}),"toConst")({"cameraViewport"});
        } else {
            const auto value=position?table.get("Vector3").construct():camera.get(property.toPropertyKey());
            base=invoke(invoke(invoke(table.get("uniform").call({}, {value}),"setName")({name}),"setGroup")({table.get("renderGroup")}),"onRenderUpdate")({position?callback("positionBase",2):callback(property.string().c_str())});
        }
        table.set(baseKey,base);
    }
    return base;
}
}
JSValue nativeCameraModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Camera exports");
    const auto table=imports();auto bindings=JSValue::object(nullptr);
    auto index=invoke(table.get("uniform").call({}, {0,"uint"}),"setName")({"u_cameraIndex"});
    index=invoke(index,"setGroup")({table.get("sharedUniformGroup").call({}, {"cameraIndex"})});
    index=invoke(index,"toVarying")({"v_cameraIndex"});own(bindings,"cameraIndex",index);own(table,"cameraIndex",index);
    for(const auto& [name,key]:std::initializer_list<std::pair<const char*,const char*>>{{"cameraNear","near"},{"cameraFar","far"}}) {
        const auto value=invoke(invoke(invoke(table.get("uniform").call({}, {"float"}),"setName")({name}),"setGroup")({table.get("renderGroup")}),"onRenderUpdate")({callback(key)});own(bindings,name,value);
    }
    for(const auto& spec:std::initializer_list<std::array<const char*,3>>{
        {"cameraProjectionMatrix","projectionMatrix","cameraProjectionMatrices"},{"cameraProjectionMatrixInverse","projectionMatrixInverse","cameraProjectionMatricesInverse"},
        {"cameraViewMatrix","matrixWorldInverse","cameraViewMatrices"},{"cameraWorldMatrix","matrixWorld","cameraWorldMatrices"},{"cameraNormalMatrix","normalMatrix","cameraNormalMatrices"},
        {"cameraPosition","matrixWorld","cameraPositions"},{"cameraViewport","viewport","cameraViewports"}}) {
        auto bodyFunction=JSValue::function(body,{table,spec[0],spec[1],spec[2]},"",1);
        bodyFunction.setFunctionSourceText(cameraFunctionSource(spec[0]));
        const auto fn=invoke(table.get("Fn").call({}, {bodyFunction}),"once")();own(bindings,spec[0],fn.call());
    }
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeCameraBinding(const std::string& name) {const auto keeper=nativeCameraModule();return lexical().lock()->get(name);}

bool CameraNames::indexHasPrefix() { return std::string(kIndexName).rfind("u_",0)==0; }
bool CameraNames::usesMatrixArray(bool isArrayCamera,std::size_t subCameraCount) { return isArrayCamera&&subCameraCount>0; }
