// Direct port of examples/jsm/utils/BufferGeometryUtils.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See repository LICENSE.
//
// The pinned revision is the single source of truth for every branch below: the evaluation
// order of the arithmetic, the ordering of the merged output, the messages written on the
// failure paths and the `null`/`undefined` results are all kept as written upstream.
#include "utils/BufferGeometryUtils.h"
#include "math/MathUtils.h"
#include "core/InstancedBufferAttribute.h"
#include "math/MathUtils.h"
#include "core/InstancedInterleavedBuffer.h"
#include "math/MathUtils.h"
#include "core/InterleavedBuffer.h"
#include "math/MathUtils.h"
#include "core/InterleavedBufferAttribute.h"
#include "math/MathUtils.h"
#include "core/NativeObject.h"
#include "math/MathUtils.h"
#include "core/Object3D.h"
#include "math/MathUtils.h"
#include "core/SharedArray.h"
#include "math/MathUtils.h"
#include "math/Vector3.h"
#include "math/MathUtils.h"
#include "objects/Line.h"
#include "math/MathUtils.h"
#include "objects/Mesh.h"
#include "math/MathUtils.h"
#include "objects/Points.h"
#include "math/MathUtils.h"
#include "objects/SkinnedMesh.h"
#include "math/MathUtils.h"
#include "objects/Sprite.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace BufferGeometryUtils {
namespace {

using Buffers = std::unordered_map<const VertexAttribute*, std::unordered_map<double, double>>;

// ---------------------------------------------------------------------------
// JavaScript numeric conversions used by the module.
// ---------------------------------------------------------------------------

// ToInt32: NaN and infinities become zero, other values wrap modulo 2^32.
std::int32_t toInt32(double value) {
    if (!std::isfinite(value) || value == 0) return 0;
    constexpr double modulus = 4294967296.0;
    double wrapped = std::fmod(std::trunc(value), modulus);
    if (wrapped < 0) wrapped += modulus;
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(wrapped));
}

// Math.imul applies ToInt32 to both operands and multiplies with 32 bit wrapping.
double imul(double left, double right) {
    const auto product = static_cast<std::uint32_t>(toInt32(left)) *
                         static_cast<std::uint32_t>(toInt32(right));
    return double(static_cast<std::int32_t>(product));
}

double bitXor(double left, double right) { return double(toInt32(left) ^ toInt32(right)); }

double bitAnd(double left, double right) { return double(toInt32(left) & toInt32(right)); }

// `tableSize <<= 1`: ToInt32 of the current value, shifted as a signed 32 bit integer.
double shiftLeft(std::int32_t value) {
    return double(static_cast<std::int32_t>(static_cast<std::uint32_t>(value) << 1));
}

// `${ value }`, the JavaScript Number to String conversion used by the mergeVertices hash.
std::string numberString(double value) { return JSValue(value).toString(); }

// ---------------------------------------------------------------------------
// Typed array views. An attribute array is a TypedArray or, for an interleaved data buffer
// that was built from a plain Array, an ordinary array; InterpolantArray models both and
// shares its typed array identity with the storage of a BufferAttribute.
// ---------------------------------------------------------------------------

InterpolantArray attributeArray(const AttributePtr& attribute) { return attribute->getArray(); }

double arrayKind(const InterpolantArray& array) { return double(array.storage().index()); }
double arrayLengthOf(const InterpolantArray& array) { return double(array.size()); }

double arrayBytesPerElement(const InterpolantArray& array) {
    return std::visit([](const auto& values) -> double {
        using T = std::decay_t<decltype(values)>;
        // A plain Array has no BYTES_PER_ELEMENT, and the product upstream becomes NaN.
        if constexpr (std::is_same_v<T, JSValue>) return std::numeric_limits<double>::quiet_NaN();
        else return double(T::BYTES_PER_ELEMENT);
    }, array.storage());
}

// `array[ index ]`: an invalid or missing entry is undefined, which ToNumber turns into NaN.
double arrayRead(const InterpolantArray& array, double index) { return array.get(index).toNumber(); }

// `array[ index ] = value`: typed array stores convert the value, ordinary arrays keep it.
void arrayWrite(InterpolantArray& array, double index, double value) {
    array.set(index, JSValue(value));
}

// `new array.constructor( length )`.
InterpolantArray allocateArray(const InterpolantArray& like, double length) { return like.allocate(length); }

// Calls `factory( typedArray )` with the concrete typed array of `array`. A plain Array becomes
// the Float32Array that the BufferAttribute constructor creates upstream.
template <class Factory>
auto withTypedArray(const InterpolantArray& array, Factory factory)
    -> decltype(factory(std::declval<const Float32Array&>())) {
    if (const auto* ordinary = std::get_if<JSValue>(&array.storage())) {
        Float32Array converted(ordinary->length());
        for (double i = 0, length = double(converted.size()); i < length; ++i)
            converted.setIndex(i, ordinary->get(JSValue(i).toString()).toNumber());
        return factory(converted);
    }
    return std::visit(
        [&](const auto& values) -> decltype(factory(std::declval<const Float32Array&>())) {
            using T = std::decay_t<decltype(values)>;
            if constexpr (std::is_same_v<T, JSValue>)
                throw JSTypeError("BufferAttribute array must be a TypedArray");
            else return factory(values);
        },
        array.storage());
}

// `new BufferAttribute( array, itemSize, normalized )`.
std::shared_ptr<BufferAttribute> makeBufferAttribute(const InterpolantArray& array, double itemSize,
                                                     bool normalized) {
    return withTypedArray(array, [&](const auto& values) -> std::shared_ptr<BufferAttribute> {
        return std::make_shared<BufferAttribute>(values, itemSize, normalized);
    });
}

// `array.set( source, offset )` for the mergeAttributes copy.
void arrayCopyInto(InterpolantArray& destination, double offset, const InterpolantArray& source) {
    // An ordinary Array has no `set` method.
    if (std::holds_alternative<JSValue>(destination.storage()))
        throw JSTypeError("array.set is not a function");
    std::visit([&](auto& values) {
        using T = std::decay_t<decltype(values)>;
        if constexpr (!std::is_same_v<T, JSValue>) {
            if (const auto* typed = std::get_if<T>(&source.storage())) values.set(*typed, offset);
            else
                for (double i = 0, length = arrayLengthOf(source); i < length; ++i)
                    values.setIndex(offset + i, arrayRead(source, i));
        }
    }, destination.storage());
}

// The identity of the shared backing store, used to recognize two attributes that view the
// same interleaved buffer.
const void* arrayIdentity(const InterpolantArray& array) {
    return std::visit([](const auto& values) -> const void* {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, JSValue>) return nullptr;
        else return values.identity();
    }, array.storage());
}

// `new attribute.constructor( array, itemSize, normalized, meshPerAttribute )`.
std::shared_ptr<BufferAttribute> makeAttributeLike(const AttributePtr& source,
                                                   const InterpolantArray& array, double itemSize,
                                                   bool normalized) {
    if (const auto* instanced = dynamic_cast<const InstancedBufferAttribute*>(source.get()))
        return withTypedArray(array, [&](const auto& values) -> std::shared_ptr<BufferAttribute> {
            return std::make_shared<InstancedBufferAttribute>(values, itemSize, normalized,
                                                              instanced->meshPerAttribute);
        });
    if (dynamic_cast<const Float16BufferAttribute*>(source.get()) != nullptr)
        return withTypedArray(array, [&](const auto& values) -> std::shared_ptr<BufferAttribute> {
            return std::make_shared<Float16BufferAttribute>(values, itemSize, normalized);
        });
    return makeBufferAttribute(array, itemSize, normalized);
}

