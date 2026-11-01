#include "nodes/utils/EquirectUV.h"

#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

double EquirectUV::clampForAngle(double value) {
    if (value < -1.0) return -1.0;
    if (value > 1.0) return 1.0;
    return value;
}

EquirectUV::Coordinates EquirectUV::toCoordinates(double x, double y, double z) {
    Coordinates out;
    // The angle takes the third part first and the first second.
    out.u = std::atan2(z, x) * (1.0 / (kPi * 2.0)) + 0.5;
    // The vertical takes the second part, clamped first so that a direction slightly longer
    // than one still gives an angle rather than a not a number.
    out.v = std::asin(clampForAngle(y)) * (1.0 / kPi) + 0.5;
    return out;
}

EquirectUV::Direction EquirectUV::toDirection(double u, double v) {
    Direction out;
    const double theta = (u - 0.5) * (kPi * 2.0);
    const double phi = (v - 0.5) * kPi;
    const double cosPhi = std::cos(phi);
    out.x = cosPhi * std::cos(theta);
    out.y = std::sin(phi);
    out.z = cosPhi * std::sin(theta);
    return out;
}

// Native source port of pinned three.js 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "nodes/utils/EquirectUV.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "math/JSNumberMath.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/UV.h"
#include "nodes/tsl/TSLCore.h"
#include <exception>
#include <string_view>
#include <unordered_set>
namespace EquirectUVManaged{

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot assign property of null/undefined");const auto property=key.toPropertyKey();if(base.isObject()){base.set(property,value);return;}auto object=base.toObject();if(!object.reflectSet(property,value,base))throw JSTypeError("Cannot assign primitive property");}
JSValue deleteReference(JSValue base,const JSValue& key){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot delete property of null/undefined");const auto property=key.toPropertyKey();auto object=base.toObject();if(!object.erase(property))throw JSTypeError("Cannot delete nonconfigurable property");return true;}
JSValue argumentObject(const Args& args){auto object=JSValue::object();JSPropertyDescriptor length;length.value=args.size();length.writable=length.configurable=true;object.defineProperty("length",length);for(unsigned i=0;i<args.size();++i)own(object,JSValue(i),args[i]);return object;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="in")return right.has(left);if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive();const auto b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString();const auto y=b.toString();return x+y;}const auto x=a.toNumber();const auto y=b.toNumber();return x+y;}
 if(op=="<"||op==">"||op=="<="||op==">="){const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}const auto x=a.toNumber();const auto y=b.toNumber();if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}
 const auto a=left.toNumber();const auto b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue nested0(const JSValue&,const Args&,const Args&);
