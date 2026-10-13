#pragma once
#include "renderers/webgl/WebGLCapabilities.h"
#include <array>
namespace CapabilitiesRetention {
using Args=std::vector<JSValue>;
inline const std::array<const char*,4> methods{"getMaxAnisotropy","getMaxPrecision","textureFormatReadable","textureTypeReadable"};
inline const std::array<const char*,5> names{"gl","extensions","utils","parameters","cached"};
struct Kept {JSValue keep;std::array<JSWeakValue,5> refs;};
inline Kept prepare(unsigned method,bool instance){
 auto gl=JSValue::object(),extensions=JSValue::object(),utils=JSValue::object(),parameters=JSValue::object();
 gl.set("getParameter",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{if(a[0].strictEquals("anisotropy")){auto r=JSValue::object();r.set("cached",true);return r;}return 1;}));
 extensions.set("has",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return true;}));
 extensions.set("get",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{auto r=JSValue::object();r.set("MAX_TEXTURE_MAX_ANISOTROPY_EXT","anisotropy");return r;}));
 utils.set("convert",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return a[0];}));
 parameters.set("precision","lowp");auto cap=WebGLCapabilities(gl,extensions,parameters,utils);auto cached=cap.getMaxAnisotropy();
 auto fn=cap.object().get(methods[method]);JSValue keep=fn;
 if(instance){auto empty=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Empty",0,JSValue::ConstructorKind::Callable);keep=empty.construct({},fn);}
 return {keep,{JSWeakValue(gl),JSWeakValue(extensions),JSWeakValue(utils),JSWeakValue(parameters),JSWeakValue(cached)}};
}
inline JsonValue alive(const Kept& value){JsonValue::Object result;for(unsigned i=0;i<names.size();++i)result[names[i]]=!value.refs[i].expired();return result;}
}
