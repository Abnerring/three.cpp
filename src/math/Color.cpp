#include "math/Color.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unordered_map>

Color::Color(HIGH_PRECISION r, HIGH_PRECISION g, HIGH_PRECISION b)
    : m_r(r), m_g(g), m_b(b)
{
}

Color::Color(int hex)
    : m_r(1.0), m_g(1.0), m_b(1.0)
{
    setHex(hex);
}

Color::Color(const std::string &colorString)
    : m_r(1.0), m_g(1.0), m_b(1.0)
{
    setColorName(colorString);
}

Color::~Color()
{
}

HIGH_PRECISION Color::r() const { return m_r; }
HIGH_PRECISION Color::g() const { return m_g; }
HIGH_PRECISION Color::b() const { return m_b; }

Color &Color::set(HIGH_PRECISION r, HIGH_PRECISION g, HIGH_PRECISION b)
{
    m_r = r;
    m_g = g;
    m_b = b;
    return *this;
}

Color &Color::setScalar(HIGH_PRECISION scalar)
{
    m_r = m_g = m_b = scalar;
    return *this;
}

Color &Color::setHex(int hex)
{
    m_r = ((hex >> 16) & 0xFF) / 255.0;
    m_g = ((hex >> 8) & 0xFF) / 255.0;
    m_b = (hex & 0xFF) / 255.0;
    return *this;
}

Color &Color::setHSL(HIGH_PRECISION h, HIGH_PRECISION s, HIGH_PRECISION l)
{
    if (s == 0.0)
    {
        m_r = m_g = m_b = l;
    }
    else
    {
        auto hue2rgb = [](HIGH_PRECISION p, HIGH_PRECISION q, HIGH_PRECISION t) -> HIGH_PRECISION
        {
            if (t < 0.0) t += 1.0;
            if (t > 1.0) t -= 1.0;
            if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
            if (t < 1.0 / 2.0) return q;
            if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
            return p;
        };

        auto q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
        auto p = 2.0 * l - q;

        m_r = hue2rgb(p, q, h + 1.0 / 3.0);
        m_g = hue2rgb(p, q, h);
        m_b = hue2rgb(p, q, h - 1.0 / 3.0);
    }

    return *this;
}

Color &Color::setColorName(const std::string &colorName)
{
    return setHex(getHexColorName(colorName));
}

int Color::getHexColorName(const std::string &name)
{
    // Common color names
    static const std::unordered_map<std::string, int> colors = {
        {"black", 0x000000}, {"white", 0xFFFFFF}, {"red", 0xFF0000},
        {"green", 0x008000}, {"blue", 0x0000FF}, {"yellow", 0xFFFF00},
        {"cyan", 0x00FFFF}, {"magenta", 0xFF00FF}, {"orange", 0xFFA500},
        {"pink", 0xFFC0CB}, {"purple", 0x800080}, {"brown", 0xA52A2A},
        {"gray", 0x808080}, {"grey", 0x808080}, {"navy", 0x000080},
        {"teal", 0x008080}, {"olive", 0x808000}, {"maroon", 0x800000},
        {"coral", 0xFF7F50}, {"crimson", 0xDC143C}, {"gold", 0xFFD700},
        {"indigo", 0x4B0082}, {"ivory", 0xFFFFF0}, {"khaki", 0xF0E68C},
        {"lavender", 0xE6E6FA}, {"lime", 0x00FF00}, {"plum", 0xDDA0DD},
        {"salmon", 0xFA8072}, {"silver", 0xC0C0C0}, {"tan", 0xD2B48C},
        {"violet", 0xEE82EE}
    };

    auto it = colors.find(name);
    return (it != colors.end()) ? it->second : 0x000000;
}

Color Color::clone() const
{
    return Color(m_r, m_g, m_b);
}

Color &Color::copy(const Color &color)
{
    m_r = color.r();
    m_g = color.g();
    m_b = color.b();
    return *this;
}

Color &Color::add(const Color &color)
{
    m_r += color.r();
    m_g += color.g();
    m_b += color.b();
    return *this;
}

Color &Color::addColors(const Color &color1, const Color &color2)
{
    m_r = color1.r() + color2.r();
    m_g = color1.g() + color2.g();
    m_b = color1.b() + color2.b();
    return *this;
}

Color &Color::addScalar(HIGH_PRECISION s)
{
    m_r += s;
    m_g += s;
    m_b += s;
    return *this;
}

Color &Color::sub(const Color &color)
{
    m_r -= color.r();
    m_g -= color.g();
    m_b -= color.b();
    return *this;
}

Color &Color::subScalar(HIGH_PRECISION s)
{
    m_r -= s;
    m_g -= s;
    m_b -= s;
    return *this;
}

Color &Color::multiply(const Color &color)
{
    m_r *= color.r();
    m_g *= color.g();
    m_b *= color.b();
    return *this;
}

Color &Color::multiplyScalar(HIGH_PRECISION scalar)
{
    m_r *= scalar;
    m_g *= scalar;
    m_b *= scalar;
    return *this;
}

Color &Color::divideScalar(HIGH_PRECISION scalar)
{
    return multiplyScalar(1.0f / scalar);
}

Color &Color::lerp(const Color &color, HIGH_PRECISION alpha)
{
    m_r += (color.r() - m_r) * alpha;
    m_g += (color.g() - m_g) * alpha;
    m_b += (color.b() - m_b) * alpha;
    return *this;
}

