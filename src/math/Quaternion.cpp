#include "math/JSNumberMath.h"
#include "math/Quaternion.h"
#include "core/BufferAttribute.h"
#include "math/Vector3.h"
#include "math/Euler.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include "math/MathUtils.h"

Quaternion::Quaternion(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
    : m_x(x), m_y(y), m_z(z), m_w(w)
{
}

Quaternion::Quaternion(const Quaternion &q)
    : Quaternion(q.x, q.y, q.z, q.w)
{
}

Quaternion::~Quaternion()
{
}

Quaternion &Quaternion::set(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z, HIGH_PRECISION w)
{
    m_x = x;
    m_y = y;
    m_z = z;
    m_w = w;
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::setScalar(HIGH_PRECISION s)
{
    m_x = m_y = m_z = m_w = s;
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::setX(HIGH_PRECISION x) { m_x = x; notifyPropertyChange(m_onChangeCallback);
    return *this; }
Quaternion &Quaternion::setY(HIGH_PRECISION y) { m_y = y; notifyPropertyChange(m_onChangeCallback);
    return *this; }
Quaternion &Quaternion::setZ(HIGH_PRECISION z) { m_z = z; notifyPropertyChange(m_onChangeCallback);
    return *this; }
Quaternion &Quaternion::setW(HIGH_PRECISION w) { m_w = w; notifyPropertyChange(m_onChangeCallback);
    return *this; }

Quaternion Quaternion::clone() const
{
    return Quaternion(m_x, m_y, m_z, m_w);
}

Quaternion &Quaternion::copy(const Quaternion &q)
{
    m_x = q.x; m_y = q.y; m_z = q.z; m_w = q.w;
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::setFromEuler(const Euler &euler, bool update)
{
    HIGH_PRECISION x = euler.x;
    HIGH_PRECISION y = euler.y;
    HIGH_PRECISION z = euler.z;
    EulerOrder order = euler.order;

    auto c1 = JSNumberMath::cos(x / 2.0f);
    auto c2 = JSNumberMath::cos(y / 2.0f);
    auto c3 = JSNumberMath::cos(z / 2.0f);

    auto s1 = JSNumberMath::sin(x / 2.0f);
    auto s2 = JSNumberMath::sin(y / 2.0f);
    auto s3 = JSNumberMath::sin(z / 2.0f);

    if (order == EulerOrder::XYZ)
    {
        m_x = s1 * c2 * c3 + c1 * s2 * s3;
        m_y = c1 * s2 * c3 - s1 * c2 * s3;
        m_z = c1 * c2 * s3 + s1 * s2 * c3;
        m_w = c1 * c2 * c3 - s1 * s2 * s3;
    }
    else if (order == EulerOrder::YXZ)
    {
        m_x = s1 * c2 * c3 + c1 * s2 * s3;
        m_y = c1 * s2 * c3 - s1 * c2 * s3;
        m_z = c1 * c2 * s3 - s1 * s2 * c3;
        m_w = c1 * c2 * c3 + s1 * s2 * s3;
    }
    else if (order == EulerOrder::ZXY)
    {
        m_x = s1 * c2 * c3 - c1 * s2 * s3;
        m_y = c1 * s2 * c3 + s1 * c2 * s3;
        m_z = c1 * c2 * s3 + s1 * s2 * c3;
        m_w = c1 * c2 * c3 - s1 * s2 * s3;
    }
    else if (order == EulerOrder::ZYX)
    {
        m_x = s1 * c2 * c3 - c1 * s2 * s3;
        m_y = c1 * s2 * c3 + s1 * c2 * s3;
        m_z = c1 * c2 * s3 - s1 * s2 * c3;
        m_w = c1 * c2 * c3 + s1 * s2 * s3;
    }
    else if (order == EulerOrder::YZX)
    {
        m_x = s1 * c2 * c3 + c1 * s2 * s3;
        m_y = c1 * s2 * c3 + s1 * c2 * s3;
        m_z = c1 * c2 * s3 - s1 * s2 * c3;
        m_w = c1 * c2 * c3 - s1 * s2 * s3;
    }
    else if (order == EulerOrder::XZY)
    {
        m_x = s1 * c2 * c3 - c1 * s2 * s3;
        m_y = c1 * s2 * c3 - s1 * c2 * s3;
        m_z = c1 * c2 * s3 + s1 * s2 * c3;
        m_w = c1 * c2 * c3 + s1 * s2 * s3;
    }

    if (update) notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::setFromAxisAngle(const Vector3 &axis, HIGH_PRECISION angle)
{
    const auto halfAngle = angle / 2;
    const auto s = JSNumberMath::sin(halfAngle);
    m_x = axis.x * s; m_y = axis.y * s; m_z = axis.z * s; m_w = JSNumberMath::cos(halfAngle);
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::setFromRotationMatrix(const Matrix4 &m)
{
    auto e = m.elements();

    HIGH_PRECISION trace = e[0] + e[5] + e[10];
    HIGH_PRECISION s;

    if (trace > 0.0f)
    {
        s = 0.5f / std::sqrt(trace + 1.0f);
        m_w = 0.25f / s;
        m_x = (e[6] - e[9]) * s;
        m_y = (e[8] - e[2]) * s;
        m_z = (e[1] - e[4]) * s;
    }
    else if (e[0] > e[5] && e[0] > e[10])
    {
        s = 2.0f * std::sqrt(1.0f + e[0] - e[5] - e[10]);
        m_w = (e[6] - e[9]) / s;
        m_x = 0.25f * s;
        m_y = (e[1] + e[4]) / s;
        m_z = (e[8] + e[2]) / s;
    }
    else if (e[5] > e[10])
    {
        s = 2.0f * std::sqrt(1.0f + e[5] - e[0] - e[10]);
        m_w = (e[8] - e[2]) / s;
        m_x = (e[1] + e[4]) / s;
        m_y = 0.25f * s;
        m_z = (e[6] + e[9]) / s;
    }
    else
    {
        s = 2.0f * std::sqrt(1.0f + e[10] - e[0] - e[5]);
        m_w = (e[1] - e[4]) / s;
        m_x = (e[8] + e[2]) / s;
        m_y = (e[6] + e[9]) / s;
        m_z = 0.25f * s;
    }

    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::setFromUnitVectors(const Vector3 &v1, const Vector3 &v2)
{
    auto r = v1.dot(v2) + 1;
    if (r < 1e-8) {
        r = 0;
        if (std::abs(v1.x) > std::abs(v1.z)) { m_x = -v1.y; m_y = v1.x; m_z = 0; m_w = r; }
        else { m_x = 0; m_y = -v1.z; m_z = v1.y; m_w = r; }
    } else {
        m_x = v1.y * v2.z - v1.z * v2.y;
        m_y = v1.z * v2.x - v1.x * v2.z;
        m_z = v1.x * v2.y - v1.y * v2.x;
        m_w = r;
    }
    return normalize();
}

bool Quaternion::isQuaternionEqual(const Quaternion &q, HIGH_PRECISION epsilon) const
{
    return equals(q, epsilon);
}

Quaternion &Quaternion::normalize()
{
    auto len = length();
    if (len == 0) { m_x = 0; m_y = 0; m_z = 0; m_w = 1; }
    else { len = 1 / len; m_x *= len; m_y *= len; m_z *= len; m_w *= len; }
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::conjugate()
{
    m_x = -m_x;
    m_y = -m_y;
    m_z = -m_z;
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::invert()
{
    return conjugate();
}

Quaternion &Quaternion::add(const Quaternion &q)
{
    m_x += q.x;
    m_y += q.y;
    m_z += q.z;
    m_w += q.w;
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::sub(const Quaternion &q)
{
    m_x -= q.x;
    m_y -= q.y;
    m_z -= q.z;
    m_w -= q.w;
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

HIGH_PRECISION Quaternion::dot(const Quaternion &q) const
{
    return m_x * q.x + m_y * q.y + m_z * q.z + m_w * q.w;
}

HIGH_PRECISION Quaternion::lengthSq() const
{
    return m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w;
}

HIGH_PRECISION Quaternion::length() const
{
    return std::sqrt(lengthSq());
}

Quaternion &Quaternion::multiply(const Quaternion &q)
{
    multiplyQuaternions(*this, q);
    return *this;
}

Quaternion &Quaternion::premultiply(const Quaternion &q)
{
    multiplyQuaternions(q, *this);
    return *this;
}

Quaternion &Quaternion::multiplyQuaternions(const Quaternion &a, const Quaternion &b)
{
    HIGH_PRECISION qax = a.x, qay = a.y, qaz = a.z, qaw = a.w;
    HIGH_PRECISION qbx = b.x, qby = b.y, qbz = b.z, qbw = b.w;

    m_x = qax * qbw + qaw * qbx + qay * qbz - qaz * qby;
    m_y = qay * qbw + qaw * qby + qaz * qbx - qax * qbz;
    m_z = qaz * qbw + qaw * qbz + qax * qby - qay * qbx;
    m_w = qaw * qbw - qax * qbx - qay * qby - qaz * qbz;

    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::slerp(const Quaternion &qb, HIGH_PRECISION t)
{
    HIGH_PRECISION x = qb.x, y = qb.y, z = qb.z, w = qb.w;
    auto dotValue = dot(qb);
    if (dotValue < 0) { x = -x; y = -y; z = -z; w = -w; dotValue = -dotValue; }
    auto s = 1 - t;
    if (dotValue < 0.9995) {
        const auto theta = JSNumberMath::acos(dotValue);
        const auto sinTheta = JSNumberMath::sin(theta);
        s = JSNumberMath::sin(s * theta) / sinTheta;
        t = JSNumberMath::sin(t * theta) / sinTheta;
        m_x = m_x * s + x * t; m_y = m_y * s + y * t; m_z = m_z * s + z * t; m_w = m_w * s + w * t;
        notifyPropertyChange(m_onChangeCallback);
    } else {
        m_x = m_x * s + x * t; m_y = m_y * s + y * t; m_z = m_z * s + z * t; m_w = m_w * s + w * t;
        normalize();
    }
    return *this;
}

Quaternion &Quaternion::slerpQuaternions(const Quaternion &qa, const Quaternion &qb, HIGH_PRECISION t)
{
    copy(qa);
    return slerp(qb, t);
}

bool Quaternion::equals(const Quaternion &q, HIGH_PRECISION epsilon) const
{
    if (epsilon == 0) return m_x == q.x && m_y == q.y && m_z == q.z && m_w == q.w;
    return std::abs(m_x - q.x) <= epsilon && std::abs(m_y - q.y) <= epsilon && std::abs(m_z - q.z) <= epsilon && std::abs(m_w - q.w) <= epsilon;
}

Quaternion &Quaternion::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    m_x = array[offset];
    m_y = array[offset + 1];
    m_z = array[offset + 2];
    m_w = array[offset + 3];
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

std::vector<HIGH_PRECISION> &Quaternion::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 4) array.resize(offset + 4);
    array[offset] = m_x;
    array[offset + 1] = m_y;
    array[offset + 2] = m_z;
    array[offset + 3] = m_w;
    return array;
}

Vector3 &Quaternion::applyToVector3(Vector3 &v) const
{
    return v.applyQuaternion(*this);
}

void Quaternion::toEuler(Euler &euler) const
{
    euler.setFromQuaternion(*this);
}

void Quaternion::toRotationMatrix(Matrix4 &m) const
{
    m.makeRotationFromQuaternion(*this);
}

// Operators
Quaternion Quaternion::operator+(const Quaternion &q) const { return clone().add(q); }
Quaternion Quaternion::operator-(const Quaternion &q) const { return clone().sub(q); }
Quaternion Quaternion::operator*(const Quaternion &q) const { return clone().multiply(q); }
Quaternion Quaternion::operator*(HIGH_PRECISION s) const { return clone().multiplyScalar(s); }
Quaternion Quaternion::operator/(HIGH_PRECISION s) const { return clone().divideScalar(s); }

Quaternion &Quaternion::operator=(const Quaternion &q) {
    return copy(q);
}
bool Quaternion::operator==(const Quaternion &q) const { return equals(q); }

Quaternion &Quaternion::lerp(const Quaternion &qb, HIGH_PRECISION t)
{
    if (t <= 0.0f) return *this;
    if (t >= 1.0f) { copy(qb); return *this; }

    m_x += (qb.x - m_x) * t;
    m_y += (qb.y - m_y) * t;
    m_z += (qb.z - m_z) * t;
    m_w += (qb.w - m_w) * t;

    return normalize();
}

Quaternion &Quaternion::multiplyScalar(HIGH_PRECISION s)
{
    m_x *= s;
    m_y *= s;
    m_z *= s;
    m_w *= s;
    notifyPropertyChange(m_onChangeCallback);
    return *this;
}

Quaternion &Quaternion::divideScalar(HIGH_PRECISION s)
{
    return multiplyScalar(1.0f / s);
}

Quaternion &Quaternion::_onChange(std::function<void()> callback)
{
    registerPropertyChangeCallback(m_onChangeCallback,std::move(callback));
    return *this;
}
HIGH_PRECISION Quaternion::angleTo(const Quaternion &q) const
{
    return 2 * JSNumberMath::acos(std::abs(MathUtils::clamp(dot(q), -1, 1)));
}
Quaternion &Quaternion::rotateTowards(const Quaternion &q, HIGH_PRECISION step)
{
    const auto angle = angleTo(q);
    if (angle == 0) return *this;
    return slerp(q, std::min(1.0, step / angle));
}
Quaternion &Quaternion::identity()
{
    return set(0, 0, 0, 1);
}
Quaternion &Quaternion::random()
{
    const auto theta1 = 2 * MATH_PI * DIST_DOUBLE(RANDOM_ENGINE);
    const auto theta2 = 2 * MATH_PI * DIST_DOUBLE(RANDOM_ENGINE);
    const auto x0 = DIST_DOUBLE(RANDOM_ENGINE);
    const auto r1 = std::sqrt(1 - x0), r2 = std::sqrt(x0);
    return set(r1 * JSNumberMath::sin(theta1), r1 * JSNumberMath::cos(theta1), r2 * JSNumberMath::sin(theta2), r2 * JSNumberMath::cos(theta2));
}
std::vector<HIGH_PRECISION> Quaternion::toArray() const
{
    std::vector<HIGH_PRECISION> array;
    toArray(array);
    return array;
}

Quaternion &Quaternion::fromBufferAttribute(const BufferAttribute &attribute, size_t index)
{
    return set(attribute.getX(index), attribute.getY(index), attribute.getZ(index), attribute.getW(index));
}

void Quaternion::slerpFlat(std::vector<HIGH_PRECISION> &dst, size_t dstOffset, const std::vector<HIGH_PRECISION> &src0, size_t srcOffset0, const std::vector<HIGH_PRECISION> &src1, size_t srcOffset1, HIGH_PRECISION t)
{


		auto x0 = src0[ srcOffset0 + 0 ],
			y0 = src0[ srcOffset0 + 1 ],
			z0 = src0[ srcOffset0 + 2 ],
			w0 = src0[ srcOffset0 + 3 ];

		auto x1 = src1[ srcOffset1 + 0 ],
			y1 = src1[ srcOffset1 + 1 ],
			z1 = src1[ srcOffset1 + 2 ],
			w1 = src1[ srcOffset1 + 3 ];

		if ( w0 != w1 || x0 != x1 || y0 != y1 || z0 != z1 ) {

			auto dot = x0 * x1 + y0 * y1 + z0 * z1 + w0 * w1;

			if ( dot < 0 ) {

				x1 = - x1;
				y1 = - y1;
				z1 = - z1;
				w1 = - w1;

				dot = - dot;

			}

			auto s = 1 - t;

			if ( dot < 0.9995 ) {

				// slerp

				const auto theta = JSNumberMath::acos( dot );
				const auto sin = JSNumberMath::sin( theta );

				s = JSNumberMath::sin( s * theta ) / sin;
				t = JSNumberMath::sin( t * theta ) / sin;

				x0 = x0 * s + x1 * t;
				y0 = y0 * s + y1 * t;
				z0 = z0 * s + z1 * t;
				w0 = w0 * s + w1 * t;

			} else {

				// for small angles, lerp then normalize

				x0 = x0 * s + x1 * t;
				y0 = y0 * s + y1 * t;
				z0 = z0 * s + z1 * t;
				w0 = w0 * s + w1 * t;

				const auto f = 1 / std::sqrt( x0 * x0 + y0 * y0 + z0 * z0 + w0 * w0 );

				x0 *= f;
				y0 *= f;
				z0 *= f;
				w0 *= f;

			}

		}

		if (dst.size() < dstOffset + 4) dst.resize(dstOffset + 4);
        dst[ dstOffset ] = x0;
		dst[ dstOffset + 1 ] = y0;
		dst[ dstOffset + 2 ] = z0;
		dst[ dstOffset + 3 ] = w0;


}

std::vector<HIGH_PRECISION> & Quaternion::multiplyQuaternionsFlat(std::vector<HIGH_PRECISION> &dst, size_t dstOffset, const std::vector<HIGH_PRECISION> &src0, size_t srcOffset0, const std::vector<HIGH_PRECISION> &src1, size_t srcOffset1)
{


		const auto x0 = src0[ srcOffset0 ];
		const auto y0 = src0[ srcOffset0 + 1 ];
		const auto z0 = src0[ srcOffset0 + 2 ];
		const auto w0 = src0[ srcOffset0 + 3 ];

		const auto x1 = src1[ srcOffset1 ];
		const auto y1 = src1[ srcOffset1 + 1 ];
		const auto z1 = src1[ srcOffset1 + 2 ];
		const auto w1 = src1[ srcOffset1 + 3 ];

		if (dst.size() < dstOffset + 4) dst.resize(dstOffset + 4);
        dst[ dstOffset ] = x0 * w1 + w0 * x1 + y0 * z1 - z0 * y1;
		dst[ dstOffset + 1 ] = y0 * w1 + w0 * y1 + z0 * x1 - x0 * z1;
		dst[ dstOffset + 2 ] = z0 * w1 + w0 * z1 + x0 * y1 - y0 * x1;
		dst[ dstOffset + 3 ] = w0 * w1 - x0 * x1 - y0 * y1 - z0 * z1;

		return dst;


}

std::vector<HIGH_PRECISION> Quaternion::toJSON() const { return toArray(); }

Quaternion::Quaternion(Quaternion&& source) : Quaternion(static_cast<const Quaternion&>(source)) {}
Quaternion& Quaternion::operator=(Quaternion&& source) { return copy(source); }

// Native statement implementation of pinned Quaternion.js; see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSGenerator.h"
#include "core/JSStringEncoding.h"
#include "math/QuaternionRuntimeSource.inc"
#include "utils.h"
#include <string_view>
namespace QuaternionManaged {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const char* name,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(name,d);}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive(),b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString(),y=b.toString();return x+y;}const auto x=a.toNumber(),y=b.toNumber();return x+y;}
 if(op=="<"||op==">"){const auto a=left.toPrimitive(JSValue::PreferredType::Number),b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());return op=="<"?x<y:x>y;}const auto x=a.toNumber(),y=b.toNumber();return op=="<"?x<y:x>y;}
 const auto a=left.toNumber(),b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;throw std::logic_error("Unknown Quaternion arithmetic");
}
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue method0(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue x=arg(args,0);
if(x.isUndefined())x=JSValue(0);
JSValue y=arg(args,1);
if(y.isUndefined())y=JSValue(0);
JSValue z=arg(args,2);
if(z.isUndefined())z=JSValue(0);
JSValue w=arg(args,3);
if(w.isUndefined())w=JSValue(1);
([&]()->JSValue{auto qv0=self;const auto qv1=JSValue("isQuaternion");const auto qv2=JSValue(true);qv0.set(qv1,qv2);return qv2;}());
([&]()->JSValue{auto qv3=self;const auto qv4=JSValue("_x");const auto qv5=x;qv3.set(qv4,qv5);return qv5;}());
([&]()->JSValue{auto qv6=self;const auto qv7=JSValue("_y");const auto qv8=y;qv6.set(qv7,qv8);return qv8;}());
([&]()->JSValue{auto qv9=self;const auto qv10=JSValue("_z");const auto qv11=z;qv9.set(qv10,qv11);return qv11;}());
([&]()->JSValue{auto qv12=self;const auto qv13=JSValue("_w");const auto qv14=w;qv12.set(qv13,qv14);return qv14;}());
return {};
}
JSValue method1(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue dst=arg(args,0);
JSValue dstOffset=arg(args,1);
JSValue src0=arg(args,2);
JSValue srcOffset0=arg(args,3);
JSValue src1=arg(args,4);
JSValue srcOffset1=arg(args,5);
JSValue t=arg(args,6);
JSValue x0=([&]()->JSValue{const auto object=src0;const auto key=([&]()->JSValue{const auto left=srcOffset0;const auto right=JSValue(0);return binary("+",left,right);}());return object.get(key);}());
JSValue y0=([&]()->JSValue{const auto object=src0;const auto key=([&]()->JSValue{const auto left=srcOffset0;const auto right=JSValue(1);return binary("+",left,right);}());return object.get(key);}());
JSValue z0=([&]()->JSValue{const auto object=src0;const auto key=([&]()->JSValue{const auto left=srcOffset0;const auto right=JSValue(2);return binary("+",left,right);}());return object.get(key);}());
JSValue w0=([&]()->JSValue{const auto object=src0;const auto key=([&]()->JSValue{const auto left=srcOffset0;const auto right=JSValue(3);return binary("+",left,right);}());return object.get(key);}());
JSValue x1=([&]()->JSValue{const auto object=src1;const auto key=([&]()->JSValue{const auto left=srcOffset1;const auto right=JSValue(0);return binary("+",left,right);}());return object.get(key);}());
JSValue y1=([&]()->JSValue{const auto object=src1;const auto key=([&]()->JSValue{const auto left=srcOffset1;const auto right=JSValue(1);return binary("+",left,right);}());return object.get(key);}());
JSValue z1=([&]()->JSValue{const auto object=src1;const auto key=([&]()->JSValue{const auto left=srcOffset1;const auto right=JSValue(2);return binary("+",left,right);}());return object.get(key);}());
JSValue w1=([&]()->JSValue{const auto object=src1;const auto key=([&]()->JSValue{const auto left=srcOffset1;const auto right=JSValue(3);return binary("+",left,right);}());return object.get(key);}());
if(([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=w0;const auto right=w1;return binary("!==",left,right);}());if(left.toBoolean())return left;return ([&]()->JSValue{const auto left=x0;const auto right=x1;return binary("!==",left,right);}());}());if(left.toBoolean())return left;return ([&]()->JSValue{const auto left=y0;const auto right=y1;return binary("!==",left,right);}());}());if(left.toBoolean())return left;return ([&]()->JSValue{const auto left=z0;const auto right=z1;return binary("!==",left,right);}());}()).toBoolean()){
JSValue dot=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=x0;const auto right=x1;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=y0;const auto right=y1;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=z0;const auto right=z1;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=w0;const auto right=w1;return binary("*",left,right);}());return binary("+",left,right);}());
if(([&]()->JSValue{const auto left=dot;const auto right=JSValue(0);return binary("<",left,right);}()).toBoolean()){
([&]()->JSValue{const auto qv15=([&]()->JSValue{const auto value=x1;return -value.toNumber();}());x1=qv15;return qv15;}());
([&]()->JSValue{const auto qv16=([&]()->JSValue{const auto value=y1;return -value.toNumber();}());y1=qv16;return qv16;}());
([&]()->JSValue{const auto qv17=([&]()->JSValue{const auto value=z1;return -value.toNumber();}());z1=qv17;return qv17;}());
([&]()->JSValue{const auto qv18=([&]()->JSValue{const auto value=w1;return -value.toNumber();}());w1=qv18;return qv18;}());
([&]()->JSValue{const auto qv19=([&]()->JSValue{const auto value=dot;return -value.toNumber();}());dot=qv19;return qv19;}());

}
JSValue s=([&]()->JSValue{const auto left=JSValue(1);const auto right=t;return binary("-",left,right);}());
if(([&]()->JSValue{const auto left=dot;const auto right=JSValue(0.9995);return binary("<",left,right);}()).toBoolean()){
JSValue theta=([&]()->JSValue{auto qv20=nativeGlobalBinding("Math");const auto qv21=JSValue("acos");const auto callee=qv20.get(qv21);const auto receiver=qv20;Args args;args.push_back(dot);return callee.call(receiver,args);}());
JSValue sin=([&]()->JSValue{auto qv22=nativeGlobalBinding("Math");const auto qv23=JSValue("sin");const auto callee=qv22.get(qv23);const auto receiver=qv22;Args args;args.push_back(theta);return callee.call(receiver,args);}());
([&]()->JSValue{const auto qv24=([&]()->JSValue{const auto left=([&]()->JSValue{auto qv25=nativeGlobalBinding("Math");const auto qv26=JSValue("sin");const auto callee=qv25.get(qv26);const auto receiver=qv25;Args args;args.push_back(([&]()->JSValue{const auto left=s;const auto right=theta;return binary("*",left,right);}()));return callee.call(receiver,args);}());const auto right=sin;return binary("/",left,right);}());s=qv24;return qv24;}());
([&]()->JSValue{const auto qv27=([&]()->JSValue{const auto left=([&]()->JSValue{auto qv28=nativeGlobalBinding("Math");const auto qv29=JSValue("sin");const auto callee=qv28.get(qv29);const auto receiver=qv28;Args args;args.push_back(([&]()->JSValue{const auto left=t;const auto right=theta;return binary("*",left,right);}()));return callee.call(receiver,args);}());const auto right=sin;return binary("/",left,right);}());t=qv27;return qv27;}());
([&]()->JSValue{const auto qv30=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=x0;const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=x1;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());x0=qv30;return qv30;}());
([&]()->JSValue{const auto qv31=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=y0;const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=y1;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());y0=qv31;return qv31;}());
([&]()->JSValue{const auto qv32=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=z0;const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=z1;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());z0=qv32;return qv32;}());
([&]()->JSValue{const auto qv33=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=w0;const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=w1;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());w0=qv33;return qv33;}());

}
else {
([&]()->JSValue{const auto qv34=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=x0;const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=x1;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());x0=qv34;return qv34;}());
([&]()->JSValue{const auto qv35=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=y0;const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=y1;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());y0=qv35;return qv35;}());
([&]()->JSValue{const auto qv36=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=z0;const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=z1;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());z0=qv36;return qv36;}());
([&]()->JSValue{const auto qv37=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=w0;const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=w1;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());w0=qv37;return qv37;}());
JSValue f=([&]()->JSValue{const auto left=JSValue(1);const auto right=([&]()->JSValue{auto qv38=nativeGlobalBinding("Math");const auto qv39=JSValue("sqrt");const auto callee=qv38.get(qv39);const auto receiver=qv38;Args args;args.push_back(([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=x0;const auto right=x0;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=y0;const auto right=y0;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=z0;const auto right=z0;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=w0;const auto right=w0;return binary("*",left,right);}());return binary("+",left,right);}()));return callee.call(receiver,args);}());return binary("/",left,right);}());
([&]()->JSValue{const auto old=x0;const auto right=f;const auto qv40=binary("*",old,right);x0=qv40;return qv40;}());
([&]()->JSValue{const auto old=y0;const auto right=f;const auto qv41=binary("*",old,right);y0=qv41;return qv41;}());
([&]()->JSValue{const auto old=z0;const auto right=f;const auto qv42=binary("*",old,right);z0=qv42;return qv42;}());
([&]()->JSValue{const auto old=w0;const auto right=f;const auto qv43=binary("*",old,right);w0=qv43;return qv43;}());

}

}
([&]()->JSValue{auto qv44=dst;const auto qv45=dstOffset;const auto qv46=x0;qv44.set(qv45,qv46);return qv46;}());
([&]()->JSValue{auto qv47=dst;const auto qv48=([&]()->JSValue{const auto left=dstOffset;const auto right=JSValue(1);return binary("+",left,right);}());const auto qv49=y0;qv47.set(qv48,qv49);return qv49;}());
([&]()->JSValue{auto qv50=dst;const auto qv51=([&]()->JSValue{const auto left=dstOffset;const auto right=JSValue(2);return binary("+",left,right);}());const auto qv52=z0;qv50.set(qv51,qv52);return qv52;}());
([&]()->JSValue{auto qv53=dst;const auto qv54=([&]()->JSValue{const auto left=dstOffset;const auto right=JSValue(3);return binary("+",left,right);}());const auto qv55=w0;qv53.set(qv54,qv55);return qv55;}());
return {};
}
JSValue method2(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue dst=arg(args,0);
JSValue dstOffset=arg(args,1);
JSValue src0=arg(args,2);
JSValue srcOffset0=arg(args,3);
JSValue src1=arg(args,4);
JSValue srcOffset1=arg(args,5);
JSValue x0=([&]()->JSValue{const auto object=src0;const auto key=srcOffset0;return object.get(key);}());
JSValue y0=([&]()->JSValue{const auto object=src0;const auto key=([&]()->JSValue{const auto left=srcOffset0;const auto right=JSValue(1);return binary("+",left,right);}());return object.get(key);}());
JSValue z0=([&]()->JSValue{const auto object=src0;const auto key=([&]()->JSValue{const auto left=srcOffset0;const auto right=JSValue(2);return binary("+",left,right);}());return object.get(key);}());
JSValue w0=([&]()->JSValue{const auto object=src0;const auto key=([&]()->JSValue{const auto left=srcOffset0;const auto right=JSValue(3);return binary("+",left,right);}());return object.get(key);}());
JSValue x1=([&]()->JSValue{const auto object=src1;const auto key=srcOffset1;return object.get(key);}());
JSValue y1=([&]()->JSValue{const auto object=src1;const auto key=([&]()->JSValue{const auto left=srcOffset1;const auto right=JSValue(1);return binary("+",left,right);}());return object.get(key);}());
JSValue z1=([&]()->JSValue{const auto object=src1;const auto key=([&]()->JSValue{const auto left=srcOffset1;const auto right=JSValue(2);return binary("+",left,right);}());return object.get(key);}());
JSValue w1=([&]()->JSValue{const auto object=src1;const auto key=([&]()->JSValue{const auto left=srcOffset1;const auto right=JSValue(3);return binary("+",left,right);}());return object.get(key);}());
([&]()->JSValue{auto qv56=dst;const auto qv57=dstOffset;const auto qv58=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=x0;const auto right=w1;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=w0;const auto right=x1;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=y0;const auto right=z1;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=z0;const auto right=y1;return binary("*",left,right);}());return binary("-",left,right);}());qv56.set(qv57,qv58);return qv58;}());
([&]()->JSValue{auto qv59=dst;const auto qv60=([&]()->JSValue{const auto left=dstOffset;const auto right=JSValue(1);return binary("+",left,right);}());const auto qv61=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=y0;const auto right=w1;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=w0;const auto right=y1;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=z0;const auto right=x1;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=x0;const auto right=z1;return binary("*",left,right);}());return binary("-",left,right);}());qv59.set(qv60,qv61);return qv61;}());
([&]()->JSValue{auto qv62=dst;const auto qv63=([&]()->JSValue{const auto left=dstOffset;const auto right=JSValue(2);return binary("+",left,right);}());const auto qv64=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=z0;const auto right=w1;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=w0;const auto right=z1;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=x0;const auto right=y1;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=y0;const auto right=x1;return binary("*",left,right);}());return binary("-",left,right);}());qv62.set(qv63,qv64);return qv64;}());
([&]()->JSValue{auto qv65=dst;const auto qv66=([&]()->JSValue{const auto left=dstOffset;const auto right=JSValue(3);return binary("+",left,right);}());const auto qv67=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=w0;const auto right=w1;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=x0;const auto right=x1;return binary("*",left,right);}());return binary("-",left,right);}());const auto right=([&]()->JSValue{const auto left=y0;const auto right=y1;return binary("*",left,right);}());return binary("-",left,right);}());const auto right=([&]()->JSValue{const auto left=z0;const auto right=z1;return binary("*",left,right);}());return binary("-",left,right);}());qv65.set(qv66,qv67);return qv67;}());
return dst;
return {};
}
JSValue method3(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());
return {};
}
JSValue method4(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue value=arg(args,0);
([&]()->JSValue{auto qv68=self;const auto qv69=JSValue("_x");const auto qv70=value;qv68.set(qv69,qv70);return qv70;}());
([&]()->JSValue{auto qv71=self;const auto qv72=JSValue("_onChangeCallback");const auto callee=qv71.get(qv72);const auto receiver=qv71;Args args;return callee.call(receiver,args);}());
return {};
}
JSValue method5(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());
return {};
}
JSValue method6(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue value=arg(args,0);
([&]()->JSValue{auto qv73=self;const auto qv74=JSValue("_y");const auto qv75=value;qv73.set(qv74,qv75);return qv75;}());
([&]()->JSValue{auto qv76=self;const auto qv77=JSValue("_onChangeCallback");const auto callee=qv76.get(qv77);const auto receiver=qv76;Args args;return callee.call(receiver,args);}());
return {};
}
JSValue method7(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());
return {};
}
JSValue method8(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue value=arg(args,0);
([&]()->JSValue{auto qv78=self;const auto qv79=JSValue("_z");const auto qv80=value;qv78.set(qv79,qv80);return qv80;}());
([&]()->JSValue{auto qv81=self;const auto qv82=JSValue("_onChangeCallback");const auto callee=qv81.get(qv82);const auto receiver=qv81;Args args;return callee.call(receiver,args);}());
return {};
}
JSValue method9(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());
return {};
}
JSValue method10(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue value=arg(args,0);
([&]()->JSValue{auto qv83=self;const auto qv84=JSValue("_w");const auto qv85=value;qv83.set(qv84,qv85);return qv85;}());
([&]()->JSValue{auto qv86=self;const auto qv87=JSValue("_onChangeCallback");const auto callee=qv86.get(qv87);const auto receiver=qv86;Args args;return callee.call(receiver,args);}());
return {};
}
JSValue method11(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue x=arg(args,0);
JSValue y=arg(args,1);
JSValue z=arg(args,2);
JSValue w=arg(args,3);
([&]()->JSValue{auto qv88=self;const auto qv89=JSValue("_x");const auto qv90=x;qv88.set(qv89,qv90);return qv90;}());
([&]()->JSValue{auto qv91=self;const auto qv92=JSValue("_y");const auto qv93=y;qv91.set(qv92,qv93);return qv93;}());
([&]()->JSValue{auto qv94=self;const auto qv95=JSValue("_z");const auto qv96=z;qv94.set(qv95,qv96);return qv96;}());
([&]()->JSValue{auto qv97=self;const auto qv98=JSValue("_w");const auto qv99=w;qv97.set(qv98,qv99);return qv99;}());
([&]()->JSValue{auto qv100=self;const auto qv101=JSValue("_onChangeCallback");const auto callee=qv100.get(qv101);const auto receiver=qv100;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method12(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{const auto callee=([&]()->JSValue{const auto object=self;const auto key=JSValue("constructor");return object.get(key);}());Args args;args.push_back(([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}()));args.push_back(([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}()));args.push_back(([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}()));args.push_back(([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}()));return callee.construct(args);}());
return {};
}
JSValue method13(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue quaternion=arg(args,0);
([&]()->JSValue{auto qv102=self;const auto qv103=JSValue("_x");const auto qv104=([&]()->JSValue{const auto object=quaternion;const auto key=JSValue("x");return object.get(key);}());qv102.set(qv103,qv104);return qv104;}());
([&]()->JSValue{auto qv105=self;const auto qv106=JSValue("_y");const auto qv107=([&]()->JSValue{const auto object=quaternion;const auto key=JSValue("y");return object.get(key);}());qv105.set(qv106,qv107);return qv107;}());
([&]()->JSValue{auto qv108=self;const auto qv109=JSValue("_z");const auto qv110=([&]()->JSValue{const auto object=quaternion;const auto key=JSValue("z");return object.get(key);}());qv108.set(qv109,qv110);return qv110;}());
([&]()->JSValue{auto qv111=self;const auto qv112=JSValue("_w");const auto qv113=([&]()->JSValue{const auto object=quaternion;const auto key=JSValue("w");return object.get(key);}());qv111.set(qv112,qv113);return qv113;}());
([&]()->JSValue{auto qv114=self;const auto qv115=JSValue("_onChangeCallback");const auto callee=qv114.get(qv115);const auto receiver=qv114;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method14(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue euler=arg(args,0);
JSValue update=arg(args,1);
if(update.isUndefined())update=JSValue(true);
JSValue x=([&]()->JSValue{const auto object=euler;const auto key=JSValue("_x");return object.get(key);}());
JSValue y=([&]()->JSValue{const auto object=euler;const auto key=JSValue("_y");return object.get(key);}());
JSValue z=([&]()->JSValue{const auto object=euler;const auto key=JSValue("_z");return object.get(key);}());
JSValue order=([&]()->JSValue{const auto object=euler;const auto key=JSValue("_order");return object.get(key);}());
JSValue cos=([&]()->JSValue{const auto object=nativeGlobalBinding("Math");const auto key=JSValue("cos");return object.get(key);}());
JSValue sin=([&]()->JSValue{const auto object=nativeGlobalBinding("Math");const auto key=JSValue("sin");return object.get(key);}());
JSValue c1=([&]()->JSValue{const auto callee=cos;const JSValue receiver;Args args;args.push_back(([&]()->JSValue{const auto left=x;const auto right=JSValue(2);return binary("/",left,right);}()));return callee.call(receiver,args);}());
JSValue c2=([&]()->JSValue{const auto callee=cos;const JSValue receiver;Args args;args.push_back(([&]()->JSValue{const auto left=y;const auto right=JSValue(2);return binary("/",left,right);}()));return callee.call(receiver,args);}());
JSValue c3=([&]()->JSValue{const auto callee=cos;const JSValue receiver;Args args;args.push_back(([&]()->JSValue{const auto left=z;const auto right=JSValue(2);return binary("/",left,right);}()));return callee.call(receiver,args);}());
JSValue s1=([&]()->JSValue{const auto callee=sin;const JSValue receiver;Args args;args.push_back(([&]()->JSValue{const auto left=x;const auto right=JSValue(2);return binary("/",left,right);}()));return callee.call(receiver,args);}());
JSValue s2=([&]()->JSValue{const auto callee=sin;const JSValue receiver;Args args;args.push_back(([&]()->JSValue{const auto left=y;const auto right=JSValue(2);return binary("/",left,right);}()));return callee.call(receiver,args);}());
JSValue s3=([&]()->JSValue{const auto callee=sin;const JSValue receiver;Args args;args.push_back(([&]()->JSValue{const auto left=z;const auto right=JSValue(2);return binary("/",left,right);}()));return callee.call(receiver,args);}());
{const auto qv116=order;
if(qv116.strictEquals(JSValue("XYZ"))){
([&]()->JSValue{auto qv117=self;const auto qv118=JSValue("_x");const auto qv119=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv117.set(qv118,qv119);return qv119;}());
([&]()->JSValue{auto qv120=self;const auto qv121=JSValue("_y");const auto qv122=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv120.set(qv121,qv122);return qv122;}());
([&]()->JSValue{auto qv123=self;const auto qv124=JSValue("_z");const auto qv125=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());return binary("+",left,right);}());qv123.set(qv124,qv125);return qv125;}());
([&]()->JSValue{auto qv126=self;const auto qv127=JSValue("_w");const auto qv128=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv126.set(qv127,qv128);return qv128;}());
}
else if(qv116.strictEquals(JSValue("YXZ"))){
([&]()->JSValue{auto qv129=self;const auto qv130=JSValue("_x");const auto qv131=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv129.set(qv130,qv131);return qv131;}());
([&]()->JSValue{auto qv132=self;const auto qv133=JSValue("_y");const auto qv134=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv132.set(qv133,qv134);return qv134;}());
([&]()->JSValue{auto qv135=self;const auto qv136=JSValue("_z");const auto qv137=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());return binary("-",left,right);}());qv135.set(qv136,qv137);return qv137;}());
([&]()->JSValue{auto qv138=self;const auto qv139=JSValue("_w");const auto qv140=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv138.set(qv139,qv140);return qv140;}());
}
else if(qv116.strictEquals(JSValue("ZXY"))){
([&]()->JSValue{auto qv141=self;const auto qv142=JSValue("_x");const auto qv143=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv141.set(qv142,qv143);return qv143;}());
([&]()->JSValue{auto qv144=self;const auto qv145=JSValue("_y");const auto qv146=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv144.set(qv145,qv146);return qv146;}());
([&]()->JSValue{auto qv147=self;const auto qv148=JSValue("_z");const auto qv149=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());return binary("+",left,right);}());qv147.set(qv148,qv149);return qv149;}());
([&]()->JSValue{auto qv150=self;const auto qv151=JSValue("_w");const auto qv152=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv150.set(qv151,qv152);return qv152;}());
}
else if(qv116.strictEquals(JSValue("ZYX"))){
([&]()->JSValue{auto qv153=self;const auto qv154=JSValue("_x");const auto qv155=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv153.set(qv154,qv155);return qv155;}());
([&]()->JSValue{auto qv156=self;const auto qv157=JSValue("_y");const auto qv158=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv156.set(qv157,qv158);return qv158;}());
([&]()->JSValue{auto qv159=self;const auto qv160=JSValue("_z");const auto qv161=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());return binary("-",left,right);}());qv159.set(qv160,qv161);return qv161;}());
([&]()->JSValue{auto qv162=self;const auto qv163=JSValue("_w");const auto qv164=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv162.set(qv163,qv164);return qv164;}());
}
else if(qv116.strictEquals(JSValue("YZX"))){
([&]()->JSValue{auto qv165=self;const auto qv166=JSValue("_x");const auto qv167=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv165.set(qv166,qv167);return qv167;}());
([&]()->JSValue{auto qv168=self;const auto qv169=JSValue("_y");const auto qv170=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv168.set(qv169,qv170);return qv170;}());
([&]()->JSValue{auto qv171=self;const auto qv172=JSValue("_z");const auto qv173=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());return binary("-",left,right);}());qv171.set(qv172,qv173);return qv173;}());
([&]()->JSValue{auto qv174=self;const auto qv175=JSValue("_w");const auto qv176=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv174.set(qv175,qv176);return qv176;}());
}
else if(qv116.strictEquals(JSValue("XZY"))){
([&]()->JSValue{auto qv177=self;const auto qv178=JSValue("_x");const auto qv179=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv177.set(qv178,qv179);return qv179;}());
([&]()->JSValue{auto qv180=self;const auto qv181=JSValue("_y");const auto qv182=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("-",left,right);}());qv180.set(qv181,qv182);return qv182;}());
([&]()->JSValue{auto qv183=self;const auto qv184=JSValue("_z");const auto qv185=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());return binary("+",left,right);}());qv183.set(qv184,qv185);return qv185;}());
([&]()->JSValue{auto qv186=self;const auto qv187=JSValue("_w");const auto qv188=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=c1;const auto right=c2;return binary("*",left,right);}());const auto right=c3;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=s1;const auto right=s2;return binary("*",left,right);}());const auto right=s3;return binary("*",left,right);}());return binary("+",left,right);}());qv186.set(qv187,qv188);return qv188;}());
}
else{
([&]()->JSValue{Args args;args.push_back(([&]()->JSValue{const auto left=JSValue("Quaternion: .setFromEuler() encountered an unknown order: ");const auto right=order;return binary("+",left,right);}()));ThreeUtils::warn(args);return {};}());
}
}
if(([&]()->JSValue{const auto left=update;const auto right=JSValue(true);return binary("===",left,right);}()).toBoolean())([&]()->JSValue{auto qv189=self;const auto qv190=JSValue("_onChangeCallback");const auto callee=qv189.get(qv190);const auto receiver=qv189;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method15(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue axis=arg(args,0);
JSValue angle=arg(args,1);
JSValue halfAngle=([&]()->JSValue{const auto left=angle;const auto right=JSValue(2);return binary("/",left,right);}());
JSValue s=([&]()->JSValue{auto qv191=nativeGlobalBinding("Math");const auto qv192=JSValue("sin");const auto callee=qv191.get(qv192);const auto receiver=qv191;Args args;args.push_back(halfAngle);return callee.call(receiver,args);}());
([&]()->JSValue{auto qv193=self;const auto qv194=JSValue("_x");const auto qv195=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=axis;const auto key=JSValue("x");return object.get(key);}());const auto right=s;return binary("*",left,right);}());qv193.set(qv194,qv195);return qv195;}());
([&]()->JSValue{auto qv196=self;const auto qv197=JSValue("_y");const auto qv198=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=axis;const auto key=JSValue("y");return object.get(key);}());const auto right=s;return binary("*",left,right);}());qv196.set(qv197,qv198);return qv198;}());
([&]()->JSValue{auto qv199=self;const auto qv200=JSValue("_z");const auto qv201=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=axis;const auto key=JSValue("z");return object.get(key);}());const auto right=s;return binary("*",left,right);}());qv199.set(qv200,qv201);return qv201;}());
([&]()->JSValue{auto qv202=self;const auto qv203=JSValue("_w");const auto qv204=([&]()->JSValue{auto qv205=nativeGlobalBinding("Math");const auto qv206=JSValue("cos");const auto callee=qv205.get(qv206);const auto receiver=qv205;Args args;args.push_back(halfAngle);return callee.call(receiver,args);}());qv202.set(qv203,qv204);return qv204;}());
([&]()->JSValue{auto qv207=self;const auto qv208=JSValue("_onChangeCallback");const auto callee=qv207.get(qv208);const auto receiver=qv207;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method16(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue m=arg(args,0);
JSValue te=([&]()->JSValue{const auto object=m;const auto key=JSValue("elements");return object.get(key);}());
JSValue m11=([&]()->JSValue{const auto object=te;const auto key=JSValue(0);return object.get(key);}());
JSValue m12=([&]()->JSValue{const auto object=te;const auto key=JSValue(4);return object.get(key);}());
JSValue m13=([&]()->JSValue{const auto object=te;const auto key=JSValue(8);return object.get(key);}());
JSValue m21=([&]()->JSValue{const auto object=te;const auto key=JSValue(1);return object.get(key);}());
JSValue m22=([&]()->JSValue{const auto object=te;const auto key=JSValue(5);return object.get(key);}());
JSValue m23=([&]()->JSValue{const auto object=te;const auto key=JSValue(9);return object.get(key);}());
JSValue m31=([&]()->JSValue{const auto object=te;const auto key=JSValue(2);return object.get(key);}());
JSValue m32=([&]()->JSValue{const auto object=te;const auto key=JSValue(6);return object.get(key);}());
JSValue m33=([&]()->JSValue{const auto object=te;const auto key=JSValue(10);return object.get(key);}());
JSValue trace=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m11;const auto right=m22;return binary("+",left,right);}());const auto right=m33;return binary("+",left,right);}());
if(([&]()->JSValue{const auto left=trace;const auto right=JSValue(0);return binary(">",left,right);}()).toBoolean()){
JSValue s=([&]()->JSValue{const auto left=JSValue(0.5);const auto right=([&]()->JSValue{auto qv209=nativeGlobalBinding("Math");const auto qv210=JSValue("sqrt");const auto callee=qv209.get(qv210);const auto receiver=qv209;Args args;args.push_back(([&]()->JSValue{const auto left=trace;const auto right=JSValue(1);return binary("+",left,right);}()));return callee.call(receiver,args);}());return binary("/",left,right);}());
([&]()->JSValue{auto qv211=self;const auto qv212=JSValue("_w");const auto qv213=([&]()->JSValue{const auto left=JSValue(0.25);const auto right=s;return binary("/",left,right);}());qv211.set(qv212,qv213);return qv213;}());
([&]()->JSValue{auto qv214=self;const auto qv215=JSValue("_x");const auto qv216=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m32;const auto right=m23;return binary("-",left,right);}());const auto right=s;return binary("*",left,right);}());qv214.set(qv215,qv216);return qv216;}());
([&]()->JSValue{auto qv217=self;const auto qv218=JSValue("_y");const auto qv219=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m13;const auto right=m31;return binary("-",left,right);}());const auto right=s;return binary("*",left,right);}());qv217.set(qv218,qv219);return qv219;}());
([&]()->JSValue{auto qv220=self;const auto qv221=JSValue("_z");const auto qv222=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m21;const auto right=m12;return binary("-",left,right);}());const auto right=s;return binary("*",left,right);}());qv220.set(qv221,qv222);return qv222;}());

}
else if(([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m11;const auto right=m22;return binary(">",left,right);}());if(!left.toBoolean())return left;return ([&]()->JSValue{const auto left=m11;const auto right=m33;return binary(">",left,right);}());}()).toBoolean()){
JSValue s=([&]()->JSValue{const auto left=JSValue(2);const auto right=([&]()->JSValue{auto qv223=nativeGlobalBinding("Math");const auto qv224=JSValue("sqrt");const auto callee=qv223.get(qv224);const auto receiver=qv223;Args args;args.push_back(([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=JSValue(1);const auto right=m11;return binary("+",left,right);}());const auto right=m22;return binary("-",left,right);}());const auto right=m33;return binary("-",left,right);}()));return callee.call(receiver,args);}());return binary("*",left,right);}());
([&]()->JSValue{auto qv225=self;const auto qv226=JSValue("_w");const auto qv227=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m32;const auto right=m23;return binary("-",left,right);}());const auto right=s;return binary("/",left,right);}());qv225.set(qv226,qv227);return qv227;}());
([&]()->JSValue{auto qv228=self;const auto qv229=JSValue("_x");const auto qv230=([&]()->JSValue{const auto left=JSValue(0.25);const auto right=s;return binary("*",left,right);}());qv228.set(qv229,qv230);return qv230;}());
([&]()->JSValue{auto qv231=self;const auto qv232=JSValue("_y");const auto qv233=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m12;const auto right=m21;return binary("+",left,right);}());const auto right=s;return binary("/",left,right);}());qv231.set(qv232,qv233);return qv233;}());
([&]()->JSValue{auto qv234=self;const auto qv235=JSValue("_z");const auto qv236=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m13;const auto right=m31;return binary("+",left,right);}());const auto right=s;return binary("/",left,right);}());qv234.set(qv235,qv236);return qv236;}());

}
else if(([&]()->JSValue{const auto left=m22;const auto right=m33;return binary(">",left,right);}()).toBoolean()){
JSValue s=([&]()->JSValue{const auto left=JSValue(2);const auto right=([&]()->JSValue{auto qv237=nativeGlobalBinding("Math");const auto qv238=JSValue("sqrt");const auto callee=qv237.get(qv238);const auto receiver=qv237;Args args;args.push_back(([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=JSValue(1);const auto right=m22;return binary("+",left,right);}());const auto right=m11;return binary("-",left,right);}());const auto right=m33;return binary("-",left,right);}()));return callee.call(receiver,args);}());return binary("*",left,right);}());
([&]()->JSValue{auto qv239=self;const auto qv240=JSValue("_w");const auto qv241=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m13;const auto right=m31;return binary("-",left,right);}());const auto right=s;return binary("/",left,right);}());qv239.set(qv240,qv241);return qv241;}());
([&]()->JSValue{auto qv242=self;const auto qv243=JSValue("_x");const auto qv244=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m12;const auto right=m21;return binary("+",left,right);}());const auto right=s;return binary("/",left,right);}());qv242.set(qv243,qv244);return qv244;}());
([&]()->JSValue{auto qv245=self;const auto qv246=JSValue("_y");const auto qv247=([&]()->JSValue{const auto left=JSValue(0.25);const auto right=s;return binary("*",left,right);}());qv245.set(qv246,qv247);return qv247;}());
([&]()->JSValue{auto qv248=self;const auto qv249=JSValue("_z");const auto qv250=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m23;const auto right=m32;return binary("+",left,right);}());const auto right=s;return binary("/",left,right);}());qv248.set(qv249,qv250);return qv250;}());

}
else {
JSValue s=([&]()->JSValue{const auto left=JSValue(2);const auto right=([&]()->JSValue{auto qv251=nativeGlobalBinding("Math");const auto qv252=JSValue("sqrt");const auto callee=qv251.get(qv252);const auto receiver=qv251;Args args;args.push_back(([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=JSValue(1);const auto right=m33;return binary("+",left,right);}());const auto right=m11;return binary("-",left,right);}());const auto right=m22;return binary("-",left,right);}()));return callee.call(receiver,args);}());return binary("*",left,right);}());
([&]()->JSValue{auto qv253=self;const auto qv254=JSValue("_w");const auto qv255=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m21;const auto right=m12;return binary("-",left,right);}());const auto right=s;return binary("/",left,right);}());qv253.set(qv254,qv255);return qv255;}());
([&]()->JSValue{auto qv256=self;const auto qv257=JSValue("_x");const auto qv258=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m13;const auto right=m31;return binary("+",left,right);}());const auto right=s;return binary("/",left,right);}());qv256.set(qv257,qv258);return qv258;}());
([&]()->JSValue{auto qv259=self;const auto qv260=JSValue("_y");const auto qv261=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=m23;const auto right=m32;return binary("+",left,right);}());const auto right=s;return binary("/",left,right);}());qv259.set(qv260,qv261);return qv261;}());
([&]()->JSValue{auto qv262=self;const auto qv263=JSValue("_z");const auto qv264=([&]()->JSValue{const auto left=JSValue(0.25);const auto right=s;return binary("*",left,right);}());qv262.set(qv263,qv264);return qv264;}());

}
([&]()->JSValue{auto qv265=self;const auto qv266=JSValue("_onChangeCallback");const auto callee=qv265.get(qv266);const auto receiver=qv265;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method17(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue vFrom=arg(args,0);
JSValue vTo=arg(args,1);
JSValue r=([&]()->JSValue{const auto left=([&]()->JSValue{auto qv267=vFrom;const auto qv268=JSValue("dot");const auto callee=qv267.get(qv268);const auto receiver=qv267;Args args;args.push_back(vTo);return callee.call(receiver,args);}());const auto right=JSValue(1);return binary("+",left,right);}());
if(([&]()->JSValue{const auto left=r;const auto right=JSValue(1e-8);return binary("<",left,right);}()).toBoolean()){
([&]()->JSValue{const auto qv269=JSValue(0);r=qv269;return qv269;}());
if(([&]()->JSValue{const auto left=([&]()->JSValue{auto qv270=nativeGlobalBinding("Math");const auto qv271=JSValue("abs");const auto callee=qv270.get(qv271);const auto receiver=qv270;Args args;args.push_back(([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("x");return object.get(key);}()));return callee.call(receiver,args);}());const auto right=([&]()->JSValue{auto qv272=nativeGlobalBinding("Math");const auto qv273=JSValue("abs");const auto callee=qv272.get(qv273);const auto receiver=qv272;Args args;args.push_back(([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("z");return object.get(key);}()));return callee.call(receiver,args);}());return binary(">",left,right);}()).toBoolean()){
([&]()->JSValue{auto qv274=self;const auto qv275=JSValue("_x");const auto qv276=([&]()->JSValue{const auto value=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("y");return object.get(key);}());return -value.toNumber();}());qv274.set(qv275,qv276);return qv276;}());
([&]()->JSValue{auto qv277=self;const auto qv278=JSValue("_y");const auto qv279=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("x");return object.get(key);}());qv277.set(qv278,qv279);return qv279;}());
([&]()->JSValue{auto qv280=self;const auto qv281=JSValue("_z");const auto qv282=JSValue(0);qv280.set(qv281,qv282);return qv282;}());
([&]()->JSValue{auto qv283=self;const auto qv284=JSValue("_w");const auto qv285=r;qv283.set(qv284,qv285);return qv285;}());

}
else {
([&]()->JSValue{auto qv286=self;const auto qv287=JSValue("_x");const auto qv288=JSValue(0);qv286.set(qv287,qv288);return qv288;}());
([&]()->JSValue{auto qv289=self;const auto qv290=JSValue("_y");const auto qv291=([&]()->JSValue{const auto value=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("z");return object.get(key);}());return -value.toNumber();}());qv289.set(qv290,qv291);return qv291;}());
([&]()->JSValue{auto qv292=self;const auto qv293=JSValue("_z");const auto qv294=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("y");return object.get(key);}());qv292.set(qv293,qv294);return qv294;}());
([&]()->JSValue{auto qv295=self;const auto qv296=JSValue("_w");const auto qv297=r;qv295.set(qv296,qv297);return qv297;}());

}

}
else {
([&]()->JSValue{auto qv298=self;const auto qv299=JSValue("_x");const auto qv300=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("y");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=vTo;const auto key=JSValue("z");return object.get(key);}());return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("z");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=vTo;const auto key=JSValue("y");return object.get(key);}());return binary("*",left,right);}());return binary("-",left,right);}());qv298.set(qv299,qv300);return qv300;}());
([&]()->JSValue{auto qv301=self;const auto qv302=JSValue("_y");const auto qv303=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("z");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=vTo;const auto key=JSValue("x");return object.get(key);}());return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("x");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=vTo;const auto key=JSValue("z");return object.get(key);}());return binary("*",left,right);}());return binary("-",left,right);}());qv301.set(qv302,qv303);return qv303;}());
([&]()->JSValue{auto qv304=self;const auto qv305=JSValue("_z");const auto qv306=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("x");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=vTo;const auto key=JSValue("y");return object.get(key);}());return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=vFrom;const auto key=JSValue("y");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=vTo;const auto key=JSValue("x");return object.get(key);}());return binary("*",left,right);}());return binary("-",left,right);}());qv304.set(qv305,qv306);return qv306;}());
([&]()->JSValue{auto qv307=self;const auto qv308=JSValue("_w");const auto qv309=r;qv307.set(qv308,qv309);return qv309;}());

}
return ([&]()->JSValue{auto qv310=self;const auto qv311=JSValue("normalize");const auto callee=qv310.get(qv311);const auto receiver=qv310;Args args;return callee.call(receiver,args);}());
return {};
}
JSValue method18(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue q=arg(args,0);
return ([&]()->JSValue{const auto left=JSValue(2);const auto right=([&]()->JSValue{auto qv312=nativeGlobalBinding("Math");const auto qv313=JSValue("acos");const auto callee=qv312.get(qv313);const auto receiver=qv312;Args args;args.push_back(([&]()->JSValue{auto qv314=nativeGlobalBinding("Math");const auto qv315=JSValue("abs");const auto callee=qv314.get(qv315);const auto receiver=qv314;Args args;args.push_back(([&]()->JSValue{const auto callee=captures[0];const JSValue receiver;Args args;args.push_back(([&]()->JSValue{auto qv316=self;const auto qv317=JSValue("dot");const auto callee=qv316.get(qv317);const auto receiver=qv316;Args args;args.push_back(q);return callee.call(receiver,args);}()));args.push_back(([&]()->JSValue{const auto value=JSValue(1);return -value.toNumber();}()));args.push_back(JSValue(1));return callee.call(receiver,args);}()));return callee.call(receiver,args);}()));return callee.call(receiver,args);}());return binary("*",left,right);}());
return {};
}
JSValue method19(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue q=arg(args,0);
JSValue step=arg(args,1);
JSValue angle=([&]()->JSValue{auto qv318=self;const auto qv319=JSValue("angleTo");const auto callee=qv318.get(qv319);const auto receiver=qv318;Args args;args.push_back(q);return callee.call(receiver,args);}());
if(([&]()->JSValue{const auto left=angle;const auto right=JSValue(0);return binary("===",left,right);}()).toBoolean())return self;
JSValue t=([&]()->JSValue{auto qv320=nativeGlobalBinding("Math");const auto qv321=JSValue("min");const auto callee=qv320.get(qv321);const auto receiver=qv320;Args args;args.push_back(JSValue(1));args.push_back(([&]()->JSValue{const auto left=step;const auto right=angle;return binary("/",left,right);}()));return callee.call(receiver,args);}());
([&]()->JSValue{auto qv322=self;const auto qv323=JSValue("slerp");const auto callee=qv322.get(qv323);const auto receiver=qv322;Args args;args.push_back(q);args.push_back(t);return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method20(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{auto qv324=self;const auto qv325=JSValue("set");const auto callee=qv324.get(qv325);const auto receiver=qv324;Args args;args.push_back(JSValue(0));args.push_back(JSValue(0));args.push_back(JSValue(0));args.push_back(JSValue(1));return callee.call(receiver,args);}());
return {};
}
JSValue method21(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{auto qv326=self;const auto qv327=JSValue("conjugate");const auto callee=qv326.get(qv327);const auto receiver=qv326;Args args;return callee.call(receiver,args);}());
return {};
}
JSValue method22(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
([&]()->JSValue{auto qv328=self;const auto qv329=JSValue("_x");const auto old=qv328.get(qv329);const auto right=([&]()->JSValue{const auto value=JSValue(1);return -value.toNumber();}());const auto qv330=binary("*",old,right);qv328.set(qv329,qv330);return qv330;}());
([&]()->JSValue{auto qv331=self;const auto qv332=JSValue("_y");const auto old=qv331.get(qv332);const auto right=([&]()->JSValue{const auto value=JSValue(1);return -value.toNumber();}());const auto qv333=binary("*",old,right);qv331.set(qv332,qv333);return qv333;}());
([&]()->JSValue{auto qv334=self;const auto qv335=JSValue("_z");const auto old=qv334.get(qv335);const auto right=([&]()->JSValue{const auto value=JSValue(1);return -value.toNumber();}());const auto qv336=binary("*",old,right);qv334.set(qv335,qv336);return qv336;}());
([&]()->JSValue{auto qv337=self;const auto qv338=JSValue("_onChangeCallback");const auto callee=qv337.get(qv338);const auto receiver=qv337;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method23(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue v=arg(args,0);
return ([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=v;const auto key=JSValue("_x");return object.get(key);}());return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=v;const auto key=JSValue("_y");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=v;const auto key=JSValue("_z");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=v;const auto key=JSValue("_w");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}());
return {};
}
JSValue method24(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}());
return {};
}
JSValue method25(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{auto qv339=nativeGlobalBinding("Math");const auto qv340=JSValue("sqrt");const auto callee=qv339.get(qv340);const auto receiver=qv339;Args args;args.push_back(([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());return binary("*",left,right);}());return binary("+",left,right);}()));return callee.call(receiver,args);}());
return {};
}
JSValue method26(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue l=([&]()->JSValue{auto qv341=self;const auto qv342=JSValue("length");const auto callee=qv341.get(qv342);const auto receiver=qv341;Args args;return callee.call(receiver,args);}());
if(([&]()->JSValue{const auto left=l;const auto right=JSValue(0);return binary("===",left,right);}()).toBoolean()){
([&]()->JSValue{auto qv343=self;const auto qv344=JSValue("_x");const auto qv345=JSValue(0);qv343.set(qv344,qv345);return qv345;}());
([&]()->JSValue{auto qv346=self;const auto qv347=JSValue("_y");const auto qv348=JSValue(0);qv346.set(qv347,qv348);return qv348;}());
([&]()->JSValue{auto qv349=self;const auto qv350=JSValue("_z");const auto qv351=JSValue(0);qv349.set(qv350,qv351);return qv351;}());
([&]()->JSValue{auto qv352=self;const auto qv353=JSValue("_w");const auto qv354=JSValue(1);qv352.set(qv353,qv354);return qv354;}());

}
else {
([&]()->JSValue{const auto qv355=([&]()->JSValue{const auto left=JSValue(1);const auto right=l;return binary("/",left,right);}());l=qv355;return qv355;}());
([&]()->JSValue{auto qv356=self;const auto qv357=JSValue("_x");const auto qv358=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());const auto right=l;return binary("*",left,right);}());qv356.set(qv357,qv358);return qv358;}());
([&]()->JSValue{auto qv359=self;const auto qv360=JSValue("_y");const auto qv361=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());const auto right=l;return binary("*",left,right);}());qv359.set(qv360,qv361);return qv361;}());
([&]()->JSValue{auto qv362=self;const auto qv363=JSValue("_z");const auto qv364=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());const auto right=l;return binary("*",left,right);}());qv362.set(qv363,qv364);return qv364;}());
([&]()->JSValue{auto qv365=self;const auto qv366=JSValue("_w");const auto qv367=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());const auto right=l;return binary("*",left,right);}());qv365.set(qv366,qv367);return qv367;}());

}
([&]()->JSValue{auto qv368=self;const auto qv369=JSValue("_onChangeCallback");const auto callee=qv368.get(qv369);const auto receiver=qv368;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method27(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue q=arg(args,0);
return ([&]()->JSValue{auto qv370=self;const auto qv371=JSValue("multiplyQuaternions");const auto callee=qv370.get(qv371);const auto receiver=qv370;Args args;args.push_back(self);args.push_back(q);return callee.call(receiver,args);}());
return {};
}
JSValue method28(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue q=arg(args,0);
return ([&]()->JSValue{auto qv372=self;const auto qv373=JSValue("multiplyQuaternions");const auto callee=qv372.get(qv373);const auto receiver=qv372;Args args;args.push_back(q);args.push_back(self);return callee.call(receiver,args);}());
return {};
}
JSValue method29(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue a=arg(args,0);
JSValue b=arg(args,1);
JSValue qax=([&]()->JSValue{const auto object=a;const auto key=JSValue("_x");return object.get(key);}());
JSValue qay=([&]()->JSValue{const auto object=a;const auto key=JSValue("_y");return object.get(key);}());
JSValue qaz=([&]()->JSValue{const auto object=a;const auto key=JSValue("_z");return object.get(key);}());
JSValue qaw=([&]()->JSValue{const auto object=a;const auto key=JSValue("_w");return object.get(key);}());
JSValue qbx=([&]()->JSValue{const auto object=b;const auto key=JSValue("_x");return object.get(key);}());
JSValue qby=([&]()->JSValue{const auto object=b;const auto key=JSValue("_y");return object.get(key);}());
JSValue qbz=([&]()->JSValue{const auto object=b;const auto key=JSValue("_z");return object.get(key);}());
JSValue qbw=([&]()->JSValue{const auto object=b;const auto key=JSValue("_w");return object.get(key);}());
([&]()->JSValue{auto qv374=self;const auto qv375=JSValue("_x");const auto qv376=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=qax;const auto right=qbw;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=qaw;const auto right=qbx;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=qay;const auto right=qbz;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=qaz;const auto right=qby;return binary("*",left,right);}());return binary("-",left,right);}());qv374.set(qv375,qv376);return qv376;}());
([&]()->JSValue{auto qv377=self;const auto qv378=JSValue("_y");const auto qv379=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=qay;const auto right=qbw;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=qaw;const auto right=qby;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=qaz;const auto right=qbx;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=qax;const auto right=qbz;return binary("*",left,right);}());return binary("-",left,right);}());qv377.set(qv378,qv379);return qv379;}());
([&]()->JSValue{auto qv380=self;const auto qv381=JSValue("_z");const auto qv382=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=qaz;const auto right=qbw;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=qaw;const auto right=qbz;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=qax;const auto right=qby;return binary("*",left,right);}());return binary("+",left,right);}());const auto right=([&]()->JSValue{const auto left=qay;const auto right=qbx;return binary("*",left,right);}());return binary("-",left,right);}());qv380.set(qv381,qv382);return qv382;}());
([&]()->JSValue{auto qv383=self;const auto qv384=JSValue("_w");const auto qv385=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=qaw;const auto right=qbw;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=qax;const auto right=qbx;return binary("*",left,right);}());return binary("-",left,right);}());const auto right=([&]()->JSValue{const auto left=qay;const auto right=qby;return binary("*",left,right);}());return binary("-",left,right);}());const auto right=([&]()->JSValue{const auto left=qaz;const auto right=qbz;return binary("*",left,right);}());return binary("-",left,right);}());qv383.set(qv384,qv385);return qv385;}());
([&]()->JSValue{auto qv386=self;const auto qv387=JSValue("_onChangeCallback");const auto callee=qv386.get(qv387);const auto receiver=qv386;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method30(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue qb=arg(args,0);
JSValue t=arg(args,1);
JSValue x=([&]()->JSValue{const auto object=qb;const auto key=JSValue("_x");return object.get(key);}());
JSValue y=([&]()->JSValue{const auto object=qb;const auto key=JSValue("_y");return object.get(key);}());
JSValue z=([&]()->JSValue{const auto object=qb;const auto key=JSValue("_z");return object.get(key);}());
JSValue w=([&]()->JSValue{const auto object=qb;const auto key=JSValue("_w");return object.get(key);}());
JSValue dot=([&]()->JSValue{auto qv388=self;const auto qv389=JSValue("dot");const auto callee=qv388.get(qv389);const auto receiver=qv388;Args args;args.push_back(qb);return callee.call(receiver,args);}());
if(([&]()->JSValue{const auto left=dot;const auto right=JSValue(0);return binary("<",left,right);}()).toBoolean()){
([&]()->JSValue{const auto qv390=([&]()->JSValue{const auto value=x;return -value.toNumber();}());x=qv390;return qv390;}());
([&]()->JSValue{const auto qv391=([&]()->JSValue{const auto value=y;return -value.toNumber();}());y=qv391;return qv391;}());
([&]()->JSValue{const auto qv392=([&]()->JSValue{const auto value=z;return -value.toNumber();}());z=qv392;return qv392;}());
([&]()->JSValue{const auto qv393=([&]()->JSValue{const auto value=w;return -value.toNumber();}());w=qv393;return qv393;}());
([&]()->JSValue{const auto qv394=([&]()->JSValue{const auto value=dot;return -value.toNumber();}());dot=qv394;return qv394;}());

}
JSValue s=([&]()->JSValue{const auto left=JSValue(1);const auto right=t;return binary("-",left,right);}());
if(([&]()->JSValue{const auto left=dot;const auto right=JSValue(0.9995);return binary("<",left,right);}()).toBoolean()){
JSValue theta=([&]()->JSValue{auto qv395=nativeGlobalBinding("Math");const auto qv396=JSValue("acos");const auto callee=qv395.get(qv396);const auto receiver=qv395;Args args;args.push_back(dot);return callee.call(receiver,args);}());
JSValue sin=([&]()->JSValue{auto qv397=nativeGlobalBinding("Math");const auto qv398=JSValue("sin");const auto callee=qv397.get(qv398);const auto receiver=qv397;Args args;args.push_back(theta);return callee.call(receiver,args);}());
([&]()->JSValue{const auto qv399=([&]()->JSValue{const auto left=([&]()->JSValue{auto qv400=nativeGlobalBinding("Math");const auto qv401=JSValue("sin");const auto callee=qv400.get(qv401);const auto receiver=qv400;Args args;args.push_back(([&]()->JSValue{const auto left=s;const auto right=theta;return binary("*",left,right);}()));return callee.call(receiver,args);}());const auto right=sin;return binary("/",left,right);}());s=qv399;return qv399;}());
([&]()->JSValue{const auto qv402=([&]()->JSValue{const auto left=([&]()->JSValue{auto qv403=nativeGlobalBinding("Math");const auto qv404=JSValue("sin");const auto callee=qv403.get(qv404);const auto receiver=qv403;Args args;args.push_back(([&]()->JSValue{const auto left=t;const auto right=theta;return binary("*",left,right);}()));return callee.call(receiver,args);}());const auto right=sin;return binary("/",left,right);}());t=qv402;return qv402;}());
([&]()->JSValue{auto qv405=self;const auto qv406=JSValue("_x");const auto qv407=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=x;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());qv405.set(qv406,qv407);return qv407;}());
([&]()->JSValue{auto qv408=self;const auto qv409=JSValue("_y");const auto qv410=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=y;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());qv408.set(qv409,qv410);return qv410;}());
([&]()->JSValue{auto qv411=self;const auto qv412=JSValue("_z");const auto qv413=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=z;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());qv411.set(qv412,qv413);return qv413;}());
([&]()->JSValue{auto qv414=self;const auto qv415=JSValue("_w");const auto qv416=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=w;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());qv414.set(qv415,qv416);return qv416;}());
([&]()->JSValue{auto qv417=self;const auto qv418=JSValue("_onChangeCallback");const auto callee=qv417.get(qv418);const auto receiver=qv417;Args args;return callee.call(receiver,args);}());

}
else {
([&]()->JSValue{auto qv419=self;const auto qv420=JSValue("_x");const auto qv421=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=x;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());qv419.set(qv420,qv421);return qv421;}());
([&]()->JSValue{auto qv422=self;const auto qv423=JSValue("_y");const auto qv424=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=y;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());qv422.set(qv423,qv424);return qv424;}());
([&]()->JSValue{auto qv425=self;const auto qv426=JSValue("_z");const auto qv427=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=z;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());qv425.set(qv426,qv427);return qv427;}());
([&]()->JSValue{auto qv428=self;const auto qv429=JSValue("_w");const auto qv430=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());const auto right=s;return binary("*",left,right);}());const auto right=([&]()->JSValue{const auto left=w;const auto right=t;return binary("*",left,right);}());return binary("+",left,right);}());qv428.set(qv429,qv430);return qv430;}());
([&]()->JSValue{auto qv431=self;const auto qv432=JSValue("normalize");const auto callee=qv431.get(qv432);const auto receiver=qv431;Args args;return callee.call(receiver,args);}());

}
return self;
return {};
}
JSValue method31(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue qa=arg(args,0);
JSValue qb=arg(args,1);
JSValue t=arg(args,2);
return ([&]()->JSValue{auto qv433=([&]()->JSValue{auto qv435=self;const auto qv436=JSValue("copy");const auto callee=qv435.get(qv436);const auto receiver=qv435;Args args;args.push_back(qa);return callee.call(receiver,args);}());const auto qv434=JSValue("slerp");const auto callee=qv433.get(qv434);const auto receiver=qv433;Args args;args.push_back(qb);args.push_back(t);return callee.call(receiver,args);}());
return {};
}
JSValue method32(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue theta1=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=JSValue(2);const auto right=([&]()->JSValue{const auto object=nativeGlobalBinding("Math");const auto key=JSValue("PI");return object.get(key);}());return binary("*",left,right);}());const auto right=([&]()->JSValue{auto qv437=nativeGlobalBinding("Math");const auto qv438=JSValue("random");const auto callee=qv437.get(qv438);const auto receiver=qv437;Args args;return callee.call(receiver,args);}());return binary("*",left,right);}());
JSValue theta2=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=JSValue(2);const auto right=([&]()->JSValue{const auto object=nativeGlobalBinding("Math");const auto key=JSValue("PI");return object.get(key);}());return binary("*",left,right);}());const auto right=([&]()->JSValue{auto qv439=nativeGlobalBinding("Math");const auto qv440=JSValue("random");const auto callee=qv439.get(qv440);const auto receiver=qv439;Args args;return callee.call(receiver,args);}());return binary("*",left,right);}());
JSValue x0=([&]()->JSValue{auto qv441=nativeGlobalBinding("Math");const auto qv442=JSValue("random");const auto callee=qv441.get(qv442);const auto receiver=qv441;Args args;return callee.call(receiver,args);}());
JSValue r1=([&]()->JSValue{auto qv443=nativeGlobalBinding("Math");const auto qv444=JSValue("sqrt");const auto callee=qv443.get(qv444);const auto receiver=qv443;Args args;args.push_back(([&]()->JSValue{const auto left=JSValue(1);const auto right=x0;return binary("-",left,right);}()));return callee.call(receiver,args);}());
JSValue r2=([&]()->JSValue{auto qv445=nativeGlobalBinding("Math");const auto qv446=JSValue("sqrt");const auto callee=qv445.get(qv446);const auto receiver=qv445;Args args;args.push_back(x0);return callee.call(receiver,args);}());
return ([&]()->JSValue{auto qv447=self;const auto qv448=JSValue("set");const auto callee=qv447.get(qv448);const auto receiver=qv447;Args args;args.push_back(([&]()->JSValue{const auto left=r1;const auto right=([&]()->JSValue{auto qv449=nativeGlobalBinding("Math");const auto qv450=JSValue("sin");const auto callee=qv449.get(qv450);const auto receiver=qv449;Args args;args.push_back(theta1);return callee.call(receiver,args);}());return binary("*",left,right);}()));args.push_back(([&]()->JSValue{const auto left=r1;const auto right=([&]()->JSValue{auto qv451=nativeGlobalBinding("Math");const auto qv452=JSValue("cos");const auto callee=qv451.get(qv452);const auto receiver=qv451;Args args;args.push_back(theta1);return callee.call(receiver,args);}());return binary("*",left,right);}()));args.push_back(([&]()->JSValue{const auto left=r2;const auto right=([&]()->JSValue{auto qv453=nativeGlobalBinding("Math");const auto qv454=JSValue("sin");const auto callee=qv453.get(qv454);const auto receiver=qv453;Args args;args.push_back(theta2);return callee.call(receiver,args);}());return binary("*",left,right);}()));args.push_back(([&]()->JSValue{const auto left=r2;const auto right=([&]()->JSValue{auto qv455=nativeGlobalBinding("Math");const auto qv456=JSValue("cos");const auto callee=qv455.get(qv456);const auto receiver=qv455;Args args;args.push_back(theta2);return callee.call(receiver,args);}());return binary("*",left,right);}()));return callee.call(receiver,args);}());
return {};
}
JSValue method33(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue quaternion=arg(args,0);
return ([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=quaternion;const auto key=JSValue("_x");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());return binary("===",left,right);}());if(!left.toBoolean())return left;return ([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=quaternion;const auto key=JSValue("_y");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());return binary("===",left,right);}());}());if(!left.toBoolean())return left;return ([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=quaternion;const auto key=JSValue("_z");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());return binary("===",left,right);}());}());if(!left.toBoolean())return left;return ([&]()->JSValue{const auto left=([&]()->JSValue{const auto object=quaternion;const auto key=JSValue("_w");return object.get(key);}());const auto right=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());return binary("===",left,right);}());}());
return {};
}
JSValue method34(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue array=arg(args,0);
JSValue offset=arg(args,1);
if(offset.isUndefined())offset=JSValue(0);
([&]()->JSValue{auto qv457=self;const auto qv458=JSValue("_x");const auto qv459=([&]()->JSValue{const auto object=array;const auto key=offset;return object.get(key);}());qv457.set(qv458,qv459);return qv459;}());
([&]()->JSValue{auto qv460=self;const auto qv461=JSValue("_y");const auto qv462=([&]()->JSValue{const auto object=array;const auto key=([&]()->JSValue{const auto left=offset;const auto right=JSValue(1);return binary("+",left,right);}());return object.get(key);}());qv460.set(qv461,qv462);return qv462;}());
([&]()->JSValue{auto qv463=self;const auto qv464=JSValue("_z");const auto qv465=([&]()->JSValue{const auto object=array;const auto key=([&]()->JSValue{const auto left=offset;const auto right=JSValue(2);return binary("+",left,right);}());return object.get(key);}());qv463.set(qv464,qv465);return qv465;}());
([&]()->JSValue{auto qv466=self;const auto qv467=JSValue("_w");const auto qv468=([&]()->JSValue{const auto object=array;const auto key=([&]()->JSValue{const auto left=offset;const auto right=JSValue(3);return binary("+",left,right);}());return object.get(key);}());qv466.set(qv467,qv468);return qv468;}());
([&]()->JSValue{auto qv469=self;const auto qv470=JSValue("_onChangeCallback");const auto callee=qv469.get(qv470);const auto receiver=qv469;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method35(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue array=arg(args,0);
if(array.isUndefined())array=JSValue::array();
JSValue offset=arg(args,1);
if(offset.isUndefined())offset=JSValue(0);
([&]()->JSValue{auto qv471=array;const auto qv472=offset;const auto qv473=([&]()->JSValue{const auto object=self;const auto key=JSValue("_x");return object.get(key);}());qv471.set(qv472,qv473);return qv473;}());
([&]()->JSValue{auto qv474=array;const auto qv475=([&]()->JSValue{const auto left=offset;const auto right=JSValue(1);return binary("+",left,right);}());const auto qv476=([&]()->JSValue{const auto object=self;const auto key=JSValue("_y");return object.get(key);}());qv474.set(qv475,qv476);return qv476;}());
([&]()->JSValue{auto qv477=array;const auto qv478=([&]()->JSValue{const auto left=offset;const auto right=JSValue(2);return binary("+",left,right);}());const auto qv479=([&]()->JSValue{const auto object=self;const auto key=JSValue("_z");return object.get(key);}());qv477.set(qv478,qv479);return qv479;}());
([&]()->JSValue{auto qv480=array;const auto qv481=([&]()->JSValue{const auto left=offset;const auto right=JSValue(3);return binary("+",left,right);}());const auto qv482=([&]()->JSValue{const auto object=self;const auto key=JSValue("_w");return object.get(key);}());qv480.set(qv481,qv482);return qv482;}());
return array;
return {};
}
JSValue method36(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue attribute=arg(args,0);
JSValue index=arg(args,1);
([&]()->JSValue{auto qv483=self;const auto qv484=JSValue("_x");const auto qv485=([&]()->JSValue{auto qv486=attribute;const auto qv487=JSValue("getX");const auto callee=qv486.get(qv487);const auto receiver=qv486;Args args;args.push_back(index);return callee.call(receiver,args);}());qv483.set(qv484,qv485);return qv485;}());
([&]()->JSValue{auto qv488=self;const auto qv489=JSValue("_y");const auto qv490=([&]()->JSValue{auto qv491=attribute;const auto qv492=JSValue("getY");const auto callee=qv491.get(qv492);const auto receiver=qv491;Args args;args.push_back(index);return callee.call(receiver,args);}());qv488.set(qv489,qv490);return qv490;}());
([&]()->JSValue{auto qv493=self;const auto qv494=JSValue("_z");const auto qv495=([&]()->JSValue{auto qv496=attribute;const auto qv497=JSValue("getZ");const auto callee=qv496.get(qv497);const auto receiver=qv496;Args args;args.push_back(index);return callee.call(receiver,args);}());qv493.set(qv494,qv495);return qv495;}());
([&]()->JSValue{auto qv498=self;const auto qv499=JSValue("_w");const auto qv500=([&]()->JSValue{auto qv501=attribute;const auto qv502=JSValue("getW");const auto callee=qv501.get(qv502);const auto receiver=qv501;Args args;args.push_back(index);return callee.call(receiver,args);}());qv498.set(qv499,qv500);return qv500;}());
([&]()->JSValue{auto qv503=self;const auto qv504=JSValue("_onChangeCallback");const auto callee=qv503.get(qv504);const auto receiver=qv503;Args args;return callee.call(receiver,args);}());
return self;
return {};
}
JSValue method37(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return ([&]()->JSValue{auto qv505=self;const auto qv506=JSValue("toArray");const auto callee=qv505.get(qv506);const auto receiver=qv505;Args args;return callee.call(receiver,args);}());
return {};
}
JSValue method38(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
JSValue callback=arg(args,0);
([&]()->JSValue{auto qv507=self;const auto qv508=JSValue("_onChangeCallback");const auto qv509=callback;qv507.set(qv508,qv509);return qv509;}());
return self;
return {};
}
JSValue method39(const JSValue& receiver,const Args& args,const Args& captures){
auto self=receiver;
return {};
}
JSGeneratorStep iteratorBody(const JSValue& privateState,const JSGeneratorCompletion& completion,const JSValue& self,const Args&,const Args&){
 if(completion.kind==JSGeneratorCompletionKind::Return)return JSGeneratorStep::complete(completion.value);
 if(completion.kind==JSGeneratorCompletionKind::Throw)return JSGeneratorStep::abrupt(completion.value);
 auto state=privateState;const auto pc=state.get("pc");const auto index=pc.isUndefined()?0:static_cast<unsigned>(pc.number());
 if(index>=4)return JSGeneratorStep::complete();state.set("pc",index+1);const char* fields[]={"_x","_y","_z","_w"};return JSGeneratorStep::yield(self.get(fields[index]));}
}
JSValue nativeQuaternionModule(){
using namespace QuaternionManaged;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject())return *existing;
static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Quaternion module");
const auto clamp=nativeMathUtilsClamp();auto ctor=JSValue::constructor(method0,{clamp},"Quaternion",0);ctor.setFunctionSourceText(QuaternionRuntimeSource::classSource);auto prototype=ctor.get("prototype");
{auto fn=JSValue::function(method1,{clamp},"slerpFlat",7);fn.setFunctionSourceText(QuaternionRuntimeSource::source1);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;ctor.defineProperty("slerpFlat",d);}
{auto fn=JSValue::function(method2,{clamp},"multiplyQuaternionsFlat",6);fn.setFunctionSourceText(QuaternionRuntimeSource::source2);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;ctor.defineProperty("multiplyQuaternionsFlat",d);}
{auto fn=JSValue::function(method3,{clamp},"get x",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source3);JSPropertyDescriptor d;d.getter=fn;d.configurable=true;prototype.defineProperty("x",d);}
{auto fn=JSValue::function(method4,{clamp},"set x",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source4);JSPropertyDescriptor d;d.setter=fn;d.configurable=true;prototype.defineProperty("x",d);}
{auto fn=JSValue::function(method5,{clamp},"get y",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source5);JSPropertyDescriptor d;d.getter=fn;d.configurable=true;prototype.defineProperty("y",d);}
{auto fn=JSValue::function(method6,{clamp},"set y",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source6);JSPropertyDescriptor d;d.setter=fn;d.configurable=true;prototype.defineProperty("y",d);}
{auto fn=JSValue::function(method7,{clamp},"get z",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source7);JSPropertyDescriptor d;d.getter=fn;d.configurable=true;prototype.defineProperty("z",d);}
{auto fn=JSValue::function(method8,{clamp},"set z",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source8);JSPropertyDescriptor d;d.setter=fn;d.configurable=true;prototype.defineProperty("z",d);}
{auto fn=JSValue::function(method9,{clamp},"get w",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source9);JSPropertyDescriptor d;d.getter=fn;d.configurable=true;prototype.defineProperty("w",d);}
{auto fn=JSValue::function(method10,{clamp},"set w",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source10);JSPropertyDescriptor d;d.setter=fn;d.configurable=true;prototype.defineProperty("w",d);}
{auto fn=JSValue::function(method11,{clamp},"set",4);fn.setFunctionSourceText(QuaternionRuntimeSource::source11);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("set",d);}
{auto fn=JSValue::function(method12,{clamp},"clone",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source12);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("clone",d);}
{auto fn=JSValue::function(method13,{clamp},"copy",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source13);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(method14,{clamp},"setFromEuler",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source14);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("setFromEuler",d);}
{auto fn=JSValue::function(method15,{clamp},"setFromAxisAngle",2);fn.setFunctionSourceText(QuaternionRuntimeSource::source15);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("setFromAxisAngle",d);}
{auto fn=JSValue::function(method16,{clamp},"setFromRotationMatrix",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source16);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("setFromRotationMatrix",d);}
{auto fn=JSValue::function(method17,{clamp},"setFromUnitVectors",2);fn.setFunctionSourceText(QuaternionRuntimeSource::source17);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("setFromUnitVectors",d);}
{auto fn=JSValue::function(method18,{clamp},"angleTo",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source18);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("angleTo",d);}
{auto fn=JSValue::function(method19,{clamp},"rotateTowards",2);fn.setFunctionSourceText(QuaternionRuntimeSource::source19);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("rotateTowards",d);}
{auto fn=JSValue::function(method20,{clamp},"identity",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source20);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("identity",d);}
{auto fn=JSValue::function(method21,{clamp},"invert",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source21);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("invert",d);}
{auto fn=JSValue::function(method22,{clamp},"conjugate",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source22);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("conjugate",d);}
{auto fn=JSValue::function(method23,{clamp},"dot",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source23);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("dot",d);}
{auto fn=JSValue::function(method24,{clamp},"lengthSq",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source24);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("lengthSq",d);}
{auto fn=JSValue::function(method25,{clamp},"length",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source25);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("length",d);}
{auto fn=JSValue::function(method26,{clamp},"normalize",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source26);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("normalize",d);}
{auto fn=JSValue::function(method27,{clamp},"multiply",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source27);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("multiply",d);}
{auto fn=JSValue::function(method28,{clamp},"premultiply",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source28);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("premultiply",d);}
{auto fn=JSValue::function(method29,{clamp},"multiplyQuaternions",2);fn.setFunctionSourceText(QuaternionRuntimeSource::source29);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("multiplyQuaternions",d);}
{auto fn=JSValue::function(method30,{clamp},"slerp",2);fn.setFunctionSourceText(QuaternionRuntimeSource::source30);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("slerp",d);}
{auto fn=JSValue::function(method31,{clamp},"slerpQuaternions",3);fn.setFunctionSourceText(QuaternionRuntimeSource::source31);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("slerpQuaternions",d);}
{auto fn=JSValue::function(method32,{clamp},"random",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source32);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("random",d);}
{auto fn=JSValue::function(method33,{clamp},"equals",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source33);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("equals",d);}
{auto fn=JSValue::function(method34,{clamp},"fromArray",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source34);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("fromArray",d);}
{auto fn=JSValue::function(method35,{clamp},"toArray",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source35);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("toArray",d);}
{auto fn=JSValue::function(method36,{clamp},"fromBufferAttribute",2);fn.setFunctionSourceText(QuaternionRuntimeSource::source36);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("fromBufferAttribute",d);}
{auto fn=JSValue::function(method37,{clamp},"toJSON",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source37);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("toJSON",d);}
{auto fn=JSValue::function(method38,{clamp},"_onChange",1);fn.setFunctionSourceText(QuaternionRuntimeSource::source38);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("_onChange",d);}
{auto fn=JSValue::function(method39,{clamp},"_onChangeCallback",0);fn.setFunctionSourceText(QuaternionRuntimeSource::source39);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("_onChangeCallback",d);}
{auto fn=createNativeGeneratorFunction(iteratorBody,{},"[Symbol.iterator]",0,QuaternionRuntimeSource::source40);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty(JSValue::wellKnownSymbol("iterator"),d);}
auto bindings=JSValue::object(nullptr);own(bindings,"Quaternion",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);auto module=JSValue::object();own(module,"Quaternion",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeQuaternionBinding(const std::string& name){const auto keeper=nativeQuaternionModule();return QuaternionManaged::lexical().lock()->get(name);}
JSValue nativeQuaternionConstructor(){return nativeQuaternionBinding("Quaternion");}