JSValue nested1(const JSValue&,const Args&,const Args&);
JSValue nested0(const JSValue& nativeStatementTemp3,const Args& nativeStatementTemp4,const Args& nativeStatementTemp5){
auto nativeStatementTemp6=nativeStatementTemp5[0];auto nativeStatementTemp7=nativeStatementTemp5[1];
JSValue direction;{auto nativeStatementTemp9=getJSIterator(arg(nativeStatementTemp4,0));try{const auto nativeStatementTemp10nativeStatementTemp11=(nativeStatementTemp9.done?std::optional<JSValue>{}:stepJSIterator(nativeStatementTemp9));direction=nativeStatementTemp10nativeStatementTemp11?*nativeStatementTemp10nativeStatementTemp11:JSValue{};if(direction.isUndefined())direction=nativeStatementTemp6.get("positionWorldDirection");if(!nativeStatementTemp9.done){nativeStatementTemp9.done=true;closeJSIterator(nativeStatementTemp9);}}catch(...){if(!nativeStatementTemp9.done){nativeStatementTemp9.done=true;closeJSIterator(nativeStatementTemp9,std::current_exception());}throw;}}
JSValue u=([&]()->JSValue{auto nativeStatementTemp15=([&]()->JSValue{auto nativeStatementTemp20=([&]()->JSValue{auto nativeStatementTemp25=([&]()->JSValue{const auto nativeStatementTemp27=direction;const auto nativeStatementTemp28=JSValue("z");return nativeStatementTemp27.get(nativeStatementTemp28);}());const auto nativeStatementTemp26=JSValue("atan");const auto nativeStatementTemp22=nativeStatementTemp25.get(nativeStatementTemp26);const auto nativeStatementTemp23=nativeStatementTemp25;Args nativeStatementTemp24;nativeStatementTemp24.push_back(([&]()->JSValue{const auto nativeStatementTemp29=direction;const auto nativeStatementTemp30=JSValue("x");return nativeStatementTemp29.get(nativeStatementTemp30);}()));return nativeStatementTemp22.call(nativeStatementTemp23,nativeStatementTemp24);}());const auto nativeStatementTemp21=JSValue("mul");const auto nativeStatementTemp17=nativeStatementTemp20.get(nativeStatementTemp21);const auto nativeStatementTemp18=nativeStatementTemp20;Args nativeStatementTemp19;nativeStatementTemp19.push_back(([&]()->JSValue{const auto nativeStatementTemp31=JSValue(1);const auto nativeStatementTemp32=([&]()->JSValue{const auto nativeStatementTemp33=([&]()->JSValue{const auto nativeStatementTemp35=nativeGlobalBinding("Math");const auto nativeStatementTemp36=JSValue("PI");return nativeStatementTemp35.get(nativeStatementTemp36);}());const auto nativeStatementTemp34=JSValue(2);return binary("*",nativeStatementTemp33,nativeStatementTemp34);}());return binary("/",nativeStatementTemp31,nativeStatementTemp32);}()));return nativeStatementTemp17.call(nativeStatementTemp18,nativeStatementTemp19);}());const auto nativeStatementTemp16=JSValue("add");const auto nativeStatementTemp12=nativeStatementTemp15.get(nativeStatementTemp16);const auto nativeStatementTemp13=nativeStatementTemp15;Args nativeStatementTemp14;nativeStatementTemp14.push_back(JSValue(0.5));return nativeStatementTemp12.call(nativeStatementTemp13,nativeStatementTemp14);}());
JSValue v=([&]()->JSValue{auto nativeStatementTemp40=([&]()->JSValue{auto nativeStatementTemp45=([&]()->JSValue{auto nativeStatementTemp50=([&]()->JSValue{auto nativeStatementTemp55=([&]()->JSValue{const auto nativeStatementTemp57=direction;const auto nativeStatementTemp58=JSValue("y");return nativeStatementTemp57.get(nativeStatementTemp58);}());const auto nativeStatementTemp56=JSValue("clamp");const auto nativeStatementTemp52=nativeStatementTemp55.get(nativeStatementTemp56);const auto nativeStatementTemp53=nativeStatementTemp55;Args nativeStatementTemp54;nativeStatementTemp54.push_back(([&]()->JSValue{const auto nativeStatementTemp59=JSValue(1);return -nativeStatementTemp59.toNumber();}()));nativeStatementTemp54.push_back(JSValue(1));return nativeStatementTemp52.call(nativeStatementTemp53,nativeStatementTemp54);}());const auto nativeStatementTemp51=JSValue("asin");const auto nativeStatementTemp47=nativeStatementTemp50.get(nativeStatementTemp51);const auto nativeStatementTemp48=nativeStatementTemp50;Args nativeStatementTemp49;return nativeStatementTemp47.call(nativeStatementTemp48,nativeStatementTemp49);}());const auto nativeStatementTemp46=JSValue("mul");const auto nativeStatementTemp42=nativeStatementTemp45.get(nativeStatementTemp46);const auto nativeStatementTemp43=nativeStatementTemp45;Args nativeStatementTemp44;nativeStatementTemp44.push_back(([&]()->JSValue{const auto nativeStatementTemp60=JSValue(1);const auto nativeStatementTemp61=([&]()->JSValue{const auto nativeStatementTemp62=nativeGlobalBinding("Math");const auto nativeStatementTemp63=JSValue("PI");return nativeStatementTemp62.get(nativeStatementTemp63);}());return binary("/",nativeStatementTemp60,nativeStatementTemp61);}()));return nativeStatementTemp42.call(nativeStatementTemp43,nativeStatementTemp44);}());const auto nativeStatementTemp41=JSValue("add");const auto nativeStatementTemp37=nativeStatementTemp40.get(nativeStatementTemp41);const auto nativeStatementTemp38=nativeStatementTemp40;Args nativeStatementTemp39;nativeStatementTemp39.push_back(JSValue(0.5));return nativeStatementTemp37.call(nativeStatementTemp38,nativeStatementTemp39);}());
return ([&]()->JSValue{const auto nativeStatementTemp64=nativeStatementTemp6.get("vec2");const JSValue nativeStatementTemp65;Args nativeStatementTemp66;nativeStatementTemp66.push_back(u);nativeStatementTemp66.push_back(v);return nativeStatementTemp64.call(nativeStatementTemp65,nativeStatementTemp66);}());
return JSValue{};
}

