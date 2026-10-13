// Ported from three.js WebGLCapabilities.js; see THREEJS-LICENSE.
#include "renderers/webgl/WebGLCapabilities.h"
#include "constants.h"
#include "utils.h"

namespace {
using Args=std::vector<JSValue>;
JSValue argument(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue();}
void validate(std::initializer_list<const JSValue*> values){for(const auto* value:values)static_cast<void>(JSWeakValue(*value));}
void literal(JSValue object,const char* key,JSValue value){
    JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
JSValue call(const JSValue& object,const char* method,const Args& args={}){const auto fn=object.get(method);return fn.call(object,args);}
JSValue getMaxAnisotropy(const JSValue&,const Args&,const Args& captures){
    auto state=captures[0];const auto gl=captures[1],extensions=captures[2];
    if(!state.get("maxAnisotropy").isUndefined())return state.get("maxAnisotropy");
    if(call(extensions,"has",{"EXT_texture_filter_anisotropic"}).strictEquals(true)){
        const auto extension=call(extensions,"get",{"EXT_texture_filter_anisotropic"});
        const auto get=gl.get("getParameter");
        const auto name=extension.get("MAX_TEXTURE_MAX_ANISOTROPY_EXT");
        state.set("maxAnisotropy",get.call(gl,{name}));
    }else state.set("maxAnisotropy",0);
    return state.get("maxAnisotropy");
}
JSValue textureFormatReadable(const JSValue&,const Args& args,const Args& captures){
    const auto format=argument(args,0),gl=captures[1],utils=captures[3];
    if(!format.strictEquals(RGBAFormat)){
        const auto converted=call(utils,"convert",{format});
        const auto get=gl.get("getParameter");const auto name=gl.get("IMPLEMENTATION_COLOR_READ_FORMAT");
        if(!converted.strictEquals(get.call(gl,{name})))return false;
    }
    return true;
}
JSValue textureTypeReadable(const JSValue&,const Args& args,const Args& captures){
    const auto type=argument(args,0),gl=captures[1],extensions=captures[2],utils=captures[3];
    JSValue supported=type.strictEquals(HalfFloatType);
    if(supported.toBoolean()){
        supported=call(extensions,"has",{"EXT_color_buffer_half_float"});
        if(!supported.toBoolean())supported=call(extensions,"has",{"EXT_color_buffer_float"});
    }
    if(!type.strictEquals(UnsignedByteType)&&!type.strictEquals(FloatType)&&!supported.toBoolean()){
        const auto converted=call(utils,"convert",{type});
        const auto get=gl.get("getParameter");const auto name=gl.get("IMPLEMENTATION_COLOR_READ_TYPE");
        if(!converted.strictEquals(get.call(gl,{name})))return false;
    }
    return true;
}
bool shaderPrecision(const JSValue& gl,const char* shader,const char* kind){
    const auto get=gl.get("getShaderPrecisionFormat");
    const auto shaderValue=gl.get(shader);const auto precisionValue=gl.get(kind);
    const auto result=get.call(gl,{shaderValue,precisionValue});
    return result.get("precision").toNumber()>0;
}
JSValue getMaxPrecision(const JSValue&,const Args& args,const Args& captures){
    auto precision=argument(args,0);const auto gl=captures[1];
    if(precision.strictEquals("highp")){
        if(shaderPrecision(gl,"VERTEX_SHADER","HIGH_FLOAT")&&shaderPrecision(gl,"FRAGMENT_SHADER","HIGH_FLOAT"))return "highp";
        precision="mediump";
    }
    if(precision.strictEquals("mediump")){
        if(shaderPrecision(gl,"VERTEX_SHADER","MEDIUM_FLOAT")&&shaderPrecision(gl,"FRAGMENT_SHADER","MEDIUM_FLOAT"))return "mediump";
    }
    return "lowp";
}
JSValue construct(const JSValue&,const Args& args,const Args&){
    const auto gl=argument(args,0),extensions=argument(args,1),parameters=argument(args,2),utils=argument(args,3);
    auto state=JSValue::object(nullptr);literal(state,"maxAnisotropy",JSValue());
    // All four source closures share the captured invocation environment.
    // Construction-only parameters are deliberately excluded.
    const Args environment{state,gl,extensions,utils};
    auto precision=parameters.get("precision").isUndefined()?JSValue("highp"):parameters.get("precision");
    const auto maxPrecision=getMaxPrecision({}, {precision}, environment);
    if(!maxPrecision.strictEquals(precision)){
        ThreeUtils::warn({"WebGLRenderer:",precision,"not supported, using",maxPrecision,"instead."});
        precision=maxPrecision;
    }
    const auto logarithmic=parameters.get("logarithmicDepthBuffer").strictEquals(true);
    JSValue reversed=parameters.get("reversedDepthBuffer").strictEquals(true);
    if(reversed.toBoolean())reversed=call(extensions,"has",{"EXT_clip_control"});
    if(parameters.get("reversedDepthBuffer").strictEquals(true)&&reversed.strictEquals(false))
        ThreeUtils::warn({"WebGLRenderer: Unable to use reversed depth buffer due to missing EXT_clip_control extension. Fallback to default depth buffer."});
    Args values;
    for(const auto* name:{"MAX_TEXTURE_IMAGE_UNITS","MAX_VERTEX_TEXTURE_IMAGE_UNITS","MAX_TEXTURE_SIZE","MAX_CUBE_MAP_TEXTURE_SIZE",
        "MAX_VERTEX_ATTRIBS","MAX_VERTEX_UNIFORM_VECTORS","MAX_VARYING_VECTORS","MAX_FRAGMENT_UNIFORM_VECTORS","MAX_SAMPLES","SAMPLES"}){
        const auto get=gl.get("getParameter");const auto parameter=gl.get(name);values.push_back(get.call(gl,{parameter}));
    }
    auto result=JSValue::object();literal(result,"isWebGL2",true);
    literal(result,"getMaxAnisotropy",JSValue::constructor(getMaxAnisotropy,environment,"getMaxAnisotropy",0,JSValue::ConstructorKind::Callable));
    literal(result,"getMaxPrecision",JSValue::constructor(getMaxPrecision,environment,"getMaxPrecision",1,JSValue::ConstructorKind::Callable));
    literal(result,"textureFormatReadable",JSValue::constructor(textureFormatReadable,environment,"textureFormatReadable",1,JSValue::ConstructorKind::Callable));
    literal(result,"textureTypeReadable",JSValue::constructor(textureTypeReadable,environment,"textureTypeReadable",1,JSValue::ConstructorKind::Callable));
    literal(result,"precision",precision);literal(result,"logarithmicDepthBuffer",logarithmic);literal(result,"reversedDepthBuffer",reversed);
    unsigned index=0;
    for(const auto* name:{"maxTextures","maxVertexTextures","maxTextureSize","maxCubemapSize","maxAttributes","maxVertexUniforms","maxVaryings","maxFragmentUniforms","maxSamples","samples"})literal(result,name,values[index++]);
    return result;
}
}
JSValue nativeWebGLCapabilitiesConstructor(){
    static thread_local JSWeakValue weak;
    if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    auto value=JSValue::constructor(construct,{},"WebGLCapabilities",4,JSValue::ConstructorKind::Callable);
    value.retainForHeapLifetime();weak=JSWeakValue(value);return value;
}
WebGLCapabilities::WebGLCapabilities(const JSValue& gl,const JSValue& extensions,const JSValue& parameters,const JSValue& utils){
    validate({&gl,&extensions,&parameters,&utils});object_=nativeWebGLCapabilitiesConstructor().call({}, {gl,extensions,parameters,utils});
}
JSValue WebGLCapabilities::getMaxAnisotropy(){validate({&object_});return object_.get("getMaxAnisotropy").call(object_);}
JSValue WebGLCapabilities::getMaxPrecision(const JSValue& value){validate({&object_,&value});return object_.get("getMaxPrecision").call(object_,{value});}
JSValue WebGLCapabilities::textureFormatReadable(const JSValue& value){validate({&object_,&value});return object_.get("textureFormatReadable").call(object_,{value});}
JSValue WebGLCapabilities::textureTypeReadable(const JSValue& value){validate({&object_,&value});return object_.get("textureTypeReadable").call(object_,{value});}
void WebGLCapabilities::setConsoleFunction(const JSValue& callback){ThreeUtils::setConsoleFunction(callback);}
JSValue WebGLCapabilities::getConsoleFunction(){return ThreeUtils::getConsoleFunction();}