// ---------------------------------------------------------------------------
// Attribute protocols shared by both attribute classes.
// ---------------------------------------------------------------------------

bool isInterleaved(const AttributePtr& attribute) {
    return attribute->getProperty("isInterleavedBufferAttribute").toBoolean();
}

// `attribute[ getters[ k ] ]( index )` with the upstream getters array.
double attributeComponent(const AttributePtr& attribute, double index, double component) {
    switch (static_cast<int>(component)) {
    case 0: return attribute->getX(index);
    case 1: return attribute->getY(index);
    case 2: return attribute->getZ(index);
    case 3: return attribute->getW(index);
    default: break;
    }
    // The upstream getters/setters arrays only cover four components; a larger item size reads
    // `attribute[ undefined ]( index )` and throws.
    throw JSTypeError("attribute[getters[k]] is not a function");
}

// `attribute.getComponent( index, component )` for the mergeAttributes interleaved path.
double attributeComponentByIndex(const AttributePtr& attribute, double index, double component) {
    if (const auto* buffer = dynamic_cast<const BufferAttribute*>(attribute.get()))
        return buffer->getComponent(index, component);
    if (const auto* interleaved = dynamic_cast<const InterleavedBufferAttribute*>(attribute.get()))
        return interleaved->getComponent(JSValue(index), JSValue(component)).toNumber();
    throw JSTypeError("attribute.getComponent is not a function");
}

// `geometry.setIndex( arrayOfNumbers )`: the primitive restart rule picks the storage type and
// the typed array constructor converts every entry.
void setIndexFromNumbers(BufferGeometry& geometry, const std::vector<double>& values) {
    bool needsUint32 = false;
    for (auto it = values.rbegin(); it != values.rend(); ++it)
        if (*it >= 65535) {
            needsUint32 = true;
            break;
        }
    if (needsUint32) {
        Uint32Array array(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) array.setIndex(double(i), values[i]);
        geometry.setIndex(std::make_shared<Uint32BufferAttribute>(array, 1.));
    } else {
        Uint16Array array(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) array.setIndex(double(i), values[i]);
        geometry.setIndex(std::make_shared<Uint16BufferAttribute>(array, 1.));
    }
}

// `sourceIndices[ i ]`: the fixed signature carries the upstream plain index array as an
// attribute, so entry i is getX( i ) and the array length is the attribute count.
double sourceIndexValue(const AttributePtr& sourceIndices, double index) {
    return sourceIndices->getX(index);
}

// ---------------------------------------------------------------------------
// Remapped interleaved buffers.
// ---------------------------------------------------------------------------

// remapAttribute() reuses one copied InterleavedBuffer for every attribute that views the same
// source buffer, like the upstream Map< InterleavedBuffer, InterleavedBuffer >. The public
// signature can only carry a double-valued scratch map, so the created buffers live in this
// per-thread table and the caller's map records the slot of each source buffer. A returned
// attribute holds its buffer, so a slot expires with the last attribute that used it and the
// next call copies the buffer again, exactly like an upstream map miss.
thread_local std::unordered_map<double, std::weak_ptr<InterleavedBuffer>> copiedBuffers;
std::uint64_t nextCopiedBufferSlot = 1;
// Inner map key that records the slot of the copy created for one source buffer.
constexpr double copiedBufferSlotKey = 0;

std::shared_ptr<InterleavedBuffer> findCopiedBuffer(double slot) {
    const auto found = copiedBuffers.find(slot);
    if (found == copiedBuffers.end()) return nullptr;
    if (const auto buffer = found->second.lock()) return buffer;
    copiedBuffers.erase(found);
    return nullptr;
}

double storeCopiedBuffer(const std::shared_ptr<InterleavedBuffer>& buffer) {
    if (copiedBuffers.size() > 64)
        for (auto it = copiedBuffers.begin(); it != copiedBuffers.end();)
            it = it->second.expired() ? copiedBuffers.erase(it) : std::next(it);
    const double slot = double(nextCopiedBufferSlot++);
    copiedBuffers[slot] = buffer;
    return slot;
}

// The key of the copy cache. A plain attribute is its own source; an interleaved attribute is
// keyed by the identity of the backing array of its InterleavedBuffer, so every attribute that
// views one buffer derives the same key. The pointer is an opaque identity and is never
// dereferenced.
const VertexAttribute* sourceBufferKey(const AttributePtr& attribute, bool interleaved) {
    if (!interleaved) return attribute.get();
    const auto data = attribute->getProperty("data");
    const auto identity = arrayIdentity(animationArray(data.get("array")));
    return identity == nullptr ? attribute.get() : reinterpret_cast<const VertexAttribute*>(identity);
}

// `morphInfluences[ i ]`: a missing entry is undefined and participates as NaN.
double influenceAt(const SharedArray<double>& influences, double index) {
    if (!(index >= 0) || index >= double(influences.size()))
        return std::numeric_limits<double>::quiet_NaN();
    return influences[std::size_t(index)];
}

// ---------------------------------------------------------------------------
// The object a morphed geometry is read from, as far as the module uses it.
// ---------------------------------------------------------------------------

struct GeometrySource {
    std::shared_ptr<BufferGeometry> geometry;
    bool materialIsArray = false;
    const SharedArray<double>* influences = nullptr;
    const SkinnedMesh* skinned = nullptr;
};

template <class T>
bool readRenderable(const std::shared_ptr<Object3D>& object, GeometrySource& source) {
    const auto typed = std::dynamic_pointer_cast<T>(object);
    if (!typed) return false;
    source.geometry = typed->geometry;
    source.materialIsArray = typed->materialIsArray || !typed->materials.empty();
    if (typed->morphTargetInfluences) source.influences = &*typed->morphTargetInfluences;
    if constexpr (std::is_same_v<T, Mesh>) source.skinned = dynamic_cast<const SkinnedMesh*>(typed.get());
    return true;
}

GeometrySource geometrySource(const std::shared_ptr<Object3D>& object) {
    if (!object) throw JSTypeError("Cannot read properties of undefined (reading 'geometry')");
    GeometrySource source;
    if (readRenderable<Mesh>(object, source)) return source;
    if (readRenderable<Line>(object, source)) return source;
    if (readRenderable<Points>(object, source)) return source;
    const auto sprite = std::dynamic_pointer_cast<Sprite>(object);
    if (!sprite) throw JSTypeError("Cannot read properties of undefined (reading 'index')");
    source.geometry = sprite->geometry;
    return source;
}

} // namespace

// ---------------------------------------------------------------------------
// computeMikkTSpaceTangents
// ---------------------------------------------------------------------------

