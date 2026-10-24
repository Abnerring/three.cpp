#include "nodes/core/NodeUtils.h"
#include "nodes/core/StackTrace.h"
#include "core/JSMap.h"
#include "core/JSModuleObject.h"
#include "core/JSRegExp.h"
#include "core/JSStringEncoding.h"
#include "core/JSWeakMap.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "math/Color.h"
#include "math/Matrix2.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "constants.h"
#include "utils.h"
#include <array>

#include <cmath>
#include <cstring>
#include <limits>

namespace {
// JavaScript's number to int32 conversion for the values that reach the hash: truncation
// toward zero, then wrap-around into the int32 range.
std::int32_t toInt32(double value) {
    if (value != value || value == std::numeric_limits<double>::infinity() ||
        value == -std::numeric_limits<double>::infinity())
        return 0;
    const double truncated = value < 0 ? -std::floor(-value) : std::floor(value);
    const double wrapped = std::fmod(truncated, 4294967296.0);
    const double positive = wrapped < 0 ? wrapped + 4294967296.0 : wrapped;
    return positive >= 2147483648.0 ? static_cast<std::int32_t>(positive - 4294967296.0)
                                    : static_cast<std::int32_t>(positive);
}

double cyrb53Array(std::span<const double> values) {
    std::uint32_t h1 = 0xdeadbeefu;
    std::uint32_t h2 = 0x41c6ce57u;
    for (const double value : values) {
        const auto v = static_cast<std::uint32_t>(toInt32(value));
        h1 = static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h1 ^ v), 2654435761));
        h2 = static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h2 ^ v), 1597334677));
    }
    h1 = static_cast<std::uint32_t>(
        nodeImul(static_cast<std::int32_t>(h1 ^ (h1 >> 16)), 2246822507));
    h1 ^= static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h2 ^ (h2 >> 13)), 3266489909));
    h2 = static_cast<std::uint32_t>(
        nodeImul(static_cast<std::int32_t>(h2 ^ (h2 >> 16)), 2246822507));
    h2 ^= static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h1 ^ (h1 >> 13)), 3266489909));
    return 4294967296.0 * static_cast<double>(2097151 & h2) + static_cast<double>(h1);
}
}  // namespace

std::int32_t nodeImul(std::int32_t a, std::int32_t b) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) * static_cast<std::uint32_t>(b));
}

double hashArray(std::span<const double> values) { return cyrb53Array(values); }

double hash(std::span<const double> params) { return cyrb53Array(params); }

