#include "math/Color.h"
#include "core/BufferAttribute.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <regex>
#include <stdexcept>

Color::Color() : Color(1.0, 1.0, 1.0) {}

Color::Color(HIGH_PRECISION hex) : Color() { setHex(hex); }

Color::Color(HIGH_PRECISION r, HIGH_PRECISION g, HIGH_PRECISION b)
    : r(r), g(g), b(b)
{
}

Color::Color(int hex)
    : r(1.0), g(1.0), b(1.0)
{
    setHex(hex);
}

Color::Color(const std::string &colorString)
    : r(1.0), g(1.0), b(1.0)
{
    setStyle(colorString);
}

Color::~Color()
{
}

Color &Color::set(HIGH_PRECISION r, HIGH_PRECISION g, HIGH_PRECISION b)
{
    return setRGB(r, g, b);
}

Color &Color::setScalar(HIGH_PRECISION scalar)
{
    this->r = this->g = this->b = scalar;
    return *this;
}

Color &Color::setHex(HIGH_PRECISION hex, const std::string &colorSpace)
{
    const auto value = static_cast<uint32_t>(static_cast<int64_t>(std::floor(hex)));
    this->r = ((value >> 16) & 255) / 255.0; this->g = ((value >> 8) & 255) / 255.0; this->b = (value & 255) / 255.0;
    return ColorManagement::colorSpaceToWorking(*this, colorSpace);
}

Color &Color::setHSL(HIGH_PRECISION h, HIGH_PRECISION s, HIGH_PRECISION l, const std::string &colorSpace)
{
    h = MathUtils::euclideanModulo(h, 1);
    s = MathUtils::clamp(s, 0, 1);
    l = MathUtils::clamp(l, 0, 1);
    if (s == 0.0)
    {
        this->r = this->g = this->b = l;
    }
    else
    {
        auto hue2rgb = [](HIGH_PRECISION p, HIGH_PRECISION q, HIGH_PRECISION t) -> HIGH_PRECISION
        {
            if (t < 0.0) t += 1.0;
            if (t > 1.0) t -= 1.0;
            if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
            if (t < 1.0 / 2.0) return q;
            // The factor order of `p + ( q - p ) * 6 * ( 2 / 3 - t )` is part of the result:
            // multiplying by ( 2 / 3 - t ) first rounds one ulp away from the source.
            if (t < 2.0 / 3.0) return p + (q - p) * 6.0 * (2.0 / 3.0 - t);
            return p;
        };

        auto q = l <= 0.5 ? l * (1.0 + s) : l + s - l * s;
        auto p = 2.0 * l - q;

        this->r = hue2rgb(p, q, h + 1.0 / 3.0);
        this->g = hue2rgb(p, q, h);
        this->b = hue2rgb(p, q, h - 1.0 / 3.0);
    }

    return ColorManagement::colorSpaceToWorking(*this, colorSpace);
}

Color &Color::setColorName(const std::string &colorName, const std::string &colorSpace)
{
    const auto hex = getHexColorName(colorName);
    if (hex != -1) setHex(hex, colorSpace);
    return *this;
}

