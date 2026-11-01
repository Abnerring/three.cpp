#include "math/MeshMathDependencies.h"
#include "math/Vector4.h"
#include "math/JSNumberMath.h"
#include "core/VertexAttribute.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "core/JSModuleObject.h"

Vector4::Vector4(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
    : x(x), y(y), z(z), w(w)
{
}

Vector4::~Vector4()
{
}

Vector4 &Vector4::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
    return *this;
}

Vector4 &Vector4::setScalar(HIGH_PRECISION scalar)
{
    this->x = this->y = this->z = this->w = scalar;
    return *this;
}

Vector4 &Vector4::setX(HIGH_PRECISION x) { this->x = x; return *this; }
Vector4 &Vector4::setY(HIGH_PRECISION y) { this->y = y; return *this; }
Vector4 &Vector4::setZ(HIGH_PRECISION z) { this->z = z; return *this; }
Vector4 &Vector4::setW(HIGH_PRECISION w) { this->w = w; return *this; }

Vector4 &Vector4::setComponent(size_t index, HIGH_PRECISION value)
{
    switch (index)
    {
    case 0: this->x = value; break;
    case 1: this->y = value; break;
    case 2: this->z = value; break;
    case 3: this->w = value; break;
    default: throw std::out_of_range("Index out of range [0,3]");
    }
    return *this;
}

HIGH_PRECISION Vector4::getComponent(size_t index) const
{
    switch (index)
    {
    case 0: return this->x;
    case 1: return this->y;
    case 2: return this->z;
    case 3: return this->w;
    default: throw std::out_of_range("Index out of range [0,3]");
    }
}

Vector4 Vector4::clone() const
{
    return Vector4(this->x, this->y, this->z, this->w);
}

Vector4 &Vector4::copy(const Vector4 &v)
{
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    this->w = v.w;
    return *this;
}

Vector4 &Vector4::add(const Vector4 &v)
{
    this->x += v.x;
    this->y += v.y;
    this->z += v.z;
    this->w += v.w;
    return *this;
}

Vector4 &Vector4::addScalar(HIGH_PRECISION s)
{
    this->x += s;
    this->y += s;
    this->z += s;
    this->w += s;
    return *this;
}

Vector4 &Vector4::addVectors(const Vector4 &a, const Vector4 &b)
{
    this->x = a.x + b.x;
    this->y = a.y + b.y;
    this->z = a.z + b.z;
    this->w = a.w + b.w;
    return *this;
}

Vector4 &Vector4::addScaledVector(const Vector4 &v, HIGH_PRECISION s)
{
    this->x += v.x * s;
    this->y += v.y * s;
    this->z += v.z * s;
    this->w += v.w * s;
    return *this;
}

Vector4 &Vector4::sub(const Vector4 &v)
{
    this->x -= v.x;
    this->y -= v.y;
    this->z -= v.z;
    this->w -= v.w;
    return *this;
}

Vector4 &Vector4::subScalar(HIGH_PRECISION s)
{
    this->x -= s;
    this->y -= s;
    this->z -= s;
    this->w -= s;
    return *this;
}

Vector4 &Vector4::subVectors(const Vector4 &a, const Vector4 &b)
{
    this->x = a.x - b.x;
    this->y = a.y - b.y;
    this->z = a.z - b.z;
    this->w = a.w - b.w;
    return *this;
}

Vector4 &Vector4::multiply(const Vector4 &v)
{
    this->x *= v.x;
    this->y *= v.y;
    this->z *= v.z;
    this->w *= v.w;
    return *this;
}

Vector4 &Vector4::multiplyScalar(HIGH_PRECISION scalar)
{
    this->x *= scalar;
    this->y *= scalar;
    this->z *= scalar;
    this->w *= scalar;
    return *this;
}

Vector4 &Vector4::applyMatrix4(const Matrix4 &m)
{
    auto e = m.elements();
    HIGH_PRECISION x = this->x, y = this->y, z = this->z, w = this->w;

    this->x = e[0] * x + e[4] * y + e[8] * z + e[12] * w;
    this->y = e[1] * x + e[5] * y + e[9] * z + e[13] * w;
    this->z = e[2] * x + e[6] * y + e[10] * z + e[14] * w;
    this->w = e[3] * x + e[7] * y + e[11] * z + e[15] * w;

    return *this;
}

Vector4 &Vector4::divide(const Vector4 &v)
{
    this->x /= v.x;
    this->y /= v.y;
    this->z /= v.z;
    this->w /= v.w;
    return *this;
}

Vector4 &Vector4::divideScalar(HIGH_PRECISION scalar)
{
    return multiplyScalar(1.0 / scalar);
}

Vector4 &Vector4::min(const Vector4 &v)
{
    this->x = MathUtils::min(this->x, v.x);
    this->y = MathUtils::min(this->y, v.y);
    this->z = MathUtils::min(this->z, v.z);
    this->w = MathUtils::min(this->w, v.w);
    return *this;
}

Vector4 &Vector4::max(const Vector4 &v)
{
    this->x = MathUtils::max(this->x, v.x);
    this->y = MathUtils::max(this->y, v.y);
    this->z = MathUtils::max(this->z, v.z);
    this->w = MathUtils::max(this->w, v.w);
    return *this;
}

Vector4 &Vector4::clamp(const Vector4 &min, const Vector4 &max)
{
    this->x = MathUtils::clamp(this->x, min.x, max.x);
    this->y = MathUtils::clamp(this->y, min.y, max.y);
    this->z = MathUtils::clamp(this->z, min.z, max.z);
    this->w = MathUtils::clamp(this->w, min.w, max.w);
    return *this;
}

Vector4 &Vector4::clampScalar(HIGH_PRECISION minVal, HIGH_PRECISION maxVal)
{
    this->x = MathUtils::clamp(this->x, minVal, maxVal);
    this->y = MathUtils::clamp(this->y, minVal, maxVal);
    this->z = MathUtils::clamp(this->z, minVal, maxVal);
    this->w = MathUtils::clamp(this->w, minVal, maxVal);
    return *this;
}

Vector4 &Vector4::clampLength(HIGH_PRECISION min, HIGH_PRECISION max)
{
    auto len = length();
    divideScalar((len == 0 || std::isnan(len)) ? 1.0 : len);
    multiplyScalar(MathUtils::clamp(len, min, max));
    return *this;
}

Vector4 &Vector4::floor()
{
    this->x = std::floor(this->x);
    this->y = std::floor(this->y);
    this->z = std::floor(this->z);
    this->w = std::floor(this->w);
    return *this;
}

Vector4 &Vector4::ceil()
{
    this->x = std::ceil(this->x);
    this->y = std::ceil(this->y);
    this->z = std::ceil(this->z);
    this->w = std::ceil(this->w);
    return *this;
}

Vector4 &Vector4::round()
{
    this->x = MathUtils::round(this->x);
    this->y = MathUtils::round(this->y);
    this->z = MathUtils::round(this->z);
    this->w = MathUtils::round(this->w);
    return *this;
}

Vector4 &Vector4::roundToZero()
{
    this->x = std::trunc(this->x);
    this->y = std::trunc(this->y);
    this->z = std::trunc(this->z);
    this->w = std::trunc(this->w);
    return *this;
}

Vector4 &Vector4::negate()
{
    this->x = -this->x;
    this->y = -this->y;
    this->z = -this->z;
    this->w = -this->w;
    return *this;
}

HIGH_PRECISION Vector4::dot(const Vector4 &v) const
{
    return this->x * v.x + this->y * v.y + this->z * v.z + this->w * v.w;
}

HIGH_PRECISION Vector4::lengthSq() const
{
    return this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w;
}

HIGH_PRECISION Vector4::length() const
{
    return std::sqrt(lengthSq());
}

HIGH_PRECISION Vector4::manhattanLength() const
{
    return std::abs(this->x) + std::abs(this->y) + std::abs(this->z) + std::abs(this->w);
}

Vector4 &Vector4::normalize()
{
    auto len = length();
    return divideScalar((len == 0 || std::isnan(len)) ? 1.0 : len);
}

Vector4 &Vector4::setLength(HIGH_PRECISION length)
{
    normalize();
    return multiplyScalar(length);
}

Vector4 &Vector4::lerp(const Vector4 &v, HIGH_PRECISION alpha)
{
    this->x += (v.x - this->x) * alpha;
    this->y += (v.y - this->y) * alpha;
    this->z += (v.z - this->z) * alpha;
    this->w += (v.w - this->w) * alpha;
    return *this;
}

Vector4 &Vector4::lerpVectors(const Vector4 &v1, const Vector4 &v2, HIGH_PRECISION alpha)
{
    this->x = v1.x + (v2.x - v1.x) * alpha;
    this->y = v1.y + (v2.y - v1.y) * alpha;
    this->z = v1.z + (v2.z - v1.z) * alpha;
    this->w = v1.w + (v2.w - v1.w) * alpha;
    return *this;
}

bool Vector4::equals(const Vector4 &v, HIGH_PRECISION epsilon) const
{
    if (epsilon == 0) return this->x == v.x && this->y == v.y && this->z == v.z && this->w == v.w;
    return std::abs(this->x - v.x) <= epsilon && std::abs(this->y - v.y) <= epsilon && std::abs(this->z - v.z) <= epsilon && std::abs(this->w - v.w) <= epsilon;
}

Vector4 &Vector4::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    this->x = array[offset];
    this->y = array[offset + 1];
    this->z = array[offset + 2];
    this->w = array[offset + 3];
    return *this;
}

std::vector<HIGH_PRECISION> &Vector4::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 4) array.resize(offset + 4);
    array[offset] = this->x;
    array[offset + 1] = this->y;
    array[offset + 2] = this->z;
    array[offset + 3] = this->w;
    return array;
}

Vector4 &Vector4::fromMatrixColumn(const Matrix4 &m, size_t index)
{
    fromArray(std::vector<HIGH_PRECISION>(m.elements().begin(), m.elements().end()), index * 4);
    return *this;
}

Vector4 &Vector4::setFromMatrixPosition(const Matrix4 &m)
{
    auto e = m.elements();
    this->x = e[12];
    this->y = e[13];
    this->z = e[14];
    this->w = e[15];
    return *this;
}

Vector4 &Vector4::setFromMatrixScale(const Matrix4 &m)
{
    auto e = m.elements();
    this->x = std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
    this->y = std::sqrt(e[4] * e[4] + e[5] * e[5] + e[6] * e[6]);
    this->z = std::sqrt(e[8] * e[8] + e[9] * e[9] + e[10] * e[10]);
    this->w = 1.0;
    return *this;
}

Vector4 &Vector4::random()
{
    this->x = DIST_DOUBLE(RANDOM_ENGINE);
    this->y = DIST_DOUBLE(RANDOM_ENGINE);
    this->z = DIST_DOUBLE(RANDOM_ENGINE);
    this->w = DIST_DOUBLE(RANDOM_ENGINE);
    return *this;
}

// Operators
Vector4 Vector4::operator+(const Vector4 &v) const { return clone().add(v); }
Vector4 Vector4::operator-(const Vector4 &v) const { return clone().sub(v); }
Vector4 Vector4::operator*(const Vector4 &v) const { return clone().multiply(v); }
Vector4 Vector4::operator/(const Vector4 &v) const { return clone().divide(v); }
Vector4 Vector4::operator*(HIGH_PRECISION scalar) const { return clone().multiplyScalar(scalar); }
Vector4 Vector4::operator/(HIGH_PRECISION scalar) const { return clone().divideScalar(scalar); }

Vector4 &Vector4::operator=(const Vector4 &v) { copy(v);
    return *this;
}
bool Vector4::operator==(const Vector4 &v) const { return equals(v); }
HIGH_PRECISION Vector4::operator[](size_t index) const { return getComponent(index); }

std::vector<HIGH_PRECISION> Vector4::toArray() const
{
    std::vector<HIGH_PRECISION> array;
    toArray(array);
    return array;
}

Vector4 &Vector4::setAxisAngleFromQuaternion(const Quaternion &q)
{


		// q is assumed to be normalized

		this->w = 2 * JSNumberMath::acos( q.w );

		const auto s = std::sqrt( 1 - q.w * q.w );

		if ( s < 0.0001 ) {

			this->x = 1;
			this->y = 0;
			this->z = 0;

		} else {

			this->x = q.x / s;
			this->y = q.y / s;
			this->z = q.z / s;

		}

		return *this;


}

Vector4 &Vector4::setAxisAngleFromRotationMatrix(const Matrix4 &m)
{


		// assumes the upper 3x3 of m is a pure rotation matrix (i.e, unscaled)

		HIGH_PRECISION angle, x, y, z; // variables for result
		const auto te = m.elements();
        const auto epsilon = 0.01,		// margin to allow for rounding errors
			epsilon2 = 0.1,		// margin to distinguish between 0 and 180 degrees



			m11 = te[ 0 ], m12 = te[ 4 ], m13 = te[ 8 ],
			m21 = te[ 1 ], m22 = te[ 5 ], m23 = te[ 9 ],
			m31 = te[ 2 ], m32 = te[ 6 ], m33 = te[ 10 ];

		if ( ( std::abs( m12 - m21 ) < epsilon ) &&
		     ( std::abs( m13 - m31 ) < epsilon ) &&
		     ( std::abs( m23 - m32 ) < epsilon ) ) {

			// singularity found
			// first check for identity matrix which must have +1 for all terms
			// in leading diagonal and zero in other terms

			if ( ( std::abs( m12 + m21 ) < epsilon2 ) &&
			     ( std::abs( m13 + m31 ) < epsilon2 ) &&
			     ( std::abs( m23 + m32 ) < epsilon2 ) &&
			     ( std::abs( m11 + m22 + m33 - 3 ) < epsilon2 ) ) {

				// this singularity is identity matrix so angle = 0

				set( 1, 0, 0, 0 );

				return *this; // zero angle, arbitrary axis

			}

			// otherwise this singularity is angle = 180

			angle = MATH_PI;

			const auto xx = ( m11 + 1 ) / 2;
			const auto yy = ( m22 + 1 ) / 2;
			const auto zz = ( m33 + 1 ) / 2;
			const auto xy = ( m12 + m21 ) / 4;
			const auto xz = ( m13 + m31 ) / 4;
			const auto yz = ( m23 + m32 ) / 4;

			if ( ( xx > yy ) && ( xx > zz ) ) {

				// m11 is the largest diagonal term

				if ( xx < epsilon ) {

					x = 0;
					y = 0.707106781;
					z = 0.707106781;

				} else {

					x = std::sqrt( xx );
					y = xy / x;
					z = xz / x;

				}

			} else if ( yy > zz ) {

				// m22 is the largest diagonal term

				if ( yy < epsilon ) {

					x = 0.707106781;
					y = 0;
					z = 0.707106781;

				} else {

					y = std::sqrt( yy );
					x = xy / y;
					z = yz / y;

				}

			} else {

				// m33 is the largest diagonal term so base result on this

				if ( zz < epsilon ) {

					x = 0.707106781;
					y = 0.707106781;
					z = 0;

				} else {

					z = std::sqrt( zz );
					x = xz / z;
					y = yz / z;

				}

			}

			set( x, y, z, angle );

			return *this; // return 180 deg rotation

		}

		// as we have reached here there are no singularities so we can handle normally

		auto s = std::sqrt( ( m32 - m23 ) * ( m32 - m23 ) +
			( m13 - m31 ) * ( m13 - m31 ) +
			( m21 - m12 ) * ( m21 - m12 ) ); // used to normalize

		if ( std::abs( s ) < 0.001 ) s = 1;

		// prevent divide by zero, should not happen if matrix is orthogonal and should be
		// caught by singularity test above, but I've left it in just in case

		this->x = ( m32 - m23 ) / s;
		this->y = ( m13 - m31 ) / s;
		this->z = ( m21 - m12 ) / s;
		this->w = JSNumberMath::acos( ( m11 + m22 + m33 - 1 ) / 2 );

		return *this;


}

Vector4 &Vector4::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z) { return set(x, y, z, this->w); }
Vector4 &Vector4::copy(const Vector3 &v) { return set(v.x, v.y, v.z, 1); }
Vector4 &Vector4::setFromMatrixColumn(const Matrix4 &m, size_t index) { return fromMatrixColumn(m, index); }

Vector4 &Vector4::fromBufferAttribute(const VertexAttribute &attribute, double index)
{
    x = attribute.getX(index);
    y = attribute.getY(index);
    z = attribute.getZ(index);
    w = attribute.getW(index);
    return *this;
}

Vector4::Vector4(const Vector4& source) : Vector4(source.x, source.y, source.z, source.w) {}
Vector4::Vector4(Vector4&& source) : Vector4(static_cast<const Vector4&>(source)) {}

// Full source-managed vectors. Native C++ statements; see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSGenerator.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "math/MathUtils.h"
#include "math/Quaternion.h"
#include "math/JSNumberMath.h"
#include <bit>
#include <cmath>
#include <string_view>
#include <unordered_set>