GeometryPtr computeMikkTSpaceTangents(const GeometryPtr& geometry, bool negateSign) {
    static_cast<void>(geometry);
    static_cast<void>(negateSign);
    // The source module needs examples/jsm/libs/mikktspace.module.js (or the `mikktspace` npm
    // package) and throws when the library is not ready. This project does not port MikkTSpace,
    // so the capability is reported instead of returning geometry with missing or wrong
    // tangents.
    throw std::runtime_error(
        "THREE.BufferGeometryUtils: Initialized MikkTSpace library required. "
        "three.cpp does not port the MikkTSpace library, so computeMikkTSpaceTangents() cannot "
        "compute tangents.");
}

// ---------------------------------------------------------------------------
// mergeGeometries
// ---------------------------------------------------------------------------

GeometryPtr mergeGeometries(const GeometryList& geometries, bool useGroups) {
    if (geometries.empty()) throw JSTypeError("Cannot read properties of undefined (reading 'index')");

    const bool isIndexed = geometries[0]->getIndex() != nullptr;

    std::vector<std::string> attributesUsed, morphAttributesUsed;
    for (const auto& [name, attribute] : geometries[0]->attributes) {
        static_cast<void>(attribute);
        attributesUsed.push_back(name);
    }
    for (const auto& [name, list] : geometries[0]->morphAttributes) {
        static_cast<void>(list);
        morphAttributesUsed.push_back(name);
    }
    const auto used = [](const std::vector<std::string>& names, const std::string& name) {
        return std::find(names.begin(), names.end(), name) != names.end();
    };

    OrderedMap<std::vector<AttributePtr>> attributes;
    OrderedMap<std::vector<std::vector<AttributePtr>>> morphAttributes;

    const bool morphTargetsRelative = geometries[0]->morphTargetsRelative;

    auto mergedGeometry = std::make_shared<BufferGeometry>();

    double offset = 0;

    for (std::size_t i = 0; i < geometries.size(); ++i) {
        const auto& geometry = geometries[i];
        if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'index')");
        double attributesCount = 0;
        double morphAttributesCount = 0;

        // ensure that all geometries are indexed, or none

        if (isIndexed != (geometry->getIndex() != nullptr)) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index "
                      << numberString(double(i))
                      << ". All geometries must have compatible attributes; make sure index attribute "
                         "exists among all geometries, or in none of them.\n";
            return nullptr;
        }

        // gather attributes, exit early if they're different

        for (const auto& [name, attribute] : geometry->attributes) {
            if (!used(attributesUsed, name)) {
                std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index "
                          << numberString(double(i))
                          << ". All geometries must have compatible attributes; make sure \"" << name
                          << "\" attribute exists among all geometries, or in none of them.\n";
                return nullptr;
            }

            if (attributes.find(name) == attributes.end()) attributes[name] = std::vector<AttributePtr>{};
            attributes[name].push_back(attribute);

            attributesCount++;
        }

        // ensure geometries have the same number of attributes

        if (attributesCount != double(attributesUsed.size())) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index "
                      << numberString(double(i))
                      << ". Make sure all geometries have the same number of attributes.\n";
            return nullptr;
        }

        // gather morph attributes, exit early if they're different

        if (morphTargetsRelative != geometry->morphTargetsRelative) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index "
                      << numberString(double(i))
                      << ". .morphTargetsRelative must be consistent throughout all geometries.\n";
            return nullptr;
        }

        for (const auto& [name, list] : geometry->morphAttributes) {
            if (!used(morphAttributesUsed, name)) {
                std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index "
                          << numberString(double(i))
                          << ".  .morphAttributes must be consistent throughout all geometries.\n";
                return nullptr;
            }

            const auto reference = geometries[0]->morphAttributes.find(name);
            if (reference == geometries[0]->morphAttributes.end() || list.size() != reference->second.size()) {
                std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index "
                          << numberString(double(i))
                          << ". All geometries must have the same number of morph targets for the \"" << name
                          << "\" morph attribute.\n";
                return nullptr;
            }

            if (morphAttributes.find(name) == morphAttributes.end())
                morphAttributes[name] = std::vector<std::vector<AttributePtr>>{};
            morphAttributes[name].push_back(list);

            morphAttributesCount++;
        }

        // ensure geometries have the same number of morph attributes

        if (morphAttributesCount != double(morphAttributesUsed.size())) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index "
                      << numberString(double(i))
                      << ". Make sure all geometries have the same number of morph attributes.\n";
            return nullptr;
        }

        if (useGroups) {
            double count;
            if (isIndexed) {
                const auto index = geometry->getIndex();
                if (!index) throw JSTypeError("Cannot read properties of null (reading 'count')");
                count = index->count;
            } else if (geometry->getAttribute("position") != nullptr) {
                count = geometry->getAttribute("position")->getCount();
            } else {
                std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed with geometry at index "
                          << numberString(double(i))
                          << ". The geometry must have either an index or a position attribute\n";
                return nullptr;
            }

            mergedGeometry->addGroup(offset, count, double(i));

            offset += count;
        }
    }

    // merge indices

    if (isIndexed) {
        double indexOffset = 0;
        std::vector<double> mergedIndex;

        for (std::size_t i = 0; i < geometries.size(); ++i) {
            const auto index = geometries[i]->getIndex();
            if (!index) throw JSTypeError("Cannot read properties of null (reading 'count')");

            for (double j = 0; j < index->count; ++j) mergedIndex.push_back(index->getX(j) + indexOffset);

            const auto position = geometries[i]->getAttribute("position");
            if (!position) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
            indexOffset += position->getCount();
        }

        setIndexFromNumbers(*mergedGeometry, mergedIndex);
    }

    // merge attributes

    for (const auto& [name, list] : attributes) {
        const auto mergedAttribute = mergeAttributes(list);

        if (!mergedAttribute) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed while trying to merge the "
                      << name << " attribute.\n";
            return nullptr;
        }

        mergedGeometry->setAttribute(name, mergedAttribute);
    }

    // merge morph attributes

    for (const auto& [name, lists] : morphAttributes) {
        const double numMorphTargets = double(lists[0].size());
        if (numMorphTargets == 0) continue;

        mergedGeometry->morphAttributes[name] = std::vector<AttributePtr>{};

        for (std::size_t i = 0; i < lists[0].size(); ++i) {
            AttributeList morphAttributesToMerge;
            for (const auto& list : lists) morphAttributesToMerge.push_back(list[i]);

            const auto mergedMorphAttribute = mergeAttributes(morphAttributesToMerge);

            if (!mergedMorphAttribute) {
                std::cerr << "THREE.BufferGeometryUtils: .mergeGeometries() failed while trying to merge the "
                          << name << " morphAttribute.\n";
                return nullptr;
            }

            mergedGeometry->morphAttributes[name].push_back(mergedMorphAttribute);
        }
    }

    return mergedGeometry;
}

// ---------------------------------------------------------------------------
// mergeAttributes
// ---------------------------------------------------------------------------

