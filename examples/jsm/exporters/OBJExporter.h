#pragma once

#include "core/Object3D.h"

#include <string>

// Native port of examples/jsm/exporters/OBJExporter.js from the pinned three.js revision
// (see upstream.json).
//
// Scope of this port: the mesh path. It writes the object name, an optional usemtl line from the
// material name, the transformed vertices, the uvs and the world space normals, then the faces with
// the pinned vertex/vt/vn index packing for both indexed and non indexed geometries.
//
// Not ported yet, therefore documented instead of silently skipped: the line path (Line and
// LineSegments records) and the points path (points with colour attributes). Those objects are
// ignored by this implementation.
class OBJExporter {
  public:
    std::string parse(const Object3D& object) const;
};
