#pragma once

#include "core/Object3D.h"

#include <string>

// Native port of examples/jsm/exporters/STLExporter.js from the pinned three.js revision
// (see upstream.json).
//
// The exporter walks a scene, collects its meshes and writes one facet per triangle of the position
// attribute, either raw or through the index, after transforming the vertices by the object world
// matrix. Numbers are formatted with the port's JavaScript number to string algorithm, which is what
// the pinned string concatenation does.
//
// Both flavours are implemented: the ascii document and the binary little endian layout with the
// eight byte header, the triangle count, and per triangle twelve float32 values plus the attribute
// byte count. Skinned meshes are not handled; the pinned exporter applies the bone transform for
// them, this port only uses the geometry positions and the world matrix.
class STLExporter {
  public:
    // binary = false matches the pinned default of options.binary.
    std::string parse(const Object3D& scene, bool binary = false) const;
};