int Color::getHexColorName(const std::string &name)
{
    static const std::unordered_map<std::string, int> colors = {
        {"aliceblue", 0xF0F8FF},
        {"antiquewhite", 0xFAEBD7},
        {"aqua", 0x00FFFF},
        {"aquamarine", 0x7FFFD4},
        {"azure", 0xF0FFFF},
        {"beige", 0xF5F5DC},
        {"bisque", 0xFFE4C4},
        {"black", 0x000000},
        {"blanchedalmond", 0xFFEBCD},
        {"blue", 0x0000FF},
        {"blueviolet", 0x8A2BE2},
        {"brown", 0xA52A2A},
        {"burlywood", 0xDEB887},
        {"cadetblue", 0x5F9EA0},
        {"chartreuse", 0x7FFF00},
        {"chocolate", 0xD2691E},
        {"coral", 0xFF7F50},
        {"cornflowerblue", 0x6495ED},
        {"cornsilk", 0xFFF8DC},
        {"crimson", 0xDC143C},
        {"cyan", 0x00FFFF},
        {"darkblue", 0x00008B},
        {"darkcyan", 0x008B8B},
        {"darkgoldenrod", 0xB8860B},
        {"darkgray", 0xA9A9A9},
        {"darkgreen", 0x006400},
        {"darkgrey", 0xA9A9A9},
        {"darkkhaki", 0xBDB76B},
        {"darkmagenta", 0x8B008B},
        {"darkolivegreen", 0x556B2F},
        {"darkorange", 0xFF8C00},
        {"darkorchid", 0x9932CC},
        {"darkred", 0x8B0000},
        {"darksalmon", 0xE9967A},
        {"darkseagreen", 0x8FBC8F},
        {"darkslateblue", 0x483D8B},
        {"darkslategray", 0x2F4F4F},
        {"darkslategrey", 0x2F4F4F},
        {"darkturquoise", 0x00CED1},
        {"darkviolet", 0x9400D3},
        {"deeppink", 0xFF1493},
        {"deepskyblue", 0x00BFFF},
        {"dimgray", 0x696969},
        {"dimgrey", 0x696969},
        {"dodgerblue", 0x1E90FF},
        {"firebrick", 0xB22222},
        {"floralwhite", 0xFFFAF0},
        {"forestgreen", 0x228B22},
        {"fuchsia", 0xFF00FF},
        {"gainsboro", 0xDCDCDC},
        {"ghostwhite", 0xF8F8FF},
        {"gold", 0xFFD700},
        {"goldenrod", 0xDAA520},
        {"gray", 0x808080},
        {"green", 0x008000},
        {"greenyellow", 0xADFF2F},
        {"grey", 0x808080},
        {"honeydew", 0xF0FFF0},
        {"hotpink", 0xFF69B4},
        {"indianred", 0xCD5C5C},
        {"indigo", 0x4B0082},
        {"ivory", 0xFFFFF0},
        {"khaki", 0xF0E68C},
        {"lavender", 0xE6E6FA},
        {"lavenderblush", 0xFFF0F5},
        {"lawngreen", 0x7CFC00},
        {"lemonchiffon", 0xFFFACD},
        {"lightblue", 0xADD8E6},
        {"lightcoral", 0xF08080},
        {"lightcyan", 0xE0FFFF},
        {"lightgoldenrodyellow", 0xFAFAD2},
        {"lightgray", 0xD3D3D3},
        {"lightgreen", 0x90EE90},
        {"lightgrey", 0xD3D3D3},
        {"lightpink", 0xFFB6C1},
        {"lightsalmon", 0xFFA07A},
        {"lightseagreen", 0x20B2AA},
        {"lightskyblue", 0x87CEFA},
        {"lightslategray", 0x778899},
        {"lightslategrey", 0x778899},
        {"lightsteelblue", 0xB0C4DE},
        {"lightyellow", 0xFFFFE0},
        {"lime", 0x00FF00},
        {"limegreen", 0x32CD32},
        {"linen", 0xFAF0E6},
        {"magenta", 0xFF00FF},
        {"maroon", 0x800000},
        {"mediumaquamarine", 0x66CDAA},
        {"mediumblue", 0x0000CD},
        {"mediumorchid", 0xBA55D3},
        {"mediumpurple", 0x9370DB},
        {"mediumseagreen", 0x3CB371},
        {"mediumslateblue", 0x7B68EE},
        {"mediumspringgreen", 0x00FA9A},
        {"mediumturquoise", 0x48D1CC},
        {"mediumvioletred", 0xC71585},
        {"midnightblue", 0x191970},
        {"mintcream", 0xF5FFFA},
        {"mistyrose", 0xFFE4E1},
        {"moccasin", 0xFFE4B5},
        {"navajowhite", 0xFFDEAD},
        {"navy", 0x000080},
        {"oldlace", 0xFDF5E6},
        {"olive", 0x808000},
        {"olivedrab", 0x6B8E23},
        {"orange", 0xFFA500},
        {"orangered", 0xFF4500},
        {"orchid", 0xDA70D6},
        {"palegoldenrod", 0xEEE8AA},
        {"palegreen", 0x98FB98},
        {"paleturquoise", 0xAFEEEE},
        {"palevioletred", 0xDB7093},
        {"papayawhip", 0xFFEFD5},
        {"peachpuff", 0xFFDAB9},
        {"peru", 0xCD853F},
        {"pink", 0xFFC0CB},
        {"plum", 0xDDA0DD},
        {"powderblue", 0xB0E0E6},
        {"purple", 0x800080},
        {"rebeccapurple", 0x663399},
        {"red", 0xFF0000},
        {"rosybrown", 0xBC8F8F},
        {"royalblue", 0x4169E1},
        {"saddlebrown", 0x8B4513},
        {"salmon", 0xFA8072},
        {"sandybrown", 0xF4A460},
        {"seagreen", 0x2E8B57},
        {"seashell", 0xFFF5EE},
        {"sienna", 0xA0522D},
        {"silver", 0xC0C0C0},
        {"skyblue", 0x87CEEB},
        {"slateblue", 0x6A5ACD},
        {"slategray", 0x708090},
        {"slategrey", 0x708090},
        {"snow", 0xFFFAFA},
        {"springgreen", 0x00FF7F},
        {"steelblue", 0x4682B4},
        {"tan", 0xD2B48C},
        {"teal", 0x008080},
        {"thistle", 0xD8BFD8},
        {"tomato", 0xFF6347},
        {"turquoise", 0x40E0D0},
        {"violet", 0xEE82EE},
        {"wheat", 0xF5DEB3},
        {"white", 0xFFFFFF},
        {"whitesmoke", 0xF5F5F5},
        {"yellow", 0xFFFF00},
        {"yellowgreen", 0x9ACD32}
    };
    const auto it = colors.find(name);
    return it != colors.end() ? it->second : -1;
}

