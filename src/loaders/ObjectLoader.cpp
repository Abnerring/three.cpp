#include "loaders/ObjectLoader.h"

#include "animation/AnimationClip.h"
#include "core/BufferGeometry.h"
#include "core/NativeObject.h"
#include "cameras/OrthographicCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "core/Object3D.h"
#include "lights/AmbientLight.h"
#include "lights/DirectionalLight.h"
#include "lights/HemisphereLight.h"
#include "lights/LightProbe.h"
#include "lights/PointLight.h"
#include "lights/RectAreaLight.h"
#include "lights/SpotLight.h"
#include "loaders/BufferGeometryLoader.h"
#include "loaders/MaterialLoader.h"
#include "geometries/BoxGeometry.h"
#include "geometries/CapsuleGeometry.h"
#include "geometries/CircleGeometry.h"
#include "geometries/ConeGeometry.h"
#include "geometries/CylinderGeometry.h"
#include "geometries/DodecahedronGeometry.h"
#include "geometries/ExtrudeGeometry.h"
#include "geometries/IcosahedronGeometry.h"
#include "geometries/LatheGeometry.h"
#include "geometries/OctahedronGeometry.h"
#include "geometries/PlaneGeometry.h"
#include "geometries/PolyhedronGeometry.h"
#include "geometries/RingGeometry.h"
#include "geometries/ShapeGeometry.h"
#include "geometries/SphereGeometry.h"
#include "geometries/TetrahedronGeometry.h"
#include "geometries/TorusGeometry.h"
#include "geometries/TorusKnotGeometry.h"
#include "geometries/TubeGeometry.h"
#include "math/MathUtils.h"
#include "math/SphericalHarmonics3.h"
#include "objects/BatchedMesh.h"
#include "objects/Bone.h"
#include "objects/Group.h"
#include "objects/InstancedMesh.h"
#include "objects/LOD.h"
#include "objects/Line.h"
#include "objects/LineLoop.h"
#include "objects/LineSegments.h"
#include "objects/Mesh.h"
#include "objects/Points.h"
#include "objects/Skeleton.h"
#include "objects/SkinnedMesh.h"
#include "objects/Sprite.h"
#include "scenes/FogExp2.h"
#include "scenes/Scene.h"
#include "textures/Texture.h"
#include "textures/TextureSource.h"
#include <cctype>
#include <stdexcept>

namespace {
// The maths types read a std::vector, so the JSON array is copied into one. three keeps every matrix array in
// column-major order and so does this conversion: it is a straight copy, with no transposition anywhere.
std::vector<double> numbers(const JSValue& value) {
    std::vector<double> out;
    const auto length = static_cast<int>(value.length());
    for (int index = 0; index < length; ++index) out.push_back(value.get(index).toNumber());
    return out;
}
double optionalNumber(const JSValue& data, const char* name, double fallback) {
    const auto value = data.get(name);
    return value.isUndefined() ? fallback : value.toNumber();
}
}

