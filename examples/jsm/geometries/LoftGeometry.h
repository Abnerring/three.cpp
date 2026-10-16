#pragma once
#include "core/BufferGeometry.h"
#include "math/Vector3.h"
#include <vector>

// Native port of the pinned examples/jsm/geometries/LoftGeometry.js.
//
// `sections` is an array of equally sized point rings; `options.closed` repeats the first
// point of every section so the surface wraps with continuous uvs. The upstream
// `console.error` guards return early, leaving a geometry without attributes, so the native
// constructor does the same. Like the pinned class, copy() keeps the source parameters.
class LoftGeometry : public BufferGeometry {
  public:
    using Section = std::vector<Vector3>;
    struct Options {
        bool closed = true;
        bool capStart = false;
        bool capEnd = false;
    };
    struct Parameters {
        std::vector<Section> sections;
        bool closed = true, capStart = false, capEnd = false;
    };
    Parameters parameters;

    explicit LoftGeometry(std::vector<Section> sections = {}, Options options = {});
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;

  private:
    // The cap generation of the pinned constructor, which closes over the buffers.
    void generateCap(double sectionIndex, double rows, double columns, const std::vector<Section>& sections,
                     std::vector<double>& vertices, std::vector<double>& uvs, std::vector<std::uint32_t>& indices);
};
