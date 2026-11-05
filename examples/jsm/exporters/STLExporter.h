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
// Current scope: the ascii flavour is implemented. The binary flavour needs the DataView byte layout
// and is not ported yet, so asking for it raises instead of silently producing ascii. Skinned meshes
// are also not handled; the pinned exporter applies the bone transform for them, this port only uses
// the geometry positions and the world matrix.
class STLExporter {
  public:
    // binary = false matches the pinned default of options.binary.
    std::string parse(const Object3D& scene, bool binary = false) const;
};
