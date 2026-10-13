// Ported from three.js WebGLExtensions.js; see THREEJS-LICENSE.
#include "renderers/webgl/WebGLExtensions.h"
#include "utils.h"

namespace {
using Args=std::vector<JSValue>;
JSValue argument(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue();}
void validate(std::initializer_list<const JSValue*> values){for(const auto* value:values)static_cast<void>(JSWeakValue(*value));}
void literal(JSValue object,const char* key,JSValue value){
    JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
JSValue getExtension(JSValue cache,const JSValue& gl,const JSValue& name){
    if(!cache.get(name).isUndefined())return cache.get(name);
    const auto function=gl.get("getExtension");
    const auto extension=function.call(gl,{name});
    cache.set(name,extension);
    return extension;
}
JSValue has(const JSValue&,const Args& args,const Args& captures){
    return !getExtension(captures[0],captures[1],argument(args,0)).isNull();
}
JSValue init(const JSValue&,const Args&,const Args& captures){
    for(const auto* name:{"EXT_color_buffer_float","WEBGL_clip_cull_distance","OES_texture_float_linear",
        "EXT_color_buffer_half_float","WEBGL_multisampled_render_to_texture","WEBGL_render_shared_exponent"})
        getExtension(captures[0],captures[1],name);
    return {};
}
JSValue get(const JSValue&,const Args& args,const Args& captures){
    const auto name=argument(args,0);
    const auto extension=getExtension(captures[0],captures[1],name);
    if(extension.isNull()){
        const auto primitive=name.toPrimitive();
        ThreeUtils::warnOnce({"WebGLRenderer: "+primitive.toString()+" extension not supported."});
    }
    return extension;
}
JSValue construct(const JSValue&,const Args& args,const Args&){
    auto cache=JSValue::object(),result=JSValue::object();const auto gl=argument(args,0);
    literal(result,"has",JSValue::constructor(has,{cache,gl},"has",1,JSValue::ConstructorKind::Callable));
    literal(result,"init",JSValue::constructor(init,{cache,gl},"init",0,JSValue::ConstructorKind::Callable));
    literal(result,"get",JSValue::constructor(get,{cache,gl},"get",1,JSValue::ConstructorKind::Callable));
    return result;
}
}
JSValue nativeWebGLExtensionsConstructor(){
    static thread_local JSWeakValue weak;
    if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;
    auto value=JSValue::constructor(construct,{},"WebGLExtensions",1,JSValue::ConstructorKind::Callable);
    value.retainForHeapLifetime();weak=JSWeakValue(value);return value;
}
WebGLExtensions::WebGLExtensions(const JSValue& gl){validate({&gl});object_=nativeWebGLExtensionsConstructor().call({}, {gl});}
JSValue WebGLExtensions::has(const JSValue& name){validate({&object_,&name});return object_.get("has").call(object_,{name});}
JSValue WebGLExtensions::init(){validate({&object_});return object_.get("init").call(object_);}
JSValue WebGLExtensions::get(const JSValue& name){validate({&object_,&name});return object_.get("get").call(object_,{name});}
void WebGLExtensions::setConsoleFunction(const JSValue& callback){ThreeUtils::setConsoleFunction(callback);}
JSValue WebGLExtensions::getConsoleFunction(){return ThreeUtils::getConsoleFunction();}
