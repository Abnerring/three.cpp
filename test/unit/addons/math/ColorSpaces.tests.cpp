#include "catch.hpp"
#include "core/JSValue.h"
#include "math/ColorManagement.h"
#include "math/ColorSpaces.h"
#include "constants.h"

// The module object keeps the upstream export order and the identity sharing of the pinned
// examples/jsm/math/ColorSpaces.js; the differential probe compares the same fields against
// the real module.
namespace {
double element(const JSValue& matrix, std::size_t index) {
    return matrix.get("elements").get(std::to_string(index)).toNumber();
}
}

TEST_CASE("ColorSpaces exposes the pinned colour space names", "[colorspaces]") {
    REQUIRE(std::string(ColorSpaces::DisplayP3ColorSpace) == "display-p3");
    REQUIRE(std::string(ColorSpaces::LinearDisplayP3ColorSpace) == "display-p3-linear");
    REQUIRE(std::string(ColorSpaces::LinearRec2020ColorSpace) == "rec2020-linear");
    REQUIRE(std::string(ColorSpaces::ExtendedSRGBColorSpace) == "extended-srgb");
    const auto module = ColorSpaces::nativeColorSpaces();
    REQUIRE(module.keys() == std::vector<std::string>({"DisplayP3ColorSpace", "LinearDisplayP3ColorSpace",
        "DisplayP3ColorSpaceImpl", "LinearDisplayP3ColorSpaceImpl", "LinearRec2020ColorSpace",
        "LinearRec2020ColorSpaceImpl", "ExtendedSRGBColorSpace", "ExtendedSRGBColorSpaceImpl"}));
    REQUIRE(module.get("DisplayP3ColorSpace").string() == "display-p3");
    REQUIRE(module.get("ExtendedSRGBColorSpace").string() == "extended-srgb");
    // Repeated calls return the same realm object.
    REQUIRE(module.strictEquals(ColorSpaces::nativeColorSpaces()));
    REQUIRE(ColorSpaces::displayP3ColorSpaceImpl().strictEquals(module.get("DisplayP3ColorSpaceImpl")));
}

TEST_CASE("ColorSpaces implementations carry the pinned tables and matrices", "[colorspaces]") {
    const auto displayP3 = ColorSpaces::displayP3ColorSpaceImpl();
    const auto primaries = displayP3.get("primaries");
    REQUIRE(primaries.get("0").toNumber() == 0.680);
    REQUIRE(primaries.get("5").toNumber() == 0.060);
    const auto whitePoint = displayP3.get("whitePoint");
    REQUIRE(whitePoint.get("0").toNumber() == 0.3127);
    REQUIRE(whitePoint.get("1").toNumber() == 0.3290);
    const auto coefficients = displayP3.get("luminanceCoefficients");
    REQUIRE(coefficients.get("0").toNumber() == 0.2289);
    REQUIRE(coefficients.get("1").toNumber() == 0.6917);
    REQUIRE(coefficients.get("2").toNumber() == 0.0793);
    const auto toXYZ = displayP3.get("toXYZ");
    REQUIRE(element(toXYZ, 0) == 0.4865709);
    REQUIRE(element(toXYZ, 4) == 0.6917385);
    REQUIRE(element(toXYZ, 8) == 1.0439444);
    // Matrix3 stores its elements column-major, so elements[ 1 ] is n21 and elements[ 2 ] n31.
    const auto fromXYZ = displayP3.get("fromXYZ");
    REQUIRE(element(fromXYZ, 0) == 2.4934969);
    REQUIRE(element(fromXYZ, 1) == -0.8294890);
    REQUIRE(element(fromXYZ, 2) == 0.0358458);
    REQUIRE(element(fromXYZ, 3) == -0.9313836);
    REQUIRE(displayP3.get("outputColorSpaceConfig").get("drawingBufferColorSpace").string() == "display-p3");
    REQUIRE_FALSE(displayP3.hasOwn("workingColorSpaceConfig"));
    const auto rec2020 = ColorSpaces::linearRec2020ColorSpaceImpl();
    REQUIRE(element(rec2020.get("toXYZ"), 0) == 0.6369580);
    REQUIRE(element(rec2020.get("fromXYZ"), 8) == 0.9421031);
    REQUIRE(rec2020.get("luminanceCoefficients").get("0").toNumber() == 0.2627);
}

TEST_CASE("ColorSpaces transfer and spread identity follow the pinned module", "[colorspaces]") {
    const auto spaces = nativeColorManagement().get("spaces");
    const auto srgbTransfer = spaces.get(SRGBColorSpace).get("transfer");
    const auto linearTransfer = spaces.get(LinearSRGBColorSpace).get("transfer");
    // Display P3 uses the sRGB transfer, its linear variant the linear one.
    REQUIRE(ColorSpaces::displayP3ColorSpaceImpl().get("transfer").strictEquals(srgbTransfer));
    REQUIRE(ColorSpaces::linearDisplayP3ColorSpaceImpl().get("transfer").strictEquals(linearTransfer));
    REQUIRE(ColorSpaces::linearRec2020ColorSpaceImpl().get("transfer").strictEquals(linearTransfer));
    REQUIRE_FALSE(ColorSpaces::displayP3ColorSpaceImpl().get("transfer").strictEquals(linearTransfer));
    // The Display P3 matrices are distinct objects from the sRGB space ones.
    REQUIRE_FALSE(ColorSpaces::displayP3ColorSpaceImpl().get("toXYZ")
                      .strictEquals(spaces.get(SRGBColorSpace).get("toXYZ")));
    // Extended sRGB spreads the sRGB space, so every shared field keeps its identity, while
    // the output configuration is replaced.
    const auto extended = ColorSpaces::extendedSRGBColorSpaceImpl();
    const auto srgbSpace = spaces.get(SRGBColorSpace);
    for (const auto* key : {"primaries", "whitePoint", "transfer", "toXYZ", "fromXYZ", "luminanceCoefficients"})
        REQUIRE(extended.get(key).strictEquals(srgbSpace.get(key)));
    REQUIRE_FALSE(extended.get("outputColorSpaceConfig")
                      .strictEquals(srgbSpace.get("outputColorSpaceConfig")));
    REQUIRE(extended.get("outputColorSpaceConfig").get("drawingBufferColorSpace").string() == "srgb");
    REQUIRE(extended.get("outputColorSpaceConfig").get("toneMappingMode").string() == "extended");
    REQUIRE(extended.keys() == srgbSpace.keys());
}