AttributePtr mergeAttributes(const AttributeList& attributes) {
    bool hasType = false;
    double kind = 0;
    bool hasItemSize = false;
    double itemSize = 0;
    bool hasNormalized = false;
    bool normalized = false;
    bool hasGpuType = false;
    JSValue gpuType = JSValue(-1);
    double arrayLength = 0;

    for (std::size_t i = 0; i < attributes.size(); ++i) {
        const auto& attribute = attributes[i];
        if (!attribute) throw JSTypeError("Cannot read properties of undefined (reading 'array')");
        const auto array = attributeArray(attribute);

        if (!hasType) {
            kind = arrayKind(array);
            hasType = true;
        }
        if (kind != arrayKind(array)) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.array must "
                         "be of consistent array types across matching attributes.\n";
            return nullptr;
        }

        if (!hasItemSize) {
            itemSize = attribute->getItemSize();
            hasItemSize = true;
        }
        if (itemSize != attribute->getItemSize()) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.itemSize must "
                         "be consistent across matching attributes.\n";
            return nullptr;
        }

        if (!hasNormalized) {
            normalized = attribute->getNormalized();
            hasNormalized = true;
        }
        if (normalized != attribute->getNormalized()) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.normalized "
                         "must be consistent across matching attributes.\n";
            return nullptr;
        }

        if (!hasGpuType) {
            gpuType = attribute->getProperty("gpuType");
            hasGpuType = true;
        }
        if (!gpuType.strictEquals(attribute->getProperty("gpuType"))) {
            std::cerr << "THREE.BufferGeometryUtils: .mergeAttributes() failed. BufferAttribute.gpuType must "
                         "be consistent across matching attributes.\n";
            return nullptr;
        }

        arrayLength += attribute->getCount() * itemSize;
    }

    // `new TypedArray( arrayLength )` with the still undefined constructor.
    if (!hasType) throw JSTypeError("TypedArray is not a constructor");

    auto array = allocateArray(attributeArray(attributes[0]), arrayLength);
    const auto result = makeBufferAttribute(array, itemSize, normalized);
    double offset = 0;

    for (std::size_t i = 0; i < attributes.size(); ++i) {
        const auto& attribute = attributes[i];
        if (isInterleaved(attribute)) {
            const double tupleOffset = offset / itemSize;

            for (double j = 0, l = attribute->getCount(); j < l; ++j)
                for (double c = 0; c < itemSize; ++c) {
                    const double value = attributeComponentByIndex(attribute, j, c);
                    result->setComponent(j + tupleOffset, c, value);
                }
        } else {
            arrayCopyInto(array, offset, attributeArray(attribute));
        }

        offset += attribute->getCount() * itemSize;
    }

    if (!gpuType.isUndefined()) result->gpuType = gpuType;

    return result;
}

// ---------------------------------------------------------------------------
// deepCloneAttribute
// ---------------------------------------------------------------------------

AttributePtr deepCloneAttribute(const AttributePtr& attribute) {
    if (!attribute) throw JSTypeError("Cannot read properties of undefined (reading 'array')");

    if (isInterleaved(attribute)) return deinterleaveAttribute(attribute);

    const auto buffer = std::dynamic_pointer_cast<BufferAttribute>(attribute);
    if (!buffer) throw JSTypeError("Cannot read properties of undefined (reading 'array')");

    if (dynamic_cast<const InstancedBufferAttribute*>(buffer.get()) != nullptr) {
        auto result = std::make_shared<InstancedBufferAttribute>(Float32Array(), 0., false);
        result->copy(*buffer);
        return result;
    }

    auto result = std::make_shared<BufferAttribute>(Float32Array(), 0., false);
    result->copy(*buffer);
    return result;
}

// ---------------------------------------------------------------------------
// interleaveAttributes
// ---------------------------------------------------------------------------