ObjectLoader::Result ObjectLoader::parse(const JSValue& json) {
    Result result;
    const auto animations = json.get("animations");
    if (!animations.isUndefined())
        for (double index = 0; index < animations.length(); index += 1) {
            const auto clip = AnimationClip::parse(animations.get(index));
            clips_[clip->uuid.toString()] = clip;
            result.animations.push_back(clip);
        }
    // Upstream reads the shapes before the geometries, because a shape geometry names them by uuid.
    const auto shapeList = json.get("shapes");
    if (!shapeList.isUndefined())
        for (double index = 0; index < shapeList.length(); index += 1) {
            auto shape = std::make_shared<Shape>();
            if (const auto record = shapeList.get(index).toJSON()) shape->fromJSON(*record);
            shapes_[shape->uuid] = shape;
        }    const auto geometries = json.get("geometries");
    if (!geometries.isUndefined())
        for (double index = 0; index < geometries.length(); index += 1) {
            const auto data = geometries.get(index);
            const auto type = data.get("type").toString();
            std::shared_ptr<BufferGeometry> geometry;
            if (type == "BufferGeometry" || type == "InstancedBufferGeometry") {
                BufferGeometryLoader loader;
                geometry = loader.parseGeometry(data);
            } else if (type == "ShapeGeometry" || type == "ExtrudeGeometry") {
                // The two that resolve their shapes through the table read before the geometries.
                const auto record = data.toJSON();
                if (!record) throw std::runtime_error("ObjectLoader: geometry record is not serializable");
                if (type == "ShapeGeometry") geometry = ShapeGeometry::fromJSON(*record, shapes_);
                else geometry = ExtrudeGeometry::fromJSON(*record, shapes_);
            } else if (type == "BoxGeometry") geometry = BoxGeometry::fromJSON(*data.toJSON());
            else if (type == "CapsuleGeometry") geometry = CapsuleGeometry::fromJSON(*data.toJSON());
            else if (type == "CircleGeometry") geometry = CircleGeometry::fromJSON(*data.toJSON());
            else if (type == "ConeGeometry") geometry = ConeGeometry::fromJSON(*data.toJSON());
            else if (type == "CylinderGeometry") geometry = CylinderGeometry::fromJSON(*data.toJSON());
            else if (type == "DodecahedronGeometry") geometry = DodecahedronGeometry::fromJSON(*data.toJSON());
            else if (type == "IcosahedronGeometry") geometry = IcosahedronGeometry::fromJSON(*data.toJSON());
            else if (type == "LatheGeometry") geometry = LatheGeometry::fromJSON(*data.toJSON());
            else if (type == "OctahedronGeometry") geometry = OctahedronGeometry::fromJSON(*data.toJSON());
            else if (type == "PlaneGeometry") geometry = PlaneGeometry::fromJSON(*data.toJSON());
            else if (type == "PolyhedronGeometry") geometry = PolyhedronGeometry::fromJSON(*data.toJSON());
            else if (type == "RingGeometry") geometry = RingGeometry::fromJSON(*data.toJSON());
            else if (type == "SphereGeometry") geometry = SphereGeometry::fromJSON(*data.toJSON());
            else if (type == "TetrahedronGeometry") geometry = TetrahedronGeometry::fromJSON(*data.toJSON());
            else if (type == "TorusGeometry") geometry = TorusGeometry::fromJSON(*data.toJSON());
            else if (type == "TorusKnotGeometry") geometry = TorusKnotGeometry::fromJSON(*data.toJSON());
            else if (type == "TubeGeometry") geometry = TubeGeometry::fromJSON(*data.toJSON());
            else {
                // Every other type comes from Geometries[type].fromJSON upstream, and none are ported.
                throw std::runtime_error("ObjectLoader: unknown geometry type \"" + type + "\"");
            }
            geometry->uuid = data.get("uuid").toString();
            if (!data.get("name").isUndefined()) geometry->name = data.get("name").toString();
            if (const auto userData = data.get("userData").toJSON()) geometry->userData = *userData;
            geometries_[geometry->uuid] = geometry;
        }
    // Upstream deserialises each image record and looks it up by uuid when a texture names it. A record that
    // carries data becomes a native descriptor here; a record that carries a url would need a host decoder.
    const auto imageList = json.get("images");
    if (!imageList.isUndefined())
        for (double index = 0; index < imageList.length(); index += 1) {
            const auto data = imageList.get(index);
            const auto uuid = data.get("uuid").toString();
            // Upstream reads image.url and only then looks inside it, so a record is either {data, ...} or
            // {url: {data, ...}}. A string url would go to an image loader and needs a host decoder.
            const auto nested = data.get("data").isUndefined() && data.get("url").isObject();
            if (data.get("data").isUndefined() && !nested)
                throw std::runtime_error("ObjectLoader: image records that name a url need a host decoder");
            const auto source = nested ? data.get("url") : data;
            const auto kind = source.get("type").toString();
            const auto values = numbers(source.get("data"));
            TextureImage::Ptr image;
            if (kind == "Uint8Array") {
                std::vector<std::uint8_t> bytes; for (const auto value : values) bytes.push_back(static_cast<std::uint8_t>(value));
                const Uint8Array pixels(bytes);
                image = std::make_shared<TextureImage>(pixels, source.get("width").toNumber(), source.get("height").toNumber());
            } else if (kind == "Float32Array") {
                std::vector<float> floats; for (const auto value : values) floats.push_back(static_cast<float>(value));
                const Float32Array pixels(floats);
                image = std::make_shared<TextureImage>(pixels, source.get("width").toNumber(), source.get("height").toNumber());
            } else {
                throw std::runtime_error("ObjectLoader: image data type \"" + kind + "\" is not ported yet");
            }
            images_[uuid] = image;
        }    const auto textureList = json.get("textures");
    if (!textureList.isUndefined())
        for (double index = 0; index < textureList.length(); index += 1) {
            const auto data = textureList.get(index);
            // Images are a host concern, so no source is attached here; every other field is pure data and is
            // carried over exactly. Upstream builds a cube or data texture when the image says so, and with no
            // image it builds the plain one, which is the branch this port always takes.
            const auto texture = std::make_shared<Texture>();
            texture->uuid = data.get("uuid").toString();
            if (!data.get("name").isUndefined()) texture->name = data.get("name").toString();
            if (!data.get("mapping").isUndefined()) texture->mapping = data.get("mapping").toNumber();
            if (!data.get("channel").isUndefined()) texture->channel = data.get("channel").toNumber();
            if (!data.get("offset").isUndefined()) texture->offset.fromArray(numbers(data.get("offset")));
            if (!data.get("repeat").isUndefined()) texture->repeat.fromArray(numbers(data.get("repeat")));
            if (!data.get("center").isUndefined()) texture->center.fromArray(numbers(data.get("center")));
            if (!data.get("rotation").isUndefined()) texture->rotation = data.get("rotation").toNumber();
            if (!data.get("wrap").isUndefined()) {
                texture->wrapS = data.get("wrap").get("0").toNumber();
                texture->wrapT = data.get("wrap").get("1").toNumber();
            }
            if (!data.get("format").isUndefined()) texture->format = data.get("format").toNumber();
            if (!data.get("internalFormat").isUndefined()) texture->internalFormat = data.get("internalFormat").toString();
            if (!data.get("type").isUndefined()) texture->type = data.get("type").toNumber();
            if (!data.get("colorSpace").isUndefined()) texture->colorSpace = data.get("colorSpace").toString();
            if (!data.get("minFilter").isUndefined()) texture->minFilter = data.get("minFilter").toNumber();
            if (!data.get("magFilter").isUndefined()) texture->magFilter = data.get("magFilter").toNumber();
            if (!data.get("anisotropy").isUndefined()) texture->anisotropy = data.get("anisotropy").toNumber();
            if (!data.get("flipY").isUndefined()) texture->flipY = data.get("flipY").toBoolean();
            if (!data.get("generateMipmaps").isUndefined()) texture->generateMipmaps = data.get("generateMipmaps").toBoolean();
            if (!data.get("mipmapsAutoUpdate").isUndefined()) texture->mipmapsAutoUpdate = data.get("mipmapsAutoUpdate").toBoolean();
            if (!data.get("premultiplyAlpha").isUndefined()) texture->premultiplyAlpha = data.get("premultiplyAlpha").toBoolean();
            if (!data.get("unpackAlignment").isUndefined()) texture->unpackAlignment = data.get("unpackAlignment").toNumber();
            if (!data.get("normalized").isUndefined()) texture->normalized = data.get("normalized").toBoolean();
            if (!data.get("compareFunction").isUndefined())
                throw std::runtime_error("ObjectLoader: texture compare functions are not ported yet");
            // Upstream attaches the source the record names, and silently leaves it unset when missing.
            if (!data.get("image").isUndefined()) {
                const auto found = images_.find(data.get("image").toString());
                if (found != images_.end()) texture->image = found->second;
            }
            textures_[texture->uuid] = texture;
        }    const auto materialList = json.get("materials");
    if (!materialList.isUndefined()) {
        MaterialLoader loader;
        loader.setTextures(textures_);
        for (double index = 0; index < materialList.length(); index += 1) {
            const auto data = materialList.get(index);
            const auto uuid = data.get("uuid").toString();
            if (materials_.find(uuid) == materials_.end()) materials_[uuid] = loader.parseMaterial(data);
        }
    }
    const auto object = json.get("object");
    if (object.isUndefined()) throw std::runtime_error("ObjectLoader: the document has no object");
    result.object = parseObject(object);
    // Upstream builds the skeletons after the tree, because a skeleton resolves its bones by uuid.
    const auto skeletonList = json.get("skeletons");
    if (!skeletonList.isUndefined()) {
        std::map<std::string, std::shared_ptr<Bone>> bones;
        std::vector<std::shared_ptr<Object3D>> stack{result.object};
        while (!stack.empty()) {
            const auto current = stack.back();
            stack.pop_back();
            if (!current) continue;
            if (const auto bone = std::dynamic_pointer_cast<Bone>(current)) bones[current->uuid] = bone;
            for (const auto& child : current->children) stack.push_back(child);
        }
        for (double index = 0; index < skeletonList.length(); index += 1) {
            const auto skeleton = std::make_shared<Skeleton>();
            skeleton->fromJSON(*skeletonList.get(index).toJSON(), bones);
            skeletons_[skeleton->uuid] = skeleton;
        }
    }
    bindSkeletons(result.object);
    bindLightTargets(result.object);
    return result;
}

