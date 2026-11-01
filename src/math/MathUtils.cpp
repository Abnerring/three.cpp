#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
#include <vector>
#include "core/JSGlobal.h"
#include "math/GenerateUUIDSource.inc"
#include <cstdint>
#include "math/Quaternion.h"
#include "math/JSNumberMath.h"

namespace MathUtils
{
    static constexpr const char* _lut[] = {
        "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
        "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
        "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
        "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
        "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
        "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
        "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
        "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
        "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
        "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
        "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
        "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
        "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
        "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"};

    static const HIGH_PRECISION DEG2RAD = MATH_PI / 180.0;
    static const HIGH_PRECISION RAD2DEG = 180.0 / MATH_PI;
    static const HIGH_PRECISION LN2 = 0.6931471805599453;  // JSNumberMath::log(2.0)

    // http://stackoverflow.com/questions/105034/how-to-create-a-guid-uuid-in-javascript/21963136#21963136
    std::string generateUUID()
    {
        const uint32_t d0 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d1 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d2 = DIST_UINT(RANDOM_ENGINE);
        const uint32_t d3 = DIST_UINT(RANDOM_ENGINE);

        std::string uuid =
            std::string(_lut[d0 & 0xFF]) +
            _lut[(d0 >> 8) & 0xFF] +
            _lut[(d0 >> 16) & 0xFF] +
            _lut[(d0 >> 24) & 0xFF] + '-' +

            _lut[d1 & 0xFF] +
            _lut[(d1 >> 8) & 0xFF] + '-' +
            _lut[((d1 >> 16) & 0x0F) | 0x40] +
            _lut[(d1 >> 24) & 0xFF] + '-' +

            _lut[(d2 & 0x3F) | 0x80] +
            _lut[(d2 >> 8) & 0xFF] + '-' +

            _lut[(d2 >> 16) & 0xFF] +
            _lut[(d2 >> 24) & 0xFF] +
            _lut[d3 & 0xFF] +
            _lut[(d3 >> 8) & 0xFF] +
            _lut[(d3 >> 16) & 0xFF] +
            _lut[(d3 >> 24) & 0xFF];

        // .toLowerCase() here flattens concatenated strings to save heap memory space.
        std::transform(uuid.begin(), uuid.end(), uuid.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });

        return uuid;
    }

    HIGH_PRECISION clamp(HIGH_PRECISION value, HIGH_PRECISION min, HIGH_PRECISION max)
    {
        return MathUtils::max(min, MathUtils::min(max, value));
    }

    // compute euclidean modulo of m % n
    // https://en.wikipedia.org/wiki/Modulo_operation
    HIGH_PRECISION euclideanModulo(HIGH_PRECISION n, HIGH_PRECISION m)
    {
        return std::fmod(std::fmod(n, m) + m, m);
    }

    // Linear mapping from range <a1, a2> to range <b1, b2>
    HIGH_PRECISION mapLinear(HIGH_PRECISION x, HIGH_PRECISION a1, HIGH_PRECISION a2, HIGH_PRECISION b1, HIGH_PRECISION b2)
    {
        return b1 + (x - a1) * (b2 - b1) / (a2 - a1);
    }

    // https://www.gamedev.net/tutorials/programming/general-and-gameplay-programming/inverse-lerp-a-super-useful-yet-often-overlooked-function-r5230/
    HIGH_PRECISION inverseLerp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION value)
    {
        if (x != y)
        {

            return (value - x) / (y - x);
        }
        else
        {

            return 0.0;
        }
    }

    // https://en.wikipedia.org/wiki/Linear_interpolation
    HIGH_PRECISION lerp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION t)
    {
        return (1 - t) * x + t * y;
    }

    // http://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/
    HIGH_PRECISION damp(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION lambda, HIGH_PRECISION dt)
    {
        return lerp(x, y, 1 - JSNumberMath::exp(-lambda * dt));
    }

    // https://www.desmos.com/calculator/vcsjnyz7x4
    HIGH_PRECISION pingpong(HIGH_PRECISION x, HIGH_PRECISION length)
    {
        return length - std::abs(euclideanModulo(x, length * 2) - length);
    }

    // http://en.wikipedia.org/wiki/Smoothstep
    HIGH_PRECISION smoothstep(HIGH_PRECISION x, HIGH_PRECISION min, HIGH_PRECISION max)
    {
        if (x <= min)
            return 0;
        if (x >= max)
            return 1;

        x = (x - min) / (max - min);

        return x * x * (3 - 2 * x);
    }

    HIGH_PRECISION smootherstep(HIGH_PRECISION x, HIGH_PRECISION min, HIGH_PRECISION max)
    {
        if (x <= min)
            return 0;
        if (x >= max)
            return 1;

        x = (x - min) / (max - min);

        return x * x * x * (x * (x * 6 - 15) + 10);
    }

    // Random integer from <low, high> interval
    int randInt(int low, int high)
    {
        return low + std::floor(DIST_DOUBLE(RANDOM_ENGINE) * (high - low + 1));
    }

    // Random HIGH_PRECISION from <low, high> interval
    HIGH_PRECISION randFloat(HIGH_PRECISION low, HIGH_PRECISION high)
    {
        return low + (HIGH_PRECISION)DIST_DOUBLE(RANDOM_ENGINE) * (high - low);
    }

    // Random HIGH_PRECISION from <-range/2, range/2> interval
    HIGH_PRECISION randFloatSpread(HIGH_PRECISION range)
    {
        return range * (0.5 - (HIGH_PRECISION)DIST_DOUBLE(RANDOM_ENGINE));
    }

    // Deterministic pseudo-random HIGH_PRECISION in the interval [ 0, 1 ]
    HIGH_PRECISION seededRandom(std::optional<uint32_t> seed)
    {
        static uint32_t state = 1234567;
        if (seed) state = *seed;
        uint32_t t = (state += 0x6D2B79F5u);
        t = (t ^ (t >> 15)) * (t | 1u);
        t ^= t + (t ^ (t >> 7)) * (t | 61u);
        return static_cast<HIGH_PRECISION>(t ^ (t >> 14)) / 4294967296.0;
    }

    HIGH_PRECISION degToRad(HIGH_PRECISION degrees)
    {
        return degrees * DEG2RAD;
    }

    HIGH_PRECISION radToDeg(HIGH_PRECISION radians)
    {
        return radians * RAD2DEG;
    }

    bool isPowerOfTwo(HIGH_PRECISION value)
    {
        return std::isfinite(value) && value > 0 && std::floor(value) == value && JSNumberMath::pow(2.0, MathUtils::round(JSNumberMath::log2(value))) == value;
    }

    HIGH_PRECISION ceilPowerOfTwo(HIGH_PRECISION value)
    {
        return JSNumberMath::pow(2, std::ceil(JSNumberMath::log(value) / LN2));
    }

    HIGH_PRECISION floorPowerOfTwo(HIGH_PRECISION value)
    {
        return JSNumberMath::pow(2, std::floor(JSNumberMath::log(value) / LN2));
    }

    HIGH_PRECISION round(HIGH_PRECISION value)
    {
        if (!std::isfinite(value) || value == 0) return value;
        if (value >= -0.5 && value < 0) return -0.0;
        const auto lower = std::floor(value);
        return (value - lower < 0.5) ? lower : lower + 1;
    }


    HIGH_PRECISION min(HIGH_PRECISION a, HIGH_PRECISION b)
    {
        if (std::isnan(a) || std::isnan(b)) return std::numeric_limits<HIGH_PRECISION>::quiet_NaN();
        if (a == 0 && b == 0) return std::signbit(a) || std::signbit(b) ? -0.0 : 0.0;
        return a < b ? a : b;
    }
    HIGH_PRECISION max(HIGH_PRECISION a, HIGH_PRECISION b)
    {
        if (std::isnan(a) || std::isnan(b)) return std::numeric_limits<HIGH_PRECISION>::quiet_NaN();
        if (a == 0 && b == 0) return !std::signbit(a) || !std::signbit(b) ? 0.0 : -0.0;
        return a > b ? a : b;
    }


    // Matrix3 span to vector Matrix3
    std::vector<HIGH_PRECISION> convertSpanToVector(const std::span<const HIGH_PRECISION, 9> &src_span)
    {
        return std::vector<HIGH_PRECISION>(src_span.begin(), src_span.end());
    }

    // Matrix4 span to vector Matrix3
    std::vector<HIGH_PRECISION> convertSpanToVector(const std::span<const HIGH_PRECISION, 16> &src_span)
    {
        return std::vector<HIGH_PRECISION>(src_span.begin(), src_span.end());
    }

    void setQuaternionFromProperEuler(Quaternion &q, HIGH_PRECISION a, HIGH_PRECISION b, HIGH_PRECISION c, const std::string &order)
    {





	const auto c2 = JSNumberMath::cos( b / 2 );
	const auto s2 = JSNumberMath::sin( b / 2 );

	const auto c13 = JSNumberMath::cos( ( a + c ) / 2 );
	const auto s13 = JSNumberMath::sin( ( a + c ) / 2 );

	const auto c1_3 = JSNumberMath::cos( ( a - c ) / 2 );
	const auto s1_3 = JSNumberMath::sin( ( a - c ) / 2 );

	const auto c3_1 = JSNumberMath::cos( ( c - a ) / 2 );
	const auto s3_1 = JSNumberMath::sin( ( c - a ) / 2 );

	        if (order == "XYX") { q.set( c2 * s13, s2 * c1_3, s2 * s1_3, c2 * c13 ); }
        else if (order == "YZY") { q.set( s2 * s1_3, c2 * s13, s2 * c1_3, c2 * c13 ); }
        else if (order == "ZXZ") { q.set( s2 * c1_3, s2 * s1_3, c2 * s13, c2 * c13 ); }
        else if (order == "XZX") { q.set( c2 * s13, s2 * s3_1, s2 * c3_1, c2 * c13 ); }
        else if (order == "YXY") { q.set( s2 * c3_1, c2 * s13, s2 * s3_1, c2 * c13 ); }
        else if (order == "ZYZ") { q.set( s2 * s3_1, s2 * c3_1, c2 * s13, c2 * c13 ); }
    }
}


// Complete native managed MathUtils module; pinned source, see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "utils.h"
#include <bit>
#include <cmath>
#include <unordered_set>
#include <string_view>
namespace FullMathUtilsManaged{

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot assign property of null/undefined");const auto property=key.toPropertyKey();if(base.isObject()){base.set(property,value);return;}auto object=base.toObject();if(!object.reflectSet(property,value,base))throw JSTypeError("Cannot assign primitive property");}
JSValue deleteReference(JSValue base,const JSValue& key){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot delete property of null/undefined");const auto property=key.toPropertyKey();auto object=base.toObject();if(!object.erase(property))throw JSTypeError("Cannot delete nonconfigurable property");return true;}
JSValue argumentObject(const Args& args){auto object=JSValue::object();JSPropertyDescriptor length;length.value=args.size();length.writable=length.configurable=true;object.defineProperty("length",length);for(unsigned i=0;i<args.size();++i)own(object,JSValue(i),args[i]);return object;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}

std::uint32_t uint32(double value){if(!std::isfinite(value)||value==0)return 0;auto n=std::fmod(std::trunc(value),4294967296.0);if(n<0)n+=4294967296.0;return static_cast<std::uint32_t>(n);}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="in")return right.has(left);if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive();const auto b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString();const auto y=b.toString();return x+y;}const auto x=a.toNumber();const auto y=b.toNumber();return x+y;}
 if(op=="<"||op==">"||op=="<="||op==">="){const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}const auto x=a.toNumber();const auto y=b.toNumber();if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}
 const auto a=left.toNumber();const auto b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);if(op=="%")return std::fmod(a,b);if(op=="&")return std::bit_cast<std::int32_t>(uint32(a)&uint32(b));if(op=="|")return std::bit_cast<std::int32_t>(uint32(a)|uint32(b));if(op=="^")return std::bit_cast<std::int32_t>(uint32(a)^uint32(b));if(op==">")return a>b;if(op==">>")return std::bit_cast<std::int32_t>(uint32(a))>>(uint32(b)&31);if(op==">>>")return uint32(a)>>(uint32(b)&31);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue function0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