JSValue nested1(const JSValue& nativeStatementTemp71,const Args& nativeStatementTemp72,const Args& nativeStatementTemp73){
auto nativeStatementTemp74=nativeStatementTemp73[0];auto nativeStatementTemp75=nativeStatementTemp73[1];
JSValue uv;{auto nativeStatementTemp77=getJSIterator(arg(nativeStatementTemp72,0));try{const auto nativeStatementTemp78nativeStatementTemp79=(nativeStatementTemp77.done?std::optional<JSValue>{}:stepJSIterator(nativeStatementTemp77));uv=nativeStatementTemp78nativeStatementTemp79?*nativeStatementTemp78nativeStatementTemp79:JSValue{};if(uv.isUndefined())uv=([&]()->JSValue{const auto nativeStatementTemp80=nativeStatementTemp74.get("UV");const JSValue nativeStatementTemp81;Args nativeStatementTemp82;return nativeStatementTemp80.call(nativeStatementTemp81,nativeStatementTemp82);}());if(!nativeStatementTemp77.done){nativeStatementTemp77.done=true;closeJSIterator(nativeStatementTemp77);}}catch(...){if(!nativeStatementTemp77.done){nativeStatementTemp77.done=true;closeJSIterator(nativeStatementTemp77,std::current_exception());}throw;}}
JSValue theta=([&]()->JSValue{auto nativeStatementTemp86=([&]()->JSValue{auto nativeStatementTemp91=([&]()->JSValue{const auto nativeStatementTemp93=uv;const auto nativeStatementTemp94=JSValue("x");return nativeStatementTemp93.get(nativeStatementTemp94);}());const auto nativeStatementTemp92=JSValue("sub");const auto nativeStatementTemp88=nativeStatementTemp91.get(nativeStatementTemp92);const auto nativeStatementTemp89=nativeStatementTemp91;Args nativeStatementTemp90;nativeStatementTemp90.push_back(JSValue(0.5));return nativeStatementTemp88.call(nativeStatementTemp89,nativeStatementTemp90);}());const auto nativeStatementTemp87=JSValue("mul");const auto nativeStatementTemp83=nativeStatementTemp86.get(nativeStatementTemp87);const auto nativeStatementTemp84=nativeStatementTemp86;Args nativeStatementTemp85;nativeStatementTemp85.push_back(([&]()->JSValue{const auto nativeStatementTemp95=([&]()->JSValue{const auto nativeStatementTemp97=nativeGlobalBinding("Math");const auto nativeStatementTemp98=JSValue("PI");return nativeStatementTemp97.get(nativeStatementTemp98);}());const auto nativeStatementTemp96=JSValue(2);return binary("*",nativeStatementTemp95,nativeStatementTemp96);}()));return nativeStatementTemp83.call(nativeStatementTemp84,nativeStatementTemp85);}());
JSValue phi=([&]()->JSValue{auto nativeStatementTemp102=([&]()->JSValue{auto nativeStatementTemp107=([&]()->JSValue{const auto nativeStatementTemp109=uv;const auto nativeStatementTemp110=JSValue("y");return nativeStatementTemp109.get(nativeStatementTemp110);}());const auto nativeStatementTemp108=JSValue("sub");const auto nativeStatementTemp104=nativeStatementTemp107.get(nativeStatementTemp108);const auto nativeStatementTemp105=nativeStatementTemp107;Args nativeStatementTemp106;nativeStatementTemp106.push_back(JSValue(0.5));return nativeStatementTemp104.call(nativeStatementTemp105,nativeStatementTemp106);}());const auto nativeStatementTemp103=JSValue("mul");const auto nativeStatementTemp99=nativeStatementTemp102.get(nativeStatementTemp103);const auto nativeStatementTemp100=nativeStatementTemp102;Args nativeStatementTemp101;nativeStatementTemp101.push_back(([&]()->JSValue{const auto nativeStatementTemp111=nativeGlobalBinding("Math");const auto nativeStatementTemp112=JSValue("PI");return nativeStatementTemp111.get(nativeStatementTemp112);}()));return nativeStatementTemp99.call(nativeStatementTemp100,nativeStatementTemp101);}());
JSValue cosPhi=([&]()->JSValue{auto nativeStatementTemp116=phi;const auto nativeStatementTemp117=JSValue("cos");const auto nativeStatementTemp113=nativeStatementTemp116.get(nativeStatementTemp117);const auto nativeStatementTemp114=nativeStatementTemp116;Args nativeStatementTemp115;return nativeStatementTemp113.call(nativeStatementTemp114,nativeStatementTemp115);}());
JSValue x=([&]()->JSValue{auto nativeStatementTemp121=cosPhi;const auto nativeStatementTemp122=JSValue("mul");const auto nativeStatementTemp118=nativeStatementTemp121.get(nativeStatementTemp122);const auto nativeStatementTemp119=nativeStatementTemp121;Args nativeStatementTemp120;nativeStatementTemp120.push_back(([&]()->JSValue{auto nativeStatementTemp126=theta;const auto nativeStatementTemp127=JSValue("cos");const auto nativeStatementTemp123=nativeStatementTemp126.get(nativeStatementTemp127);const auto nativeStatementTemp124=nativeStatementTemp126;Args nativeStatementTemp125;return nativeStatementTemp123.call(nativeStatementTemp124,nativeStatementTemp125);}()));return nativeStatementTemp118.call(nativeStatementTemp119,nativeStatementTemp120);}());
JSValue y=([&]()->JSValue{auto nativeStatementTemp131=phi;const auto nativeStatementTemp132=JSValue("sin");const auto nativeStatementTemp128=nativeStatementTemp131.get(nativeStatementTemp132);const auto nativeStatementTemp129=nativeStatementTemp131;Args nativeStatementTemp130;return nativeStatementTemp128.call(nativeStatementTemp129,nativeStatementTemp130);}());
JSValue z=([&]()->JSValue{auto nativeStatementTemp136=cosPhi;const auto nativeStatementTemp137=JSValue("mul");const auto nativeStatementTemp133=nativeStatementTemp136.get(nativeStatementTemp137);const auto nativeStatementTemp134=nativeStatementTemp136;Args nativeStatementTemp135;nativeStatementTemp135.push_back(([&]()->JSValue{auto nativeStatementTemp141=theta;const auto nativeStatementTemp142=JSValue("sin");const auto nativeStatementTemp138=nativeStatementTemp141.get(nativeStatementTemp142);const auto nativeStatementTemp139=nativeStatementTemp141;Args nativeStatementTemp140;return nativeStatementTemp138.call(nativeStatementTemp139,nativeStatementTemp140);}()));return nativeStatementTemp133.call(nativeStatementTemp134,nativeStatementTemp135);}());
return ([&]()->JSValue{const auto nativeStatementTemp143=nativeStatementTemp74.get("vec3");const JSValue nativeStatementTemp144;Args nativeStatementTemp145;nativeStatementTemp145.push_back(x);nativeStatementTemp145.push_back(y);nativeStatementTemp145.push_back(z);return nativeStatementTemp143.call(nativeStatementTemp144,nativeStatementTemp145);}());
return JSValue{};
}
}
JSValue nativeEquirectUVModule(){using namespace EquirectUVManaged;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"EquirectUV module");auto module=JSValue::object(nullptr);
own(module,"positionWorldDirection",nativePositionBinding("positionWorldDirection"));
own(module,"UV",nativeUVBinding("uv"));
own(module,"Fn",nativeTSLCoreBinding("Fn"));
own(module,"vec2",nativeTSLCoreBinding("vec2"));
own(module,"vec3",nativeTSLCoreBinding("vec3"));
own(module,"equirectUV",([&]()->JSValue{const auto nativeStatementTemp0=module.get("Fn");const JSValue nativeStatementTemp1;Args nativeStatementTemp2;nativeStatementTemp2.push_back(([&]()->JSValue{auto nativeStatementTemp67=JSValue::function(nested0,{module,JSValue{}},"",1);nativeStatementTemp67.setFunctionSourceText("( [ direction = positionWorldDirection ] ) => {\r\n\r\n\tconst u = direction.z.atan( direction.x ).mul( 1 / ( Math.PI * 2 ) ).add( 0.5 );\r\n\tconst v = direction.y.clamp( - 1.0, 1.0 ).asin().mul( 1 / Math.PI ).add( 0.5 );\r\n\r\n\treturn vec2( u, v );\r\n\r\n}");return nativeStatementTemp67;}()));return nativeStatementTemp0.call(nativeStatementTemp1,nativeStatementTemp2);}()));
own(module,"equirectDirection",([&]()->JSValue{const auto nativeStatementTemp68=module.get("Fn");const JSValue nativeStatementTemp69;Args nativeStatementTemp70;nativeStatementTemp70.push_back(([&]()->JSValue{auto nativeStatementTemp146=JSValue::function(nested1,{module,JSValue{}},"",1);nativeStatementTemp146.setFunctionSourceText("( [ uv = UV() ] ) => {\r\n\r\n\tconst theta = uv.x.sub( 0.5 ).mul( Math.PI * 2 );\r\n\tconst phi = uv.y.sub( 0.5 ).mul( Math.PI );\r\n\tconst cosPhi = phi.cos();\r\n\tconst x = cosPhi.mul( theta.cos() );\r\n\tconst y = phi.sin();\r\n\tconst z = cosPhi.mul( theta.sin() );\r\n\r\n\treturn vec3( x, y, z );\r\n\r\n}");return nativeStatementTemp146;}()));return nativeStatementTemp68.call(nativeStatementTemp69,nativeStatementTemp70);}()));
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();
own(facade,"equirectUV",module.get("equirectUV"));
own(facade,"equirectDirection",module.get("equirectDirection"));
facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeEquirectUVBinding(const std::string& name){const auto keeper=nativeEquirectUVModule();return EquirectUVManaged::lexical().lock()->get(name);}
