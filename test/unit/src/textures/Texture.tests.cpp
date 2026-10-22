#include "catch.hpp"
#include "core/WebGLCubeRenderTarget.h"
#include "textures/DataTexture.h"
#include "textures/DataArrayTexture.h"
#include "textures/Data3DTexture.h"
#include "textures/DepthTexture.h"
#include "textures/CubeDepthTexture.h"
#include "textures/CubeTexture.h"
#include "textures/FramebufferTexture.h"
#include "textures/CompressedArrayTexture.h"
#include "textures/CompressedCubeTexture.h"
#include "textures/ExternalTexture.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "scenes/Scene.h"
#include "math/Plane.h"
#include <cmath>

TEST_CASE("Texture updates use strict true and shared source versions") {
    Texture texture;
    texture.needsUpdate = 1; texture.needsUpdate = "true"; texture.needsUpdate = false;
    REQUIRE(texture.version == 0); REQUIRE(texture.source->version == 0);
    texture.needsUpdate = true;
    auto clone = texture.clone();
    REQUIRE(clone->source == texture.source); REQUIRE(clone->version == 1);
    REQUIRE(texture.source->version == 2); REQUIRE(texture.version == 1);
    texture.source->needsUpdate = 1; REQUIRE(texture.source->version == 2);
    texture.needsPMREMUpdate = true; texture.needsPMREMUpdate = 1;
    REQUIRE(texture.pmremVersion == 1); REQUIRE(clone->pmremVersion == 0);
    REQUIRE(texture.uuid != clone->uuid); REQUIRE(texture.id != clone->id);
}
TEST_CASE("Texture cloning preserves image view and mip identities but copies JSON user data") {
    Uint8Array pixels{1, 2, 3, 4};
    DataTexture texture(pixels, 1, 1);
    auto level = std::make_shared<TextureImage>(pixels, 1, 1);
    texture.mipmaps->push_back(level); texture.addUpdateRange(0, 4);
    texture.userData.set("nested", JSValue::object());
    texture.userData.get("nested").set("value", 4);
    auto clone = texture.clone();
    REQUIRE(clone->isDataTexture); REQUIRE(clone->image.get() == texture.image.get());
    REQUIRE(clone->mipmaps != texture.mipmaps); REQUIRE(clone->mipmaps->at(0) == level);
    REQUIRE(clone->updateRanges->empty());
    std::get<Uint8Array>(clone->image->data)[0] = 9;
    REQUIRE(pixels[0] == 9);
    clone->userData.get("nested").set("value", 7);
    REQUIRE(texture.userData.get("nested").get("value").number() == 4);
    auto ranges = texture.updateRanges; auto range = ranges->at(0);
    texture.clearUpdateRanges(); REQUIRE(ranges->empty()); REQUIRE(range->count == 4);
    auto next = std::make_shared<TextureImage>(2, 3, 4);
    clone->image = next; REQUIRE(texture.image.get() == next);
    REQUIRE(double(texture.width) == 2); REQUIRE(double(texture.height) == 3); REQUIRE(double(texture.depth) == 4);
}
TEST_CASE("Texture dispose dispatches every time without changing versions") {
    Texture texture; int calls = 0;
    texture.addEventListener("dispose", [&](Event& event) {
        REQUIRE(event.target == &texture); ++calls;
    });
    texture.dispose(); texture.dispose(); REQUIRE(calls == 2); REQUIRE(texture.version == 0);
}
TEST_CASE("Texture upload callbacks retain mutable captures and survive self replacement") {
    Texture texture;
    std::vector<int> calls;
    texture.onUpdate = [counter = 0, &calls](Texture& self) mutable {
        calls.push_back(++counter);
        if (counter == 2) self.onUpdate = nullptr;
    };
    auto retained = texture.onUpdate;
    retained(texture); texture.onUpdate(texture);
    REQUIRE((calls == std::vector<int>{1, 2})); REQUIRE(!texture.onUpdate);
    retained(texture); REQUIRE(calls.back() == 3);
}
TEST_CASE("Texture serialization caches identities across textures sharing a source") {
    DataTexture texture(Uint8Array{1, 2, 3, 4}, 1, 1);
    auto clone = texture.clone();
    TextureJSONContext meta;
    auto first = texture.toJSON(&meta); clone->toJSON(&meta);
    REQUIRE(meta.textures.size() == 2); REQUIRE(meta.images.size() == 1);
    first["name"] = "cached";
    REQUIRE(texture.toJSON(&meta).at("name").get<std::string>() == "cached");
    REQUIRE(meta.images.at(texture.source->uuid).at("url").at("type").get<std::string>() == "Uint8Array");
}
TEST_CASE("Depth and framebuffer copies allocate image descriptors after source version increment") {
    DepthTexture depth(8, 4, UnsignedIntType, UVMapping, ClampToEdgeWrapping, ClampToEdgeWrapping,
                       NearestFilter, NearestFilter, 1, DepthFormat, 3);
    depth.compareFunction = LessEqualCompare;
    auto clone = std::dynamic_pointer_cast<DepthTexture>(depth.clone());
    REQUIRE(depth.isArrayTexture); REQUIRE(clone->source != depth.source);
    REQUIRE(clone->image.get() != depth.image.get()); REQUIRE(clone->compareFunction == depth.compareFunction);
    REQUIRE(depth.source->version == 1); REQUIRE(clone->source->version == 0); REQUIRE(clone->version == 1);
    FramebufferTexture framebuffer(3, 2); auto frameClone = framebuffer.clone();
    REQUIRE(frameClone->version == 2); REQUIRE(framebuffer.source->version == 2);
    REQUIRE(frameClone->source->version == 0); REQUIRE(frameClone->image.get() != framebuffer.image.get());
    CubeDepthTexture cube(16); REQUIRE(cube.image->faces[0] == cube.image->faces[5]);
    auto cubeClone = cube.clone(); REQUIRE(!cubeClone->image->isArray);
    REQUIRE(cubeClone->image->faces[0] == cube.image->faces[0]);
}
TEST_CASE("Array texture layer sets preserve insertion order and SameValueZero") {
    DataArrayTexture texture(nullptr, 2, 3, 4);
    REQUIRE(!texture.isArrayTexture); REQUIRE(texture.isDataArrayTexture);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    texture.addLayerUpdate(2); texture.addLayerUpdate(-0.0); texture.addLayerUpdate(nan);
    texture.addLayerUpdate(2); texture.addLayerUpdate(0); texture.addLayerUpdate(nan);
    REQUIRE(texture.layerUpdates->size() == 3); REQUIRE(!std::signbit(texture.layerUpdates->at(1)));
    REQUIRE(std::isnan(texture.layerUpdates->at(2)));
    auto clone = std::dynamic_pointer_cast<DataArrayTexture>(texture.clone());
    REQUIRE(clone->layerUpdates->empty());
    auto set = texture.layerUpdates; texture.clearLayerUpdates(); REQUIRE(set->empty());
}
TEST_CASE("Texture setValues updates subclass properties and retains numeric values") {
    auto values = JSValue::object(); values.set("wrapR", MirroredRepeatWrapping);
    DataArrayTexture array; Data3DTexture volume; CompressedArrayTexture compressed;
    array.setValues(values); volume.setValues(values); compressed.setValues(values);
    REQUIRE(array.wrapR == MirroredRepeatWrapping); REQUIRE(volume.wrapR == MirroredRepeatWrapping);
    REQUIRE(compressed.wrapR == MirroredRepeatWrapping);
    values = JSValue::object(); values.set("compareFunction", LessEqualCompare);
    DepthTexture depth; CubeDepthTexture cube;
    depth.setValues(values); cube.setValues(values);
    REQUIRE(depth.compareFunction.value() == LessEqualCompare); REQUIRE(cube.compareFunction.value() == LessEqualCompare);
    values.set("compareFunction", nullptr); depth.setValues(values); REQUIRE(!depth.compareFunction);
    values = JSValue::object(); values.set("channel", 1.5); values.set("mapping", std::numeric_limits<double>::quiet_NaN());
    depth.setValues(values); REQUIRE(depth.channel == 1.5); REQUIRE(std::isnan(depth.mapping));
    values = JSValue::object(); values.set("version", 7.5); values.set("pmremVersion", -0.5);
    depth.setValues(values); REQUIRE(depth.version == 7.5); REQUIRE(depth.pmremVersion == -0.5);
    REQUIRE(depth.source->version == 0); depth.needsUpdate = true; REQUIRE(depth.version == 8.5);
    depth.needsPMREMUpdate = true; REQUIRE(depth.pmremVersion == 0.5);
}
TEST_CASE("Texture JSON applies Object.keys to primitive userData") {
    Texture texture;
    for (const JSValue value : {JSValue(3), JSValue(true), JSValue("")}) {
        texture.userData = value; REQUIRE(!texture.toJSON().contains("userData"));
    }
    texture.userData = "abc"; REQUIRE(texture.toJSON().at("userData").get<std::string>() == "abc");
    texture.userData = nullptr; REQUIRE_THROWS_AS(texture.toJSON(), JSTypeError);
}
TEST_CASE("Texture setValues keeps its input when it is the overwritten userData field") {
    Texture texture;
    texture.userData = JSValue::fromJSON(JsonValue::Object{
        {"userData", JsonValue::Object{{"rotation", 4}}}, {"rotation", 2}});
    texture.setValues(texture.userData);
    REQUIRE(texture.rotation == 2); REQUIRE(texture.userData.get("rotation").number() == 4);
}
TEST_CASE("Compressed cube matches upstream failed default construction on clone") {
    auto face = std::make_shared<TextureImage>(4, 4);
    CompressedCubeTexture cube(TextureImage::array({face, face, face, face, face, face}));
    REQUIRE_THROWS_AS(cube.clone(), JSTypeError);
    CompressedTexture texture; REQUIRE_THROWS_AS(texture.clone(), JSTypeError);
}
TEST_CASE("Texture setValues copies vectors, reads inherited values and skips setter-only properties") {
    Texture texture;
    auto prototype = JSValue::object(); prototype.set("rotation", 0.25);
    auto values = JSValue::object(prototype);
    values.set("offset", JSValue::fromJSON(JsonValue::Object{{"isVector2", true}, {"x", 0.1}, {"y", 0.2}}));
    values.set("needsUpdate", true); values.set("missing", 5); values.set("minFilter", JSValue());
    texture.setValues(values);
    REQUIRE(texture.rotation == 0.25); REQUIRE(texture.offset.x == 0.1); REQUIRE(texture.offset.y == 0.2);
    REQUIRE(texture.version == 0); REQUIRE(texture.minFilter == LinearMipmapLinearFilter);
    values = JSValue::object(); values.set("width", 9);
    REQUIRE_THROWS_AS(texture.setValues(values), JSTypeError);
}
TEST_CASE("Mesh material textures share the scene cache and preserve serializer callback order") {
    struct CountingTexture : DataTexture {
        using DataTexture::DataTexture;
        mutable std::vector<std::string>* calls = nullptr;
        mutable TextureJSONContext* seen = nullptr;
        JsonValue toJSON(TextureJSONContext* meta = nullptr) const override {
            if (calls) calls->push_back(name);
            seen = meta;
            return DataTexture::toJSON(meta);
        }
    };
    auto texture = std::make_shared<CountingTexture>(Uint8Array{255, 0, 0, 255}, 1, 1);
    texture->name = "map";
    auto alpha = std::make_shared<CountingTexture>(); alpha->source = texture->source; alpha->name = "alpha";
    auto material = std::make_shared<MeshBasicMaterial>();
    material->map = texture; material->alphaMap = alpha; material->lightMap = texture;
    material->lightMapIntensity = 0.5; material->envMap = texture; material->combine = MixOperation;
    std::vector<std::string> calls; texture->calls = &calls; alpha->calls = &calls;
    auto geometry = std::make_shared<BufferGeometry>();
    auto mesh1 = std::make_shared<Mesh>(geometry, material), mesh2 = std::make_shared<Mesh>(geometry, material);
    Scene scene; scene.add(mesh1); scene.add(mesh2);
    Object3DJSONContext meta;
    scene.toJSON(&meta);
    REQUIRE(meta.materials.size() == 1); REQUIRE(meta.textures.size() == 2); REQUIRE(meta.images.size() == 1);
    REQUIRE(texture->seen == &meta); REQUIRE(alpha->seen == &meta);
    REQUIRE((calls == std::vector<std::string>{"map", "alpha", "map", "map"}));
    REQUIRE(meta.materials.at(material->uuid).at("lightMapIntensity").get<double>() == 0.5);
    REQUIRE(meta.materials.at(material->uuid).at("combine").get<double>() == MixOperation);
    REQUIRE(!meta.materials.at(material->uuid).contains("textures"));
    auto root = material->toJSON();
    REQUIRE(root.at("textures").size() == 2); REQUIRE(root.at("images").size() == 1);
    REQUIRE(!root.at("textures")[0].contains("metadata"));
}
TEST_CASE("Scene environment cloning shares the image source and excludes render target JSON") {
    Scene scene;
    scene.environment = std::make_shared<DataTexture>(Uint8Array{1, 2, 3, 4}, 1, 1);
    auto cloned = std::dynamic_pointer_cast<Scene>(scene.clone());
    REQUIRE(cloned->environment != scene.environment);
    REQUIRE(cloned->environment->source == scene.environment->source);
    REQUIRE(scene.environment->source->version == 1);
    auto document = scene.toJSON();
    REQUIRE(document.at("object").at("environment").get<std::string>() == scene.environment->uuid);
    REQUIRE(document.at("textures").size() == 1); REQUIRE(document.at("images").size() == 1);
    scene.environment->isRenderTargetTexture = true;
    document = scene.toJSON();
    REQUIRE(!document.at("object").contains("environment")); REQUIRE(!document.contains("textures"));
}
namespace {
thread_local TextureImage* textureSerializationImage = nullptr;
JSValue replaceTextureImageData(const JSValue&, const std::vector<JSValue>&,
                               const std::vector<JSValue>& captures) {
    auto& image = *textureSerializationImage;
    const auto replacement = captures[0].number();
    if (replacement == 0) image.data = JSValue::fromJSON(JsonValue::Array{7, 8});
    else if (replacement == 1) image.data = Uint8Array{7, 8};
    else image.data = nullptr;
    image.width = 4; image.height = 5;
    return 1;
}
}
TEST_CASE("TextureSource retains input pixels across callback replacement and rereads image metadata") {
    for (int replacement = 0; replacement < 3; ++replacement) {
        auto pixels = JSValue::fromJSON(JsonValue::Array{0, 2, 3});
        JSPropertyDescriptor first;
        first.enumerable = true; first.configurable = true;
        first.getter = JSValue::function(replaceTextureImageData, {replacement});
        pixels.defineProperty("0", first);
        auto image = std::make_shared<TextureImage>(pixels, 1, 1);
        textureSerializationImage = image.get();
        TextureSource source(image);
        TextureJSONContext context;
        if (replacement == 2) {
            REQUIRE_THROWS_AS(source.toJSON(&context), JSTypeError);
            REQUIRE(context.images.empty());
        } else {
            auto output = source.toJSON(&context);
            REQUIRE(output.at("url").at("data") == JsonValue(JsonValue::Array{1, 2, 3}));
            REQUIRE(output.at("url").at("width").get<double>() == 4);
            REQUIRE(output.at("url").at("height").get<double>() == 5);
            REQUIRE(output.at("url").at("type").get<std::string>() == (replacement == 0 ? "Array" : "Uint8Array"));
            image->data = nullptr;
            output["url"]["width"] = 77;
            REQUIRE(source.toJSON(&context).at("url").at("width").get<double>() == 77);
        }
        textureSerializationImage = nullptr;
    }
}
TEST_CASE("Texture source callbacks retain source lifetime and cache under the final UUID") {
    struct ReplacingSource : TextureSource {
        Texture* texture = nullptr;
        JsonValue toJSON(TextureJSONContext* meta = nullptr) const override {
            texture->uuid = "after"; texture->name = "changed"; texture->mapping = CubeReflectionMapping;
            texture->source = std::make_shared<TextureSource>();
            return TextureSource::toJSON(meta);
        }
    };
    Texture texture; texture.uuid = "before"; texture.name = "original";
    auto source = std::make_shared<ReplacingSource>(); source->texture = &texture; source->uuid = "original-source";
    std::weak_ptr<TextureSource> lifetime = source;
    texture.source = std::move(source);
    TextureJSONContext context;
    auto output = texture.toJSON(&context);
    REQUIRE(lifetime.expired());
    REQUIRE(output.at("uuid").get<std::string>() == "before");
    REQUIRE(output.at("name").get<std::string>() == "original");
    REQUIRE(output.at("image").get<std::string>() == "original-source");
    REQUIRE(output.at("mapping").get<double>() == CubeReflectionMapping);
    REQUIRE(context.textures.contains("after")); REQUIRE_FALSE(context.textures.contains("before"));
    output["name"] = "cached";
    REQUIRE(texture.toJSON(&context).at("name").get<std::string>() == "cached");
}