Color Color::clone() const
{
    return Color(this->r, this->g, this->b);
}

Color &Color::copy(const Color &color)
{
    this->r = color.r;
    this->g = color.g;
    this->b = color.b;
    return *this;
}

Color &Color::add(const Color &color)
{
    this->r += color.r;
    this->g += color.g;
    this->b += color.b;
    return *this;
}

Color &Color::addColors(const Color &color1, const Color &color2)
{
    this->r = color1.r + color2.r;
    this->g = color1.g + color2.g;
    this->b = color1.b + color2.b;
    return *this;
}

Color &Color::addScalar(HIGH_PRECISION s)
{
    this->r += s;
    this->g += s;
    this->b += s;
    return *this;
}

Color &Color::sub(const Color &color)
{
    this->r = MathUtils::max(0.0, this->r - color.r); this->g = MathUtils::max(0.0, this->g - color.g); this->b = MathUtils::max(0.0, this->b - color.b);
    return *this;
}

Color &Color::subScalar(HIGH_PRECISION s)
{
    this->r -= s;
    this->g -= s;
    this->b -= s;
    return *this;
}

Color &Color::multiply(const Color &color)
{
    this->r *= color.r;
    this->g *= color.g;
    this->b *= color.b;
    return *this;
}

Color &Color::multiplyScalar(HIGH_PRECISION scalar)
{
    this->r *= scalar;
    this->g *= scalar;
    this->b *= scalar;
    return *this;
}

Color &Color::divideScalar(HIGH_PRECISION scalar)
{
    return multiplyScalar(1.0f / scalar);
}

Color &Color::lerp(const Color &color, HIGH_PRECISION alpha)
{
    this->r += (color.r - this->r) * alpha;
    this->g += (color.g - this->g) * alpha;
    this->b += (color.b - this->b) * alpha;
    return *this;
}

Color &Color::lerpHSL(const Color &color, HIGH_PRECISION alpha)
{
    Vector3 a, b;
    getHSL(a); color.getHSL(b);
    return setHSL(MathUtils::lerp(a.x, b.x, alpha), MathUtils::lerp(a.y, b.y, alpha), MathUtils::lerp(a.z, b.z, alpha));
}

bool Color::equals(const Color &color, HIGH_PRECISION epsilon) const
{
    if (epsilon == 0) return this->r == color.r && this->g == color.g && this->b == color.b;
    return std::abs(this->r - color.r) <= epsilon && std::abs(this->g - color.g) <= epsilon && std::abs(this->b - color.b) <= epsilon;
}

int Color::getHex(const std::string &colorSpace) const
{
    auto color = clone();
    ColorManagement::workingToColorSpace(color, colorSpace);
    return static_cast<int>(MathUtils::round(MathUtils::clamp(color.r * 255, 0, 255))) * 65536 + static_cast<int>(MathUtils::round(MathUtils::clamp(color.g * 255, 0, 255))) * 256 + static_cast<int>(MathUtils::round(MathUtils::clamp(color.b * 255, 0, 255)));
}

