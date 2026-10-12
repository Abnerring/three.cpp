#pragma once
#include "loaders/Loader.h"
#include "materials/Material.h"
class MaterialLoader : public Loader {
  public:
    explicit MaterialLoader(JSValue manager={});
    static JSValue prototype();
    JSMember textures{&object_,"textures"};
    using Loader::parse;
    std::shared_ptr<Material> parseMaterial(JSValue json);
    std::shared_ptr<Material> parse(const JsonValue& json){return parseMaterial(JSValue::fromJSON(json));}
    MaterialLoader& setTextures(JSValue value);
    MaterialLoader& setTextures(const OrderedMap<std::shared_ptr<Texture>>& value);
    static JSValue createMaterialFromType(JSValue type);
    static void registerMaterial(JSValue type,JSValue materialClass);
};