std::shared_ptr<Object3D> ObjectLoader::parseObject(const JSValue& data) {
    const auto type = data.get("type").toString();
    const auto geometryOf = [this](const JSValue& source) {
        const auto found = geometries_.find(source.toString());
        return found == geometries_.end() ? std::shared_ptr<BufferGeometry>() : found->second;
    };
    const auto materialOf = [this](const JSValue& source) {
        // A string is a uuid; an array is an object here, and only meshes carry arrays in this port, so the
        // other types refuse one by name rather than looking up a comma joined string and finding nothing.
        if (source.isObject() && source.length() > 0)
            throw std::runtime_error("ObjectLoader: material arrays are only ported for meshes so far");
        const auto found = materials_.find(source.toString());
        return found == materials_.end() ? std::shared_ptr<Material>() : found->second;
    };
    // Upstream resolves a material field that may be an array of uuids; a mesh built from one keeps it.
    const auto materialsOf = [this](const JSValue& source) {
        std::vector<std::shared_ptr<Material>> out;
        if (source.isUndefined()) return out;
        if (source.isObject() && source.length() > 0) {
            for (double index = 0; index < source.length(); index += 1) {
                const auto found = materials_.find(source.get(index).toString());
                if (found != materials_.end()) out.push_back(found->second);
            }
        } else if (const auto found = materials_.find(source.toString()); found != materials_.end()) {
            out.push_back(found->second);
        }
        return out;
    };
    std::shared_ptr<Object3D> object;
    if (type == "Scene") {
        const auto scene = std::make_shared<Scene>();
        // Upstream tests Number.isInteger: a number is a colour, anything else is a texture key, and textures
        // are not part of this port, so a string is refused rather than dropped.
        const auto background = data.get("background");
        if (!background.isUndefined()) {
            const auto text = background.toString();
            const bool numeric = !text.empty() &&
                (std::isdigit(static_cast<unsigned char>(text[0])) != 0 || text[0] == '-' || text[0] == '+');
            if (!numeric) throw std::runtime_error("ObjectLoader: texture backgrounds are not ported yet");
            scene->background = Color(background.toNumber());
        }
        if (!data.get("environment").isUndefined())
            throw std::runtime_error("ObjectLoader: texture environments are not ported yet");
        // Upstream builds one of the two fog types from the record and only copies a non-empty name.
        const auto fog = data.get("fog");
        if (!fog.isUndefined()) {
            const auto fogType = fog.get("type").toString();
            if (fogType == "Fog")
                scene->fog = std::make_shared<Fog>(static_cast<int>(fog.get("color").toNumber()),
                    optionalNumber(fog, "near", 1.0), optionalNumber(fog, "far", 1000.0));
            else if (fogType == "FogExp2")
                scene->fog = std::make_shared<FogExp2>(static_cast<int>(fog.get("color").toNumber()),
                    optionalNumber(fog, "density", 0.00025));
            else
                throw std::runtime_error("ObjectLoader: unknown fog type \"" + fogType + "\"");
        }
        if (!data.get("backgroundBlurriness").isUndefined()) scene->backgroundBlurriness = data.get("backgroundBlurriness").toNumber();
        if (!data.get("backgroundIntensity").isUndefined()) scene->backgroundIntensity = data.get("backgroundIntensity").toNumber();
        if (!data.get("backgroundRotation").isUndefined()) scene->backgroundRotation.fromArray(numbers(data.get("backgroundRotation")));
        if (!data.get("environmentIntensity").isUndefined()) scene->environmentIntensity = data.get("environmentIntensity").toNumber();
        if (!data.get("environmentRotation").isUndefined()) scene->environmentRotation.fromArray(numbers(data.get("environmentRotation")));        object = scene;
    } else if (type == "Group") object = std::make_shared<Group>();
    else if (type == "Mesh") {
        const auto resolved = materialsOf(data.get("material"));
        const auto geometry = geometryOf(data.get("geometry"));
        if (resolved.size() > 1) {
            object = std::make_shared<Mesh>(geometry ? geometry : std::make_shared<BufferGeometry>(), resolved);
        } else {
            const auto mesh = std::make_shared<Mesh>();
            mesh->geometry = geometry;
            mesh->material = resolved.empty() ? nullptr : resolved.front();
            object = mesh;
        }
    } else if (type == "Line") {
        const auto line = std::make_shared<Line>();
        line->geometry = geometryOf(data.get("geometry"));
        line->material = materialOf(data.get("material"));
        object = line;
    } else if (type == "LineLoop") {
        const auto line = std::make_shared<LineLoop>();
        line->geometry = geometryOf(data.get("geometry"));
        line->material = materialOf(data.get("material"));
        object = line;
    } else if (type == "LineSegments") {
        const auto line = std::make_shared<LineSegments>();
        line->geometry = geometryOf(data.get("geometry"));
        line->material = materialOf(data.get("material"));
        object = line;
    } else if (type == "Points" || type == "PointCloud") {
        const auto points = std::make_shared<Points>();
        points->geometry = geometryOf(data.get("geometry"));
        points->material = materialOf(data.get("material"));
        object = points;
    } else if (type == "Sprite") object = std::make_shared<Sprite>(materialOf(data.get("material")));
    else if (type == "LOD") object = std::make_shared<LOD>();
    else if (type == "Bone") object = std::make_shared<Bone>();
    else if (type == "Object3D" || type.empty()) object = std::make_shared<Object3D>();
    else if (type == "AmbientLight")
        object = std::make_shared<AmbientLight>(Color(data.get("color").toNumber()), optionalNumber(data, "intensity", 1.0));
    else if (type == "DirectionalLight") {
        const auto light = std::make_shared<DirectionalLight>(Color(data.get("color").toNumber()), optionalNumber(data, "intensity", 1.0));
        // Upstream records the uuid and binds the object once the tree is complete.
        if (!data.get("target").isUndefined() && !data.get("target").toString().empty())
            pendingTargets_.emplace_back(light, data.get("target").toString());
        object = light;
    } else if (type == "PointLight")
        object = std::make_shared<PointLight>(Color(data.get("color").toNumber()), optionalNumber(data, "intensity", 1.0),
            optionalNumber(data, "distance", 0.0), optionalNumber(data, "decay", 2.0));
    else if (type == "SpotLight") {
        const auto light = std::make_shared<SpotLight>(Color(data.get("color").toNumber()), optionalNumber(data, "intensity", 1.0),
            optionalNumber(data, "distance", 0.0), optionalNumber(data, "angle", MathUtils::PI),
            optionalNumber(data, "penumbra", 0.0), optionalNumber(data, "decay", 2.0));
        if (!data.get("target").isUndefined() && !data.get("target").toString().empty())
            pendingTargets_.emplace_back(light, data.get("target").toString());
        object = light;
    } else if (type == "HemisphereLight")
        object = std::make_shared<HemisphereLight>(Color(data.get("color").toNumber()),
            Color(data.get("groundColor").toNumber()), optionalNumber(data, "intensity", 1.0));
    else if (type == "InstancedMesh") {
        const auto geometry = geometryOf(data.get("geometry"));
        const auto material = materialOf(data.get("material"));
        const auto count = static_cast<int>(data.get("count").toNumber());
        const auto mesh = std::make_shared<InstancedMesh>(geometry ? geometry : std::make_shared<BufferGeometry>(), material, count);
        // Upstream re-reads both attributes as Float32Array with the item sizes sixteen and the stored one.
        const auto matrixArray = numbers(data.get("instanceMatrix").get("array"));
        mesh->instanceMatrix = std::make_shared<InstancedBufferAttribute>(
            TypedArray<float>(std::vector<float>(matrixArray.begin(), matrixArray.end())), 16);
        if (!data.get("instanceColor").isUndefined()) {
            const auto colorArray = numbers(data.get("instanceColor").get("array"));
            mesh->instanceColor = std::make_shared<InstancedBufferAttribute>(
                TypedArray<float>(std::vector<float>(colorArray.begin(), colorArray.end())),
                data.get("instanceColor").get("itemSize").toNumber());
        }
        object = mesh;
    }    else if (type == "PerspectiveCamera") {
        const auto camera = std::make_shared<PerspectiveCamera>(optionalNumber(data, "fov", 50.0), optionalNumber(data, "aspect", 1.0),
            optionalNumber(data, "near", 0.1), optionalNumber(data, "far", 2000.0));
        if (!data.get("focus").isUndefined()) camera->focus = data.get("focus").toNumber();
        if (!data.get("zoom").isUndefined()) camera->zoom = data.get("zoom").toNumber();
        if (!data.get("filmGauge").isUndefined()) camera->filmGauge = data.get("filmGauge").toNumber();
        if (!data.get("filmOffset").isUndefined()) camera->filmOffset = data.get("filmOffset").toNumber();
        if (!data.get("view").isUndefined()) throw std::runtime_error("ObjectLoader: camera views are not ported yet");
        object = camera;
    } else if (type == "OrthographicCamera") {
        const auto camera = std::make_shared<OrthographicCamera>(optionalNumber(data, "left", -1.0), optionalNumber(data, "right", 1.0),
            optionalNumber(data, "top", 1.0), optionalNumber(data, "bottom", -1.0),
            optionalNumber(data, "near", 0.1), optionalNumber(data, "far", 2000.0));
        if (!data.get("zoom").isUndefined()) camera->zoom = data.get("zoom").toNumber();
        if (!data.get("view").isUndefined()) throw std::runtime_error("ObjectLoader: camera views are not ported yet");
        object = camera;
    }
    else if (type == "SkinnedMesh") {
        const auto mesh = std::make_shared<SkinnedMesh>();
        mesh->geometry = geometryOf(data.get("geometry"));
        mesh->material = materialOf(data.get("material"));
        if (!data.get("bindMode").isUndefined()) mesh->bindMode = data.get("bindMode").toString();
        if (!data.get("bindMatrix").isUndefined()) mesh->bindMatrix.fromArray(numbers(data.get("bindMatrix")));
        // Upstream stores the uuid here and binds it after the skeletons are built.
        if (!data.get("skeleton").isUndefined()) pendingSkeletons_.emplace_back(mesh, data.get("skeleton").toString());
        object = mesh;
    }    else if (type == "BatchedMesh") {
        const auto material = materialOf(data.get("material"));
        const auto mesh = std::make_shared<BatchedMesh>(data.get("maxInstanceCount").toNumber(),
            data.get("maxVertexCount").toNumber(), data.get("maxIndexCount").toNumber(), material);
        const auto geometry = geometryOf(data.get("geometry"));
        mesh->geometry = geometry ? geometry : std::make_shared<BufferGeometry>();
        if (!data.get("perObjectFrustumCulled").isUndefined()) mesh->perObjectFrustumCulled = data.get("perObjectFrustumCulled").toBoolean();
        if (!data.get("sortObjects").isUndefined()) mesh->sortObjects = data.get("sortObjects").toBoolean();
        // The ranges come back as one record per geometry, with the bounds decoded from their JSON forms.
        const auto infos = data.get("geometryInfo");
        if (!infos.isUndefined())
            for (double index = 0; index < infos.length(); index += 1) {
                const auto source = infos.get(index);
                BatchedMesh::GeometryInfo info;
                info.vertexStart = source.get("vertexStart").toNumber();
                info.vertexCount = source.get("vertexCount").toNumber();
                info.reservedVertexCount = source.get("reservedVertexCount").toNumber();
                info.indexStart = source.get("indexStart").toNumber();
                info.indexCount = source.get("indexCount").toNumber();
                info.reservedIndexCount = source.get("reservedIndexCount").toNumber();
                info.start = source.get("start").toNumber();
                info.count = source.get("count").toNumber();
                info.active = source.get("active").isUndefined() ? true : source.get("active").toBoolean();
                if (!source.get("boundingBox").isUndefined()) { Box3 decoded; decoded.fromJSON(source.get("boundingBox")); info.boundingBox = decoded; }
                if (!source.get("boundingSphere").isUndefined()) { Sphere decoded; decoded.fromJSON(source.get("boundingSphere")); info.boundingSphere = decoded; }
                mesh->_geometryInfo.push_back(info);
            }
        const auto instances = data.get("instanceInfo");
        if (!instances.isUndefined())
            for (double index = 0; index < instances.length(); index += 1) {
                const auto source = instances.get(index);
                BatchedMesh::InstanceInfo info;
                info.geometryId = source.get("geometryIndex").toNumber();
                info.active = source.get("active").isUndefined() ? true : source.get("active").toBoolean();
                info.visible = source.get("visible").isUndefined() ? true : source.get("visible").toBoolean();
                mesh->_instanceInfo.push_back(info);
            }
        const auto readIds = [](const JSValue& source, std::vector<double>& target) {
            if (source.isUndefined()) return;
            for (double index = 0; index < source.length(); index += 1) target.push_back(source.get(index).toNumber());
        };
        readIds(data.get("availableInstanceIds"), mesh->_availableInstanceIds);
        readIds(data.get("availableGeometryIds"), mesh->_availableGeometryIds);
        if (!data.get("nextIndexStart").isUndefined()) mesh->_nextIndexStart = data.get("nextIndexStart").toNumber();
        if (!data.get("nextVertexStart").isUndefined()) mesh->_nextVertexStart = data.get("nextVertexStart").toNumber();
        if (!data.get("geometryCount").isUndefined()) mesh->_geometryCount = data.get("geometryCount").toNumber();
        if (!data.get("geometryInitialized").isUndefined()) mesh->_geometryInitialized = data.get("geometryInitialized").toBoolean();
        if (!data.get("boundingBox").isUndefined()) { Box3 decoded; decoded.fromJSON(data.get("boundingBox")); mesh->boundingBox = decoded; }
        if (!data.get("boundingSphere").isUndefined()) { Sphere decoded; decoded.fromJSON(data.get("boundingSphere")); mesh->boundingSphere = decoded; }
        // The matrices, indirect and colours textures the document names have no native counterpart: the
        // matrices and colours live in vectors here, and the indirect table only exists for multi draw.
        object = mesh;
    }
    else if (type == "RectAreaLight")
        object = std::make_shared<RectAreaLight>(Color(data.get("color").toNumber()), optionalNumber(data, "intensity", 1.0),
            optionalNumber(data, "width", 10.0), optionalNumber(data, "height", 10.0));
    else if (type == "LightProbe") {
        // Upstream builds the harmonics from the flat coefficient array the document carries.
        auto harmonics = std::make_shared<SphericalHarmonics3>();
        harmonics->fromArray(numbers(data.get("sh")));
        object = std::make_shared<LightProbe>(harmonics, optionalNumber(data, "intensity", 1.0));
    }
    else object = std::make_shared<Object3D>();   // upstream falls back to Object3D for an unknown type

    // The epilogue below is shared by every type and mirrors the upstream tail line for line.
    object->uuid = data.get("uuid").toString();
    if (!data.get("name").isUndefined()) object->name = data.get("name").toString();
    if (!data.get("pivot").isUndefined()) {
        Vector3 pivot;
        pivot.fromArray(numbers(data.get("pivot")));
        object->pivot = pivot;
    }
    const auto matrix = data.get("matrix");
    if (!matrix.isUndefined()) {
        object->matrix.fromArray(numbers(matrix));
        if (!data.get("matrixAutoUpdate").isUndefined())
            object->matrixAutoUpdate = data.get("matrixAutoUpdate").toBoolean();
        if (object->matrixAutoUpdate) {
            object->matrix.decompose(object->position, object->quaternion, object->scale);
            if (object->pivot) {
                object->position.copy(*object->pivot);
                object->position.applyMatrix4(object->matrix);
                object->position.sub(*object->pivot);
            }
        }
    } else {
        if (!data.get("position").isUndefined()) object->position.fromArray(numbers(data.get("position")));
        if (!data.get("rotation").isUndefined()) object->rotation.fromArray(numbers(data.get("rotation")));
        if (!data.get("quaternion").isUndefined()) object->quaternion.fromArray(numbers(data.get("quaternion")));
        if (!data.get("scale").isUndefined()) object->scale.fromArray(numbers(data.get("scale")));
    }
    if (!data.get("up").isUndefined()) object->up.fromArray(numbers(data.get("up")));
    if (!data.get("morphTargetDictionary").isUndefined() || !data.get("morphTargetInfluences").isUndefined())
        throw std::runtime_error("ObjectLoader: morph target dictionaries are not ported yet");
    if (!data.get("castShadow").isUndefined()) object->castShadow = data.get("castShadow").toBoolean();
    if (!data.get("receiveShadow").isUndefined()) object->receiveShadow = data.get("receiveShadow").toBoolean();
    if (!data.get("shadow").isUndefined())
        throw std::runtime_error("ObjectLoader: light shadows are not ported in this slice");
    if (!data.get("visible").isUndefined()) object->visible = data.get("visible").toBoolean();
    if (!data.get("frustumCulled").isUndefined()) object->frustumCulled = data.get("frustumCulled").toBoolean();
    if (!data.get("renderOrder").isUndefined()) object->renderOrder = data.get("renderOrder").toNumber();
    if (!data.get("static").isUndefined()) object->isStatic = data.get("static").toBoolean();
    if (const auto userData = data.get("userData").toJSON()) object->userData = *userData;
    if (!data.get("layers").isUndefined()) object->layers.mask = data.get("layers").toNumber();
    const auto children = data.get("children");
    if (!children.isUndefined())
        for (double index = 0; index < children.length(); index += 1)
            object->add(parseObject(children.get(index)));
    const auto objectAnimations = data.get("animations");
    if (!objectAnimations.isUndefined())
        for (double index = 0; index < objectAnimations.length(); index += 1) {
            const auto found = clips_.find(objectAnimations.get(index).toString());
            if (found != clips_.end()) object->animations.push_back(found->second);
        }
    // Upstream restores the levels last, after the children exist, because a level names a child by uuid.
    if (type == "LOD") {
        const auto lod = std::dynamic_pointer_cast<LOD>(object);
        if (!data.get("autoUpdate").isUndefined()) lod->autoUpdate = data.get("autoUpdate");
        const auto levels = data.get("levels");
        if (!levels.isUndefined())
            for (double index = 0; index < levels.length(); index += 1) {
                const auto level = levels.get(index);
                const auto wanted = level.get("object").toString();
                // The upstream lookup searches this object's subtree, not the whole document.
                std::shared_ptr<Object3D> found;
                std::vector<std::shared_ptr<Object3D>> stack{object};
                while (!stack.empty() && !found) {
                    const auto current = stack.back();
                    stack.pop_back();
                    if (!current) continue;
                    if (current->uuid == wanted) { found = current; break; }
                    for (const auto& child : current->children) stack.push_back(child);
                }
                if (found) lod->addLevel(found, level.get("distance"), level.get("hysteresis"));
            }
    }    return object;
}