TEST_CASE("Material JSON reads fields in upstream order across texture serializer callbacks") {
    struct CallbackTexture : Texture {
        std::function<void()> callback;
        JsonValue toJSON(TextureJSONContext* meta = nullptr) const override {
            if (callback) callback();
            return Texture::toJSON(meta);
        }
    };
    MeshBasicMaterial material;
    material.name = "before"; material.opacity = .75;
    material.userData = JsonValue::Object{{"stage", "before"}};
    material.clippingPlanes = std::vector<std::shared_ptr<Plane>>{nullptr};
    auto map = std::make_shared<CallbackTexture>(), alpha = std::make_shared<CallbackTexture>(),
         environment = std::make_shared<CallbackTexture>();
    map->uuid = "map"; alpha->uuid = "alpha"; environment->uuid = "environment";
    std::vector<std::string> calls;
    material.map = map;
    map->callback = [&] {
        calls.emplace_back("map"); material.name = "after"; material.opacity = .25;
        material.color.set(1,0,0); material.alphaMap = alpha;
        material.userData = JsonValue::Object{{"stage", "map"}};
    };
    alpha->callback = [&] { calls.emplace_back("alpha"); material.envMap = environment; };
    environment->callback = [&] {
        calls.emplace_back("environment"); material.envMap.reset();
        material.combine = MixOperation; material.envMapRotation.set(.1,.2,.3);
        material.reflectivity = .4; material.refractionRatio = .7;
        material.clippingPlanes = std::vector<std::shared_ptr<Plane>>{std::make_shared<Plane>(Vector3(0,1,0),4)};
        material.wireframe = true; material.fog = false;
        material.userData = JsonValue::Object{{"stage", "environment"}};
    };
    auto output = material.toJSON();
    REQUIRE((calls == std::vector<std::string>{"map", "alpha", "environment"}));
    REQUIRE(output.at("name").get<std::string>() == "before");
    REQUIRE(output.at("opacity").get<double>() == .75);
    REQUIRE(output.at("color").get<double>() == 0xffffff);
    REQUIRE(output.at("alphaMap").get<std::string>() == "alpha");
    REQUIRE(output.at("envMap").get<std::string>() == "environment");
    REQUIRE(output.at("combine").get<double>() == MixOperation);
    REQUIRE(output.at("envMapRotation")[0].get<double>() == .1);
    REQUIRE(output.at("reflectivity").get<double>() == .4);
    REQUIRE(output.at("refractionRatio").get<double>() == .7);
    REQUIRE(output.at("clippingPlanes")[0].at("constant").get<double>() == 4);
    REQUIRE(output.at("wireframe").get<bool>());
    REQUIRE_FALSE(output.at("fog").get<bool>());
    REQUIRE(output.at("userData").at("stage").get<std::string>() == "environment");
    material.userData["later"] = 9;
    REQUIRE(output.at("userData").at("later").get<double>() == 9);
    REQUIRE(output.at("textures").size() == 3);
    REQUIRE_FALSE(output.at("textures")[0].contains("metadata"));
}
TEST_CASE("Material JSON delays invalid late fields until after texture callbacks") {
    struct ClearingTexture : Texture {
        MeshBasicMaterial* owner = nullptr;
        JsonValue toJSON(TextureJSONContext* meta = nullptr) const override {
            owner->clippingPlanes.reset(); owner->userData = JsonValue::Object{};
            return Texture::toJSON(meta);
        }
    };
    MeshBasicMaterial material;
    material.clippingPlanes = std::vector<std::shared_ptr<Plane>>{nullptr}; material.userData = nullptr;
    auto texture = std::make_shared<ClearingTexture>(); texture->owner = &material; material.map = texture;
    const auto output = material.toJSON();
    REQUIRE_FALSE(output.contains("clippingPlanes")); REQUIRE_FALSE(output.contains("userData"));
    material.map.reset(); material.userData = "abc";
    REQUIRE(material.toJSON().at("userData").get<std::string>() == "abc");
    material.userData = nullptr;
    REQUIRE_THROWS_AS(material.toJSON(), JSTypeError);
}
TEST_CASE("Cube render targets own a six face cube texture") {
    WebGLCubeRenderTarget target(8);
    REQUIRE(target.isWebGLCubeRenderTarget);
    REQUIRE(target.width == 8);
    REQUIRE(target.height == 8);
    REQUIRE(target.texture.get() != nullptr);
    REQUIRE(target.texture.get()->isRenderTargetTexture);
    auto cube = std::dynamic_pointer_cast<CubeTexture>(target.texture.get());
    REQUIRE(cube != nullptr);
    REQUIRE(cube->images.get()->faces.size() == 6);
    for (const auto& image : cube->images.get()->faces) {
        REQUIRE(image->width == 8);
        REQUIRE(image->height == 8);
        REQUIRE(image->depth == 1);
    }
    // The two methods that need GPU work report that instead of pretending to succeed.
    const auto renderer = JSValue::object();
    REQUIRE_THROWS_AS(target.fromEquirectangularTexture(renderer, JSValue::object()), std::logic_error);
    REQUIRE_THROWS_AS(target.clear(renderer), std::logic_error);
}