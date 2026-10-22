#pragma once
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
};