#include "math/Vector4RuntimeSource.inc"
namespace Vector4Managed{

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
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
JSValue x=arg(nativeStatementTemp1,0);
if(x.isUndefined())x=JSValue(0);
JSValue y=arg(nativeStatementTemp1,1);
if(y.isUndefined())y=JSValue(0);
JSValue z=arg(nativeStatementTemp1,2);
if(z.isUndefined())z=JSValue(0);
JSValue w=arg(nativeStatementTemp1,3);
if(w.isUndefined())w=JSValue(1);
([&]()->JSValue{auto nativeStatementTemp6=nativeStatementTemp4;const auto nativeStatementTemp7=JSValue("x");const auto nativeStatementTemp8=x;setReference(nativeStatementTemp6,nativeStatementTemp7,nativeStatementTemp8);return nativeStatementTemp8;}());
([&]()->JSValue{auto nativeStatementTemp11=nativeStatementTemp4;const auto nativeStatementTemp12=JSValue("y");const auto nativeStatementTemp13=y;setReference(nativeStatementTemp11,nativeStatementTemp12,nativeStatementTemp13);return nativeStatementTemp13;}());
([&]()->JSValue{auto nativeStatementTemp16=nativeStatementTemp4;const auto nativeStatementTemp17=JSValue("z");const auto nativeStatementTemp18=z;setReference(nativeStatementTemp16,nativeStatementTemp17,nativeStatementTemp18);return nativeStatementTemp18;}());
([&]()->JSValue{auto nativeStatementTemp21=nativeStatementTemp4;const auto nativeStatementTemp22=JSValue("w");const auto nativeStatementTemp23=w;setReference(nativeStatementTemp21,nativeStatementTemp22,nativeStatementTemp23);return nativeStatementTemp23;}());
return JSValue{};
}
JSValue method1(const JSValue& nativeStatementTemp26,const Args& nativeStatementTemp27,const Args& nativeStatementTemp28){
auto nativeStatementTemp29=nativeStatementTemp28[0];auto nativeStatementTemp30=nativeStatementTemp26;
return ([&]()->JSValue{const auto nativeStatementTemp32=nativeStatementTemp30;const auto nativeStatementTemp33=JSValue("z");return nativeStatementTemp32.get(nativeStatementTemp33);}());
return JSValue{};
}
JSValue method2(const JSValue& nativeStatementTemp34,const Args& nativeStatementTemp35,const Args& nativeStatementTemp36){
auto nativeStatementTemp37=nativeStatementTemp36[0];auto nativeStatementTemp38=nativeStatementTemp34;
JSValue value=arg(nativeStatementTemp35,0);
([&]()->JSValue{auto nativeStatementTemp40=nativeStatementTemp38;const auto nativeStatementTemp41=JSValue("z");const auto nativeStatementTemp42=value;setReference(nativeStatementTemp40,nativeStatementTemp41,nativeStatementTemp42);return nativeStatementTemp42;}());
return JSValue{};
}
JSValue method3(const JSValue& nativeStatementTemp45,const Args& nativeStatementTemp46,const Args& nativeStatementTemp47){
auto nativeStatementTemp48=nativeStatementTemp47[0];auto nativeStatementTemp49=nativeStatementTemp45;
return ([&]()->JSValue{const auto nativeStatementTemp51=nativeStatementTemp49;const auto nativeStatementTemp52=JSValue("w");return nativeStatementTemp51.get(nativeStatementTemp52);}());
return JSValue{};
}
JSValue method4(const JSValue& nativeStatementTemp53,const Args& nativeStatementTemp54,const Args& nativeStatementTemp55){
auto nativeStatementTemp56=nativeStatementTemp55[0];auto nativeStatementTemp57=nativeStatementTemp53;
JSValue value=arg(nativeStatementTemp54,0);
([&]()->JSValue{auto nativeStatementTemp59=nativeStatementTemp57;const auto nativeStatementTemp60=JSValue("w");const auto nativeStatementTemp61=value;setReference(nativeStatementTemp59,nativeStatementTemp60,nativeStatementTemp61);return nativeStatementTemp61;}());
return JSValue{};
}
JSValue method5(const JSValue& nativeStatementTemp64,const Args& nativeStatementTemp65,const Args& nativeStatementTemp66){
auto nativeStatementTemp67=nativeStatementTemp66[0];auto nativeStatementTemp68=nativeStatementTemp64;
JSValue x=arg(nativeStatementTemp65,0);
JSValue y=arg(nativeStatementTemp65,1);
JSValue z=arg(nativeStatementTemp65,2);
JSValue w=arg(nativeStatementTemp65,3);
([&]()->JSValue{auto nativeStatementTemp70=nativeStatementTemp68;const auto nativeStatementTemp71=JSValue("x");const auto nativeStatementTemp72=x;setReference(nativeStatementTemp70,nativeStatementTemp71,nativeStatementTemp72);return nativeStatementTemp72;}());
([&]()->JSValue{auto nativeStatementTemp75=nativeStatementTemp68;const auto nativeStatementTemp76=JSValue("y");const auto nativeStatementTemp77=y;setReference(nativeStatementTemp75,nativeStatementTemp76,nativeStatementTemp77);return nativeStatementTemp77;}());
([&]()->JSValue{auto nativeStatementTemp80=nativeStatementTemp68;const auto nativeStatementTemp81=JSValue("z");const auto nativeStatementTemp82=z;setReference(nativeStatementTemp80,nativeStatementTemp81,nativeStatementTemp82);return nativeStatementTemp82;}());
([&]()->JSValue{auto nativeStatementTemp85=nativeStatementTemp68;const auto nativeStatementTemp86=JSValue("w");const auto nativeStatementTemp87=w;setReference(nativeStatementTemp85,nativeStatementTemp86,nativeStatementTemp87);return nativeStatementTemp87;}());
return nativeStatementTemp68;
return JSValue{};
}
JSValue method6(const JSValue& nativeStatementTemp90,const Args& nativeStatementTemp91,const Args& nativeStatementTemp92){
auto nativeStatementTemp93=nativeStatementTemp92[0];auto nativeStatementTemp94=nativeStatementTemp90;
JSValue scalar=arg(nativeStatementTemp91,0);
([&]()->JSValue{auto nativeStatementTemp96=nativeStatementTemp94;const auto nativeStatementTemp97=JSValue("x");const auto nativeStatementTemp98=scalar;setReference(nativeStatementTemp96,nativeStatementTemp97,nativeStatementTemp98);return nativeStatementTemp98;}());
([&]()->JSValue{auto nativeStatementTemp101=nativeStatementTemp94;const auto nativeStatementTemp102=JSValue("y");const auto nativeStatementTemp103=scalar;setReference(nativeStatementTemp101,nativeStatementTemp102,nativeStatementTemp103);return nativeStatementTemp103;}());
([&]()->JSValue{auto nativeStatementTemp106=nativeStatementTemp94;const auto nativeStatementTemp107=JSValue("z");const auto nativeStatementTemp108=scalar;setReference(nativeStatementTemp106,nativeStatementTemp107,nativeStatementTemp108);return nativeStatementTemp108;}());
([&]()->JSValue{auto nativeStatementTemp111=nativeStatementTemp94;const auto nativeStatementTemp112=JSValue("w");const auto nativeStatementTemp113=scalar;setReference(nativeStatementTemp111,nativeStatementTemp112,nativeStatementTemp113);return nativeStatementTemp113;}());
return nativeStatementTemp94;
return JSValue{};
}
JSValue method7(const JSValue& nativeStatementTemp116,const Args& nativeStatementTemp117,const Args& nativeStatementTemp118){
auto nativeStatementTemp119=nativeStatementTemp118[0];auto nativeStatementTemp120=nativeStatementTemp116;
JSValue x=arg(nativeStatementTemp117,0);
([&]()->JSValue{auto nativeStatementTemp122=nativeStatementTemp120;const auto nativeStatementTemp123=JSValue("x");const auto nativeStatementTemp124=x;setReference(nativeStatementTemp122,nativeStatementTemp123,nativeStatementTemp124);return nativeStatementTemp124;}());
return nativeStatementTemp120;
return JSValue{};
}
JSValue method8(const JSValue& nativeStatementTemp127,const Args& nativeStatementTemp128,const Args& nativeStatementTemp129){
auto nativeStatementTemp130=nativeStatementTemp129[0];auto nativeStatementTemp131=nativeStatementTemp127;
JSValue y=arg(nativeStatementTemp128,0);
([&]()->JSValue{auto nativeStatementTemp133=nativeStatementTemp131;const auto nativeStatementTemp134=JSValue("y");const auto nativeStatementTemp135=y;setReference(nativeStatementTemp133,nativeStatementTemp134,nativeStatementTemp135);return nativeStatementTemp135;}());
return nativeStatementTemp131;
return JSValue{};
}
JSValue method9(const JSValue& nativeStatementTemp138,const Args& nativeStatementTemp139,const Args& nativeStatementTemp140){
auto nativeStatementTemp141=nativeStatementTemp140[0];auto nativeStatementTemp142=nativeStatementTemp138;
JSValue z=arg(nativeStatementTemp139,0);
([&]()->JSValue{auto nativeStatementTemp144=nativeStatementTemp142;const auto nativeStatementTemp145=JSValue("z");const auto nativeStatementTemp146=z;setReference(nativeStatementTemp144,nativeStatementTemp145,nativeStatementTemp146);return nativeStatementTemp146;}());
return nativeStatementTemp142;
return JSValue{};
}
JSValue method10(const JSValue& nativeStatementTemp149,const Args& nativeStatementTemp150,const Args& nativeStatementTemp151){
auto nativeStatementTemp152=nativeStatementTemp151[0];auto nativeStatementTemp153=nativeStatementTemp149;
JSValue w=arg(nativeStatementTemp150,0);
([&]()->JSValue{auto nativeStatementTemp155=nativeStatementTemp153;const auto nativeStatementTemp156=JSValue("w");const auto nativeStatementTemp157=w;setReference(nativeStatementTemp155,nativeStatementTemp156,nativeStatementTemp157);return nativeStatementTemp157;}());
return nativeStatementTemp153;
return JSValue{};
}
JSValue method11(const JSValue& nativeStatementTemp160,const Args& nativeStatementTemp161,const Args& nativeStatementTemp162){
auto nativeStatementTemp163=nativeStatementTemp162[0];auto nativeStatementTemp164=nativeStatementTemp160;
JSValue index=arg(nativeStatementTemp161,0);
JSValue value=arg(nativeStatementTemp161,1);
{const auto nativeStatementTemp166=index;int nativeStatementTemp167=-1;
if(nativeStatementTemp167==-1&&nativeStatementTemp166.strictEquals(JSValue(0)))nativeStatementTemp167=0;
if(nativeStatementTemp167==-1&&nativeStatementTemp166.strictEquals(JSValue(1)))nativeStatementTemp167=1;
if(nativeStatementTemp167==-1&&nativeStatementTemp166.strictEquals(JSValue(2)))nativeStatementTemp167=2;
if(nativeStatementTemp167==-1&&nativeStatementTemp166.strictEquals(JSValue(3)))nativeStatementTemp167=3;
if(nativeStatementTemp167==-1)nativeStatementTemp167=4;
switch(nativeStatementTemp167){
case 0:
([&]()->JSValue{auto nativeStatementTemp168=nativeStatementTemp164;const auto nativeStatementTemp169=JSValue("x");const auto nativeStatementTemp170=value;setReference(nativeStatementTemp168,nativeStatementTemp169,nativeStatementTemp170);return nativeStatementTemp170;}());
break;
case 1:
([&]()->JSValue{auto nativeStatementTemp173=nativeStatementTemp164;const auto nativeStatementTemp174=JSValue("y");const auto nativeStatementTemp175=value;setReference(nativeStatementTemp173,nativeStatementTemp174,nativeStatementTemp175);return nativeStatementTemp175;}());
break;
case 2:
([&]()->JSValue{auto nativeStatementTemp178=nativeStatementTemp164;const auto nativeStatementTemp179=JSValue("z");const auto nativeStatementTemp180=value;setReference(nativeStatementTemp178,nativeStatementTemp179,nativeStatementTemp180);return nativeStatementTemp180;}());
break;
case 3:
([&]()->JSValue{auto nativeStatementTemp183=nativeStatementTemp164;const auto nativeStatementTemp184=JSValue("w");const auto nativeStatementTemp185=value;setReference(nativeStatementTemp183,nativeStatementTemp184,nativeStatementTemp185);return nativeStatementTemp185;}());
break;
case 4:
throw JSThrownValue(([&]()->JSValue{const auto nativeStatementTemp188=nativeGlobalBinding("Error");Args nativeStatementTemp189;nativeStatementTemp189.push_back(([&]()->JSValue{const auto nativeStatementTemp190=JSValue("THREE.Vector4: index is out of range: ");const auto nativeStatementTemp191=index;return binary("+",nativeStatementTemp190,nativeStatementTemp191);}()));return nativeStatementTemp188.construct(nativeStatementTemp189);}()));
}
}
return nativeStatementTemp164;
return JSValue{};
}
JSValue method12(const JSValue& nativeStatementTemp192,const Args& nativeStatementTemp193,const Args& nativeStatementTemp194){
auto nativeStatementTemp195=nativeStatementTemp194[0];auto nativeStatementTemp196=nativeStatementTemp192;
JSValue index=arg(nativeStatementTemp193,0);
{const auto nativeStatementTemp198=index;int nativeStatementTemp199=-1;
if(nativeStatementTemp199==-1&&nativeStatementTemp198.strictEquals(JSValue(0)))nativeStatementTemp199=0;
if(nativeStatementTemp199==-1&&nativeStatementTemp198.strictEquals(JSValue(1)))nativeStatementTemp199=1;
if(nativeStatementTemp199==-1&&nativeStatementTemp198.strictEquals(JSValue(2)))nativeStatementTemp199=2;
if(nativeStatementTemp199==-1&&nativeStatementTemp198.strictEquals(JSValue(3)))nativeStatementTemp199=3;
if(nativeStatementTemp199==-1)nativeStatementTemp199=4;
switch(nativeStatementTemp199){
case 0:
return ([&]()->JSValue{const auto nativeStatementTemp200=nativeStatementTemp196;const auto nativeStatementTemp201=JSValue("x");return nativeStatementTemp200.get(nativeStatementTemp201);}());
case 1:
return ([&]()->JSValue{const auto nativeStatementTemp202=nativeStatementTemp196;const auto nativeStatementTemp203=JSValue("y");return nativeStatementTemp202.get(nativeStatementTemp203);}());
case 2:
return ([&]()->JSValue{const auto nativeStatementTemp204=nativeStatementTemp196;const auto nativeStatementTemp205=JSValue("z");return nativeStatementTemp204.get(nativeStatementTemp205);}());
case 3:
return ([&]()->JSValue{const auto nativeStatementTemp206=nativeStatementTemp196;const auto nativeStatementTemp207=JSValue("w");return nativeStatementTemp206.get(nativeStatementTemp207);}());
case 4:
throw JSThrownValue(([&]()->JSValue{const auto nativeStatementTemp208=nativeGlobalBinding("Error");Args nativeStatementTemp209;nativeStatementTemp209.push_back(([&]()->JSValue{const auto nativeStatementTemp210=JSValue("THREE.Vector4: index is out of range: ");const auto nativeStatementTemp211=index;return binary("+",nativeStatementTemp210,nativeStatementTemp211);}()));return nativeStatementTemp208.construct(nativeStatementTemp209);}()));
}
}
return JSValue{};
}
JSValue method13(const JSValue& nativeStatementTemp212,const Args& nativeStatementTemp213,const Args& nativeStatementTemp214){
auto nativeStatementTemp215=nativeStatementTemp214[0];auto nativeStatementTemp216=nativeStatementTemp212;
return ([&]()->JSValue{const auto nativeStatementTemp218=([&]()->JSValue{const auto nativeStatementTemp220=nativeStatementTemp216;const auto nativeStatementTemp221=JSValue("constructor");return nativeStatementTemp220.get(nativeStatementTemp221);}());Args nativeStatementTemp219;nativeStatementTemp219.push_back(([&]()->JSValue{const auto nativeStatementTemp222=nativeStatementTemp216;const auto nativeStatementTemp223=JSValue("x");return nativeStatementTemp222.get(nativeStatementTemp223);}()));nativeStatementTemp219.push_back(([&]()->JSValue{const auto nativeStatementTemp224=nativeStatementTemp216;const auto nativeStatementTemp225=JSValue("y");return nativeStatementTemp224.get(nativeStatementTemp225);}()));nativeStatementTemp219.push_back(([&]()->JSValue{const auto nativeStatementTemp226=nativeStatementTemp216;const auto nativeStatementTemp227=JSValue("z");return nativeStatementTemp226.get(nativeStatementTemp227);}()));nativeStatementTemp219.push_back(([&]()->JSValue{const auto nativeStatementTemp228=nativeStatementTemp216;const auto nativeStatementTemp229=JSValue("w");return nativeStatementTemp228.get(nativeStatementTemp229);}()));return nativeStatementTemp218.construct(nativeStatementTemp219);}());
return JSValue{};
}
JSValue method14(const JSValue& nativeStatementTemp230,const Args& nativeStatementTemp231,const Args& nativeStatementTemp232){
auto nativeStatementTemp233=nativeStatementTemp232[0];auto nativeStatementTemp234=nativeStatementTemp230;
JSValue v=arg(nativeStatementTemp231,0);
([&]()->JSValue{auto nativeStatementTemp236=nativeStatementTemp234;const auto nativeStatementTemp237=JSValue("x");const auto nativeStatementTemp238=([&]()->JSValue{const auto nativeStatementTemp241=v;const auto nativeStatementTemp242=JSValue("x");return nativeStatementTemp241.get(nativeStatementTemp242);}());setReference(nativeStatementTemp236,nativeStatementTemp237,nativeStatementTemp238);return nativeStatementTemp238;}());
([&]()->JSValue{auto nativeStatementTemp243=nativeStatementTemp234;const auto nativeStatementTemp244=JSValue("y");const auto nativeStatementTemp245=([&]()->JSValue{const auto nativeStatementTemp248=v;const auto nativeStatementTemp249=JSValue("y");return nativeStatementTemp248.get(nativeStatementTemp249);}());setReference(nativeStatementTemp243,nativeStatementTemp244,nativeStatementTemp245);return nativeStatementTemp245;}());
([&]()->JSValue{auto nativeStatementTemp250=nativeStatementTemp234;const auto nativeStatementTemp251=JSValue("z");const auto nativeStatementTemp252=([&]()->JSValue{const auto nativeStatementTemp255=v;const auto nativeStatementTemp256=JSValue("z");return nativeStatementTemp255.get(nativeStatementTemp256);}());setReference(nativeStatementTemp250,nativeStatementTemp251,nativeStatementTemp252);return nativeStatementTemp252;}());
([&]()->JSValue{auto nativeStatementTemp257=nativeStatementTemp234;const auto nativeStatementTemp258=JSValue("w");const auto nativeStatementTemp259=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp262=([&]()->JSValue{const auto nativeStatementTemp264=v;const auto nativeStatementTemp265=JSValue("w");return nativeStatementTemp264.get(nativeStatementTemp265);}());const auto nativeStatementTemp263=JSValue{};return binary("!==",nativeStatementTemp262,nativeStatementTemp263);}()).toBoolean())return ([&]()->JSValue{const auto nativeStatementTemp266=v;const auto nativeStatementTemp267=JSValue("w");return nativeStatementTemp266.get(nativeStatementTemp267);}());return JSValue(1);}());setReference(nativeStatementTemp257,nativeStatementTemp258,nativeStatementTemp259);return nativeStatementTemp259;}());
return nativeStatementTemp234;
return JSValue{};
}
JSValue method15(const JSValue& nativeStatementTemp268,const Args& nativeStatementTemp269,const Args& nativeStatementTemp270){
auto nativeStatementTemp271=nativeStatementTemp270[0];auto nativeStatementTemp272=nativeStatementTemp268;
JSValue v=arg(nativeStatementTemp269,0);
([&]()->JSValue{auto nativeStatementTemp274=nativeStatementTemp272;const auto nativeStatementTemp275=JSValue("x");const auto nativeStatementTemp277=nativeStatementTemp274.get(nativeStatementTemp275);const auto nativeStatementTemp278=([&]()->JSValue{const auto nativeStatementTemp279=v;const auto nativeStatementTemp280=JSValue("x");return nativeStatementTemp279.get(nativeStatementTemp280);}());const auto nativeStatementTemp276=binary("+",nativeStatementTemp277,nativeStatementTemp278);setReference(nativeStatementTemp274,nativeStatementTemp275,nativeStatementTemp276);return nativeStatementTemp276;}());
([&]()->JSValue{auto nativeStatementTemp281=nativeStatementTemp272;const auto nativeStatementTemp282=JSValue("y");const auto nativeStatementTemp284=nativeStatementTemp281.get(nativeStatementTemp282);const auto nativeStatementTemp285=([&]()->JSValue{const auto nativeStatementTemp286=v;const auto nativeStatementTemp287=JSValue("y");return nativeStatementTemp286.get(nativeStatementTemp287);}());const auto nativeStatementTemp283=binary("+",nativeStatementTemp284,nativeStatementTemp285);setReference(nativeStatementTemp281,nativeStatementTemp282,nativeStatementTemp283);return nativeStatementTemp283;}());
([&]()->JSValue{auto nativeStatementTemp288=nativeStatementTemp272;const auto nativeStatementTemp289=JSValue("z");const auto nativeStatementTemp291=nativeStatementTemp288.get(nativeStatementTemp289);const auto nativeStatementTemp292=([&]()->JSValue{const auto nativeStatementTemp293=v;const auto nativeStatementTemp294=JSValue("z");return nativeStatementTemp293.get(nativeStatementTemp294);}());const auto nativeStatementTemp290=binary("+",nativeStatementTemp291,nativeStatementTemp292);setReference(nativeStatementTemp288,nativeStatementTemp289,nativeStatementTemp290);return nativeStatementTemp290;}());
([&]()->JSValue{auto nativeStatementTemp295=nativeStatementTemp272;const auto nativeStatementTemp296=JSValue("w");const auto nativeStatementTemp298=nativeStatementTemp295.get(nativeStatementTemp296);const auto nativeStatementTemp299=([&]()->JSValue{const auto nativeStatementTemp300=v;const auto nativeStatementTemp301=JSValue("w");return nativeStatementTemp300.get(nativeStatementTemp301);}());const auto nativeStatementTemp297=binary("+",nativeStatementTemp298,nativeStatementTemp299);setReference(nativeStatementTemp295,nativeStatementTemp296,nativeStatementTemp297);return nativeStatementTemp297;}());
return nativeStatementTemp272;
return JSValue{};
}
JSValue method16(const JSValue& nativeStatementTemp302,const Args& nativeStatementTemp303,const Args& nativeStatementTemp304){
auto nativeStatementTemp305=nativeStatementTemp304[0];auto nativeStatementTemp306=nativeStatementTemp302;
JSValue s=arg(nativeStatementTemp303,0);
([&]()->JSValue{auto nativeStatementTemp308=nativeStatementTemp306;const auto nativeStatementTemp309=JSValue("x");const auto nativeStatementTemp311=nativeStatementTemp308.get(nativeStatementTemp309);const auto nativeStatementTemp312=s;const auto nativeStatementTemp310=binary("+",nativeStatementTemp311,nativeStatementTemp312);setReference(nativeStatementTemp308,nativeStatementTemp309,nativeStatementTemp310);return nativeStatementTemp310;}());
([&]()->JSValue{auto nativeStatementTemp313=nativeStatementTemp306;const auto nativeStatementTemp314=JSValue("y");const auto nativeStatementTemp316=nativeStatementTemp313.get(nativeStatementTemp314);const auto nativeStatementTemp317=s;const auto nativeStatementTemp315=binary("+",nativeStatementTemp316,nativeStatementTemp317);setReference(nativeStatementTemp313,nativeStatementTemp314,nativeStatementTemp315);return nativeStatementTemp315;}());
([&]()->JSValue{auto nativeStatementTemp318=nativeStatementTemp306;const auto nativeStatementTemp319=JSValue("z");const auto nativeStatementTemp321=nativeStatementTemp318.get(nativeStatementTemp319);const auto nativeStatementTemp322=s;const auto nativeStatementTemp320=binary("+",nativeStatementTemp321,nativeStatementTemp322);setReference(nativeStatementTemp318,nativeStatementTemp319,nativeStatementTemp320);return nativeStatementTemp320;}());
([&]()->JSValue{auto nativeStatementTemp323=nativeStatementTemp306;const auto nativeStatementTemp324=JSValue("w");const auto nativeStatementTemp326=nativeStatementTemp323.get(nativeStatementTemp324);const auto nativeStatementTemp327=s;const auto nativeStatementTemp325=binary("+",nativeStatementTemp326,nativeStatementTemp327);setReference(nativeStatementTemp323,nativeStatementTemp324,nativeStatementTemp325);return nativeStatementTemp325;}());
return nativeStatementTemp306;
return JSValue{};
}
JSValue method17(const JSValue& nativeStatementTemp328,const Args& nativeStatementTemp329,const Args& nativeStatementTemp330){
auto nativeStatementTemp331=nativeStatementTemp330[0];auto nativeStatementTemp332=nativeStatementTemp328;
JSValue a=arg(nativeStatementTemp329,0);
JSValue b=arg(nativeStatementTemp329,1);
([&]()->JSValue{auto nativeStatementTemp334=nativeStatementTemp332;const auto nativeStatementTemp335=JSValue("x");const auto nativeStatementTemp336=([&]()->JSValue{const auto nativeStatementTemp339=([&]()->JSValue{const auto nativeStatementTemp341=a;const auto nativeStatementTemp342=JSValue("x");return nativeStatementTemp341.get(nativeStatementTemp342);}());const auto nativeStatementTemp340=([&]()->JSValue{const auto nativeStatementTemp343=b;const auto nativeStatementTemp344=JSValue("x");return nativeStatementTemp343.get(nativeStatementTemp344);}());return binary("+",nativeStatementTemp339,nativeStatementTemp340);}());setReference(nativeStatementTemp334,nativeStatementTemp335,nativeStatementTemp336);return nativeStatementTemp336;}());
([&]()->JSValue{auto nativeStatementTemp345=nativeStatementTemp332;const auto nativeStatementTemp346=JSValue("y");const auto nativeStatementTemp347=([&]()->JSValue{const auto nativeStatementTemp350=([&]()->JSValue{const auto nativeStatementTemp352=a;const auto nativeStatementTemp353=JSValue("y");return nativeStatementTemp352.get(nativeStatementTemp353);}());const auto nativeStatementTemp351=([&]()->JSValue{const auto nativeStatementTemp354=b;const auto nativeStatementTemp355=JSValue("y");return nativeStatementTemp354.get(nativeStatementTemp355);}());return binary("+",nativeStatementTemp350,nativeStatementTemp351);}());setReference(nativeStatementTemp345,nativeStatementTemp346,nativeStatementTemp347);return nativeStatementTemp347;}());
([&]()->JSValue{auto nativeStatementTemp356=nativeStatementTemp332;const auto nativeStatementTemp357=JSValue("z");const auto nativeStatementTemp358=([&]()->JSValue{const auto nativeStatementTemp361=([&]()->JSValue{const auto nativeStatementTemp363=a;const auto nativeStatementTemp364=JSValue("z");return nativeStatementTemp363.get(nativeStatementTemp364);}());const auto nativeStatementTemp362=([&]()->JSValue{const auto nativeStatementTemp365=b;const auto nativeStatementTemp366=JSValue("z");return nativeStatementTemp365.get(nativeStatementTemp366);}());return binary("+",nativeStatementTemp361,nativeStatementTemp362);}());setReference(nativeStatementTemp356,nativeStatementTemp357,nativeStatementTemp358);return nativeStatementTemp358;}());
([&]()->JSValue{auto nativeStatementTemp367=nativeStatementTemp332;const auto nativeStatementTemp368=JSValue("w");const auto nativeStatementTemp369=([&]()->JSValue{const auto nativeStatementTemp372=([&]()->JSValue{const auto nativeStatementTemp374=a;const auto nativeStatementTemp375=JSValue("w");return nativeStatementTemp374.get(nativeStatementTemp375);}());const auto nativeStatementTemp373=([&]()->JSValue{const auto nativeStatementTemp376=b;const auto nativeStatementTemp377=JSValue("w");return nativeStatementTemp376.get(nativeStatementTemp377);}());return binary("+",nativeStatementTemp372,nativeStatementTemp373);}());setReference(nativeStatementTemp367,nativeStatementTemp368,nativeStatementTemp369);return nativeStatementTemp369;}());
return nativeStatementTemp332;
return JSValue{};
}
JSValue method18(const JSValue& nativeStatementTemp378,const Args& nativeStatementTemp379,const Args& nativeStatementTemp380){
auto nativeStatementTemp381=nativeStatementTemp380[0];auto nativeStatementTemp382=nativeStatementTemp378;
JSValue v=arg(nativeStatementTemp379,0);
JSValue s=arg(nativeStatementTemp379,1);
([&]()->JSValue{auto nativeStatementTemp384=nativeStatementTemp382;const auto nativeStatementTemp385=JSValue("x");const auto nativeStatementTemp387=nativeStatementTemp384.get(nativeStatementTemp385);const auto nativeStatementTemp388=([&]()->JSValue{const auto nativeStatementTemp389=([&]()->JSValue{const auto nativeStatementTemp391=v;const auto nativeStatementTemp392=JSValue("x");return nativeStatementTemp391.get(nativeStatementTemp392);}());const auto nativeStatementTemp390=s;return binary("*",nativeStatementTemp389,nativeStatementTemp390);}());const auto nativeStatementTemp386=binary("+",nativeStatementTemp387,nativeStatementTemp388);setReference(nativeStatementTemp384,nativeStatementTemp385,nativeStatementTemp386);return nativeStatementTemp386;}());
([&]()->JSValue{auto nativeStatementTemp393=nativeStatementTemp382;const auto nativeStatementTemp394=JSValue("y");const auto nativeStatementTemp396=nativeStatementTemp393.get(nativeStatementTemp394);const auto nativeStatementTemp397=([&]()->JSValue{const auto nativeStatementTemp398=([&]()->JSValue{const auto nativeStatementTemp400=v;const auto nativeStatementTemp401=JSValue("y");return nativeStatementTemp400.get(nativeStatementTemp401);}());const auto nativeStatementTemp399=s;return binary("*",nativeStatementTemp398,nativeStatementTemp399);}());const auto nativeStatementTemp395=binary("+",nativeStatementTemp396,nativeStatementTemp397);setReference(nativeStatementTemp393,nativeStatementTemp394,nativeStatementTemp395);return nativeStatementTemp395;}());
([&]()->JSValue{auto nativeStatementTemp402=nativeStatementTemp382;const auto nativeStatementTemp403=JSValue("z");const auto nativeStatementTemp405=nativeStatementTemp402.get(nativeStatementTemp403);const auto nativeStatementTemp406=([&]()->JSValue{const auto nativeStatementTemp407=([&]()->JSValue{const auto nativeStatementTemp409=v;const auto nativeStatementTemp410=JSValue("z");return nativeStatementTemp409.get(nativeStatementTemp410);}());const auto nativeStatementTemp408=s;return binary("*",nativeStatementTemp407,nativeStatementTemp408);}());const auto nativeStatementTemp404=binary("+",nativeStatementTemp405,nativeStatementTemp406);setReference(nativeStatementTemp402,nativeStatementTemp403,nativeStatementTemp404);return nativeStatementTemp404;}());
([&]()->JSValue{auto nativeStatementTemp411=nativeStatementTemp382;const auto nativeStatementTemp412=JSValue("w");const auto nativeStatementTemp414=nativeStatementTemp411.get(nativeStatementTemp412);const auto nativeStatementTemp415=([&]()->JSValue{const auto nativeStatementTemp416=([&]()->JSValue{const auto nativeStatementTemp418=v;const auto nativeStatementTemp419=JSValue("w");return nativeStatementTemp418.get(nativeStatementTemp419);}());const auto nativeStatementTemp417=s;return binary("*",nativeStatementTemp416,nativeStatementTemp417);}());const auto nativeStatementTemp413=binary("+",nativeStatementTemp414,nativeStatementTemp415);setReference(nativeStatementTemp411,nativeStatementTemp412,nativeStatementTemp413);return nativeStatementTemp413;}());
return nativeStatementTemp382;
return JSValue{};
}
JSValue method19(const JSValue& nativeStatementTemp420,const Args& nativeStatementTemp421,const Args& nativeStatementTemp422){
auto nativeStatementTemp423=nativeStatementTemp422[0];auto nativeStatementTemp424=nativeStatementTemp420;
JSValue v=arg(nativeStatementTemp421,0);
([&]()->JSValue{auto nativeStatementTemp426=nativeStatementTemp424;const auto nativeStatementTemp427=JSValue("x");const auto nativeStatementTemp429=nativeStatementTemp426.get(nativeStatementTemp427);const auto nativeStatementTemp430=([&]()->JSValue{const auto nativeStatementTemp431=v;const auto nativeStatementTemp432=JSValue("x");return nativeStatementTemp431.get(nativeStatementTemp432);}());const auto nativeStatementTemp428=binary("-",nativeStatementTemp429,nativeStatementTemp430);setReference(nativeStatementTemp426,nativeStatementTemp427,nativeStatementTemp428);return nativeStatementTemp428;}());
([&]()->JSValue{auto nativeStatementTemp433=nativeStatementTemp424;const auto nativeStatementTemp434=JSValue("y");const auto nativeStatementTemp436=nativeStatementTemp433.get(nativeStatementTemp434);const auto nativeStatementTemp437=([&]()->JSValue{const auto nativeStatementTemp438=v;const auto nativeStatementTemp439=JSValue("y");return nativeStatementTemp438.get(nativeStatementTemp439);}());const auto nativeStatementTemp435=binary("-",nativeStatementTemp436,nativeStatementTemp437);setReference(nativeStatementTemp433,nativeStatementTemp434,nativeStatementTemp435);return nativeStatementTemp435;}());
([&]()->JSValue{auto nativeStatementTemp440=nativeStatementTemp424;const auto nativeStatementTemp441=JSValue("z");const auto nativeStatementTemp443=nativeStatementTemp440.get(nativeStatementTemp441);const auto nativeStatementTemp444=([&]()->JSValue{const auto nativeStatementTemp445=v;const auto nativeStatementTemp446=JSValue("z");return nativeStatementTemp445.get(nativeStatementTemp446);}());const auto nativeStatementTemp442=binary("-",nativeStatementTemp443,nativeStatementTemp444);setReference(nativeStatementTemp440,nativeStatementTemp441,nativeStatementTemp442);return nativeStatementTemp442;}());
([&]()->JSValue{auto nativeStatementTemp447=nativeStatementTemp424;const auto nativeStatementTemp448=JSValue("w");const auto nativeStatementTemp450=nativeStatementTemp447.get(nativeStatementTemp448);const auto nativeStatementTemp451=([&]()->JSValue{const auto nativeStatementTemp452=v;const auto nativeStatementTemp453=JSValue("w");return nativeStatementTemp452.get(nativeStatementTemp453);}());const auto nativeStatementTemp449=binary("-",nativeStatementTemp450,nativeStatementTemp451);setReference(nativeStatementTemp447,nativeStatementTemp448,nativeStatementTemp449);return nativeStatementTemp449;}());
return nativeStatementTemp424;
return JSValue{};
}
JSValue method20(const JSValue& nativeStatementTemp454,const Args& nativeStatementTemp455,const Args& nativeStatementTemp456){
auto nativeStatementTemp457=nativeStatementTemp456[0];auto nativeStatementTemp458=nativeStatementTemp454;
JSValue s=arg(nativeStatementTemp455,0);
([&]()->JSValue{auto nativeStatementTemp460=nativeStatementTemp458;const auto nativeStatementTemp461=JSValue("x");const auto nativeStatementTemp463=nativeStatementTemp460.get(nativeStatementTemp461);const auto nativeStatementTemp464=s;const auto nativeStatementTemp462=binary("-",nativeStatementTemp463,nativeStatementTemp464);setReference(nativeStatementTemp460,nativeStatementTemp461,nativeStatementTemp462);return nativeStatementTemp462;}());
([&]()->JSValue{auto nativeStatementTemp465=nativeStatementTemp458;const auto nativeStatementTemp466=JSValue("y");const auto nativeStatementTemp468=nativeStatementTemp465.get(nativeStatementTemp466);const auto nativeStatementTemp469=s;const auto nativeStatementTemp467=binary("-",nativeStatementTemp468,nativeStatementTemp469);setReference(nativeStatementTemp465,nativeStatementTemp466,nativeStatementTemp467);return nativeStatementTemp467;}());
([&]()->JSValue{auto nativeStatementTemp470=nativeStatementTemp458;const auto nativeStatementTemp471=JSValue("z");const auto nativeStatementTemp473=nativeStatementTemp470.get(nativeStatementTemp471);const auto nativeStatementTemp474=s;const auto nativeStatementTemp472=binary("-",nativeStatementTemp473,nativeStatementTemp474);setReference(nativeStatementTemp470,nativeStatementTemp471,nativeStatementTemp472);return nativeStatementTemp472;}());
([&]()->JSValue{auto nativeStatementTemp475=nativeStatementTemp458;const auto nativeStatementTemp476=JSValue("w");const auto nativeStatementTemp478=nativeStatementTemp475.get(nativeStatementTemp476);const auto nativeStatementTemp479=s;const auto nativeStatementTemp477=binary("-",nativeStatementTemp478,nativeStatementTemp479);setReference(nativeStatementTemp475,nativeStatementTemp476,nativeStatementTemp477);return nativeStatementTemp477;}());
return nativeStatementTemp458;
return JSValue{};
}
JSValue method21(const JSValue& nativeStatementTemp480,const Args& nativeStatementTemp481,const Args& nativeStatementTemp482){
auto nativeStatementTemp483=nativeStatementTemp482[0];auto nativeStatementTemp484=nativeStatementTemp480;
JSValue a=arg(nativeStatementTemp481,0);
JSValue b=arg(nativeStatementTemp481,1);
([&]()->JSValue{auto nativeStatementTemp486=nativeStatementTemp484;const auto nativeStatementTemp487=JSValue("x");const auto nativeStatementTemp488=([&]()->JSValue{const auto nativeStatementTemp491=([&]()->JSValue{const auto nativeStatementTemp493=a;const auto nativeStatementTemp494=JSValue("x");return nativeStatementTemp493.get(nativeStatementTemp494);}());const auto nativeStatementTemp492=([&]()->JSValue{const auto nativeStatementTemp495=b;const auto nativeStatementTemp496=JSValue("x");return nativeStatementTemp495.get(nativeStatementTemp496);}());return binary("-",nativeStatementTemp491,nativeStatementTemp492);}());setReference(nativeStatementTemp486,nativeStatementTemp487,nativeStatementTemp488);return nativeStatementTemp488;}());
([&]()->JSValue{auto nativeStatementTemp497=nativeStatementTemp484;const auto nativeStatementTemp498=JSValue("y");const auto nativeStatementTemp499=([&]()->JSValue{const auto nativeStatementTemp502=([&]()->JSValue{const auto nativeStatementTemp504=a;const auto nativeStatementTemp505=JSValue("y");return nativeStatementTemp504.get(nativeStatementTemp505);}());const auto nativeStatementTemp503=([&]()->JSValue{const auto nativeStatementTemp506=b;const auto nativeStatementTemp507=JSValue("y");return nativeStatementTemp506.get(nativeStatementTemp507);}());return binary("-",nativeStatementTemp502,nativeStatementTemp503);}());setReference(nativeStatementTemp497,nativeStatementTemp498,nativeStatementTemp499);return nativeStatementTemp499;}());
([&]()->JSValue{auto nativeStatementTemp508=nativeStatementTemp484;const auto nativeStatementTemp509=JSValue("z");const auto nativeStatementTemp510=([&]()->JSValue{const auto nativeStatementTemp513=([&]()->JSValue{const auto nativeStatementTemp515=a;const auto nativeStatementTemp516=JSValue("z");return nativeStatementTemp515.get(nativeStatementTemp516);}());const auto nativeStatementTemp514=([&]()->JSValue{const auto nativeStatementTemp517=b;const auto nativeStatementTemp518=JSValue("z");return nativeStatementTemp517.get(nativeStatementTemp518);}());return binary("-",nativeStatementTemp513,nativeStatementTemp514);}());setReference(nativeStatementTemp508,nativeStatementTemp509,nativeStatementTemp510);return nativeStatementTemp510;}());
([&]()->JSValue{auto nativeStatementTemp519=nativeStatementTemp484;const auto nativeStatementTemp520=JSValue("w");const auto nativeStatementTemp521=([&]()->JSValue{const auto nativeStatementTemp524=([&]()->JSValue{const auto nativeStatementTemp526=a;const auto nativeStatementTemp527=JSValue("w");return nativeStatementTemp526.get(nativeStatementTemp527);}());const auto nativeStatementTemp525=([&]()->JSValue{const auto nativeStatementTemp528=b;const auto nativeStatementTemp529=JSValue("w");return nativeStatementTemp528.get(nativeStatementTemp529);}());return binary("-",nativeStatementTemp524,nativeStatementTemp525);}());setReference(nativeStatementTemp519,nativeStatementTemp520,nativeStatementTemp521);return nativeStatementTemp521;}());
return nativeStatementTemp484;
return JSValue{};
}
JSValue method22(const JSValue& nativeStatementTemp530,const Args& nativeStatementTemp531,const Args& nativeStatementTemp532){
auto nativeStatementTemp533=nativeStatementTemp532[0];auto nativeStatementTemp534=nativeStatementTemp530;
JSValue v=arg(nativeStatementTemp531,0);
([&]()->JSValue{auto nativeStatementTemp536=nativeStatementTemp534;const auto nativeStatementTemp537=JSValue("x");const auto nativeStatementTemp539=nativeStatementTemp536.get(nativeStatementTemp537);const auto nativeStatementTemp540=([&]()->JSValue{const auto nativeStatementTemp541=v;const auto nativeStatementTemp542=JSValue("x");return nativeStatementTemp541.get(nativeStatementTemp542);}());const auto nativeStatementTemp538=binary("*",nativeStatementTemp539,nativeStatementTemp540);setReference(nativeStatementTemp536,nativeStatementTemp537,nativeStatementTemp538);return nativeStatementTemp538;}());
([&]()->JSValue{auto nativeStatementTemp543=nativeStatementTemp534;const auto nativeStatementTemp544=JSValue("y");const auto nativeStatementTemp546=nativeStatementTemp543.get(nativeStatementTemp544);const auto nativeStatementTemp547=([&]()->JSValue{const auto nativeStatementTemp548=v;const auto nativeStatementTemp549=JSValue("y");return nativeStatementTemp548.get(nativeStatementTemp549);}());const auto nativeStatementTemp545=binary("*",nativeStatementTemp546,nativeStatementTemp547);setReference(nativeStatementTemp543,nativeStatementTemp544,nativeStatementTemp545);return nativeStatementTemp545;}());
([&]()->JSValue{auto nativeStatementTemp550=nativeStatementTemp534;const auto nativeStatementTemp551=JSValue("z");const auto nativeStatementTemp553=nativeStatementTemp550.get(nativeStatementTemp551);const auto nativeStatementTemp554=([&]()->JSValue{const auto nativeStatementTemp555=v;const auto nativeStatementTemp556=JSValue("z");return nativeStatementTemp555.get(nativeStatementTemp556);}());const auto nativeStatementTemp552=binary("*",nativeStatementTemp553,nativeStatementTemp554);setReference(nativeStatementTemp550,nativeStatementTemp551,nativeStatementTemp552);return nativeStatementTemp552;}());
([&]()->JSValue{auto nativeStatementTemp557=nativeStatementTemp534;const auto nativeStatementTemp558=JSValue("w");const auto nativeStatementTemp560=nativeStatementTemp557.get(nativeStatementTemp558);const auto nativeStatementTemp561=([&]()->JSValue{const auto nativeStatementTemp562=v;const auto nativeStatementTemp563=JSValue("w");return nativeStatementTemp562.get(nativeStatementTemp563);}());const auto nativeStatementTemp559=binary("*",nativeStatementTemp560,nativeStatementTemp561);setReference(nativeStatementTemp557,nativeStatementTemp558,nativeStatementTemp559);return nativeStatementTemp559;}());
return nativeStatementTemp534;
return JSValue{};
}
JSValue method23(const JSValue& nativeStatementTemp564,const Args& nativeStatementTemp565,const Args& nativeStatementTemp566){
auto nativeStatementTemp567=nativeStatementTemp566[0];auto nativeStatementTemp568=nativeStatementTemp564;
JSValue scalar=arg(nativeStatementTemp565,0);
([&]()->JSValue{auto nativeStatementTemp570=nativeStatementTemp568;const auto nativeStatementTemp571=JSValue("x");const auto nativeStatementTemp573=nativeStatementTemp570.get(nativeStatementTemp571);const auto nativeStatementTemp574=scalar;const auto nativeStatementTemp572=binary("*",nativeStatementTemp573,nativeStatementTemp574);setReference(nativeStatementTemp570,nativeStatementTemp571,nativeStatementTemp572);return nativeStatementTemp572;}());
([&]()->JSValue{auto nativeStatementTemp575=nativeStatementTemp568;const auto nativeStatementTemp576=JSValue("y");const auto nativeStatementTemp578=nativeStatementTemp575.get(nativeStatementTemp576);const auto nativeStatementTemp579=scalar;const auto nativeStatementTemp577=binary("*",nativeStatementTemp578,nativeStatementTemp579);setReference(nativeStatementTemp575,nativeStatementTemp576,nativeStatementTemp577);return nativeStatementTemp577;}());
([&]()->JSValue{auto nativeStatementTemp580=nativeStatementTemp568;const auto nativeStatementTemp581=JSValue("z");const auto nativeStatementTemp583=nativeStatementTemp580.get(nativeStatementTemp581);const auto nativeStatementTemp584=scalar;const auto nativeStatementTemp582=binary("*",nativeStatementTemp583,nativeStatementTemp584);setReference(nativeStatementTemp580,nativeStatementTemp581,nativeStatementTemp582);return nativeStatementTemp582;}());
([&]()->JSValue{auto nativeStatementTemp585=nativeStatementTemp568;const auto nativeStatementTemp586=JSValue("w");const auto nativeStatementTemp588=nativeStatementTemp585.get(nativeStatementTemp586);const auto nativeStatementTemp589=scalar;const auto nativeStatementTemp587=binary("*",nativeStatementTemp588,nativeStatementTemp589);setReference(nativeStatementTemp585,nativeStatementTemp586,nativeStatementTemp587);return nativeStatementTemp587;}());
return nativeStatementTemp568;
return JSValue{};
}
JSValue method24(const JSValue& nativeStatementTemp590,const Args& nativeStatementTemp591,const Args& nativeStatementTemp592){
auto nativeStatementTemp593=nativeStatementTemp592[0];auto nativeStatementTemp594=nativeStatementTemp590;
JSValue m=arg(nativeStatementTemp591,0);
JSValue x=([&]()->JSValue{const auto nativeStatementTemp596=nativeStatementTemp594;const auto nativeStatementTemp597=JSValue("x");return nativeStatementTemp596.get(nativeStatementTemp597);}());
JSValue y=([&]()->JSValue{const auto nativeStatementTemp598=nativeStatementTemp594;const auto nativeStatementTemp599=JSValue("y");return nativeStatementTemp598.get(nativeStatementTemp599);}());
JSValue z=([&]()->JSValue{const auto nativeStatementTemp600=nativeStatementTemp594;const auto nativeStatementTemp601=JSValue("z");return nativeStatementTemp600.get(nativeStatementTemp601);}());
JSValue w=([&]()->JSValue{const auto nativeStatementTemp602=nativeStatementTemp594;const auto nativeStatementTemp603=JSValue("w");return nativeStatementTemp602.get(nativeStatementTemp603);}());
JSValue e=([&]()->JSValue{const auto nativeStatementTemp604=m;const auto nativeStatementTemp605=JSValue("elements");return nativeStatementTemp604.get(nativeStatementTemp605);}());
([&]()->JSValue{auto nativeStatementTemp606=nativeStatementTemp594;const auto nativeStatementTemp607=JSValue("x");const auto nativeStatementTemp608=([&]()->JSValue{const auto nativeStatementTemp611=([&]()->JSValue{const auto nativeStatementTemp613=([&]()->JSValue{const auto nativeStatementTemp615=([&]()->JSValue{const auto nativeStatementTemp617=([&]()->JSValue{const auto nativeStatementTemp619=e;const auto nativeStatementTemp620=JSValue(0);return nativeStatementTemp619.get(nativeStatementTemp620);}());const auto nativeStatementTemp618=x;return binary("*",nativeStatementTemp617,nativeStatementTemp618);}());const auto nativeStatementTemp616=([&]()->JSValue{const auto nativeStatementTemp621=([&]()->JSValue{const auto nativeStatementTemp623=e;const auto nativeStatementTemp624=JSValue(4);return nativeStatementTemp623.get(nativeStatementTemp624);}());const auto nativeStatementTemp622=y;return binary("*",nativeStatementTemp621,nativeStatementTemp622);}());return binary("+",nativeStatementTemp615,nativeStatementTemp616);}());const auto nativeStatementTemp614=([&]()->JSValue{const auto nativeStatementTemp625=([&]()->JSValue{const auto nativeStatementTemp627=e;const auto nativeStatementTemp628=JSValue(8);return nativeStatementTemp627.get(nativeStatementTemp628);}());const auto nativeStatementTemp626=z;return binary("*",nativeStatementTemp625,nativeStatementTemp626);}());return binary("+",nativeStatementTemp613,nativeStatementTemp614);}());const auto nativeStatementTemp612=([&]()->JSValue{const auto nativeStatementTemp629=([&]()->JSValue{const auto nativeStatementTemp631=e;const auto nativeStatementTemp632=JSValue(12);return nativeStatementTemp631.get(nativeStatementTemp632);}());const auto nativeStatementTemp630=w;return binary("*",nativeStatementTemp629,nativeStatementTemp630);}());return binary("+",nativeStatementTemp611,nativeStatementTemp612);}());setReference(nativeStatementTemp606,nativeStatementTemp607,nativeStatementTemp608);return nativeStatementTemp608;}());
([&]()->JSValue{auto nativeStatementTemp633=nativeStatementTemp594;const auto nativeStatementTemp634=JSValue("y");const auto nativeStatementTemp635=([&]()->JSValue{const auto nativeStatementTemp638=([&]()->JSValue{const auto nativeStatementTemp640=([&]()->JSValue{const auto nativeStatementTemp642=([&]()->JSValue{const auto nativeStatementTemp644=([&]()->JSValue{const auto nativeStatementTemp646=e;const auto nativeStatementTemp647=JSValue(1);return nativeStatementTemp646.get(nativeStatementTemp647);}());const auto nativeStatementTemp645=x;return binary("*",nativeStatementTemp644,nativeStatementTemp645);}());const auto nativeStatementTemp643=([&]()->JSValue{const auto nativeStatementTemp648=([&]()->JSValue{const auto nativeStatementTemp650=e;const auto nativeStatementTemp651=JSValue(5);return nativeStatementTemp650.get(nativeStatementTemp651);}());const auto nativeStatementTemp649=y;return binary("*",nativeStatementTemp648,nativeStatementTemp649);}());return binary("+",nativeStatementTemp642,nativeStatementTemp643);}());const auto nativeStatementTemp641=([&]()->JSValue{const auto nativeStatementTemp652=([&]()->JSValue{const auto nativeStatementTemp654=e;const auto nativeStatementTemp655=JSValue(9);return nativeStatementTemp654.get(nativeStatementTemp655);}());const auto nativeStatementTemp653=z;return binary("*",nativeStatementTemp652,nativeStatementTemp653);}());return binary("+",nativeStatementTemp640,nativeStatementTemp641);}());const auto nativeStatementTemp639=([&]()->JSValue{const auto nativeStatementTemp656=([&]()->JSValue{const auto nativeStatementTemp658=e;const auto nativeStatementTemp659=JSValue(13);return nativeStatementTemp658.get(nativeStatementTemp659);}());const auto nativeStatementTemp657=w;return binary("*",nativeStatementTemp656,nativeStatementTemp657);}());return binary("+",nativeStatementTemp638,nativeStatementTemp639);}());setReference(nativeStatementTemp633,nativeStatementTemp634,nativeStatementTemp635);return nativeStatementTemp635;}());
([&]()->JSValue{auto nativeStatementTemp660=nativeStatementTemp594;const auto nativeStatementTemp661=JSValue("z");const auto nativeStatementTemp662=([&]()->JSValue{const auto nativeStatementTemp665=([&]()->JSValue{const auto nativeStatementTemp667=([&]()->JSValue{const auto nativeStatementTemp669=([&]()->JSValue{const auto nativeStatementTemp671=([&]()->JSValue{const auto nativeStatementTemp673=e;const auto nativeStatementTemp674=JSValue(2);return nativeStatementTemp673.get(nativeStatementTemp674);}());const auto nativeStatementTemp672=x;return binary("*",nativeStatementTemp671,nativeStatementTemp672);}());const auto nativeStatementTemp670=([&]()->JSValue{const auto nativeStatementTemp675=([&]()->JSValue{const auto nativeStatementTemp677=e;const auto nativeStatementTemp678=JSValue(6);return nativeStatementTemp677.get(nativeStatementTemp678);}());const auto nativeStatementTemp676=y;return binary("*",nativeStatementTemp675,nativeStatementTemp676);}());return binary("+",nativeStatementTemp669,nativeStatementTemp670);}());const auto nativeStatementTemp668=([&]()->JSValue{const auto nativeStatementTemp679=([&]()->JSValue{const auto nativeStatementTemp681=e;const auto nativeStatementTemp682=JSValue(10);return nativeStatementTemp681.get(nativeStatementTemp682);}());const auto nativeStatementTemp680=z;return binary("*",nativeStatementTemp679,nativeStatementTemp680);}());return binary("+",nativeStatementTemp667,nativeStatementTemp668);}());const auto nativeStatementTemp666=([&]()->JSValue{const auto nativeStatementTemp683=([&]()->JSValue{const auto nativeStatementTemp685=e;const auto nativeStatementTemp686=JSValue(14);return nativeStatementTemp685.get(nativeStatementTemp686);}());const auto nativeStatementTemp684=w;return binary("*",nativeStatementTemp683,nativeStatementTemp684);}());return binary("+",nativeStatementTemp665,nativeStatementTemp666);}());setReference(nativeStatementTemp660,nativeStatementTemp661,nativeStatementTemp662);return nativeStatementTemp662;}());
([&]()->JSValue{auto nativeStatementTemp687=nativeStatementTemp594;const auto nativeStatementTemp688=JSValue("w");const auto nativeStatementTemp689=([&]()->JSValue{const auto nativeStatementTemp692=([&]()->JSValue{const auto nativeStatementTemp694=([&]()->JSValue{const auto nativeStatementTemp696=([&]()->JSValue{const auto nativeStatementTemp698=([&]()->JSValue{const auto nativeStatementTemp700=e;const auto nativeStatementTemp701=JSValue(3);return nativeStatementTemp700.get(nativeStatementTemp701);}());const auto nativeStatementTemp699=x;return binary("*",nativeStatementTemp698,nativeStatementTemp699);}());const auto nativeStatementTemp697=([&]()->JSValue{const auto nativeStatementTemp702=([&]()->JSValue{const auto nativeStatementTemp704=e;const auto nativeStatementTemp705=JSValue(7);return nativeStatementTemp704.get(nativeStatementTemp705);}());const auto nativeStatementTemp703=y;return binary("*",nativeStatementTemp702,nativeStatementTemp703);}());return binary("+",nativeStatementTemp696,nativeStatementTemp697);}());const auto nativeStatementTemp695=([&]()->JSValue{const auto nativeStatementTemp706=([&]()->JSValue{const auto nativeStatementTemp708=e;const auto nativeStatementTemp709=JSValue(11);return nativeStatementTemp708.get(nativeStatementTemp709);}());const auto nativeStatementTemp707=z;return binary("*",nativeStatementTemp706,nativeStatementTemp707);}());return binary("+",nativeStatementTemp694,nativeStatementTemp695);}());const auto nativeStatementTemp693=([&]()->JSValue{const auto nativeStatementTemp710=([&]()->JSValue{const auto nativeStatementTemp712=e;const auto nativeStatementTemp713=JSValue(15);return nativeStatementTemp712.get(nativeStatementTemp713);}());const auto nativeStatementTemp711=w;return binary("*",nativeStatementTemp710,nativeStatementTemp711);}());return binary("+",nativeStatementTemp692,nativeStatementTemp693);}());setReference(nativeStatementTemp687,nativeStatementTemp688,nativeStatementTemp689);return nativeStatementTemp689;}());
return nativeStatementTemp594;
return JSValue{};
}
JSValue method25(const JSValue& nativeStatementTemp714,const Args& nativeStatementTemp715,const Args& nativeStatementTemp716){
auto nativeStatementTemp717=nativeStatementTemp716[0];auto nativeStatementTemp718=nativeStatementTemp714;
JSValue v=arg(nativeStatementTemp715,0);
([&]()->JSValue{auto nativeStatementTemp720=nativeStatementTemp718;const auto nativeStatementTemp721=JSValue("x");const auto nativeStatementTemp723=nativeStatementTemp720.get(nativeStatementTemp721);const auto nativeStatementTemp724=([&]()->JSValue{const auto nativeStatementTemp725=v;const auto nativeStatementTemp726=JSValue("x");return nativeStatementTemp725.get(nativeStatementTemp726);}());const auto nativeStatementTemp722=binary("/",nativeStatementTemp723,nativeStatementTemp724);setReference(nativeStatementTemp720,nativeStatementTemp721,nativeStatementTemp722);return nativeStatementTemp722;}());
([&]()->JSValue{auto nativeStatementTemp727=nativeStatementTemp718;const auto nativeStatementTemp728=JSValue("y");const auto nativeStatementTemp730=nativeStatementTemp727.get(nativeStatementTemp728);const auto nativeStatementTemp731=([&]()->JSValue{const auto nativeStatementTemp732=v;const auto nativeStatementTemp733=JSValue("y");return nativeStatementTemp732.get(nativeStatementTemp733);}());const auto nativeStatementTemp729=binary("/",nativeStatementTemp730,nativeStatementTemp731);setReference(nativeStatementTemp727,nativeStatementTemp728,nativeStatementTemp729);return nativeStatementTemp729;}());
([&]()->JSValue{auto nativeStatementTemp734=nativeStatementTemp718;const auto nativeStatementTemp735=JSValue("z");const auto nativeStatementTemp737=nativeStatementTemp734.get(nativeStatementTemp735);const auto nativeStatementTemp738=([&]()->JSValue{const auto nativeStatementTemp739=v;const auto nativeStatementTemp740=JSValue("z");return nativeStatementTemp739.get(nativeStatementTemp740);}());const auto nativeStatementTemp736=binary("/",nativeStatementTemp737,nativeStatementTemp738);setReference(nativeStatementTemp734,nativeStatementTemp735,nativeStatementTemp736);return nativeStatementTemp736;}());
([&]()->JSValue{auto nativeStatementTemp741=nativeStatementTemp718;const auto nativeStatementTemp742=JSValue("w");const auto nativeStatementTemp744=nativeStatementTemp741.get(nativeStatementTemp742);const auto nativeStatementTemp745=([&]()->JSValue{const auto nativeStatementTemp746=v;const auto nativeStatementTemp747=JSValue("w");return nativeStatementTemp746.get(nativeStatementTemp747);}());const auto nativeStatementTemp743=binary("/",nativeStatementTemp744,nativeStatementTemp745);setReference(nativeStatementTemp741,nativeStatementTemp742,nativeStatementTemp743);return nativeStatementTemp743;}());
return nativeStatementTemp718;
return JSValue{};
}
JSValue method26(const JSValue& nativeStatementTemp748,const Args& nativeStatementTemp749,const Args& nativeStatementTemp750){
auto nativeStatementTemp751=nativeStatementTemp750[0];auto nativeStatementTemp752=nativeStatementTemp748;
JSValue scalar=arg(nativeStatementTemp749,0);
return ([&]()->JSValue{auto nativeStatementTemp757=nativeStatementTemp752;const auto nativeStatementTemp758=JSValue("multiplyScalar");const auto nativeStatementTemp754=nativeStatementTemp757.get(nativeStatementTemp758);const auto nativeStatementTemp755=nativeStatementTemp757;Args nativeStatementTemp756;nativeStatementTemp756.push_back(([&]()->JSValue{const auto nativeStatementTemp759=JSValue(1);const auto nativeStatementTemp760=scalar;return binary("/",nativeStatementTemp759,nativeStatementTemp760);}()));return nativeStatementTemp754.call(nativeStatementTemp755,nativeStatementTemp756);}());
return JSValue{};
}
JSValue method27(const JSValue& nativeStatementTemp761,const Args& nativeStatementTemp762,const Args& nativeStatementTemp763){
auto nativeStatementTemp764=nativeStatementTemp763[0];auto nativeStatementTemp765=nativeStatementTemp761;
JSValue q=arg(nativeStatementTemp762,0);
([&]()->JSValue{auto nativeStatementTemp767=nativeStatementTemp765;const auto nativeStatementTemp768=JSValue("w");const auto nativeStatementTemp769=([&]()->JSValue{const auto nativeStatementTemp772=JSValue(2);const auto nativeStatementTemp773=([&]()->JSValue{auto nativeStatementTemp777=nativeGlobalBinding("Math");const auto nativeStatementTemp778=JSValue("acos");const auto nativeStatementTemp774=nativeStatementTemp777.get(nativeStatementTemp778);const auto nativeStatementTemp775=nativeStatementTemp777;Args nativeStatementTemp776;nativeStatementTemp776.push_back(([&]()->JSValue{const auto nativeStatementTemp779=q;const auto nativeStatementTemp780=JSValue("w");return nativeStatementTemp779.get(nativeStatementTemp780);}()));return nativeStatementTemp774.call(nativeStatementTemp775,nativeStatementTemp776);}());return binary("*",nativeStatementTemp772,nativeStatementTemp773);}());setReference(nativeStatementTemp767,nativeStatementTemp768,nativeStatementTemp769);return nativeStatementTemp769;}());
JSValue s=([&]()->JSValue{auto nativeStatementTemp784=nativeGlobalBinding("Math");const auto nativeStatementTemp785=JSValue("sqrt");const auto nativeStatementTemp781=nativeStatementTemp784.get(nativeStatementTemp785);const auto nativeStatementTemp782=nativeStatementTemp784;Args nativeStatementTemp783;nativeStatementTemp783.push_back(([&]()->JSValue{const auto nativeStatementTemp786=JSValue(1);const auto nativeStatementTemp787=([&]()->JSValue{const auto nativeStatementTemp788=([&]()->JSValue{const auto nativeStatementTemp790=q;const auto nativeStatementTemp791=JSValue("w");return nativeStatementTemp790.get(nativeStatementTemp791);}());const auto nativeStatementTemp789=([&]()->JSValue{const auto nativeStatementTemp792=q;const auto nativeStatementTemp793=JSValue("w");return nativeStatementTemp792.get(nativeStatementTemp793);}());return binary("*",nativeStatementTemp788,nativeStatementTemp789);}());return binary("-",nativeStatementTemp786,nativeStatementTemp787);}()));return nativeStatementTemp781.call(nativeStatementTemp782,nativeStatementTemp783);}());
if(([&]()->JSValue{const auto nativeStatementTemp794=s;const auto nativeStatementTemp795=JSValue(0.0001);return binary("<",nativeStatementTemp794,nativeStatementTemp795);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp796=nativeStatementTemp765;const auto nativeStatementTemp797=JSValue("x");const auto nativeStatementTemp798=JSValue(1);setReference(nativeStatementTemp796,nativeStatementTemp797,nativeStatementTemp798);return nativeStatementTemp798;}());
([&]()->JSValue{auto nativeStatementTemp801=nativeStatementTemp765;const auto nativeStatementTemp802=JSValue("y");const auto nativeStatementTemp803=JSValue(0);setReference(nativeStatementTemp801,nativeStatementTemp802,nativeStatementTemp803);return nativeStatementTemp803;}());
([&]()->JSValue{auto nativeStatementTemp806=nativeStatementTemp765;const auto nativeStatementTemp807=JSValue("z");const auto nativeStatementTemp808=JSValue(0);setReference(nativeStatementTemp806,nativeStatementTemp807,nativeStatementTemp808);return nativeStatementTemp808;}());

}
else {
([&]()->JSValue{auto nativeStatementTemp811=nativeStatementTemp765;const auto nativeStatementTemp812=JSValue("x");const auto nativeStatementTemp813=([&]()->JSValue{const auto nativeStatementTemp816=([&]()->JSValue{const auto nativeStatementTemp818=q;const auto nativeStatementTemp819=JSValue("x");return nativeStatementTemp818.get(nativeStatementTemp819);}());const auto nativeStatementTemp817=s;return binary("/",nativeStatementTemp816,nativeStatementTemp817);}());setReference(nativeStatementTemp811,nativeStatementTemp812,nativeStatementTemp813);return nativeStatementTemp813;}());
([&]()->JSValue{auto nativeStatementTemp820=nativeStatementTemp765;const auto nativeStatementTemp821=JSValue("y");const auto nativeStatementTemp822=([&]()->JSValue{const auto nativeStatementTemp825=([&]()->JSValue{const auto nativeStatementTemp827=q;const auto nativeStatementTemp828=JSValue("y");return nativeStatementTemp827.get(nativeStatementTemp828);}());const auto nativeStatementTemp826=s;return binary("/",nativeStatementTemp825,nativeStatementTemp826);}());setReference(nativeStatementTemp820,nativeStatementTemp821,nativeStatementTemp822);return nativeStatementTemp822;}());
([&]()->JSValue{auto nativeStatementTemp829=nativeStatementTemp765;const auto nativeStatementTemp830=JSValue("z");const auto nativeStatementTemp831=([&]()->JSValue{const auto nativeStatementTemp834=([&]()->JSValue{const auto nativeStatementTemp836=q;const auto nativeStatementTemp837=JSValue("z");return nativeStatementTemp836.get(nativeStatementTemp837);}());const auto nativeStatementTemp835=s;return binary("/",nativeStatementTemp834,nativeStatementTemp835);}());setReference(nativeStatementTemp829,nativeStatementTemp830,nativeStatementTemp831);return nativeStatementTemp831;}());

}
return nativeStatementTemp765;
return JSValue{};
}
JSValue method28(const JSValue& nativeStatementTemp838,const Args& nativeStatementTemp839,const Args& nativeStatementTemp840){
auto nativeStatementTemp841=nativeStatementTemp840[0];auto nativeStatementTemp842=nativeStatementTemp838;
JSValue m=arg(nativeStatementTemp839,0);
JSValue angle=JSValue{};
JSValue x=JSValue{};
JSValue y=JSValue{};
JSValue z=JSValue{};
JSValue epsilon=JSValue(0.01);
JSValue epsilon2=JSValue(0.1);
JSValue te=([&]()->JSValue{const auto nativeStatementTemp844=m;const auto nativeStatementTemp845=JSValue("elements");return nativeStatementTemp844.get(nativeStatementTemp845);}());
JSValue m11=([&]()->JSValue{const auto nativeStatementTemp846=te;const auto nativeStatementTemp847=JSValue(0);return nativeStatementTemp846.get(nativeStatementTemp847);}());
JSValue m12=([&]()->JSValue{const auto nativeStatementTemp848=te;const auto nativeStatementTemp849=JSValue(4);return nativeStatementTemp848.get(nativeStatementTemp849);}());
JSValue m13=([&]()->JSValue{const auto nativeStatementTemp850=te;const auto nativeStatementTemp851=JSValue(8);return nativeStatementTemp850.get(nativeStatementTemp851);}());
JSValue m21=([&]()->JSValue{const auto nativeStatementTemp852=te;const auto nativeStatementTemp853=JSValue(1);return nativeStatementTemp852.get(nativeStatementTemp853);}());
JSValue m22=([&]()->JSValue{const auto nativeStatementTemp854=te;const auto nativeStatementTemp855=JSValue(5);return nativeStatementTemp854.get(nativeStatementTemp855);}());
JSValue m23=([&]()->JSValue{const auto nativeStatementTemp856=te;const auto nativeStatementTemp857=JSValue(9);return nativeStatementTemp856.get(nativeStatementTemp857);}());
JSValue m31=([&]()->JSValue{const auto nativeStatementTemp858=te;const auto nativeStatementTemp859=JSValue(2);return nativeStatementTemp858.get(nativeStatementTemp859);}());
JSValue m32=([&]()->JSValue{const auto nativeStatementTemp860=te;const auto nativeStatementTemp861=JSValue(6);return nativeStatementTemp860.get(nativeStatementTemp861);}());
JSValue m33=([&]()->JSValue{const auto nativeStatementTemp862=te;const auto nativeStatementTemp863=JSValue(10);return nativeStatementTemp862.get(nativeStatementTemp863);}());
if(([&]()->JSValue{const auto nativeStatementTemp864=([&]()->JSValue{const auto nativeStatementTemp865=([&]()->JSValue{const auto nativeStatementTemp866=([&]()->JSValue{auto nativeStatementTemp871=nativeGlobalBinding("Math");const auto nativeStatementTemp872=JSValue("abs");const auto nativeStatementTemp868=nativeStatementTemp871.get(nativeStatementTemp872);const auto nativeStatementTemp869=nativeStatementTemp871;Args nativeStatementTemp870;nativeStatementTemp870.push_back(([&]()->JSValue{const auto nativeStatementTemp873=m12;const auto nativeStatementTemp874=m21;return binary("-",nativeStatementTemp873,nativeStatementTemp874);}()));return nativeStatementTemp868.call(nativeStatementTemp869,nativeStatementTemp870);}());const auto nativeStatementTemp867=epsilon;return binary("<",nativeStatementTemp866,nativeStatementTemp867);}());if(!nativeStatementTemp865.toBoolean())return nativeStatementTemp865;return ([&]()->JSValue{const auto nativeStatementTemp875=([&]()->JSValue{auto nativeStatementTemp880=nativeGlobalBinding("Math");const auto nativeStatementTemp881=JSValue("abs");const auto nativeStatementTemp877=nativeStatementTemp880.get(nativeStatementTemp881);const auto nativeStatementTemp878=nativeStatementTemp880;Args nativeStatementTemp879;nativeStatementTemp879.push_back(([&]()->JSValue{const auto nativeStatementTemp882=m13;const auto nativeStatementTemp883=m31;return binary("-",nativeStatementTemp882,nativeStatementTemp883);}()));return nativeStatementTemp877.call(nativeStatementTemp878,nativeStatementTemp879);}());const auto nativeStatementTemp876=epsilon;return binary("<",nativeStatementTemp875,nativeStatementTemp876);}());}());if(!nativeStatementTemp864.toBoolean())return nativeStatementTemp864;return ([&]()->JSValue{const auto nativeStatementTemp884=([&]()->JSValue{auto nativeStatementTemp889=nativeGlobalBinding("Math");const auto nativeStatementTemp890=JSValue("abs");const auto nativeStatementTemp886=nativeStatementTemp889.get(nativeStatementTemp890);const auto nativeStatementTemp887=nativeStatementTemp889;Args nativeStatementTemp888;nativeStatementTemp888.push_back(([&]()->JSValue{const auto nativeStatementTemp891=m23;const auto nativeStatementTemp892=m32;return binary("-",nativeStatementTemp891,nativeStatementTemp892);}()));return nativeStatementTemp886.call(nativeStatementTemp887,nativeStatementTemp888);}());const auto nativeStatementTemp885=epsilon;return binary("<",nativeStatementTemp884,nativeStatementTemp885);}());}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp893=([&]()->JSValue{const auto nativeStatementTemp894=([&]()->JSValue{const auto nativeStatementTemp895=([&]()->JSValue{const auto nativeStatementTemp896=([&]()->JSValue{auto nativeStatementTemp901=nativeGlobalBinding("Math");const auto nativeStatementTemp902=JSValue("abs");const auto nativeStatementTemp898=nativeStatementTemp901.get(nativeStatementTemp902);const auto nativeStatementTemp899=nativeStatementTemp901;Args nativeStatementTemp900;nativeStatementTemp900.push_back(([&]()->JSValue{const auto nativeStatementTemp903=m12;const auto nativeStatementTemp904=m21;return binary("+",nativeStatementTemp903,nativeStatementTemp904);}()));return nativeStatementTemp898.call(nativeStatementTemp899,nativeStatementTemp900);}());const auto nativeStatementTemp897=epsilon2;return binary("<",nativeStatementTemp896,nativeStatementTemp897);}());if(!nativeStatementTemp895.toBoolean())return nativeStatementTemp895;return ([&]()->JSValue{const auto nativeStatementTemp905=([&]()->JSValue{auto nativeStatementTemp910=nativeGlobalBinding("Math");const auto nativeStatementTemp911=JSValue("abs");const auto nativeStatementTemp907=nativeStatementTemp910.get(nativeStatementTemp911);const auto nativeStatementTemp908=nativeStatementTemp910;Args nativeStatementTemp909;nativeStatementTemp909.push_back(([&]()->JSValue{const auto nativeStatementTemp912=m13;const auto nativeStatementTemp913=m31;return binary("+",nativeStatementTemp912,nativeStatementTemp913);}()));return nativeStatementTemp907.call(nativeStatementTemp908,nativeStatementTemp909);}());const auto nativeStatementTemp906=epsilon2;return binary("<",nativeStatementTemp905,nativeStatementTemp906);}());}());if(!nativeStatementTemp894.toBoolean())return nativeStatementTemp894;return ([&]()->JSValue{const auto nativeStatementTemp914=([&]()->JSValue{auto nativeStatementTemp919=nativeGlobalBinding("Math");const auto nativeStatementTemp920=JSValue("abs");const auto nativeStatementTemp916=nativeStatementTemp919.get(nativeStatementTemp920);const auto nativeStatementTemp917=nativeStatementTemp919;Args nativeStatementTemp918;nativeStatementTemp918.push_back(([&]()->JSValue{const auto nativeStatementTemp921=m23;const auto nativeStatementTemp922=m32;return binary("+",nativeStatementTemp921,nativeStatementTemp922);}()));return nativeStatementTemp916.call(nativeStatementTemp917,nativeStatementTemp918);}());const auto nativeStatementTemp915=epsilon2;return binary("<",nativeStatementTemp914,nativeStatementTemp915);}());}());if(!nativeStatementTemp893.toBoolean())return nativeStatementTemp893;return ([&]()->JSValue{const auto nativeStatementTemp923=([&]()->JSValue{auto nativeStatementTemp928=nativeGlobalBinding("Math");const auto nativeStatementTemp929=JSValue("abs");const auto nativeStatementTemp925=nativeStatementTemp928.get(nativeStatementTemp929);const auto nativeStatementTemp926=nativeStatementTemp928;Args nativeStatementTemp927;nativeStatementTemp927.push_back(([&]()->JSValue{const auto nativeStatementTemp930=([&]()->JSValue{const auto nativeStatementTemp932=([&]()->JSValue{const auto nativeStatementTemp934=m11;const auto nativeStatementTemp935=m22;return binary("+",nativeStatementTemp934,nativeStatementTemp935);}());const auto nativeStatementTemp933=m33;return binary("+",nativeStatementTemp932,nativeStatementTemp933);}());const auto nativeStatementTemp931=JSValue(3);return binary("-",nativeStatementTemp930,nativeStatementTemp931);}()));return nativeStatementTemp925.call(nativeStatementTemp926,nativeStatementTemp927);}());const auto nativeStatementTemp924=epsilon2;return binary("<",nativeStatementTemp923,nativeStatementTemp924);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp939=nativeStatementTemp842;const auto nativeStatementTemp940=JSValue("set");const auto nativeStatementTemp936=nativeStatementTemp939.get(nativeStatementTemp940);const auto nativeStatementTemp937=nativeStatementTemp939;Args nativeStatementTemp938;nativeStatementTemp938.push_back(JSValue(1));nativeStatementTemp938.push_back(JSValue(0));nativeStatementTemp938.push_back(JSValue(0));nativeStatementTemp938.push_back(JSValue(0));return nativeStatementTemp936.call(nativeStatementTemp937,nativeStatementTemp938);}());
return nativeStatementTemp842;

}
([&]()->JSValue{const auto nativeStatementTemp941=([&]()->JSValue{const auto nativeStatementTemp944=nativeGlobalBinding("Math");const auto nativeStatementTemp945=JSValue("PI");return nativeStatementTemp944.get(nativeStatementTemp945);}());angle=nativeStatementTemp941;return nativeStatementTemp941;}());
JSValue xx=([&]()->JSValue{const auto nativeStatementTemp946=([&]()->JSValue{const auto nativeStatementTemp948=m11;const auto nativeStatementTemp949=JSValue(1);return binary("+",nativeStatementTemp948,nativeStatementTemp949);}());const auto nativeStatementTemp947=JSValue(2);return binary("/",nativeStatementTemp946,nativeStatementTemp947);}());
JSValue yy=([&]()->JSValue{const auto nativeStatementTemp950=([&]()->JSValue{const auto nativeStatementTemp952=m22;const auto nativeStatementTemp953=JSValue(1);return binary("+",nativeStatementTemp952,nativeStatementTemp953);}());const auto nativeStatementTemp951=JSValue(2);return binary("/",nativeStatementTemp950,nativeStatementTemp951);}());
JSValue zz=([&]()->JSValue{const auto nativeStatementTemp954=([&]()->JSValue{const auto nativeStatementTemp956=m33;const auto nativeStatementTemp957=JSValue(1);return binary("+",nativeStatementTemp956,nativeStatementTemp957);}());const auto nativeStatementTemp955=JSValue(2);return binary("/",nativeStatementTemp954,nativeStatementTemp955);}());
JSValue xy=([&]()->JSValue{const auto nativeStatementTemp958=([&]()->JSValue{const auto nativeStatementTemp960=m12;const auto nativeStatementTemp961=m21;return binary("+",nativeStatementTemp960,nativeStatementTemp961);}());const auto nativeStatementTemp959=JSValue(4);return binary("/",nativeStatementTemp958,nativeStatementTemp959);}());
JSValue xz=([&]()->JSValue{const auto nativeStatementTemp962=([&]()->JSValue{const auto nativeStatementTemp964=m13;const auto nativeStatementTemp965=m31;return binary("+",nativeStatementTemp964,nativeStatementTemp965);}());const auto nativeStatementTemp963=JSValue(4);return binary("/",nativeStatementTemp962,nativeStatementTemp963);}());
JSValue yz=([&]()->JSValue{const auto nativeStatementTemp966=([&]()->JSValue{const auto nativeStatementTemp968=m23;const auto nativeStatementTemp969=m32;return binary("+",nativeStatementTemp968,nativeStatementTemp969);}());const auto nativeStatementTemp967=JSValue(4);return binary("/",nativeStatementTemp966,nativeStatementTemp967);}());
if(([&]()->JSValue{const auto nativeStatementTemp970=([&]()->JSValue{const auto nativeStatementTemp971=xx;const auto nativeStatementTemp972=yy;return binary(">",nativeStatementTemp971,nativeStatementTemp972);}());if(!nativeStatementTemp970.toBoolean())return nativeStatementTemp970;return ([&]()->JSValue{const auto nativeStatementTemp973=xx;const auto nativeStatementTemp974=zz;return binary(">",nativeStatementTemp973,nativeStatementTemp974);}());}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp975=xx;const auto nativeStatementTemp976=epsilon;return binary("<",nativeStatementTemp975,nativeStatementTemp976);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp977=JSValue(0);x=nativeStatementTemp977;return nativeStatementTemp977;}());
([&]()->JSValue{const auto nativeStatementTemp980=JSValue(0.707106781);y=nativeStatementTemp980;return nativeStatementTemp980;}());
([&]()->JSValue{const auto nativeStatementTemp983=JSValue(0.707106781);z=nativeStatementTemp983;return nativeStatementTemp983;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp986=([&]()->JSValue{auto nativeStatementTemp992=nativeGlobalBinding("Math");const auto nativeStatementTemp993=JSValue("sqrt");const auto nativeStatementTemp989=nativeStatementTemp992.get(nativeStatementTemp993);const auto nativeStatementTemp990=nativeStatementTemp992;Args nativeStatementTemp991;nativeStatementTemp991.push_back(xx);return nativeStatementTemp989.call(nativeStatementTemp990,nativeStatementTemp991);}());x=nativeStatementTemp986;return nativeStatementTemp986;}());
([&]()->JSValue{const auto nativeStatementTemp994=([&]()->JSValue{const auto nativeStatementTemp997=xy;const auto nativeStatementTemp998=x;return binary("/",nativeStatementTemp997,nativeStatementTemp998);}());y=nativeStatementTemp994;return nativeStatementTemp994;}());
([&]()->JSValue{const auto nativeStatementTemp999=([&]()->JSValue{const auto nativeStatementTemp1002=xz;const auto nativeStatementTemp1003=x;return binary("/",nativeStatementTemp1002,nativeStatementTemp1003);}());z=nativeStatementTemp999;return nativeStatementTemp999;}());

}

}
else if(([&]()->JSValue{const auto nativeStatementTemp1004=yy;const auto nativeStatementTemp1005=zz;return binary(">",nativeStatementTemp1004,nativeStatementTemp1005);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp1006=yy;const auto nativeStatementTemp1007=epsilon;return binary("<",nativeStatementTemp1006,nativeStatementTemp1007);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1008=JSValue(0.707106781);x=nativeStatementTemp1008;return nativeStatementTemp1008;}());
([&]()->JSValue{const auto nativeStatementTemp1011=JSValue(0);y=nativeStatementTemp1011;return nativeStatementTemp1011;}());
([&]()->JSValue{const auto nativeStatementTemp1014=JSValue(0.707106781);z=nativeStatementTemp1014;return nativeStatementTemp1014;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp1017=([&]()->JSValue{auto nativeStatementTemp1023=nativeGlobalBinding("Math");const auto nativeStatementTemp1024=JSValue("sqrt");const auto nativeStatementTemp1020=nativeStatementTemp1023.get(nativeStatementTemp1024);const auto nativeStatementTemp1021=nativeStatementTemp1023;Args nativeStatementTemp1022;nativeStatementTemp1022.push_back(yy);return nativeStatementTemp1020.call(nativeStatementTemp1021,nativeStatementTemp1022);}());y=nativeStatementTemp1017;return nativeStatementTemp1017;}());
([&]()->JSValue{const auto nativeStatementTemp1025=([&]()->JSValue{const auto nativeStatementTemp1028=xy;const auto nativeStatementTemp1029=y;return binary("/",nativeStatementTemp1028,nativeStatementTemp1029);}());x=nativeStatementTemp1025;return nativeStatementTemp1025;}());
([&]()->JSValue{const auto nativeStatementTemp1030=([&]()->JSValue{const auto nativeStatementTemp1033=yz;const auto nativeStatementTemp1034=y;return binary("/",nativeStatementTemp1033,nativeStatementTemp1034);}());z=nativeStatementTemp1030;return nativeStatementTemp1030;}());

}

}
else {
if(([&]()->JSValue{const auto nativeStatementTemp1035=zz;const auto nativeStatementTemp1036=epsilon;return binary("<",nativeStatementTemp1035,nativeStatementTemp1036);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1037=JSValue(0.707106781);x=nativeStatementTemp1037;return nativeStatementTemp1037;}());
([&]()->JSValue{const auto nativeStatementTemp1040=JSValue(0.707106781);y=nativeStatementTemp1040;return nativeStatementTemp1040;}());
([&]()->JSValue{const auto nativeStatementTemp1043=JSValue(0);z=nativeStatementTemp1043;return nativeStatementTemp1043;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp1046=([&]()->JSValue{auto nativeStatementTemp1052=nativeGlobalBinding("Math");const auto nativeStatementTemp1053=JSValue("sqrt");const auto nativeStatementTemp1049=nativeStatementTemp1052.get(nativeStatementTemp1053);const auto nativeStatementTemp1050=nativeStatementTemp1052;Args nativeStatementTemp1051;nativeStatementTemp1051.push_back(zz);return nativeStatementTemp1049.call(nativeStatementTemp1050,nativeStatementTemp1051);}());z=nativeStatementTemp1046;return nativeStatementTemp1046;}());
([&]()->JSValue{const auto nativeStatementTemp1054=([&]()->JSValue{const auto nativeStatementTemp1057=xz;const auto nativeStatementTemp1058=z;return binary("/",nativeStatementTemp1057,nativeStatementTemp1058);}());x=nativeStatementTemp1054;return nativeStatementTemp1054;}());
([&]()->JSValue{const auto nativeStatementTemp1059=([&]()->JSValue{const auto nativeStatementTemp1062=yz;const auto nativeStatementTemp1063=z;return binary("/",nativeStatementTemp1062,nativeStatementTemp1063);}());y=nativeStatementTemp1059;return nativeStatementTemp1059;}());

}

}
([&]()->JSValue{auto nativeStatementTemp1067=nativeStatementTemp842;const auto nativeStatementTemp1068=JSValue("set");const auto nativeStatementTemp1064=nativeStatementTemp1067.get(nativeStatementTemp1068);const auto nativeStatementTemp1065=nativeStatementTemp1067;Args nativeStatementTemp1066;nativeStatementTemp1066.push_back(x);nativeStatementTemp1066.push_back(y);nativeStatementTemp1066.push_back(z);nativeStatementTemp1066.push_back(angle);return nativeStatementTemp1064.call(nativeStatementTemp1065,nativeStatementTemp1066);}());
return nativeStatementTemp842;

}
JSValue s=([&]()->JSValue{auto nativeStatementTemp1072=nativeGlobalBinding("Math");const auto nativeStatementTemp1073=JSValue("sqrt");const auto nativeStatementTemp1069=nativeStatementTemp1072.get(nativeStatementTemp1073);const auto nativeStatementTemp1070=nativeStatementTemp1072;Args nativeStatementTemp1071;nativeStatementTemp1071.push_back(([&]()->JSValue{const auto nativeStatementTemp1074=([&]()->JSValue{const auto nativeStatementTemp1076=([&]()->JSValue{const auto nativeStatementTemp1078=([&]()->JSValue{const auto nativeStatementTemp1080=m32;const auto nativeStatementTemp1081=m23;return binary("-",nativeStatementTemp1080,nativeStatementTemp1081);}());const auto nativeStatementTemp1079=([&]()->JSValue{const auto nativeStatementTemp1082=m32;const auto nativeStatementTemp1083=m23;return binary("-",nativeStatementTemp1082,nativeStatementTemp1083);}());return binary("*",nativeStatementTemp1078,nativeStatementTemp1079);}());const auto nativeStatementTemp1077=([&]()->JSValue{const auto nativeStatementTemp1084=([&]()->JSValue{const auto nativeStatementTemp1086=m13;const auto nativeStatementTemp1087=m31;return binary("-",nativeStatementTemp1086,nativeStatementTemp1087);}());const auto nativeStatementTemp1085=([&]()->JSValue{const auto nativeStatementTemp1088=m13;const auto nativeStatementTemp1089=m31;return binary("-",nativeStatementTemp1088,nativeStatementTemp1089);}());return binary("*",nativeStatementTemp1084,nativeStatementTemp1085);}());return binary("+",nativeStatementTemp1076,nativeStatementTemp1077);}());const auto nativeStatementTemp1075=([&]()->JSValue{const auto nativeStatementTemp1090=([&]()->JSValue{const auto nativeStatementTemp1092=m21;const auto nativeStatementTemp1093=m12;return binary("-",nativeStatementTemp1092,nativeStatementTemp1093);}());const auto nativeStatementTemp1091=([&]()->JSValue{const auto nativeStatementTemp1094=m21;const auto nativeStatementTemp1095=m12;return binary("-",nativeStatementTemp1094,nativeStatementTemp1095);}());return binary("*",nativeStatementTemp1090,nativeStatementTemp1091);}());return binary("+",nativeStatementTemp1074,nativeStatementTemp1075);}()));return nativeStatementTemp1069.call(nativeStatementTemp1070,nativeStatementTemp1071);}());
if(([&]()->JSValue{const auto nativeStatementTemp1096=([&]()->JSValue{auto nativeStatementTemp1101=nativeGlobalBinding("Math");const auto nativeStatementTemp1102=JSValue("abs");const auto nativeStatementTemp1098=nativeStatementTemp1101.get(nativeStatementTemp1102);const auto nativeStatementTemp1099=nativeStatementTemp1101;Args nativeStatementTemp1100;nativeStatementTemp1100.push_back(s);return nativeStatementTemp1098.call(nativeStatementTemp1099,nativeStatementTemp1100);}());const auto nativeStatementTemp1097=JSValue(0.001);return binary("<",nativeStatementTemp1096,nativeStatementTemp1097);}()).toBoolean())([&]()->JSValue{const auto nativeStatementTemp1103=JSValue(1);s=nativeStatementTemp1103;return nativeStatementTemp1103;}());
([&]()->JSValue{auto nativeStatementTemp1106=nativeStatementTemp842;const auto nativeStatementTemp1107=JSValue("x");const auto nativeStatementTemp1108=([&]()->JSValue{const auto nativeStatementTemp1111=([&]()->JSValue{const auto nativeStatementTemp1113=m32;const auto nativeStatementTemp1114=m23;return binary("-",nativeStatementTemp1113,nativeStatementTemp1114);}());const auto nativeStatementTemp1112=s;return binary("/",nativeStatementTemp1111,nativeStatementTemp1112);}());setReference(nativeStatementTemp1106,nativeStatementTemp1107,nativeStatementTemp1108);return nativeStatementTemp1108;}());
([&]()->JSValue{auto nativeStatementTemp1115=nativeStatementTemp842;const auto nativeStatementTemp1116=JSValue("y");const auto nativeStatementTemp1117=([&]()->JSValue{const auto nativeStatementTemp1120=([&]()->JSValue{const auto nativeStatementTemp1122=m13;const auto nativeStatementTemp1123=m31;return binary("-",nativeStatementTemp1122,nativeStatementTemp1123);}());const auto nativeStatementTemp1121=s;return binary("/",nativeStatementTemp1120,nativeStatementTemp1121);}());setReference(nativeStatementTemp1115,nativeStatementTemp1116,nativeStatementTemp1117);return nativeStatementTemp1117;}());
([&]()->JSValue{auto nativeStatementTemp1124=nativeStatementTemp842;const auto nativeStatementTemp1125=JSValue("z");const auto nativeStatementTemp1126=([&]()->JSValue{const auto nativeStatementTemp1129=([&]()->JSValue{const auto nativeStatementTemp1131=m21;const auto nativeStatementTemp1132=m12;return binary("-",nativeStatementTemp1131,nativeStatementTemp1132);}());const auto nativeStatementTemp1130=s;return binary("/",nativeStatementTemp1129,nativeStatementTemp1130);}());setReference(nativeStatementTemp1124,nativeStatementTemp1125,nativeStatementTemp1126);return nativeStatementTemp1126;}());
([&]()->JSValue{auto nativeStatementTemp1133=nativeStatementTemp842;const auto nativeStatementTemp1134=JSValue("w");const auto nativeStatementTemp1135=([&]()->JSValue{auto nativeStatementTemp1141=nativeGlobalBinding("Math");const auto nativeStatementTemp1142=JSValue("acos");const auto nativeStatementTemp1138=nativeStatementTemp1141.get(nativeStatementTemp1142);const auto nativeStatementTemp1139=nativeStatementTemp1141;Args nativeStatementTemp1140;nativeStatementTemp1140.push_back(([&]()->JSValue{const auto nativeStatementTemp1143=([&]()->JSValue{const auto nativeStatementTemp1145=([&]()->JSValue{const auto nativeStatementTemp1147=([&]()->JSValue{const auto nativeStatementTemp1149=m11;const auto nativeStatementTemp1150=m22;return binary("+",nativeStatementTemp1149,nativeStatementTemp1150);}());const auto nativeStatementTemp1148=m33;return binary("+",nativeStatementTemp1147,nativeStatementTemp1148);}());const auto nativeStatementTemp1146=JSValue(1);return binary("-",nativeStatementTemp1145,nativeStatementTemp1146);}());const auto nativeStatementTemp1144=JSValue(2);return binary("/",nativeStatementTemp1143,nativeStatementTemp1144);}()));return nativeStatementTemp1138.call(nativeStatementTemp1139,nativeStatementTemp1140);}());setReference(nativeStatementTemp1133,nativeStatementTemp1134,nativeStatementTemp1135);return nativeStatementTemp1135;}());
return nativeStatementTemp842;
return JSValue{};
}
JSValue method29(const JSValue& nativeStatementTemp1151,const Args& nativeStatementTemp1152,const Args& nativeStatementTemp1153){
auto nativeStatementTemp1154=nativeStatementTemp1153[0];auto nativeStatementTemp1155=nativeStatementTemp1151;
JSValue m=arg(nativeStatementTemp1152,0);
JSValue e=([&]()->JSValue{const auto nativeStatementTemp1157=m;const auto nativeStatementTemp1158=JSValue("elements");return nativeStatementTemp1157.get(nativeStatementTemp1158);}());
([&]()->JSValue{auto nativeStatementTemp1159=nativeStatementTemp1155;const auto nativeStatementTemp1160=JSValue("x");const auto nativeStatementTemp1161=([&]()->JSValue{const auto nativeStatementTemp1164=e;const auto nativeStatementTemp1165=JSValue(12);return nativeStatementTemp1164.get(nativeStatementTemp1165);}());setReference(nativeStatementTemp1159,nativeStatementTemp1160,nativeStatementTemp1161);return nativeStatementTemp1161;}());
([&]()->JSValue{auto nativeStatementTemp1166=nativeStatementTemp1155;const auto nativeStatementTemp1167=JSValue("y");const auto nativeStatementTemp1168=([&]()->JSValue{const auto nativeStatementTemp1171=e;const auto nativeStatementTemp1172=JSValue(13);return nativeStatementTemp1171.get(nativeStatementTemp1172);}());setReference(nativeStatementTemp1166,nativeStatementTemp1167,nativeStatementTemp1168);return nativeStatementTemp1168;}());
([&]()->JSValue{auto nativeStatementTemp1173=nativeStatementTemp1155;const auto nativeStatementTemp1174=JSValue("z");const auto nativeStatementTemp1175=([&]()->JSValue{const auto nativeStatementTemp1178=e;const auto nativeStatementTemp1179=JSValue(14);return nativeStatementTemp1178.get(nativeStatementTemp1179);}());setReference(nativeStatementTemp1173,nativeStatementTemp1174,nativeStatementTemp1175);return nativeStatementTemp1175;}());
([&]()->JSValue{auto nativeStatementTemp1180=nativeStatementTemp1155;const auto nativeStatementTemp1181=JSValue("w");const auto nativeStatementTemp1182=([&]()->JSValue{const auto nativeStatementTemp1185=e;const auto nativeStatementTemp1186=JSValue(15);return nativeStatementTemp1185.get(nativeStatementTemp1186);}());setReference(nativeStatementTemp1180,nativeStatementTemp1181,nativeStatementTemp1182);return nativeStatementTemp1182;}());
return nativeStatementTemp1155;
return JSValue{};
}
JSValue method30(const JSValue& nativeStatementTemp1187,const Args& nativeStatementTemp1188,const Args& nativeStatementTemp1189){
auto nativeStatementTemp1190=nativeStatementTemp1189[0];auto nativeStatementTemp1191=nativeStatementTemp1187;
JSValue v=arg(nativeStatementTemp1188,0);
([&]()->JSValue{auto nativeStatementTemp1193=nativeStatementTemp1191;const auto nativeStatementTemp1194=JSValue("x");const auto nativeStatementTemp1195=([&]()->JSValue{auto nativeStatementTemp1201=nativeGlobalBinding("Math");const auto nativeStatementTemp1202=JSValue("min");const auto nativeStatementTemp1198=nativeStatementTemp1201.get(nativeStatementTemp1202);const auto nativeStatementTemp1199=nativeStatementTemp1201;Args nativeStatementTemp1200;nativeStatementTemp1200.push_back(([&]()->JSValue{const auto nativeStatementTemp1203=nativeStatementTemp1191;const auto nativeStatementTemp1204=JSValue("x");return nativeStatementTemp1203.get(nativeStatementTemp1204);}()));nativeStatementTemp1200.push_back(([&]()->JSValue{const auto nativeStatementTemp1205=v;const auto nativeStatementTemp1206=JSValue("x");return nativeStatementTemp1205.get(nativeStatementTemp1206);}()));return nativeStatementTemp1198.call(nativeStatementTemp1199,nativeStatementTemp1200);}());setReference(nativeStatementTemp1193,nativeStatementTemp1194,nativeStatementTemp1195);return nativeStatementTemp1195;}());
([&]()->JSValue{auto nativeStatementTemp1207=nativeStatementTemp1191;const auto nativeStatementTemp1208=JSValue("y");const auto nativeStatementTemp1209=([&]()->JSValue{auto nativeStatementTemp1215=nativeGlobalBinding("Math");const auto nativeStatementTemp1216=JSValue("min");const auto nativeStatementTemp1212=nativeStatementTemp1215.get(nativeStatementTemp1216);const auto nativeStatementTemp1213=nativeStatementTemp1215;Args nativeStatementTemp1214;nativeStatementTemp1214.push_back(([&]()->JSValue{const auto nativeStatementTemp1217=nativeStatementTemp1191;const auto nativeStatementTemp1218=JSValue("y");return nativeStatementTemp1217.get(nativeStatementTemp1218);}()));nativeStatementTemp1214.push_back(([&]()->JSValue{const auto nativeStatementTemp1219=v;const auto nativeStatementTemp1220=JSValue("y");return nativeStatementTemp1219.get(nativeStatementTemp1220);}()));return nativeStatementTemp1212.call(nativeStatementTemp1213,nativeStatementTemp1214);}());setReference(nativeStatementTemp1207,nativeStatementTemp1208,nativeStatementTemp1209);return nativeStatementTemp1209;}());
([&]()->JSValue{auto nativeStatementTemp1221=nativeStatementTemp1191;const auto nativeStatementTemp1222=JSValue("z");const auto nativeStatementTemp1223=([&]()->JSValue{auto nativeStatementTemp1229=nativeGlobalBinding("Math");const auto nativeStatementTemp1230=JSValue("min");const auto nativeStatementTemp1226=nativeStatementTemp1229.get(nativeStatementTemp1230);const auto nativeStatementTemp1227=nativeStatementTemp1229;Args nativeStatementTemp1228;nativeStatementTemp1228.push_back(([&]()->JSValue{const auto nativeStatementTemp1231=nativeStatementTemp1191;const auto nativeStatementTemp1232=JSValue("z");return nativeStatementTemp1231.get(nativeStatementTemp1232);}()));nativeStatementTemp1228.push_back(([&]()->JSValue{const auto nativeStatementTemp1233=v;const auto nativeStatementTemp1234=JSValue("z");return nativeStatementTemp1233.get(nativeStatementTemp1234);}()));return nativeStatementTemp1226.call(nativeStatementTemp1227,nativeStatementTemp1228);}());setReference(nativeStatementTemp1221,nativeStatementTemp1222,nativeStatementTemp1223);return nativeStatementTemp1223;}());
([&]()->JSValue{auto nativeStatementTemp1235=nativeStatementTemp1191;const auto nativeStatementTemp1236=JSValue("w");const auto nativeStatementTemp1237=([&]()->JSValue{auto nativeStatementTemp1243=nativeGlobalBinding("Math");const auto nativeStatementTemp1244=JSValue("min");const auto nativeStatementTemp1240=nativeStatementTemp1243.get(nativeStatementTemp1244);const auto nativeStatementTemp1241=nativeStatementTemp1243;Args nativeStatementTemp1242;nativeStatementTemp1242.push_back(([&]()->JSValue{const auto nativeStatementTemp1245=nativeStatementTemp1191;const auto nativeStatementTemp1246=JSValue("w");return nativeStatementTemp1245.get(nativeStatementTemp1246);}()));nativeStatementTemp1242.push_back(([&]()->JSValue{const auto nativeStatementTemp1247=v;const auto nativeStatementTemp1248=JSValue("w");return nativeStatementTemp1247.get(nativeStatementTemp1248);}()));return nativeStatementTemp1240.call(nativeStatementTemp1241,nativeStatementTemp1242);}());setReference(nativeStatementTemp1235,nativeStatementTemp1236,nativeStatementTemp1237);return nativeStatementTemp1237;}());
return nativeStatementTemp1191;
return JSValue{};
}
JSValue method31(const JSValue& nativeStatementTemp1249,const Args& nativeStatementTemp1250,const Args& nativeStatementTemp1251){
auto nativeStatementTemp1252=nativeStatementTemp1251[0];auto nativeStatementTemp1253=nativeStatementTemp1249;
JSValue v=arg(nativeStatementTemp1250,0);
([&]()->JSValue{auto nativeStatementTemp1255=nativeStatementTemp1253;const auto nativeStatementTemp1256=JSValue("x");const auto nativeStatementTemp1257=([&]()->JSValue{auto nativeStatementTemp1263=nativeGlobalBinding("Math");const auto nativeStatementTemp1264=JSValue("max");const auto nativeStatementTemp1260=nativeStatementTemp1263.get(nativeStatementTemp1264);const auto nativeStatementTemp1261=nativeStatementTemp1263;Args nativeStatementTemp1262;nativeStatementTemp1262.push_back(([&]()->JSValue{const auto nativeStatementTemp1265=nativeStatementTemp1253;const auto nativeStatementTemp1266=JSValue("x");return nativeStatementTemp1265.get(nativeStatementTemp1266);}()));nativeStatementTemp1262.push_back(([&]()->JSValue{const auto nativeStatementTemp1267=v;const auto nativeStatementTemp1268=JSValue("x");return nativeStatementTemp1267.get(nativeStatementTemp1268);}()));return nativeStatementTemp1260.call(nativeStatementTemp1261,nativeStatementTemp1262);}());setReference(nativeStatementTemp1255,nativeStatementTemp1256,nativeStatementTemp1257);return nativeStatementTemp1257;}());
([&]()->JSValue{auto nativeStatementTemp1269=nativeStatementTemp1253;const auto nativeStatementTemp1270=JSValue("y");const auto nativeStatementTemp1271=([&]()->JSValue{auto nativeStatementTemp1277=nativeGlobalBinding("Math");const auto nativeStatementTemp1278=JSValue("max");const auto nativeStatementTemp1274=nativeStatementTemp1277.get(nativeStatementTemp1278);const auto nativeStatementTemp1275=nativeStatementTemp1277;Args nativeStatementTemp1276;nativeStatementTemp1276.push_back(([&]()->JSValue{const auto nativeStatementTemp1279=nativeStatementTemp1253;const auto nativeStatementTemp1280=JSValue("y");return nativeStatementTemp1279.get(nativeStatementTemp1280);}()));nativeStatementTemp1276.push_back(([&]()->JSValue{const auto nativeStatementTemp1281=v;const auto nativeStatementTemp1282=JSValue("y");return nativeStatementTemp1281.get(nativeStatementTemp1282);}()));return nativeStatementTemp1274.call(nativeStatementTemp1275,nativeStatementTemp1276);}());setReference(nativeStatementTemp1269,nativeStatementTemp1270,nativeStatementTemp1271);return nativeStatementTemp1271;}());
([&]()->JSValue{auto nativeStatementTemp1283=nativeStatementTemp1253;const auto nativeStatementTemp1284=JSValue("z");const auto nativeStatementTemp1285=([&]()->JSValue{auto nativeStatementTemp1291=nativeGlobalBinding("Math");const auto nativeStatementTemp1292=JSValue("max");const auto nativeStatementTemp1288=nativeStatementTemp1291.get(nativeStatementTemp1292);const auto nativeStatementTemp1289=nativeStatementTemp1291;Args nativeStatementTemp1290;nativeStatementTemp1290.push_back(([&]()->JSValue{const auto nativeStatementTemp1293=nativeStatementTemp1253;const auto nativeStatementTemp1294=JSValue("z");return nativeStatementTemp1293.get(nativeStatementTemp1294);}()));nativeStatementTemp1290.push_back(([&]()->JSValue{const auto nativeStatementTemp1295=v;const auto nativeStatementTemp1296=JSValue("z");return nativeStatementTemp1295.get(nativeStatementTemp1296);}()));return nativeStatementTemp1288.call(nativeStatementTemp1289,nativeStatementTemp1290);}());setReference(nativeStatementTemp1283,nativeStatementTemp1284,nativeStatementTemp1285);return nativeStatementTemp1285;}());
([&]()->JSValue{auto nativeStatementTemp1297=nativeStatementTemp1253;const auto nativeStatementTemp1298=JSValue("w");const auto nativeStatementTemp1299=([&]()->JSValue{auto nativeStatementTemp1305=nativeGlobalBinding("Math");const auto nativeStatementTemp1306=JSValue("max");const auto nativeStatementTemp1302=nativeStatementTemp1305.get(nativeStatementTemp1306);const auto nativeStatementTemp1303=nativeStatementTemp1305;Args nativeStatementTemp1304;nativeStatementTemp1304.push_back(([&]()->JSValue{const auto nativeStatementTemp1307=nativeStatementTemp1253;const auto nativeStatementTemp1308=JSValue("w");return nativeStatementTemp1307.get(nativeStatementTemp1308);}()));nativeStatementTemp1304.push_back(([&]()->JSValue{const auto nativeStatementTemp1309=v;const auto nativeStatementTemp1310=JSValue("w");return nativeStatementTemp1309.get(nativeStatementTemp1310);}()));return nativeStatementTemp1302.call(nativeStatementTemp1303,nativeStatementTemp1304);}());setReference(nativeStatementTemp1297,nativeStatementTemp1298,nativeStatementTemp1299);return nativeStatementTemp1299;}());
return nativeStatementTemp1253;
return JSValue{};
}
JSValue method32(const JSValue& nativeStatementTemp1311,const Args& nativeStatementTemp1312,const Args& nativeStatementTemp1313){
auto nativeStatementTemp1314=nativeStatementTemp1313[0];auto nativeStatementTemp1315=nativeStatementTemp1311;
JSValue min=arg(nativeStatementTemp1312,0);
JSValue max=arg(nativeStatementTemp1312,1);
([&]()->JSValue{auto nativeStatementTemp1317=nativeStatementTemp1315;const auto nativeStatementTemp1318=JSValue("x");const auto nativeStatementTemp1319=([&]()->JSValue{const auto nativeStatementTemp1322=nativeStatementTemp1314.get("clamp");const JSValue nativeStatementTemp1323;Args nativeStatementTemp1324;nativeStatementTemp1324.push_back(([&]()->JSValue{const auto nativeStatementTemp1325=nativeStatementTemp1315;const auto nativeStatementTemp1326=JSValue("x");return nativeStatementTemp1325.get(nativeStatementTemp1326);}()));nativeStatementTemp1324.push_back(([&]()->JSValue{const auto nativeStatementTemp1327=min;const auto nativeStatementTemp1328=JSValue("x");return nativeStatementTemp1327.get(nativeStatementTemp1328);}()));nativeStatementTemp1324.push_back(([&]()->JSValue{const auto nativeStatementTemp1329=max;const auto nativeStatementTemp1330=JSValue("x");return nativeStatementTemp1329.get(nativeStatementTemp1330);}()));return nativeStatementTemp1322.call(nativeStatementTemp1323,nativeStatementTemp1324);}());setReference(nativeStatementTemp1317,nativeStatementTemp1318,nativeStatementTemp1319);return nativeStatementTemp1319;}());
([&]()->JSValue{auto nativeStatementTemp1331=nativeStatementTemp1315;const auto nativeStatementTemp1332=JSValue("y");const auto nativeStatementTemp1333=([&]()->JSValue{const auto nativeStatementTemp1336=nativeStatementTemp1314.get("clamp");const JSValue nativeStatementTemp1337;Args nativeStatementTemp1338;nativeStatementTemp1338.push_back(([&]()->JSValue{const auto nativeStatementTemp1339=nativeStatementTemp1315;const auto nativeStatementTemp1340=JSValue("y");return nativeStatementTemp1339.get(nativeStatementTemp1340);}()));nativeStatementTemp1338.push_back(([&]()->JSValue{const auto nativeStatementTemp1341=min;const auto nativeStatementTemp1342=JSValue("y");return nativeStatementTemp1341.get(nativeStatementTemp1342);}()));nativeStatementTemp1338.push_back(([&]()->JSValue{const auto nativeStatementTemp1343=max;const auto nativeStatementTemp1344=JSValue("y");return nativeStatementTemp1343.get(nativeStatementTemp1344);}()));return nativeStatementTemp1336.call(nativeStatementTemp1337,nativeStatementTemp1338);}());setReference(nativeStatementTemp1331,nativeStatementTemp1332,nativeStatementTemp1333);return nativeStatementTemp1333;}());
([&]()->JSValue{auto nativeStatementTemp1345=nativeStatementTemp1315;const auto nativeStatementTemp1346=JSValue("z");const auto nativeStatementTemp1347=([&]()->JSValue{const auto nativeStatementTemp1350=nativeStatementTemp1314.get("clamp");const JSValue nativeStatementTemp1351;Args nativeStatementTemp1352;nativeStatementTemp1352.push_back(([&]()->JSValue{const auto nativeStatementTemp1353=nativeStatementTemp1315;const auto nativeStatementTemp1354=JSValue("z");return nativeStatementTemp1353.get(nativeStatementTemp1354);}()));nativeStatementTemp1352.push_back(([&]()->JSValue{const auto nativeStatementTemp1355=min;const auto nativeStatementTemp1356=JSValue("z");return nativeStatementTemp1355.get(nativeStatementTemp1356);}()));nativeStatementTemp1352.push_back(([&]()->JSValue{const auto nativeStatementTemp1357=max;const auto nativeStatementTemp1358=JSValue("z");return nativeStatementTemp1357.get(nativeStatementTemp1358);}()));return nativeStatementTemp1350.call(nativeStatementTemp1351,nativeStatementTemp1352);}());setReference(nativeStatementTemp1345,nativeStatementTemp1346,nativeStatementTemp1347);return nativeStatementTemp1347;}());
([&]()->JSValue{auto nativeStatementTemp1359=nativeStatementTemp1315;const auto nativeStatementTemp1360=JSValue("w");const auto nativeStatementTemp1361=([&]()->JSValue{const auto nativeStatementTemp1364=nativeStatementTemp1314.get("clamp");const JSValue nativeStatementTemp1365;Args nativeStatementTemp1366;nativeStatementTemp1366.push_back(([&]()->JSValue{const auto nativeStatementTemp1367=nativeStatementTemp1315;const auto nativeStatementTemp1368=JSValue("w");return nativeStatementTemp1367.get(nativeStatementTemp1368);}()));nativeStatementTemp1366.push_back(([&]()->JSValue{const auto nativeStatementTemp1369=min;const auto nativeStatementTemp1370=JSValue("w");return nativeStatementTemp1369.get(nativeStatementTemp1370);}()));nativeStatementTemp1366.push_back(([&]()->JSValue{const auto nativeStatementTemp1371=max;const auto nativeStatementTemp1372=JSValue("w");return nativeStatementTemp1371.get(nativeStatementTemp1372);}()));return nativeStatementTemp1364.call(nativeStatementTemp1365,nativeStatementTemp1366);}());setReference(nativeStatementTemp1359,nativeStatementTemp1360,nativeStatementTemp1361);return nativeStatementTemp1361;}());
return nativeStatementTemp1315;
return JSValue{};
}
JSValue method33(const JSValue& nativeStatementTemp1373,const Args& nativeStatementTemp1374,const Args& nativeStatementTemp1375){
auto nativeStatementTemp1376=nativeStatementTemp1375[0];auto nativeStatementTemp1377=nativeStatementTemp1373;
JSValue minVal=arg(nativeStatementTemp1374,0);
JSValue maxVal=arg(nativeStatementTemp1374,1);
([&]()->JSValue{auto nativeStatementTemp1379=nativeStatementTemp1377;const auto nativeStatementTemp1380=JSValue("x");const auto nativeStatementTemp1381=([&]()->JSValue{const auto nativeStatementTemp1384=nativeStatementTemp1376.get("clamp");const JSValue nativeStatementTemp1385;Args nativeStatementTemp1386;nativeStatementTemp1386.push_back(([&]()->JSValue{const auto nativeStatementTemp1387=nativeStatementTemp1377;const auto nativeStatementTemp1388=JSValue("x");return nativeStatementTemp1387.get(nativeStatementTemp1388);}()));nativeStatementTemp1386.push_back(minVal);nativeStatementTemp1386.push_back(maxVal);return nativeStatementTemp1384.call(nativeStatementTemp1385,nativeStatementTemp1386);}());setReference(nativeStatementTemp1379,nativeStatementTemp1380,nativeStatementTemp1381);return nativeStatementTemp1381;}());
([&]()->JSValue{auto nativeStatementTemp1389=nativeStatementTemp1377;const auto nativeStatementTemp1390=JSValue("y");const auto nativeStatementTemp1391=([&]()->JSValue{const auto nativeStatementTemp1394=nativeStatementTemp1376.get("clamp");const JSValue nativeStatementTemp1395;Args nativeStatementTemp1396;nativeStatementTemp1396.push_back(([&]()->JSValue{const auto nativeStatementTemp1397=nativeStatementTemp1377;const auto nativeStatementTemp1398=JSValue("y");return nativeStatementTemp1397.get(nativeStatementTemp1398);}()));nativeStatementTemp1396.push_back(minVal);nativeStatementTemp1396.push_back(maxVal);return nativeStatementTemp1394.call(nativeStatementTemp1395,nativeStatementTemp1396);}());setReference(nativeStatementTemp1389,nativeStatementTemp1390,nativeStatementTemp1391);return nativeStatementTemp1391;}());
([&]()->JSValue{auto nativeStatementTemp1399=nativeStatementTemp1377;const auto nativeStatementTemp1400=JSValue("z");const auto nativeStatementTemp1401=([&]()->JSValue{const auto nativeStatementTemp1404=nativeStatementTemp1376.get("clamp");const JSValue nativeStatementTemp1405;Args nativeStatementTemp1406;nativeStatementTemp1406.push_back(([&]()->JSValue{const auto nativeStatementTemp1407=nativeStatementTemp1377;const auto nativeStatementTemp1408=JSValue("z");return nativeStatementTemp1407.get(nativeStatementTemp1408);}()));nativeStatementTemp1406.push_back(minVal);nativeStatementTemp1406.push_back(maxVal);return nativeStatementTemp1404.call(nativeStatementTemp1405,nativeStatementTemp1406);}());setReference(nativeStatementTemp1399,nativeStatementTemp1400,nativeStatementTemp1401);return nativeStatementTemp1401;}());
([&]()->JSValue{auto nativeStatementTemp1409=nativeStatementTemp1377;const auto nativeStatementTemp1410=JSValue("w");const auto nativeStatementTemp1411=([&]()->JSValue{const auto nativeStatementTemp1414=nativeStatementTemp1376.get("clamp");const JSValue nativeStatementTemp1415;Args nativeStatementTemp1416;nativeStatementTemp1416.push_back(([&]()->JSValue{const auto nativeStatementTemp1417=nativeStatementTemp1377;const auto nativeStatementTemp1418=JSValue("w");return nativeStatementTemp1417.get(nativeStatementTemp1418);}()));nativeStatementTemp1416.push_back(minVal);nativeStatementTemp1416.push_back(maxVal);return nativeStatementTemp1414.call(nativeStatementTemp1415,nativeStatementTemp1416);}());setReference(nativeStatementTemp1409,nativeStatementTemp1410,nativeStatementTemp1411);return nativeStatementTemp1411;}());
return nativeStatementTemp1377;
return JSValue{};
}
JSValue method34(const JSValue& nativeStatementTemp1419,const Args& nativeStatementTemp1420,const Args& nativeStatementTemp1421){
auto nativeStatementTemp1422=nativeStatementTemp1421[0];auto nativeStatementTemp1423=nativeStatementTemp1419;
JSValue min=arg(nativeStatementTemp1420,0);
JSValue max=arg(nativeStatementTemp1420,1);
JSValue length=([&]()->JSValue{auto nativeStatementTemp1428=nativeStatementTemp1423;const auto nativeStatementTemp1429=JSValue("length");const auto nativeStatementTemp1425=nativeStatementTemp1428.get(nativeStatementTemp1429);const auto nativeStatementTemp1426=nativeStatementTemp1428;Args nativeStatementTemp1427;return nativeStatementTemp1425.call(nativeStatementTemp1426,nativeStatementTemp1427);}());
return ([&]()->JSValue{auto nativeStatementTemp1433=([&]()->JSValue{auto nativeStatementTemp1438=nativeStatementTemp1423;const auto nativeStatementTemp1439=JSValue("divideScalar");const auto nativeStatementTemp1435=nativeStatementTemp1438.get(nativeStatementTemp1439);const auto nativeStatementTemp1436=nativeStatementTemp1438;Args nativeStatementTemp1437;nativeStatementTemp1437.push_back(([&]()->JSValue{const auto nativeStatementTemp1440=length;if(nativeStatementTemp1440.toBoolean())return nativeStatementTemp1440;return JSValue(1);}()));return nativeStatementTemp1435.call(nativeStatementTemp1436,nativeStatementTemp1437);}());const auto nativeStatementTemp1434=JSValue("multiplyScalar");const auto nativeStatementTemp1430=nativeStatementTemp1433.get(nativeStatementTemp1434);const auto nativeStatementTemp1431=nativeStatementTemp1433;Args nativeStatementTemp1432;nativeStatementTemp1432.push_back(([&]()->JSValue{const auto nativeStatementTemp1441=nativeStatementTemp1422.get("clamp");const JSValue nativeStatementTemp1442;Args nativeStatementTemp1443;nativeStatementTemp1443.push_back(length);nativeStatementTemp1443.push_back(min);nativeStatementTemp1443.push_back(max);return nativeStatementTemp1441.call(nativeStatementTemp1442,nativeStatementTemp1443);}()));return nativeStatementTemp1430.call(nativeStatementTemp1431,nativeStatementTemp1432);}());
return JSValue{};
}
JSValue method35(const JSValue& nativeStatementTemp1444,const Args& nativeStatementTemp1445,const Args& nativeStatementTemp1446){
auto nativeStatementTemp1447=nativeStatementTemp1446[0];auto nativeStatementTemp1448=nativeStatementTemp1444;
([&]()->JSValue{auto nativeStatementTemp1450=nativeStatementTemp1448;const auto nativeStatementTemp1451=JSValue("x");const auto nativeStatementTemp1452=([&]()->JSValue{auto nativeStatementTemp1458=nativeGlobalBinding("Math");const auto nativeStatementTemp1459=JSValue("floor");const auto nativeStatementTemp1455=nativeStatementTemp1458.get(nativeStatementTemp1459);const auto nativeStatementTemp1456=nativeStatementTemp1458;Args nativeStatementTemp1457;nativeStatementTemp1457.push_back(([&]()->JSValue{const auto nativeStatementTemp1460=nativeStatementTemp1448;const auto nativeStatementTemp1461=JSValue("x");return nativeStatementTemp1460.get(nativeStatementTemp1461);}()));return nativeStatementTemp1455.call(nativeStatementTemp1456,nativeStatementTemp1457);}());setReference(nativeStatementTemp1450,nativeStatementTemp1451,nativeStatementTemp1452);return nativeStatementTemp1452;}());
([&]()->JSValue{auto nativeStatementTemp1462=nativeStatementTemp1448;const auto nativeStatementTemp1463=JSValue("y");const auto nativeStatementTemp1464=([&]()->JSValue{auto nativeStatementTemp1470=nativeGlobalBinding("Math");const auto nativeStatementTemp1471=JSValue("floor");const auto nativeStatementTemp1467=nativeStatementTemp1470.get(nativeStatementTemp1471);const auto nativeStatementTemp1468=nativeStatementTemp1470;Args nativeStatementTemp1469;nativeStatementTemp1469.push_back(([&]()->JSValue{const auto nativeStatementTemp1472=nativeStatementTemp1448;const auto nativeStatementTemp1473=JSValue("y");return nativeStatementTemp1472.get(nativeStatementTemp1473);}()));return nativeStatementTemp1467.call(nativeStatementTemp1468,nativeStatementTemp1469);}());setReference(nativeStatementTemp1462,nativeStatementTemp1463,nativeStatementTemp1464);return nativeStatementTemp1464;}());
([&]()->JSValue{auto nativeStatementTemp1474=nativeStatementTemp1448;const auto nativeStatementTemp1475=JSValue("z");const auto nativeStatementTemp1476=([&]()->JSValue{auto nativeStatementTemp1482=nativeGlobalBinding("Math");const auto nativeStatementTemp1483=JSValue("floor");const auto nativeStatementTemp1479=nativeStatementTemp1482.get(nativeStatementTemp1483);const auto nativeStatementTemp1480=nativeStatementTemp1482;Args nativeStatementTemp1481;nativeStatementTemp1481.push_back(([&]()->JSValue{const auto nativeStatementTemp1484=nativeStatementTemp1448;const auto nativeStatementTemp1485=JSValue("z");return nativeStatementTemp1484.get(nativeStatementTemp1485);}()));return nativeStatementTemp1479.call(nativeStatementTemp1480,nativeStatementTemp1481);}());setReference(nativeStatementTemp1474,nativeStatementTemp1475,nativeStatementTemp1476);return nativeStatementTemp1476;}());
([&]()->JSValue{auto nativeStatementTemp1486=nativeStatementTemp1448;const auto nativeStatementTemp1487=JSValue("w");const auto nativeStatementTemp1488=([&]()->JSValue{auto nativeStatementTemp1494=nativeGlobalBinding("Math");const auto nativeStatementTemp1495=JSValue("floor");const auto nativeStatementTemp1491=nativeStatementTemp1494.get(nativeStatementTemp1495);const auto nativeStatementTemp1492=nativeStatementTemp1494;Args nativeStatementTemp1493;nativeStatementTemp1493.push_back(([&]()->JSValue{const auto nativeStatementTemp1496=nativeStatementTemp1448;const auto nativeStatementTemp1497=JSValue("w");return nativeStatementTemp1496.get(nativeStatementTemp1497);}()));return nativeStatementTemp1491.call(nativeStatementTemp1492,nativeStatementTemp1493);}());setReference(nativeStatementTemp1486,nativeStatementTemp1487,nativeStatementTemp1488);return nativeStatementTemp1488;}());
return nativeStatementTemp1448;
return JSValue{};
}
JSValue method36(const JSValue& nativeStatementTemp1498,const Args& nativeStatementTemp1499,const Args& nativeStatementTemp1500){
auto nativeStatementTemp1501=nativeStatementTemp1500[0];auto nativeStatementTemp1502=nativeStatementTemp1498;
([&]()->JSValue{auto nativeStatementTemp1504=nativeStatementTemp1502;const auto nativeStatementTemp1505=JSValue("x");const auto nativeStatementTemp1506=([&]()->JSValue{auto nativeStatementTemp1512=nativeGlobalBinding("Math");const auto nativeStatementTemp1513=JSValue("ceil");const auto nativeStatementTemp1509=nativeStatementTemp1512.get(nativeStatementTemp1513);const auto nativeStatementTemp1510=nativeStatementTemp1512;Args nativeStatementTemp1511;nativeStatementTemp1511.push_back(([&]()->JSValue{const auto nativeStatementTemp1514=nativeStatementTemp1502;const auto nativeStatementTemp1515=JSValue("x");return nativeStatementTemp1514.get(nativeStatementTemp1515);}()));return nativeStatementTemp1509.call(nativeStatementTemp1510,nativeStatementTemp1511);}());setReference(nativeStatementTemp1504,nativeStatementTemp1505,nativeStatementTemp1506);return nativeStatementTemp1506;}());
([&]()->JSValue{auto nativeStatementTemp1516=nativeStatementTemp1502;const auto nativeStatementTemp1517=JSValue("y");const auto nativeStatementTemp1518=([&]()->JSValue{auto nativeStatementTemp1524=nativeGlobalBinding("Math");const auto nativeStatementTemp1525=JSValue("ceil");const auto nativeStatementTemp1521=nativeStatementTemp1524.get(nativeStatementTemp1525);const auto nativeStatementTemp1522=nativeStatementTemp1524;Args nativeStatementTemp1523;nativeStatementTemp1523.push_back(([&]()->JSValue{const auto nativeStatementTemp1526=nativeStatementTemp1502;const auto nativeStatementTemp1527=JSValue("y");return nativeStatementTemp1526.get(nativeStatementTemp1527);}()));return nativeStatementTemp1521.call(nativeStatementTemp1522,nativeStatementTemp1523);}());setReference(nativeStatementTemp1516,nativeStatementTemp1517,nativeStatementTemp1518);return nativeStatementTemp1518;}());
([&]()->JSValue{auto nativeStatementTemp1528=nativeStatementTemp1502;const auto nativeStatementTemp1529=JSValue("z");const auto nativeStatementTemp1530=([&]()->JSValue{auto nativeStatementTemp1536=nativeGlobalBinding("Math");const auto nativeStatementTemp1537=JSValue("ceil");const auto nativeStatementTemp1533=nativeStatementTemp1536.get(nativeStatementTemp1537);const auto nativeStatementTemp1534=nativeStatementTemp1536;Args nativeStatementTemp1535;nativeStatementTemp1535.push_back(([&]()->JSValue{const auto nativeStatementTemp1538=nativeStatementTemp1502;const auto nativeStatementTemp1539=JSValue("z");return nativeStatementTemp1538.get(nativeStatementTemp1539);}()));return nativeStatementTemp1533.call(nativeStatementTemp1534,nativeStatementTemp1535);}());setReference(nativeStatementTemp1528,nativeStatementTemp1529,nativeStatementTemp1530);return nativeStatementTemp1530;}());
([&]()->JSValue{auto nativeStatementTemp1540=nativeStatementTemp1502;const auto nativeStatementTemp1541=JSValue("w");const auto nativeStatementTemp1542=([&]()->JSValue{auto nativeStatementTemp1548=nativeGlobalBinding("Math");const auto nativeStatementTemp1549=JSValue("ceil");const auto nativeStatementTemp1545=nativeStatementTemp1548.get(nativeStatementTemp1549);const auto nativeStatementTemp1546=nativeStatementTemp1548;Args nativeStatementTemp1547;nativeStatementTemp1547.push_back(([&]()->JSValue{const auto nativeStatementTemp1550=nativeStatementTemp1502;const auto nativeStatementTemp1551=JSValue("w");return nativeStatementTemp1550.get(nativeStatementTemp1551);}()));return nativeStatementTemp1545.call(nativeStatementTemp1546,nativeStatementTemp1547);}());setReference(nativeStatementTemp1540,nativeStatementTemp1541,nativeStatementTemp1542);return nativeStatementTemp1542;}());
return nativeStatementTemp1502;
return JSValue{};
}
JSValue method37(const JSValue& nativeStatementTemp1552,const Args& nativeStatementTemp1553,const Args& nativeStatementTemp1554){
auto nativeStatementTemp1555=nativeStatementTemp1554[0];auto nativeStatementTemp1556=nativeStatementTemp1552;
([&]()->JSValue{auto nativeStatementTemp1558=nativeStatementTemp1556;const auto nativeStatementTemp1559=JSValue("x");const auto nativeStatementTemp1560=([&]()->JSValue{auto nativeStatementTemp1566=nativeGlobalBinding("Math");const auto nativeStatementTemp1567=JSValue("round");const auto nativeStatementTemp1563=nativeStatementTemp1566.get(nativeStatementTemp1567);const auto nativeStatementTemp1564=nativeStatementTemp1566;Args nativeStatementTemp1565;nativeStatementTemp1565.push_back(([&]()->JSValue{const auto nativeStatementTemp1568=nativeStatementTemp1556;const auto nativeStatementTemp1569=JSValue("x");return nativeStatementTemp1568.get(nativeStatementTemp1569);}()));return nativeStatementTemp1563.call(nativeStatementTemp1564,nativeStatementTemp1565);}());setReference(nativeStatementTemp1558,nativeStatementTemp1559,nativeStatementTemp1560);return nativeStatementTemp1560;}());
([&]()->JSValue{auto nativeStatementTemp1570=nativeStatementTemp1556;const auto nativeStatementTemp1571=JSValue("y");const auto nativeStatementTemp1572=([&]()->JSValue{auto nativeStatementTemp1578=nativeGlobalBinding("Math");const auto nativeStatementTemp1579=JSValue("round");const auto nativeStatementTemp1575=nativeStatementTemp1578.get(nativeStatementTemp1579);const auto nativeStatementTemp1576=nativeStatementTemp1578;Args nativeStatementTemp1577;nativeStatementTemp1577.push_back(([&]()->JSValue{const auto nativeStatementTemp1580=nativeStatementTemp1556;const auto nativeStatementTemp1581=JSValue("y");return nativeStatementTemp1580.get(nativeStatementTemp1581);}()));return nativeStatementTemp1575.call(nativeStatementTemp1576,nativeStatementTemp1577);}());setReference(nativeStatementTemp1570,nativeStatementTemp1571,nativeStatementTemp1572);return nativeStatementTemp1572;}());
([&]()->JSValue{auto nativeStatementTemp1582=nativeStatementTemp1556;const auto nativeStatementTemp1583=JSValue("z");const auto nativeStatementTemp1584=([&]()->JSValue{auto nativeStatementTemp1590=nativeGlobalBinding("Math");const auto nativeStatementTemp1591=JSValue("round");const auto nativeStatementTemp1587=nativeStatementTemp1590.get(nativeStatementTemp1591);const auto nativeStatementTemp1588=nativeStatementTemp1590;Args nativeStatementTemp1589;nativeStatementTemp1589.push_back(([&]()->JSValue{const auto nativeStatementTemp1592=nativeStatementTemp1556;const auto nativeStatementTemp1593=JSValue("z");return nativeStatementTemp1592.get(nativeStatementTemp1593);}()));return nativeStatementTemp1587.call(nativeStatementTemp1588,nativeStatementTemp1589);}());setReference(nativeStatementTemp1582,nativeStatementTemp1583,nativeStatementTemp1584);return nativeStatementTemp1584;}());
([&]()->JSValue{auto nativeStatementTemp1594=nativeStatementTemp1556;const auto nativeStatementTemp1595=JSValue("w");const auto nativeStatementTemp1596=([&]()->JSValue{auto nativeStatementTemp1602=nativeGlobalBinding("Math");const auto nativeStatementTemp1603=JSValue("round");const auto nativeStatementTemp1599=nativeStatementTemp1602.get(nativeStatementTemp1603);const auto nativeStatementTemp1600=nativeStatementTemp1602;Args nativeStatementTemp1601;nativeStatementTemp1601.push_back(([&]()->JSValue{const auto nativeStatementTemp1604=nativeStatementTemp1556;const auto nativeStatementTemp1605=JSValue("w");return nativeStatementTemp1604.get(nativeStatementTemp1605);}()));return nativeStatementTemp1599.call(nativeStatementTemp1600,nativeStatementTemp1601);}());setReference(nativeStatementTemp1594,nativeStatementTemp1595,nativeStatementTemp1596);return nativeStatementTemp1596;}());
return nativeStatementTemp1556;
return JSValue{};
}
JSValue method38(const JSValue& nativeStatementTemp1606,const Args& nativeStatementTemp1607,const Args& nativeStatementTemp1608){
auto nativeStatementTemp1609=nativeStatementTemp1608[0];auto nativeStatementTemp1610=nativeStatementTemp1606;
([&]()->JSValue{auto nativeStatementTemp1612=nativeStatementTemp1610;const auto nativeStatementTemp1613=JSValue("x");const auto nativeStatementTemp1614=([&]()->JSValue{auto nativeStatementTemp1620=nativeGlobalBinding("Math");const auto nativeStatementTemp1621=JSValue("trunc");const auto nativeStatementTemp1617=nativeStatementTemp1620.get(nativeStatementTemp1621);const auto nativeStatementTemp1618=nativeStatementTemp1620;Args nativeStatementTemp1619;nativeStatementTemp1619.push_back(([&]()->JSValue{const auto nativeStatementTemp1622=nativeStatementTemp1610;const auto nativeStatementTemp1623=JSValue("x");return nativeStatementTemp1622.get(nativeStatementTemp1623);}()));return nativeStatementTemp1617.call(nativeStatementTemp1618,nativeStatementTemp1619);}());setReference(nativeStatementTemp1612,nativeStatementTemp1613,nativeStatementTemp1614);return nativeStatementTemp1614;}());
([&]()->JSValue{auto nativeStatementTemp1624=nativeStatementTemp1610;const auto nativeStatementTemp1625=JSValue("y");const auto nativeStatementTemp1626=([&]()->JSValue{auto nativeStatementTemp1632=nativeGlobalBinding("Math");const auto nativeStatementTemp1633=JSValue("trunc");const auto nativeStatementTemp1629=nativeStatementTemp1632.get(nativeStatementTemp1633);const auto nativeStatementTemp1630=nativeStatementTemp1632;Args nativeStatementTemp1631;nativeStatementTemp1631.push_back(([&]()->JSValue{const auto nativeStatementTemp1634=nativeStatementTemp1610;const auto nativeStatementTemp1635=JSValue("y");return nativeStatementTemp1634.get(nativeStatementTemp1635);}()));return nativeStatementTemp1629.call(nativeStatementTemp1630,nativeStatementTemp1631);}());setReference(nativeStatementTemp1624,nativeStatementTemp1625,nativeStatementTemp1626);return nativeStatementTemp1626;}());
([&]()->JSValue{auto nativeStatementTemp1636=nativeStatementTemp1610;const auto nativeStatementTemp1637=JSValue("z");const auto nativeStatementTemp1638=([&]()->JSValue{auto nativeStatementTemp1644=nativeGlobalBinding("Math");const auto nativeStatementTemp1645=JSValue("trunc");const auto nativeStatementTemp1641=nativeStatementTemp1644.get(nativeStatementTemp1645);const auto nativeStatementTemp1642=nativeStatementTemp1644;Args nativeStatementTemp1643;nativeStatementTemp1643.push_back(([&]()->JSValue{const auto nativeStatementTemp1646=nativeStatementTemp1610;const auto nativeStatementTemp1647=JSValue("z");return nativeStatementTemp1646.get(nativeStatementTemp1647);}()));return nativeStatementTemp1641.call(nativeStatementTemp1642,nativeStatementTemp1643);}());setReference(nativeStatementTemp1636,nativeStatementTemp1637,nativeStatementTemp1638);return nativeStatementTemp1638;}());
([&]()->JSValue{auto nativeStatementTemp1648=nativeStatementTemp1610;const auto nativeStatementTemp1649=JSValue("w");const auto nativeStatementTemp1650=([&]()->JSValue{auto nativeStatementTemp1656=nativeGlobalBinding("Math");const auto nativeStatementTemp1657=JSValue("trunc");const auto nativeStatementTemp1653=nativeStatementTemp1656.get(nativeStatementTemp1657);const auto nativeStatementTemp1654=nativeStatementTemp1656;Args nativeStatementTemp1655;nativeStatementTemp1655.push_back(([&]()->JSValue{const auto nativeStatementTemp1658=nativeStatementTemp1610;const auto nativeStatementTemp1659=JSValue("w");return nativeStatementTemp1658.get(nativeStatementTemp1659);}()));return nativeStatementTemp1653.call(nativeStatementTemp1654,nativeStatementTemp1655);}());setReference(nativeStatementTemp1648,nativeStatementTemp1649,nativeStatementTemp1650);return nativeStatementTemp1650;}());
return nativeStatementTemp1610;
return JSValue{};
}
JSValue method39(const JSValue& nativeStatementTemp1660,const Args& nativeStatementTemp1661,const Args& nativeStatementTemp1662){
auto nativeStatementTemp1663=nativeStatementTemp1662[0];auto nativeStatementTemp1664=nativeStatementTemp1660;
([&]()->JSValue{auto nativeStatementTemp1666=nativeStatementTemp1664;const auto nativeStatementTemp1667=JSValue("x");const auto nativeStatementTemp1668=([&]()->JSValue{const auto nativeStatementTemp1671=([&]()->JSValue{const auto nativeStatementTemp1672=nativeStatementTemp1664;const auto nativeStatementTemp1673=JSValue("x");return nativeStatementTemp1672.get(nativeStatementTemp1673);}());return -nativeStatementTemp1671.toNumber();}());setReference(nativeStatementTemp1666,nativeStatementTemp1667,nativeStatementTemp1668);return nativeStatementTemp1668;}());
([&]()->JSValue{auto nativeStatementTemp1674=nativeStatementTemp1664;const auto nativeStatementTemp1675=JSValue("y");const auto nativeStatementTemp1676=([&]()->JSValue{const auto nativeStatementTemp1679=([&]()->JSValue{const auto nativeStatementTemp1680=nativeStatementTemp1664;const auto nativeStatementTemp1681=JSValue("y");return nativeStatementTemp1680.get(nativeStatementTemp1681);}());return -nativeStatementTemp1679.toNumber();}());setReference(nativeStatementTemp1674,nativeStatementTemp1675,nativeStatementTemp1676);return nativeStatementTemp1676;}());
([&]()->JSValue{auto nativeStatementTemp1682=nativeStatementTemp1664;const auto nativeStatementTemp1683=JSValue("z");const auto nativeStatementTemp1684=([&]()->JSValue{const auto nativeStatementTemp1687=([&]()->JSValue{const auto nativeStatementTemp1688=nativeStatementTemp1664;const auto nativeStatementTemp1689=JSValue("z");return nativeStatementTemp1688.get(nativeStatementTemp1689);}());return -nativeStatementTemp1687.toNumber();}());setReference(nativeStatementTemp1682,nativeStatementTemp1683,nativeStatementTemp1684);return nativeStatementTemp1684;}());
([&]()->JSValue{auto nativeStatementTemp1690=nativeStatementTemp1664;const auto nativeStatementTemp1691=JSValue("w");const auto nativeStatementTemp1692=([&]()->JSValue{const auto nativeStatementTemp1695=([&]()->JSValue{const auto nativeStatementTemp1696=nativeStatementTemp1664;const auto nativeStatementTemp1697=JSValue("w");return nativeStatementTemp1696.get(nativeStatementTemp1697);}());return -nativeStatementTemp1695.toNumber();}());setReference(nativeStatementTemp1690,nativeStatementTemp1691,nativeStatementTemp1692);return nativeStatementTemp1692;}());
return nativeStatementTemp1664;
return JSValue{};
}
JSValue method40(const JSValue& nativeStatementTemp1698,const Args& nativeStatementTemp1699,const Args& nativeStatementTemp1700){
auto nativeStatementTemp1701=nativeStatementTemp1700[0];auto nativeStatementTemp1702=nativeStatementTemp1698;
JSValue v=arg(nativeStatementTemp1699,0);
return ([&]()->JSValue{const auto nativeStatementTemp1704=([&]()->JSValue{const auto nativeStatementTemp1706=([&]()->JSValue{const auto nativeStatementTemp1708=([&]()->JSValue{const auto nativeStatementTemp1710=([&]()->JSValue{const auto nativeStatementTemp1712=nativeStatementTemp1702;const auto nativeStatementTemp1713=JSValue("x");return nativeStatementTemp1712.get(nativeStatementTemp1713);}());const auto nativeStatementTemp1711=([&]()->JSValue{const auto nativeStatementTemp1714=v;const auto nativeStatementTemp1715=JSValue("x");return nativeStatementTemp1714.get(nativeStatementTemp1715);}());return binary("*",nativeStatementTemp1710,nativeStatementTemp1711);}());const auto nativeStatementTemp1709=([&]()->JSValue{const auto nativeStatementTemp1716=([&]()->JSValue{const auto nativeStatementTemp1718=nativeStatementTemp1702;const auto nativeStatementTemp1719=JSValue("y");return nativeStatementTemp1718.get(nativeStatementTemp1719);}());const auto nativeStatementTemp1717=([&]()->JSValue{const auto nativeStatementTemp1720=v;const auto nativeStatementTemp1721=JSValue("y");return nativeStatementTemp1720.get(nativeStatementTemp1721);}());return binary("*",nativeStatementTemp1716,nativeStatementTemp1717);}());return binary("+",nativeStatementTemp1708,nativeStatementTemp1709);}());const auto nativeStatementTemp1707=([&]()->JSValue{const auto nativeStatementTemp1722=([&]()->JSValue{const auto nativeStatementTemp1724=nativeStatementTemp1702;const auto nativeStatementTemp1725=JSValue("z");return nativeStatementTemp1724.get(nativeStatementTemp1725);}());const auto nativeStatementTemp1723=([&]()->JSValue{const auto nativeStatementTemp1726=v;const auto nativeStatementTemp1727=JSValue("z");return nativeStatementTemp1726.get(nativeStatementTemp1727);}());return binary("*",nativeStatementTemp1722,nativeStatementTemp1723);}());return binary("+",nativeStatementTemp1706,nativeStatementTemp1707);}());const auto nativeStatementTemp1705=([&]()->JSValue{const auto nativeStatementTemp1728=([&]()->JSValue{const auto nativeStatementTemp1730=nativeStatementTemp1702;const auto nativeStatementTemp1731=JSValue("w");return nativeStatementTemp1730.get(nativeStatementTemp1731);}());const auto nativeStatementTemp1729=([&]()->JSValue{const auto nativeStatementTemp1732=v;const auto nativeStatementTemp1733=JSValue("w");return nativeStatementTemp1732.get(nativeStatementTemp1733);}());return binary("*",nativeStatementTemp1728,nativeStatementTemp1729);}());return binary("+",nativeStatementTemp1704,nativeStatementTemp1705);}());
return JSValue{};
}
JSValue method41(const JSValue& nativeStatementTemp1734,const Args& nativeStatementTemp1735,const Args& nativeStatementTemp1736){
auto nativeStatementTemp1737=nativeStatementTemp1736[0];auto nativeStatementTemp1738=nativeStatementTemp1734;
return ([&]()->JSValue{const auto nativeStatementTemp1740=([&]()->JSValue{const auto nativeStatementTemp1742=([&]()->JSValue{const auto nativeStatementTemp1744=([&]()->JSValue{const auto nativeStatementTemp1746=([&]()->JSValue{const auto nativeStatementTemp1748=nativeStatementTemp1738;const auto nativeStatementTemp1749=JSValue("x");return nativeStatementTemp1748.get(nativeStatementTemp1749);}());const auto nativeStatementTemp1747=([&]()->JSValue{const auto nativeStatementTemp1750=nativeStatementTemp1738;const auto nativeStatementTemp1751=JSValue("x");return nativeStatementTemp1750.get(nativeStatementTemp1751);}());return binary("*",nativeStatementTemp1746,nativeStatementTemp1747);}());const auto nativeStatementTemp1745=([&]()->JSValue{const auto nativeStatementTemp1752=([&]()->JSValue{const auto nativeStatementTemp1754=nativeStatementTemp1738;const auto nativeStatementTemp1755=JSValue("y");return nativeStatementTemp1754.get(nativeStatementTemp1755);}());const auto nativeStatementTemp1753=([&]()->JSValue{const auto nativeStatementTemp1756=nativeStatementTemp1738;const auto nativeStatementTemp1757=JSValue("y");return nativeStatementTemp1756.get(nativeStatementTemp1757);}());return binary("*",nativeStatementTemp1752,nativeStatementTemp1753);}());return binary("+",nativeStatementTemp1744,nativeStatementTemp1745);}());const auto nativeStatementTemp1743=([&]()->JSValue{const auto nativeStatementTemp1758=([&]()->JSValue{const auto nativeStatementTemp1760=nativeStatementTemp1738;const auto nativeStatementTemp1761=JSValue("z");return nativeStatementTemp1760.get(nativeStatementTemp1761);}());const auto nativeStatementTemp1759=([&]()->JSValue{const auto nativeStatementTemp1762=nativeStatementTemp1738;const auto nativeStatementTemp1763=JSValue("z");return nativeStatementTemp1762.get(nativeStatementTemp1763);}());return binary("*",nativeStatementTemp1758,nativeStatementTemp1759);}());return binary("+",nativeStatementTemp1742,nativeStatementTemp1743);}());const auto nativeStatementTemp1741=([&]()->JSValue{const auto nativeStatementTemp1764=([&]()->JSValue{const auto nativeStatementTemp1766=nativeStatementTemp1738;const auto nativeStatementTemp1767=JSValue("w");return nativeStatementTemp1766.get(nativeStatementTemp1767);}());const auto nativeStatementTemp1765=([&]()->JSValue{const auto nativeStatementTemp1768=nativeStatementTemp1738;const auto nativeStatementTemp1769=JSValue("w");return nativeStatementTemp1768.get(nativeStatementTemp1769);}());return binary("*",nativeStatementTemp1764,nativeStatementTemp1765);}());return binary("+",nativeStatementTemp1740,nativeStatementTemp1741);}());
return JSValue{};
}
JSValue method42(const JSValue& nativeStatementTemp1770,const Args& nativeStatementTemp1771,const Args& nativeStatementTemp1772){
auto nativeStatementTemp1773=nativeStatementTemp1772[0];auto nativeStatementTemp1774=nativeStatementTemp1770;
return ([&]()->JSValue{auto nativeStatementTemp1779=nativeGlobalBinding("Math");const auto nativeStatementTemp1780=JSValue("sqrt");const auto nativeStatementTemp1776=nativeStatementTemp1779.get(nativeStatementTemp1780);const auto nativeStatementTemp1777=nativeStatementTemp1779;Args nativeStatementTemp1778;nativeStatementTemp1778.push_back(([&]()->JSValue{const auto nativeStatementTemp1781=([&]()->JSValue{const auto nativeStatementTemp1783=([&]()->JSValue{const auto nativeStatementTemp1785=([&]()->JSValue{const auto nativeStatementTemp1787=([&]()->JSValue{const auto nativeStatementTemp1789=nativeStatementTemp1774;const auto nativeStatementTemp1790=JSValue("x");return nativeStatementTemp1789.get(nativeStatementTemp1790);}());const auto nativeStatementTemp1788=([&]()->JSValue{const auto nativeStatementTemp1791=nativeStatementTemp1774;const auto nativeStatementTemp1792=JSValue("x");return nativeStatementTemp1791.get(nativeStatementTemp1792);}());return binary("*",nativeStatementTemp1787,nativeStatementTemp1788);}());const auto nativeStatementTemp1786=([&]()->JSValue{const auto nativeStatementTemp1793=([&]()->JSValue{const auto nativeStatementTemp1795=nativeStatementTemp1774;const auto nativeStatementTemp1796=JSValue("y");return nativeStatementTemp1795.get(nativeStatementTemp1796);}());const auto nativeStatementTemp1794=([&]()->JSValue{const auto nativeStatementTemp1797=nativeStatementTemp1774;const auto nativeStatementTemp1798=JSValue("y");return nativeStatementTemp1797.get(nativeStatementTemp1798);}());return binary("*",nativeStatementTemp1793,nativeStatementTemp1794);}());return binary("+",nativeStatementTemp1785,nativeStatementTemp1786);}());const auto nativeStatementTemp1784=([&]()->JSValue{const auto nativeStatementTemp1799=([&]()->JSValue{const auto nativeStatementTemp1801=nativeStatementTemp1774;const auto nativeStatementTemp1802=JSValue("z");return nativeStatementTemp1801.get(nativeStatementTemp1802);}());const auto nativeStatementTemp1800=([&]()->JSValue{const auto nativeStatementTemp1803=nativeStatementTemp1774;const auto nativeStatementTemp1804=JSValue("z");return nativeStatementTemp1803.get(nativeStatementTemp1804);}());return binary("*",nativeStatementTemp1799,nativeStatementTemp1800);}());return binary("+",nativeStatementTemp1783,nativeStatementTemp1784);}());const auto nativeStatementTemp1782=([&]()->JSValue{const auto nativeStatementTemp1805=([&]()->JSValue{const auto nativeStatementTemp1807=nativeStatementTemp1774;const auto nativeStatementTemp1808=JSValue("w");return nativeStatementTemp1807.get(nativeStatementTemp1808);}());const auto nativeStatementTemp1806=([&]()->JSValue{const auto nativeStatementTemp1809=nativeStatementTemp1774;const auto nativeStatementTemp1810=JSValue("w");return nativeStatementTemp1809.get(nativeStatementTemp1810);}());return binary("*",nativeStatementTemp1805,nativeStatementTemp1806);}());return binary("+",nativeStatementTemp1781,nativeStatementTemp1782);}()));return nativeStatementTemp1776.call(nativeStatementTemp1777,nativeStatementTemp1778);}());
return JSValue{};
}
JSValue method43(const JSValue& nativeStatementTemp1811,const Args& nativeStatementTemp1812,const Args& nativeStatementTemp1813){
auto nativeStatementTemp1814=nativeStatementTemp1813[0];auto nativeStatementTemp1815=nativeStatementTemp1811;
return ([&]()->JSValue{const auto nativeStatementTemp1817=([&]()->JSValue{const auto nativeStatementTemp1819=([&]()->JSValue{const auto nativeStatementTemp1821=([&]()->JSValue{auto nativeStatementTemp1826=nativeGlobalBinding("Math");const auto nativeStatementTemp1827=JSValue("abs");const auto nativeStatementTemp1823=nativeStatementTemp1826.get(nativeStatementTemp1827);const auto nativeStatementTemp1824=nativeStatementTemp1826;Args nativeStatementTemp1825;nativeStatementTemp1825.push_back(([&]()->JSValue{const auto nativeStatementTemp1828=nativeStatementTemp1815;const auto nativeStatementTemp1829=JSValue("x");return nativeStatementTemp1828.get(nativeStatementTemp1829);}()));return nativeStatementTemp1823.call(nativeStatementTemp1824,nativeStatementTemp1825);}());const auto nativeStatementTemp1822=([&]()->JSValue{auto nativeStatementTemp1833=nativeGlobalBinding("Math");const auto nativeStatementTemp1834=JSValue("abs");const auto nativeStatementTemp1830=nativeStatementTemp1833.get(nativeStatementTemp1834);const auto nativeStatementTemp1831=nativeStatementTemp1833;Args nativeStatementTemp1832;nativeStatementTemp1832.push_back(([&]()->JSValue{const auto nativeStatementTemp1835=nativeStatementTemp1815;const auto nativeStatementTemp1836=JSValue("y");return nativeStatementTemp1835.get(nativeStatementTemp1836);}()));return nativeStatementTemp1830.call(nativeStatementTemp1831,nativeStatementTemp1832);}());return binary("+",nativeStatementTemp1821,nativeStatementTemp1822);}());const auto nativeStatementTemp1820=([&]()->JSValue{auto nativeStatementTemp1840=nativeGlobalBinding("Math");const auto nativeStatementTemp1841=JSValue("abs");const auto nativeStatementTemp1837=nativeStatementTemp1840.get(nativeStatementTemp1841);const auto nativeStatementTemp1838=nativeStatementTemp1840;Args nativeStatementTemp1839;nativeStatementTemp1839.push_back(([&]()->JSValue{const auto nativeStatementTemp1842=nativeStatementTemp1815;const auto nativeStatementTemp1843=JSValue("z");return nativeStatementTemp1842.get(nativeStatementTemp1843);}()));return nativeStatementTemp1837.call(nativeStatementTemp1838,nativeStatementTemp1839);}());return binary("+",nativeStatementTemp1819,nativeStatementTemp1820);}());const auto nativeStatementTemp1818=([&]()->JSValue{auto nativeStatementTemp1847=nativeGlobalBinding("Math");const auto nativeStatementTemp1848=JSValue("abs");const auto nativeStatementTemp1844=nativeStatementTemp1847.get(nativeStatementTemp1848);const auto nativeStatementTemp1845=nativeStatementTemp1847;Args nativeStatementTemp1846;nativeStatementTemp1846.push_back(([&]()->JSValue{const auto nativeStatementTemp1849=nativeStatementTemp1815;const auto nativeStatementTemp1850=JSValue("w");return nativeStatementTemp1849.get(nativeStatementTemp1850);}()));return nativeStatementTemp1844.call(nativeStatementTemp1845,nativeStatementTemp1846);}());return binary("+",nativeStatementTemp1817,nativeStatementTemp1818);}());
return JSValue{};
}
JSValue method44(const JSValue& nativeStatementTemp1851,const Args& nativeStatementTemp1852,const Args& nativeStatementTemp1853){
auto nativeStatementTemp1854=nativeStatementTemp1853[0];auto nativeStatementTemp1855=nativeStatementTemp1851;
return ([&]()->JSValue{auto nativeStatementTemp1860=nativeStatementTemp1855;const auto nativeStatementTemp1861=JSValue("divideScalar");const auto nativeStatementTemp1857=nativeStatementTemp1860.get(nativeStatementTemp1861);const auto nativeStatementTemp1858=nativeStatementTemp1860;Args nativeStatementTemp1859;nativeStatementTemp1859.push_back(([&]()->JSValue{const auto nativeStatementTemp1862=([&]()->JSValue{auto nativeStatementTemp1866=nativeStatementTemp1855;const auto nativeStatementTemp1867=JSValue("length");const auto nativeStatementTemp1863=nativeStatementTemp1866.get(nativeStatementTemp1867);const auto nativeStatementTemp1864=nativeStatementTemp1866;Args nativeStatementTemp1865;return nativeStatementTemp1863.call(nativeStatementTemp1864,nativeStatementTemp1865);}());if(nativeStatementTemp1862.toBoolean())return nativeStatementTemp1862;return JSValue(1);}()));return nativeStatementTemp1857.call(nativeStatementTemp1858,nativeStatementTemp1859);}());
return JSValue{};
}
JSValue method45(const JSValue& nativeStatementTemp1868,const Args& nativeStatementTemp1869,const Args& nativeStatementTemp1870){
auto nativeStatementTemp1871=nativeStatementTemp1870[0];auto nativeStatementTemp1872=nativeStatementTemp1868;
JSValue length=arg(nativeStatementTemp1869,0);
return ([&]()->JSValue{auto nativeStatementTemp1877=([&]()->JSValue{auto nativeStatementTemp1882=nativeStatementTemp1872;const auto nativeStatementTemp1883=JSValue("normalize");const auto nativeStatementTemp1879=nativeStatementTemp1882.get(nativeStatementTemp1883);const auto nativeStatementTemp1880=nativeStatementTemp1882;Args nativeStatementTemp1881;return nativeStatementTemp1879.call(nativeStatementTemp1880,nativeStatementTemp1881);}());const auto nativeStatementTemp1878=JSValue("multiplyScalar");const auto nativeStatementTemp1874=nativeStatementTemp1877.get(nativeStatementTemp1878);const auto nativeStatementTemp1875=nativeStatementTemp1877;Args nativeStatementTemp1876;nativeStatementTemp1876.push_back(length);return nativeStatementTemp1874.call(nativeStatementTemp1875,nativeStatementTemp1876);}());
return JSValue{};
}
JSValue method46(const JSValue& nativeStatementTemp1884,const Args& nativeStatementTemp1885,const Args& nativeStatementTemp1886){
auto nativeStatementTemp1887=nativeStatementTemp1886[0];auto nativeStatementTemp1888=nativeStatementTemp1884;
JSValue v=arg(nativeStatementTemp1885,0);
JSValue alpha=arg(nativeStatementTemp1885,1);
([&]()->JSValue{auto nativeStatementTemp1890=nativeStatementTemp1888;const auto nativeStatementTemp1891=JSValue("x");const auto nativeStatementTemp1893=nativeStatementTemp1890.get(nativeStatementTemp1891);const auto nativeStatementTemp1894=([&]()->JSValue{const auto nativeStatementTemp1895=([&]()->JSValue{const auto nativeStatementTemp1897=([&]()->JSValue{const auto nativeStatementTemp1899=v;const auto nativeStatementTemp1900=JSValue("x");return nativeStatementTemp1899.get(nativeStatementTemp1900);}());const auto nativeStatementTemp1898=([&]()->JSValue{const auto nativeStatementTemp1901=nativeStatementTemp1888;const auto nativeStatementTemp1902=JSValue("x");return nativeStatementTemp1901.get(nativeStatementTemp1902);}());return binary("-",nativeStatementTemp1897,nativeStatementTemp1898);}());const auto nativeStatementTemp1896=alpha;return binary("*",nativeStatementTemp1895,nativeStatementTemp1896);}());const auto nativeStatementTemp1892=binary("+",nativeStatementTemp1893,nativeStatementTemp1894);setReference(nativeStatementTemp1890,nativeStatementTemp1891,nativeStatementTemp1892);return nativeStatementTemp1892;}());
([&]()->JSValue{auto nativeStatementTemp1903=nativeStatementTemp1888;const auto nativeStatementTemp1904=JSValue("y");const auto nativeStatementTemp1906=nativeStatementTemp1903.get(nativeStatementTemp1904);const auto nativeStatementTemp1907=([&]()->JSValue{const auto nativeStatementTemp1908=([&]()->JSValue{const auto nativeStatementTemp1910=([&]()->JSValue{const auto nativeStatementTemp1912=v;const auto nativeStatementTemp1913=JSValue("y");return nativeStatementTemp1912.get(nativeStatementTemp1913);}());const auto nativeStatementTemp1911=([&]()->JSValue{const auto nativeStatementTemp1914=nativeStatementTemp1888;const auto nativeStatementTemp1915=JSValue("y");return nativeStatementTemp1914.get(nativeStatementTemp1915);}());return binary("-",nativeStatementTemp1910,nativeStatementTemp1911);}());const auto nativeStatementTemp1909=alpha;return binary("*",nativeStatementTemp1908,nativeStatementTemp1909);}());const auto nativeStatementTemp1905=binary("+",nativeStatementTemp1906,nativeStatementTemp1907);setReference(nativeStatementTemp1903,nativeStatementTemp1904,nativeStatementTemp1905);return nativeStatementTemp1905;}());
([&]()->JSValue{auto nativeStatementTemp1916=nativeStatementTemp1888;const auto nativeStatementTemp1917=JSValue("z");const auto nativeStatementTemp1919=nativeStatementTemp1916.get(nativeStatementTemp1917);const auto nativeStatementTemp1920=([&]()->JSValue{const auto nativeStatementTemp1921=([&]()->JSValue{const auto nativeStatementTemp1923=([&]()->JSValue{const auto nativeStatementTemp1925=v;const auto nativeStatementTemp1926=JSValue("z");return nativeStatementTemp1925.get(nativeStatementTemp1926);}());const auto nativeStatementTemp1924=([&]()->JSValue{const auto nativeStatementTemp1927=nativeStatementTemp1888;const auto nativeStatementTemp1928=JSValue("z");return nativeStatementTemp1927.get(nativeStatementTemp1928);}());return binary("-",nativeStatementTemp1923,nativeStatementTemp1924);}());const auto nativeStatementTemp1922=alpha;return binary("*",nativeStatementTemp1921,nativeStatementTemp1922);}());const auto nativeStatementTemp1918=binary("+",nativeStatementTemp1919,nativeStatementTemp1920);setReference(nativeStatementTemp1916,nativeStatementTemp1917,nativeStatementTemp1918);return nativeStatementTemp1918;}());
([&]()->JSValue{auto nativeStatementTemp1929=nativeStatementTemp1888;const auto nativeStatementTemp1930=JSValue("w");const auto nativeStatementTemp1932=nativeStatementTemp1929.get(nativeStatementTemp1930);const auto nativeStatementTemp1933=([&]()->JSValue{const auto nativeStatementTemp1934=([&]()->JSValue{const auto nativeStatementTemp1936=([&]()->JSValue{const auto nativeStatementTemp1938=v;const auto nativeStatementTemp1939=JSValue("w");return nativeStatementTemp1938.get(nativeStatementTemp1939);}());const auto nativeStatementTemp1937=([&]()->JSValue{const auto nativeStatementTemp1940=nativeStatementTemp1888;const auto nativeStatementTemp1941=JSValue("w");return nativeStatementTemp1940.get(nativeStatementTemp1941);}());return binary("-",nativeStatementTemp1936,nativeStatementTemp1937);}());const auto nativeStatementTemp1935=alpha;return binary("*",nativeStatementTemp1934,nativeStatementTemp1935);}());const auto nativeStatementTemp1931=binary("+",nativeStatementTemp1932,nativeStatementTemp1933);setReference(nativeStatementTemp1929,nativeStatementTemp1930,nativeStatementTemp1931);return nativeStatementTemp1931;}());
return nativeStatementTemp1888;
return JSValue{};
}
JSValue method47(const JSValue& nativeStatementTemp1942,const Args& nativeStatementTemp1943,const Args& nativeStatementTemp1944){
auto nativeStatementTemp1945=nativeStatementTemp1944[0];auto nativeStatementTemp1946=nativeStatementTemp1942;
JSValue v1=arg(nativeStatementTemp1943,0);
JSValue v2=arg(nativeStatementTemp1943,1);
JSValue alpha=arg(nativeStatementTemp1943,2);
([&]()->JSValue{auto nativeStatementTemp1948=nativeStatementTemp1946;const auto nativeStatementTemp1949=JSValue("x");const auto nativeStatementTemp1950=([&]()->JSValue{const auto nativeStatementTemp1953=([&]()->JSValue{const auto nativeStatementTemp1955=v1;const auto nativeStatementTemp1956=JSValue("x");return nativeStatementTemp1955.get(nativeStatementTemp1956);}());const auto nativeStatementTemp1954=([&]()->JSValue{const auto nativeStatementTemp1957=([&]()->JSValue{const auto nativeStatementTemp1959=([&]()->JSValue{const auto nativeStatementTemp1961=v2;const auto nativeStatementTemp1962=JSValue("x");return nativeStatementTemp1961.get(nativeStatementTemp1962);}());const auto nativeStatementTemp1960=([&]()->JSValue{const auto nativeStatementTemp1963=v1;const auto nativeStatementTemp1964=JSValue("x");return nativeStatementTemp1963.get(nativeStatementTemp1964);}());return binary("-",nativeStatementTemp1959,nativeStatementTemp1960);}());const auto nativeStatementTemp1958=alpha;return binary("*",nativeStatementTemp1957,nativeStatementTemp1958);}());return binary("+",nativeStatementTemp1953,nativeStatementTemp1954);}());setReference(nativeStatementTemp1948,nativeStatementTemp1949,nativeStatementTemp1950);return nativeStatementTemp1950;}());
([&]()->JSValue{auto nativeStatementTemp1965=nativeStatementTemp1946;const auto nativeStatementTemp1966=JSValue("y");const auto nativeStatementTemp1967=([&]()->JSValue{const auto nativeStatementTemp1970=([&]()->JSValue{const auto nativeStatementTemp1972=v1;const auto nativeStatementTemp1973=JSValue("y");return nativeStatementTemp1972.get(nativeStatementTemp1973);}());const auto nativeStatementTemp1971=([&]()->JSValue{const auto nativeStatementTemp1974=([&]()->JSValue{const auto nativeStatementTemp1976=([&]()->JSValue{const auto nativeStatementTemp1978=v2;const auto nativeStatementTemp1979=JSValue("y");return nativeStatementTemp1978.get(nativeStatementTemp1979);}());const auto nativeStatementTemp1977=([&]()->JSValue{const auto nativeStatementTemp1980=v1;const auto nativeStatementTemp1981=JSValue("y");return nativeStatementTemp1980.get(nativeStatementTemp1981);}());return binary("-",nativeStatementTemp1976,nativeStatementTemp1977);}());const auto nativeStatementTemp1975=alpha;return binary("*",nativeStatementTemp1974,nativeStatementTemp1975);}());return binary("+",nativeStatementTemp1970,nativeStatementTemp1971);}());setReference(nativeStatementTemp1965,nativeStatementTemp1966,nativeStatementTemp1967);return nativeStatementTemp1967;}());
([&]()->JSValue{auto nativeStatementTemp1982=nativeStatementTemp1946;const auto nativeStatementTemp1983=JSValue("z");const auto nativeStatementTemp1984=([&]()->JSValue{const auto nativeStatementTemp1987=([&]()->JSValue{const auto nativeStatementTemp1989=v1;const auto nativeStatementTemp1990=JSValue("z");return nativeStatementTemp1989.get(nativeStatementTemp1990);}());const auto nativeStatementTemp1988=([&]()->JSValue{const auto nativeStatementTemp1991=([&]()->JSValue{const auto nativeStatementTemp1993=([&]()->JSValue{const auto nativeStatementTemp1995=v2;const auto nativeStatementTemp1996=JSValue("z");return nativeStatementTemp1995.get(nativeStatementTemp1996);}());const auto nativeStatementTemp1994=([&]()->JSValue{const auto nativeStatementTemp1997=v1;const auto nativeStatementTemp1998=JSValue("z");return nativeStatementTemp1997.get(nativeStatementTemp1998);}());return binary("-",nativeStatementTemp1993,nativeStatementTemp1994);}());const auto nativeStatementTemp1992=alpha;return binary("*",nativeStatementTemp1991,nativeStatementTemp1992);}());return binary("+",nativeStatementTemp1987,nativeStatementTemp1988);}());setReference(nativeStatementTemp1982,nativeStatementTemp1983,nativeStatementTemp1984);return nativeStatementTemp1984;}());
([&]()->JSValue{auto nativeStatementTemp1999=nativeStatementTemp1946;const auto nativeStatementTemp2000=JSValue("w");const auto nativeStatementTemp2001=([&]()->JSValue{const auto nativeStatementTemp2004=([&]()->JSValue{const auto nativeStatementTemp2006=v1;const auto nativeStatementTemp2007=JSValue("w");return nativeStatementTemp2006.get(nativeStatementTemp2007);}());const auto nativeStatementTemp2005=([&]()->JSValue{const auto nativeStatementTemp2008=([&]()->JSValue{const auto nativeStatementTemp2010=([&]()->JSValue{const auto nativeStatementTemp2012=v2;const auto nativeStatementTemp2013=JSValue("w");return nativeStatementTemp2012.get(nativeStatementTemp2013);}());const auto nativeStatementTemp2011=([&]()->JSValue{const auto nativeStatementTemp2014=v1;const auto nativeStatementTemp2015=JSValue("w");return nativeStatementTemp2014.get(nativeStatementTemp2015);}());return binary("-",nativeStatementTemp2010,nativeStatementTemp2011);}());const auto nativeStatementTemp2009=alpha;return binary("*",nativeStatementTemp2008,nativeStatementTemp2009);}());return binary("+",nativeStatementTemp2004,nativeStatementTemp2005);}());setReference(nativeStatementTemp1999,nativeStatementTemp2000,nativeStatementTemp2001);return nativeStatementTemp2001;}());
return nativeStatementTemp1946;
return JSValue{};
}
JSValue method48(const JSValue& nativeStatementTemp2016,const Args& nativeStatementTemp2017,const Args& nativeStatementTemp2018){
auto nativeStatementTemp2019=nativeStatementTemp2018[0];auto nativeStatementTemp2020=nativeStatementTemp2016;
JSValue v=arg(nativeStatementTemp2017,0);
return ([&]()->JSValue{const auto nativeStatementTemp2022=([&]()->JSValue{const auto nativeStatementTemp2023=([&]()->JSValue{const auto nativeStatementTemp2024=([&]()->JSValue{const auto nativeStatementTemp2025=([&]()->JSValue{const auto nativeStatementTemp2027=v;const auto nativeStatementTemp2028=JSValue("x");return nativeStatementTemp2027.get(nativeStatementTemp2028);}());const auto nativeStatementTemp2026=([&]()->JSValue{const auto nativeStatementTemp2029=nativeStatementTemp2020;const auto nativeStatementTemp2030=JSValue("x");return nativeStatementTemp2029.get(nativeStatementTemp2030);}());return binary("===",nativeStatementTemp2025,nativeStatementTemp2026);}());if(!nativeStatementTemp2024.toBoolean())return nativeStatementTemp2024;return ([&]()->JSValue{const auto nativeStatementTemp2031=([&]()->JSValue{const auto nativeStatementTemp2033=v;const auto nativeStatementTemp2034=JSValue("y");return nativeStatementTemp2033.get(nativeStatementTemp2034);}());const auto nativeStatementTemp2032=([&]()->JSValue{const auto nativeStatementTemp2035=nativeStatementTemp2020;const auto nativeStatementTemp2036=JSValue("y");return nativeStatementTemp2035.get(nativeStatementTemp2036);}());return binary("===",nativeStatementTemp2031,nativeStatementTemp2032);}());}());if(!nativeStatementTemp2023.toBoolean())return nativeStatementTemp2023;return ([&]()->JSValue{const auto nativeStatementTemp2037=([&]()->JSValue{const auto nativeStatementTemp2039=v;const auto nativeStatementTemp2040=JSValue("z");return nativeStatementTemp2039.get(nativeStatementTemp2040);}());const auto nativeStatementTemp2038=([&]()->JSValue{const auto nativeStatementTemp2041=nativeStatementTemp2020;const auto nativeStatementTemp2042=JSValue("z");return nativeStatementTemp2041.get(nativeStatementTemp2042);}());return binary("===",nativeStatementTemp2037,nativeStatementTemp2038);}());}());if(!nativeStatementTemp2022.toBoolean())return nativeStatementTemp2022;return ([&]()->JSValue{const auto nativeStatementTemp2043=([&]()->JSValue{const auto nativeStatementTemp2045=v;const auto nativeStatementTemp2046=JSValue("w");return nativeStatementTemp2045.get(nativeStatementTemp2046);}());const auto nativeStatementTemp2044=([&]()->JSValue{const auto nativeStatementTemp2047=nativeStatementTemp2020;const auto nativeStatementTemp2048=JSValue("w");return nativeStatementTemp2047.get(nativeStatementTemp2048);}());return binary("===",nativeStatementTemp2043,nativeStatementTemp2044);}());}());
return JSValue{};
}
JSValue method49(const JSValue& nativeStatementTemp2049,const Args& nativeStatementTemp2050,const Args& nativeStatementTemp2051){
auto nativeStatementTemp2052=nativeStatementTemp2051[0];auto nativeStatementTemp2053=nativeStatementTemp2049;
JSValue array=arg(nativeStatementTemp2050,0);
JSValue offset=arg(nativeStatementTemp2050,1);
if(offset.isUndefined())offset=JSValue(0);
([&]()->JSValue{auto nativeStatementTemp2055=nativeStatementTemp2053;const auto nativeStatementTemp2056=JSValue("x");const auto nativeStatementTemp2057=([&]()->JSValue{const auto nativeStatementTemp2060=array;const auto nativeStatementTemp2061=offset;return nativeStatementTemp2060.get(nativeStatementTemp2061);}());setReference(nativeStatementTemp2055,nativeStatementTemp2056,nativeStatementTemp2057);return nativeStatementTemp2057;}());
([&]()->JSValue{auto nativeStatementTemp2062=nativeStatementTemp2053;const auto nativeStatementTemp2063=JSValue("y");const auto nativeStatementTemp2064=([&]()->JSValue{const auto nativeStatementTemp2067=array;const auto nativeStatementTemp2068=([&]()->JSValue{const auto nativeStatementTemp2069=offset;const auto nativeStatementTemp2070=JSValue(1);return binary("+",nativeStatementTemp2069,nativeStatementTemp2070);}());return nativeStatementTemp2067.get(nativeStatementTemp2068);}());setReference(nativeStatementTemp2062,nativeStatementTemp2063,nativeStatementTemp2064);return nativeStatementTemp2064;}());
([&]()->JSValue{auto nativeStatementTemp2071=nativeStatementTemp2053;const auto nativeStatementTemp2072=JSValue("z");const auto nativeStatementTemp2073=([&]()->JSValue{const auto nativeStatementTemp2076=array;const auto nativeStatementTemp2077=([&]()->JSValue{const auto nativeStatementTemp2078=offset;const auto nativeStatementTemp2079=JSValue(2);return binary("+",nativeStatementTemp2078,nativeStatementTemp2079);}());return nativeStatementTemp2076.get(nativeStatementTemp2077);}());setReference(nativeStatementTemp2071,nativeStatementTemp2072,nativeStatementTemp2073);return nativeStatementTemp2073;}());
([&]()->JSValue{auto nativeStatementTemp2080=nativeStatementTemp2053;const auto nativeStatementTemp2081=JSValue("w");const auto nativeStatementTemp2082=([&]()->JSValue{const auto nativeStatementTemp2085=array;const auto nativeStatementTemp2086=([&]()->JSValue{const auto nativeStatementTemp2087=offset;const auto nativeStatementTemp2088=JSValue(3);return binary("+",nativeStatementTemp2087,nativeStatementTemp2088);}());return nativeStatementTemp2085.get(nativeStatementTemp2086);}());setReference(nativeStatementTemp2080,nativeStatementTemp2081,nativeStatementTemp2082);return nativeStatementTemp2082;}());
return nativeStatementTemp2053;
return JSValue{};
}
JSValue method50(const JSValue& nativeStatementTemp2089,const Args& nativeStatementTemp2090,const Args& nativeStatementTemp2091){
auto nativeStatementTemp2092=nativeStatementTemp2091[0];auto nativeStatementTemp2093=nativeStatementTemp2089;
JSValue array=arg(nativeStatementTemp2090,0);
if(array.isUndefined())array=([&]()->JSValue{auto nativeStatementTemp2095=JSValue::array();return nativeStatementTemp2095;}());
JSValue offset=arg(nativeStatementTemp2090,1);
if(offset.isUndefined())offset=JSValue(0);
([&]()->JSValue{auto nativeStatementTemp2096=array;const auto nativeStatementTemp2097=offset;const auto nativeStatementTemp2098=([&]()->JSValue{const auto nativeStatementTemp2101=nativeStatementTemp2093;const auto nativeStatementTemp2102=JSValue("x");return nativeStatementTemp2101.get(nativeStatementTemp2102);}());setReference(nativeStatementTemp2096,nativeStatementTemp2097,nativeStatementTemp2098);return nativeStatementTemp2098;}());
([&]()->JSValue{auto nativeStatementTemp2103=array;const auto nativeStatementTemp2104=([&]()->JSValue{const auto nativeStatementTemp2105=offset;const auto nativeStatementTemp2106=JSValue(1);return binary("+",nativeStatementTemp2105,nativeStatementTemp2106);}());const auto nativeStatementTemp2107=([&]()->JSValue{const auto nativeStatementTemp2110=nativeStatementTemp2093;const auto nativeStatementTemp2111=JSValue("y");return nativeStatementTemp2110.get(nativeStatementTemp2111);}());setReference(nativeStatementTemp2103,nativeStatementTemp2104,nativeStatementTemp2107);return nativeStatementTemp2107;}());
([&]()->JSValue{auto nativeStatementTemp2112=array;const auto nativeStatementTemp2113=([&]()->JSValue{const auto nativeStatementTemp2114=offset;const auto nativeStatementTemp2115=JSValue(2);return binary("+",nativeStatementTemp2114,nativeStatementTemp2115);}());const auto nativeStatementTemp2116=([&]()->JSValue{const auto nativeStatementTemp2119=nativeStatementTemp2093;const auto nativeStatementTemp2120=JSValue("z");return nativeStatementTemp2119.get(nativeStatementTemp2120);}());setReference(nativeStatementTemp2112,nativeStatementTemp2113,nativeStatementTemp2116);return nativeStatementTemp2116;}());
([&]()->JSValue{auto nativeStatementTemp2121=array;const auto nativeStatementTemp2122=([&]()->JSValue{const auto nativeStatementTemp2123=offset;const auto nativeStatementTemp2124=JSValue(3);return binary("+",nativeStatementTemp2123,nativeStatementTemp2124);}());const auto nativeStatementTemp2125=([&]()->JSValue{const auto nativeStatementTemp2128=nativeStatementTemp2093;const auto nativeStatementTemp2129=JSValue("w");return nativeStatementTemp2128.get(nativeStatementTemp2129);}());setReference(nativeStatementTemp2121,nativeStatementTemp2122,nativeStatementTemp2125);return nativeStatementTemp2125;}());
return array;
return JSValue{};
}
JSValue method51(const JSValue& nativeStatementTemp2130,const Args& nativeStatementTemp2131,const Args& nativeStatementTemp2132){
auto nativeStatementTemp2133=nativeStatementTemp2132[0];auto nativeStatementTemp2134=nativeStatementTemp2130;
JSValue attribute=arg(nativeStatementTemp2131,0);
JSValue index=arg(nativeStatementTemp2131,1);
([&]()->JSValue{auto nativeStatementTemp2136=nativeStatementTemp2134;const auto nativeStatementTemp2137=JSValue("x");const auto nativeStatementTemp2138=([&]()->JSValue{auto nativeStatementTemp2144=attribute;const auto nativeStatementTemp2145=JSValue("getX");const auto nativeStatementTemp2141=nativeStatementTemp2144.get(nativeStatementTemp2145);const auto nativeStatementTemp2142=nativeStatementTemp2144;Args nativeStatementTemp2143;nativeStatementTemp2143.push_back(index);return nativeStatementTemp2141.call(nativeStatementTemp2142,nativeStatementTemp2143);}());setReference(nativeStatementTemp2136,nativeStatementTemp2137,nativeStatementTemp2138);return nativeStatementTemp2138;}());
([&]()->JSValue{auto nativeStatementTemp2146=nativeStatementTemp2134;const auto nativeStatementTemp2147=JSValue("y");const auto nativeStatementTemp2148=([&]()->JSValue{auto nativeStatementTemp2154=attribute;const auto nativeStatementTemp2155=JSValue("getY");const auto nativeStatementTemp2151=nativeStatementTemp2154.get(nativeStatementTemp2155);const auto nativeStatementTemp2152=nativeStatementTemp2154;Args nativeStatementTemp2153;nativeStatementTemp2153.push_back(index);return nativeStatementTemp2151.call(nativeStatementTemp2152,nativeStatementTemp2153);}());setReference(nativeStatementTemp2146,nativeStatementTemp2147,nativeStatementTemp2148);return nativeStatementTemp2148;}());
([&]()->JSValue{auto nativeStatementTemp2156=nativeStatementTemp2134;const auto nativeStatementTemp2157=JSValue("z");const auto nativeStatementTemp2158=([&]()->JSValue{auto nativeStatementTemp2164=attribute;const auto nativeStatementTemp2165=JSValue("getZ");const auto nativeStatementTemp2161=nativeStatementTemp2164.get(nativeStatementTemp2165);const auto nativeStatementTemp2162=nativeStatementTemp2164;Args nativeStatementTemp2163;nativeStatementTemp2163.push_back(index);return nativeStatementTemp2161.call(nativeStatementTemp2162,nativeStatementTemp2163);}());setReference(nativeStatementTemp2156,nativeStatementTemp2157,nativeStatementTemp2158);return nativeStatementTemp2158;}());
([&]()->JSValue{auto nativeStatementTemp2166=nativeStatementTemp2134;const auto nativeStatementTemp2167=JSValue("w");const auto nativeStatementTemp2168=([&]()->JSValue{auto nativeStatementTemp2174=attribute;const auto nativeStatementTemp2175=JSValue("getW");const auto nativeStatementTemp2171=nativeStatementTemp2174.get(nativeStatementTemp2175);const auto nativeStatementTemp2172=nativeStatementTemp2174;Args nativeStatementTemp2173;nativeStatementTemp2173.push_back(index);return nativeStatementTemp2171.call(nativeStatementTemp2172,nativeStatementTemp2173);}());setReference(nativeStatementTemp2166,nativeStatementTemp2167,nativeStatementTemp2168);return nativeStatementTemp2168;}());
return nativeStatementTemp2134;
return JSValue{};
}
JSValue method52(const JSValue& nativeStatementTemp2176,const Args& nativeStatementTemp2177,const Args& nativeStatementTemp2178){
auto nativeStatementTemp2179=nativeStatementTemp2178[0];auto nativeStatementTemp2180=nativeStatementTemp2176;
([&]()->JSValue{auto nativeStatementTemp2182=nativeStatementTemp2180;const auto nativeStatementTemp2183=JSValue("x");const auto nativeStatementTemp2184=([&]()->JSValue{auto nativeStatementTemp2190=nativeGlobalBinding("Math");const auto nativeStatementTemp2191=JSValue("random");const auto nativeStatementTemp2187=nativeStatementTemp2190.get(nativeStatementTemp2191);const auto nativeStatementTemp2188=nativeStatementTemp2190;Args nativeStatementTemp2189;return nativeStatementTemp2187.call(nativeStatementTemp2188,nativeStatementTemp2189);}());setReference(nativeStatementTemp2182,nativeStatementTemp2183,nativeStatementTemp2184);return nativeStatementTemp2184;}());
([&]()->JSValue{auto nativeStatementTemp2192=nativeStatementTemp2180;const auto nativeStatementTemp2193=JSValue("y");const auto nativeStatementTemp2194=([&]()->JSValue{auto nativeStatementTemp2200=nativeGlobalBinding("Math");const auto nativeStatementTemp2201=JSValue("random");const auto nativeStatementTemp2197=nativeStatementTemp2200.get(nativeStatementTemp2201);const auto nativeStatementTemp2198=nativeStatementTemp2200;Args nativeStatementTemp2199;return nativeStatementTemp2197.call(nativeStatementTemp2198,nativeStatementTemp2199);}());setReference(nativeStatementTemp2192,nativeStatementTemp2193,nativeStatementTemp2194);return nativeStatementTemp2194;}());
([&]()->JSValue{auto nativeStatementTemp2202=nativeStatementTemp2180;const auto nativeStatementTemp2203=JSValue("z");const auto nativeStatementTemp2204=([&]()->JSValue{auto nativeStatementTemp2210=nativeGlobalBinding("Math");const auto nativeStatementTemp2211=JSValue("random");const auto nativeStatementTemp2207=nativeStatementTemp2210.get(nativeStatementTemp2211);const auto nativeStatementTemp2208=nativeStatementTemp2210;Args nativeStatementTemp2209;return nativeStatementTemp2207.call(nativeStatementTemp2208,nativeStatementTemp2209);}());setReference(nativeStatementTemp2202,nativeStatementTemp2203,nativeStatementTemp2204);return nativeStatementTemp2204;}());
([&]()->JSValue{auto nativeStatementTemp2212=nativeStatementTemp2180;const auto nativeStatementTemp2213=JSValue("w");const auto nativeStatementTemp2214=([&]()->JSValue{auto nativeStatementTemp2220=nativeGlobalBinding("Math");const auto nativeStatementTemp2221=JSValue("random");const auto nativeStatementTemp2217=nativeStatementTemp2220.get(nativeStatementTemp2221);const auto nativeStatementTemp2218=nativeStatementTemp2220;Args nativeStatementTemp2219;return nativeStatementTemp2217.call(nativeStatementTemp2218,nativeStatementTemp2219);}());setReference(nativeStatementTemp2212,nativeStatementTemp2213,nativeStatementTemp2214);return nativeStatementTemp2214;}());
return nativeStatementTemp2180;
return JSValue{};
}
JSGeneratorStep iteratorBody(const JSValue& privateState,const JSGeneratorCompletion& completion,const JSValue& self,const Args&,const Args&){if(completion.kind==JSGeneratorCompletionKind::Return)return JSGeneratorStep::complete(completion.value);if(completion.kind==JSGeneratorCompletionKind::Throw)return JSGeneratorStep::abrupt(completion.value);auto state=privateState;const auto pc=state.get("pc");const auto index=pc.isUndefined()?0u:static_cast<unsigned>(pc.number());if(index>=4)return JSGeneratorStep::complete();state.set("pc",index+1);const char* fields[]={"x","y","z","w"};return JSGeneratorStep::yield(self.get(fields[index]));}
}
JSValue nativeVector4Module(){using namespace Vector4Managed;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject()){const auto status=existing->getInternalSlot("Vector4.status");if(status.strictEquals("evaluated"))return existing->getInternalSlot("Vector4.facade");if(status.strictEquals("failed"))throw JSThrownValue(existing->getInternalSlot("Vector4.error"));throw JSModuleInitializationError("Vector4 module");}auto module=JSValue::object(nullptr);module.retainForHeapLifetime();weak=JSWeakValue(module);lexical()=weak;module.setInternalSlot("Vector4.status","initializing");try{own(module,"clamp",nativeMathUtilsClamp());
auto ctor=JSValue::constructor(method0,{module},"Vector4",0);ctor.setFunctionSourceText(Vector4RuntimeSource::classSource);own(module,"Vector4",ctor);auto prototype=ctor.get("prototype");
{auto fn=JSValue::function(method1,{module},"get width",0);fn.setFunctionSourceText(Vector4RuntimeSource::source1);JSPropertyDescriptor d;d.getter=fn;d.configurable=true;prototype.defineProperty("width",d);}
{auto fn=JSValue::function(method2,{module},"set width",1);fn.setFunctionSourceText(Vector4RuntimeSource::source2);JSPropertyDescriptor d;d.setter=fn;d.configurable=true;prototype.defineProperty("width",d);}
{auto fn=JSValue::function(method3,{module},"get height",0);fn.setFunctionSourceText(Vector4RuntimeSource::source3);JSPropertyDescriptor d;d.getter=fn;d.configurable=true;prototype.defineProperty("height",d);}
{auto fn=JSValue::function(method4,{module},"set height",1);fn.setFunctionSourceText(Vector4RuntimeSource::source4);JSPropertyDescriptor d;d.setter=fn;d.configurable=true;prototype.defineProperty("height",d);}
{auto fn=JSValue::function(method5,{module},"set",4);fn.setFunctionSourceText(Vector4RuntimeSource::source5);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("set",d);}
{auto fn=JSValue::function(method6,{module},"setScalar",1);fn.setFunctionSourceText(Vector4RuntimeSource::source6);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setScalar",d);}
{auto fn=JSValue::function(method7,{module},"setX",1);fn.setFunctionSourceText(Vector4RuntimeSource::source7);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setX",d);}
{auto fn=JSValue::function(method8,{module},"setY",1);fn.setFunctionSourceText(Vector4RuntimeSource::source8);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setY",d);}
{auto fn=JSValue::function(method9,{module},"setZ",1);fn.setFunctionSourceText(Vector4RuntimeSource::source9);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setZ",d);}
{auto fn=JSValue::function(method10,{module},"setW",1);fn.setFunctionSourceText(Vector4RuntimeSource::source10);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setW",d);}
{auto fn=JSValue::function(method11,{module},"setComponent",2);fn.setFunctionSourceText(Vector4RuntimeSource::source11);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setComponent",d);}
{auto fn=JSValue::function(method12,{module},"getComponent",1);fn.setFunctionSourceText(Vector4RuntimeSource::source12);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("getComponent",d);}
{auto fn=JSValue::function(method13,{module},"clone",0);fn.setFunctionSourceText(Vector4RuntimeSource::source13);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("clone",d);}
{auto fn=JSValue::function(method14,{module},"copy",1);fn.setFunctionSourceText(Vector4RuntimeSource::source14);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(method15,{module},"add",1);fn.setFunctionSourceText(Vector4RuntimeSource::source15);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("add",d);}
{auto fn=JSValue::function(method16,{module},"addScalar",1);fn.setFunctionSourceText(Vector4RuntimeSource::source16);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("addScalar",d);}
{auto fn=JSValue::function(method17,{module},"addVectors",2);fn.setFunctionSourceText(Vector4RuntimeSource::source17);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("addVectors",d);}
{auto fn=JSValue::function(method18,{module},"addScaledVector",2);fn.setFunctionSourceText(Vector4RuntimeSource::source18);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("addScaledVector",d);}
{auto fn=JSValue::function(method19,{module},"sub",1);fn.setFunctionSourceText(Vector4RuntimeSource::source19);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("sub",d);}
{auto fn=JSValue::function(method20,{module},"subScalar",1);fn.setFunctionSourceText(Vector4RuntimeSource::source20);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("subScalar",d);}
{auto fn=JSValue::function(method21,{module},"subVectors",2);fn.setFunctionSourceText(Vector4RuntimeSource::source21);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("subVectors",d);}
{auto fn=JSValue::function(method22,{module},"multiply",1);fn.setFunctionSourceText(Vector4RuntimeSource::source22);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("multiply",d);}
{auto fn=JSValue::function(method23,{module},"multiplyScalar",1);fn.setFunctionSourceText(Vector4RuntimeSource::source23);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("multiplyScalar",d);}
{auto fn=JSValue::function(method24,{module},"applyMatrix4",1);fn.setFunctionSourceText(Vector4RuntimeSource::source24);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("applyMatrix4",d);}
{auto fn=JSValue::function(method25,{module},"divide",1);fn.setFunctionSourceText(Vector4RuntimeSource::source25);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("divide",d);}
{auto fn=JSValue::function(method26,{module},"divideScalar",1);fn.setFunctionSourceText(Vector4RuntimeSource::source26);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("divideScalar",d);}
{auto fn=JSValue::function(method27,{module},"setAxisAngleFromQuaternion",1);fn.setFunctionSourceText(Vector4RuntimeSource::source27);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setAxisAngleFromQuaternion",d);}
{auto fn=JSValue::function(method28,{module},"setAxisAngleFromRotationMatrix",1);fn.setFunctionSourceText(Vector4RuntimeSource::source28);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setAxisAngleFromRotationMatrix",d);}
{auto fn=JSValue::function(method29,{module},"setFromMatrixPosition",1);fn.setFunctionSourceText(Vector4RuntimeSource::source29);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setFromMatrixPosition",d);}
{auto fn=JSValue::function(method30,{module},"min",1);fn.setFunctionSourceText(Vector4RuntimeSource::source30);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("min",d);}
{auto fn=JSValue::function(method31,{module},"max",1);fn.setFunctionSourceText(Vector4RuntimeSource::source31);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("max",d);}
{auto fn=JSValue::function(method32,{module},"clamp",2);fn.setFunctionSourceText(Vector4RuntimeSource::source32);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("clamp",d);}
{auto fn=JSValue::function(method33,{module},"clampScalar",2);fn.setFunctionSourceText(Vector4RuntimeSource::source33);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("clampScalar",d);}
{auto fn=JSValue::function(method34,{module},"clampLength",2);fn.setFunctionSourceText(Vector4RuntimeSource::source34);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("clampLength",d);}
{auto fn=JSValue::function(method35,{module},"floor",0);fn.setFunctionSourceText(Vector4RuntimeSource::source35);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("floor",d);}
{auto fn=JSValue::function(method36,{module},"ceil",0);fn.setFunctionSourceText(Vector4RuntimeSource::source36);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("ceil",d);}
{auto fn=JSValue::function(method37,{module},"round",0);fn.setFunctionSourceText(Vector4RuntimeSource::source37);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("round",d);}
{auto fn=JSValue::function(method38,{module},"roundToZero",0);fn.setFunctionSourceText(Vector4RuntimeSource::source38);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("roundToZero",d);}
{auto fn=JSValue::function(method39,{module},"negate",0);fn.setFunctionSourceText(Vector4RuntimeSource::source39);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("negate",d);}
{auto fn=JSValue::function(method40,{module},"dot",1);fn.setFunctionSourceText(Vector4RuntimeSource::source40);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("dot",d);}
{auto fn=JSValue::function(method41,{module},"lengthSq",0);fn.setFunctionSourceText(Vector4RuntimeSource::source41);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("lengthSq",d);}
{auto fn=JSValue::function(method42,{module},"length",0);fn.setFunctionSourceText(Vector4RuntimeSource::source42);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("length",d);}
{auto fn=JSValue::function(method43,{module},"manhattanLength",0);fn.setFunctionSourceText(Vector4RuntimeSource::source43);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("manhattanLength",d);}
{auto fn=JSValue::function(method44,{module},"normalize",0);fn.setFunctionSourceText(Vector4RuntimeSource::source44);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("normalize",d);}
{auto fn=JSValue::function(method45,{module},"setLength",1);fn.setFunctionSourceText(Vector4RuntimeSource::source45);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("setLength",d);}
{auto fn=JSValue::function(method46,{module},"lerp",2);fn.setFunctionSourceText(Vector4RuntimeSource::source46);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("lerp",d);}
{auto fn=JSValue::function(method47,{module},"lerpVectors",3);fn.setFunctionSourceText(Vector4RuntimeSource::source47);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("lerpVectors",d);}
{auto fn=JSValue::function(method48,{module},"equals",1);fn.setFunctionSourceText(Vector4RuntimeSource::source48);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("equals",d);}
{auto fn=JSValue::function(method49,{module},"fromArray",1);fn.setFunctionSourceText(Vector4RuntimeSource::source49);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("fromArray",d);}
{auto fn=JSValue::function(method50,{module},"toArray",0);fn.setFunctionSourceText(Vector4RuntimeSource::source50);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("toArray",d);}
{auto fn=JSValue::function(method51,{module},"fromBufferAttribute",2);fn.setFunctionSourceText(Vector4RuntimeSource::source51);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("fromBufferAttribute",d);}
{auto fn=JSValue::function(method52,{module},"random",0);fn.setFunctionSourceText(Vector4RuntimeSource::source52);JSPropertyDescriptor d;d.value=fn;d.writable=true;d.configurable=true;prototype.defineProperty("random",d);}
{auto fn=createNativeGeneratorFunction(iteratorBody,{module},"[Symbol.iterator]",0,Vector4RuntimeSource::source53);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty(JSValue::wellKnownSymbol("iterator"),d);}
([&]()->JSValue{auto nativeStatementTemp2222=([&]()->JSValue{const auto nativeStatementTemp2224=ctor;const auto nativeStatementTemp2225=JSValue("prototype");return nativeStatementTemp2224.get(nativeStatementTemp2225);}());const auto nativeStatementTemp2223=JSValue("isVector4");const auto nativeStatementTemp2226=JSValue(true);setReference(nativeStatementTemp2222,nativeStatementTemp2223,nativeStatementTemp2226);return nativeStatementTemp2226;}());
auto facade=JSValue::object();own(facade,"Vector4",ctor);module.setInternalSlot("Vector4.facade",facade);module.setInternalSlot("Vector4.status","evaluated");return facade;}catch(...){module.setInternalSlot("Vector4.error",nativeExceptionValue(std::current_exception()));module.setInternalSlot("Vector4.status","failed");throw JSThrownValue(module.getInternalSlot("Vector4.error"));}}
JSValue nativeVector4Binding(const std::string& name){const auto keeper=nativeVector4Module();return Vector4Managed::lexical().lock()->get(name);}
JSValue nativeVector4Constructor(){return nativeVector4Binding("Vector4");}