void ObjectLoader::bindLightTargets(const std::shared_ptr<Object3D>& root) {
    // Upstream resolves each recorded uuid against the finished tree and points the light at that object.
    std::map<std::string, std::shared_ptr<Object3D>> byUuid;
    std::vector<std::shared_ptr<Object3D>> stack{root};
    while (!stack.empty()) {
        const auto current = stack.back();
        stack.pop_back();
        if (!current) continue;
        byUuid[current->uuid] = current;
        for (const auto& child : current->children) stack.push_back(child);
    }
    for (const auto& [light, uuid] : pendingTargets_) {
        const auto found = byUuid.find(uuid);
        if (found == byUuid.end()) continue;
        if (const auto directional = std::dynamic_pointer_cast<DirectionalLight>(light)) directional->target = found->second;
        else if (const auto spot = std::dynamic_pointer_cast<SpotLight>(light)) spot->target = found->second;
    }
    pendingTargets_.clear();
}
void ObjectLoader::bindSkeletons(const std::shared_ptr<Object3D>& root) {
    static_cast<void>(root);
    if (pendingSkeletons_.empty()) return;
    // Upstream resolves the recorded uuid and binds with the mesh own bind matrix.
    for (const auto& [object, uuid] : pendingSkeletons_) {
        const auto found = skeletons_.find(uuid);
        if (found == skeletons_.end()) continue;
        const auto mesh = std::dynamic_pointer_cast<SkinnedMesh>(object);
        if (!mesh) continue;
        mesh->bind(found->second, mesh->bindMatrix);
    }
    pendingSkeletons_.clear();
}