JSValue d0=([&]()->JSValue{const auto nativeStatementTemp6=([&]()->JSValue{const auto nativeStatementTemp8=([&]()->JSValue{auto nativeStatementTemp13=nativeGlobalBinding("Math");const auto nativeStatementTemp14=JSValue("random");const auto nativeStatementTemp10=nativeStatementTemp13.get(nativeStatementTemp14);const auto nativeStatementTemp11=nativeStatementTemp13;Args nativeStatementTemp12;return nativeStatementTemp10.call(nativeStatementTemp11,nativeStatementTemp12);}());const auto nativeStatementTemp9=JSValue(4294967295);return binary("*",nativeStatementTemp8,nativeStatementTemp9);}());const auto nativeStatementTemp7=JSValue(0);return binary("|",nativeStatementTemp6,nativeStatementTemp7);}());
JSValue d1=([&]()->JSValue{const auto nativeStatementTemp15=([&]()->JSValue{const auto nativeStatementTemp17=([&]()->JSValue{auto nativeStatementTemp22=nativeGlobalBinding("Math");const auto nativeStatementTemp23=JSValue("random");const auto nativeStatementTemp19=nativeStatementTemp22.get(nativeStatementTemp23);const auto nativeStatementTemp20=nativeStatementTemp22;Args nativeStatementTemp21;return nativeStatementTemp19.call(nativeStatementTemp20,nativeStatementTemp21);}());const auto nativeStatementTemp18=JSValue(4294967295);return binary("*",nativeStatementTemp17,nativeStatementTemp18);}());const auto nativeStatementTemp16=JSValue(0);return binary("|",nativeStatementTemp15,nativeStatementTemp16);}());
JSValue d2=([&]()->JSValue{const auto nativeStatementTemp24=([&]()->JSValue{const auto nativeStatementTemp26=([&]()->JSValue{auto nativeStatementTemp31=nativeGlobalBinding("Math");const auto nativeStatementTemp32=JSValue("random");const auto nativeStatementTemp28=nativeStatementTemp31.get(nativeStatementTemp32);const auto nativeStatementTemp29=nativeStatementTemp31;Args nativeStatementTemp30;return nativeStatementTemp28.call(nativeStatementTemp29,nativeStatementTemp30);}());const auto nativeStatementTemp27=JSValue(4294967295);return binary("*",nativeStatementTemp26,nativeStatementTemp27);}());const auto nativeStatementTemp25=JSValue(0);return binary("|",nativeStatementTemp24,nativeStatementTemp25);}());
JSValue d3=([&]()->JSValue{const auto nativeStatementTemp33=([&]()->JSValue{const auto nativeStatementTemp35=([&]()->JSValue{auto nativeStatementTemp40=nativeGlobalBinding("Math");const auto nativeStatementTemp41=JSValue("random");const auto nativeStatementTemp37=nativeStatementTemp40.get(nativeStatementTemp41);const auto nativeStatementTemp38=nativeStatementTemp40;Args nativeStatementTemp39;return nativeStatementTemp37.call(nativeStatementTemp38,nativeStatementTemp39);}());const auto nativeStatementTemp36=JSValue(4294967295);return binary("*",nativeStatementTemp35,nativeStatementTemp36);}());const auto nativeStatementTemp34=JSValue(0);return binary("|",nativeStatementTemp33,nativeStatementTemp34);}());
JSValue uuid=([&]()->JSValue{const auto nativeStatementTemp42=([&]()->JSValue{const auto nativeStatementTemp44=([&]()->JSValue{const auto nativeStatementTemp46=([&]()->JSValue{const auto nativeStatementTemp48=([&]()->JSValue{const auto nativeStatementTemp50=([&]()->JSValue{const auto nativeStatementTemp52=([&]()->JSValue{const auto nativeStatementTemp54=([&]()->JSValue{const auto nativeStatementTemp56=([&]()->JSValue{const auto nativeStatementTemp58=([&]()->JSValue{const auto nativeStatementTemp60=([&]()->JSValue{const auto nativeStatementTemp62=([&]()->JSValue{const auto nativeStatementTemp64=([&]()->JSValue{const auto nativeStatementTemp66=([&]()->JSValue{const auto nativeStatementTemp68=([&]()->JSValue{const auto nativeStatementTemp70=([&]()->JSValue{const auto nativeStatementTemp72=([&]()->JSValue{const auto nativeStatementTemp74=([&]()->JSValue{const auto nativeStatementTemp76=([&]()->JSValue{const auto nativeStatementTemp78=([&]()->JSValue{const auto nativeStatementTemp80=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp81=([&]()->JSValue{const auto nativeStatementTemp82=d0;const auto nativeStatementTemp83=JSValue(255);return binary("&",nativeStatementTemp82,nativeStatementTemp83);}());return nativeStatementTemp80.get(nativeStatementTemp81);}());const auto nativeStatementTemp79=([&]()->JSValue{const auto nativeStatementTemp84=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp85=([&]()->JSValue{const auto nativeStatementTemp86=([&]()->JSValue{const auto nativeStatementTemp88=d0;const auto nativeStatementTemp89=JSValue(8);return binary(">>",nativeStatementTemp88,nativeStatementTemp89);}());const auto nativeStatementTemp87=JSValue(255);return binary("&",nativeStatementTemp86,nativeStatementTemp87);}());return nativeStatementTemp84.get(nativeStatementTemp85);}());return binary("+",nativeStatementTemp78,nativeStatementTemp79);}());const auto nativeStatementTemp77=([&]()->JSValue{const auto nativeStatementTemp90=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp91=([&]()->JSValue{const auto nativeStatementTemp92=([&]()->JSValue{const auto nativeStatementTemp94=d0;const auto nativeStatementTemp95=JSValue(16);return binary(">>",nativeStatementTemp94,nativeStatementTemp95);}());const auto nativeStatementTemp93=JSValue(255);return binary("&",nativeStatementTemp92,nativeStatementTemp93);}());return nativeStatementTemp90.get(nativeStatementTemp91);}());return binary("+",nativeStatementTemp76,nativeStatementTemp77);}());const auto nativeStatementTemp75=([&]()->JSValue{const auto nativeStatementTemp96=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp97=([&]()->JSValue{const auto nativeStatementTemp98=([&]()->JSValue{const auto nativeStatementTemp100=d0;const auto nativeStatementTemp101=JSValue(24);return binary(">>",nativeStatementTemp100,nativeStatementTemp101);}());const auto nativeStatementTemp99=JSValue(255);return binary("&",nativeStatementTemp98,nativeStatementTemp99);}());return nativeStatementTemp96.get(nativeStatementTemp97);}());return binary("+",nativeStatementTemp74,nativeStatementTemp75);}());const auto nativeStatementTemp73=JSValue("-");return binary("+",nativeStatementTemp72,nativeStatementTemp73);}());const auto nativeStatementTemp71=([&]()->JSValue{const auto nativeStatementTemp102=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp103=([&]()->JSValue{const auto nativeStatementTemp104=d1;const auto nativeStatementTemp105=JSValue(255);return binary("&",nativeStatementTemp104,nativeStatementTemp105);}());return nativeStatementTemp102.get(nativeStatementTemp103);}());return binary("+",nativeStatementTemp70,nativeStatementTemp71);}());const auto nativeStatementTemp69=([&]()->JSValue{const auto nativeStatementTemp106=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp107=([&]()->JSValue{const auto nativeStatementTemp108=([&]()->JSValue{const auto nativeStatementTemp110=d1;const auto nativeStatementTemp111=JSValue(8);return binary(">>",nativeStatementTemp110,nativeStatementTemp111);}());const auto nativeStatementTemp109=JSValue(255);return binary("&",nativeStatementTemp108,nativeStatementTemp109);}());return nativeStatementTemp106.get(nativeStatementTemp107);}());return binary("+",nativeStatementTemp68,nativeStatementTemp69);}());const auto nativeStatementTemp67=JSValue("-");return binary("+",nativeStatementTemp66,nativeStatementTemp67);}());const auto nativeStatementTemp65=([&]()->JSValue{const auto nativeStatementTemp112=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp113=([&]()->JSValue{const auto nativeStatementTemp114=([&]()->JSValue{const auto nativeStatementTemp116=([&]()->JSValue{const auto nativeStatementTemp118=d1;const auto nativeStatementTemp119=JSValue(16);return binary(">>",nativeStatementTemp118,nativeStatementTemp119);}());const auto nativeStatementTemp117=JSValue(15);return binary("&",nativeStatementTemp116,nativeStatementTemp117);}());const auto nativeStatementTemp115=JSValue(64);return binary("|",nativeStatementTemp114,nativeStatementTemp115);}());return nativeStatementTemp112.get(nativeStatementTemp113);}());return binary("+",nativeStatementTemp64,nativeStatementTemp65);}());const auto nativeStatementTemp63=([&]()->JSValue{const auto nativeStatementTemp120=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp121=([&]()->JSValue{const auto nativeStatementTemp122=([&]()->JSValue{const auto nativeStatementTemp124=d1;const auto nativeStatementTemp125=JSValue(24);return binary(">>",nativeStatementTemp124,nativeStatementTemp125);}());const auto nativeStatementTemp123=JSValue(255);return binary("&",nativeStatementTemp122,nativeStatementTemp123);}());return nativeStatementTemp120.get(nativeStatementTemp121);}());return binary("+",nativeStatementTemp62,nativeStatementTemp63);}());const auto nativeStatementTemp61=JSValue("-");return binary("+",nativeStatementTemp60,nativeStatementTemp61);}());const auto nativeStatementTemp59=([&]()->JSValue{const auto nativeStatementTemp126=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp127=([&]()->JSValue{const auto nativeStatementTemp128=([&]()->JSValue{const auto nativeStatementTemp130=d2;const auto nativeStatementTemp131=JSValue(63);return binary("&",nativeStatementTemp130,nativeStatementTemp131);}());const auto nativeStatementTemp129=JSValue(128);return binary("|",nativeStatementTemp128,nativeStatementTemp129);}());return nativeStatementTemp126.get(nativeStatementTemp127);}());return binary("+",nativeStatementTemp58,nativeStatementTemp59);}());const auto nativeStatementTemp57=([&]()->JSValue{const auto nativeStatementTemp132=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp133=([&]()->JSValue{const auto nativeStatementTemp134=([&]()->JSValue{const auto nativeStatementTemp136=d2;const auto nativeStatementTemp137=JSValue(8);return binary(">>",nativeStatementTemp136,nativeStatementTemp137);}());const auto nativeStatementTemp135=JSValue(255);return binary("&",nativeStatementTemp134,nativeStatementTemp135);}());return nativeStatementTemp132.get(nativeStatementTemp133);}());return binary("+",nativeStatementTemp56,nativeStatementTemp57);}());const auto nativeStatementTemp55=JSValue("-");return binary("+",nativeStatementTemp54,nativeStatementTemp55);}());const auto nativeStatementTemp53=([&]()->JSValue{const auto nativeStatementTemp138=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp139=([&]()->JSValue{const auto nativeStatementTemp140=([&]()->JSValue{const auto nativeStatementTemp142=d2;const auto nativeStatementTemp143=JSValue(16);return binary(">>",nativeStatementTemp142,nativeStatementTemp143);}());const auto nativeStatementTemp141=JSValue(255);return binary("&",nativeStatementTemp140,nativeStatementTemp141);}());return nativeStatementTemp138.get(nativeStatementTemp139);}());return binary("+",nativeStatementTemp52,nativeStatementTemp53);}());const auto nativeStatementTemp51=([&]()->JSValue{const auto nativeStatementTemp144=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp145=([&]()->JSValue{const auto nativeStatementTemp146=([&]()->JSValue{const auto nativeStatementTemp148=d2;const auto nativeStatementTemp149=JSValue(24);return binary(">>",nativeStatementTemp148,nativeStatementTemp149);}());const auto nativeStatementTemp147=JSValue(255);return binary("&",nativeStatementTemp146,nativeStatementTemp147);}());return nativeStatementTemp144.get(nativeStatementTemp145);}());return binary("+",nativeStatementTemp50,nativeStatementTemp51);}());const auto nativeStatementTemp49=([&]()->JSValue{const auto nativeStatementTemp150=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp151=([&]()->JSValue{const auto nativeStatementTemp152=d3;const auto nativeStatementTemp153=JSValue(255);return binary("&",nativeStatementTemp152,nativeStatementTemp153);}());return nativeStatementTemp150.get(nativeStatementTemp151);}());return binary("+",nativeStatementTemp48,nativeStatementTemp49);}());const auto nativeStatementTemp47=([&]()->JSValue{const auto nativeStatementTemp154=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp155=([&]()->JSValue{const auto nativeStatementTemp156=([&]()->JSValue{const auto nativeStatementTemp158=d3;const auto nativeStatementTemp159=JSValue(8);return binary(">>",nativeStatementTemp158,nativeStatementTemp159);}());const auto nativeStatementTemp157=JSValue(255);return binary("&",nativeStatementTemp156,nativeStatementTemp157);}());return nativeStatementTemp154.get(nativeStatementTemp155);}());return binary("+",nativeStatementTemp46,nativeStatementTemp47);}());const auto nativeStatementTemp45=([&]()->JSValue{const auto nativeStatementTemp160=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp161=([&]()->JSValue{const auto nativeStatementTemp162=([&]()->JSValue{const auto nativeStatementTemp164=d3;const auto nativeStatementTemp165=JSValue(16);return binary(">>",nativeStatementTemp164,nativeStatementTemp165);}());const auto nativeStatementTemp163=JSValue(255);return binary("&",nativeStatementTemp162,nativeStatementTemp163);}());return nativeStatementTemp160.get(nativeStatementTemp161);}());return binary("+",nativeStatementTemp44,nativeStatementTemp45);}());const auto nativeStatementTemp43=([&]()->JSValue{const auto nativeStatementTemp166=nativeStatementTemp3.get("_lut");const auto nativeStatementTemp167=([&]()->JSValue{const auto nativeStatementTemp168=([&]()->JSValue{const auto nativeStatementTemp170=d3;const auto nativeStatementTemp171=JSValue(24);return binary(">>",nativeStatementTemp170,nativeStatementTemp171);}());const auto nativeStatementTemp169=JSValue(255);return binary("&",nativeStatementTemp168,nativeStatementTemp169);}());return nativeStatementTemp166.get(nativeStatementTemp167);}());return binary("+",nativeStatementTemp42,nativeStatementTemp43);}());
return ([&]()->JSValue{auto nativeStatementTemp175=uuid;const auto nativeStatementTemp176=JSValue("toLowerCase");const auto nativeStatementTemp172=nativeStatementTemp175.get(nativeStatementTemp176);const auto nativeStatementTemp173=nativeStatementTemp175;Args nativeStatementTemp174;return nativeStatementTemp172.call(nativeStatementTemp173,nativeStatementTemp174);}());
return JSValue{};
}
JSValue function1(const JSValue& nativeStatementTemp177,const Args& nativeStatementTemp178,const Args& nativeStatementTemp179){
auto nativeStatementTemp180=nativeStatementTemp179[0];auto nativeStatementTemp181=nativeStatementTemp177;
JSValue value=arg(nativeStatementTemp178,0);
JSValue min=arg(nativeStatementTemp178,1);
JSValue max=arg(nativeStatementTemp178,2);
return ([&]()->JSValue{auto nativeStatementTemp186=nativeGlobalBinding("Math");const auto nativeStatementTemp187=JSValue("max");const auto nativeStatementTemp183=nativeStatementTemp186.get(nativeStatementTemp187);const auto nativeStatementTemp184=nativeStatementTemp186;Args nativeStatementTemp185;nativeStatementTemp185.push_back(min);nativeStatementTemp185.push_back(([&]()->JSValue{auto nativeStatementTemp191=nativeGlobalBinding("Math");const auto nativeStatementTemp192=JSValue("min");const auto nativeStatementTemp188=nativeStatementTemp191.get(nativeStatementTemp192);const auto nativeStatementTemp189=nativeStatementTemp191;Args nativeStatementTemp190;nativeStatementTemp190.push_back(max);nativeStatementTemp190.push_back(value);return nativeStatementTemp188.call(nativeStatementTemp189,nativeStatementTemp190);}()));return nativeStatementTemp183.call(nativeStatementTemp184,nativeStatementTemp185);}());
return JSValue{};
}
JSValue function2(const JSValue& nativeStatementTemp193,const Args& nativeStatementTemp194,const Args& nativeStatementTemp195){
auto nativeStatementTemp196=nativeStatementTemp195[0];auto nativeStatementTemp197=nativeStatementTemp193;
JSValue n=arg(nativeStatementTemp194,0);
JSValue m=arg(nativeStatementTemp194,1);
return ([&]()->JSValue{const auto nativeStatementTemp199=([&]()->JSValue{const auto nativeStatementTemp201=([&]()->JSValue{const auto nativeStatementTemp203=n;const auto nativeStatementTemp204=m;return binary("%",nativeStatementTemp203,nativeStatementTemp204);}());const auto nativeStatementTemp202=m;return binary("+",nativeStatementTemp201,nativeStatementTemp202);}());const auto nativeStatementTemp200=m;return binary("%",nativeStatementTemp199,nativeStatementTemp200);}());
return JSValue{};
}
JSValue function3(const JSValue& nativeStatementTemp205,const Args& nativeStatementTemp206,const Args& nativeStatementTemp207){
auto nativeStatementTemp208=nativeStatementTemp207[0];auto nativeStatementTemp209=nativeStatementTemp205;
JSValue x=arg(nativeStatementTemp206,0);
JSValue a1=arg(nativeStatementTemp206,1);
JSValue a2=arg(nativeStatementTemp206,2);
JSValue b1=arg(nativeStatementTemp206,3);
JSValue b2=arg(nativeStatementTemp206,4);
return ([&]()->JSValue{const auto nativeStatementTemp211=b1;const auto nativeStatementTemp212=([&]()->JSValue{const auto nativeStatementTemp213=([&]()->JSValue{const auto nativeStatementTemp215=([&]()->JSValue{const auto nativeStatementTemp217=x;const auto nativeStatementTemp218=a1;return binary("-",nativeStatementTemp217,nativeStatementTemp218);}());const auto nativeStatementTemp216=([&]()->JSValue{const auto nativeStatementTemp219=b2;const auto nativeStatementTemp220=b1;return binary("-",nativeStatementTemp219,nativeStatementTemp220);}());return binary("*",nativeStatementTemp215,nativeStatementTemp216);}());const auto nativeStatementTemp214=([&]()->JSValue{const auto nativeStatementTemp221=a2;const auto nativeStatementTemp222=a1;return binary("-",nativeStatementTemp221,nativeStatementTemp222);}());return binary("/",nativeStatementTemp213,nativeStatementTemp214);}());return binary("+",nativeStatementTemp211,nativeStatementTemp212);}());
return JSValue{};
}
JSValue function4(const JSValue& nativeStatementTemp223,const Args& nativeStatementTemp224,const Args& nativeStatementTemp225){
auto nativeStatementTemp226=nativeStatementTemp225[0];auto nativeStatementTemp227=nativeStatementTemp223;
JSValue x=arg(nativeStatementTemp224,0);
JSValue y=arg(nativeStatementTemp224,1);
JSValue value=arg(nativeStatementTemp224,2);
if(([&]()->JSValue{const auto nativeStatementTemp229=x;const auto nativeStatementTemp230=y;return binary("!==",nativeStatementTemp229,nativeStatementTemp230);}()).toBoolean()){
return ([&]()->JSValue{const auto nativeStatementTemp231=([&]()->JSValue{const auto nativeStatementTemp233=value;const auto nativeStatementTemp234=x;return binary("-",nativeStatementTemp233,nativeStatementTemp234);}());const auto nativeStatementTemp232=([&]()->JSValue{const auto nativeStatementTemp235=y;const auto nativeStatementTemp236=x;return binary("-",nativeStatementTemp235,nativeStatementTemp236);}());return binary("/",nativeStatementTemp231,nativeStatementTemp232);}());

}
else {
return JSValue(0);

}
return JSValue{};
}
JSValue function5(const JSValue& nativeStatementTemp237,const Args& nativeStatementTemp238,const Args& nativeStatementTemp239){
auto nativeStatementTemp240=nativeStatementTemp239[0];auto nativeStatementTemp241=nativeStatementTemp237;
JSValue x=arg(nativeStatementTemp238,0);
JSValue y=arg(nativeStatementTemp238,1);
JSValue t=arg(nativeStatementTemp238,2);
return ([&]()->JSValue{const auto nativeStatementTemp243=([&]()->JSValue{const auto nativeStatementTemp245=([&]()->JSValue{const auto nativeStatementTemp247=JSValue(1);const auto nativeStatementTemp248=t;return binary("-",nativeStatementTemp247,nativeStatementTemp248);}());const auto nativeStatementTemp246=x;return binary("*",nativeStatementTemp245,nativeStatementTemp246);}());const auto nativeStatementTemp244=([&]()->JSValue{const auto nativeStatementTemp249=t;const auto nativeStatementTemp250=y;return binary("*",nativeStatementTemp249,nativeStatementTemp250);}());return binary("+",nativeStatementTemp243,nativeStatementTemp244);}());
return JSValue{};
}
JSValue function6(const JSValue& nativeStatementTemp251,const Args& nativeStatementTemp252,const Args& nativeStatementTemp253){
auto nativeStatementTemp254=nativeStatementTemp253[0];auto nativeStatementTemp255=nativeStatementTemp251;
JSValue x=arg(nativeStatementTemp252,0);
JSValue y=arg(nativeStatementTemp252,1);
JSValue lambda=arg(nativeStatementTemp252,2);
JSValue dt=arg(nativeStatementTemp252,3);
return ([&]()->JSValue{const auto nativeStatementTemp257=nativeStatementTemp254.get("lerp");const JSValue nativeStatementTemp258;Args nativeStatementTemp259;nativeStatementTemp259.push_back(x);nativeStatementTemp259.push_back(y);nativeStatementTemp259.push_back(([&]()->JSValue{const auto nativeStatementTemp260=JSValue(1);const auto nativeStatementTemp261=([&]()->JSValue{auto nativeStatementTemp265=nativeGlobalBinding("Math");const auto nativeStatementTemp266=JSValue("exp");const auto nativeStatementTemp262=nativeStatementTemp265.get(nativeStatementTemp266);const auto nativeStatementTemp263=nativeStatementTemp265;Args nativeStatementTemp264;nativeStatementTemp264.push_back(([&]()->JSValue{const auto nativeStatementTemp267=([&]()->JSValue{const auto nativeStatementTemp269=lambda;return -nativeStatementTemp269.toNumber();}());const auto nativeStatementTemp268=dt;return binary("*",nativeStatementTemp267,nativeStatementTemp268);}()));return nativeStatementTemp262.call(nativeStatementTemp263,nativeStatementTemp264);}());return binary("-",nativeStatementTemp260,nativeStatementTemp261);}()));return nativeStatementTemp257.call(nativeStatementTemp258,nativeStatementTemp259);}());
return JSValue{};
}
JSValue function7(const JSValue& nativeStatementTemp270,const Args& nativeStatementTemp271,const Args& nativeStatementTemp272){
auto nativeStatementTemp273=nativeStatementTemp272[0];auto nativeStatementTemp274=nativeStatementTemp270;
JSValue x=arg(nativeStatementTemp271,0);
JSValue length=arg(nativeStatementTemp271,1);
if(length.isUndefined())length=JSValue(1);
return ([&]()->JSValue{const auto nativeStatementTemp276=length;const auto nativeStatementTemp277=([&]()->JSValue{auto nativeStatementTemp281=nativeGlobalBinding("Math");const auto nativeStatementTemp282=JSValue("abs");const auto nativeStatementTemp278=nativeStatementTemp281.get(nativeStatementTemp282);const auto nativeStatementTemp279=nativeStatementTemp281;Args nativeStatementTemp280;nativeStatementTemp280.push_back(([&]()->JSValue{const auto nativeStatementTemp283=([&]()->JSValue{const auto nativeStatementTemp285=nativeStatementTemp273.get("euclideanModulo");const JSValue nativeStatementTemp286;Args nativeStatementTemp287;nativeStatementTemp287.push_back(x);nativeStatementTemp287.push_back(([&]()->JSValue{const auto nativeStatementTemp288=length;const auto nativeStatementTemp289=JSValue(2);return binary("*",nativeStatementTemp288,nativeStatementTemp289);}()));return nativeStatementTemp285.call(nativeStatementTemp286,nativeStatementTemp287);}());const auto nativeStatementTemp284=length;return binary("-",nativeStatementTemp283,nativeStatementTemp284);}()));return nativeStatementTemp278.call(nativeStatementTemp279,nativeStatementTemp280);}());return binary("-",nativeStatementTemp276,nativeStatementTemp277);}());
return JSValue{};
}
JSValue function8(const JSValue& nativeStatementTemp290,const Args& nativeStatementTemp291,const Args& nativeStatementTemp292){
auto nativeStatementTemp293=nativeStatementTemp292[0];auto nativeStatementTemp294=nativeStatementTemp290;
JSValue x=arg(nativeStatementTemp291,0);
JSValue min=arg(nativeStatementTemp291,1);
JSValue max=arg(nativeStatementTemp291,2);
if(([&]()->JSValue{const auto nativeStatementTemp296=x;const auto nativeStatementTemp297=min;return binary("<=",nativeStatementTemp296,nativeStatementTemp297);}()).toBoolean())return JSValue(0);
if(([&]()->JSValue{const auto nativeStatementTemp298=x;const auto nativeStatementTemp299=max;return binary(">=",nativeStatementTemp298,nativeStatementTemp299);}()).toBoolean())return JSValue(1);
([&]()->JSValue{const auto nativeStatementTemp300=([&]()->JSValue{const auto nativeStatementTemp303=([&]()->JSValue{const auto nativeStatementTemp305=x;const auto nativeStatementTemp306=min;return binary("-",nativeStatementTemp305,nativeStatementTemp306);}());const auto nativeStatementTemp304=([&]()->JSValue{const auto nativeStatementTemp307=max;const auto nativeStatementTemp308=min;return binary("-",nativeStatementTemp307,nativeStatementTemp308);}());return binary("/",nativeStatementTemp303,nativeStatementTemp304);}());x=nativeStatementTemp300;return nativeStatementTemp300;}());
return ([&]()->JSValue{const auto nativeStatementTemp309=([&]()->JSValue{const auto nativeStatementTemp311=x;const auto nativeStatementTemp312=x;return binary("*",nativeStatementTemp311,nativeStatementTemp312);}());const auto nativeStatementTemp310=([&]()->JSValue{const auto nativeStatementTemp313=JSValue(3);const auto nativeStatementTemp314=([&]()->JSValue{const auto nativeStatementTemp315=JSValue(2);const auto nativeStatementTemp316=x;return binary("*",nativeStatementTemp315,nativeStatementTemp316);}());return binary("-",nativeStatementTemp313,nativeStatementTemp314);}());return binary("*",nativeStatementTemp309,nativeStatementTemp310);}());
return JSValue{};
}
JSValue function9(const JSValue& nativeStatementTemp317,const Args& nativeStatementTemp318,const Args& nativeStatementTemp319){
auto nativeStatementTemp320=nativeStatementTemp319[0];auto nativeStatementTemp321=nativeStatementTemp317;
JSValue x=arg(nativeStatementTemp318,0);
JSValue min=arg(nativeStatementTemp318,1);
JSValue max=arg(nativeStatementTemp318,2);
if(([&]()->JSValue{const auto nativeStatementTemp323=x;const auto nativeStatementTemp324=min;return binary("<=",nativeStatementTemp323,nativeStatementTemp324);}()).toBoolean())return JSValue(0);
if(([&]()->JSValue{const auto nativeStatementTemp325=x;const auto nativeStatementTemp326=max;return binary(">=",nativeStatementTemp325,nativeStatementTemp326);}()).toBoolean())return JSValue(1);
([&]()->JSValue{const auto nativeStatementTemp327=([&]()->JSValue{const auto nativeStatementTemp330=([&]()->JSValue{const auto nativeStatementTemp332=x;const auto nativeStatementTemp333=min;return binary("-",nativeStatementTemp332,nativeStatementTemp333);}());const auto nativeStatementTemp331=([&]()->JSValue{const auto nativeStatementTemp334=max;const auto nativeStatementTemp335=min;return binary("-",nativeStatementTemp334,nativeStatementTemp335);}());return binary("/",nativeStatementTemp330,nativeStatementTemp331);}());x=nativeStatementTemp327;return nativeStatementTemp327;}());
return ([&]()->JSValue{const auto nativeStatementTemp336=([&]()->JSValue{const auto nativeStatementTemp338=([&]()->JSValue{const auto nativeStatementTemp340=x;const auto nativeStatementTemp341=x;return binary("*",nativeStatementTemp340,nativeStatementTemp341);}());const auto nativeStatementTemp339=x;return binary("*",nativeStatementTemp338,nativeStatementTemp339);}());const auto nativeStatementTemp337=([&]()->JSValue{const auto nativeStatementTemp342=([&]()->JSValue{const auto nativeStatementTemp344=x;const auto nativeStatementTemp345=([&]()->JSValue{const auto nativeStatementTemp346=([&]()->JSValue{const auto nativeStatementTemp348=x;const auto nativeStatementTemp349=JSValue(6);return binary("*",nativeStatementTemp348,nativeStatementTemp349);}());const auto nativeStatementTemp347=JSValue(15);return binary("-",nativeStatementTemp346,nativeStatementTemp347);}());return binary("*",nativeStatementTemp344,nativeStatementTemp345);}());const auto nativeStatementTemp343=JSValue(10);return binary("+",nativeStatementTemp342,nativeStatementTemp343);}());return binary("*",nativeStatementTemp336,nativeStatementTemp337);}());
return JSValue{};
}
JSValue function10(const JSValue& nativeStatementTemp350,const Args& nativeStatementTemp351,const Args& nativeStatementTemp352){
auto nativeStatementTemp353=nativeStatementTemp352[0];auto nativeStatementTemp354=nativeStatementTemp350;
JSValue low=arg(nativeStatementTemp351,0);
JSValue high=arg(nativeStatementTemp351,1);
return ([&]()->JSValue{const auto nativeStatementTemp356=low;const auto nativeStatementTemp357=([&]()->JSValue{auto nativeStatementTemp361=nativeGlobalBinding("Math");const auto nativeStatementTemp362=JSValue("floor");const auto nativeStatementTemp358=nativeStatementTemp361.get(nativeStatementTemp362);const auto nativeStatementTemp359=nativeStatementTemp361;Args nativeStatementTemp360;nativeStatementTemp360.push_back(([&]()->JSValue{const auto nativeStatementTemp363=([&]()->JSValue{auto nativeStatementTemp368=nativeGlobalBinding("Math");const auto nativeStatementTemp369=JSValue("random");const auto nativeStatementTemp365=nativeStatementTemp368.get(nativeStatementTemp369);const auto nativeStatementTemp366=nativeStatementTemp368;Args nativeStatementTemp367;return nativeStatementTemp365.call(nativeStatementTemp366,nativeStatementTemp367);}());const auto nativeStatementTemp364=([&]()->JSValue{const auto nativeStatementTemp370=([&]()->JSValue{const auto nativeStatementTemp372=high;const auto nativeStatementTemp373=low;return binary("-",nativeStatementTemp372,nativeStatementTemp373);}());const auto nativeStatementTemp371=JSValue(1);return binary("+",nativeStatementTemp370,nativeStatementTemp371);}());return binary("*",nativeStatementTemp363,nativeStatementTemp364);}()));return nativeStatementTemp358.call(nativeStatementTemp359,nativeStatementTemp360);}());return binary("+",nativeStatementTemp356,nativeStatementTemp357);}());
return JSValue{};
}
JSValue function11(const JSValue& nativeStatementTemp374,const Args& nativeStatementTemp375,const Args& nativeStatementTemp376){
auto nativeStatementTemp377=nativeStatementTemp376[0];auto nativeStatementTemp378=nativeStatementTemp374;
JSValue low=arg(nativeStatementTemp375,0);
JSValue high=arg(nativeStatementTemp375,1);
return ([&]()->JSValue{const auto nativeStatementTemp380=low;const auto nativeStatementTemp381=([&]()->JSValue{const auto nativeStatementTemp382=([&]()->JSValue{auto nativeStatementTemp387=nativeGlobalBinding("Math");const auto nativeStatementTemp388=JSValue("random");const auto nativeStatementTemp384=nativeStatementTemp387.get(nativeStatementTemp388);const auto nativeStatementTemp385=nativeStatementTemp387;Args nativeStatementTemp386;return nativeStatementTemp384.call(nativeStatementTemp385,nativeStatementTemp386);}());const auto nativeStatementTemp383=([&]()->JSValue{const auto nativeStatementTemp389=high;const auto nativeStatementTemp390=low;return binary("-",nativeStatementTemp389,nativeStatementTemp390);}());return binary("*",nativeStatementTemp382,nativeStatementTemp383);}());return binary("+",nativeStatementTemp380,nativeStatementTemp381);}());
return JSValue{};
}
JSValue function12(const JSValue& nativeStatementTemp391,const Args& nativeStatementTemp392,const Args& nativeStatementTemp393){
auto nativeStatementTemp394=nativeStatementTemp393[0];auto nativeStatementTemp395=nativeStatementTemp391;
JSValue range=arg(nativeStatementTemp392,0);
return ([&]()->JSValue{const auto nativeStatementTemp397=range;const auto nativeStatementTemp398=([&]()->JSValue{const auto nativeStatementTemp399=JSValue(0.5);const auto nativeStatementTemp400=([&]()->JSValue{auto nativeStatementTemp404=nativeGlobalBinding("Math");const auto nativeStatementTemp405=JSValue("random");const auto nativeStatementTemp401=nativeStatementTemp404.get(nativeStatementTemp405);const auto nativeStatementTemp402=nativeStatementTemp404;Args nativeStatementTemp403;return nativeStatementTemp401.call(nativeStatementTemp402,nativeStatementTemp403);}());return binary("-",nativeStatementTemp399,nativeStatementTemp400);}());return binary("*",nativeStatementTemp397,nativeStatementTemp398);}());
return JSValue{};
}
JSValue function13(const JSValue& nativeStatementTemp406,const Args& nativeStatementTemp407,const Args& nativeStatementTemp408){
auto nativeStatementTemp409=nativeStatementTemp408[0];auto nativeStatementTemp410=nativeStatementTemp406;
JSValue s=arg(nativeStatementTemp407,0);
if(([&]()->JSValue{const auto nativeStatementTemp412=s;const auto nativeStatementTemp413=JSValue{};return binary("!==",nativeStatementTemp412,nativeStatementTemp413);}()).toBoolean())([&]()->JSValue{const auto nativeStatementTemp414=s;nativeStatementTemp409.set("_seed",nativeStatementTemp414);return nativeStatementTemp414;}());
JSValue t=([&]()->JSValue{const auto nativeStatementTemp418=nativeStatementTemp409.get("_seed");const auto nativeStatementTemp419=JSValue(1831565813);const auto nativeStatementTemp417=binary("+",nativeStatementTemp418,nativeStatementTemp419);nativeStatementTemp409.set("_seed",nativeStatementTemp417);return nativeStatementTemp417;}());
([&]()->JSValue{const auto nativeStatementTemp420=([&]()->JSValue{auto nativeStatementTemp426=nativeGlobalBinding("Math");const auto nativeStatementTemp427=JSValue("imul");const auto nativeStatementTemp423=nativeStatementTemp426.get(nativeStatementTemp427);const auto nativeStatementTemp424=nativeStatementTemp426;Args nativeStatementTemp425;nativeStatementTemp425.push_back(([&]()->JSValue{const auto nativeStatementTemp428=t;const auto nativeStatementTemp429=([&]()->JSValue{const auto nativeStatementTemp430=t;const auto nativeStatementTemp431=JSValue(15);return binary(">>>",nativeStatementTemp430,nativeStatementTemp431);}());return binary("^",nativeStatementTemp428,nativeStatementTemp429);}()));nativeStatementTemp425.push_back(([&]()->JSValue{const auto nativeStatementTemp432=t;const auto nativeStatementTemp433=JSValue(1);return binary("|",nativeStatementTemp432,nativeStatementTemp433);}()));return nativeStatementTemp423.call(nativeStatementTemp424,nativeStatementTemp425);}());t=nativeStatementTemp420;return nativeStatementTemp420;}());
([&]()->JSValue{const auto nativeStatementTemp435=t;const auto nativeStatementTemp436=([&]()->JSValue{const auto nativeStatementTemp437=t;const auto nativeStatementTemp438=([&]()->JSValue{auto nativeStatementTemp442=nativeGlobalBinding("Math");const auto nativeStatementTemp443=JSValue("imul");const auto nativeStatementTemp439=nativeStatementTemp442.get(nativeStatementTemp443);const auto nativeStatementTemp440=nativeStatementTemp442;Args nativeStatementTemp441;nativeStatementTemp441.push_back(([&]()->JSValue{const auto nativeStatementTemp444=t;const auto nativeStatementTemp445=([&]()->JSValue{const auto nativeStatementTemp446=t;const auto nativeStatementTemp447=JSValue(7);return binary(">>>",nativeStatementTemp446,nativeStatementTemp447);}());return binary("^",nativeStatementTemp444,nativeStatementTemp445);}()));nativeStatementTemp441.push_back(([&]()->JSValue{const auto nativeStatementTemp448=t;const auto nativeStatementTemp449=JSValue(61);return binary("|",nativeStatementTemp448,nativeStatementTemp449);}()));return nativeStatementTemp439.call(nativeStatementTemp440,nativeStatementTemp441);}());return binary("+",nativeStatementTemp437,nativeStatementTemp438);}());const auto nativeStatementTemp434=binary("^",nativeStatementTemp435,nativeStatementTemp436);t=nativeStatementTemp434;return nativeStatementTemp434;}());
return ([&]()->JSValue{const auto nativeStatementTemp450=([&]()->JSValue{const auto nativeStatementTemp452=([&]()->JSValue{const auto nativeStatementTemp454=t;const auto nativeStatementTemp455=([&]()->JSValue{const auto nativeStatementTemp456=t;const auto nativeStatementTemp457=JSValue(14);return binary(">>>",nativeStatementTemp456,nativeStatementTemp457);}());return binary("^",nativeStatementTemp454,nativeStatementTemp455);}());const auto nativeStatementTemp453=JSValue(0);return binary(">>>",nativeStatementTemp452,nativeStatementTemp453);}());const auto nativeStatementTemp451=JSValue(4294967296);return binary("/",nativeStatementTemp450,nativeStatementTemp451);}());
return JSValue{};
}
JSValue function14(const JSValue& nativeStatementTemp458,const Args& nativeStatementTemp459,const Args& nativeStatementTemp460){
auto nativeStatementTemp461=nativeStatementTemp460[0];auto nativeStatementTemp462=nativeStatementTemp458;
JSValue degrees=arg(nativeStatementTemp459,0);
return ([&]()->JSValue{const auto nativeStatementTemp464=degrees;const auto nativeStatementTemp465=nativeStatementTemp461.get("DEG2RAD");return binary("*",nativeStatementTemp464,nativeStatementTemp465);}());
return JSValue{};
}
JSValue function15(const JSValue& nativeStatementTemp466,const Args& nativeStatementTemp467,const Args& nativeStatementTemp468){
auto nativeStatementTemp469=nativeStatementTemp468[0];auto nativeStatementTemp470=nativeStatementTemp466;
JSValue radians=arg(nativeStatementTemp467,0);
return ([&]()->JSValue{const auto nativeStatementTemp472=radians;const auto nativeStatementTemp473=nativeStatementTemp469.get("RAD2DEG");return binary("*",nativeStatementTemp472,nativeStatementTemp473);}());
return JSValue{};
}
JSValue function16(const JSValue& nativeStatementTemp474,const Args& nativeStatementTemp475,const Args& nativeStatementTemp476){
auto nativeStatementTemp477=nativeStatementTemp476[0];auto nativeStatementTemp478=nativeStatementTemp474;
JSValue value=arg(nativeStatementTemp475,0);
return ([&]()->JSValue{const auto nativeStatementTemp480=([&]()->JSValue{const auto nativeStatementTemp481=([&]()->JSValue{const auto nativeStatementTemp482=value;const auto nativeStatementTemp483=JSValue(0);return binary(">",nativeStatementTemp482,nativeStatementTemp483);}());if(!nativeStatementTemp481.toBoolean())return nativeStatementTemp481;return ([&]()->JSValue{auto nativeStatementTemp487=nativeGlobalBinding("Number");const auto nativeStatementTemp488=JSValue("isInteger");const auto nativeStatementTemp484=nativeStatementTemp487.get(nativeStatementTemp488);const auto nativeStatementTemp485=nativeStatementTemp487;Args nativeStatementTemp486;nativeStatementTemp486.push_back(value);return nativeStatementTemp484.call(nativeStatementTemp485,nativeStatementTemp486);}());}());if(!nativeStatementTemp480.toBoolean())return nativeStatementTemp480;return ([&]()->JSValue{const auto nativeStatementTemp489=([&]()->JSValue{const auto nativeStatementTemp491=JSValue(2);const auto nativeStatementTemp492=([&]()->JSValue{auto nativeStatementTemp496=nativeGlobalBinding("Math");const auto nativeStatementTemp497=JSValue("round");const auto nativeStatementTemp493=nativeStatementTemp496.get(nativeStatementTemp497);const auto nativeStatementTemp494=nativeStatementTemp496;Args nativeStatementTemp495;nativeStatementTemp495.push_back(([&]()->JSValue{auto nativeStatementTemp501=nativeGlobalBinding("Math");const auto nativeStatementTemp502=JSValue("log2");const auto nativeStatementTemp498=nativeStatementTemp501.get(nativeStatementTemp502);const auto nativeStatementTemp499=nativeStatementTemp501;Args nativeStatementTemp500;nativeStatementTemp500.push_back(value);return nativeStatementTemp498.call(nativeStatementTemp499,nativeStatementTemp500);}()));return nativeStatementTemp493.call(nativeStatementTemp494,nativeStatementTemp495);}());return binary("**",nativeStatementTemp491,nativeStatementTemp492);}());const auto nativeStatementTemp490=value;return binary("===",nativeStatementTemp489,nativeStatementTemp490);}());}());
return JSValue{};
}
JSValue function17(const JSValue& nativeStatementTemp503,const Args& nativeStatementTemp504,const Args& nativeStatementTemp505){
auto nativeStatementTemp506=nativeStatementTemp505[0];auto nativeStatementTemp507=nativeStatementTemp503;
JSValue value=arg(nativeStatementTemp504,0);
return ([&]()->JSValue{auto nativeStatementTemp512=nativeGlobalBinding("Math");const auto nativeStatementTemp513=JSValue("pow");const auto nativeStatementTemp509=nativeStatementTemp512.get(nativeStatementTemp513);const auto nativeStatementTemp510=nativeStatementTemp512;Args nativeStatementTemp511;nativeStatementTemp511.push_back(JSValue(2));nativeStatementTemp511.push_back(([&]()->JSValue{auto nativeStatementTemp517=nativeGlobalBinding("Math");const auto nativeStatementTemp518=JSValue("ceil");const auto nativeStatementTemp514=nativeStatementTemp517.get(nativeStatementTemp518);const auto nativeStatementTemp515=nativeStatementTemp517;Args nativeStatementTemp516;nativeStatementTemp516.push_back(([&]()->JSValue{const auto nativeStatementTemp519=([&]()->JSValue{auto nativeStatementTemp524=nativeGlobalBinding("Math");const auto nativeStatementTemp525=JSValue("log");const auto nativeStatementTemp521=nativeStatementTemp524.get(nativeStatementTemp525);const auto nativeStatementTemp522=nativeStatementTemp524;Args nativeStatementTemp523;nativeStatementTemp523.push_back(value);return nativeStatementTemp521.call(nativeStatementTemp522,nativeStatementTemp523);}());const auto nativeStatementTemp520=([&]()->JSValue{const auto nativeStatementTemp526=nativeGlobalBinding("Math");const auto nativeStatementTemp527=JSValue("LN2");return nativeStatementTemp526.get(nativeStatementTemp527);}());return binary("/",nativeStatementTemp519,nativeStatementTemp520);}()));return nativeStatementTemp514.call(nativeStatementTemp515,nativeStatementTemp516);}()));return nativeStatementTemp509.call(nativeStatementTemp510,nativeStatementTemp511);}());
return JSValue{};
}
JSValue function18(const JSValue& nativeStatementTemp528,const Args& nativeStatementTemp529,const Args& nativeStatementTemp530){
auto nativeStatementTemp531=nativeStatementTemp530[0];auto nativeStatementTemp532=nativeStatementTemp528;
JSValue value=arg(nativeStatementTemp529,0);
return ([&]()->JSValue{auto nativeStatementTemp537=nativeGlobalBinding("Math");const auto nativeStatementTemp538=JSValue("pow");const auto nativeStatementTemp534=nativeStatementTemp537.get(nativeStatementTemp538);const auto nativeStatementTemp535=nativeStatementTemp537;Args nativeStatementTemp536;nativeStatementTemp536.push_back(JSValue(2));nativeStatementTemp536.push_back(([&]()->JSValue{auto nativeStatementTemp542=nativeGlobalBinding("Math");const auto nativeStatementTemp543=JSValue("floor");const auto nativeStatementTemp539=nativeStatementTemp542.get(nativeStatementTemp543);const auto nativeStatementTemp540=nativeStatementTemp542;Args nativeStatementTemp541;nativeStatementTemp541.push_back(([&]()->JSValue{const auto nativeStatementTemp544=([&]()->JSValue{auto nativeStatementTemp549=nativeGlobalBinding("Math");const auto nativeStatementTemp550=JSValue("log");const auto nativeStatementTemp546=nativeStatementTemp549.get(nativeStatementTemp550);const auto nativeStatementTemp547=nativeStatementTemp549;Args nativeStatementTemp548;nativeStatementTemp548.push_back(value);return nativeStatementTemp546.call(nativeStatementTemp547,nativeStatementTemp548);}());const auto nativeStatementTemp545=([&]()->JSValue{const auto nativeStatementTemp551=nativeGlobalBinding("Math");const auto nativeStatementTemp552=JSValue("LN2");return nativeStatementTemp551.get(nativeStatementTemp552);}());return binary("/",nativeStatementTemp544,nativeStatementTemp545);}()));return nativeStatementTemp539.call(nativeStatementTemp540,nativeStatementTemp541);}()));return nativeStatementTemp534.call(nativeStatementTemp535,nativeStatementTemp536);}());
return JSValue{};
}
JSValue function19(const JSValue& nativeStatementTemp553,const Args& nativeStatementTemp554,const Args& nativeStatementTemp555){
auto nativeStatementTemp556=nativeStatementTemp555[0];auto nativeStatementTemp557=nativeStatementTemp553;
JSValue q=arg(nativeStatementTemp554,0);
JSValue a=arg(nativeStatementTemp554,1);
JSValue b=arg(nativeStatementTemp554,2);
JSValue c=arg(nativeStatementTemp554,3);
JSValue order=arg(nativeStatementTemp554,4);
JSValue cos=([&]()->JSValue{const auto nativeStatementTemp559=nativeGlobalBinding("Math");const auto nativeStatementTemp560=JSValue("cos");return nativeStatementTemp559.get(nativeStatementTemp560);}());
JSValue sin=([&]()->JSValue{const auto nativeStatementTemp561=nativeGlobalBinding("Math");const auto nativeStatementTemp562=JSValue("sin");return nativeStatementTemp561.get(nativeStatementTemp562);}());
JSValue c2=([&]()->JSValue{const auto nativeStatementTemp563=cos;const JSValue nativeStatementTemp564;Args nativeStatementTemp565;nativeStatementTemp565.push_back(([&]()->JSValue{const auto nativeStatementTemp566=b;const auto nativeStatementTemp567=JSValue(2);return binary("/",nativeStatementTemp566,nativeStatementTemp567);}()));return nativeStatementTemp563.call(nativeStatementTemp564,nativeStatementTemp565);}());
JSValue s2=([&]()->JSValue{const auto nativeStatementTemp568=sin;const JSValue nativeStatementTemp569;Args nativeStatementTemp570;nativeStatementTemp570.push_back(([&]()->JSValue{const auto nativeStatementTemp571=b;const auto nativeStatementTemp572=JSValue(2);return binary("/",nativeStatementTemp571,nativeStatementTemp572);}()));return nativeStatementTemp568.call(nativeStatementTemp569,nativeStatementTemp570);}());
JSValue c13=([&]()->JSValue{const auto nativeStatementTemp573=cos;const JSValue nativeStatementTemp574;Args nativeStatementTemp575;nativeStatementTemp575.push_back(([&]()->JSValue{const auto nativeStatementTemp576=([&]()->JSValue{const auto nativeStatementTemp578=a;const auto nativeStatementTemp579=c;return binary("+",nativeStatementTemp578,nativeStatementTemp579);}());const auto nativeStatementTemp577=JSValue(2);return binary("/",nativeStatementTemp576,nativeStatementTemp577);}()));return nativeStatementTemp573.call(nativeStatementTemp574,nativeStatementTemp575);}());
JSValue s13=([&]()->JSValue{const auto nativeStatementTemp580=sin;const JSValue nativeStatementTemp581;Args nativeStatementTemp582;nativeStatementTemp582.push_back(([&]()->JSValue{const auto nativeStatementTemp583=([&]()->JSValue{const auto nativeStatementTemp585=a;const auto nativeStatementTemp586=c;return binary("+",nativeStatementTemp585,nativeStatementTemp586);}());const auto nativeStatementTemp584=JSValue(2);return binary("/",nativeStatementTemp583,nativeStatementTemp584);}()));return nativeStatementTemp580.call(nativeStatementTemp581,nativeStatementTemp582);}());
JSValue c1_3=([&]()->JSValue{const auto nativeStatementTemp587=cos;const JSValue nativeStatementTemp588;Args nativeStatementTemp589;nativeStatementTemp589.push_back(([&]()->JSValue{const auto nativeStatementTemp590=([&]()->JSValue{const auto nativeStatementTemp592=a;const auto nativeStatementTemp593=c;return binary("-",nativeStatementTemp592,nativeStatementTemp593);}());const auto nativeStatementTemp591=JSValue(2);return binary("/",nativeStatementTemp590,nativeStatementTemp591);}()));return nativeStatementTemp587.call(nativeStatementTemp588,nativeStatementTemp589);}());
JSValue s1_3=([&]()->JSValue{const auto nativeStatementTemp594=sin;const JSValue nativeStatementTemp595;Args nativeStatementTemp596;nativeStatementTemp596.push_back(([&]()->JSValue{const auto nativeStatementTemp597=([&]()->JSValue{const auto nativeStatementTemp599=a;const auto nativeStatementTemp600=c;return binary("-",nativeStatementTemp599,nativeStatementTemp600);}());const auto nativeStatementTemp598=JSValue(2);return binary("/",nativeStatementTemp597,nativeStatementTemp598);}()));return nativeStatementTemp594.call(nativeStatementTemp595,nativeStatementTemp596);}());
JSValue c3_1=([&]()->JSValue{const auto nativeStatementTemp601=cos;const JSValue nativeStatementTemp602;Args nativeStatementTemp603;nativeStatementTemp603.push_back(([&]()->JSValue{const auto nativeStatementTemp604=([&]()->JSValue{const auto nativeStatementTemp606=c;const auto nativeStatementTemp607=a;return binary("-",nativeStatementTemp606,nativeStatementTemp607);}());const auto nativeStatementTemp605=JSValue(2);return binary("/",nativeStatementTemp604,nativeStatementTemp605);}()));return nativeStatementTemp601.call(nativeStatementTemp602,nativeStatementTemp603);}());
JSValue s3_1=([&]()->JSValue{const auto nativeStatementTemp608=sin;const JSValue nativeStatementTemp609;Args nativeStatementTemp610;nativeStatementTemp610.push_back(([&]()->JSValue{const auto nativeStatementTemp611=([&]()->JSValue{const auto nativeStatementTemp613=c;const auto nativeStatementTemp614=a;return binary("-",nativeStatementTemp613,nativeStatementTemp614);}());const auto nativeStatementTemp612=JSValue(2);return binary("/",nativeStatementTemp611,nativeStatementTemp612);}()));return nativeStatementTemp608.call(nativeStatementTemp609,nativeStatementTemp610);}());
{const auto nativeStatementTemp615=order;int nativeStatementTemp616=-1;
if(nativeStatementTemp616==-1&&nativeStatementTemp615.strictEquals(JSValue("XYX")))nativeStatementTemp616=0;
if(nativeStatementTemp616==-1&&nativeStatementTemp615.strictEquals(JSValue("YZY")))nativeStatementTemp616=1;
if(nativeStatementTemp616==-1&&nativeStatementTemp615.strictEquals(JSValue("ZXZ")))nativeStatementTemp616=2;
if(nativeStatementTemp616==-1&&nativeStatementTemp615.strictEquals(JSValue("XZX")))nativeStatementTemp616=3;
if(nativeStatementTemp616==-1&&nativeStatementTemp615.strictEquals(JSValue("YXY")))nativeStatementTemp616=4;
if(nativeStatementTemp616==-1&&nativeStatementTemp615.strictEquals(JSValue("ZYZ")))nativeStatementTemp616=5;
if(nativeStatementTemp616==-1)nativeStatementTemp616=6;
switch(nativeStatementTemp616){
case 0:
([&]()->JSValue{auto nativeStatementTemp620=q;const auto nativeStatementTemp621=JSValue("set");const auto nativeStatementTemp617=nativeStatementTemp620.get(nativeStatementTemp621);const auto nativeStatementTemp618=nativeStatementTemp620;Args nativeStatementTemp619;nativeStatementTemp619.push_back(([&]()->JSValue{const auto nativeStatementTemp622=c2;const auto nativeStatementTemp623=s13;return binary("*",nativeStatementTemp622,nativeStatementTemp623);}()));nativeStatementTemp619.push_back(([&]()->JSValue{const auto nativeStatementTemp624=s2;const auto nativeStatementTemp625=c1_3;return binary("*",nativeStatementTemp624,nativeStatementTemp625);}()));nativeStatementTemp619.push_back(([&]()->JSValue{const auto nativeStatementTemp626=s2;const auto nativeStatementTemp627=s1_3;return binary("*",nativeStatementTemp626,nativeStatementTemp627);}()));nativeStatementTemp619.push_back(([&]()->JSValue{const auto nativeStatementTemp628=c2;const auto nativeStatementTemp629=c13;return binary("*",nativeStatementTemp628,nativeStatementTemp629);}()));return nativeStatementTemp617.call(nativeStatementTemp618,nativeStatementTemp619);}());
break;
case 1:
([&]()->JSValue{auto nativeStatementTemp633=q;const auto nativeStatementTemp634=JSValue("set");const auto nativeStatementTemp630=nativeStatementTemp633.get(nativeStatementTemp634);const auto nativeStatementTemp631=nativeStatementTemp633;Args nativeStatementTemp632;nativeStatementTemp632.push_back(([&]()->JSValue{const auto nativeStatementTemp635=s2;const auto nativeStatementTemp636=s1_3;return binary("*",nativeStatementTemp635,nativeStatementTemp636);}()));nativeStatementTemp632.push_back(([&]()->JSValue{const auto nativeStatementTemp637=c2;const auto nativeStatementTemp638=s13;return binary("*",nativeStatementTemp637,nativeStatementTemp638);}()));nativeStatementTemp632.push_back(([&]()->JSValue{const auto nativeStatementTemp639=s2;const auto nativeStatementTemp640=c1_3;return binary("*",nativeStatementTemp639,nativeStatementTemp640);}()));nativeStatementTemp632.push_back(([&]()->JSValue{const auto nativeStatementTemp641=c2;const auto nativeStatementTemp642=c13;return binary("*",nativeStatementTemp641,nativeStatementTemp642);}()));return nativeStatementTemp630.call(nativeStatementTemp631,nativeStatementTemp632);}());
break;
case 2:
([&]()->JSValue{auto nativeStatementTemp646=q;const auto nativeStatementTemp647=JSValue("set");const auto nativeStatementTemp643=nativeStatementTemp646.get(nativeStatementTemp647);const auto nativeStatementTemp644=nativeStatementTemp646;Args nativeStatementTemp645;nativeStatementTemp645.push_back(([&]()->JSValue{const auto nativeStatementTemp648=s2;const auto nativeStatementTemp649=c1_3;return binary("*",nativeStatementTemp648,nativeStatementTemp649);}()));nativeStatementTemp645.push_back(([&]()->JSValue{const auto nativeStatementTemp650=s2;const auto nativeStatementTemp651=s1_3;return binary("*",nativeStatementTemp650,nativeStatementTemp651);}()));nativeStatementTemp645.push_back(([&]()->JSValue{const auto nativeStatementTemp652=c2;const auto nativeStatementTemp653=s13;return binary("*",nativeStatementTemp652,nativeStatementTemp653);}()));nativeStatementTemp645.push_back(([&]()->JSValue{const auto nativeStatementTemp654=c2;const auto nativeStatementTemp655=c13;return binary("*",nativeStatementTemp654,nativeStatementTemp655);}()));return nativeStatementTemp643.call(nativeStatementTemp644,nativeStatementTemp645);}());
break;
case 3:
([&]()->JSValue{auto nativeStatementTemp659=q;const auto nativeStatementTemp660=JSValue("set");const auto nativeStatementTemp656=nativeStatementTemp659.get(nativeStatementTemp660);const auto nativeStatementTemp657=nativeStatementTemp659;Args nativeStatementTemp658;nativeStatementTemp658.push_back(([&]()->JSValue{const auto nativeStatementTemp661=c2;const auto nativeStatementTemp662=s13;return binary("*",nativeStatementTemp661,nativeStatementTemp662);}()));nativeStatementTemp658.push_back(([&]()->JSValue{const auto nativeStatementTemp663=s2;const auto nativeStatementTemp664=s3_1;return binary("*",nativeStatementTemp663,nativeStatementTemp664);}()));nativeStatementTemp658.push_back(([&]()->JSValue{const auto nativeStatementTemp665=s2;const auto nativeStatementTemp666=c3_1;return binary("*",nativeStatementTemp665,nativeStatementTemp666);}()));nativeStatementTemp658.push_back(([&]()->JSValue{const auto nativeStatementTemp667=c2;const auto nativeStatementTemp668=c13;return binary("*",nativeStatementTemp667,nativeStatementTemp668);}()));return nativeStatementTemp656.call(nativeStatementTemp657,nativeStatementTemp658);}());
break;
case 4:
([&]()->JSValue{auto nativeStatementTemp672=q;const auto nativeStatementTemp673=JSValue("set");const auto nativeStatementTemp669=nativeStatementTemp672.get(nativeStatementTemp673);const auto nativeStatementTemp670=nativeStatementTemp672;Args nativeStatementTemp671;nativeStatementTemp671.push_back(([&]()->JSValue{const auto nativeStatementTemp674=s2;const auto nativeStatementTemp675=c3_1;return binary("*",nativeStatementTemp674,nativeStatementTemp675);}()));nativeStatementTemp671.push_back(([&]()->JSValue{const auto nativeStatementTemp676=c2;const auto nativeStatementTemp677=s13;return binary("*",nativeStatementTemp676,nativeStatementTemp677);}()));nativeStatementTemp671.push_back(([&]()->JSValue{const auto nativeStatementTemp678=s2;const auto nativeStatementTemp679=s3_1;return binary("*",nativeStatementTemp678,nativeStatementTemp679);}()));nativeStatementTemp671.push_back(([&]()->JSValue{const auto nativeStatementTemp680=c2;const auto nativeStatementTemp681=c13;return binary("*",nativeStatementTemp680,nativeStatementTemp681);}()));return nativeStatementTemp669.call(nativeStatementTemp670,nativeStatementTemp671);}());
break;
case 5:
([&]()->JSValue{auto nativeStatementTemp685=q;const auto nativeStatementTemp686=JSValue("set");const auto nativeStatementTemp682=nativeStatementTemp685.get(nativeStatementTemp686);const auto nativeStatementTemp683=nativeStatementTemp685;Args nativeStatementTemp684;nativeStatementTemp684.push_back(([&]()->JSValue{const auto nativeStatementTemp687=s2;const auto nativeStatementTemp688=s3_1;return binary("*",nativeStatementTemp687,nativeStatementTemp688);}()));nativeStatementTemp684.push_back(([&]()->JSValue{const auto nativeStatementTemp689=s2;const auto nativeStatementTemp690=c3_1;return binary("*",nativeStatementTemp689,nativeStatementTemp690);}()));nativeStatementTemp684.push_back(([&]()->JSValue{const auto nativeStatementTemp691=c2;const auto nativeStatementTemp692=s13;return binary("*",nativeStatementTemp691,nativeStatementTemp692);}()));nativeStatementTemp684.push_back(([&]()->JSValue{const auto nativeStatementTemp693=c2;const auto nativeStatementTemp694=c13;return binary("*",nativeStatementTemp693,nativeStatementTemp694);}()));return nativeStatementTemp682.call(nativeStatementTemp683,nativeStatementTemp684);}());
break;
case 6:
([&]()->JSValue{const auto nativeStatementTemp695=nativeStatementTemp556.get("warn");const JSValue nativeStatementTemp696;Args nativeStatementTemp697;nativeStatementTemp697.push_back(([&]()->JSValue{const auto nativeStatementTemp698=JSValue("MathUtils: .setQuaternionFromProperEuler() encountered an unknown order: ");const auto nativeStatementTemp699=order;return binary("+",nativeStatementTemp698,nativeStatementTemp699);}()));return nativeStatementTemp695.call(nativeStatementTemp696,nativeStatementTemp697);}());
}
}
return JSValue{};
}
JSValue function20(const JSValue& nativeStatementTemp700,const Args& nativeStatementTemp701,const Args& nativeStatementTemp702){
auto nativeStatementTemp703=nativeStatementTemp702[0];auto nativeStatementTemp704=nativeStatementTemp700;
JSValue value=arg(nativeStatementTemp701,0);
JSValue array=arg(nativeStatementTemp701,1);
{const auto nativeStatementTemp706=([&]()->JSValue{const auto nativeStatementTemp708=array;const auto nativeStatementTemp709=JSValue("constructor");return nativeStatementTemp708.get(nativeStatementTemp709);}());int nativeStatementTemp707=-1;
if(nativeStatementTemp707==-1&&nativeStatementTemp706.strictEquals(nativeGlobalBinding("Float32Array")))nativeStatementTemp707=0;
if(nativeStatementTemp707==-1&&nativeStatementTemp706.strictEquals(nativeGlobalBinding("Uint32Array")))nativeStatementTemp707=1;
if(nativeStatementTemp707==-1&&nativeStatementTemp706.strictEquals(nativeGlobalBinding("Uint16Array")))nativeStatementTemp707=2;
if(nativeStatementTemp707==-1&&nativeStatementTemp706.strictEquals(nativeGlobalBinding("Uint8Array")))nativeStatementTemp707=3;
if(nativeStatementTemp707==-1&&nativeStatementTemp706.strictEquals(nativeGlobalBinding("Uint8ClampedArray")))nativeStatementTemp707=4;
if(nativeStatementTemp707==-1&&nativeStatementTemp706.strictEquals(nativeGlobalBinding("Int32Array")))nativeStatementTemp707=5;
if(nativeStatementTemp707==-1&&nativeStatementTemp706.strictEquals(nativeGlobalBinding("Int16Array")))nativeStatementTemp707=6;
if(nativeStatementTemp707==-1&&nativeStatementTemp706.strictEquals(nativeGlobalBinding("Int8Array")))nativeStatementTemp707=7;
if(nativeStatementTemp707==-1)nativeStatementTemp707=8;
switch(nativeStatementTemp707){
case 0:
return value;
case 1:
return ([&]()->JSValue{const auto nativeStatementTemp710=value;const auto nativeStatementTemp711=JSValue(4294967295);return binary("/",nativeStatementTemp710,nativeStatementTemp711);}());
case 2:
return ([&]()->JSValue{const auto nativeStatementTemp712=value;const auto nativeStatementTemp713=JSValue(65535);return binary("/",nativeStatementTemp712,nativeStatementTemp713);}());
case 3:
case 4:
return ([&]()->JSValue{const auto nativeStatementTemp714=value;const auto nativeStatementTemp715=JSValue(255);return binary("/",nativeStatementTemp714,nativeStatementTemp715);}());
case 5:
return ([&]()->JSValue{auto nativeStatementTemp719=nativeGlobalBinding("Math");const auto nativeStatementTemp720=JSValue("max");const auto nativeStatementTemp716=nativeStatementTemp719.get(nativeStatementTemp720);const auto nativeStatementTemp717=nativeStatementTemp719;Args nativeStatementTemp718;nativeStatementTemp718.push_back(([&]()->JSValue{const auto nativeStatementTemp721=value;const auto nativeStatementTemp722=JSValue(2147483647);return binary("/",nativeStatementTemp721,nativeStatementTemp722);}()));nativeStatementTemp718.push_back(([&]()->JSValue{const auto nativeStatementTemp723=JSValue(1);return -nativeStatementTemp723.toNumber();}()));return nativeStatementTemp716.call(nativeStatementTemp717,nativeStatementTemp718);}());
case 6:
return ([&]()->JSValue{auto nativeStatementTemp727=nativeGlobalBinding("Math");const auto nativeStatementTemp728=JSValue("max");const auto nativeStatementTemp724=nativeStatementTemp727.get(nativeStatementTemp728);const auto nativeStatementTemp725=nativeStatementTemp727;Args nativeStatementTemp726;nativeStatementTemp726.push_back(([&]()->JSValue{const auto nativeStatementTemp729=value;const auto nativeStatementTemp730=JSValue(32767);return binary("/",nativeStatementTemp729,nativeStatementTemp730);}()));nativeStatementTemp726.push_back(([&]()->JSValue{const auto nativeStatementTemp731=JSValue(1);return -nativeStatementTemp731.toNumber();}()));return nativeStatementTemp724.call(nativeStatementTemp725,nativeStatementTemp726);}());
case 7:
return ([&]()->JSValue{auto nativeStatementTemp735=nativeGlobalBinding("Math");const auto nativeStatementTemp736=JSValue("max");const auto nativeStatementTemp732=nativeStatementTemp735.get(nativeStatementTemp736);const auto nativeStatementTemp733=nativeStatementTemp735;Args nativeStatementTemp734;nativeStatementTemp734.push_back(([&]()->JSValue{const auto nativeStatementTemp737=value;const auto nativeStatementTemp738=JSValue(127);return binary("/",nativeStatementTemp737,nativeStatementTemp738);}()));nativeStatementTemp734.push_back(([&]()->JSValue{const auto nativeStatementTemp739=JSValue(1);return -nativeStatementTemp739.toNumber();}()));return nativeStatementTemp732.call(nativeStatementTemp733,nativeStatementTemp734);}());
case 8:
throw JSThrownValue(([&]()->JSValue{const auto nativeStatementTemp740=nativeGlobalBinding("Error");Args nativeStatementTemp741;nativeStatementTemp741.push_back(JSValue("THREE.MathUtils: Invalid component type."));return nativeStatementTemp740.construct(nativeStatementTemp741);}()));
}
}
return JSValue{};
}
JSValue function21(const JSValue& nativeStatementTemp742,const Args& nativeStatementTemp743,const Args& nativeStatementTemp744){
auto nativeStatementTemp745=nativeStatementTemp744[0];auto nativeStatementTemp746=nativeStatementTemp742;
JSValue value=arg(nativeStatementTemp743,0);
JSValue array=arg(nativeStatementTemp743,1);
{const auto nativeStatementTemp748=([&]()->JSValue{const auto nativeStatementTemp750=array;const auto nativeStatementTemp751=JSValue("constructor");return nativeStatementTemp750.get(nativeStatementTemp751);}());int nativeStatementTemp749=-1;
if(nativeStatementTemp749==-1&&nativeStatementTemp748.strictEquals(nativeGlobalBinding("Float32Array")))nativeStatementTemp749=0;
if(nativeStatementTemp749==-1&&nativeStatementTemp748.strictEquals(nativeGlobalBinding("Uint32Array")))nativeStatementTemp749=1;
if(nativeStatementTemp749==-1&&nativeStatementTemp748.strictEquals(nativeGlobalBinding("Uint16Array")))nativeStatementTemp749=2;
if(nativeStatementTemp749==-1&&nativeStatementTemp748.strictEquals(nativeGlobalBinding("Uint8Array")))nativeStatementTemp749=3;
if(nativeStatementTemp749==-1&&nativeStatementTemp748.strictEquals(nativeGlobalBinding("Uint8ClampedArray")))nativeStatementTemp749=4;
if(nativeStatementTemp749==-1&&nativeStatementTemp748.strictEquals(nativeGlobalBinding("Int32Array")))nativeStatementTemp749=5;
if(nativeStatementTemp749==-1&&nativeStatementTemp748.strictEquals(nativeGlobalBinding("Int16Array")))nativeStatementTemp749=6;
if(nativeStatementTemp749==-1&&nativeStatementTemp748.strictEquals(nativeGlobalBinding("Int8Array")))nativeStatementTemp749=7;
if(nativeStatementTemp749==-1)nativeStatementTemp749=8;
switch(nativeStatementTemp749){
case 0:
return value;
case 1:
return ([&]()->JSValue{auto nativeStatementTemp755=nativeGlobalBinding("Math");const auto nativeStatementTemp756=JSValue("round");const auto nativeStatementTemp752=nativeStatementTemp755.get(nativeStatementTemp756);const auto nativeStatementTemp753=nativeStatementTemp755;Args nativeStatementTemp754;nativeStatementTemp754.push_back(([&]()->JSValue{const auto nativeStatementTemp757=value;const auto nativeStatementTemp758=JSValue(4294967295);return binary("*",nativeStatementTemp757,nativeStatementTemp758);}()));return nativeStatementTemp752.call(nativeStatementTemp753,nativeStatementTemp754);}());
case 2:
return ([&]()->JSValue{auto nativeStatementTemp762=nativeGlobalBinding("Math");const auto nativeStatementTemp763=JSValue("round");const auto nativeStatementTemp759=nativeStatementTemp762.get(nativeStatementTemp763);const auto nativeStatementTemp760=nativeStatementTemp762;Args nativeStatementTemp761;nativeStatementTemp761.push_back(([&]()->JSValue{const auto nativeStatementTemp764=value;const auto nativeStatementTemp765=JSValue(65535);return binary("*",nativeStatementTemp764,nativeStatementTemp765);}()));return nativeStatementTemp759.call(nativeStatementTemp760,nativeStatementTemp761);}());
case 3:
case 4:
return ([&]()->JSValue{auto nativeStatementTemp769=nativeGlobalBinding("Math");const auto nativeStatementTemp770=JSValue("round");const auto nativeStatementTemp766=nativeStatementTemp769.get(nativeStatementTemp770);const auto nativeStatementTemp767=nativeStatementTemp769;Args nativeStatementTemp768;nativeStatementTemp768.push_back(([&]()->JSValue{const auto nativeStatementTemp771=value;const auto nativeStatementTemp772=JSValue(255);return binary("*",nativeStatementTemp771,nativeStatementTemp772);}()));return nativeStatementTemp766.call(nativeStatementTemp767,nativeStatementTemp768);}());
case 5:
return ([&]()->JSValue{auto nativeStatementTemp776=nativeGlobalBinding("Math");const auto nativeStatementTemp777=JSValue("round");const auto nativeStatementTemp773=nativeStatementTemp776.get(nativeStatementTemp777);const auto nativeStatementTemp774=nativeStatementTemp776;Args nativeStatementTemp775;nativeStatementTemp775.push_back(([&]()->JSValue{const auto nativeStatementTemp778=value;const auto nativeStatementTemp779=JSValue(2147483647);return binary("*",nativeStatementTemp778,nativeStatementTemp779);}()));return nativeStatementTemp773.call(nativeStatementTemp774,nativeStatementTemp775);}());
case 6:
return ([&]()->JSValue{auto nativeStatementTemp783=nativeGlobalBinding("Math");const auto nativeStatementTemp784=JSValue("round");const auto nativeStatementTemp780=nativeStatementTemp783.get(nativeStatementTemp784);const auto nativeStatementTemp781=nativeStatementTemp783;Args nativeStatementTemp782;nativeStatementTemp782.push_back(([&]()->JSValue{const auto nativeStatementTemp785=value;const auto nativeStatementTemp786=JSValue(32767);return binary("*",nativeStatementTemp785,nativeStatementTemp786);}()));return nativeStatementTemp780.call(nativeStatementTemp781,nativeStatementTemp782);}());
case 7:
return ([&]()->JSValue{auto nativeStatementTemp790=nativeGlobalBinding("Math");const auto nativeStatementTemp791=JSValue("round");const auto nativeStatementTemp787=nativeStatementTemp790.get(nativeStatementTemp791);const auto nativeStatementTemp788=nativeStatementTemp790;Args nativeStatementTemp789;nativeStatementTemp789.push_back(([&]()->JSValue{const auto nativeStatementTemp792=value;const auto nativeStatementTemp793=JSValue(127);return binary("*",nativeStatementTemp792,nativeStatementTemp793);}()));return nativeStatementTemp787.call(nativeStatementTemp788,nativeStatementTemp789);}());
case 8:
throw JSThrownValue(([&]()->JSValue{const auto nativeStatementTemp794=nativeGlobalBinding("Error");Args nativeStatementTemp795;nativeStatementTemp795.push_back(JSValue("THREE.MathUtils: Invalid component type."));return nativeStatementTemp794.construct(nativeStatementTemp795);}()));
}
}
return JSValue{};
}
}
JSValue nativeMathUtilsModule(){using namespace FullMathUtilsManaged;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject()){const auto status=existing->getInternalSlot("MathUtils.status");if(status.strictEquals("evaluated"))return existing->getInternalSlot("MathUtils.facade");if(status.strictEquals("failed"))throw JSThrownValue(existing->getInternalSlot("MathUtils.error"));throw JSModuleInitializationError("MathUtils module");}auto module=JSValue::object(nullptr);module.retainForHeapLifetime();weak=JSWeakValue(module);lexical()=weak;module.setInternalSlot("MathUtils.status","initializing");try{own(module,"warn",ThreeUtils::nativeWarnFunction());
{auto fn=JSValue::constructor(function0,{module},"generateUUID",0,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function generateUUID() {\r\n\r\n\t// http://stackoverflow.com/questions/105034/how-to-create-a-guid-uuid-in-javascript/21963136#21963136\r\n\r\n\tconst d0 = Math.random() * 0xffffffff | 0;\r\n\tconst d1 = Math.random() * 0xffffffff | 0;\r\n\tconst d2 = Math.random() * 0xffffffff | 0;\r\n\tconst d3 = Math.random() * 0xffffffff | 0;\r\n\tconst uuid = _lut[ d0 & 0xff ] + _lut[ d0 >> 8 & 0xff ] + _lut[ d0 >> 16 & 0xff ] + _lut[ d0 >> 24 & 0xff ] + '-' +\r\n\t\t\t_lut[ d1 & 0xff ] + _lut[ d1 >> 8 & 0xff ] + '-' + _lut[ d1 >> 16 & 0x0f | 0x40 ] + _lut[ d1 >> 24 & 0xff ] + '-' +\r\n\t\t\t_lut[ d2 & 0x3f | 0x80 ] + _lut[ d2 >> 8 & 0xff ] + '-' + _lut[ d2 >> 16 & 0xff ] + _lut[ d2 >> 24 & 0xff ] +\r\n\t\t\t_lut[ d3 & 0xff ] + _lut[ d3 >> 8 & 0xff ] + _lut[ d3 >> 16 & 0xff ] + _lut[ d3 >> 24 & 0xff ];\r\n\r\n\t// .toLowerCase() here flattens concatenated strings to save heap memory space.\r\n\treturn uuid.toLowerCase();\r\n\r\n}");own(module,"generateUUID",fn);}
{auto fn=JSValue::constructor(function1,{module},"clamp",3,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function clamp( value, min, max ) {\r\n\r\n\treturn Math.max( min, Math.min( max, value ) );\r\n\r\n}");own(module,"clamp",fn);}
{auto fn=JSValue::constructor(function2,{module},"euclideanModulo",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function euclideanModulo( n, m ) {\r\n\r\n\t// https://en.wikipedia.org/wiki/Modulo_operation\r\n\r\n\treturn ( ( n % m ) + m ) % m;\r\n\r\n}");own(module,"euclideanModulo",fn);}
{auto fn=JSValue::constructor(function3,{module},"mapLinear",5,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function mapLinear( x, a1, a2, b1, b2 ) {\r\n\r\n\treturn b1 + ( x - a1 ) * ( b2 - b1 ) / ( a2 - a1 );\r\n\r\n}");own(module,"mapLinear",fn);}
{auto fn=JSValue::constructor(function4,{module},"inverseLerp",3,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function inverseLerp( x, y, value ) {\r\n\r\n\t// https://www.gamedev.net/tutorials/programming/general-and-gameplay-programming/inverse-lerp-a-super-useful-yet-often-overlooked-function-r5230/\r\n\r\n\tif ( x !== y ) {\r\n\r\n\t\treturn ( value - x ) / ( y - x );\r\n\r\n\t} else {\r\n\r\n\t\treturn 0;\r\n\r\n\t}\r\n\r\n}");own(module,"inverseLerp",fn);}
{auto fn=JSValue::constructor(function5,{module},"lerp",3,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function lerp( x, y, t ) {\r\n\r\n\treturn ( 1 - t ) * x + t * y;\r\n\r\n}");own(module,"lerp",fn);}
{auto fn=JSValue::constructor(function6,{module},"damp",4,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function damp( x, y, lambda, dt ) {\r\n\r\n\treturn lerp( x, y, 1 - Math.exp( - lambda * dt ) );\r\n\r\n}");own(module,"damp",fn);}
{auto fn=JSValue::constructor(function7,{module},"pingpong",1,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function pingpong( x, length = 1 ) {\r\n\r\n\t// https://www.desmos.com/calculator/vcsjnyz7x4\r\n\r\n\treturn length - Math.abs( euclideanModulo( x, length * 2 ) - length );\r\n\r\n}");own(module,"pingpong",fn);}
{auto fn=JSValue::constructor(function8,{module},"smoothstep",3,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function smoothstep( x, min, max ) {\r\n\r\n\tif ( x <= min ) return 0;\r\n\tif ( x >= max ) return 1;\r\n\r\n\tx = ( x - min ) / ( max - min );\r\n\r\n\treturn x * x * ( 3 - 2 * x );\r\n\r\n}");own(module,"smoothstep",fn);}
{auto fn=JSValue::constructor(function9,{module},"smootherstep",3,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function smootherstep( x, min, max ) {\r\n\r\n\tif ( x <= min ) return 0;\r\n\tif ( x >= max ) return 1;\r\n\r\n\tx = ( x - min ) / ( max - min );\r\n\r\n\treturn x * x * x * ( x * ( x * 6 - 15 ) + 10 );\r\n\r\n}");own(module,"smootherstep",fn);}
{auto fn=JSValue::constructor(function10,{module},"randInt",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function randInt( low, high ) {\r\n\r\n\treturn low + Math.floor( Math.random() * ( high - low + 1 ) );\r\n\r\n}");own(module,"randInt",fn);}
{auto fn=JSValue::constructor(function11,{module},"randFloat",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function randFloat( low, high ) {\r\n\r\n\treturn low + Math.random() * ( high - low );\r\n\r\n}");own(module,"randFloat",fn);}
{auto fn=JSValue::constructor(function12,{module},"randFloatSpread",1,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function randFloatSpread( range ) {\r\n\r\n\treturn range * ( 0.5 - Math.random() );\r\n\r\n}");own(module,"randFloatSpread",fn);}
{auto fn=JSValue::constructor(function13,{module},"seededRandom",1,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function seededRandom( s ) {\r\n\r\n\tif ( s !== undefined ) _seed = s;\r\n\r\n\t// Mulberry32 generator\r\n\r\n\tlet t = _seed += 0x6D2B79F5;\r\n\r\n\tt = Math.imul( t ^ t >>> 15, t | 1 );\r\n\r\n\tt ^= t + Math.imul( t ^ t >>> 7, t | 61 );\r\n\r\n\treturn ( ( t ^ t >>> 14 ) >>> 0 ) / 4294967296;\r\n\r\n}");own(module,"seededRandom",fn);}
{auto fn=JSValue::constructor(function14,{module},"degToRad",1,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function degToRad( degrees ) {\r\n\r\n\treturn degrees * DEG2RAD;\r\n\r\n}");own(module,"degToRad",fn);}
{auto fn=JSValue::constructor(function15,{module},"radToDeg",1,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function radToDeg( radians ) {\r\n\r\n\treturn radians * RAD2DEG;\r\n\r\n}");own(module,"radToDeg",fn);}
{auto fn=JSValue::constructor(function16,{module},"isPowerOfTwo",1,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function isPowerOfTwo( value ) {\r\n\r\n\treturn value > 0 && Number.isInteger( value ) && 2 ** Math.round( Math.log2( value ) ) === value;\r\n\r\n}");own(module,"isPowerOfTwo",fn);}
{auto fn=JSValue::constructor(function17,{module},"ceilPowerOfTwo",1,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function ceilPowerOfTwo( value ) {\r\n\r\n\treturn Math.pow( 2, Math.ceil( Math.log( value ) / Math.LN2 ) );\r\n\r\n}");own(module,"ceilPowerOfTwo",fn);}
{auto fn=JSValue::constructor(function18,{module},"floorPowerOfTwo",1,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function floorPowerOfTwo( value ) {\r\n\r\n\treturn Math.pow( 2, Math.floor( Math.log( value ) / Math.LN2 ) );\r\n\r\n}");own(module,"floorPowerOfTwo",fn);}
{auto fn=JSValue::constructor(function19,{module},"setQuaternionFromProperEuler",5,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function setQuaternionFromProperEuler( q, a, b, c, order ) {\r\n\r\n\tconst cos = Math.cos;\r\n\tconst sin = Math.sin;\r\n\r\n\tconst c2 = cos( b / 2 );\r\n\tconst s2 = sin( b / 2 );\r\n\r\n\tconst c13 = cos( ( a + c ) / 2 );\r\n\tconst s13 = sin( ( a + c ) / 2 );\r\n\r\n\tconst c1_3 = cos( ( a - c ) / 2 );\r\n\tconst s1_3 = sin( ( a - c ) / 2 );\r\n\r\n\tconst c3_1 = cos( ( c - a ) / 2 );\r\n\tconst s3_1 = sin( ( c - a ) / 2 );\r\n\r\n\tswitch ( order ) {\r\n\r\n\t\tcase 'XYX':\r\n\t\t\tq.set( c2 * s13, s2 * c1_3, s2 * s1_3, c2 * c13 );\r\n\t\t\tbreak;\r\n\r\n\t\tcase 'YZY':\r\n\t\t\tq.set( s2 * s1_3, c2 * s13, s2 * c1_3, c2 * c13 );\r\n\t\t\tbreak;\r\n\r\n\t\tcase 'ZXZ':\r\n\t\t\tq.set( s2 * c1_3, s2 * s1_3, c2 * s13, c2 * c13 );\r\n\t\t\tbreak;\r\n\r\n\t\tcase 'XZX':\r\n\t\t\tq.set( c2 * s13, s2 * s3_1, s2 * c3_1, c2 * c13 );\r\n\t\t\tbreak;\r\n\r\n\t\tcase 'YXY':\r\n\t\t\tq.set( s2 * c3_1, c2 * s13, s2 * s3_1, c2 * c13 );\r\n\t\t\tbreak;\r\n\r\n\t\tcase 'ZYZ':\r\n\t\t\tq.set( s2 * s3_1, s2 * c3_1, c2 * s13, c2 * c13 );\r\n\t\t\tbreak;\r\n\r\n\t\tdefault:\r\n\t\t\twarn( 'MathUtils: .setQuaternionFromProperEuler() encountered an unknown order: ' + order );\r\n\r\n\t}\r\n\r\n}");own(module,"setQuaternionFromProperEuler",fn);}
{auto fn=JSValue::constructor(function20,{module},"denormalize",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function denormalize( value, array ) {\r\n\r\n\tswitch ( array.constructor ) {\r\n\r\n\t\tcase Float32Array:\r\n\r\n\t\t\treturn value;\r\n\r\n\t\tcase Uint32Array:\r\n\r\n\t\t\treturn value / 4294967295.0;\r\n\r\n\t\tcase Uint16Array:\r\n\r\n\t\t\treturn value / 65535.0;\r\n\r\n\t\tcase Uint8Array:\r\n\t\tcase Uint8ClampedArray:\r\n\r\n\t\t\treturn value / 255.0;\r\n\r\n\t\tcase Int32Array:\r\n\r\n\t\t\treturn Math.max( value / 2147483647.0, - 1.0 );\r\n\r\n\t\tcase Int16Array:\r\n\r\n\t\t\treturn Math.max( value / 32767.0, - 1.0 );\r\n\r\n\t\tcase Int8Array:\r\n\r\n\t\t\treturn Math.max( value / 127.0, - 1.0 );\r\n\r\n\t\tdefault:\r\n\r\n\t\t\tthrow new Error( 'THREE.MathUtils: Invalid component type.' );\r\n\r\n\t}\r\n\r\n}");own(module,"denormalize",fn);}
{auto fn=JSValue::constructor(function21,{module},"normalize",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText("function normalize( value, array ) {\r\n\r\n\tswitch ( array.constructor ) {\r\n\r\n\t\tcase Float32Array:\r\n\r\n\t\t\treturn value;\r\n\r\n\t\tcase Uint32Array:\r\n\r\n\t\t\treturn Math.round( value * 4294967295.0 );\r\n\r\n\t\tcase Uint16Array:\r\n\r\n\t\t\treturn Math.round( value * 65535.0 );\r\n\r\n\t\tcase Uint8Array:\r\n\t\tcase Uint8ClampedArray:\r\n\r\n\t\t\treturn Math.round( value * 255.0 );\r\n\r\n\t\tcase Int32Array:\r\n\r\n\t\t\treturn Math.round( value * 2147483647.0 );\r\n\r\n\t\tcase Int16Array:\r\n\r\n\t\t\treturn Math.round( value * 32767.0 );\r\n\r\n\t\tcase Int8Array:\r\n\r\n\t\t\treturn Math.round( value * 127.0 );\r\n\r\n\t\tdefault:\r\n\r\n\t\t\tthrow new Error( 'THREE.MathUtils: Invalid component type.' );\r\n\r\n\t}\r\n\r\n}");own(module,"normalize",fn);}
own(module,"_lut",([&]()->JSValue{auto nativeStatementTemp796=JSValue::array();own(nativeStatementTemp796,JSValue(0),JSValue("00"));own(nativeStatementTemp796,JSValue(1),JSValue("01"));own(nativeStatementTemp796,JSValue(2),JSValue("02"));own(nativeStatementTemp796,JSValue(3),JSValue("03"));own(nativeStatementTemp796,JSValue(4),JSValue("04"));own(nativeStatementTemp796,JSValue(5),JSValue("05"));own(nativeStatementTemp796,JSValue(6),JSValue("06"));own(nativeStatementTemp796,JSValue(7),JSValue("07"));own(nativeStatementTemp796,JSValue(8),JSValue("08"));own(nativeStatementTemp796,JSValue(9),JSValue("09"));own(nativeStatementTemp796,JSValue(10),JSValue("0a"));own(nativeStatementTemp796,JSValue(11),JSValue("0b"));own(nativeStatementTemp796,JSValue(12),JSValue("0c"));own(nativeStatementTemp796,JSValue(13),JSValue("0d"));own(nativeStatementTemp796,JSValue(14),JSValue("0e"));own(nativeStatementTemp796,JSValue(15),JSValue("0f"));own(nativeStatementTemp796,JSValue(16),JSValue("10"));own(nativeStatementTemp796,JSValue(17),JSValue("11"));own(nativeStatementTemp796,JSValue(18),JSValue("12"));own(nativeStatementTemp796,JSValue(19),JSValue("13"));own(nativeStatementTemp796,JSValue(20),JSValue("14"));own(nativeStatementTemp796,JSValue(21),JSValue("15"));own(nativeStatementTemp796,JSValue(22),JSValue("16"));own(nativeStatementTemp796,JSValue(23),JSValue("17"));own(nativeStatementTemp796,JSValue(24),JSValue("18"));own(nativeStatementTemp796,JSValue(25),JSValue("19"));own(nativeStatementTemp796,JSValue(26),JSValue("1a"));own(nativeStatementTemp796,JSValue(27),JSValue("1b"));own(nativeStatementTemp796,JSValue(28),JSValue("1c"));own(nativeStatementTemp796,JSValue(29),JSValue("1d"));own(nativeStatementTemp796,JSValue(30),JSValue("1e"));own(nativeStatementTemp796,JSValue(31),JSValue("1f"));own(nativeStatementTemp796,JSValue(32),JSValue("20"));own(nativeStatementTemp796,JSValue(33),JSValue("21"));own(nativeStatementTemp796,JSValue(34),JSValue("22"));own(nativeStatementTemp796,JSValue(35),JSValue("23"));own(nativeStatementTemp796,JSValue(36),JSValue("24"));own(nativeStatementTemp796,JSValue(37),JSValue("25"));own(nativeStatementTemp796,JSValue(38),JSValue("26"));own(nativeStatementTemp796,JSValue(39),JSValue("27"));own(nativeStatementTemp796,JSValue(40),JSValue("28"));own(nativeStatementTemp796,JSValue(41),JSValue("29"));own(nativeStatementTemp796,JSValue(42),JSValue("2a"));own(nativeStatementTemp796,JSValue(43),JSValue("2b"));own(nativeStatementTemp796,JSValue(44),JSValue("2c"));own(nativeStatementTemp796,JSValue(45),JSValue("2d"));own(nativeStatementTemp796,JSValue(46),JSValue("2e"));own(nativeStatementTemp796,JSValue(47),JSValue("2f"));own(nativeStatementTemp796,JSValue(48),JSValue("30"));own(nativeStatementTemp796,JSValue(49),JSValue("31"));own(nativeStatementTemp796,JSValue(50),JSValue("32"));own(nativeStatementTemp796,JSValue(51),JSValue("33"));own(nativeStatementTemp796,JSValue(52),JSValue("34"));own(nativeStatementTemp796,JSValue(53),JSValue("35"));own(nativeStatementTemp796,JSValue(54),JSValue("36"));own(nativeStatementTemp796,JSValue(55),JSValue("37"));own(nativeStatementTemp796,JSValue(56),JSValue("38"));own(nativeStatementTemp796,JSValue(57),JSValue("39"));own(nativeStatementTemp796,JSValue(58),JSValue("3a"));own(nativeStatementTemp796,JSValue(59),JSValue("3b"));own(nativeStatementTemp796,JSValue(60),JSValue("3c"));own(nativeStatementTemp796,JSValue(61),JSValue("3d"));own(nativeStatementTemp796,JSValue(62),JSValue("3e"));own(nativeStatementTemp796,JSValue(63),JSValue("3f"));own(nativeStatementTemp796,JSValue(64),JSValue("40"));own(nativeStatementTemp796,JSValue(65),JSValue("41"));own(nativeStatementTemp796,JSValue(66),JSValue("42"));own(nativeStatementTemp796,JSValue(67),JSValue("43"));own(nativeStatementTemp796,JSValue(68),JSValue("44"));own(nativeStatementTemp796,JSValue(69),JSValue("45"));own(nativeStatementTemp796,JSValue(70),JSValue("46"));own(nativeStatementTemp796,JSValue(71),JSValue("47"));own(nativeStatementTemp796,JSValue(72),JSValue("48"));own(nativeStatementTemp796,JSValue(73),JSValue("49"));own(nativeStatementTemp796,JSValue(74),JSValue("4a"));own(nativeStatementTemp796,JSValue(75),JSValue("4b"));own(nativeStatementTemp796,JSValue(76),JSValue("4c"));own(nativeStatementTemp796,JSValue(77),JSValue("4d"));own(nativeStatementTemp796,JSValue(78),JSValue("4e"));own(nativeStatementTemp796,JSValue(79),JSValue("4f"));own(nativeStatementTemp796,JSValue(80),JSValue("50"));own(nativeStatementTemp796,JSValue(81),JSValue("51"));own(nativeStatementTemp796,JSValue(82),JSValue("52"));own(nativeStatementTemp796,JSValue(83),JSValue("53"));own(nativeStatementTemp796,JSValue(84),JSValue("54"));own(nativeStatementTemp796,JSValue(85),JSValue("55"));own(nativeStatementTemp796,JSValue(86),JSValue("56"));own(nativeStatementTemp796,JSValue(87),JSValue("57"));own(nativeStatementTemp796,JSValue(88),JSValue("58"));own(nativeStatementTemp796,JSValue(89),JSValue("59"));own(nativeStatementTemp796,JSValue(90),JSValue("5a"));own(nativeStatementTemp796,JSValue(91),JSValue("5b"));own(nativeStatementTemp796,JSValue(92),JSValue("5c"));own(nativeStatementTemp796,JSValue(93),JSValue("5d"));own(nativeStatementTemp796,JSValue(94),JSValue("5e"));own(nativeStatementTemp796,JSValue(95),JSValue("5f"));own(nativeStatementTemp796,JSValue(96),JSValue("60"));own(nativeStatementTemp796,JSValue(97),JSValue("61"));own(nativeStatementTemp796,JSValue(98),JSValue("62"));own(nativeStatementTemp796,JSValue(99),JSValue("63"));own(nativeStatementTemp796,JSValue(100),JSValue("64"));own(nativeStatementTemp796,JSValue(101),JSValue("65"));own(nativeStatementTemp796,JSValue(102),JSValue("66"));own(nativeStatementTemp796,JSValue(103),JSValue("67"));own(nativeStatementTemp796,JSValue(104),JSValue("68"));own(nativeStatementTemp796,JSValue(105),JSValue("69"));own(nativeStatementTemp796,JSValue(106),JSValue("6a"));own(nativeStatementTemp796,JSValue(107),JSValue("6b"));own(nativeStatementTemp796,JSValue(108),JSValue("6c"));own(nativeStatementTemp796,JSValue(109),JSValue("6d"));own(nativeStatementTemp796,JSValue(110),JSValue("6e"));own(nativeStatementTemp796,JSValue(111),JSValue("6f"));own(nativeStatementTemp796,JSValue(112),JSValue("70"));own(nativeStatementTemp796,JSValue(113),JSValue("71"));own(nativeStatementTemp796,JSValue(114),JSValue("72"));own(nativeStatementTemp796,JSValue(115),JSValue("73"));own(nativeStatementTemp796,JSValue(116),JSValue("74"));own(nativeStatementTemp796,JSValue(117),JSValue("75"));own(nativeStatementTemp796,JSValue(118),JSValue("76"));own(nativeStatementTemp796,JSValue(119),JSValue("77"));own(nativeStatementTemp796,JSValue(120),JSValue("78"));own(nativeStatementTemp796,JSValue(121),JSValue("79"));own(nativeStatementTemp796,JSValue(122),JSValue("7a"));own(nativeStatementTemp796,JSValue(123),JSValue("7b"));own(nativeStatementTemp796,JSValue(124),JSValue("7c"));own(nativeStatementTemp796,JSValue(125),JSValue("7d"));own(nativeStatementTemp796,JSValue(126),JSValue("7e"));own(nativeStatementTemp796,JSValue(127),JSValue("7f"));own(nativeStatementTemp796,JSValue(128),JSValue("80"));own(nativeStatementTemp796,JSValue(129),JSValue("81"));own(nativeStatementTemp796,JSValue(130),JSValue("82"));own(nativeStatementTemp796,JSValue(131),JSValue("83"));own(nativeStatementTemp796,JSValue(132),JSValue("84"));own(nativeStatementTemp796,JSValue(133),JSValue("85"));own(nativeStatementTemp796,JSValue(134),JSValue("86"));own(nativeStatementTemp796,JSValue(135),JSValue("87"));own(nativeStatementTemp796,JSValue(136),JSValue("88"));own(nativeStatementTemp796,JSValue(137),JSValue("89"));own(nativeStatementTemp796,JSValue(138),JSValue("8a"));own(nativeStatementTemp796,JSValue(139),JSValue("8b"));own(nativeStatementTemp796,JSValue(140),JSValue("8c"));own(nativeStatementTemp796,JSValue(141),JSValue("8d"));own(nativeStatementTemp796,JSValue(142),JSValue("8e"));own(nativeStatementTemp796,JSValue(143),JSValue("8f"));own(nativeStatementTemp796,JSValue(144),JSValue("90"));own(nativeStatementTemp796,JSValue(145),JSValue("91"));own(nativeStatementTemp796,JSValue(146),JSValue("92"));own(nativeStatementTemp796,JSValue(147),JSValue("93"));own(nativeStatementTemp796,JSValue(148),JSValue("94"));own(nativeStatementTemp796,JSValue(149),JSValue("95"));own(nativeStatementTemp796,JSValue(150),JSValue("96"));own(nativeStatementTemp796,JSValue(151),JSValue("97"));own(nativeStatementTemp796,JSValue(152),JSValue("98"));own(nativeStatementTemp796,JSValue(153),JSValue("99"));own(nativeStatementTemp796,JSValue(154),JSValue("9a"));own(nativeStatementTemp796,JSValue(155),JSValue("9b"));own(nativeStatementTemp796,JSValue(156),JSValue("9c"));own(nativeStatementTemp796,JSValue(157),JSValue("9d"));own(nativeStatementTemp796,JSValue(158),JSValue("9e"));own(nativeStatementTemp796,JSValue(159),JSValue("9f"));own(nativeStatementTemp796,JSValue(160),JSValue("a0"));own(nativeStatementTemp796,JSValue(161),JSValue("a1"));own(nativeStatementTemp796,JSValue(162),JSValue("a2"));own(nativeStatementTemp796,JSValue(163),JSValue("a3"));own(nativeStatementTemp796,JSValue(164),JSValue("a4"));own(nativeStatementTemp796,JSValue(165),JSValue("a5"));own(nativeStatementTemp796,JSValue(166),JSValue("a6"));own(nativeStatementTemp796,JSValue(167),JSValue("a7"));own(nativeStatementTemp796,JSValue(168),JSValue("a8"));own(nativeStatementTemp796,JSValue(169),JSValue("a9"));own(nativeStatementTemp796,JSValue(170),JSValue("aa"));own(nativeStatementTemp796,JSValue(171),JSValue("ab"));own(nativeStatementTemp796,JSValue(172),JSValue("ac"));own(nativeStatementTemp796,JSValue(173),JSValue("ad"));own(nativeStatementTemp796,JSValue(174),JSValue("ae"));own(nativeStatementTemp796,JSValue(175),JSValue("af"));own(nativeStatementTemp796,JSValue(176),JSValue("b0"));own(nativeStatementTemp796,JSValue(177),JSValue("b1"));own(nativeStatementTemp796,JSValue(178),JSValue("b2"));own(nativeStatementTemp796,JSValue(179),JSValue("b3"));own(nativeStatementTemp796,JSValue(180),JSValue("b4"));own(nativeStatementTemp796,JSValue(181),JSValue("b5"));own(nativeStatementTemp796,JSValue(182),JSValue("b6"));own(nativeStatementTemp796,JSValue(183),JSValue("b7"));own(nativeStatementTemp796,JSValue(184),JSValue("b8"));own(nativeStatementTemp796,JSValue(185),JSValue("b9"));own(nativeStatementTemp796,JSValue(186),JSValue("ba"));own(nativeStatementTemp796,JSValue(187),JSValue("bb"));own(nativeStatementTemp796,JSValue(188),JSValue("bc"));own(nativeStatementTemp796,JSValue(189),JSValue("bd"));own(nativeStatementTemp796,JSValue(190),JSValue("be"));own(nativeStatementTemp796,JSValue(191),JSValue("bf"));own(nativeStatementTemp796,JSValue(192),JSValue("c0"));own(nativeStatementTemp796,JSValue(193),JSValue("c1"));own(nativeStatementTemp796,JSValue(194),JSValue("c2"));own(nativeStatementTemp796,JSValue(195),JSValue("c3"));own(nativeStatementTemp796,JSValue(196),JSValue("c4"));own(nativeStatementTemp796,JSValue(197),JSValue("c5"));own(nativeStatementTemp796,JSValue(198),JSValue("c6"));own(nativeStatementTemp796,JSValue(199),JSValue("c7"));own(nativeStatementTemp796,JSValue(200),JSValue("c8"));own(nativeStatementTemp796,JSValue(201),JSValue("c9"));own(nativeStatementTemp796,JSValue(202),JSValue("ca"));own(nativeStatementTemp796,JSValue(203),JSValue("cb"));own(nativeStatementTemp796,JSValue(204),JSValue("cc"));own(nativeStatementTemp796,JSValue(205),JSValue("cd"));own(nativeStatementTemp796,JSValue(206),JSValue("ce"));own(nativeStatementTemp796,JSValue(207),JSValue("cf"));own(nativeStatementTemp796,JSValue(208),JSValue("d0"));own(nativeStatementTemp796,JSValue(209),JSValue("d1"));own(nativeStatementTemp796,JSValue(210),JSValue("d2"));own(nativeStatementTemp796,JSValue(211),JSValue("d3"));own(nativeStatementTemp796,JSValue(212),JSValue("d4"));own(nativeStatementTemp796,JSValue(213),JSValue("d5"));own(nativeStatementTemp796,JSValue(214),JSValue("d6"));own(nativeStatementTemp796,JSValue(215),JSValue("d7"));own(nativeStatementTemp796,JSValue(216),JSValue("d8"));own(nativeStatementTemp796,JSValue(217),JSValue("d9"));own(nativeStatementTemp796,JSValue(218),JSValue("da"));own(nativeStatementTemp796,JSValue(219),JSValue("db"));own(nativeStatementTemp796,JSValue(220),JSValue("dc"));own(nativeStatementTemp796,JSValue(221),JSValue("dd"));own(nativeStatementTemp796,JSValue(222),JSValue("de"));own(nativeStatementTemp796,JSValue(223),JSValue("df"));own(nativeStatementTemp796,JSValue(224),JSValue("e0"));own(nativeStatementTemp796,JSValue(225),JSValue("e1"));own(nativeStatementTemp796,JSValue(226),JSValue("e2"));own(nativeStatementTemp796,JSValue(227),JSValue("e3"));own(nativeStatementTemp796,JSValue(228),JSValue("e4"));own(nativeStatementTemp796,JSValue(229),JSValue("e5"));own(nativeStatementTemp796,JSValue(230),JSValue("e6"));own(nativeStatementTemp796,JSValue(231),JSValue("e7"));own(nativeStatementTemp796,JSValue(232),JSValue("e8"));own(nativeStatementTemp796,JSValue(233),JSValue("e9"));own(nativeStatementTemp796,JSValue(234),JSValue("ea"));own(nativeStatementTemp796,JSValue(235),JSValue("eb"));own(nativeStatementTemp796,JSValue(236),JSValue("ec"));own(nativeStatementTemp796,JSValue(237),JSValue("ed"));own(nativeStatementTemp796,JSValue(238),JSValue("ee"));own(nativeStatementTemp796,JSValue(239),JSValue("ef"));own(nativeStatementTemp796,JSValue(240),JSValue("f0"));own(nativeStatementTemp796,JSValue(241),JSValue("f1"));own(nativeStatementTemp796,JSValue(242),JSValue("f2"));own(nativeStatementTemp796,JSValue(243),JSValue("f3"));own(nativeStatementTemp796,JSValue(244),JSValue("f4"));own(nativeStatementTemp796,JSValue(245),JSValue("f5"));own(nativeStatementTemp796,JSValue(246),JSValue("f6"));own(nativeStatementTemp796,JSValue(247),JSValue("f7"));own(nativeStatementTemp796,JSValue(248),JSValue("f8"));own(nativeStatementTemp796,JSValue(249),JSValue("f9"));own(nativeStatementTemp796,JSValue(250),JSValue("fa"));own(nativeStatementTemp796,JSValue(251),JSValue("fb"));own(nativeStatementTemp796,JSValue(252),JSValue("fc"));own(nativeStatementTemp796,JSValue(253),JSValue("fd"));own(nativeStatementTemp796,JSValue(254),JSValue("fe"));own(nativeStatementTemp796,JSValue(255),JSValue("ff"));return nativeStatementTemp796;}()));
own(module,"_seed",JSValue(1234567));
own(module,"DEG2RAD",([&]()->JSValue{const auto nativeStatementTemp797=([&]()->JSValue{const auto nativeStatementTemp799=nativeGlobalBinding("Math");const auto nativeStatementTemp800=JSValue("PI");return nativeStatementTemp799.get(nativeStatementTemp800);}());const auto nativeStatementTemp798=JSValue(180);return binary("/",nativeStatementTemp797,nativeStatementTemp798);}()));
own(module,"RAD2DEG",([&]()->JSValue{const auto nativeStatementTemp801=JSValue(180);const auto nativeStatementTemp802=([&]()->JSValue{const auto nativeStatementTemp803=nativeGlobalBinding("Math");const auto nativeStatementTemp804=JSValue("PI");return nativeStatementTemp803.get(nativeStatementTemp804);}());return binary("/",nativeStatementTemp801,nativeStatementTemp802);}()));
own(module,"MathUtils",([&]()->JSValue{auto nativeStatementTemp805=JSValue::object();{const auto nativeStatementTemp806=JSValue("DEG2RAD");const auto nativeStatementTemp807=module.get("DEG2RAD");own(nativeStatementTemp805,nativeStatementTemp806,nativeStatementTemp807);}{const auto nativeStatementTemp808=JSValue("RAD2DEG");const auto nativeStatementTemp809=module.get("RAD2DEG");own(nativeStatementTemp805,nativeStatementTemp808,nativeStatementTemp809);}{const auto nativeStatementTemp810=JSValue("generateUUID");const auto nativeStatementTemp811=module.get("generateUUID");own(nativeStatementTemp805,nativeStatementTemp810,nativeStatementTemp811);}{const auto nativeStatementTemp812=JSValue("clamp");const auto nativeStatementTemp813=module.get("clamp");own(nativeStatementTemp805,nativeStatementTemp812,nativeStatementTemp813);}{const auto nativeStatementTemp814=JSValue("euclideanModulo");const auto nativeStatementTemp815=module.get("euclideanModulo");own(nativeStatementTemp805,nativeStatementTemp814,nativeStatementTemp815);}{const auto nativeStatementTemp816=JSValue("mapLinear");const auto nativeStatementTemp817=module.get("mapLinear");own(nativeStatementTemp805,nativeStatementTemp816,nativeStatementTemp817);}{const auto nativeStatementTemp818=JSValue("inverseLerp");const auto nativeStatementTemp819=module.get("inverseLerp");own(nativeStatementTemp805,nativeStatementTemp818,nativeStatementTemp819);}{const auto nativeStatementTemp820=JSValue("lerp");const auto nativeStatementTemp821=module.get("lerp");own(nativeStatementTemp805,nativeStatementTemp820,nativeStatementTemp821);}{const auto nativeStatementTemp822=JSValue("damp");const auto nativeStatementTemp823=module.get("damp");own(nativeStatementTemp805,nativeStatementTemp822,nativeStatementTemp823);}{const auto nativeStatementTemp824=JSValue("pingpong");const auto nativeStatementTemp825=module.get("pingpong");own(nativeStatementTemp805,nativeStatementTemp824,nativeStatementTemp825);}{const auto nativeStatementTemp826=JSValue("smoothstep");const auto nativeStatementTemp827=module.get("smoothstep");own(nativeStatementTemp805,nativeStatementTemp826,nativeStatementTemp827);}{const auto nativeStatementTemp828=JSValue("smootherstep");const auto nativeStatementTemp829=module.get("smootherstep");own(nativeStatementTemp805,nativeStatementTemp828,nativeStatementTemp829);}{const auto nativeStatementTemp830=JSValue("randInt");const auto nativeStatementTemp831=module.get("randInt");own(nativeStatementTemp805,nativeStatementTemp830,nativeStatementTemp831);}{const auto nativeStatementTemp832=JSValue("randFloat");const auto nativeStatementTemp833=module.get("randFloat");own(nativeStatementTemp805,nativeStatementTemp832,nativeStatementTemp833);}{const auto nativeStatementTemp834=JSValue("randFloatSpread");const auto nativeStatementTemp835=module.get("randFloatSpread");own(nativeStatementTemp805,nativeStatementTemp834,nativeStatementTemp835);}{const auto nativeStatementTemp836=JSValue("seededRandom");const auto nativeStatementTemp837=module.get("seededRandom");own(nativeStatementTemp805,nativeStatementTemp836,nativeStatementTemp837);}{const auto nativeStatementTemp838=JSValue("degToRad");const auto nativeStatementTemp839=module.get("degToRad");own(nativeStatementTemp805,nativeStatementTemp838,nativeStatementTemp839);}{const auto nativeStatementTemp840=JSValue("radToDeg");const auto nativeStatementTemp841=module.get("radToDeg");own(nativeStatementTemp805,nativeStatementTemp840,nativeStatementTemp841);}{const auto nativeStatementTemp842=JSValue("isPowerOfTwo");const auto nativeStatementTemp843=module.get("isPowerOfTwo");own(nativeStatementTemp805,nativeStatementTemp842,nativeStatementTemp843);}{const auto nativeStatementTemp844=JSValue("ceilPowerOfTwo");const auto nativeStatementTemp845=module.get("ceilPowerOfTwo");own(nativeStatementTemp805,nativeStatementTemp844,nativeStatementTemp845);}{const auto nativeStatementTemp846=JSValue("floorPowerOfTwo");const auto nativeStatementTemp847=module.get("floorPowerOfTwo");own(nativeStatementTemp805,nativeStatementTemp846,nativeStatementTemp847);}{const auto nativeStatementTemp848=JSValue("setQuaternionFromProperEuler");const auto nativeStatementTemp849=module.get("setQuaternionFromProperEuler");own(nativeStatementTemp805,nativeStatementTemp848,nativeStatementTemp849);}{const auto nativeStatementTemp850=JSValue("normalize");const auto nativeStatementTemp851=module.get("normalize");own(nativeStatementTemp805,nativeStatementTemp850,nativeStatementTemp851);}{const auto nativeStatementTemp852=JSValue("denormalize");const auto nativeStatementTemp853=module.get("denormalize");own(nativeStatementTemp805,nativeStatementTemp852,nativeStatementTemp853);}return nativeStatementTemp805;}()));
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();
own(facade,"DEG2RAD",module.get("DEG2RAD"));
own(facade,"RAD2DEG",module.get("RAD2DEG"));
own(facade,"generateUUID",module.get("generateUUID"));
own(facade,"clamp",module.get("clamp"));
own(facade,"euclideanModulo",module.get("euclideanModulo"));
own(facade,"mapLinear",module.get("mapLinear"));
own(facade,"inverseLerp",module.get("inverseLerp"));
own(facade,"lerp",module.get("lerp"));
own(facade,"damp",module.get("damp"));
own(facade,"pingpong",module.get("pingpong"));
own(facade,"smoothstep",module.get("smoothstep"));
own(facade,"smootherstep",module.get("smootherstep"));
own(facade,"randInt",module.get("randInt"));
own(facade,"randFloat",module.get("randFloat"));
own(facade,"randFloatSpread",module.get("randFloatSpread"));
own(facade,"seededRandom",module.get("seededRandom"));
own(facade,"degToRad",module.get("degToRad"));
own(facade,"radToDeg",module.get("radToDeg"));
own(facade,"isPowerOfTwo",module.get("isPowerOfTwo"));
own(facade,"ceilPowerOfTwo",module.get("ceilPowerOfTwo"));
own(facade,"floorPowerOfTwo",module.get("floorPowerOfTwo"));
own(facade,"setQuaternionFromProperEuler",module.get("setQuaternionFromProperEuler"));
own(facade,"normalize",module.get("normalize"));
own(facade,"denormalize",module.get("denormalize"));
own(facade,"MathUtils",module.get("MathUtils"));
facade.retainForHeapLifetime();module.setInternalSlot("MathUtils.facade",facade);module.setInternalSlot("MathUtils.status","evaluated");return facade;}catch(...){module.setInternalSlot("MathUtils.error",nativeExceptionValue(std::current_exception()));module.setInternalSlot("MathUtils.status","failed");throw JSThrownValue(module.getInternalSlot("MathUtils.error"));}}
JSValue nativeMathUtilsBinding(const std::string& name){const auto keeper=nativeMathUtilsModule();return FullMathUtilsManaged::lexical().lock()->get(name);}
JSValue nativeMathUtilsGenerateUUID(){return nativeMathUtilsBinding("generateUUID");}
JSValue nativeMathUtilsClamp(){return nativeMathUtilsBinding("clamp");}