std::string Color::getHexString(const std::string &colorSpace) const
{
    std::ostringstream ss;
    ss << std::hex << std::setw(6) << std::setfill('0') << getHex(colorSpace);
    return ss.str();
}

std::string Color::getStyle(const std::string &colorSpace) const
{
    auto color = clone();
    ColorManagement::workingToColorSpace(color, colorSpace);
    std::ostringstream ss;
    if (colorSpace != SRGBColorSpace) ss << "color(" << colorSpace << " " << std::fixed << std::setprecision(3) << color.r << " " << color.g << " " << color.b << ")";
    else ss << "rgb(" << MathUtils::round(color.r * 255) << "," << MathUtils::round(color.g * 255) << "," << MathUtils::round(color.b * 255) << ")";
    return ss.str();
}

Vector3 &Color::getHSL(Vector3 &result, const std::string &colorSpace) const
{
    auto color = clone();
    ColorManagement::workingToColorSpace(color, colorSpace);
    auto r = color.r, g = color.g, b = color.b;

    auto max = MathUtils::max(MathUtils::max(r, g), b);
    auto min = MathUtils::min(MathUtils::min(r, g), b);
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
    return result;
}

std::string Color::getHSL() const
{
    Vector3 hsl;
    getHSL(hsl);
    std::ostringstream ss;
    ss << "hsl(" << int(hsl.x * 360) << "," << int(hsl.y * 100) << "%," << int(hsl.z * 100) << "%)";
    return ss.str();
}

Color &Color::fromVector3(const Vector3 &v)
{
    return set(v.x, v.y, v.z);
}

Color &Color::fromVector4(const Vector4 &v)
{
    return set(v.x, v.y, v.z);
}

void Color::toVector3(Vector3 &result) const
{
    result.set(this->r, this->g, this->b);
}

void Color::toVector4(Vector4 &result) const
{
    result.set(this->r, this->g, this->b, 1.0);
}

Color &Color::offsetHSL(HIGH_PRECISION h, HIGH_PRECISION s, HIGH_PRECISION l)
{
    Vector3 hsl;
    getHSL(hsl);
    hsl.setX(hsl.x + h);
    hsl.setY(hsl.y + s);
    hsl.setZ(hsl.z + l);
    setHSL(hsl.x, hsl.y, hsl.z);
    return *this;
}

Color &Color::copyGammaToLinear(const Color &color, HIGH_PRECISION gammaFactor)
{
    this->r = std::pow(color.r, gammaFactor);
    this->g = std::pow(color.g, gammaFactor);
    this->b = std::pow(color.b, gammaFactor);
    return *this;
}

Color &Color::copyLinearToGamma(const Color &color, HIGH_PRECISION gammaFactor)
{
    auto inverseGamma = 1.0f / gammaFactor;
    return copyGammaToLinear(color, inverseGamma);
}

Color &Color::convertGammaToLinear(HIGH_PRECISION gammaFactor)
{
    this->r = std::pow(this->r, gammaFactor);
    this->g = std::pow(this->g, gammaFactor);
    this->b = std::pow(this->b, gammaFactor);
    return *this;
}

Color &Color::convertLinearToGamma(HIGH_PRECISION gammaFactor)
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

Color &Color::operator=(const Color &c) {
    return copy(c);
}
bool Color::operator==(const Color &c) const { return equals(c); }

HIGH_PRECISION Color::operator[](size_t index) const
{
    switch (index)
    {
    case 0: return this->r;
    case 1: return this->g;
    case 2: return this->b;
    default: return this->r;
    }
}

