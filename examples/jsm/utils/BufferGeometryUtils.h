#pragma once
// Native port of the pinned examples/jsm/utils/BufferGeometryUtils.js
// (three.js r187dev, commit 11738a9d55e502fb484cefffcea3a66dc44e9fe9; see upstream.json).
//
// Every export of the source module is provided except computeMikkTSpaceTangents(), which
// needs the third-party MikkTSpace WebAssembly library this project does not ship: that
// entry point throws explicitly instead of returning geometry without tangents.
//
// C++ adaptations of the source module (no behaviour change):
//
//   * Upstream `null` results ("log to the console and give up") are `nullptr` here and the
//     same message text is written to std::cerr. The TypeError/JSON errors the source module
//     raises while reading missing attributes are preserved as JSTypeError/JSRangeError, so a
//     missing `position` attribute still fails loudly instead of silently welding nothing.
//   * A JavaScript `undefined` mesh attribute slot is a null GeometryPtr/AttributePtr.
//   * The source iterates ordinary objects with `for... in`; OrderedMap already reproduces
//     that key order, so attribute and morph-attribute maps keep the upstream ordering.
//   * Buffers use the managed InterleavedBuffer/InterleavedBufferAttribute classes, which
//     keep the whole stride of the source buffer as the source does.
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/TypedArray.h"
#include "core/VertexAttribute.h"
#include <memory>
#include <unordered_map>
#include <vector>

class Object3D;

namespace BufferGeometryUtils {

using GeometryPtr = std::shared_ptr<BufferGeometry>;
using AttributePtr = std::shared_ptr<VertexAttribute>;
using GeometryList = std::vector<GeometryPtr>;
using AttributeList = std::vector<AttributePtr>;

// Upstream: computeMikkTSpaceTangents( geometry, MikkTSpace, negateSign = true ).
// MikkTSpace is an external WebAssembly library that is not available in this port, so the
// call always throws. It never returns a geometry with missing or wrong tangent data.
GeometryPtr computeMikkTSpaceTangents(const GeometryPtr& geometry, bool negateSign = true);

// Merges a set of geometries. Returns nullptr (upstream `null`) when the geometries are not
// compatible; every failure prints the exact upstream console.error text to std::cerr.
GeometryPtr mergeGeometries(const GeometryList& geometries, bool useGroups = false);

// Merges compatible attributes. InterleavedBufferAttribute input is supported and is copied
// component by component, exactly like the source. Returns nullptr on failure. An empty list
// is the upstream `new undefined( 0 )` TypeError.
AttributePtr mergeAttributes(const AttributeList& attributes);

// Deep clone: interleaved input is de-interleaved, instanced input keeps meshPerAttribute,
// anything else becomes a plain BufferAttribute with a copied array.
AttributePtr deepCloneAttribute(const AttributePtr& attribute);

// Interleaves attributes sharing one InterleavedBuffer. An empty result is the upstream
// `null` return after the "different types cannot be interleaved" console.error.
std::vector<AttributePtr> interleaveAttributes(const AttributeList& attributes);

// Returns a non-interleaved copy of the given interleaved attribute.
AttributePtr deinterleaveAttribute(const AttributePtr& attribute);

// Replaces every interleaved attribute of the geometry (including morph attributes) by a
// de-interleaved copy. Attributes sharing one InterleavedBuffer share one copy.
void deinterleaveGeometry(BufferGeometry& geometry);

// Estimated bytes of all attributes plus the index.
double estimateBytesUsed(const BufferGeometry& geometry);

// Returns a new geometry whose vertices are welded within the tolerance. Like the source, the
// tolerance is raised to Number.EPSILON when it is smaller, and a geometry without a position
// attribute (and without an index) raises the upstream TypeError.
GeometryPtr mergeVertices(const GeometryPtr& geometry, double tolerance = 1e-4);

// Rewrites the index so the geometry draws triangles, and clears the groups.
GeometryPtr toTrianglesDrawMode(const GeometryPtr& geometry, double drawMode);

// Sorts and merges the geometry groups by material index.
GeometryPtr mergeGroups(const GeometryPtr& geometry);

// Smooth normals with a crease threshold. A non-indexed input geometry is modified in place,
// an indexed one is de-indexed into a new geometry first, as upstream does.
GeometryPtr toCreasedNormals(const GeometryPtr& geometry, double creaseAngle = 1.0471975511965976);

// The source returns
//   { positionAttribute, normalAttribute, morphedPositionAttribute, morphedNormalAttribute },
// so the last two members keep those upstream names.
struct MorphedAttributes {
    AttributePtr positionAttribute, normalAttribute, morphedPositionAttribute, morphedNormalAttribute;
};

// Morphed position/normal attributes of a Mesh/Line/Points object. Null members mean the
// geometry did not provide that attribute, like the upstream `undefined` fields.
MorphedAttributes computeMorphedAttributes(const std::shared_ptr<Object3D>& object);

// Copies attribute entries in the order given by `sourceIndices`, preserving the attribute
// type and metadata. `sourceIndices` carries the upstream plain index array: entry i is
// read with getX( i ) and the count is the array length. `buffers` is the upstream
// `Map< InterleavedBuffer, InterleavedBuffer >` reuse cache: it must be the same map for
// every attribute of one pass, and the outer key identifies the source buffer (for
// interleaved attributes: the identity of the shared backing array, so all attributes that
// view one InterleavedBuffer share one copied buffer). The created interleaved buffers are
// kept alive by the returned attributes, which is why an entry expires when the attributes
// that used it are released.
AttributePtr remapAttribute(const AttributePtr& attribute, const AttributePtr& sourceIndices,
                            std::unordered_map<const VertexAttribute*, std::unordered_map<double, double>>& buffers);

} // namespace BufferGeometryUtils
