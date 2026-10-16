// Ported from three.js WebGLPrograms.js; see THREEJS-LICENSE.
#include "renderers/webgl/WebGLPrograms.h"
#include "renderers/webgl/WebGLProgram.h"
#include "renderers/webgl/WebGLShaderCache.h"
#include "renderers/shaders/ShaderLib.h"
#include "renderers/shaders/UniformsUtils.h"
#include "math/ColorManagement.h"
#include "core/Layers.h"
#include "core/JSMap.h"
#include "core/JSSet.h"
#include "core/JSString.h"
#include "core/JSNumber.h"
#include "constants.h"
#include "utils.h"
#include <tuple>
#include <unordered_set>

namespace {
using Args=std::vector<JSValue>;
JSValue argument(const Args& values,std::size_t index){return index<values.size()?values[index]:JSValue();}
void validate(std::initializer_list<const JSValue*> values){for(const auto* value:values)static_cast<void>(JSWeakValue(*value));}
void literal(JSValue object,const std::string& key,JSValue value){
    JSPropertyDescriptor descriptor;descriptor.value=std::move(value);
    descriptor.writable=descriptor.enumerable=descriptor.configurable=true;
    object.defineProperty(key,descriptor);
}
JSValue call(const JSValue& receiver,const char* key,const Args& args={}){
    const auto method=receiver.get(key);return method.call(receiver,args);
}
// Arguments are evaluated only after Get(method), as in a source method call.
template<class F> JSValue invoke(const JSValue& receiver,const char* key,F&& arguments){
    const auto method=receiver.get(key);return method.call(receiver,arguments());
}
template<class L,class R> JSValue logicalAnd(L&& left,R&& right){auto value=left();return value.toBoolean()?right():value;}
template<class L,class R> JSValue logicalOr(L&& left,R&& right){auto value=left();return value.toBoolean()?value:right();}
void pushProperty(const JSValue& array,const JSValue& source,const char* key){
    const auto push=array.get("push");const auto value=source.get(key);push.call(array,{value});
}
JSValue isPackedRGFormat(const JSValue& format){
    return format.strictEquals(RGFormat)||format.strictEquals(RG11_EAC_Format)||format.strictEquals(RED_GREEN_RGTC2_Format);
}
JSValue getChannel(const JSValue& state,const JSValue& value){
    call(state.get("activeChannels"),"add",{value});
    if(value.strictEquals(0))return "uv";
    return "uv"+value.toString();
}

JSValue getParameters(const JSValue&,const Args& args,const Args& captures){
    const auto state=captures[0];
    const auto renderer=state.get("renderer"),environments=state.get("environments"),extensions=state.get("extensions");
    const auto capabilities=state.get("capabilities"),clipping=state.get("clipping"),shaderIDs=state.get("shaderIDs");
    const auto customShaders=state.get("customShaders"),logarithmicDepthBuffer=state.get("logarithmicDepthBuffer");
    const auto material=argument(args,0),lights=argument(args,1),shadows=argument(args,2);
    const auto scene=argument(args,3),object=argument(args,4),lightProbeGrids=argument(args,5);
    const auto fog=scene.get("fog");
    const auto geometry=object.get("geometry");
    const auto environment=(material.get("isMeshStandardMaterial").toBoolean()||material.get("isMeshLambertMaterial").toBoolean()||material.get("isMeshPhongMaterial").toBoolean())?scene.get("environment"):JSValue(nullptr);
    const auto usePMREM=logicalOr([&]{return logicalOr([&]{return material.get("isMeshStandardMaterial");},[&]{return logicalAnd([&]{return material.get("isMeshLambertMaterial");},[&]{return JSValue(!material.get("envMap").toBoolean());});});},[&]{return logicalAnd([&]{return material.get("isMeshPhongMaterial");},[&]{return JSValue(!material.get("envMap").toBoolean());});});
    const auto envMap=invoke(environments,"get",[&]{return Args{logicalOr([&]{return material.get("envMap");},[&]{return environment;}),usePMREM};});
    const auto shaderID=shaderIDs.get(material.get("type"));
    auto precision=capabilities.get("precision");
    if(!material.get("precision").isNull()){
        precision=invoke(capabilities,"getMaxPrecision",[&]{return Args{material.get("precision")};});
        if(!precision.strictEquals(material.get("precision")))
            ThreeUtils::warn({"WebGLProgram.getParameters:",material.get("precision"),"not supported, using",precision,"instead."});
    }
    const auto morphAttribute=logicalOr([&]{return logicalOr([&]{return geometry.get("morphAttributes").get("position");},[&]{return geometry.get("morphAttributes").get("normal");});},[&]{return geometry.get("morphAttributes").get("color");});
    const auto morphTargetsCount=!morphAttribute.isUndefined()?morphAttribute.get("length"):JSValue(0);
    JSValue morphTextureStride=0;
    if(!geometry.get("morphAttributes").get("position").isUndefined())morphTextureStride=1;
    if(!geometry.get("morphAttributes").get("normal").isUndefined())morphTextureStride=2;
    if(!geometry.get("morphAttributes").get("color").isUndefined())morphTextureStride=3;
    JSValue vertexShader,fragmentShader,customVertexShaderID,customFragmentShaderID;
    if(shaderID.toBoolean()){
        const auto shader=nativeShaderLib().get(shaderID);
        vertexShader=shader.get("vertexShader");fragmentShader=shader.get("fragmentShader");
    }else{
        vertexShader=material.get("vertexShader");fragmentShader=material.get("fragmentShader");
        const auto vertexShaderStage=call(customShaders,"getVertexShaderStage",{material});
        const auto fragmentShaderStage=call(customShaders,"getFragmentShaderStage",{material});
        call(customShaders,"update",{material,vertexShaderStage,fragmentShaderStage});
        customVertexShaderID=vertexShaderStage.get("id");customFragmentShaderID=fragmentShaderStage.get("id");
    }
    const auto currentRenderTarget=call(renderer,"getRenderTarget");
    const auto reversedDepthBuffer=call(renderer.get("state").get("buffers").get("depth"),"getReversed");
    const auto IS_INSTANCEDMESH = ([&]{const auto left=(object).get("isInstancedMesh");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());
    const auto IS_BATCHEDMESH = ([&]{const auto left=(object).get("isBatchedMesh");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());
    const auto HAS_MAP = JSValue(!(JSValue(!((material).get("map")).toBoolean())).toBoolean());
    const auto HAS_MATCAP = JSValue(!(JSValue(!((material).get("matcap")).toBoolean())).toBoolean());
    const auto HAS_ENVMAP = JSValue(!(JSValue(!(envMap).toBoolean())).toBoolean());
    const auto HAS_PMREM = logicalAnd([&]{return HAS_ENVMAP;},[&]{return ([&]{const auto left=(envMap).get("isPMREMTexture");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());});
    const auto HAS_AOMAP = JSValue(!(JSValue(!((material).get("aoMap")).toBoolean())).toBoolean());
    const auto HAS_LIGHTMAP = JSValue(!(JSValue(!((material).get("lightMap")).toBoolean())).toBoolean());
    const auto HAS_BUMPMAP = logicalAnd([&]{return JSValue(!(JSValue(!((material).get("bumpMap")).toBoolean())).toBoolean());},[&]{return ([&]{const auto left=(material).get("wireframe");const auto right=JSValue(false);return JSValue(left.strictEquals(right));}());});
    const auto HAS_NORMALMAP = JSValue(!(JSValue(!((material).get("normalMap")).toBoolean())).toBoolean());
    const auto HAS_DISPLACEMENTMAP = JSValue(!(JSValue(!((material).get("displacementMap")).toBoolean())).toBoolean());
    const auto HAS_EMISSIVEMAP = JSValue(!(JSValue(!((material).get("emissiveMap")).toBoolean())).toBoolean());
    const auto HAS_METALNESSMAP = JSValue(!(JSValue(!((material).get("metalnessMap")).toBoolean())).toBoolean());
    const auto HAS_ROUGHNESSMAP = JSValue(!(JSValue(!((material).get("roughnessMap")).toBoolean())).toBoolean());
    const auto HAS_ANISOTROPY = ([&]{const auto left=(material).get("anisotropy");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_CLEARCOAT = ([&]{const auto left=(material).get("clearcoat");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_DIFFUSE_ROUGHNESS = ([&]{const auto left=(material).get("diffuseRoughness");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_DISPERSION = ([&]{const auto left=(material).get("dispersion");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_RETROREFLECTION = ([&]{const auto left=(material).get("retroreflectivity");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_IRIDESCENCE = ([&]{const auto left=(material).get("iridescence");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_SHEEN = ([&]{const auto left=(material).get("sheen");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_TRANSMISSION = ([&]{const auto left=(material).get("transmission");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_ANISOTROPYMAP = logicalAnd([&]{return HAS_ANISOTROPY;},[&]{return JSValue(!(JSValue(!((material).get("anisotropyMap")).toBoolean())).toBoolean());});
    const auto HAS_CLEARCOATMAP = logicalAnd([&]{return HAS_CLEARCOAT;},[&]{return JSValue(!(JSValue(!((material).get("clearcoatMap")).toBoolean())).toBoolean());});
    const auto HAS_CLEARCOAT_NORMALMAP = logicalAnd([&]{return HAS_CLEARCOAT;},[&]{return JSValue(!(JSValue(!((material).get("clearcoatNormalMap")).toBoolean())).toBoolean());});
    const auto HAS_CLEARCOAT_ROUGHNESSMAP = logicalAnd([&]{return HAS_CLEARCOAT;},[&]{return JSValue(!(JSValue(!((material).get("clearcoatRoughnessMap")).toBoolean())).toBoolean());});
    const auto HAS_DIFFUSE_ROUGHNESSMAP = logicalAnd([&]{return HAS_DIFFUSE_ROUGHNESS;},[&]{return JSValue(!(JSValue(!((material).get("diffuseRoughnessMap")).toBoolean())).toBoolean());});
    const auto HAS_IRIDESCENCEMAP = logicalAnd([&]{return HAS_IRIDESCENCE;},[&]{return JSValue(!(JSValue(!((material).get("iridescenceMap")).toBoolean())).toBoolean());});
    const auto HAS_IRIDESCENCE_THICKNESSMAP = logicalAnd([&]{return HAS_IRIDESCENCE;},[&]{return JSValue(!(JSValue(!((material).get("iridescenceThicknessMap")).toBoolean())).toBoolean());});
    const auto HAS_SHEEN_COLORMAP = logicalAnd([&]{return HAS_SHEEN;},[&]{return JSValue(!(JSValue(!((material).get("sheenColorMap")).toBoolean())).toBoolean());});
    const auto HAS_SHEEN_ROUGHNESSMAP = logicalAnd([&]{return HAS_SHEEN;},[&]{return JSValue(!(JSValue(!((material).get("sheenRoughnessMap")).toBoolean())).toBoolean());});
    const auto HAS_SPECULARMAP = JSValue(!(JSValue(!((material).get("specularMap")).toBoolean())).toBoolean());
    const auto HAS_SPECULAR_COLORMAP = JSValue(!(JSValue(!((material).get("specularColorMap")).toBoolean())).toBoolean());
    const auto HAS_SPECULAR_INTENSITYMAP = JSValue(!(JSValue(!((material).get("specularIntensityMap")).toBoolean())).toBoolean());
    const auto HAS_TRANSMISSIONMAP = logicalAnd([&]{return HAS_TRANSMISSION;},[&]{return JSValue(!(JSValue(!((material).get("transmissionMap")).toBoolean())).toBoolean());});
    const auto HAS_THICKNESSMAP = logicalAnd([&]{return HAS_TRANSMISSION;},[&]{return JSValue(!(JSValue(!((material).get("thicknessMap")).toBoolean())).toBoolean());});
    const auto HAS_GRADIENTMAP = JSValue(!(JSValue(!((material).get("gradientMap")).toBoolean())).toBoolean());
    const auto HAS_ALPHAMAP = JSValue(!(JSValue(!((material).get("alphaMap")).toBoolean())).toBoolean());
    const auto HAS_ALPHATEST = ([&]{const auto left=(material).get("alphaTest");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());
    const auto HAS_ALPHAHASH = JSValue(!(JSValue(!((material).get("alphaHash")).toBoolean())).toBoolean());
    const auto HAS_EXTENSIONS = JSValue(!(JSValue(!((material).get("extensions")).toBoolean())).toBoolean());
    JSValue toneMapping=NoToneMapping;
    if(material.get("toneMapped").toBoolean()){
        if(currentRenderTarget.isNull()||currentRenderTarget.get("isXRRenderTarget").strictEquals(true))toneMapping=renderer.get("toneMapping");
    }
    const auto envMapMipmaps=HAS_PMREM.toBoolean()?(envMap.get("isCompressedCubeTexture").toBoolean()?envMap.get("image").get("0").get("mipmaps"):envMap.get("mipmaps")):JSValue(nullptr);
    auto parameters=JSValue::object();
    literal(parameters,"shaderID",shaderID);
    literal(parameters,"shaderType",(material).get("type"));
    literal(parameters,"shaderName",(material).get("name"));
    literal(parameters,"vertexShader",vertexShader);
    literal(parameters,"fragmentShader",fragmentShader);
    literal(parameters,"defines",(material).get("defines"));
    literal(parameters,"customVertexShaderID",customVertexShaderID);
    literal(parameters,"customFragmentShaderID",customFragmentShaderID);
    literal(parameters,"isRawShaderMaterial",([&]{const auto left=(material).get("isRawShaderMaterial");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}()));
    literal(parameters,"glslVersion",(material).get("glslVersion"));
    literal(parameters,"precision",precision);
    literal(parameters,"batching",IS_BATCHEDMESH);
    literal(parameters,"batchingColor",logicalAnd([&]{return IS_BATCHEDMESH;},[&]{return ([&]{const auto left=(object).get("_colorsTexture");const auto right=JSValue(nullptr);return JSValue(!left.strictEquals(right));}());}));
    literal(parameters,"instancing",IS_INSTANCEDMESH);
    literal(parameters,"instancingColor",logicalAnd([&]{return IS_INSTANCEDMESH;},[&]{return ([&]{const auto left=(object).get("instanceColor");const auto right=JSValue(nullptr);return JSValue(!left.strictEquals(right));}());}));
    literal(parameters,"instancingMorph",logicalAnd([&]{return IS_INSTANCEDMESH;},[&]{return ([&]{const auto left=(object).get("morphTexture");const auto right=JSValue(nullptr);return JSValue(!left.strictEquals(right));}());}));
    literal(parameters,"outputColorSpace",((([&]{const auto left=currentRenderTarget;const auto right=JSValue(nullptr);return JSValue(left.strictEquals(right));}())).toBoolean() ? (renderer).get("outputColorSpace") : ((([&]{const auto left=(currentRenderTarget).get("isXRRenderTarget");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}())).toBoolean() ? ((currentRenderTarget).get("texture")).get("colorSpace") : (nativeColorManagement()).get("workingColorSpace"))));
    literal(parameters,"alphaToCoverage",JSValue(!(JSValue(!((material).get("alphaToCoverage")).toBoolean())).toBoolean()));
    literal(parameters,"map",HAS_MAP);
    literal(parameters,"matcap",HAS_MATCAP);
    literal(parameters,"envMap",HAS_ENVMAP);
    literal(parameters,"envMapMode",logicalAnd([&]{return HAS_ENVMAP;},[&]{return (envMap).get("mapping");}));
    literal(parameters,"envMapPMREM",HAS_PMREM);
    literal(parameters,"envMapMaxLod",((HAS_PMREM).toBoolean() ? ([&]{const auto left=(envMapMipmaps).get("length");const auto right=JSValue(1);return JSValue(left.toNumber()-right.toNumber());}()) : JSValue(nullptr)));
    literal(parameters,"envMapSize",((HAS_PMREM).toBoolean() ? ((envMapMipmaps).get(JSValue(0))).get("width") : JSValue(nullptr)));
    literal(parameters,"aoMap",HAS_AOMAP);
    literal(parameters,"lightMap",HAS_LIGHTMAP);
    literal(parameters,"bumpMap",HAS_BUMPMAP);
    literal(parameters,"normalMap",HAS_NORMALMAP);
    literal(parameters,"displacementMap",HAS_DISPLACEMENTMAP);
    literal(parameters,"emissiveMap",HAS_EMISSIVEMAP);
    literal(parameters,"normalMapObjectSpace",logicalAnd([&]{return HAS_NORMALMAP;},[&]{return ([&]{const auto left=(material).get("normalMapType");const auto right=JSValue(ObjectSpaceNormalMap);return JSValue(left.strictEquals(right));}());}));
    literal(parameters,"normalMapTangentSpace",logicalAnd([&]{return HAS_NORMALMAP;},[&]{return ([&]{const auto left=(material).get("normalMapType");const auto right=JSValue(TangentSpaceNormalMap);return JSValue(left.strictEquals(right));}());}));
    literal(parameters,"packedNormalMap",logicalAnd([&]{return logicalAnd([&]{return HAS_NORMALMAP;},[&]{return ([&]{const auto left=(material).get("normalMapType");const auto right=JSValue(TangentSpaceNormalMap);return JSValue(left.strictEquals(right));}());});},[&]{return isPackedRGFormat(((material).get("normalMap")).get("format"));}));
    literal(parameters,"metalnessMap",HAS_METALNESSMAP);
    literal(parameters,"roughnessMap",HAS_ROUGHNESSMAP);
    literal(parameters,"anisotropy",HAS_ANISOTROPY);
    literal(parameters,"anisotropyMap",HAS_ANISOTROPYMAP);
    literal(parameters,"clearcoat",HAS_CLEARCOAT);
    literal(parameters,"clearcoatMap",HAS_CLEARCOATMAP);
    literal(parameters,"clearcoatNormalMap",HAS_CLEARCOAT_NORMALMAP);
    literal(parameters,"clearcoatRoughnessMap",HAS_CLEARCOAT_ROUGHNESSMAP);
    literal(parameters,"diffuseRoughness",HAS_DIFFUSE_ROUGHNESS);
    literal(parameters,"diffuseRoughnessMap",HAS_DIFFUSE_ROUGHNESSMAP);
    literal(parameters,"dispersion",HAS_DISPERSION);
    literal(parameters,"retroreflection",HAS_RETROREFLECTION);
    literal(parameters,"iridescence",HAS_IRIDESCENCE);
    literal(parameters,"iridescenceMap",HAS_IRIDESCENCEMAP);
    literal(parameters,"iridescenceThicknessMap",HAS_IRIDESCENCE_THICKNESSMAP);
    literal(parameters,"sheen",HAS_SHEEN);
    literal(parameters,"sheenColorMap",HAS_SHEEN_COLORMAP);
    literal(parameters,"sheenRoughnessMap",HAS_SHEEN_ROUGHNESSMAP);
    literal(parameters,"specularMap",HAS_SPECULARMAP);
    literal(parameters,"specularColorMap",HAS_SPECULAR_COLORMAP);
    literal(parameters,"specularIntensityMap",HAS_SPECULAR_INTENSITYMAP);
    literal(parameters,"transmission",HAS_TRANSMISSION);
    literal(parameters,"transmissionMap",HAS_TRANSMISSIONMAP);
    literal(parameters,"thicknessMap",HAS_THICKNESSMAP);
    literal(parameters,"gradientMap",HAS_GRADIENTMAP);
    literal(parameters,"opaque",logicalAnd([&]{return logicalAnd([&]{return ([&]{const auto left=(material).get("transparent");const auto right=JSValue(false);return JSValue(left.strictEquals(right));}());},[&]{return ([&]{const auto left=(material).get("blending");const auto right=JSValue(NormalBlending);return JSValue(left.strictEquals(right));}());});},[&]{return ([&]{const auto left=(material).get("alphaToCoverage");const auto right=JSValue(false);return JSValue(left.strictEquals(right));}());}));
    literal(parameters,"alphaMap",HAS_ALPHAMAP);
    literal(parameters,"alphaTest",HAS_ALPHATEST);
    literal(parameters,"alphaHash",HAS_ALPHAHASH);
    literal(parameters,"combine",(material).get("combine"));
    literal(parameters,"mapUv",logicalAnd([&]{return HAS_MAP;},[&]{return getChannel(state,((material).get("map")).get("channel"));}));
    literal(parameters,"aoMapUv",logicalAnd([&]{return HAS_AOMAP;},[&]{return getChannel(state,((material).get("aoMap")).get("channel"));}));
    literal(parameters,"lightMapUv",logicalAnd([&]{return HAS_LIGHTMAP;},[&]{return getChannel(state,((material).get("lightMap")).get("channel"));}));
    literal(parameters,"bumpMapUv",logicalAnd([&]{return HAS_BUMPMAP;},[&]{return getChannel(state,((material).get("bumpMap")).get("channel"));}));
    literal(parameters,"normalMapUv",logicalAnd([&]{return HAS_NORMALMAP;},[&]{return getChannel(state,((material).get("normalMap")).get("channel"));}));
    literal(parameters,"displacementMapUv",logicalAnd([&]{return HAS_DISPLACEMENTMAP;},[&]{return getChannel(state,((material).get("displacementMap")).get("channel"));}));
    literal(parameters,"emissiveMapUv",logicalAnd([&]{return HAS_EMISSIVEMAP;},[&]{return getChannel(state,((material).get("emissiveMap")).get("channel"));}));
    literal(parameters,"metalnessMapUv",logicalAnd([&]{return HAS_METALNESSMAP;},[&]{return getChannel(state,((material).get("metalnessMap")).get("channel"));}));
    literal(parameters,"roughnessMapUv",logicalAnd([&]{return HAS_ROUGHNESSMAP;},[&]{return getChannel(state,((material).get("roughnessMap")).get("channel"));}));
    literal(parameters,"anisotropyMapUv",logicalAnd([&]{return HAS_ANISOTROPYMAP;},[&]{return getChannel(state,((material).get("anisotropyMap")).get("channel"));}));
    literal(parameters,"clearcoatMapUv",logicalAnd([&]{return HAS_CLEARCOATMAP;},[&]{return getChannel(state,((material).get("clearcoatMap")).get("channel"));}));
    literal(parameters,"clearcoatNormalMapUv",logicalAnd([&]{return HAS_CLEARCOAT_NORMALMAP;},[&]{return getChannel(state,((material).get("clearcoatNormalMap")).get("channel"));}));
    literal(parameters,"clearcoatRoughnessMapUv",logicalAnd([&]{return HAS_CLEARCOAT_ROUGHNESSMAP;},[&]{return getChannel(state,((material).get("clearcoatRoughnessMap")).get("channel"));}));
    literal(parameters,"diffuseRoughnessMapUv",logicalAnd([&]{return HAS_DIFFUSE_ROUGHNESSMAP;},[&]{return getChannel(state,((material).get("diffuseRoughnessMap")).get("channel"));}));
    literal(parameters,"iridescenceMapUv",logicalAnd([&]{return HAS_IRIDESCENCEMAP;},[&]{return getChannel(state,((material).get("iridescenceMap")).get("channel"));}));
    literal(parameters,"iridescenceThicknessMapUv",logicalAnd([&]{return HAS_IRIDESCENCE_THICKNESSMAP;},[&]{return getChannel(state,((material).get("iridescenceThicknessMap")).get("channel"));}));
    literal(parameters,"sheenColorMapUv",logicalAnd([&]{return HAS_SHEEN_COLORMAP;},[&]{return getChannel(state,((material).get("sheenColorMap")).get("channel"));}));
    literal(parameters,"sheenRoughnessMapUv",logicalAnd([&]{return HAS_SHEEN_ROUGHNESSMAP;},[&]{return getChannel(state,((material).get("sheenRoughnessMap")).get("channel"));}));
    literal(parameters,"specularMapUv",logicalAnd([&]{return HAS_SPECULARMAP;},[&]{return getChannel(state,((material).get("specularMap")).get("channel"));}));
    literal(parameters,"specularColorMapUv",logicalAnd([&]{return HAS_SPECULAR_COLORMAP;},[&]{return getChannel(state,((material).get("specularColorMap")).get("channel"));}));
    literal(parameters,"specularIntensityMapUv",logicalAnd([&]{return HAS_SPECULAR_INTENSITYMAP;},[&]{return getChannel(state,((material).get("specularIntensityMap")).get("channel"));}));
    literal(parameters,"transmissionMapUv",logicalAnd([&]{return HAS_TRANSMISSIONMAP;},[&]{return getChannel(state,((material).get("transmissionMap")).get("channel"));}));
    literal(parameters,"thicknessMapUv",logicalAnd([&]{return HAS_THICKNESSMAP;},[&]{return getChannel(state,((material).get("thicknessMap")).get("channel"));}));
    literal(parameters,"alphaMapUv",logicalAnd([&]{return HAS_ALPHAMAP;},[&]{return getChannel(state,((material).get("alphaMap")).get("channel"));}));
    literal(parameters,"vertexTangents",logicalAnd([&]{return JSValue(!(JSValue(!(((geometry).get("attributes")).get("tangent")).toBoolean())).toBoolean());},[&]{return logicalOr([&]{return HAS_NORMALMAP;},[&]{return HAS_ANISOTROPY;});}));
    literal(parameters,"vertexNormals",JSValue(!(JSValue(!(((geometry).get("attributes")).get("normal")).toBoolean())).toBoolean()));
    literal(parameters,"vertexColors",(material).get("vertexColors"));
    literal(parameters,"vertexAlphas",logicalAnd([&]{return logicalAnd([&]{return ([&]{const auto left=(material).get("vertexColors");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());},[&]{return JSValue(!(JSValue(!(((geometry).get("attributes")).get("color")).toBoolean())).toBoolean());});},[&]{return ([&]{const auto left=(((geometry).get("attributes")).get("color")).get("itemSize");const auto right=JSValue(4);return JSValue(left.strictEquals(right));}());}));
    literal(parameters,"pointsUvs",logicalAnd([&]{return logicalAnd([&]{return ([&]{const auto left=(object).get("isPoints");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());},[&]{return JSValue(!(JSValue(!(((geometry).get("attributes")).get("uv")).toBoolean())).toBoolean());});},[&]{return logicalOr([&]{return HAS_MAP;},[&]{return HAS_ALPHAMAP;});}));
    literal(parameters,"fog",JSValue(!(JSValue(!(fog).toBoolean())).toBoolean()));
    literal(parameters,"useFog",([&]{const auto left=(material).get("fog");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}()));
    literal(parameters,"fogExp2",logicalAnd([&]{return JSValue(!(JSValue(!(fog).toBoolean())).toBoolean());},[&]{return (fog).get("isFogExp2");}));
    literal(parameters,"flatShading",logicalAnd([&]{return ([&]{const auto left=(material).get("wireframe");const auto right=JSValue(false);return JSValue(left.strictEquals(right));}());},[&]{return logicalOr([&]{return ([&]{const auto left=(material).get("flatShading");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());},[&]{return logicalAnd([&]{return logicalAnd([&]{return ([&]{const auto left=((geometry).get("attributes")).get("normal");const auto right=JSValue();return JSValue(left.strictEquals(right));}());},[&]{return ([&]{const auto left=HAS_NORMALMAP;const auto right=JSValue(false);return JSValue(left.strictEquals(right));}());});},[&]{return logicalOr([&]{return logicalOr([&]{return logicalOr([&]{return (material).get("isMeshLambertMaterial");},[&]{return (material).get("isMeshPhongMaterial");});},[&]{return (material).get("isMeshStandardMaterial");});},[&]{return (material).get("isMeshPhysicalMaterial");});});});}));
    literal(parameters,"sizeAttenuation",([&]{const auto left=(material).get("sizeAttenuation");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}()));
    literal(parameters,"logarithmicDepthBuffer",logarithmicDepthBuffer);
    literal(parameters,"reversedDepthBuffer",reversedDepthBuffer);
    literal(parameters,"skinning",([&]{const auto left=(object).get("isSkinnedMesh");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}()));
    literal(parameters,"hasPositionAttribute",([&]{const auto left=((geometry).get("attributes")).get("position");const auto right=JSValue();return JSValue(!left.strictEquals(right));}()));
    literal(parameters,"morphTargets",([&]{const auto left=((geometry).get("morphAttributes")).get("position");const auto right=JSValue();return JSValue(!left.strictEquals(right));}()));
    literal(parameters,"morphNormals",([&]{const auto left=((geometry).get("morphAttributes")).get("normal");const auto right=JSValue();return JSValue(!left.strictEquals(right));}()));
    literal(parameters,"morphColors",([&]{const auto left=((geometry).get("morphAttributes")).get("color");const auto right=JSValue();return JSValue(!left.strictEquals(right));}()));
    literal(parameters,"morphTargetsCount",morphTargetsCount);
    literal(parameters,"morphTextureStride",morphTextureStride);
    literal(parameters,"numSunLights",((lights).get("sun")).get("length"));
    literal(parameters,"numDirLights",((lights).get("directional")).get("length"));
    literal(parameters,"numPointLights",((lights).get("point")).get("length"));
    literal(parameters,"numSpotLights",((lights).get("spot")).get("length"));
    literal(parameters,"numSpotLightMaps",((lights).get("spotLightMap")).get("length"));
    literal(parameters,"numRectAreaLights",((lights).get("rectArea")).get("length"));
    literal(parameters,"numHemiLights",((lights).get("hemi")).get("length"));
    literal(parameters,"numSunLightShadows",((lights).get("sunShadowMap")).get("length"));
    literal(parameters,"numDirLightShadows",((lights).get("directionalShadowMap")).get("length"));
    literal(parameters,"numPointLightShadows",((lights).get("pointShadowMap")).get("length"));
    literal(parameters,"numSpotLightShadows",((lights).get("spotShadowMap")).get("length"));
    literal(parameters,"numSpotLightShadowsWithMaps",(lights).get("numSpotLightShadowsWithMaps"));
    literal(parameters,"numLightProbes",(lights).get("numLightProbes"));
    literal(parameters,"numLightProbeGrids",(lightProbeGrids).get("length"));
    literal(parameters,"numClippingPlanes",(clipping).get("numPlanes"));
    literal(parameters,"numClipIntersection",(clipping).get("numIntersection"));
    literal(parameters,"dithering",(material).get("dithering"));
    literal(parameters,"shadowMapEnabled",logicalAnd([&]{return ((renderer).get("shadowMap")).get("enabled");},[&]{return ([&]{const auto left=(shadows).get("length");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}());}));
    literal(parameters,"shadowMapType",((renderer).get("shadowMap")).get("type"));
    literal(parameters,"toneMapping",toneMapping);
    literal(parameters,"decodeVideoTexture",logicalAnd([&]{return logicalAnd([&]{return HAS_MAP;},[&]{return ([&]{const auto left=((material).get("map")).get("isVideoTexture");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());});},[&]{return ([&]{const auto left=invoke(nativeColorManagement(),"getTransfer",[&]{return Args{((material).get("map")).get("colorSpace")};});const auto right=JSValue(SRGBTransfer);return JSValue(left.strictEquals(right));}());}));
    literal(parameters,"decodeVideoTextureEmissive",logicalAnd([&]{return logicalAnd([&]{return HAS_EMISSIVEMAP;},[&]{return ([&]{const auto left=((material).get("emissiveMap")).get("isVideoTexture");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());});},[&]{return ([&]{const auto left=invoke(nativeColorManagement(),"getTransfer",[&]{return Args{((material).get("emissiveMap")).get("colorSpace")};});const auto right=JSValue(SRGBTransfer);return JSValue(left.strictEquals(right));}());}));
    literal(parameters,"premultipliedAlpha",(material).get("premultipliedAlpha"));
    literal(parameters,"doubleSided",([&]{const auto left=(material).get("side");const auto right=JSValue(DoubleSide);return JSValue(left.strictEquals(right));}()));
    literal(parameters,"flipSided",([&]{const auto left=(material).get("side");const auto right=JSValue(BackSide);return JSValue(left.strictEquals(right));}()));
    literal(parameters,"useDepthPacking",([&]{const auto left=(material).get("depthPacking");const auto right=JSValue(0);return JSValue(left.toNumber()>=right.toNumber());}()));
    literal(parameters,"depthPacking",logicalOr([&]{return (material).get("depthPacking");},[&]{return JSValue(0);}));
    literal(parameters,"index0AttributeName",(material).get("index0AttributeName"));
    literal(parameters,"extensionClipCullDistance",logicalAnd([&]{return logicalAnd([&]{return HAS_EXTENSIONS;},[&]{return ([&]{const auto left=((material).get("extensions")).get("clipCullDistance");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());});},[&]{return invoke(extensions,"has",[&]{return Args{JSValue("WEBGL_clip_cull_distance")};});}));
    literal(parameters,"extensionMultiDraw",logicalAnd([&]{return logicalOr([&]{return logicalAnd([&]{return HAS_EXTENSIONS;},[&]{return ([&]{const auto left=((material).get("extensions")).get("multiDraw");const auto right=JSValue(true);return JSValue(left.strictEquals(right));}());});},[&]{return IS_BATCHEDMESH;});},[&]{return invoke(extensions,"has",[&]{return Args{JSValue("WEBGL_multi_draw")};});}));
    literal(parameters,"rendererExtensionParallelShaderCompile",invoke(extensions,"has",[&]{return Args{JSValue("KHR_parallel_shader_compile")};}));
    literal(parameters,"customProgramCacheKey",invoke(material,"customProgramCacheKey",[&]{return Args{};}));
    parameters.set("vertexUv1s",call(state.get("activeChannels"),"has",{1}));
    parameters.set("vertexUv2s",call(state.get("activeChannels"),"has",{2}));
    parameters.set("vertexUv3s",call(state.get("activeChannels"),"has",{3}));
    call(state.get("activeChannels"),"clear");
    return parameters;
}

// The supported ordinary-object enumeration protocol: names are snapshotted,
// duplicates (including non-enumerable own shadows) are suppressed, deleted
// properties are checked again before each iteration. Symbols are excluded.
std::vector<std::string> enumerableNames(JSValue object){
    if(object.type()==JSValue::Type::String)object=createNativeStringObject(object.string());
    else if(object.type()==JSValue::Type::Number)object=createNativeNumberObject(object.number());
    if(!object.isObject())return {};
    std::unordered_set<std::string> visited;
    std::vector<std::string> result;
    for(auto cursor=object;cursor.isObject();cursor=cursor.getPrototype()){
        for(const auto& key:cursor.ownPropertyNames())if(visited.insert(key).second){
            const auto descriptor=cursor.getOwnPropertyDescriptor(key);
            if(descriptor&&descriptor->enumerable.value_or(false))result.push_back(key);
        }
    }
    return result;
}
void cacheKeyParameters(const JSValue& array,const JSValue& parameters){
    for(const auto* key:{"precision","outputColorSpace","envMapMode","envMapPMREM","envMapMaxLod","envMapSize","mapUv","alphaMapUv","lightMapUv","aoMapUv","bumpMapUv","normalMapUv","displacementMapUv","emissiveMapUv","metalnessMapUv","roughnessMapUv","anisotropyMapUv","clearcoatMapUv","clearcoatNormalMapUv","clearcoatRoughnessMapUv","diffuseRoughnessMapUv","iridescenceMapUv","iridescenceThicknessMapUv","sheenColorMapUv","sheenRoughnessMapUv","specularMapUv","specularColorMapUv","specularIntensityMapUv","transmissionMapUv","thicknessMapUv","combine","fogExp2","sizeAttenuation","morphTargetsCount","numSunLights","numDirLights","numPointLights","numSpotLights","numSpotLightMaps","numHemiLights","numRectAreaLights","numSunLightShadows","numDirLightShadows","numPointLightShadows","numSpotLightShadows","numSpotLightShadowsWithMaps","numLightProbes","shadowMapType","toneMapping","numClippingPlanes","numClipIntersection","depthPacking"})pushProperty(array,parameters,key);
}
void cacheKeyBooleans(const JSValue& state,const JSValue& array,const JSValue& parameters){
    const auto layers=state.get("layers");
    call(layers,"disableAll");
    if(((parameters).get("instancing")).toBoolean())call(layers,"enable",{0});
    if(((parameters).get("instancingColor")).toBoolean())call(layers,"enable",{1});
    if(((parameters).get("instancingMorph")).toBoolean())call(layers,"enable",{2});
    if(((parameters).get("matcap")).toBoolean())call(layers,"enable",{3});
    if(((parameters).get("envMap")).toBoolean())call(layers,"enable",{4});
    if(((parameters).get("normalMapObjectSpace")).toBoolean())call(layers,"enable",{5});
    if(((parameters).get("normalMapTangentSpace")).toBoolean())call(layers,"enable",{6});
    if(((parameters).get("clearcoat")).toBoolean())call(layers,"enable",{7});
    if(((parameters).get("iridescence")).toBoolean())call(layers,"enable",{8});
    if(((parameters).get("alphaTest")).toBoolean())call(layers,"enable",{9});
    if(((parameters).get("vertexColors")).toBoolean())call(layers,"enable",{10});
    if(((parameters).get("vertexAlphas")).toBoolean())call(layers,"enable",{11});
    if(((parameters).get("vertexUv1s")).toBoolean())call(layers,"enable",{12});
    if(((parameters).get("vertexUv2s")).toBoolean())call(layers,"enable",{13});
    if(((parameters).get("vertexUv3s")).toBoolean())call(layers,"enable",{14});
    if(((parameters).get("vertexTangents")).toBoolean())call(layers,"enable",{15});
    if(((parameters).get("anisotropy")).toBoolean())call(layers,"enable",{16});
    if(((parameters).get("alphaHash")).toBoolean())call(layers,"enable",{17});
    if(((parameters).get("batching")).toBoolean())call(layers,"enable",{18});
    if(((parameters).get("dispersion")).toBoolean())call(layers,"enable",{19});
    if(((parameters).get("retroreflection")).toBoolean())call(layers,"enable",{24});
    if(((parameters).get("diffuseRoughness")).toBoolean())call(layers,"enable",{25});
    if(((parameters).get("batchingColor")).toBoolean())call(layers,"enable",{20});
    if(((parameters).get("gradientMap")).toBoolean())call(layers,"enable",{21});
    if(((parameters).get("packedNormalMap")).toBoolean())call(layers,"enable",{22});
    if(((parameters).get("vertexNormals")).toBoolean())call(layers,"enable",{23});
    pushProperty(array,layers,"mask");
    call(layers,"disableAll");
    if(((parameters).get("fog")).toBoolean())call(layers,"enable",{0});
    if(((parameters).get("useFog")).toBoolean())call(layers,"enable",{1});
    if(((parameters).get("flatShading")).toBoolean())call(layers,"enable",{2});
    if(((parameters).get("logarithmicDepthBuffer")).toBoolean())call(layers,"enable",{3});
    if(((parameters).get("reversedDepthBuffer")).toBoolean())call(layers,"enable",{4});
    if(((parameters).get("skinning")).toBoolean())call(layers,"enable",{5});
    if(((parameters).get("morphTargets")).toBoolean())call(layers,"enable",{6});
    if(((parameters).get("morphNormals")).toBoolean())call(layers,"enable",{7});
    if(((parameters).get("morphColors")).toBoolean())call(layers,"enable",{8});
    if(((parameters).get("premultipliedAlpha")).toBoolean())call(layers,"enable",{9});
    if(((parameters).get("shadowMapEnabled")).toBoolean())call(layers,"enable",{10});
    if(((parameters).get("doubleSided")).toBoolean())call(layers,"enable",{11});
    if(((parameters).get("flipSided")).toBoolean())call(layers,"enable",{12});
    if(((parameters).get("useDepthPacking")).toBoolean())call(layers,"enable",{13});
    if(((parameters).get("dithering")).toBoolean())call(layers,"enable",{14});
    if(((parameters).get("transmission")).toBoolean())call(layers,"enable",{15});
    if(((parameters).get("sheen")).toBoolean())call(layers,"enable",{16});
    if(((parameters).get("opaque")).toBoolean())call(layers,"enable",{17});
    if(((parameters).get("pointsUvs")).toBoolean())call(layers,"enable",{18});
    if(((parameters).get("decodeVideoTexture")).toBoolean())call(layers,"enable",{19});
    if(((parameters).get("decodeVideoTextureEmissive")).toBoolean())call(layers,"enable",{20});
    if(((parameters).get("alphaToCoverage")).toBoolean())call(layers,"enable",{21});
    if((([&]{const auto left=(parameters).get("numLightProbeGrids");const auto right=JSValue(0);return JSValue(left.toNumber()>right.toNumber());}())).toBoolean())call(layers,"enable",{22});
    if(((parameters).get("hasPositionAttribute")).toBoolean())call(layers,"enable",{23});
    pushProperty(array,layers,"mask");

}
JSValue getProgramCacheKey(const JSValue&,const Args& args,const Args& captures){
    const auto state=captures[0],parameters=argument(args,0),array=JSValue::array();
    if(parameters.get("shaderID").toBoolean())pushProperty(array,parameters,"shaderID");
    else{pushProperty(array,parameters,"customVertexShaderID");pushProperty(array,parameters,"customFragmentShaderID");}
    if(!parameters.get("defines").isUndefined()){
        const auto definitions=parameters.get("defines");
        for(const auto& key:enumerableNames(definitions)){
            if(definitions.isObject()&&!definitions.has(key))continue;
            call(array,"push",{key});
            const auto push=array.get("push");
            const auto value=parameters.get("defines").get(key);push.call(array,{value});
        }
    }
    if(parameters.get("isRawShaderMaterial").strictEquals(false)){
        cacheKeyParameters(array,parameters);cacheKeyBooleans(state,array,parameters);
        pushProperty(array,state.get("renderer"),"outputColorSpace");
    }
    pushProperty(array,parameters,"customProgramCacheKey");
    return call(array,"join");
}
JSValue getUniforms(const JSValue&,const Args& args,const Args& captures){
    const auto material=argument(args,0);
    const auto shaderIDs=captures[0].get("shaderIDs");const auto shaderID=shaderIDs.get(material.get("type"));
    if(shaderID.toBoolean()){
        const auto shader=nativeShaderLib().get(shaderID);
        return invoke(nativeUniformsUtils(),"clone",[&]{return Args{shader.get("uniforms")};});
    }
    return material.get("uniforms");
}
JSValue acquireProgram(const JSValue&,const Args& args,const Args& captures){
    const auto state=captures[0],parameters=argument(args,0),cacheKey=argument(args,1);
    const auto programsMap=state.get("programsMap");auto program=call(programsMap,"get",{cacheKey});
    if(!program.isUndefined())program.set("usedTimes",program.get("usedTimes").toNumber()+1);
    else{
        program=nativeWebGLProgramConstructor().construct({state.get("renderer"),cacheKey,parameters,state.get("bindingStates")});
        call(state.get("programs"),"push",{program});call(programsMap,"set",{cacheKey,program});
    }
    return program;
}
JSValue releaseProgram(const JSValue&,const Args& args,const Args& captures){
    const auto state=captures[0];auto program=argument(args,0);
    const auto count=program.get("usedTimes").toNumber()-1;program.set("usedTimes",count);
    if(count==0){
        auto programs=state.get("programs");const auto index=call(programs,"indexOf",{program});
        const auto key=index.toPropertyKey();
        const auto last=programs.get("length").toNumber()-1;
        const auto value=programs.get(JSValue(last));programs.set(key,value);
        call(programs,"pop");
        invoke(state.get("programsMap"),"delete",[&]{return Args{program.get("cacheKey")};});
        call(program,"destroy");
    }
    return {};
}
JSValue releaseShaderCache(const JSValue&,const Args& args,const Args& captures){
    call(captures[0].get("customShaders"),"remove",{argument(args,0)});return {};
}
JSValue dispose(const JSValue&,const Args&,const Args& captures){call(captures[0].get("customShaders"),"dispose");return {};}
JSValue construct(const JSValue&,const Args& args,const Args&){
    // Lexical state is not a source object: inherited setters cannot alter it.
    auto state=JSValue::object(nullptr);
    const auto layers=nativeLayersConstructor().construct();literal(state,"layers",layers);
    const auto customShaders=nativeWebGLShaderCacheConstructor().construct();literal(state,"customShaders",customShaders);
    const auto channels=nativeSetConstructor().construct();literal(state,"activeChannels",channels);
    const auto programs=JSValue::array();literal(state,"programs",programs);
    const auto programsMap=nativeMapConstructor().construct();literal(state,"programsMap",programsMap);
    const char* names[]{"renderer","environments","extensions","capabilities","bindingStates","clipping"};
    for(std::size_t i=0;i<6;++i)literal(state,names[i],argument(args,i));
    literal(state,"logarithmicDepthBuffer",argument(args,3).get("logarithmicDepthBuffer"));
    auto shaderIDs=JSValue::object();
    for(const auto& pair:std::initializer_list<std::pair<const char*,const char*>>{
        {"MeshDepthMaterial","depth"},{"MeshDistanceMaterial","distance"},{"MeshNormalMaterial","normal"},{"MeshBasicMaterial","basic"},
        {"MeshLambertMaterial","lambert"},{"MeshPhongMaterial","phong"},{"MeshToonMaterial","toon"},{"MeshStandardMaterial","physical"},
        {"MeshPhysicalMaterial","physical"},{"MeshMatcapMaterial","matcap"},{"LineBasicMaterial","basic"},{"LineDashedMaterial","dashed"},
        {"PointsMaterial","points"},{"ShadowMaterial","shadow"},{"SpriteMaterial","sprite"}})literal(shaderIDs,pair.first,pair.second);
    literal(state,"shaderIDs",shaderIDs);
    auto result=JSValue::object();
    for(const auto& method:std::initializer_list<std::tuple<const char*,JSValue::NativeFunction,std::size_t>>{
        {"getParameters",getParameters,6},{"getProgramCacheKey",getProgramCacheKey,1},{"getUniforms",getUniforms,1},
        {"acquireProgram",acquireProgram,2},{"releaseProgram",releaseProgram,1},{"releaseShaderCache",releaseShaderCache,1}})
        literal(result,std::get<0>(method),JSValue::constructor(std::get<1>(method),{state},std::get<0>(method),std::get<2>(method),JSValue::ConstructorKind::Callable));
    literal(result,"programs",programs);
    literal(result,"dispose",JSValue::constructor(dispose,{state},"dispose",0,JSValue::ConstructorKind::Callable));
    return result;
}
}
JSValue nativeWebGLProgramsConstructor(){
    static thread_local JSWeakValue weak;
    if(const auto cached=weak.lock();cached&&cached->isCallable())return *cached;
    auto result=JSValue::constructor(construct,{},"WebGLPrograms",6,JSValue::ConstructorKind::Callable);
    result.retainForHeapLifetime();weak=JSWeakValue(result);return result;
}
WebGLPrograms::WebGLPrograms(const JSValue& renderer,const JSValue& environments,const JSValue& extensions,
    const JSValue& capabilities,const JSValue& bindingStates,const JSValue& clipping){
    validate({&renderer,&environments,&extensions,&capabilities,&bindingStates,&clipping});
    object_=nativeWebGLProgramsConstructor().call({}, {renderer,environments,extensions,capabilities,bindingStates,clipping});
}
JSValue WebGLPrograms::getParameters(const JSValue& material,const JSValue& lights,const JSValue& shadows,const JSValue& scene,const JSValue& object,const JSValue& lightProbeGrids){
    validate({&object_,&material,&lights,&shadows,&scene,&object,&lightProbeGrids});return call(object_,"getParameters",{material,lights,shadows,scene,object,lightProbeGrids});
}
JSValue WebGLPrograms::getProgramCacheKey(const JSValue& parameters){validate({&object_,&parameters});return call(object_,"getProgramCacheKey",{parameters});}
JSValue WebGLPrograms::getUniforms(const JSValue& material){validate({&object_,&material});return call(object_,"getUniforms",{material});}
JSValue WebGLPrograms::acquireProgram(const JSValue& parameters,const JSValue& key){validate({&object_,&parameters,&key});return call(object_,"acquireProgram",{parameters,key});}
JSValue WebGLPrograms::releaseProgram(const JSValue& program){validate({&object_,&program});return call(object_,"releaseProgram",{program});}
JSValue WebGLPrograms::releaseShaderCache(const JSValue& material){validate({&object_,&material});return call(object_,"releaseShaderCache",{material});}
JSValue WebGLPrograms::dispose(){validate({&object_});return call(object_,"dispose");}
