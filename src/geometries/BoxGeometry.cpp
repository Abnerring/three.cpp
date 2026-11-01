#include "geometries/BoxGeometry.h"
#include <array>
#include <cmath>
BoxGeometry::BoxGeometry(double width, double height, double depth, double widthSegments,
                         double heightSegments, double depthSegments)
    : parameters{width, height, depth, widthSegments, heightSegments, depthSegments} {
    type = "BoxGeometry";
    const double ws = std::floor(widthSegments), hs = std::floor(heightSegments), ds = std::floor(depthSegments);
    std::vector<double> vertices, normals, uvs;
    std::vector<std::uint32_t> indices;
    std::uint32_t vertexOffset = 0;
    std::size_t groupStart = 0;
    const auto buildPlane = [&](int u, int v, int w, int udir, int vdir, double width,
                                double height, double depth, double gx, double gy,
                                std::size_t materialIndex) {
        if (gy == INFINITY || (gy + 1 > 0 && gx == INFINITY))
            throw JSRangeError("BoxGeometry infinite subdivision does not terminate");
        const double sw = width / gx, sh = height / gy;
        std::uint32_t vertexCount = 0;
        std::size_t groupCount = 0;
        for (double iy = 0; iy < gy + 1; ++iy)
            for (double ix = 0; ix < gx + 1; ++ix) {
                std::array<double, 3> p{}, n{};
                p[u] = (ix * sw - width / 2) * udir;
                p[v] = (iy * sh - height / 2) * vdir;
                p[w] = depth / 2;
                n[w] = depth > 0 ? 1 : -1;
                vertices.insert(vertices.end(), p.begin(), p.end());
                normals.insert(normals.end(), n.begin(), n.end());
                uvs.insert(uvs.end(), {ix / gx, 1 - iy / gy});
                ++vertexCount;
            }
        for (double iy = 0; iy < gy; ++iy)
            for (double ix = 0; ix < gx; ++ix) {
                const auto a = vertexOffset + ix + (gx + 1) * iy,
                           b = vertexOffset + ix + (gx + 1) * (iy + 1),
                           c = vertexOffset + ix + 1 + (gx + 1) * (iy + 1),
                           d = vertexOffset + ix + 1 + (gx + 1) * iy;
                for (double index : {a, b, d, b, c, d}) indices.push_back(static_cast<std::uint32_t>(index));
                groupCount += 6;
            }
        addGroup(groupStart, groupCount, materialIndex);
        groupStart += groupCount;
        vertexOffset += vertexCount;
    };
    buildPlane(2, 1, 0, -1, -1, depth, height, width, ds, hs, 0);
    buildPlane(2, 1, 0, 1, -1, depth, height, -width, ds, hs, 1);
    buildPlane(0, 2, 1, 1, 1, width, depth, height, ws, ds, 2);
    buildPlane(0, 2, 1, 1, -1, width, depth, -height, ws, ds, 3);
    buildPlane(0, 1, 2, 1, -1, width, height, depth, ws, hs, 4);
    buildPlane(0, 1, 2, -1, -1, width, height, -depth, ws, hs, 5);
    setIndex(indices);
    setAttribute("position", std::make_shared<Float32BufferAttribute>(std::move(vertices), 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(std::move(normals), 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(std::move(uvs), 2));
}
BufferGeometry& BoxGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (auto p = dynamic_cast<const BoxGeometry*>(&source))
        parameters = p->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> BoxGeometry::clone() const {
    auto result = std::make_shared<BoxGeometry>();
    result->copy(*this);
    return result;
}
JsonValue BoxGeometry::toJSON() const {
    if (_transformed) {
        auto result = BufferGeometry::toJSON();
        result["type"] = "BufferGeometry";
        return result;
    }
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    result["width"] = parameters.width;
    result["height"] = parameters.height;
    result["depth"] = parameters.depth;
    result["widthSegments"] = parameters.widthSegments;
    result["heightSegments"] = parameters.heightSegments;
    result["depthSegments"] = parameters.depthSegments;
    return result;
}
std::shared_ptr<BoxGeometry> BoxGeometry::fromJSON(const JsonValue& json) {
    const auto number = [&](const char* key) { return json.contains(key) ? JSValue::fromJSON(json.at(key)).toNumber() : 1.; };
    return std::make_shared<BoxGeometry>(number("width"), number("height"), number("depth"),
        number("widthSegments"), number("heightSegments"), number("depthSegments"));
}

// Complete managed native port of pinned BoxGeometry.js. See THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "core/JSError.h"
#include "math/JSNumberMath.h"
#include "geometries/BoxGeometryRuntimeSource.inc"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "math/Vector3.h"
#include <string_view>
#include <unordered_set>
namespace BoxGeometryManaged{

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

JSValue makeLexicalCell(bool mutableBinding){auto cell=JSValue::object(nullptr);cell.setInternalSlot("initialized",false);cell.setInternalSlot("mutable",mutableBinding);return cell;}
JSValue readLexical(const JSValue& cell){if(!cell.getInternalSlot("initialized").toBoolean())throw JSReferenceError("Cannot access lexical binding before initialization");return cell.getInternalSlot("value");}
void initializeLexical(JSValue cell,const JSValue& value){if(cell.getInternalSlot("initialized").toBoolean())throw std::logic_error("Lexical binding initialized twice");cell.setInternalSlot("value",value);cell.setInternalSlot("initialized",true);}
void writeLexical(JSValue cell,const JSValue& value){if(!cell.getInternalSlot("initialized").toBoolean())throw JSReferenceError("Cannot access lexical binding before initialization");if(!cell.getInternalSlot("mutable").toBoolean())throw JSTypeError("Assignment to constant variable");cell.setInternalSlot("value",value);}

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue nested0(const JSValue& nativeStatementTemp13,const Args& nativeStatementTemp14,const Args& nativeStatementTemp15){
auto nativeStatementTemp16=nativeStatementTemp15[0];auto nativeStatementTemp17=nativeStatementTemp13;
JSValue u=arg(nativeStatementTemp14,0);
JSValue v=arg(nativeStatementTemp14,1);
JSValue w=arg(nativeStatementTemp14,2);
JSValue udir=arg(nativeStatementTemp14,3);
JSValue vdir=arg(nativeStatementTemp14,4);
JSValue width=arg(nativeStatementTemp14,5);
JSValue height=arg(nativeStatementTemp14,6);
JSValue depth=arg(nativeStatementTemp14,7);
JSValue gridX=arg(nativeStatementTemp14,8);
JSValue gridY=arg(nativeStatementTemp14,9);
JSValue materialIndex=arg(nativeStatementTemp14,10);
JSValue segmentWidth=([&]()->JSValue{const auto nativeStatementTemp19=width;const auto nativeStatementTemp20=gridX;return binary("/",nativeStatementTemp19,nativeStatementTemp20);}());
JSValue segmentHeight=([&]()->JSValue{const auto nativeStatementTemp21=height;const auto nativeStatementTemp22=gridY;return binary("/",nativeStatementTemp21,nativeStatementTemp22);}());
JSValue widthHalf=([&]()->JSValue{const auto nativeStatementTemp23=width;const auto nativeStatementTemp24=JSValue(2);return binary("/",nativeStatementTemp23,nativeStatementTemp24);}());
JSValue heightHalf=([&]()->JSValue{const auto nativeStatementTemp25=height;const auto nativeStatementTemp26=JSValue(2);return binary("/",nativeStatementTemp25,nativeStatementTemp26);}());
JSValue depthHalf=([&]()->JSValue{const auto nativeStatementTemp27=depth;const auto nativeStatementTemp28=JSValue(2);return binary("/",nativeStatementTemp27,nativeStatementTemp28);}());
JSValue gridX1=([&]()->JSValue{const auto nativeStatementTemp29=gridX;const auto nativeStatementTemp30=JSValue(1);return binary("+",nativeStatementTemp29,nativeStatementTemp30);}());
JSValue gridY1=([&]()->JSValue{const auto nativeStatementTemp31=gridY;const auto nativeStatementTemp32=JSValue(1);return binary("+",nativeStatementTemp31,nativeStatementTemp32);}());
JSValue vertexCounter=JSValue(0);
JSValue groupCount=JSValue(0);
JSValue vector=([&]()->JSValue{const auto nativeStatementTemp33=nativeStatementTemp16.get("Vector3");Args nativeStatementTemp34;return nativeStatementTemp33.construct(nativeStatementTemp34);}());
{JSValue iy=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp35=iy;const auto nativeStatementTemp36=gridY1;return binary("<",nativeStatementTemp35,nativeStatementTemp36);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp37=iy.toNumber();const auto nativeStatementTemp38=nativeStatementTemp37+1;iy=nativeStatementTemp38;return nativeStatementTemp37;}())){
JSValue y=([&]()->JSValue{const auto nativeStatementTemp39=([&]()->JSValue{const auto nativeStatementTemp41=iy;const auto nativeStatementTemp42=segmentHeight;return binary("*",nativeStatementTemp41,nativeStatementTemp42);}());const auto nativeStatementTemp40=heightHalf;return binary("-",nativeStatementTemp39,nativeStatementTemp40);}());
{JSValue ix=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp43=ix;const auto nativeStatementTemp44=gridX1;return binary("<",nativeStatementTemp43,nativeStatementTemp44);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp45=ix.toNumber();const auto nativeStatementTemp46=nativeStatementTemp45+1;ix=nativeStatementTemp46;return nativeStatementTemp45;}())){
JSValue x=([&]()->JSValue{const auto nativeStatementTemp47=([&]()->JSValue{const auto nativeStatementTemp49=ix;const auto nativeStatementTemp50=segmentWidth;return binary("*",nativeStatementTemp49,nativeStatementTemp50);}());const auto nativeStatementTemp48=widthHalf;return binary("-",nativeStatementTemp47,nativeStatementTemp48);}());
([&]()->JSValue{auto nativeStatementTemp51=vector;const auto nativeStatementTemp52=u;const auto nativeStatementTemp53=([&]()->JSValue{const auto nativeStatementTemp56=x;const auto nativeStatementTemp57=udir;return binary("*",nativeStatementTemp56,nativeStatementTemp57);}());setReference(nativeStatementTemp51,nativeStatementTemp52,nativeStatementTemp53);return nativeStatementTemp53;}());
([&]()->JSValue{auto nativeStatementTemp58=vector;const auto nativeStatementTemp59=v;const auto nativeStatementTemp60=([&]()->JSValue{const auto nativeStatementTemp63=y;const auto nativeStatementTemp64=vdir;return binary("*",nativeStatementTemp63,nativeStatementTemp64);}());setReference(nativeStatementTemp58,nativeStatementTemp59,nativeStatementTemp60);return nativeStatementTemp60;}());
([&]()->JSValue{auto nativeStatementTemp65=vector;const auto nativeStatementTemp66=w;const auto nativeStatementTemp67=depthHalf;setReference(nativeStatementTemp65,nativeStatementTemp66,nativeStatementTemp67);return nativeStatementTemp67;}());
([&]()->JSValue{auto nativeStatementTemp73=readLexical(nativeStatementTemp15[1]);const auto nativeStatementTemp74=JSValue("push");const auto nativeStatementTemp70=nativeStatementTemp73.get(nativeStatementTemp74);const auto nativeStatementTemp71=nativeStatementTemp73;Args nativeStatementTemp72;nativeStatementTemp72.push_back(([&]()->JSValue{const auto nativeStatementTemp75=vector;const auto nativeStatementTemp76=JSValue("x");return nativeStatementTemp75.get(nativeStatementTemp76);}()));nativeStatementTemp72.push_back(([&]()->JSValue{const auto nativeStatementTemp77=vector;const auto nativeStatementTemp78=JSValue("y");return nativeStatementTemp77.get(nativeStatementTemp78);}()));nativeStatementTemp72.push_back(([&]()->JSValue{const auto nativeStatementTemp79=vector;const auto nativeStatementTemp80=JSValue("z");return nativeStatementTemp79.get(nativeStatementTemp80);}()));return nativeStatementTemp70.call(nativeStatementTemp71,nativeStatementTemp72);}());
([&]()->JSValue{auto nativeStatementTemp81=vector;const auto nativeStatementTemp82=u;const auto nativeStatementTemp83=JSValue(0);setReference(nativeStatementTemp81,nativeStatementTemp82,nativeStatementTemp83);return nativeStatementTemp83;}());
([&]()->JSValue{auto nativeStatementTemp86=vector;const auto nativeStatementTemp87=v;const auto nativeStatementTemp88=JSValue(0);setReference(nativeStatementTemp86,nativeStatementTemp87,nativeStatementTemp88);return nativeStatementTemp88;}());
([&]()->JSValue{auto nativeStatementTemp91=vector;const auto nativeStatementTemp92=w;const auto nativeStatementTemp93=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp96=depth;const auto nativeStatementTemp97=JSValue(0);return binary(">",nativeStatementTemp96,nativeStatementTemp97);}()).toBoolean())return JSValue(1);return ([&]()->JSValue{const auto nativeStatementTemp98=JSValue(1);return -nativeStatementTemp98.toNumber();}());}());setReference(nativeStatementTemp91,nativeStatementTemp92,nativeStatementTemp93);return nativeStatementTemp93;}());
([&]()->JSValue{auto nativeStatementTemp102=readLexical(nativeStatementTemp15[2]);const auto nativeStatementTemp103=JSValue("push");const auto nativeStatementTemp99=nativeStatementTemp102.get(nativeStatementTemp103);const auto nativeStatementTemp100=nativeStatementTemp102;Args nativeStatementTemp101;nativeStatementTemp101.push_back(([&]()->JSValue{const auto nativeStatementTemp104=vector;const auto nativeStatementTemp105=JSValue("x");return nativeStatementTemp104.get(nativeStatementTemp105);}()));nativeStatementTemp101.push_back(([&]()->JSValue{const auto nativeStatementTemp106=vector;const auto nativeStatementTemp107=JSValue("y");return nativeStatementTemp106.get(nativeStatementTemp107);}()));nativeStatementTemp101.push_back(([&]()->JSValue{const auto nativeStatementTemp108=vector;const auto nativeStatementTemp109=JSValue("z");return nativeStatementTemp108.get(nativeStatementTemp109);}()));return nativeStatementTemp99.call(nativeStatementTemp100,nativeStatementTemp101);}());
([&]()->JSValue{auto nativeStatementTemp113=readLexical(nativeStatementTemp15[3]);const auto nativeStatementTemp114=JSValue("push");const auto nativeStatementTemp110=nativeStatementTemp113.get(nativeStatementTemp114);const auto nativeStatementTemp111=nativeStatementTemp113;Args nativeStatementTemp112;nativeStatementTemp112.push_back(([&]()->JSValue{const auto nativeStatementTemp115=ix;const auto nativeStatementTemp116=gridX;return binary("/",nativeStatementTemp115,nativeStatementTemp116);}()));return nativeStatementTemp110.call(nativeStatementTemp111,nativeStatementTemp112);}());
([&]()->JSValue{auto nativeStatementTemp120=readLexical(nativeStatementTemp15[3]);const auto nativeStatementTemp121=JSValue("push");const auto nativeStatementTemp117=nativeStatementTemp120.get(nativeStatementTemp121);const auto nativeStatementTemp118=nativeStatementTemp120;Args nativeStatementTemp119;nativeStatementTemp119.push_back(([&]()->JSValue{const auto nativeStatementTemp122=JSValue(1);const auto nativeStatementTemp123=([&]()->JSValue{const auto nativeStatementTemp124=iy;const auto nativeStatementTemp125=gridY;return binary("/",nativeStatementTemp124,nativeStatementTemp125);}());return binary("-",nativeStatementTemp122,nativeStatementTemp123);}()));return nativeStatementTemp117.call(nativeStatementTemp118,nativeStatementTemp119);}());
([&]()->JSValue{const auto nativeStatementTemp127=vertexCounter;const auto nativeStatementTemp128=JSValue(1);const auto nativeStatementTemp126=binary("+",nativeStatementTemp127,nativeStatementTemp128);vertexCounter=nativeStatementTemp126;return nativeStatementTemp126;}());

}
}

}
}
{JSValue iy=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp129=iy;const auto nativeStatementTemp130=gridY;return binary("<",nativeStatementTemp129,nativeStatementTemp130);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp131=iy.toNumber();const auto nativeStatementTemp132=nativeStatementTemp131+1;iy=nativeStatementTemp132;return nativeStatementTemp131;}())){
{JSValue ix=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp133=ix;const auto nativeStatementTemp134=gridX;return binary("<",nativeStatementTemp133,nativeStatementTemp134);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp135=ix.toNumber();const auto nativeStatementTemp136=nativeStatementTemp135+1;ix=nativeStatementTemp136;return nativeStatementTemp135;}())){
JSValue a=([&]()->JSValue{const auto nativeStatementTemp137=([&]()->JSValue{const auto nativeStatementTemp139=readLexical(nativeStatementTemp15[4]);const auto nativeStatementTemp140=ix;return binary("+",nativeStatementTemp139,nativeStatementTemp140);}());const auto nativeStatementTemp138=([&]()->JSValue{const auto nativeStatementTemp141=gridX1;const auto nativeStatementTemp142=iy;return binary("*",nativeStatementTemp141,nativeStatementTemp142);}());return binary("+",nativeStatementTemp137,nativeStatementTemp138);}());
JSValue b=([&]()->JSValue{const auto nativeStatementTemp143=([&]()->JSValue{const auto nativeStatementTemp145=readLexical(nativeStatementTemp15[4]);const auto nativeStatementTemp146=ix;return binary("+",nativeStatementTemp145,nativeStatementTemp146);}());const auto nativeStatementTemp144=([&]()->JSValue{const auto nativeStatementTemp147=gridX1;const auto nativeStatementTemp148=([&]()->JSValue{const auto nativeStatementTemp149=iy;const auto nativeStatementTemp150=JSValue(1);return binary("+",nativeStatementTemp149,nativeStatementTemp150);}());return binary("*",nativeStatementTemp147,nativeStatementTemp148);}());return binary("+",nativeStatementTemp143,nativeStatementTemp144);}());
JSValue c=([&]()->JSValue{const auto nativeStatementTemp151=([&]()->JSValue{const auto nativeStatementTemp153=readLexical(nativeStatementTemp15[4]);const auto nativeStatementTemp154=([&]()->JSValue{const auto nativeStatementTemp155=ix;const auto nativeStatementTemp156=JSValue(1);return binary("+",nativeStatementTemp155,nativeStatementTemp156);}());return binary("+",nativeStatementTemp153,nativeStatementTemp154);}());const auto nativeStatementTemp152=([&]()->JSValue{const auto nativeStatementTemp157=gridX1;const auto nativeStatementTemp158=([&]()->JSValue{const auto nativeStatementTemp159=iy;const auto nativeStatementTemp160=JSValue(1);return binary("+",nativeStatementTemp159,nativeStatementTemp160);}());return binary("*",nativeStatementTemp157,nativeStatementTemp158);}());return binary("+",nativeStatementTemp151,nativeStatementTemp152);}());
JSValue d=([&]()->JSValue{const auto nativeStatementTemp161=([&]()->JSValue{const auto nativeStatementTemp163=readLexical(nativeStatementTemp15[4]);const auto nativeStatementTemp164=([&]()->JSValue{const auto nativeStatementTemp165=ix;const auto nativeStatementTemp166=JSValue(1);return binary("+",nativeStatementTemp165,nativeStatementTemp166);}());return binary("+",nativeStatementTemp163,nativeStatementTemp164);}());const auto nativeStatementTemp162=([&]()->JSValue{const auto nativeStatementTemp167=gridX1;const auto nativeStatementTemp168=iy;return binary("*",nativeStatementTemp167,nativeStatementTemp168);}());return binary("+",nativeStatementTemp161,nativeStatementTemp162);}());
([&]()->JSValue{auto nativeStatementTemp172=readLexical(nativeStatementTemp15[5]);const auto nativeStatementTemp173=JSValue("push");const auto nativeStatementTemp169=nativeStatementTemp172.get(nativeStatementTemp173);const auto nativeStatementTemp170=nativeStatementTemp172;Args nativeStatementTemp171;nativeStatementTemp171.push_back(a);nativeStatementTemp171.push_back(b);nativeStatementTemp171.push_back(d);return nativeStatementTemp169.call(nativeStatementTemp170,nativeStatementTemp171);}());
([&]()->JSValue{auto nativeStatementTemp177=readLexical(nativeStatementTemp15[5]);const auto nativeStatementTemp178=JSValue("push");const auto nativeStatementTemp174=nativeStatementTemp177.get(nativeStatementTemp178);const auto nativeStatementTemp175=nativeStatementTemp177;Args nativeStatementTemp176;nativeStatementTemp176.push_back(b);nativeStatementTemp176.push_back(c);nativeStatementTemp176.push_back(d);return nativeStatementTemp174.call(nativeStatementTemp175,nativeStatementTemp176);}());
([&]()->JSValue{const auto nativeStatementTemp180=groupCount;const auto nativeStatementTemp181=JSValue(6);const auto nativeStatementTemp179=binary("+",nativeStatementTemp180,nativeStatementTemp181);groupCount=nativeStatementTemp179;return nativeStatementTemp179;}());

}
}

}
}
([&]()->JSValue{auto nativeStatementTemp185=readLexical(nativeStatementTemp15[6]);const auto nativeStatementTemp186=JSValue("addGroup");const auto nativeStatementTemp182=nativeStatementTemp185.get(nativeStatementTemp186);const auto nativeStatementTemp183=nativeStatementTemp185;Args nativeStatementTemp184;nativeStatementTemp184.push_back(readLexical(nativeStatementTemp15[7]));nativeStatementTemp184.push_back(groupCount);nativeStatementTemp184.push_back(materialIndex);return nativeStatementTemp182.call(nativeStatementTemp183,nativeStatementTemp184);}());
([&]()->JSValue{const auto nativeStatementTemp188=readLexical(nativeStatementTemp15[7]);const auto nativeStatementTemp189=groupCount;const auto nativeStatementTemp187=binary("+",nativeStatementTemp188,nativeStatementTemp189);writeLexical(nativeStatementTemp15[7],nativeStatementTemp187);return nativeStatementTemp187;}());
([&]()->JSValue{const auto nativeStatementTemp191=readLexical(nativeStatementTemp15[4]);const auto nativeStatementTemp192=vertexCounter;const auto nativeStatementTemp190=binary("+",nativeStatementTemp191,nativeStatementTemp192);writeLexical(nativeStatementTemp15[4],nativeStatementTemp190);return nativeStatementTemp190;}());
return JSValue{};
}

JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
JSValue width=arg(nativeStatementTemp1,0);
if(width.isUndefined())width=JSValue(1);
JSValue height=arg(nativeStatementTemp1,1);
if(height.isUndefined())height=JSValue(1);
JSValue depth=arg(nativeStatementTemp1,2);
if(depth.isUndefined())depth=JSValue(1);
JSValue widthSegments=arg(nativeStatementTemp1,3);
if(widthSegments.isUndefined())widthSegments=JSValue(1);
JSValue heightSegments=arg(nativeStatementTemp1,4);
if(heightSegments.isUndefined())heightSegments=JSValue(1);
JSValue depthSegments=arg(nativeStatementTemp1,5);
if(depthSegments.isUndefined())depthSegments=JSValue(1);
auto nativeStatementTemp6=makeLexicalCell(false);
auto nativeStatementTemp7=makeLexicalCell(false);
auto nativeStatementTemp8=makeLexicalCell(false);
auto nativeStatementTemp9=makeLexicalCell(true);
auto nativeStatementTemp10=makeLexicalCell(false);
auto nativeStatementTemp11=makeLexicalCell(false);
auto nativeStatementTemp12=makeLexicalCell(true);
JSValue buildPlane=([&]()->JSValue{auto nativeStatementTemp193=JSValue::constructor(nested0,{nativeStatementTemp3,nativeStatementTemp6,nativeStatementTemp7,nativeStatementTemp8,nativeStatementTemp9,nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12},"buildPlane",11,JSValue::ConstructorKind::Callable);nativeStatementTemp193.setFunctionSourceText("function buildPlane( u, v, w, udir, vdir, width, height, depth, gridX, gridY, materialIndex ) {\r\n\r\n\t\t\tconst segmentWidth = width / gridX;\r\n\t\t\tconst segmentHeight = height / gridY;\r\n\r\n\t\t\tconst widthHalf = width / 2;\r\n\t\t\tconst heightHalf = height / 2;\r\n\t\t\tconst depthHalf = depth / 2;\r\n\r\n\t\t\tconst gridX1 = gridX + 1;\r\n\t\t\tconst gridY1 = gridY + 1;\r\n\r\n\t\t\tlet vertexCounter = 0;\r\n\t\t\tlet groupCount = 0;\r\n\r\n\t\t\tconst vector = new Vector3();\r\n\r\n\t\t\t// generate vertices, normals and uvs\r\n\r\n\t\t\tfor ( let iy = 0; iy < gridY1; iy ++ ) {\r\n\r\n\t\t\t\tconst y = iy * segmentHeight - heightHalf;\r\n\r\n\t\t\t\tfor ( let ix = 0; ix < gridX1; ix ++ ) {\r\n\r\n\t\t\t\t\tconst x = ix * segmentWidth - widthHalf;\r\n\r\n\t\t\t\t\t// set values to correct vector component\r\n\r\n\t\t\t\t\tvector[ u ] = x * udir;\r\n\t\t\t\t\tvector[ v ] = y * vdir;\r\n\t\t\t\t\tvector[ w ] = depthHalf;\r\n\r\n\t\t\t\t\t// now apply vector to vertex buffer\r\n\r\n\t\t\t\t\tvertices.push( vector.x, vector.y, vector.z );\r\n\r\n\t\t\t\t\t// set values to correct vector component\r\n\r\n\t\t\t\t\tvector[ u ] = 0;\r\n\t\t\t\t\tvector[ v ] = 0;\r\n\t\t\t\t\tvector[ w ] = depth > 0 ? 1 : - 1;\r\n\r\n\t\t\t\t\t// now apply vector to normal buffer\r\n\r\n\t\t\t\t\tnormals.push( vector.x, vector.y, vector.z );\r\n\r\n\t\t\t\t\t// uvs\r\n\r\n\t\t\t\t\tuvs.push( ix / gridX );\r\n\t\t\t\t\tuvs.push( 1 - ( iy / gridY ) );\r\n\r\n\t\t\t\t\t// counters\r\n\r\n\t\t\t\t\tvertexCounter += 1;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t\t// indices\r\n\r\n\t\t\t// 1. you need three indices to draw a single face\r\n\t\t\t// 2. a single segment consists of two faces\r\n\t\t\t// 3. so we need to generate six (2*3) indices per segment\r\n\r\n\t\t\tfor ( let iy = 0; iy < gridY; iy ++ ) {\r\n\r\n\t\t\t\tfor ( let ix = 0; ix < gridX; ix ++ ) {\r\n\r\n\t\t\t\t\tconst a = numberOfVertices + ix + gridX1 * iy;\r\n\t\t\t\t\tconst b = numberOfVertices + ix + gridX1 * ( iy + 1 );\r\n\t\t\t\t\tconst c = numberOfVertices + ( ix + 1 ) + gridX1 * ( iy + 1 );\r\n\t\t\t\t\tconst d = numberOfVertices + ( ix + 1 ) + gridX1 * iy;\r\n\r\n\t\t\t\t\t// faces\r\n\r\n\t\t\t\t\tindices.push( a, b, d );\r\n\t\t\t\t\tindices.push( b, c, d );\r\n\r\n\t\t\t\t\t// increase counter\r\n\r\n\t\t\t\t\tgroupCount += 6;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t\t// add a group to the geometry. this will ensure multi material support\r\n\r\n\t\t\tscope.addGroup( groupStart, groupCount, materialIndex );\r\n\r\n\t\t\t// calculate new start value for groups\r\n\r\n\t\t\tgroupStart += groupCount;\r\n\r\n\t\t\t// update total number of vertices\r\n\r\n\t\t\tnumberOfVertices += vertexCounter;\r\n\r\n\t\t}");return nativeStatementTemp193;}());
([&]()->JSValue{const auto nativeStatementTemp197=nativeStatementTemp3.get("BoxGeometry").getPrototype();Args nativeStatementTemp196;nativeStatementTemp4=nativeStatementTemp197.construct(nativeStatementTemp196,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp198=nativeStatementTemp4;const auto nativeStatementTemp199=JSValue("type");const auto nativeStatementTemp200=JSValue("BoxGeometry");setReference(nativeStatementTemp198,nativeStatementTemp199,nativeStatementTemp200);return nativeStatementTemp200;}());
([&]()->JSValue{auto nativeStatementTemp203=nativeStatementTemp4;const auto nativeStatementTemp204=JSValue("parameters");const auto nativeStatementTemp205=([&]()->JSValue{auto nativeStatementTemp208=JSValue::object();{const auto nativeStatementTemp209=JSValue("width");const auto nativeStatementTemp210=width;own(nativeStatementTemp208,nativeStatementTemp209,nativeStatementTemp210);}{const auto nativeStatementTemp211=JSValue("height");const auto nativeStatementTemp212=height;own(nativeStatementTemp208,nativeStatementTemp211,nativeStatementTemp212);}{const auto nativeStatementTemp213=JSValue("depth");const auto nativeStatementTemp214=depth;own(nativeStatementTemp208,nativeStatementTemp213,nativeStatementTemp214);}{const auto nativeStatementTemp215=JSValue("widthSegments");const auto nativeStatementTemp216=widthSegments;own(nativeStatementTemp208,nativeStatementTemp215,nativeStatementTemp216);}{const auto nativeStatementTemp217=JSValue("heightSegments");const auto nativeStatementTemp218=heightSegments;own(nativeStatementTemp208,nativeStatementTemp217,nativeStatementTemp218);}{const auto nativeStatementTemp219=JSValue("depthSegments");const auto nativeStatementTemp220=depthSegments;own(nativeStatementTemp208,nativeStatementTemp219,nativeStatementTemp220);}return nativeStatementTemp208;}());setReference(nativeStatementTemp203,nativeStatementTemp204,nativeStatementTemp205);return nativeStatementTemp205;}());
initializeLexical(nativeStatementTemp11,nativeStatementTemp4);
([&]()->JSValue{const auto nativeStatementTemp221=([&]()->JSValue{auto nativeStatementTemp227=nativeGlobalBinding("Math");const auto nativeStatementTemp228=JSValue("floor");const auto nativeStatementTemp224=nativeStatementTemp227.get(nativeStatementTemp228);const auto nativeStatementTemp225=nativeStatementTemp227;Args nativeStatementTemp226;nativeStatementTemp226.push_back(widthSegments);return nativeStatementTemp224.call(nativeStatementTemp225,nativeStatementTemp226);}());widthSegments=nativeStatementTemp221;return nativeStatementTemp221;}());
([&]()->JSValue{const auto nativeStatementTemp229=([&]()->JSValue{auto nativeStatementTemp235=nativeGlobalBinding("Math");const auto nativeStatementTemp236=JSValue("floor");const auto nativeStatementTemp232=nativeStatementTemp235.get(nativeStatementTemp236);const auto nativeStatementTemp233=nativeStatementTemp235;Args nativeStatementTemp234;nativeStatementTemp234.push_back(heightSegments);return nativeStatementTemp232.call(nativeStatementTemp233,nativeStatementTemp234);}());heightSegments=nativeStatementTemp229;return nativeStatementTemp229;}());
([&]()->JSValue{const auto nativeStatementTemp237=([&]()->JSValue{auto nativeStatementTemp243=nativeGlobalBinding("Math");const auto nativeStatementTemp244=JSValue("floor");const auto nativeStatementTemp240=nativeStatementTemp243.get(nativeStatementTemp244);const auto nativeStatementTemp241=nativeStatementTemp243;Args nativeStatementTemp242;nativeStatementTemp242.push_back(depthSegments);return nativeStatementTemp240.call(nativeStatementTemp241,nativeStatementTemp242);}());depthSegments=nativeStatementTemp237;return nativeStatementTemp237;}());
initializeLexical(nativeStatementTemp10,([&]()->JSValue{auto nativeStatementTemp245=JSValue::array();return nativeStatementTemp245;}()));
initializeLexical(nativeStatementTemp6,([&]()->JSValue{auto nativeStatementTemp246=JSValue::array();return nativeStatementTemp246;}()));
initializeLexical(nativeStatementTemp7,([&]()->JSValue{auto nativeStatementTemp247=JSValue::array();return nativeStatementTemp247;}()));
initializeLexical(nativeStatementTemp8,([&]()->JSValue{auto nativeStatementTemp248=JSValue::array();return nativeStatementTemp248;}()));
initializeLexical(nativeStatementTemp9,JSValue(0));
initializeLexical(nativeStatementTemp12,JSValue(0));
([&]()->JSValue{const auto nativeStatementTemp249=buildPlane;const JSValue nativeStatementTemp250;Args nativeStatementTemp251;nativeStatementTemp251.push_back(JSValue("z"));nativeStatementTemp251.push_back(JSValue("y"));nativeStatementTemp251.push_back(JSValue("x"));nativeStatementTemp251.push_back(([&]()->JSValue{const auto nativeStatementTemp252=JSValue(1);return -nativeStatementTemp252.toNumber();}()));nativeStatementTemp251.push_back(([&]()->JSValue{const auto nativeStatementTemp253=JSValue(1);return -nativeStatementTemp253.toNumber();}()));nativeStatementTemp251.push_back(depth);nativeStatementTemp251.push_back(height);nativeStatementTemp251.push_back(width);nativeStatementTemp251.push_back(depthSegments);nativeStatementTemp251.push_back(heightSegments);nativeStatementTemp251.push_back(JSValue(0));return nativeStatementTemp249.call(nativeStatementTemp250,nativeStatementTemp251);}());
([&]()->JSValue{const auto nativeStatementTemp254=buildPlane;const JSValue nativeStatementTemp255;Args nativeStatementTemp256;nativeStatementTemp256.push_back(JSValue("z"));nativeStatementTemp256.push_back(JSValue("y"));nativeStatementTemp256.push_back(JSValue("x"));nativeStatementTemp256.push_back(JSValue(1));nativeStatementTemp256.push_back(([&]()->JSValue{const auto nativeStatementTemp257=JSValue(1);return -nativeStatementTemp257.toNumber();}()));nativeStatementTemp256.push_back(depth);nativeStatementTemp256.push_back(height);nativeStatementTemp256.push_back(([&]()->JSValue{const auto nativeStatementTemp258=width;return -nativeStatementTemp258.toNumber();}()));nativeStatementTemp256.push_back(depthSegments);nativeStatementTemp256.push_back(heightSegments);nativeStatementTemp256.push_back(JSValue(1));return nativeStatementTemp254.call(nativeStatementTemp255,nativeStatementTemp256);}());
([&]()->JSValue{const auto nativeStatementTemp259=buildPlane;const JSValue nativeStatementTemp260;Args nativeStatementTemp261;nativeStatementTemp261.push_back(JSValue("x"));nativeStatementTemp261.push_back(JSValue("z"));nativeStatementTemp261.push_back(JSValue("y"));nativeStatementTemp261.push_back(JSValue(1));nativeStatementTemp261.push_back(JSValue(1));nativeStatementTemp261.push_back(width);nativeStatementTemp261.push_back(depth);nativeStatementTemp261.push_back(height);nativeStatementTemp261.push_back(widthSegments);nativeStatementTemp261.push_back(depthSegments);nativeStatementTemp261.push_back(JSValue(2));return nativeStatementTemp259.call(nativeStatementTemp260,nativeStatementTemp261);}());
([&]()->JSValue{const auto nativeStatementTemp262=buildPlane;const JSValue nativeStatementTemp263;Args nativeStatementTemp264;nativeStatementTemp264.push_back(JSValue("x"));nativeStatementTemp264.push_back(JSValue("z"));nativeStatementTemp264.push_back(JSValue("y"));nativeStatementTemp264.push_back(JSValue(1));nativeStatementTemp264.push_back(([&]()->JSValue{const auto nativeStatementTemp265=JSValue(1);return -nativeStatementTemp265.toNumber();}()));nativeStatementTemp264.push_back(width);nativeStatementTemp264.push_back(depth);nativeStatementTemp264.push_back(([&]()->JSValue{const auto nativeStatementTemp266=height;return -nativeStatementTemp266.toNumber();}()));nativeStatementTemp264.push_back(widthSegments);nativeStatementTemp264.push_back(depthSegments);nativeStatementTemp264.push_back(JSValue(3));return nativeStatementTemp262.call(nativeStatementTemp263,nativeStatementTemp264);}());
([&]()->JSValue{const auto nativeStatementTemp267=buildPlane;const JSValue nativeStatementTemp268;Args nativeStatementTemp269;nativeStatementTemp269.push_back(JSValue("x"));nativeStatementTemp269.push_back(JSValue("y"));nativeStatementTemp269.push_back(JSValue("z"));nativeStatementTemp269.push_back(JSValue(1));nativeStatementTemp269.push_back(([&]()->JSValue{const auto nativeStatementTemp270=JSValue(1);return -nativeStatementTemp270.toNumber();}()));nativeStatementTemp269.push_back(width);nativeStatementTemp269.push_back(height);nativeStatementTemp269.push_back(depth);nativeStatementTemp269.push_back(widthSegments);nativeStatementTemp269.push_back(heightSegments);nativeStatementTemp269.push_back(JSValue(4));return nativeStatementTemp267.call(nativeStatementTemp268,nativeStatementTemp269);}());
([&]()->JSValue{const auto nativeStatementTemp271=buildPlane;const JSValue nativeStatementTemp272;Args nativeStatementTemp273;nativeStatementTemp273.push_back(JSValue("x"));nativeStatementTemp273.push_back(JSValue("y"));nativeStatementTemp273.push_back(JSValue("z"));nativeStatementTemp273.push_back(([&]()->JSValue{const auto nativeStatementTemp274=JSValue(1);return -nativeStatementTemp274.toNumber();}()));nativeStatementTemp273.push_back(([&]()->JSValue{const auto nativeStatementTemp275=JSValue(1);return -nativeStatementTemp275.toNumber();}()));nativeStatementTemp273.push_back(width);nativeStatementTemp273.push_back(height);nativeStatementTemp273.push_back(([&]()->JSValue{const auto nativeStatementTemp276=depth;return -nativeStatementTemp276.toNumber();}()));nativeStatementTemp273.push_back(widthSegments);nativeStatementTemp273.push_back(heightSegments);nativeStatementTemp273.push_back(JSValue(5));return nativeStatementTemp271.call(nativeStatementTemp272,nativeStatementTemp273);}());
([&]()->JSValue{auto nativeStatementTemp280=nativeStatementTemp4;const auto nativeStatementTemp281=JSValue("setIndex");const auto nativeStatementTemp277=nativeStatementTemp280.get(nativeStatementTemp281);const auto nativeStatementTemp278=nativeStatementTemp280;Args nativeStatementTemp279;nativeStatementTemp279.push_back(readLexical(nativeStatementTemp10));return nativeStatementTemp277.call(nativeStatementTemp278,nativeStatementTemp279);}());
([&]()->JSValue{auto nativeStatementTemp285=nativeStatementTemp4;const auto nativeStatementTemp286=JSValue("setAttribute");const auto nativeStatementTemp282=nativeStatementTemp285.get(nativeStatementTemp286);const auto nativeStatementTemp283=nativeStatementTemp285;Args nativeStatementTemp284;nativeStatementTemp284.push_back(JSValue("position"));nativeStatementTemp284.push_back(([&]()->JSValue{const auto nativeStatementTemp287=nativeStatementTemp3.get("Float32BufferAttribute");Args nativeStatementTemp288;nativeStatementTemp288.push_back(readLexical(nativeStatementTemp6));nativeStatementTemp288.push_back(JSValue(3));return nativeStatementTemp287.construct(nativeStatementTemp288);}()));return nativeStatementTemp282.call(nativeStatementTemp283,nativeStatementTemp284);}());
([&]()->JSValue{auto nativeStatementTemp292=nativeStatementTemp4;const auto nativeStatementTemp293=JSValue("setAttribute");const auto nativeStatementTemp289=nativeStatementTemp292.get(nativeStatementTemp293);const auto nativeStatementTemp290=nativeStatementTemp292;Args nativeStatementTemp291;nativeStatementTemp291.push_back(JSValue("normal"));nativeStatementTemp291.push_back(([&]()->JSValue{const auto nativeStatementTemp294=nativeStatementTemp3.get("Float32BufferAttribute");Args nativeStatementTemp295;nativeStatementTemp295.push_back(readLexical(nativeStatementTemp7));nativeStatementTemp295.push_back(JSValue(3));return nativeStatementTemp294.construct(nativeStatementTemp295);}()));return nativeStatementTemp289.call(nativeStatementTemp290,nativeStatementTemp291);}());
([&]()->JSValue{auto nativeStatementTemp299=nativeStatementTemp4;const auto nativeStatementTemp300=JSValue("setAttribute");const auto nativeStatementTemp296=nativeStatementTemp299.get(nativeStatementTemp300);const auto nativeStatementTemp297=nativeStatementTemp299;Args nativeStatementTemp298;nativeStatementTemp298.push_back(JSValue("uv"));nativeStatementTemp298.push_back(([&]()->JSValue{const auto nativeStatementTemp301=nativeStatementTemp3.get("Float32BufferAttribute");Args nativeStatementTemp302;nativeStatementTemp302.push_back(readLexical(nativeStatementTemp8));nativeStatementTemp302.push_back(JSValue(2));return nativeStatementTemp301.construct(nativeStatementTemp302);}()));return nativeStatementTemp296.call(nativeStatementTemp297,nativeStatementTemp298);}());
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp303,const Args& nativeStatementTemp304,const Args& nativeStatementTemp305){
auto nativeStatementTemp306=nativeStatementTemp305[0];auto nativeStatementTemp307=nativeStatementTemp303;
JSValue source=arg(nativeStatementTemp304,0);
([&]()->JSValue{const auto nativeStatementTemp309=nativeStatementTemp305[1].getPrototype();const auto nativeStatementTemp310=JSValue("copy");const auto nativeStatementTemp311=nativeStatementTemp309.get(nativeStatementTemp310,nativeStatementTemp307);Args nativeStatementTemp312;nativeStatementTemp312.push_back(source);return nativeStatementTemp311.call(nativeStatementTemp307,nativeStatementTemp312);}());
([&]()->JSValue{auto nativeStatementTemp313=nativeStatementTemp307;const auto nativeStatementTemp314=JSValue("parameters");const auto nativeStatementTemp315=([&]()->JSValue{auto nativeStatementTemp321=nativeGlobalBinding("Object");const auto nativeStatementTemp322=JSValue("assign");const auto nativeStatementTemp318=nativeStatementTemp321.get(nativeStatementTemp322);const auto nativeStatementTemp319=nativeStatementTemp321;Args nativeStatementTemp320;nativeStatementTemp320.push_back(([&]()->JSValue{auto nativeStatementTemp323=JSValue::object();return nativeStatementTemp323;}()));nativeStatementTemp320.push_back(([&]()->JSValue{const auto nativeStatementTemp324=source;const auto nativeStatementTemp325=JSValue("parameters");return nativeStatementTemp324.get(nativeStatementTemp325);}()));return nativeStatementTemp318.call(nativeStatementTemp319,nativeStatementTemp320);}());setReference(nativeStatementTemp313,nativeStatementTemp314,nativeStatementTemp315);return nativeStatementTemp315;}());
return nativeStatementTemp307;
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp326,const Args& nativeStatementTemp327,const Args& nativeStatementTemp328){
auto nativeStatementTemp329=nativeStatementTemp328[0];auto nativeStatementTemp330=nativeStatementTemp326;
JSValue data=arg(nativeStatementTemp327,0);
return ([&]()->JSValue{const auto nativeStatementTemp332=nativeStatementTemp329.get("BoxGeometry");Args nativeStatementTemp333;nativeStatementTemp333.push_back(([&]()->JSValue{const auto nativeStatementTemp334=data;const auto nativeStatementTemp335=JSValue("width");return nativeStatementTemp334.get(nativeStatementTemp335);}()));nativeStatementTemp333.push_back(([&]()->JSValue{const auto nativeStatementTemp336=data;const auto nativeStatementTemp337=JSValue("height");return nativeStatementTemp336.get(nativeStatementTemp337);}()));nativeStatementTemp333.push_back(([&]()->JSValue{const auto nativeStatementTemp338=data;const auto nativeStatementTemp339=JSValue("depth");return nativeStatementTemp338.get(nativeStatementTemp339);}()));nativeStatementTemp333.push_back(([&]()->JSValue{const auto nativeStatementTemp340=data;const auto nativeStatementTemp341=JSValue("widthSegments");return nativeStatementTemp340.get(nativeStatementTemp341);}()));nativeStatementTemp333.push_back(([&]()->JSValue{const auto nativeStatementTemp342=data;const auto nativeStatementTemp343=JSValue("heightSegments");return nativeStatementTemp342.get(nativeStatementTemp343);}()));nativeStatementTemp333.push_back(([&]()->JSValue{const auto nativeStatementTemp344=data;const auto nativeStatementTemp345=JSValue("depthSegments");return nativeStatementTemp344.get(nativeStatementTemp345);}()));return nativeStatementTemp332.construct(nativeStatementTemp333);}());
return JSValue{};
}
}
JSValue nativeBoxGeometryModule(){using namespace BoxGeometryManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject()){const auto status=cached->getInternalSlot("status");if(status.strictEquals("evaluated"))return cached->getInternalSlot("facade");if(status.strictEquals("failed"))throw JSThrownValue(cached->getInternalSlot("error"));throw JSModuleInitializationError("BoxGeometry module");}auto module=JSValue::object(nullptr);module.retainForHeapLifetime();weak=JSWeakValue(module);lexical()=weak;module.setInternalSlot("status","initializing");try{
own(module,"BufferGeometry",nativeBufferGeometryConstructor());
own(module,"Float32BufferAttribute",nativeFloat32BufferAttributeConstructor());
own(module,"Vector3",nativeVector3Constructor());
auto ctor=JSValue::builtinConstructor(method0,{module},"BoxGeometry",0);ctor.setFunctionSourceText(BoxGeometryRuntimeSource::classSource);ctor.setPrototype(module.get("BufferGeometry"));auto prototype=ctor.get("prototype");prototype.setPrototype(module.get("BufferGeometry").get("prototype"));own(module,"BoxGeometry",ctor);
{auto home=prototype;auto fn=JSValue::function(method1,{module,home},"copy",1);fn.setFunctionSourceText(BoxGeometryRuntimeSource::method1);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;home.defineProperty("copy",d);}
{auto home=ctor;auto fn=JSValue::function(method2,{module,home},"fromJSON",1);fn.setFunctionSourceText(BoxGeometryRuntimeSource::method2);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;home.defineProperty("fromJSON",d);}
auto facade=JSValue::object();own(facade,"BoxGeometry",ctor);module.setInternalSlot("facade",facade);module.setInternalSlot("status","evaluated");return facade;}catch(...){module.setInternalSlot("error",nativeExceptionValue(std::current_exception()));module.setInternalSlot("status","failed");throw JSThrownValue(module.getInternalSlot("error"));}}
JSValue nativeBoxGeometryBinding(const std::string& name){const auto keeper=nativeBoxGeometryModule();return BoxGeometryManaged::lexical().lock()->get(name);}
JSValue nativeBoxGeometryConstructor(){return nativeBoxGeometryBinding("BoxGeometry");}