std::vector<AttributePtr> interleaveAttributes(const AttributeList& attributes) {
    bool hasType = false;
    double kind = 0;
    double arrayLength = 0;
    double stride = 0;

    // calculate the length and type of the interleavedBuffer
    for (std::size_t i = 0, l = attributes.size(); i < l; ++i) {
        const auto& attribute = attributes[i];
        if (!attribute) throw JSTypeError("Cannot read properties of undefined (reading 'array')");
        const auto array = attributeArray(attribute);

        if (!hasType) {
            kind = arrayKind(array);
            hasType = true;
        }
        if (kind != arrayKind(array)) {
            std::cerr << "AttributeBuffers of different types cannot be interleaved\n";
            return {};
        }

        arrayLength += arrayLengthOf(array);
        stride += attribute->getItemSize();
    }

    // `new TypedArray( arrayLength )` with the still undefined constructor.
    if (!hasType) throw JSTypeError("TypedArray is not a constructor");

    // Create the set of buffer attributes
    auto bufferArray = allocateArray(attributeArray(attributes[0]), arrayLength);
    InterleavedBuffer interleaved(nativeArray(bufferArray), JSValue(stride));
    double offset = 0;
    std::vector<AttributePtr> result;

    for (std::size_t j = 0, l = attributes.size(); j < l; j++) {
        const auto& attribute = attributes[j];
        const double itemSize = attribute->getItemSize();
        const double count = attribute->getCount();
        auto iba = std::make_shared<InterleavedBufferAttribute>(interleaved.object(), JSValue(itemSize),
                                                                JSValue(offset),
                                                                JSValue(attribute->getNormalized()));
        result.push_back(iba);

        offset += itemSize;

        // Move the data for each attribute into the new interleavedBuffer at the appropriate
        // offset. The upstream getters/setters arrays only cover four components.
        if (itemSize > 4) throw JSTypeError("attribute[getters[k]] is not a function");
        for (double c = 0; c < count; c++) {
            for (double k = 0; k < itemSize; k++) {
                const double value = attributeComponent(attribute, c, k);
                switch (static_cast<int>(k)) {
                case 0: iba->setX(c, value); break;
                case 1: iba->setY(c, value); break;
                case 2: iba->setZ(c, value); break;
                default: iba->setW(c, value); break;
                }
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// deinterleaveAttribute / deinterleaveGeometry
// ---------------------------------------------------------------------------

AttributePtr deinterleaveAttribute(const AttributePtr& attribute) {
    if (!attribute) throw JSTypeError("Cannot read properties of undefined (reading 'data')");

    const auto data = attribute->getProperty("data");
    const auto sourceArray = animationArray(data.get("array"));
    const double count = attribute->getProperty("count").toNumber();
    const double itemSize = attribute->getProperty("itemSize").toNumber();
    const bool normalized = attribute->getProperty("normalized").toBoolean();

    auto array = allocateArray(sourceArray, count * itemSize);
    std::shared_ptr<BufferAttribute> result;
    if (attribute->getProperty("isInstancedInterleavedBufferAttribute").toBoolean()) {
        const auto meshPerAttribute = attribute->getProperty("meshPerAttribute");
        result = withTypedArray(array, [&](const auto& values) -> std::shared_ptr<BufferAttribute> {
            return std::make_shared<InstancedBufferAttribute>(values, itemSize, normalized, meshPerAttribute);
        });
    } else {
        result = makeBufferAttribute(array, itemSize, normalized);
    }

    for (double i = 0; i < count; i++) {
        result->setX(i, attribute->getX(i));

        if (itemSize >= 2) result->setY(i, attribute->getY(i));
        if (itemSize >= 3) result->setZ(i, attribute->getZ(i));
        if (itemSize >= 4) result->setW(i, attribute->getW(i));
    }

    return result;
}

void deinterleaveGeometry(BufferGeometry& geometry) {
    std::unordered_map<const VertexAttribute*, AttributePtr> attrMap;

    for (auto& [key, attr] : geometry.attributes) {
        static_cast<void>(key);
        if (!attr || !isInterleaved(attr)) continue;
        auto cached = attrMap.find(attr.get());
        if (cached == attrMap.end()) cached = attrMap.emplace(attr.get(), deinterleaveAttribute(attr)).first;
        attr = cached->second;
    }

    for (auto& [key, morphAttribute] : geometry.morphAttributes) {
        static_cast<void>(key);
        for (std::size_t i = 0, l = morphAttribute.size(); i < l; i++) {
            const auto& attr = morphAttribute[i];
            if (!attr || !isInterleaved(attr)) continue;
            auto cached = attrMap.find(attr.get());
            if (cached == attrMap.end())
                cached = attrMap.emplace(attr.get(), deinterleaveAttribute(attr)).first;
            morphAttribute[i] = cached->second;
        }
    }
}

// ---------------------------------------------------------------------------
// estimateBytesUsed
// ---------------------------------------------------------------------------

double estimateBytesUsed(const BufferGeometry& geometry) {
    double mem = 0;
    for (const auto& [name, attribute] : geometry.attributes) {
        static_cast<void>(attribute);
        const auto attr = geometry.getAttribute(name);
        if (!attr) continue;
        mem += attr->getCount() * attr->getItemSize() * arrayBytesPerElement(attributeArray(attr));
    }

    const auto indices = geometry.getIndex();
    mem += indices ? indices->count * indices->itemSize * arrayBytesPerElement(attributeArray(indices)) : 0;
    return mem;
}

// ---------------------------------------------------------------------------
// mergeVertices
// ---------------------------------------------------------------------------

GeometryPtr mergeVertices(const GeometryPtr& geometry, double tolerance) {
    if (!geometry) throw JSTypeError("Cannot read properties of undefined (reading 'getIndex')");

    tolerance = MathUtils::max(tolerance, std::numeric_limits<double>::epsilon());

    // Generate an index buffer if the geometry doesn't have one, or optimize it if it's already
    // available.
    std::unordered_map<std::string, double> hashToIndex;
    const auto indices = geometry->getIndex();
    const auto positions = geometry->getAttribute("position");
    if (!indices && !positions) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    const double vertexCount = indices ? indices->count : positions->getCount();

    // next value for triangle indices
    double nextIndex = 0;

    // attributes and new attribute arrays
    std::vector<std::string> attributeNames;
    for (const auto& [name, attribute] : geometry->attributes) {
        static_cast<void>(attribute);
        attributeNames.push_back(name);
    }
    std::vector<double> sourceIndices;
    Buffers buffers;
    std::vector<double> newIndices;

    // convert the error tolerance to an amount of decimal places to truncate to
    const double halfTolerance = tolerance * 0.5;
    const double exponent = std::log10(1 / tolerance);
    const double hashMultiplier = std::pow(10, exponent);
    const double hashAdditive = halfTolerance * hashMultiplier;
    for (double i = 0; i < vertexCount; i++) {
        const double index = indices ? indices->getX(i) : i;

        // Generate a hash for the vertex attributes at the current index 'i'
        std::string hash;
        for (std::size_t j = 0, l = attributeNames.size(); j < l; j++) {
            const auto attribute = geometry->getAttribute(attributeNames[j]);
            if (!attribute) throw JSTypeError("Cannot read properties of undefined (reading 'itemSize')");
            const double itemSize = attribute->getItemSize();

            for (double k = 0; k < itemSize; k++) {
                // Math.trunc() preserves the full Number range, ~~ would wrap to int32
                hash += numberString(std::trunc(
                            attributeComponent(attribute, index, k) * hashMultiplier + hashAdditive)) + ",";
            }
        }

        // Add another reference to the vertex if it's already used by another index
        const auto existing = hashToIndex.find(hash);
        if (existing != hashToIndex.end()) {
            newIndices.push_back(existing->second);
        } else {
            sourceIndices.push_back(index);

            hashToIndex[hash] = nextIndex;
            newIndices.push_back(nextIndex);
            nextIndex++;
        }
    }

    const AttributePtr sourceIndexAttribute =
        std::make_shared<Float64BufferAttribute>(sourceIndices, 1.);

    // generate result BufferGeometry
    const auto result = geometry->clone();
    for (const auto& [name, attribute] : geometry->attributes)
        result->setAttribute(name, remapAttribute(attribute, sourceIndexAttribute, buffers));

    for (const auto& [name, morphAttribute] : geometry->morphAttributes) {
        std::vector<AttributePtr> remapped;
        remapped.reserve(morphAttribute.size());
        for (const auto& attribute : morphAttribute)
            remapped.push_back(remapAttribute(attribute, sourceIndexAttribute, buffers));
        result->morphAttributes[name] = std::move(remapped);
    }

    // indices

    setIndexFromNumbers(*result, newIndices);

    return result;
}

// ---------------------------------------------------------------------------
// toTrianglesDrawMode
// ---------------------------------------------------------------------------

GeometryPtr toTrianglesDrawMode(const GeometryPtr& geometry, double drawMode) {
    if (!geometry) throw JSTypeError("Cannot read properties of undefined (reading 'getIndex')");

    if (drawMode == TrianglesDrawMode) {
        std::cerr << "THREE.BufferGeometryUtils.toTrianglesDrawMode(): Geometry already defined as triangles.\n";
        return geometry;
    }

    if (drawMode == TriangleFanDrawMode || drawMode == TriangleStripDrawMode) {
        auto index = geometry->getIndex();

        // generate index if not present

        if (!index) {
            std::vector<double> indices;

            const auto position = geometry->getAttribute("position");

            if (position) {
                for (double i = 0; i < position->getCount(); i++) indices.push_back(i);

                setIndexFromNumbers(*geometry, indices);
                index = geometry->getIndex();
            } else {
                std::cerr << "THREE.BufferGeometryUtils.toTrianglesDrawMode(): Undefined position attribute. "
                             "Processing not possible.\n";
                return geometry;
            }
        }

        const double numberOfTriangles = index->count - 2;
        std::vector<double> newIndices;

        if (drawMode == TriangleFanDrawMode) {
            // gl.TRIANGLE_FAN

            for (double i = 1; i <= numberOfTriangles; i++) {
                newIndices.push_back(index->getX(0));
                newIndices.push_back(index->getX(i));
                newIndices.push_back(index->getX(i + 1));
            }
        } else {
            // gl.TRIANGLE_STRIP

            for (double i = 0; i < numberOfTriangles; i++) {
                if (std::fmod(i, 2) == 0) {
                    newIndices.push_back(index->getX(i));
                    newIndices.push_back(index->getX(i + 1));
                    newIndices.push_back(index->getX(i + 2));
                } else {
                    newIndices.push_back(index->getX(i + 2));
                    newIndices.push_back(index->getX(i + 1));
                    newIndices.push_back(index->getX(i));
                }
            }
        }

        if ((double(newIndices.size()) / 3) != numberOfTriangles) {
            std::cerr << "THREE.BufferGeometryUtils.toTrianglesDrawMode(): Unable to generate correct amount "
                         "of triangles.\n";
        }

        // updated indices

        setIndexFromNumbers(*geometry, newIndices);
        geometry->clearGroups();

        return geometry;
    }

    std::cerr << "THREE.BufferGeometryUtils.toTrianglesDrawMode(): Unknown draw mode: "
              << numberString(drawMode) << "\n";
    return geometry;
}

// ---------------------------------------------------------------------------
// computeMorphedAttributes
// ---------------------------------------------------------------------------

MorphedAttributes computeMorphedAttributes(const std::shared_ptr<Object3D>& object) {
    Vector3 _vA, _vB, _vC;
    Vector3 _tempA, _tempB, _tempC;
    Vector3 _morphA, _morphB, _morphC;

    const auto source = geometrySource(object);

    const auto geometry = source.geometry;
    if (!geometry) throw JSTypeError("Cannot read properties of undefined (reading 'index')");
    const bool materialIsArray = source.materialIsArray;

    const auto index = geometry->getIndex();
    const auto positionAttribute = geometry->getAttribute("position");
    const auto normalAttribute = geometry->getAttribute("normal");
    const auto& morphAttributeMap = geometry->morphAttributes;
    const auto morphPositionEntry = morphAttributeMap.find("position");
    const auto morphNormalEntry = morphAttributeMap.find("normal");
    const std::vector<AttributePtr>* morphPosition =
        morphPositionEntry == morphAttributeMap.end() ? nullptr : &morphPositionEntry->second;
    const std::vector<AttributePtr>* morphNormal =
        morphNormalEntry == morphAttributeMap.end() ? nullptr : &morphNormalEntry->second;
    const bool morphTargetsRelative = geometry->morphTargetsRelative;

    double i, j, il, jl;
    double start, end;

    if (!positionAttribute) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    Float32Array modifiedPosition(positionAttribute->getCount() * positionAttribute->getItemSize());
    if (!normalAttribute) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    Float32Array modifiedNormal(normalAttribute->getCount() * normalAttribute->getItemSize());

    const auto calculateMorphedAttributeData =
        [&](const AttributePtr& attribute, const std::vector<AttributePtr>* morphAttribute, double a,
            double b, double c, Float32Array& modifiedAttributeArray) {
            _vA.fromBufferAttribute(*attribute, a);
            _vB.fromBufferAttribute(*attribute, b);
            _vC.fromBufferAttribute(*attribute, c);

            const auto* morphInfluences = source.influences;

            if (morphAttribute && morphInfluences) {
                _morphA.set(0, 0, 0);
                _morphB.set(0, 0, 0);
                _morphC.set(0, 0, 0);

                for (std::size_t target = 0, limit = morphAttribute->size(); target < limit; target++) {
                    const double influence = influenceAt(*morphInfluences, double(target));
                    const auto& morph = (*morphAttribute)[target];

                    if (influence == 0) continue;

                    if (!morph) throw JSTypeError("Cannot read properties of undefined (reading 'getX')");
                    _tempA.fromBufferAttribute(*morph, a);
                    _tempB.fromBufferAttribute(*morph, b);
                    _tempC.fromBufferAttribute(*morph, c);

                    if (morphTargetsRelative) {
                        _morphA.addScaledVector(_tempA, influence);
                        _morphB.addScaledVector(_tempB, influence);
                        _morphC.addScaledVector(_tempC, influence);
                    } else {
                        _morphA.addScaledVector(_tempA.sub(_vA), influence);
                        _morphB.addScaledVector(_tempB.sub(_vB), influence);
                        _morphC.addScaledVector(_tempC.sub(_vC), influence);
                    }
                }

                _vA.add(_morphA);
                _vB.add(_morphB);
                _vC.add(_morphC);
            }

            if (source.skinned) {
                source.skinned->applyBoneTransform(a, _vA);
                source.skinned->applyBoneTransform(b, _vB);
                source.skinned->applyBoneTransform(c, _vC);
            }

            modifiedAttributeArray.setIndex(a * 3 + 0, _vA.x);
            modifiedAttributeArray.setIndex(a * 3 + 1, _vA.y);
            modifiedAttributeArray.setIndex(a * 3 + 2, _vA.z);
            modifiedAttributeArray.setIndex(b * 3 + 0, _vB.x);
            modifiedAttributeArray.setIndex(b * 3 + 1, _vB.y);
            modifiedAttributeArray.setIndex(b * 3 + 2, _vB.z);
            modifiedAttributeArray.setIndex(c * 3 + 0, _vC.x);
            modifiedAttributeArray.setIndex(c * 3 + 1, _vC.y);
            modifiedAttributeArray.setIndex(c * 3 + 2, _vC.z);
        };

    if (index) {
        // indexed buffer geometry

        if (materialIsArray) {
            const auto& groups = geometry->groups;

            for (i = 0, il = double(groups.size()); i < il; i++) {
                const auto& group = groups[std::size_t(i)];

                start = MathUtils::max(group.start, geometry->drawRange.start);
                end = MathUtils::min(group.start + group.count,
                               geometry->drawRange.start + geometry->drawRange.count);

                for (j = start, jl = end; j < jl; j += 3) {
                    const double a = index->getX(j);
                    const double b = index->getX(j + 1);
                    const double c = index->getX(j + 2);

                    calculateMorphedAttributeData(positionAttribute, morphPosition, a, b, c, modifiedPosition);
                    calculateMorphedAttributeData(normalAttribute, morphNormal, a, b, c, modifiedNormal);
                }
            }
        } else {
            start = MathUtils::max(0., geometry->drawRange.start);
            end = MathUtils::min(index->count, geometry->drawRange.start + geometry->drawRange.count);

            for (i = start, il = end; i < il; i += 3) {
                const double a = index->getX(i);
                const double b = index->getX(i + 1);
                const double c = index->getX(i + 2);

                calculateMorphedAttributeData(positionAttribute, morphPosition, a, b, c, modifiedPosition);
                calculateMorphedAttributeData(normalAttribute, morphNormal, a, b, c, modifiedNormal);
            }
        }
    } else {
        // non-indexed buffer geometry

        if (materialIsArray) {
            const auto& groups = geometry->groups;

            for (i = 0, il = double(groups.size()); i < il; i++) {
                const auto& group = groups[std::size_t(i)];

                start = MathUtils::max(group.start, geometry->drawRange.start);
                end = MathUtils::min(group.start + group.count,
                               geometry->drawRange.start + geometry->drawRange.count);

                for (j = start, jl = end; j < jl; j += 3) {
                    const double a = j;
                    const double b = j + 1;
                    const double c = j + 2;

                    calculateMorphedAttributeData(positionAttribute, morphPosition, a, b, c, modifiedPosition);
                    calculateMorphedAttributeData(normalAttribute, morphNormal, a, b, c, modifiedNormal);
                }
            }
        } else {
            start = MathUtils::max(0., geometry->drawRange.start);
            end = MathUtils::min(positionAttribute->getCount(),
                           geometry->drawRange.start + geometry->drawRange.count);

            for (i = start, il = end; i < il; i += 3) {
                const double a = i;
                const double b = i + 1;
                const double c = i + 2;

                calculateMorphedAttributeData(positionAttribute, morphPosition, a, b, c, modifiedPosition);
                calculateMorphedAttributeData(normalAttribute, morphNormal, a, b, c, modifiedNormal);
            }
        }
    }

    MorphedAttributes result;
    result.positionAttribute = positionAttribute;
    result.normalAttribute = normalAttribute;
    result.morphedPositionAttribute = std::make_shared<Float32BufferAttribute>(modifiedPosition, 3.);
    result.morphedNormalAttribute = std::make_shared<Float32BufferAttribute>(modifiedNormal, 3.);
    return result;
}

// ---------------------------------------------------------------------------
// mergeGroups
// ---------------------------------------------------------------------------

GeometryPtr mergeGroups(const GeometryPtr& geometry) {
    if (!geometry) throw JSTypeError("Cannot read properties of undefined (reading 'groups')");

    if (geometry->groups.empty()) {
        std::cerr << "THREE.BufferGeometryUtils.mergeGroups(): No groups are defined. Nothing to merge.\n";
        return geometry;
    }

    // sort groups by material index. The geometry owns the array, so the sort happens in place
    // exactly like `groups = geometry.groups; groups.sort( ... )`.
    auto& groups = geometry->groups;
    std::stable_sort(groups.begin(), groups.end(),
                     [](const BufferGeometry::Group& a, const BufferGeometry::Group& b) {
                         if (a.materialIndex != b.materialIndex) return a.materialIndex - b.materialIndex < 0;
                         return a.start - b.start < 0;
                     });

    // create index for non-indexed geometries

    if (geometry->getIndex() == nullptr) {
        const auto positionAttribute = geometry->getAttribute("position");
        if (!positionAttribute) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
        std::vector<double> indices;

        for (double i = 0; i < positionAttribute->getCount(); i += 3)
            indices.insert(indices.end(), {i, i + 1, i + 2});

        setIndexFromNumbers(*geometry, indices);
    }

    // sort index

    const auto index = geometry->getIndex();
    std::vector<double> newIndices;

    for (std::size_t i = 0; i < groups.size(); i++) {
        const auto& group = groups[i];

        const double groupStart = group.start;
        const double groupLength = groupStart + group.count;

        for (double j = groupStart; j < groupLength; j++) newIndices.push_back(index->getX(j));
    }

    geometry->dispose(); // Required to force buffer recreation
    setIndexFromNumbers(*geometry, newIndices);

    // update groups indices

    double start = 0;

    for (std::size_t i = 0; i < groups.size(); i++) {
        auto& group = groups[i];

        group.start = start;
        start += group.count;
    }

    // merge groups

    std::size_t current = 0;
    std::vector<std::size_t> dropped;

    for (std::size_t i = 1; i < groups.size(); i++) {
        if (groups[current].materialIndex == groups[i].materialIndex) {
            groups[current].count += groups[i].count;
            dropped.push_back(i);
        } else {
            current = i;
        }
    }
    for (auto it = dropped.rbegin(); it != dropped.rend(); ++it)
        groups.erase(groups.begin() + std::ptrdiff_t(*it));

    return geometry;
}

// ---------------------------------------------------------------------------
// toCreasedNormals
// ---------------------------------------------------------------------------

GeometryPtr toCreasedNormals(const GeometryPtr& geometry, double creaseAngle) {
    if (!geometry) throw JSTypeError("Cannot read properties of undefined (reading 'index')");

    // BufferGeometry.toNonIndexed() warns if the geometry is non-indexed and returns the
    // original geometry
    const GeometryPtr resultGeometry = geometry->getIndex() ? geometry->toNonIndexed() : geometry;
    const auto posAttr = resultGeometry->getAttribute("position");
    if (!posAttr) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    const double vertexCount = posAttr->getCount();

    InterpolantArray positions;
    const auto* plainPositions = dynamic_cast<const BufferAttribute*>(posAttr.get());

    if (plainPositions != nullptr && posAttr->getItemSize() == 3 && !posAttr->getNormalized()) {
        positions = plainPositions->getArray();
    } else {
        // flatten the position buffer so the math below operates on plain numbers
        Float64Array flattened(vertexCount * 3);

        for (double i = 0; i < vertexCount; i++) {
            flattened.setIndex(3 * i + 0, posAttr->getX(i));
            flattened.setIndex(3 * i + 1, posAttr->getY(i));
            flattened.setIndex(3 * i + 2, posAttr->getZ(i));
        }

        positions = flattened;
    }

    const double creaseDot = std::cos(creaseAngle);
    const double hashMultiplier = (1 + 1e-10) * 1e2;
    const double faceCount = vertexCount / 3;

    // compute the normal of each face
    Float64Array faceNormals(faceCount * 3);
    for (double f = 0; f < faceCount; f++) {
        const double f9 = 9 * f;
        const double ax = arrayRead(positions, f9 + 0), ay = arrayRead(positions, f9 + 1),
                     az = arrayRead(positions, f9 + 2);
        const double bx = arrayRead(positions, f9 + 3), by = arrayRead(positions, f9 + 4),
                     bz = arrayRead(positions, f9 + 5);
        const double cx = arrayRead(positions, f9 + 6), cy = arrayRead(positions, f9 + 7),
                     cz = arrayRead(positions, f9 + 8);

        const double v1x = cx - bx, v1y = cy - by, v1z = cz - bz;
        const double v2x = ax - bx, v2y = ay - by, v2z = az - bz;

        const double nx = v1y * v2z - v1z * v2y;
        const double ny = v1z * v2x - v1x * v2z;
        const double nz = v1x * v2y - v1y * v2x;

        const double squared = nx * nx + ny * ny + nz * nz;
        const double length = std::sqrt(squared);
        const double invLength = 1 / ((length == 0 || std::isnan(length)) ? 1 : length);
        faceNormals.setIndex(3 * f + 0, nx * invLength);
        faceNormals.setIndex(3 * f + 1, ny * invLength);
        faceNormals.setIndex(3 * f + 2, nz * invLength);
    }

    // assign an id to each vertex, sharing the id between vertices with the same quantized
    // position via an open-addressed hash table (slots hold id + 1, 0 means empty)
    Int32Array vertexIds(vertexCount);
    Float64Array quantized(vertexCount * 3);

    double tableSize = 1;
    const double needed = vertexCount * 2;
    while (tableSize < needed) {
        const auto next = shiftLeft(toInt32(tableSize));
        // Once the shift leaves the positive 32 bit range the upstream loop cannot reach the
        // target size any more; the port reports that instead of spinning forever.
        if (next <= 0) throw JSRangeError("Invalid typed array length");
        tableSize = next;
    }
    const double tableMask = tableSize - 1;
    Int32Array table(tableSize);

    double uniqueCount = 0;
    for (double i = 0; i < vertexCount; i++) {
        const double i3 = 3 * i;
        const double qx = std::trunc(arrayRead(positions, i3 + 0) * hashMultiplier);
        const double qy = std::trunc(arrayRead(positions, i3 + 1) * hashMultiplier);
        const double qz = std::trunc(arrayRead(positions, i3 + 2) * hashMultiplier);

        double slot = bitAnd(bitXor(bitXor(imul(qx, 73856093), imul(qy, 19349663)), imul(qz, 83492791)),
                             tableMask);

        while (true) {
            const double id = table.get(slot).toNumber();

            if (id == 0) {
                const double q3 = 3 * uniqueCount;
                quantized.setIndex(q3 + 0, qx);
                quantized.setIndex(q3 + 1, qy);
                quantized.setIndex(q3 + 2, qz);

                table.setIndex(slot, uniqueCount + 1);
                vertexIds.setIndex(i, uniqueCount);
                uniqueCount++;
                break;
            }

            const double q3 = 3 * (id - 1);

            if (quantized.get(q3 + 0).toNumber() == qx && quantized.get(q3 + 1).toNumber() == qy &&
                quantized.get(q3 + 2).toNumber() == qz) {
                vertexIds.setIndex(i, id - 1);
                break;
            }

            slot = bitAnd(slot + 1, tableMask);
        }
    }

    // bucket the faces surrounding each unique vertex position
    Int32Array bucketOffsets(uniqueCount + 1);
    for (double i = 0; i < vertexCount; i++) {
        const double offset = vertexIds.get(i).toNumber() + 1;
        bucketOffsets.setIndex(offset, bucketOffsets.get(offset).toNumber() + 1);
    }
    for (double i = 0; i < uniqueCount; i++)
        bucketOffsets.setIndex(i + 1, bucketOffsets.get(i + 1).toNumber() + bucketOffsets.get(i).toNumber());

    Int32Array bucketFaces(vertexCount);
    Int32Array bucketCursors = bucketOffsets.slice(0, uniqueCount);
    for (double f = 0; f < faceCount; f++) {
        const double f3 = 3 * f;
        const auto place = [&](double id) {
            const double cursor = bucketCursors.get(id).toNumber();
            bucketFaces.setIndex(cursor, f);
            bucketCursors.setIndex(id, cursor + 1);
        };
        place(vertexIds.get(f3 + 0).toNumber());
        place(vertexIds.get(f3 + 1).toNumber());
        place(vertexIds.get(f3 + 2).toNumber());
    }

    // average the normals of the faces surrounding each vertex if they are within the provided
    // crease threshold
    Float32Array normalArray(vertexCount * 3);
    for (double f = 0; f < faceCount; f++) {
        const double f3 = 3 * f;
        const double nx = faceNormals.get(f3 + 0).toNumber();
        const double ny = faceNormals.get(f3 + 1).toNumber();
        const double nz = faceNormals.get(f3 + 2).toNumber();

        for (double n = 0; n < 3; n++) {
            const double i = f3 + n;
            const double id = vertexIds.get(i).toNumber();

            double sumX = 0, sumY = 0, sumZ = 0;

            for (double k = bucketOffsets.get(id).toNumber(), limit = bucketOffsets.get(id + 1).toNumber();
                 k < limit; k++) {
                const double o3 = 3 * bucketFaces.get(k).toNumber();
                const double ox = faceNormals.get(o3 + 0).toNumber();
                const double oy = faceNormals.get(o3 + 1).toNumber();
                const double oz = faceNormals.get(o3 + 2).toNumber();

                if (nx * ox + ny * oy + nz * oz > creaseDot) {
                    sumX += ox;
                    sumY += oy;
                    sumZ += oz;
                }
            }

            const double squared = sumX * sumX + sumY * sumY + sumZ * sumZ;
            const double length = std::sqrt(squared);
            const double invLength = 1 / ((length == 0 || std::isnan(length)) ? 1 : length);
            normalArray.setIndex(3 * i + 0, sumX * invLength);
            normalArray.setIndex(3 * i + 1, sumY * invLength);
            normalArray.setIndex(3 * i + 2, sumZ * invLength);
        }
    }

    resultGeometry->setAttribute("normal", std::make_shared<BufferAttribute>(normalArray, 3, false));
    return resultGeometry;
}

// ---------------------------------------------------------------------------
// remapAttribute
// ---------------------------------------------------------------------------

AttributePtr remapAttribute(const AttributePtr& attribute, const AttributePtr& sourceIndices,
                            Buffers& buffers) {
    if (!attribute)
        throw JSTypeError("Cannot read properties of undefined (reading 'isInterleavedBufferAttribute')");
    if (!sourceIndices) throw JSTypeError("Cannot read properties of undefined (reading 'length')");

    const bool interleaved = isInterleaved(attribute);
    const auto source = interleaved ? attribute->getProperty("data") : JSValue();
    const double stride = interleaved ? source.get("stride").toNumber() : attribute->getItemSize();
    const auto sourceArray = interleaved ? animationArray(source.get("array")) : attributeArray(attribute);
    const auto* key = sourceBufferKey(attribute, interleaved);

    std::shared_ptr<InterleavedBuffer> copiedBuffer;
    if (interleaved) {
        const auto entry = buffers.find(key);

        if (entry != buffers.end()) {
            const auto slot = entry->second.find(copiedBufferSlotKey);
            if (slot != entry->second.end()) copiedBuffer = findCopiedBuffer(slot->second);
        }
    }

    // A cache hit reuses the copied buffer without copying any data, exactly like the upstream
    // `data !== undefined` branch.
    std::shared_ptr<BufferAttribute> data;
    if (!interleaved || !copiedBuffer) {
        const double length = double(sourceIndices->getCount());
        auto array = allocateArray(sourceArray, length * stride);

        for (double i = 0; i < length; i++)
            for (double j = 0; j < stride; j++)
                arrayWrite(array, i * stride + j,
                           arrayRead(sourceArray, sourceIndexValue(sourceIndices, i) * stride + j));

        if (interleaved) {
            if (source.get("isInstancedInterleavedBuffer").toBoolean())
                copiedBuffer =
                    std::make_shared<InstancedInterleavedBuffer>(nativeArray(array), JSValue(stride),
                                                                 source.get("meshPerAttribute"));
            else copiedBuffer = std::make_shared<InterleavedBuffer>(nativeArray(array), JSValue(stride));

            copiedBuffer->setUsage(source.get("usage"));
            buffers[key][copiedBufferSlotKey] = storeCopiedBuffer(copiedBuffer);
        } else {
            data = makeAttributeLike(attribute, array, attribute->getProperty("itemSize").toNumber(),
                                     attribute->getProperty("normalized").toBoolean());
            data->setUsage(attribute->getProperty("usage"));
        }
    }

    if (interleaved) {
        // The returned attribute keeps the copied buffer alive, so the cache slot above expires
        // with the last attribute that views it.
        const auto result = std::shared_ptr<InterleavedBufferAttribute>(
            new InterleavedBufferAttribute(copiedBuffer->object(), attribute->getProperty("itemSize"),
                                           attribute->getProperty("offset"),
                                           attribute->getProperty("normalized")),
            [keepAlive = copiedBuffer](InterleavedBufferAttribute* value) { delete value; });
        result->name = attribute->getProperty("name");
        if (attribute->getProperty("isInstancedInterleavedBufferAttribute").toBoolean())
            result->object().set("isInstancedInterleavedBufferAttribute", true);
        return result;
    }

    data->name = attribute->getProperty("name").toString();
    data->gpuType = attribute->getProperty("gpuType");
    return data;
}

} // namespace BufferGeometryUtils