Color &Color::set(int hex) { return setHex(hex); }
Color &Color::set(const std::string &style) { return setStyle(style); }
Color &Color::set(const Color &color) { return copy(color); }
Color &Color::setRGB(HIGH_PRECISION r, HIGH_PRECISION g, HIGH_PRECISION b, const std::string &colorSpace)
{
    this->r = r; this->g = g; this->b = b;
    return ColorManagement::colorSpaceToWorking(*this, colorSpace);
}
Color &Color::copySRGBToLinear(const Color &color)
{
    this->r = SRGBToLinear(color.r); this->g = SRGBToLinear(color.g); this->b = SRGBToLinear(color.b);
    return *this;
}
Color &Color::copyLinearToSRGB(const Color &color)
{
    this->r = LinearToSRGB(color.r); this->g = LinearToSRGB(color.g); this->b = LinearToSRGB(color.b);
    return *this;
}
Color &Color::convertSRGBToLinear() { return copySRGBToLinear(*this); }
Color &Color::convertLinearToSRGB() { return copyLinearToSRGB(*this); }
Color &Color::getRGB(Color &result, const std::string &colorSpace) const
{
    result.copy(*this);
    return ColorManagement::workingToColorSpace(result, colorSpace);
}
Color &Color::lerpColors(const Color &a, const Color &b, HIGH_PRECISION alpha)
{
    this->r = a.r + (b.r - a.r) * alpha; this->g = a.g + (b.g - a.g) * alpha; this->b = a.b + (b.b - a.b) * alpha;
    return *this;
}
Color &Color::setFromVector3(const Vector3 &v) { return fromVector3(v); }
Color &Color::fromArray(const std::vector<HIGH_PRECISION> &array, size_t offset)
{
    this->r = array.at(offset); this->g = array.at(offset + 1); this->b = array.at(offset + 2);
    return *this;
}
std::vector<HIGH_PRECISION> &Color::toArray(std::vector<HIGH_PRECISION> &array, size_t offset) const
{
    if (array.size() < offset + 3) array.resize(offset + 3);
    array[offset] = this->r; array[offset + 1] = this->g; array[offset + 2] = this->b;
    return array;
}
std::vector<HIGH_PRECISION> Color::toArray() const { return { this->r, this->g, this->b }; }
int Color::toJSON() const { return getHex(); }
Color &Color::setStyle(const std::string &style, const std::string &colorSpace)
{
    std::smatch match;
    static const std::regex model(R"(^([a-zA-Z0-9_]+)\(([^)]*)\))");
    static const std::regex rgb(R"(^\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*(\d*\.?\d+)\s*)?$)");
    static const std::regex percentage(R"(^\s*(\d*\.?\d+)%\s*,\s*(\d*\.?\d+)%\s*,\s*(\d*\.?\d+)%\s*(?:,\s*(\d*\.?\d+)\s*)?$)");
    static const std::regex hsl(R"(^\s*([+-]?\d*\.?\d+)\s*,\s*(\d*\.?\d+)%\s*,\s*(\d*\.?\d+)%\s*(?:,\s*(\d*\.?\d+)\s*)?$)");
    static const std::regex hex(R"(^#([A-Fa-f\d]+)$)");
    if (std::regex_search(style, match, model)) {
        const auto name = match[1].str(), components = match[2].str();
        if (name == "rgb" || name == "rgba") {
            if (std::regex_match(components, match, rgb)) return setRGB(std::min(255.0, std::stod(match[1])) / 255, std::min(255.0, std::stod(match[2])) / 255, std::min(255.0, std::stod(match[3])) / 255, colorSpace);
            if (std::regex_match(components, match, percentage)) return setRGB(std::min(100.0, std::stod(match[1])) / 100, std::min(100.0, std::stod(match[2])) / 100, std::min(100.0, std::stod(match[3])) / 100, colorSpace);
        } else if (name == "hsl" || name == "hsla") {
            if (std::regex_match(components, match, hsl)) return setHSL(std::stod(match[1]) / 360, std::stod(match[2]) / 100, std::stod(match[3]) / 100, colorSpace);
        }
    } else if (std::regex_match(style, match, hex)) {
        const auto value = match[1].str();
        if (value.size() == 6) return setHex(std::stoi(value, nullptr, 16), colorSpace);
        if (value.size() == 3) return setRGB(std::stoi(value.substr(0, 1), nullptr, 16) / 15.0, std::stoi(value.substr(1, 1), nullptr, 16) / 15.0, std::stoi(value.substr(2, 1), nullptr, 16) / 15.0, colorSpace);
    } else if (!style.empty()) return setColorName(style, colorSpace);
    return *this;
}

Color &Color::fromBufferAttribute(const BufferAttribute &attribute, double index)
{
    r = attribute.getX(index);
    g = attribute.getY(index);
    b = attribute.getZ(index);
    return *this;
}

Color &Color::applyMatrix3(const Matrix3 &m)
{
    const auto e = m.elements();
    const auto r = this->r, g = this->g, b = this->b;
    this->r = e[0] * r + e[3] * g + e[6] * b;
    this->g = e[1] * r + e[4] * g + e[7] * b;
    this->b = e[2] * r + e[5] * g + e[8] * b;
    return *this;
}
