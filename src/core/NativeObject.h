#pragma once
#include "animation/AnimationUtils.h"
#include "core/NativeArray.h"
#include <memory>

class Object3D;
class Material;
class Vector2;
class Vector3;
class Vector4;
class Quaternion;
class Euler;
class Color;
class Texture;
class Matrix3;
class Matrix4;
class Plane;
class VertexAttribute;
class BufferGeometry;

// Host wrappers retain their native owner. Ordinary JS properties remain available
// for extensions and PropertyBinding uses the same get/has/set/call operations.
JSValue nativeObject(const std::shared_ptr<Object3D>& object);
JSValue nativeMaterial(const std::shared_ptr<Material>& material);
JSValue nativeTexture(const std::shared_ptr<Texture>& texture);
JSValue nativeVector(const std::shared_ptr<Vector2>& vector);
JSValue nativeVector(const std::shared_ptr<Vector3>& vector);
JSValue nativeVector(const std::shared_ptr<Vector4>& vector);
JSValue nativeQuaternion(const std::shared_ptr<Quaternion>& quaternion);
JSValue nativeEuler(const std::shared_ptr<Euler>& euler);
JSValue nativeColor(const std::shared_ptr<Color>& color);
JSValue nativeColorPrototype();
JSValue nativeMatrix(const std::shared_ptr<Matrix3>& matrix);
JSValue nativeMatrix(const std::shared_ptr<Matrix4>& matrix);
JSValue nativeMatrix3Prototype();
JSValue nativeMatrix4Prototype();
JSValue nativePlane(const std::shared_ptr<Plane>& plane);
JSValue nativePlanePrototype();
JSValue nativeArray(const AnimationArray& array);
AnimationArray animationArray(const JSValue& value);
JSValue nativeAttribute(const std::shared_ptr<VertexAttribute>& attribute);
JSValue nativeGeometry(const std::shared_ptr<BufferGeometry>& geometry);
std::shared_ptr<VertexAttribute> vertexAttribute(const JSValue& value);