double hashString(const std::string& value) {
    std::uint32_t h1 = 0xdeadbeefu;
    std::uint32_t h2 = 0x41c6ce57u;
    for (const auto ch : jsUTF16(value)) {
        const auto c = static_cast<std::uint32_t>(ch);
        h1 = static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h1 ^ c), 2654435761));
        h2 = static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h2 ^ c), 1597334677));
    }
    h1 = static_cast<std::uint32_t>(
        nodeImul(static_cast<std::int32_t>(h1 ^ (h1 >> 16)), 2246822507));
    h1 ^= static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h2 ^ (h2 >> 13)), 3266489909));
    h2 = static_cast<std::uint32_t>(
        nodeImul(static_cast<std::int32_t>(h2 ^ (h2 >> 16)), 2246822507));
    h2 ^= static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h1 ^ (h1 >> 13)), 3266489909));
    return 4294967296.0 * static_cast<double>(2097151 & h2) + static_cast<double>(h1);
}

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSPropertyDescriptor data(JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=d.configurable=d.enumerable=true;return d;}
JSValue call(const JSValue& receiver,const char* name,const Args& args={}){return receiver.get(name).call(receiver,args);}
double cyrb53(const JSValue& value){
    std::uint32_t h1=0xdeadbeefu,h2=0x41c6ce57u;const bool array=value.type()==JSValue::Type::Array;
    // Each length read, dynamic charCodeAt call and the TWO conversions of
    // every element are observable, including during callback reentry.
    for(double i=0;i<value.get("length").toNumber();++i){
        const auto element=array?value.get(JSValue(i).toString()):call(value,"charCodeAt",{i});
        const auto first=static_cast<std::uint32_t>(toInt32(element.toNumber()));h1=(h1^first)*2654435761u;
        const auto second=static_cast<std::uint32_t>(toInt32(element.toNumber()));h2=(h2^second)*1597334677u;
    }
    h1=(h1^(h1>>16))*2246822507u;h1^=(h2^(h2>>13))*3266489909u;
    h2=(h2^(h2>>16))*2246822507u;h2^=(h1^(h1>>13))*3266489909u;
    return 4294967296.0*double(2097151u&h2)+double(h1);
}
JSValue rest(const Args& args){auto result=JSValue::array(static_cast<std::uint32_t>(args.size()));for(std::size_t i=0;i<args.size();++i)result.defineProperty(std::to_string(i),data(args[i]));return result;}
JSValue hashOne(const JSValue&,const Args& args,const Args&){return cyrb53(arg(args,0));}
JSValue hashRest(const JSValue&,const Args& args,const Args&){return cyrb53(rest(args));}
bool test(const char* pattern,const JSValue& value){const auto regexp=createNativeRegExp(pattern);return call(regexp,"test",{value}).toBoolean();}
JSValue typeFromLength(const JSValue&,const Args& args,const Args& captures){return call(captures[0],"get",{arg(args,0)});}
JSValue typedArrayFromType(const JSValue&,const Args& args,const Args&){
    const auto type=arg(args,0);
    if(test("[iu]?vec\\d",type)){
        if(call(type,"startsWith",{"ivec"}).toBoolean())return NativeArray_detail::constructor(4);
        if(call(type,"startsWith",{"uvec"}).toBoolean())return NativeArray_detail::constructor(5);
        return NativeArray_detail::constructor(6);
    }
    if(test("mat\\d",type))return NativeArray_detail::constructor(6);
    if(test("float",type))return NativeArray_detail::constructor(6);
    if(test("uint",type))return NativeArray_detail::constructor(5);
    if(test("int",type))return NativeArray_detail::constructor(4);
    throw std::runtime_error("THREE.NodeUtils: Unsupported type: "+type.toString());
}
JSValue dimensions(const JSValue&,const Args& args,const Args& captures){
    const auto type=arg(args,0);const auto mode=captures[0].number();
    static constexpr std::array<const char*,7> patterns{"float|int|uint|bool","vec2","vec3","vec4","mat2","mat3","mat4"};
    static constexpr std::array<int,7> lengths{1,2,3,4,4,9,16},memory{1,2,3,4,4,12,16},alignment{1,2,4,4,2,4,4};
    for(std::size_t i=0;i<patterns.size();++i)if(test(patterns[i],type))return mode==0?lengths[i]:mode==1?memory[i]:alignment[i];
    const auto message="TSL: Unsupported type: "+type.toString();
    const auto stack=nativeStackTraceConstructor().construct();ThreeUtils::error({message,stack});return {};
}
bool instanceOf(const JSValue& value,const JSValue& constructor){
    const auto method=constructor.get(JSValue::wellKnownSymbol("hasInstance"));
    if(!method.isUndefined()&&!method.isNull())return method.call(constructor,{value}).toBoolean();
    if(!value.isObject())return false;
    const auto prototype=constructor.get("prototype");if(!prototype.isObject())throw JSTypeError("Non-object instanceof prototype");
    for(auto current=value.getPrototype();!current.isNull();current=current.getPrototype())if(current.strictEquals(prototype))return true;
    return false;
}
JSValue valueType(const JSValue&,const Args& args,const Args& captures){
    const auto value=arg(args,0);if(value.isUndefined()||value.isNull())return nullptr;
    if(value.get("isNode").strictEquals(true))return "node";
    if(value.type()==JSValue::Type::Number)return "float";
    if(value.type()==JSValue::Type::Boolean)return "bool";
    if(value.type()==JSValue::Type::String)return "string";
    if(value.isCallable())return "shader";
    for(const auto& [flag,type]:std::initializer_list<std::pair<const char*,const char*>>{
        {"isVector2","vec2"},{"isVector3","vec3"},{"isVector4","vec4"},{"isMatrix2","mat2"},
        {"isMatrix3","mat3"},{"isMatrix4","mat4"},{"isColor","color"}})if(value.get(flag).strictEquals(true))return type;
    if(instanceOf(value,captures[0]))return "ArrayBuffer";return nullptr;
}
JSValue textureType(const JSValue&,const Args& args,const Args& captures){
    const auto texture=arg(args,0);if(texture.get("isDepthTexture").strictEquals(true))return "float";
    const auto format=texture.get("format");int length=4;
    if(format.strictEquals(RedFormat)||format.strictEquals(RedIntegerFormat)||format.strictEquals(DepthFormat)||format.strictEquals(DepthStencilFormat)||format.strictEquals(AlphaFormat))length=1;
    else if(format.strictEquals(RGFormat)||format.strictEquals(RGIntegerFormat))length=2;
    else if(format.strictEquals(RGBFormat)||format.strictEquals(RGBIntegerFormat))length=3;
    const char* component;
    if(texture.get("type").strictEquals(UnsignedIntType))component="uint";
    else if(texture.get("type").strictEquals(IntType))component="int";else component="float";
    if(length==1)return component;
    const auto base=call(captures[0],"get",{length});if(component[0]=='f')return base;
    return std::string(1,component[0])+base.toPrimitive().toString();
}
JSValue dataFromObject(const JSValue&,const Args& args,const Args& captures){
    const auto object=arg(args,0),map=captures[0];auto result=call(map,"get",{object});
    if(result.isUndefined()){result=JSValue::object();call(map,"set",{object,result});}return result;
}
constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
JSValue encodeBuffer(const JSValue&,const Args& args,const Args&){
    const auto array=NativeArray_detail::constructor(1).construct({arg(args,0)});std::vector<std::uint8_t> bytes;
    for(double i=0;i<array.get("length").toNumber();++i){
        const auto c=static_cast<std::uint32_t>(toInt32(array.get(JSValue(i).toString()).toNumber()))&65535u;
        if(c>255)throw NodeUtilsUnsupported("btoa InvalidCharacterError DOMException is not available");bytes.push_back(static_cast<std::uint8_t>(c));
    }
    std::string output;for(std::size_t i=0;i<bytes.size();i+=3){
        const unsigned a=bytes[i],b=i+1<bytes.size()?bytes[i+1]:0,c=i+2<bytes.size()?bytes[i+2]:0;
        output+=alphabet[a>>2];output+=alphabet[((a&3)<<4)|(b>>4)];output+=i+1<bytes.size()?alphabet[((b&15)<<2)|(c>>6)]:'=';output+=i+2<bytes.size()?alphabet[c&63]:'=';
    }return output;
}
JSValue decodeBuffer(const JSValue&,const Args& args,const Args&){
    const auto units=jsUTF16(arg(args,0).toString());std::string input;
    auto invalid=[]()->void{throw NodeUtilsUnsupported("atob InvalidCharacterError DOMException is not available");};
    for(const auto unit:units){if(unit==9||unit==10||unit==12||unit==13||unit==32)continue;if(unit>127)invalid();input+=static_cast<char>(unit);}
    if(input.size()%4==0){if(!input.empty()&&input.back()=='=')input.pop_back();if(!input.empty()&&input.back()=='=')input.pop_back();}
    if(input.size()%4==1)invalid();
    std::vector<std::uint8_t> bytes;std::uint32_t bits=0;unsigned count=0;
    for(const char character:input){const auto found=std::string_view(alphabet,64).find(character);if(found==std::string_view::npos)invalid();bits=(bits<<6)|static_cast<std::uint32_t>(found);count+=6;if(count>=8){count-=8;bytes.push_back(static_cast<std::uint8_t>((bits>>count)&255));}}
    Uint8Array array(bytes.size());for(std::size_t i=0;i<bytes.size();++i)array.setIndex(double(i),bytes[i]);return nativeArrayBuffer(array.buffer);
}
double numeric(const Args& args,std::size_t index,double fallback=0){
    const auto value=arg(args,index);if(value.isUndefined())return fallback;
    if(value.type()!=JSValue::Type::Number)throw NodeUtilsUnsupported("NodeUtils: raw non-number math components require canonical JSValue constructors");return value.number();
}
JSValue valueFromType(const JSValue&,const Args& args,const Args&){
    const auto type=arg(args,0);auto params=Args(args.begin()+std::min<std::size_t>(args.size(),1),args.end());
    const auto last4=type.toBoolean()?call(type,"slice",{-4}):JSValue{};
    if(params.size()==1&&(last4.strictEquals("vec2")||last4.strictEquals("vec3")||last4.strictEquals("vec4")))params.resize(last4.strictEquals("vec2")?2:last4.strictEquals("vec3")?3:4,params[0]);
    if(type.strictEquals("color")){
        if(arg(params,1).isUndefined()&&arg(params,2).isUndefined()){
            const auto first=arg(params,0);if(first.isUndefined())return nativeColor(std::make_shared<Color>());
            if(first.type()==JSValue::Type::Number)return nativeColor(std::make_shared<Color>(first.number()));
            if(first.type()==JSValue::Type::String)return nativeColor(std::make_shared<Color>(first.string()));
            throw NodeUtilsUnsupported("NodeUtils: canonical Color constructor requires its full dynamic set protocol");
        }
        if(arg(params,0).isUndefined()||arg(params,1).isUndefined()||arg(params,2).isUndefined())throw NodeUtilsUnsupported("NodeUtils: raw undefined Color components require its canonical JSValue constructor");
        return nativeColor(std::make_shared<Color>(numeric(params,0),numeric(params,1),numeric(params,2)));
    }
    if(last4.strictEquals("vec2")){const auto x=numeric(params,0),y=numeric(params,1);return nativeVector(std::make_shared<Vector2>(x,y));}
    if(last4.strictEquals("vec3")){const auto x=numeric(params,0),y=numeric(params,1),z=numeric(params,2);return nativeVector(std::make_shared<Vector3>(x,y,z));}
    if(last4.strictEquals("vec4")){const auto x=numeric(params,0),y=numeric(params,1),z=numeric(params,2),w=numeric(params,3,1);return nativeVector(std::make_shared<Vector4>(x,y,z,w));}
    if(last4.strictEquals("mat2"))return nativeMatrix2Constructor().construct(params);
    if(last4.strictEquals("mat3")||last4.strictEquals("mat4")){
        const bool matrix4=last4.strictEquals("mat4");const auto value=matrix4?nativeMatrix(std::make_shared<Matrix4>()):nativeMatrix(std::make_shared<Matrix3>());
        if(!arg(params,0).isUndefined()){
            const auto size=matrix4?16:9;if(params.size()<static_cast<std::size_t>(size))throw NodeUtilsUnsupported("NodeUtils: incomplete raw matrix arguments require canonical JSValue constructors");
            Args values;for(int i=0;i<size;++i)values.emplace_back(numeric(params,i));call(value,"set",values);
        }return value;
    }
    const auto first=arg(params,0);
    if(type.strictEquals("bool"))return first.toBoolean()?first:JSValue(false);
    if(type.strictEquals("float")||type.strictEquals("int")||type.strictEquals("uint"))return first.toBoolean()?first:JSValue(0);
    if(type.strictEquals("string"))return first.toBoolean()?first:JSValue("");
    if(type.strictEquals("ArrayBuffer"))return decodeBuffer({},params,{});return nullptr;
}
JSValue arrayAsParameter(const JSValue&,const Args& args,const Args&){
    const auto params=arg(args,0);if(params.get("0").isUndefined()||params.get("0").isNull())return false;
    const auto node=params.get("0").get("isNode");if(node.toBoolean())return node;const auto object=params.get("0");
    // Object.getPrototypeOf mutation/boxing is an explicit remaining intrinsic
    // boundary; its unmodified source result is retained for supported values.
    if(!object.isObject()){if(object.isNull()||object.isUndefined())throw JSTypeError("Cannot convert undefined or null to object");return true;}
    return !object.getPrototype().strictEquals(JSValue::intrinsicObjectConstructor().get("prototype"));
}
void install(JSValue module,const char* name,JSValue::NativeFunction callback,unsigned arity,const Args& captures={},bool arrow=false){
    const auto function=arrow?JSValue::function(callback,captures,name,arity):JSValue::constructor(callback,captures,name,arity,JSValue::ConstructorKind::Callable);module.defineProperty(name,data(function));
}
}
JSValue nativeNodeUtils(){
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto module=weak.lock();module&&module->isObject())return *module;
    JSModuleInitializationScope scope(initializing,"NodeUtils");auto module=JSValue::object();const auto types=createNativeMap(),objects=createNativeWeakMap();
    for(const auto& [length,type]:std::initializer_list<std::pair<int,const char*>>{{1,"float"},{2,"vec2"},{3,"vec3"},{4,"vec4"},{9,"mat3"},{16,"mat4"}})call(types,"set",{length,type});
    install(module,"hashString",hashOne,1,{},true);install(module,"hashArray",hashOne,1,{},true);install(module,"hash",hashRest,0,{},true);
    install(module,"getTypeFromLength",typeFromLength,1,{types});install(module,"getTypedArrayFromType",typedArrayFromType,1);
    install(module,"getLengthFromType",dimensions,1,{0});install(module,"getMemoryLengthFromType",dimensions,1,{1});install(module,"getAlignmentFromType",dimensions,1,{2});
    const auto bufferConstructor=nativeArrayBufferConstructor();install(module,"getValueType",valueType,1,{bufferConstructor});install(module,"getTextureType",textureType,1,{types});
    install(module,"getValueFromType",valueFromType,1);install(module,"getDataFromObject",dataFromObject,1,{objects});install(module,"arrayBufferToBase64",encodeBuffer,1);install(module,"base64ToArrayBuffer",decodeBuffer,1);install(module,"isArrayAsParameter",arrayAsParameter,1);
    module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
