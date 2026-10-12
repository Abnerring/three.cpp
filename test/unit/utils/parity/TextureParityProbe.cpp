#include "Three.Core.h"
#include <iostream>
#include <limits>

JsonValue number(double x) {
    if (std::isnan(x)) return "NaN";
    if (std::isinf(x)) return x < 0 ? "-Infinity" : "Infinity";
    if (x == 0 && std::signbit(x)) return "-0";
    return x;
}
JsonValue state(Texture& texture) {
    texture.uuid = "texture"; texture.source->uuid = "image";
    return JsonValue::Object{{"json", texture.toJSON()}, {"source", texture.source->toJSON()},
        {"size", JsonValue::Array{number(texture.width), number(texture.height), number(texture.depth)}},
        {"version", texture.version}, {"sourceVersion", texture.source->version}, {"pmremVersion", texture.pmremVersion},
        {"isArrayTexture", texture.isArrayTexture}, {"dataReady", texture.source->dataReady}};
}
int main() {
    JsonValue result = JsonValue::Object{};
    Texture texture; result["base"] = state(texture);
    DataTexture data(Uint8Array{1, 2, 3, 255, 5, 6, 7, 128}, 2, 1);
    data.name = "pixels"; data.needsUpdate = 1; data.needsUpdate = true;
    data.needsPMREMUpdate = true; data.needsPMREMUpdate = "true";
    data.source->dataReady = false; data.offset.set(0.2, -0.4); data.repeat.set(2, 3);
    data.center.set(0.5, 0.7); data.rotation = 0.6; data.updateMatrix();
    data.userData.set("value", JSValue::fromJSON(JsonValue::Array{1, nullptr, "a"}));
    data.addUpdateRange(-0.5, 7.2);
    auto cloned = data.clone();
    result["data"] = state(data); result["clone"] = state(*cloned);
    result["cloneIdentity"] = JsonValue::Array{cloned->source == data.source, cloned->image.get() == data.image.get(),
        cloned->mipmaps != data.mipmaps, cloned->updateRanges->empty(), cloned->userData.strictEquals(data.userData)};
    DataArrayTexture array(Uint16Array{1, 65535, 2, 9}, 2, 1, 2); result["array"] = state(array);
    Data3DTexture volume(Float32Array{0.25, 0.5, 0.75, 1}, 1, 1, 4); result["volume"] = state(volume);
    DepthTexture depth(8, 4, UnsignedIntType, UVMapping, ClampToEdgeWrapping, ClampToEdgeWrapping,
                       NearestFilter, NearestFilter, 1, DepthFormat, 3);
    depth.compareFunction = LessEqualCompare; auto depthClone = depth.clone();
    result["depth"] = state(depth); result["depthClone"] = state(*depthClone);
    FramebufferTexture framebuffer(10, 20); auto framebufferClone = framebuffer.clone();
    result["framebuffer"] = state(framebuffer); result["framebufferClone"] = state(*framebufferClone);
    CubeDepthTexture cubeDepth(16); auto cubeDepthClone = cubeDepth.clone();
    result["cubeDepth"] = state(cubeDepth); result["cubeDepthClone"] = state(*cubeDepthClone);
    result["cubeDepthIdentity"] = JsonValue::Array{cubeDepth.image->faces[0] == cubeDepth.image->faces[5],
        cubeDepthClone->image->isArray, cubeDepthClone->image->faces[0] == cubeDepth.image->faces[0]};
    auto face = std::make_shared<TextureImage>(Uint8Array{1, 2, 3, 4}, 1, 1);
    CubeTexture cube(TextureImage::array({face, face, face, face, face, face})); result["cube"] = state(cube);
    auto levels = std::make_shared<std::vector<Texture::Image>>(); levels->push_back(face);
    CompressedTexture compressed(levels, 1, 1, RGBA_S3TC_DXT5_Format); result["compressed"] = state(compressed);
    CompressedArrayTexture compressedArray(levels, 1, 1, 6, RGBA_S3TC_DXT5_Format); result["compressedArray"] = state(compressedArray);
    CompressedCubeTexture compressedCube(TextureImage::array({face, face, face, face, face, face}), RGBA_S3TC_DXT5_Format);
    result["compressedCube"] = state(compressedCube);
    try { compressedCube.clone(); result["cubeCloneError"] = "none"; } catch (const JSTypeError&) { result["cubeCloneError"] = "TypeError"; }
    array.addLayerUpdate(2); array.addLayerUpdate(-0.0); array.addLayerUpdate(std::numeric_limits<double>::quiet_NaN());
    array.addLayerUpdate(2); array.addLayerUpdate(0); array.addLayerUpdate(std::numeric_limits<double>::quiet_NaN());
    JsonValue::Array layerValues; for (auto value : *array.layerUpdates) layerValues.push_back(number(value));
    result["layerUpdates"] = std::move(layerValues);
    {
        auto values = JSValue::object(); values.set("wrapR", MirroredRepeatWrapping);
        array.setValues(values); volume.setValues(values); compressedArray.setValues(values);
        values = JSValue::object(); values.set("compareFunction", GreaterEqualCompare);
        depth.setValues(values); cubeDepth.setValues(values);
        result["setValuesDerived"] = JsonValue::Array{array.wrapR, volume.wrapR, compressedArray.wrapR,
            *depth.compareFunction, *cubeDepth.compareFunction};
        values.set("compareFunction", nullptr); depth.setValues(values);
        result["setValuesNullCompare"] = !depth.compareFunction;
        values = JSValue::object(); values.set("version", 7.5); values.set("pmremVersion", -0.5);
        Texture texture; texture.setValues(values);
        result["setValuesVersions"] = JsonValue::Array{texture.version, texture.pmremVersion, texture.source->version};
        texture.needsUpdate = true; texture.needsPMREMUpdate = true;
        result["incrementVersions"] = JsonValue::Array{texture.version, texture.pmremVersion, texture.source->version};
        JsonValue::Array primitiveData;
        for (const JSValue value : {JSValue(3), JSValue(true), JSValue(""), JSValue("abc")}) {
            texture.userData = value; const auto json = texture.toJSON();
            primitiveData.push_back(json.contains("userData") ? json.at("userData") : JsonValue{});
        }
        result["primitiveUserData"] = std::move(primitiveData);
    }
    {
        auto map = std::make_shared<DataTexture>(Uint8Array{200, 80, 40, 255}, 1, 1);
        map->uuid = "map"; map->source->uuid = "shared-image";
        auto alpha = map->clone(); alpha->uuid = "alpha";
        auto material = std::make_shared<MeshBasicMaterial>(); material->uuid = "material";
        material->map = map; material->alphaMap = alpha; material->lightMap = map;
        material->lightMapIntensity = 2; material->aoMap = alpha; material->aoMapIntensity = 0.25;
        material->specularMap = map; material->envMap = alpha; material->combine = AddOperation;
        auto geometry = std::make_shared<BufferGeometry>();
        Scene scene; scene.add(std::make_shared<Mesh>(geometry, material)); scene.add(std::make_shared<Mesh>(geometry, material));
        const auto document = scene.toJSON(); const auto standalone = material->toJSON();
        JsonValue links = JsonValue::Object{};
        for (const auto* field : {"map", "alphaMap", "lightMap", "lightMapIntensity", "aoMap", "aoMapIntensity", "specularMap", "envMap", "combine"})
            links[field] = document.at("materials")[0].at(field);
        result["materialLinks"] = std::move(links);
        result["sceneTextures"] = document.at("textures"); result["sceneImages"] = document.at("images");
        result["standaloneTextures"] = standalone.at("textures"); result["standaloneImages"] = standalone.at("images");
        result["sceneMaterialCount"] = document.at("materials").size();
    }
    JsonValue::Array uvValues;
    for (int mode : {UVMapping, CubeReflectionMapping})
    for (int s : {RepeatWrapping, ClampToEdgeWrapping, MirroredRepeatWrapping})
    for (int t : {RepeatWrapping, ClampToEdgeWrapping, MirroredRepeatWrapping})
    for (bool flip : {false, true}) {
        data.mapping = mode; data.wrapS = s; data.wrapT = t; data.flipY = flip;
        for (int i = -17; i <= 17; ++i) {
            Vector2 uv(i / 5.0, (i * i % 11 - 5) / 3.0);
            data.transformUv(uv); uvValues.push_back(number(uv.x)); uvValues.push_back(number(uv.y));
        }
    }
    result["uv"] = std::move(uvValues);
    {
        Texture texture;
        texture.userData = JSValue::fromJSON(JsonValue::Object{
            {"userData", JsonValue::Object{{"rotation", 4}}}, {"rotation", 2}});
        texture.setValues(texture.userData);
        result["aliasedSetValues"] = JsonValue::Array{texture.rotation, texture.userData.get("rotation").number()};
    }
    {
        static TextureImage* currentImage;
        JsonValue::Array serialized;
        for (int replacement = 0; replacement < 3; ++replacement) {
            auto pixels = JSValue::fromJSON(JsonValue::Array{0, 2, 3});
            JSPropertyDescriptor first; first.enumerable = true; first.configurable = true;
            first.getter = JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                               const std::vector<JSValue>& captures) -> JSValue {
                if (captures[0].number() == 0) currentImage->data = JSValue::fromJSON(JsonValue::Array{7, 8});
                else if (captures[0].number() == 1) currentImage->data = Uint8Array{7, 8};
                else currentImage->data = nullptr;
                currentImage->width = 4; currentImage->height = 5;
                return 1;
            }, {replacement});
            pixels.defineProperty("0", first);
            auto image = std::make_shared<TextureImage>(pixels, 1, 1); currentImage = image.get();
            TextureSource source(image);
            try { serialized.push_back(source.toJSON().at("url")); }
            catch (const JSTypeError&) { serialized.emplace_back("TypeError"); }
            currentImage = nullptr;
        }
        result["reentrantSourcePixels"] = std::move(serialized);
    }
    {
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
        texture.source = std::move(source);
        TextureJSONContext context;
        auto output = texture.toJSON(&context);
        result["reentrantSourceCache"] = JsonValue::Array{output.at("uuid"), output.at("name"), output.at("image"),
            output.at("mapping"), context.textures.contains("after"), context.textures.contains("before")};
        output["name"] = "cached";
        result["reentrantCachedName"] = texture.toJSON(&context).at("name");
    }
    {
        Material base; base.uuid = "base";
        result["baseMaterialJSON"] = base.toJSON();
        MeshBasicMaterial basic; basic.uuid = "basic";
        result["basicMaterialJSON"] = basic.toJSON();
    }
    {
        struct CallbackTexture : Texture {
            std::function<void()> callback;
            JsonValue toJSON(TextureJSONContext* meta = nullptr) const override {
                if (callback) callback();
                return Texture::toJSON(meta);
            }
        };
        MeshBasicMaterial material; material.uuid = "ordered-material";
        material.name = "before"; material.opacity = .75;
        material.userData = JsonValue::Object{{"stage", "before"}};
        material.clippingPlanes = std::vector<std::shared_ptr<Plane>>{std::make_shared<Plane>(Vector3(1,0,0),1)};
        JsonValue::Array calls;
        auto image = std::make_shared<TextureSource>(); image->uuid = "ordered-image";
        const auto make = [&](const char* name) {
            auto texture = std::make_shared<CallbackTexture>();
            texture->uuid = name; texture->name = name; texture->source = image;
            texture->callback = [&, name] { calls.emplace_back(name); };
            return texture;
        };
        auto map = make("map"), alpha = make("alpha"), light = make("light"), ao = make("ao"),
             specular = make("specular"), unused = make("unused"), environment = make("environment");
        material.map = map; material.lightMap = light; material.aoMap = ao;
        material.specularMap = specular; material.envMap = unused;
        map->callback = [&] {
            calls.emplace_back("map"); material.name = "after"; material.opacity = .25;
            material.color.set(1,0,0); material.alphaMap = alpha;
            material.userData = JsonValue::Object{{"stage", "map"}};
        };
        light->callback = [&] { calls.emplace_back("light"); material.lightMapIntensity = 2; };
        ao->callback = [&] { calls.emplace_back("ao"); material.aoMapIntensity = 3; };
        specular->callback = [&] { calls.emplace_back("specular"); material.envMap = environment; };
        environment->callback = [&] {
            calls.emplace_back("environment"); material.envMap.reset(); material.combine = MixOperation;
            material.envMapRotation.set(.1,.2,.3); material.reflectivity = .4; material.refractionRatio = .7;
            material.clippingPlanes = std::vector<std::shared_ptr<Plane>>{std::make_shared<Plane>(Vector3(0,1,0),4)};
            material.wireframe = true; material.wireframeLinewidth = 2;
            material.wireframeLinecap = "square"; material.wireframeLinejoin = "bevel"; material.fog = false;
            material.userData = JsonValue::Object{{"stage", "environment"}};
        };
        auto output = material.toJSON();
        result["materialSerializationOrder"] = output;
        result["materialSerializationCalls"] = std::move(calls);
        material.userData["later"] = 9;
        result["materialUserDataAlias"] = output.at("userData").at("later");
    }
    {
        JsonValue::Array values;
        for (const JsonValue& value : {JsonValue(3),JsonValue(true),JsonValue(""),JsonValue("abc"),JsonValue(nullptr)}) {
            Material material; material.userData = value;
            try { auto output = material.toJSON(); values.push_back(output.contains("userData") ? output.at("userData") : JsonValue(nullptr)); }
            catch (const JSTypeError&) { values.emplace_back("TypeError"); }
        }
        result["materialPrimitiveUserData"] = std::move(values);
        struct ClearingTexture : Texture {
            MeshBasicMaterial* owner = nullptr;
            JsonValue toJSON(TextureJSONContext* meta = nullptr) const override {
                owner->clippingPlanes.reset(); owner->userData = JsonValue::Object{};
                return Texture::toJSON(meta);
            }
        };
        MeshBasicMaterial material;
        material.clippingPlanes = std::vector<std::shared_ptr<Plane>>{nullptr};
        material.userData = nullptr;
        auto texture = std::make_shared<ClearingTexture>(); texture->owner = &material;
        material.map = texture;
        auto output = material.toJSON();
        result["materialCallbackClearsInvalidLateFields"] = !output.contains("clippingPlanes") && !output.contains("userData");
    }
    std::cout << result.dump();
}
