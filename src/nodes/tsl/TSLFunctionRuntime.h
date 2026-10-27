#pragma once
#include "core/JSValue.h"
#include "core/JSIterator.h"
#include "core/JSModuleObject.h"

namespace TSLFunctionRuntime {
using Args=std::vector<JSValue>;
inline JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
struct Invocation {JSValue receiver,method;JSValue operator()(const Args& args={})const{return method.call(receiver,args);}};
inline Invocation invoke(const JSValue& receiver,const char* key){return {receiver,receiver.get(key)};}
inline JSValue first(const JSValue& inputs) {
    auto iterator=getJSIterator(inputs);const auto value=stepJSIterator(iterator);
    if(!iterator.done)closeJSIterator(iterator);return value?*value:JSValue{};
}
inline void own(JSValue object,const char* name,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(name,d);}
inline JSValue shader(const JSValue& fn,JSValue::NativeFunction body,const Args& captures,const char* parameter,const char* inputType,const char* outputType) {
    auto layout=JSValue::object();own(layout,parameter,inputType);own(layout,"return",outputType);
    return fn.call({}, {JSValue::function(body,captures,"",1),layout});
}
}
