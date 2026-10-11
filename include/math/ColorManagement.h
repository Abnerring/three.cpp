#ifndef THREE_COLOR_MANAGEMENT_H
#define THREE_COLOR_MANAGEMENT_H
#include "common/BasicType.h"
#include <array>
#include <string>
class Color;
namespace ColorManagement {
inline bool enabled = true;
inline std::string workingColorSpace = "srgb-linear";
Color& convert(Color& color, const std::string& sourceColorSpace,
               const std::string& targetColorSpace);
Color& colorSpaceToWorking(Color& color, const std::string& sourceColorSpace);
Color& workingToColorSpace(Color& color, const std::string& targetColorSpace);
inline constexpr std::array<HIGH_PRECISION, 3> luminanceCoefficients{0.2126, 0.7152, 0.0722};
} // namespace ColorManagement
inline HIGH_PRECISION SRGBToLinear(HIGH_PRECISION c) {
    return c < 0.04045 ? c * 0.0773993808 : std::pow(c * 0.9478672986 + 0.0521327014, 2.4);
}
inline HIGH_PRECISION LinearToSRGB(HIGH_PRECISION c) {
    return c < 0.0031308 ? c * 12.92 : 1.055 * std::pow(c, 0.41666) - 0.055;
}
#endif
