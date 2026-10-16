#pragma once
#include "core/JSMember.h"
#include <utility>

// Source program selection/cache logic over the injected native GPU protocol.
// GPU shader creation is delegated to the real WebGLProgram implementation.
JSValue nativeWebGLProgramsConstructor();
class WebGLPrograms {
    JSValue object_;
    struct Existing {};
    static JSValue copy(const JSValue& value){static_cast<void>(JSWeakValue(value));return value;}
    static JSValue take(JSValue& value){static_cast<void>(JSWeakValue(value));return std::move(value);}
    WebGLPrograms(const JSValue& value,Existing):object_(copy(value)){}
  public:
    explicit WebGLPrograms(const JSValue& renderer={},const JSValue& environments={},const JSValue& extensions={},
        const JSValue& capabilities={},const JSValue& bindingStates={},const JSValue& clipping={});
    WebGLPrograms(const WebGLPrograms& other):object_(copy(other.object_)){}
    WebGLPrograms(WebGLPrograms&& other):object_(take(other.object_)){}
    WebGLPrograms& operator=(const WebGLPrograms& other){static_cast<void>(JSWeakValue(object_));object_=copy(other.object_);return *this;}
    WebGLPrograms& operator=(WebGLPrograms&& other){static_cast<void>(JSWeakValue(object_));if(this!=&other)object_=take(other.object_);return *this;}
    static WebGLPrograms fromObject(const JSValue& object){return {object,Existing{}};}
    JSValue object()const{return copy(object_);}
    operator JSValue()const{return object();}
    JSMember programs{&object_,"programs"};
    JSValue getParameters(const JSValue& material={},const JSValue& lights={},const JSValue& shadows={},const JSValue& scene={},const JSValue& object={},const JSValue& lightProbeGrids={});
    JSValue getProgramCacheKey(const JSValue& parameters={});
    JSValue getUniforms(const JSValue& material={});
    JSValue acquireProgram(const JSValue& parameters={},const JSValue& cacheKey={});
    JSValue releaseProgram(const JSValue& program={});
    JSValue releaseShaderCache(const JSValue& material={});
    JSValue dispose();
};
