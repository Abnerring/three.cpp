#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Port of the header scan in examples/jsm/loaders/GaussianSplatPLYLoader.js at the pinned
// revision. A Gaussian splat PLY is an ordinary PLY whose properties are named after the splat
// fields, and the spherical harmonics degree is not declared: it follows from how many f_rest
// properties the header lists. This slice reads the plain text header and works the degree out;
// the geometry itself comes from the PLY loader and the splat utilities.
class GaussianSplatPLYLoader {
  public:
    // How many f_rest components each degree holds.
    static constexpr int kDegreeComponents[4] = {0, 9, 24, 45};

    struct Header {
        bool isPly = false;
        // How many f_rest properties the header declares, and the degree that implies. A count
        // that matches no degree leaves the degree at zero.
        int restComponents = 0;
        int degree = 0;
        // The property names found, in the order they were declared.
        std::vector<std::string> properties;
        // The properties a splat PLY must declare but this one did not.
        std::vector<std::string> missing;
    };

    // Reads the header text up to the end of the property list. Only the text is examined, not
    // the parsed geometry, because the PLY reader creates an attribute for any declared custom
    // property even when the file has no such data.
    static Header scanHeader(const std::string& text);

    // The custom property mapping a PLY reader needs to expose the splat fields: each logical
    // field names the file properties that carry it. The rest band grows with the degree, so a
    // degree of one lists nine names and a degree of three lists forty five.
    static std::map<std::string, std::vector<std::string>> propertyMapping(int degree);

    // The five attribute streams a splat PLY must provide, each three or one components wide
    // except the rotation which is four.
    struct Attributes {
        std::vector<double> position;
        std::vector<double> scale;
        std::vector<double> rotation;
        std::vector<double> fDc;
        std::vector<double> opacity;
    };

    // The geometry the conversion produces: a centre and six covariance components per splat,
    // plus four colour bytes.
    struct Converted {
        std::vector<double> centers;
        // The utility writes floats, so the buffer is float rather than double.
        std::vector<float> covariances;
        std::vector<std::uint8_t> colors;
        std::size_t count = 0;
    };

    // Converts the attribute streams. The scale is stored as a logarithm, so it is raised by e
    // to the stored value; the rotation is stored as w x y z even though it is read by
    // component position, so the first component is w. Throws when a stream is missing, has
    // the wrong width, or disagrees on the count.
    static Converted convertGeometry(const Attributes& attributes);
};