Color &Color::lerpHSL(const Color &color, HIGH_PRECISION alpha)
{
    Vector3 v1, v2;
    getHSL(v1);
    color.getHSL(v2);

    if (v1.y() == 0.0)
    {
        v2.setX(v1.x()); // No saturation, keep hue
    }

    auto resultH = v1.x();
    auto resultS = v1.y();
    auto resultL = v1.z();

    if (v2.y() == 0.0)
    {
        resultH = v2.x();
    }
    else
    {
        // Simple lerp for hue (needs improvement for circular nature)
        resultH = v1.x() + MathUtils::lerp(0.0f, v2.x() - v1.x(), alpha);
    }

    resultS = MathUtils::lerp(v1.y(), v2.y(), alpha);
    resultL = MathUtils::lerp(v1.z(), v2.z(), alpha);

    setHSL(resultH, resultS, resultL);
    return *this;
}

bool Color::equals(const Color &color, float epsilon) const
{
    return std::abs(m_r - color.r()) <= epsilon &&
           std::abs(m_g - color.g()) <= epsilon &&
           std::abs(m_b - color.b()) <= epsilon;
}

int Color::getHex() const
{
    return (int(m_r * 255) << 16) | (int(m_g * 255) << 8) | int(m_b * 255);
}

std::string Color::getHexString() const
{
    std::ostringstream ss;
    ss << "#" << std::hex << std::setw(6) << std::setfill('0') << getHex();
    return ss.str();
}

std::string Color::getStyle() const
{
    std::ostringstream ss;
    ss << "rgb(" << int(m_r * 255) << "," << int(m_g * 255) << "," << int(m_b * 255) << ")";
    return ss.str();
}

void Color::getHSL(Vector3 &result) const
{
    auto r = m_r, g = m_g, b = m_b;

    auto max = std::max({r, g, b});
    auto min = std::min({r, g, b});
    auto l = (max + min) / 2.0;
    auto h = 0.0;
    auto s = 0.0;

    if (max == min)
    {
        h = s = 0.0;
    }
    else
    {
        auto d = max - min;
        s = l > 0.5 ? d / (2.0 - max - min) : d / (max + min);

        if (max == r)
            h = (g - b) / d + (g < b ? 6.0 : 0.0);
        else if (max == g)
            h = (b - r) / d + 2.0;
        else
            h = (r - g) / d + 4.0;

        h /= 6.0;
    }

    result.setX(h);
    result.setY(s);
    result.setZ(l);
}

std::string Color::getHSL() const
{
    Vector3 hsl;
    getHSL(hsl);
    std::ostringstream ss;
    ss << "hsl(" << int(hsl.x() * 360) << "," << int(hsl.y() * 100) << "%," << int(hsl.z() * 100) << "%)";
    return ss.str();
}

Color &Color::fromVector3(const Vector3 &v)
{
    return set(v.x(), v.y(), v.z());
}

Color &Color::fromVector4(const Vector4 &v)
{
    return set(v.x(), v.y(), v.z());
}

void Color::toVector3(Vector3 &result) const
{
    result.set(m_r, m_g, m_b);
}

void Color::toVector4(Vector4 &result) const
{
    result.set(m_r, m_g, m_b, 1.0);
}

Color &Color::offsetHSL(HIGH_PRECISION h, HIGH_PRECISION s, HIGH_PRECISION l)
{
    Vector3 hsl;
    getHSL(hsl);
    hsl.setX(hsl.x() + h);
    hsl.setY(hsl.y() + s);
    hsl.setZ(hsl.z() + l);
    setHSL(hsl.x(), hsl.y(), hsl.z());
    return *this;
}

Color &Color::copyGammaToLinear(const Color &color, float gammaFactor)
{
    m_r = std::pow(color.r(), gammaFactor);
    m_g = std::pow(color.g(), gammaFactor);
    m_b = std::pow(color.b(), gammaFactor);
    return *this;
}

Color &Color::copyLinearToGamma(const Color &color, float gammaFactor)
{
    auto inverseGamma = 1.0f / gammaFactor;
    return copyGammaToLinear(color, inverseGamma);
}

Color &Color::convertGammaToLinear(float gammaFactor)
{
    m_r = std::pow(m_r, gammaFactor);
    m_g = std::pow(m_g, gammaFactor);
    m_b = std::pow(m_b, gammaFactor);
    return *this;
}

Color &Color::convertLinearToGamma(float gammaFactor)
{
    auto inverseGamma = 1.0f / gammaFactor;
    return convertGammaToLinear(inverseGamma);
}

// Operators
Color Color::operator+(const Color &c) const { return clone().add(c); }
Color Color::operator-(const Color &c) const { return clone().sub(c); }
Color Color::operator*(const Color &c) const { return clone().multiply(c); }
Color Color::operator*(HIGH_PRECISION s) const { return clone().multiplyScalar(s); }
Color Color::operator/(HIGH_PRECISION s) const { return clone().divideScalar(s); }

void Color::operator=(const Color &c) { copy(c); }
bool Color::operator==(const Color &c) const { return equals(c); }

HIGH_PRECISION Color::operator[](size_t index) const
{
    switch (index)
    {
    case 0: return m_r;
    case 1: return m_g;
    case 2: return m_b;
    default: return m_r;
    }
}
