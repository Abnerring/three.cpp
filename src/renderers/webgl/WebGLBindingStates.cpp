// Direct port of three.js WebGLBindingStates.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLBindingStates.h"
#include "constants.h"
#include <unordered_set>

namespace {
using Args=std::vector<JSValue>;
JSValue argument(const Args& args,std::size_t i) { return i<args.size()?args[i]:JSValue{}; }
void literal(JSValue object,const char* key,JSValue value) {
    JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;
    object.defineProperty(key,d);
}
std::string key(const JSValue& value) { return value.toPrimitive(JSValue::PreferredType::String).toString(); }
JSValue get(const JSValue& object,const JSValue& name) { return object.get(key(name)); }
void set(JSValue object,const JSValue& name,const JSValue& value) { object.set(key(name),value); }
void erase(JSValue object,const JSValue& name) {
    if(!object.erase(key(name)))throw JSTypeError("Cannot delete binding state property");
}
JSValue add(const JSValue& left,const JSValue& right) {
    const auto a=left.toPrimitive(),b=right.toPrimitive();
    if(a.type()==JSValue::Type::String || b.type()==JSValue::Type::String)return a.toString()+b.toString();
    const auto x=a.toNumber();const auto y=b.toNumber();return x+y;
}
double multiply(const JSValue& left,const JSValue& right) {
    const auto x=left.toNumber();const auto y=right.toNumber();return x*y;
}
double divide(const JSValue& left,const JSValue& right) {
    const auto x=left.toNumber();const auto y=right.toNumber();return x/y;
}
// Preserve method lookup before argument evaluation, including a method getter
// that changes a later command constant or calls back into the manager.
template<class F> JSValue callLater(const JSValue& receiver,const char* name,F arguments) {
    const auto method=receiver.get(name);const auto args=arguments();return method.call(receiver,args);
}
JSValue call(const JSValue& receiver,const char* name,Args args={}) {
    return receiver.get(name).call(receiver,args);
}
// Match the pinned Node oracle for ordinary objects: collect the initial chain's
// enumerable keys, including non-enumerable shadowing, then check whether each
// key still exists on the live chain. Getter changes to enumerability do not
// remove an initial candidate; new keys are considered by the next enumeration.
// Proxy, primitive wrapper prototypes and Symbol remain runtime boundaries.
template<class F> bool forIn(JSValue object,F visit) {
    if(object.isNull() || object.isUndefined())return false;
    if(!object.isObject()) {
        if(object.type()==JSValue::Type::String) {
            // Runtime string indexing/length already follows UTF-16 code units.
            const auto length=object.get("length").toNumber();
            for(double i=0;i<length;++i)if(visit(JSValue(i).toString()))return true;
        }
        return false;
    }
    std::unordered_set<std::string> visited;
    std::vector<std::string> candidates;
    auto level=object;
    while(!level.isNull()) {
        const auto names=level.ownPropertyNames();
        for(const auto& name:names) {
            if(visited.contains(name))continue;
            const auto d=level.getOwnPropertyDescriptor(name);
            if(!d)continue;
            visited.insert(name);
            if(d->enumerable.value_or(false))candidates.push_back(name);
        }
        level=level.getPrototype();
    }
    for(const auto& name:candidates)if(object.has(name) && visit(name))return true;
    return false;
}
JSValue current(const JSValue& closure) { return closure.get("currentState"); }
JSValue createState(const JSValue& max,const JSValue& vao) {
    auto fresh=JSValue::array(),enabled=JSValue::array(),divisors=JSValue::array();
    for(double i=0;i<max.toNumber();++i) {
        const auto name=JSValue(i).toString();fresh.set(name,0);enabled.set(name,0);divisors.set(name,0);
    }
    auto state=JSValue::object();
    literal(state,"geometry",nullptr);literal(state,"program",nullptr);literal(state,"wireframe",false);
    literal(state,"newAttributes",fresh);literal(state,"enabledAttributes",enabled);
    literal(state,"attributeDivisors",divisors);literal(state,"object",vao);
    literal(state,"attributes",JSValue::object());literal(state,"index",nullptr);return state;
}
JSValue bindingState(const JSValue& closure,const JSValue& object,const JSValue& geometry,
                     const JSValue& program,const JSValue& material) {
    const bool wireframe=material.get("wireframe").strictEquals(true);
    const auto states=closure.get("bindingStates");
    auto objectMap=get(states,geometry.get("id"));
    if(objectMap.isUndefined()) { objectMap=JSValue::object();set(states,geometry.get("id"),objectMap); }
    const auto objectId=object.get("isInstancedMesh").strictEquals(true)?object.get("id"):JSValue(0);
    auto programMap=get(objectMap,objectId);
    if(programMap.isUndefined()) { programMap=JSValue::object();set(objectMap,objectId,programMap); }
    auto stateMap=get(programMap,program.get("id"));
    if(stateMap.isUndefined()) { stateMap=JSValue::object();set(programMap,program.get("id"),stateMap); }
    auto state=get(stateMap,wireframe);
    if(state.isUndefined()) {
        const auto vao=call(closure.get("gl"),"createVertexArray");
        state=createState(closure.get("maxVertexAttributes"),vao);set(stateMap,wireframe,state);
    }
    return state;
}
JSValue geometryAttribute(const JSValue& attributes,const std::string& name,const JSValue& object) {
    auto attribute=attributes.get(name);
    if(attribute.isUndefined()) {
        if(name=="instanceMatrix" && object.get("instanceMatrix").toBoolean())attribute=object.get("instanceMatrix");
        if(name=="instanceColor" && object.get("instanceColor").toBoolean())attribute=object.get("instanceColor");
    }
    return attribute;
}
bool needsUpdate(const JSValue& closure,const JSValue& object,const JSValue& geometry,
                 const JSValue& program,const JSValue& index) {
    const auto cached=current(closure).get("attributes"),attributes=geometry.get("attributes");
    double count=0;
    const auto programAttributes=call(program,"getAttributes");
    if(forIn(programAttributes,[&](const std::string& name) {
        const auto programAttribute=programAttributes.get(name);
        if(programAttribute.get("location").toNumber()>=0) {
            const auto previous=cached.get(name),attribute=geometryAttribute(attributes,name,object);
            if(previous.isUndefined())return true;
            if(!previous.get("attribute").strictEquals(attribute))return true;
            if(attribute.toBoolean()) {
                const auto oldData=previous.get("data"),newData=attribute.get("data");
                if(!oldData.strictEquals(newData))return true;
            }
            ++count;
        }
        return false;
    }))return true;
    if(!current(closure).get("attributesNum").strictEquals(count))return true;
    if(!current(closure).get("index").strictEquals(index))return true;
    return false;
}
void saveCache(const JSValue& closure,const JSValue& object,const JSValue& geometry,
               const JSValue& program,const JSValue& index) {
    auto cache=JSValue::object();const auto attributes=geometry.get("attributes");double count=0;
    const auto programAttributes=call(program,"getAttributes");
    forIn(programAttributes,[&](const std::string& name) {
        const auto programAttribute=programAttributes.get(name);
        if(programAttribute.get("location").toNumber()>=0) {
            const auto attribute=geometryAttribute(attributes,name,object);
            auto data=JSValue::object();data.set("attribute",attribute);
            if(attribute.toBoolean() && attribute.get("data").toBoolean())data.set("data",attribute.get("data"));
            cache.set(name,data);++count;
        }
        return false;
    });
    current(closure).set("attributes",cache);current(closure).set("attributesNum",count);
    current(closure).set("index",index);
}
void init(const JSValue& closure) {
    auto fresh=current(closure).get("newAttributes");const auto length=fresh.get("length");
    for(double i=0;i<length.toNumber();++i)fresh.set(JSValue(i).toString(),0);
}
void enable(const JSValue& closure,const JSValue& attribute,const JSValue& divisor) {
    auto fresh=current(closure).get("newAttributes"),enabled=current(closure).get("enabledAttributes"),
         divisors=current(closure).get("attributeDivisors");
    set(fresh,attribute,1);
    if(get(enabled,attribute).strictEquals(0)) {
        call(closure.get("gl"),"enableVertexAttribArray",{attribute});set(enabled,attribute,1);
    }
    if(!get(divisors,attribute).strictEquals(divisor)) {
        call(closure.get("gl"),"vertexAttribDivisor",{attribute,divisor});set(divisors,attribute,divisor);
    }
}
void disable(const JSValue& closure) {
    const auto fresh=current(closure).get("newAttributes");auto enabled=current(closure).get("enabledAttributes");
    const auto length=enabled.get("length");
    for(double i=0;i<length.toNumber();++i) {
        const auto name=JSValue(i).toString();const auto old=enabled.get(name),next=fresh.get(name);
        if(!old.strictEquals(next)) {
            call(closure.get("gl"),"disableVertexAttribArray",{i});enabled.set(name,0);
        }
    }
}
void pointer(const JSValue& closure,JSValue index,JSValue size,JSValue type,JSValue normalized,
             JSValue stride,JSValue offset,bool integer) {
    const auto gl=closure.get("gl");
    if(integer)call(gl,"vertexAttribIPointer",{index,size,type,stride,offset});
    else call(gl,"vertexAttribPointer",{index,size,type,normalized,stride,offset});
}
void setupAttributes(const JSValue& closure,const JSValue& object,const JSValue& material,
                     const JSValue& program,const JSValue& geometry) {
    init(closure);
    const auto geometryAttributes=geometry.get("attributes"),programAttributes=call(program,"getAttributes"),
               defaults=material.get("defaultAttributeValues");
    forIn(programAttributes,[&](const std::string& name) {
        const auto programAttribute=programAttributes.get(name);
        if(!(programAttribute.get("location").toNumber()>=0))return false;
        const auto attribute=geometryAttribute(geometryAttributes,name,object);
        if(!attribute.isUndefined()) {
            const auto normalized=attribute.get("normalized"),size=attribute.get("itemSize");
            const auto cached=call(closure.get("attributes"),"get",{attribute});
            if(cached.isUndefined())return false;
            const auto buffer=cached.get("buffer"),type=cached.get("type"),bytes=cached.get("bytesPerElement"),
                       gl=closure.get("gl");
            bool integer=type.strictEquals(gl.get("INT"));
            if(!integer)integer=type.strictEquals(gl.get("UNSIGNED_INT"));
            if(!integer)integer=attribute.get("gpuType").strictEquals(IntType);
            if(attribute.get("isInterleavedBufferAttribute").toBoolean()) {
                const auto data=attribute.get("data"),stride=data.get("stride"),offset=attribute.get("offset");
                if(data.get("isInstancedInterleavedBuffer").toBoolean()) {
                    for(double i=0;i<programAttribute.get("locationSize").toNumber();++i) {
                        const auto location=add(programAttribute.get("location"),i),divisor=data.get("meshPerAttribute");
                        enable(closure,location,divisor);
                    }
                    if(!object.get("isInstancedMesh").strictEquals(true) && geometry.get("_maxInstanceCount").isUndefined()) {
                        const auto divisor=data.get("meshPerAttribute"),count=data.get("count");
                        auto target=geometry;target.set("_maxInstanceCount",multiply(divisor,count));
                    }
                } else {
                    for(double i=0;i<programAttribute.get("locationSize").toNumber();++i)
                        enable(closure,add(programAttribute.get("location"),i),0);
                }
                callLater(gl,"bindBuffer",[&]{return Args{gl.get("ARRAY_BUFFER"),buffer};});
                for(double i=0;i<programAttribute.get("locationSize").toNumber();++i) {
                    const auto location=add(programAttribute.get("location"),i);
                    const auto componentSize=divide(size,programAttribute.get("locationSize"));
                    const auto byteStride=multiply(stride,bytes);
                    const auto componentOffset=multiply(divide(size,programAttribute.get("locationSize")),i);
                    const auto byteOffset=multiply(add(offset,componentOffset),bytes);
                    pointer(closure,location,componentSize,type,normalized,byteStride,byteOffset,integer);
                }
            } else {
                if(attribute.get("isInstancedBufferAttribute").toBoolean()) {
                    for(double i=0;i<programAttribute.get("locationSize").toNumber();++i) {
                        const auto location=add(programAttribute.get("location"),i),divisor=attribute.get("meshPerAttribute");
                        enable(closure,location,divisor);
                    }
                    if(!object.get("isInstancedMesh").strictEquals(true) && geometry.get("_maxInstanceCount").isUndefined()) {
                        const auto divisor=attribute.get("meshPerAttribute"),count=attribute.get("count");
                        auto target=geometry;target.set("_maxInstanceCount",multiply(divisor,count));
                    }
                } else {
                    for(double i=0;i<programAttribute.get("locationSize").toNumber();++i)
                        enable(closure,add(programAttribute.get("location"),i),0);
                }
                callLater(gl,"bindBuffer",[&]{return Args{gl.get("ARRAY_BUFFER"),buffer};});
                for(double i=0;i<programAttribute.get("locationSize").toNumber();++i) {
                    const auto location=add(programAttribute.get("location"),i);
                    const auto componentSize=divide(size,programAttribute.get("locationSize"));
                    const auto byteStride=multiply(size,bytes);
                    const auto offset=multiply(divide(size,programAttribute.get("locationSize")),i);
                    const auto byteOffset=multiply(offset,bytes);
                    pointer(closure,location,componentSize,type,normalized,byteStride,byteOffset,integer);
                }
            }
        } else if(!defaults.isUndefined()) {
            const auto value=defaults.get(name);
            if(!value.isUndefined()) {
                const auto length=value.get("length");
                const char* method=length.strictEquals(2)?"vertexAttrib2fv":length.strictEquals(3)?"vertexAttrib3fv":
                                   length.strictEquals(4)?"vertexAttrib4fv":"vertexAttrib1fv";
                callLater(closure.get("gl"),method,[&]{return Args{programAttribute.get("location"),value};});
            }
        }
        return false;
    });
    disable(closure);
}
void resetDefault(const JSValue& closure) {
    auto state=closure.get("defaultState");state.set("geometry",nullptr);state.set("program",nullptr);state.set("wireframe",false);
}
void reset(const JSValue& closure) {
    resetDefault(closure);auto frame=closure;frame.set("forceUpdate",true);
    if(current(closure).strictEquals(closure.get("defaultState")))return;
    frame.set("currentState",closure.get("defaultState"));
    call(closure.get("gl"),"bindVertexArray",{current(closure).get("object")});
}
void deleteStateMap(const JSValue& closure,JSValue stateMap) {
    forIn(stateMap,[&](const std::string& wireframe) {
        call(closure.get("gl"),"deleteVertexArray",{stateMap.get(wireframe).get("object")});
        erase(stateMap,wireframe);return false;
    });
}
void deleteProgramMap(const JSValue& closure,JSValue programMap) {
    forIn(programMap,[&](const std::string& programId) {
        deleteStateMap(closure,programMap.get(programId));erase(programMap,programId);return false;
    });
}
JSValue method(const JSValue&,const Args& args,const Args& captures) {
    const auto closure=captures[0];const auto name=captures[1].string();
    if(name=="setup") {
        const auto object=argument(args,0),material=argument(args,1),program=argument(args,2),geometry=argument(args,3),index=argument(args,4);
        const auto state=bindingState(closure,object,geometry,program,material);
        if(!current(closure).strictEquals(state)) {
            auto frame=closure;frame.set("currentState",state);
            call(closure.get("gl"),"bindVertexArray",{current(closure).get("object")});
        }
        const bool update=needsUpdate(closure,object,geometry,program,index);
        if(update)saveCache(closure,object,geometry,program,index);
        if(!index.isNull())callLater(closure.get("attributes"),"update",[&]{return Args{index,closure.get("gl").get("ELEMENT_ARRAY_BUFFER")};});
        if(update || closure.get("forceUpdate").toBoolean()) {
            auto frame=closure;frame.set("forceUpdate",false);
            setupAttributes(closure,object,material,program,geometry);
            if(!index.isNull())callLater(closure.get("gl"),"bindBuffer",[&] {
                const auto target=closure.get("gl").get("ELEMENT_ARRAY_BUFFER");
                const auto cached=call(closure.get("attributes"),"get",{index});
                return Args{target,cached.get("buffer")};
            });
        }
    } else if(name=="reset")reset(closure);
    else if(name=="resetDefaultState")resetDefault(closure);
    else if(name=="initAttributes")init(closure);
    else if(name=="enableAttribute")enable(closure,argument(args,0),0);
    else if(name=="disableUnusedAttributes")disable(closure);
    else if(name=="dispose") {
        reset(closure);const auto states=closure.get("bindingStates");
        forIn(states,[&](const std::string& geometryId) {
            const auto objectMap=states.get(geometryId);
            forIn(objectMap,[&](const std::string& objectId) {deleteProgramMap(closure,objectMap.get(objectId));return false;});
            erase(states,geometryId);return false;
        });
    } else if(name=="releaseStatesOfGeometry") {
        const auto geometry=argument(args,0),states=closure.get("bindingStates");
        if(get(states,geometry.get("id")).isUndefined())return {};
        const auto objectMap=get(states,geometry.get("id"));
        forIn(objectMap,[&](const std::string& objectId) {deleteProgramMap(closure,objectMap.get(objectId));return false;});
        erase(states,geometry.get("id"));
    } else if(name=="releaseStatesOfProgram") {
        const auto program=argument(args,0),states=closure.get("bindingStates");
        forIn(states,[&](const std::string& geometryId) {
            const auto objectMap=states.get(geometryId);
            forIn(objectMap,[&](const std::string& objectId) {
                const auto programMap=objectMap.get(objectId);
                if(get(programMap,program.get("id")).isUndefined())return false;
                deleteStateMap(closure,get(programMap,program.get("id")));erase(programMap,program.get("id"));return false;
            });return false;
        });
    } else if(name=="releaseStatesOfObject") {
        const auto object=argument(args,0),states=closure.get("bindingStates");
        forIn(states,[&](const std::string& geometryId) {
            const auto objectMap=states.get(geometryId);
            const auto objectId=object.get("isInstancedMesh").strictEquals(true)?object.get("id"):JSValue(0);
            const auto programMap=get(objectMap,objectId);
            if(programMap.isUndefined())return false;
            deleteProgramMap(closure,programMap);erase(objectMap,objectId);
            if(objectMap.keys().empty())erase(states,geometryId);return false;
        });
    }
    return {};
}
}
WebGLBindingStates::WebGLBindingStates(const JSValue& gl,const JSValue& attributes) {
    static_cast<void>(JSWeakValue(gl));static_cast<void>(JSWeakValue(attributes));object_=JSValue::object();
    const auto max=callLater(gl,"getParameter",[&]{return Args{gl.get("MAX_VERTEX_ATTRIBS")};});
    auto closure=JSValue::object(nullptr);literal(closure,"gl",gl);literal(closure,"attributes",attributes);
    literal(closure,"maxVertexAttributes",max);literal(closure,"bindingStates",JSValue::object());
    const auto state=createState(max,nullptr);literal(closure,"defaultState",state);
    literal(closure,"currentState",state);literal(closure,"forceUpdate",false);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
        {"setup",5},{"reset",0},{"resetDefaultState",0},{"dispose",0},{"releaseStatesOfGeometry",1},
        {"releaseStatesOfObject",1},{"releaseStatesOfProgram",1},{"initAttributes",0},{"enableAttribute",1},{"disableUnusedAttributes",0}})
        literal(object_,name,JSValue::constructor(method,{closure,name},name,arity,JSValue::ConstructorKind::Callable));
}
JSValue WebGLBindingStates::setup(const JSValue& object,const JSValue& material,const JSValue& program,const JSValue& geometry,const JSValue& index) {
    for(const auto* value:std::initializer_list<const JSValue*>{&object_,&object,&material,&program,&geometry,&index})static_cast<void>(JSWeakValue(*value));
    return object_.get("setup").call(object_,{object,material,program,geometry,index});
}
#define THREE_BINDING_NOARG(name) JSValue WebGLBindingStates::name(){static_cast<void>(JSWeakValue(object_));return object_.get(#name).call(object_);}
THREE_BINDING_NOARG(reset)
THREE_BINDING_NOARG(resetDefaultState)
THREE_BINDING_NOARG(dispose)
THREE_BINDING_NOARG(initAttributes)
THREE_BINDING_NOARG(disableUnusedAttributes)
#undef THREE_BINDING_NOARG
#define THREE_BINDING_ONEARG(name) JSValue WebGLBindingStates::name(const JSValue& value){static_cast<void>(JSWeakValue(object_));static_cast<void>(JSWeakValue(value));return object_.get(#name).call(object_,{value});}
THREE_BINDING_ONEARG(releaseStatesOfGeometry)
THREE_BINDING_ONEARG(releaseStatesOfObject)
THREE_BINDING_ONEARG(releaseStatesOfProgram)
THREE_BINDING_ONEARG(enableAttribute)
#undef THREE_BINDING_ONEARG
