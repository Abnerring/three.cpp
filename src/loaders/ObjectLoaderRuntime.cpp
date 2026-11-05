// Native translation of pinned src/loaders/ObjectLoader.js. The typed facade is separate.
#include "loaders/ObjectLoader.h"
#include "loaders/ObjectLoaderSources.inc"
#include "loaders/ObjectLoaderGeometries.h"
#include "loaders/ObjectLoaderShapes.h"
#include "core/JSAsyncFunction.h"
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSNumber.h"
#include "core/NativeObject.h"
#include "core/Object3D.h"
#include "core/InstancedBufferAttribute.h"
#include "core/InterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "loaders/Loader.h"
#include "loaders/LoaderUtils.h"
#include "loaders/FileLoader.h"
#include "loaders/ImageLoader.h"
#include "loaders/BufferGeometryLoader.h"
#include "loaders/MaterialLoader.h"
#include "animation/AnimationClip.h"
#include "animation/AnimationRuntime.h"
#include "objects/Group.h"
#include "objects/LOD.h"
#include "objects/Line.h"
#include "objects/LineLoop.h"
#include "objects/LineSegments.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include "objects/Mesh.h"
#include "objects/SkinnedMesh.h"
#include "objects/InstancedMesh.h"
#include "objects/BatchedMesh.h"
#include "objects/Bone.h"
#include "objects/Skeleton.h"
#include "scenes/Scene.h"
#include "scenes/Fog.h"
#include "scenes/FogExp2.h"
#include "cameras/PerspectiveCamera.h"
#include "cameras/OrthographicCamera.h"
#include "lights/AmbientLight.h"
#include "lights/DirectionalLight.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "lights/HemisphereLight.h"
#include "lights/RectAreaLight.h"
#include "lights/LightProbe.h"
#include "math/Box3.h"
#include "math/Sphere.h"
#include "math/SphericalHarmonics3.h"
#include "math/Color.h"
#include "math/Vector3.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include "math/Matrix4.h"
#include "math/Frustum.h"
#include "textures/CubeTexture.h"
#include "textures/DataTexture.h"
#include "textures/Texture.h"
#include "textures/TextureSource.h"
#include "utils.h"
#include <cmath>
#include <regex>
#include <tuple>
#include <unordered_set>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& a, std::size_t i) { return i < a.size() ? a[i] : JSValue{}; }
struct Invocation {
    JSValue receiver, method;
    JSValue operator()(const Args& args = {}) const { return method.call(receiver, args); }
};
Invocation invoke(const JSValue& receiver, const char* name) { return {receiver, receiver.get(name)}; }
void own(JSValue object, const char* name, const JSValue& value) {
    JSPropertyDescriptor d; d.value = value; d.writable = d.configurable = d.enumerable = true;
    object.defineProperty(name, d);
}
JSValue plus(const JSValue& left, const JSValue& right) {
    const auto a=left.toPrimitive(), b=right.toPrimitive();
    if(a.type()==JSValue::Type::String || b.type()==JSValue::Type::String) {
        const auto first=a.toString();return first+b.toString();
    }
    const auto first=a.toNumber();return first+b.toNumber();
}
JSValue index(const JSValue& object, double i) { return object.get(JSValue(i)); }
bool isArray(const JSValue& value) { return invoke(nativeGlobalBinding("Array"), "isArray")({value}).toBoolean(); }
bool isImage(const JSValue& value) { return value.instanceOf(nativeGlobalBinding("HTMLImageElement")); }
class ForInCursor {
    JSValue object_; std::vector<JSValue> keys_; std::unordered_set<std::string> seen_;
    std::size_t index_=0; bool loaded_=false;
  public:
    explicit ForInCursor(JSValue source):object_(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()) {}
    std::optional<JSValue> next() {
        while(!object_.isNull()) {
            if(!loaded_) {keys_=object_.ownPropertyKeys();index_=0;loaded_=true;}
            while(index_<keys_.size()) {
                const auto key=keys_[index_++];if(key.isSymbol())continue;
                if(seen_.contains(key.string()))continue;
                const auto descriptor=object_.getOwnPropertyDescriptor(key);if(!descriptor)continue;
                seen_.insert(key.string());if(descriptor->enumerable.value_or(false))return key;
            }
            object_=object_.getPrototype();loaded_=false;
        }
        return std::nullopt;
    }
};
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSValue environment() { nativeObjectLoaderModule(); return *lexical().lock(); }
JSValue imported(const char* name) { return environment().get(name); }
JSValue parseJSON(const JSValue& text) { return invoke(nativeGlobalBinding("JSON"), "parse")({text}); }
JSValue configureFileLoader(const JSValue& self, const JSValue& url) {
    const auto pathTest=self.get("path");
    const auto path=pathTest.strictEquals("") ? LoaderUtils::extractUrlBase(url) : self.get("path");
    auto target=self;
    const auto resourcePath=self.get("resourcePath");
    target.set("resourcePath",resourcePath.toBoolean()?resourcePath:path);
    const auto loader=imported("FileLoader").construct({self.get("manager")});
    invoke(loader,"setPath")({self.get("path")});
    invoke(loader,"setRequestHeader")({self.get("requestHeader")});
    invoke(loader,"setWithCredentials")({self.get("withCredentials")});
    return loader;
}
bool invalidMetadata(const JSValue& json) {
    const auto metadata=json.get("metadata");
    if(metadata.isUndefined())return true;
    if(metadata.get("type").isUndefined())return true;
    return invoke(metadata.get("type"),"toLowerCase")({}).strictEquals("geometry");
}
JSValue load(const JSValue& self,const Args& args,const Args&) {
    const auto url=arg(args,0), onLoad=arg(args,1), onProgress=arg(args,2), onError=arg(args,3);
    const auto loader=configureFileLoader(self,url);
    auto callback=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
        JSValue json=nullptr;
        try { json=parseJSON(arg(a,0)); }
        catch(...) {
            const auto error=nativeExceptionValue(std::current_exception());
            if(!c[2].isUndefined())c[2].call({}, {error});
            const auto message=plus(plus("ObjectLoader: Can't parse ",c[3]),".");
            ThreeUtils::error({message,error.get("message")});
            return {};
        }
        if(invalidMetadata(json)) {
            if(!c[2].isUndefined()) {
                const auto errorConstructor=nativeGlobalBinding("Error");
                c[2].call({}, {errorConstructor.construct({plus("THREE.ObjectLoader: Can't load ",c[3])})});
            }
            ThreeUtils::error({plus("ObjectLoader: Can't load ",c[3])});
            return {};
        }
        invoke(c[0],"parse")({json,c[1]});
        return {};
    }, {self,onLoad,onError,url}, "",1);
    callback.setFunctionSourceText(ObjectLoaderSource::callback1Source);
    invoke(loader,"load")({url,callback,onProgress,onError});
    return {};
}
JSAsyncStep loadAsyncStep(const JSValue& frame,const JSAsyncCompletion& completion,
                         const JSValue& self,const Args& args,const Args&) {
    auto state=frame;
    const auto pc=state.get("pc");
    if(pc.isUndefined()) {
        const auto loader=configureFileLoader(self,arg(args,0));
        state.set("pc",1);
        return JSAsyncStep::awaitValue(invoke(loader,"loadAsync")({arg(args,0),arg(args,1)}));
    }
    if(completion.kind==JSAsyncCompletionKind::Throw)return JSAsyncStep::abrupt(completion.value);
    if(pc.strictEquals(1)) {
        JSValue json;
        try { json=parseJSON(completion.value); }
        catch(...) {
            const auto error=nativeExceptionValue(std::current_exception());
            const auto errorConstructor=nativeGlobalBinding("Error");
            auto message=plus(plus("THREE.ObjectLoader: Can't parse ",arg(args,0)),". ");
            message=plus(message,error.get("message"));
            return JSAsyncStep::abrupt(errorConstructor.construct({message}));
        }
        if(invalidMetadata(json)) {
            const auto errorConstructor=nativeGlobalBinding("Error");
            return JSAsyncStep::abrupt(errorConstructor.construct({plus("THREE.ObjectLoader: Can't load ",arg(args,0))}));
        }
        state.set("pc",2);
        return JSAsyncStep::awaitValue(invoke(self,"parseAsync")({json}));
    }
    return JSAsyncStep::complete(completion.value);
}
JSValue finishParse(const JSValue& self,const JSValue& json,const JSValue& animations,
                    const JSValue& geometries,const JSValue& images) {
    const auto textures=invoke(self,"parseTextures")({json.get("textures"),images});
    const auto materials=invoke(self,"parseMaterials")({json.get("materials"),textures});
    const auto object=invoke(self,"parseObject")({json.get("object"),geometries,materials,textures,animations});
    const auto skeletons=invoke(self,"parseSkeletons")({json.get("skeletons"),object});
    invoke(self,"bindSkeletons")({object,skeletons});
    invoke(self,"bindLightTargets")({object});
    return object;
}
JSValue parse(const JSValue& self,const Args& args,const Args&) {
    const auto json=arg(args,0), onLoad=arg(args,1);
    const auto animations=invoke(self,"parseAnimations")({json.get("animations")});
    const auto shapes=invoke(self,"parseShapes")({json.get("shapes")});
    const auto geometries=invoke(self,"parseGeometries")({json.get("geometries"),shapes});
    auto cell=JSValue::object(nullptr);
    auto callback=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        if(!c[0].isUndefined()) {
            // The source closure reads a lexical const declared after parseImages returns.
            if(!c[1].get("initialized").toBoolean())throw JSReferenceError("Cannot access 'object' before initialization");
            c[0].call({}, {c[1].get("object")});
        }
        return {};
    }, {onLoad,cell},"",0);
    callback.setFunctionSourceText(ObjectLoaderSource::callback2Source);
    const auto images=invoke(self,"parseImages")({json.get("images"),callback});
    // object is initialized before skeleton binding, matching the lexical declaration in the source.
    const auto textures=invoke(self,"parseTextures")({json.get("textures"),images});
    const auto materials=invoke(self,"parseMaterials")({json.get("materials"),textures});
    const auto object=invoke(self,"parseObject")({json.get("object"),geometries,materials,textures,animations});
    cell.set("object",object); cell.set("initialized",true);
    const auto skeletons=invoke(self,"parseSkeletons")({json.get("skeletons"),object});
    invoke(self,"bindSkeletons")({object,skeletons});
    invoke(self,"bindLightTargets")({object});
    if(!onLoad.isUndefined()) {
        bool hasImages=false;
        ForInCursor keys(images);
        while(const auto uuid=keys.next()) {
            if(isImage(images.get(*uuid).get("data"))) { hasImages=true;break; }
        }
        if(!hasImages)onLoad.call({}, {object});
    }
    return object;
}
JSAsyncStep parseAsyncStep(const JSValue& frame,const JSAsyncCompletion& completion,
                          const JSValue& self,const Args& args,const Args&) {
    auto state=frame; const auto json=arg(args,0);
    if(state.get("pc").isUndefined()) {
        const auto animations=invoke(self,"parseAnimations")({json.get("animations")});
        const auto shapes=invoke(self,"parseShapes")({json.get("shapes")});
        const auto geometries=invoke(self,"parseGeometries")({json.get("geometries"),shapes});
        state.set("animations",animations);state.set("geometries",geometries);state.set("pc",1);
        return JSAsyncStep::awaitValue(invoke(self,"parseImagesAsync")({json.get("images")}));
    }
    if(completion.kind==JSAsyncCompletionKind::Throw)return JSAsyncStep::abrupt(completion.value);
    return JSAsyncStep::complete(finishParse(self,json,state.get("animations"),state.get("geometries"),completion.value));
}
JSValue parseShapes(const JSValue&,const Args& args,const Args&) {
    auto shapes=JSValue::object(); const auto json=arg(args,0);
    if(!json.isUndefined()) {
        const auto length=json.get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto shape=invoke(imported("Shape").construct(),"fromJSON")({index(json,i)});
            shapes.set(shape.get("uuid"),shape);
        }
    }
    return shapes;
}
JSValue parseSkeletons(const JSValue&,const Args& args,const Args&) {
    auto skeletons=JSValue::object(), bones=JSValue::object();
    auto callback=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
        const auto child=arg(a,0);
        if(child.get("isBone").toBoolean()) {auto bones=c[0];bones.set(child.get("uuid"),child);}
        return {};
    },{bones},"",1);
    callback.setFunctionSourceText(ObjectLoaderSource::callback3Source);
    invoke(arg(args,1),"traverse")({callback});
    const auto json=arg(args,0);
    if(!json.isUndefined()) {
        const auto length=json.get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto skeleton=invoke(imported("Skeleton").construct(),"fromJSON")({index(json,i),bones});
            skeletons.set(skeleton.get("uuid"),skeleton);
        }
    }
    return skeletons;
}
JSValue parseGeometries(const JSValue&,const Args& args,const Args&) {
    auto geometries=JSValue::object(); const auto json=arg(args,0), shapes=arg(args,1);
    if(!json.isUndefined()) {
        const auto loader=imported("BufferGeometryLoader").construct();
        const auto length=json.get("length");
        for(double i=0;i<length.toNumber();++i) {
            JSValue geometry; const auto data=index(json,i), type=data.get("type");
            if(type.strictEquals("BufferGeometry") || type.strictEquals("InstancedBufferGeometry")) {
                geometry=invoke(loader,"parse")({data});
            } else {
                const auto library=imported("Geometries");
                if(library.has(data.get("type")))geometry=invoke(library.get(data.get("type")),"fromJSON")({data,shapes});
                else {
                    const auto custom=imported("_customGeometries");
                    if(custom.has(data.get("type")))geometry=invoke(custom.get(data.get("type")),"fromJSON")({data,shapes});
                    else {
                        const auto typeName=data.get("type").toString();
                        ThreeUtils::warn({"ObjectLoader: Unknown geometry type \""+typeName+"\". Use .registerGeometry() before starting the deserialization process."});
                    }
                }
            }
            geometry.set("uuid",data.get("uuid"));
            if(!data.get("name").isUndefined())geometry.set("name",data.get("name"));
            if(!data.get("userData").isUndefined())geometry.set("userData",data.get("userData"));
            geometries.set(data.get("uuid"),geometry);
        }
    }
    return geometries;
}
JSValue parseMaterials(const JSValue&,const Args& args,const Args&) {
    auto cache=JSValue::object(), materials=JSValue::object();const auto json=arg(args,0);
    if(!json.isUndefined()) {
        const auto loader=imported("MaterialLoader").construct();
        invoke(loader,"setTextures")({arg(args,1)});
        const auto length=json.get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto data=index(json,i);
            if(cache.get(data.get("uuid")).isUndefined()) {
                const auto key=data.get("uuid");
                const auto material=invoke(loader,"parse")({data});cache.set(key,material);
            }
            const auto key=data.get("uuid");
            materials.set(key,cache.get(data.get("uuid")));
        }
    }
    return materials;
}
JSValue parseAnimations(const JSValue&,const Args& args,const Args&) {
    auto animations=JSValue::object();const auto json=arg(args,0);
    if(!json.isUndefined())for(double i=0;i<json.get("length").toNumber();++i) {
        const auto data=index(json,i);
        const auto clip=invoke(imported("AnimationClip"),"parse")({data});
        animations.set(clip.get("uuid"),clip);
    }
    return animations;
}

JSValue deserializeDataImage(const JSValue& image) {
    if(image.get("data").toBoolean()) {
        auto result=JSValue::object();
        const auto type=image.get("type");const auto data=image.get("data");
        own(result,"data",imported("TypedArrays").get(type).construct({data}));
        own(result,"width",image.get("width"));own(result,"height",image.get("height"));
        return result;
    }
    return nullptr;
}
JSValue imagePath(const JSValue& self,const JSValue& image) {
    static const std::regex absolute("^(//)|([a-z]+:(//)?)",std::regex::icase);
    if(std::regex_search(image.string(),absolute))return image;
    return plus(self.get("resourcePath"),image);
}
JSValue parseImages(const JSValue& self,const Args& args,const Args&) {
    auto images=JSValue::object();const auto json=arg(args,0);
    if(!json.isUndefined() && json.get("length").toNumber()>0) {
        const auto manager=imported("LoadingManager").construct({arg(args,1)});
        const auto loader=imported("ImageLoader").construct({manager});
        invoke(loader,"setCrossOrigin")({self.get("crossOrigin")});
        const auto deserialize=[&](const JSValue& image)->JSValue {
            if(image.type()!=JSValue::Type::String)return deserializeDataImage(image);
            auto url=imagePath(self,image);
            url=invoke(self.get("manager"),"resolveURL")({url});
            invoke(self.get("manager"),"itemStart")({url});
            auto done=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
                invoke(c[0].get("manager"),"itemEnd")({c[1]});return {};
            },{self,url},"",0);
            done.setFunctionSourceText(ObjectLoaderSource::callback5Source);
            auto error=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
                invoke(c[0].get("manager"),"itemError")({c[1]});
                invoke(c[0].get("manager"),"itemEnd")({c[1]});return {};
            },{self,url},"",0);
            error.setFunctionSourceText(ObjectLoaderSource::callback6Source);
            return invoke(loader,"load")({url,done,{},error});
        };
        const auto length=json.get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto image=index(json,i), url=image.get("url");
            if(isArray(url)) {
                auto imageArray=JSValue::array();const auto count=url.get("length");
                for(double j=0;j<count.toNumber();++j) {
                    const auto currentUrl=index(url,j);const auto decoded=deserialize(currentUrl);
                    if(!decoded.isNull()) {
                        if(isImage(decoded))invoke(imageArray,"push")({decoded});
                        else invoke(imageArray,"push")({imported("DataTexture").construct({decoded.get("data"),decoded.get("width"),decoded.get("height")})});
                    }
                }
                const auto key=image.get("uuid");images.set(key,imported("TextureSource").construct({imageArray}));
            } else {
                const auto decoded=deserialize(image.get("url"));
                const auto key=image.get("uuid");images.set(key,imported("TextureSource").construct({decoded}));
            }
        }
    }
    return images;
}

JSAsyncStep deserializeImageStep(const JSValue& frame,const JSAsyncCompletion& completion,
                                const JSValue&,const Args& args,const Args& captures) {
    auto state=frame;
    if(state.get("pc").isUndefined()) {
        const auto image=arg(args,0);
        if(image.type()!=JSValue::Type::String)return JSAsyncStep::complete(deserializeDataImage(image));
        const auto path=imagePath(captures[0],image);state.set("pc",1);
        return JSAsyncStep::awaitValue(invoke(captures[1],"loadAsync")({path}));
    }
    if(completion.kind==JSAsyncCompletionKind::Throw)return JSAsyncStep::abrupt(completion.value);
    return JSAsyncStep::complete(completion.value);
}
JSAsyncStep parseImagesAsyncStep(const JSValue& frame,const JSAsyncCompletion& completion,
                                const JSValue& self,const Args& args,const Args&) {
    auto state=frame;const auto json=arg(args,0);
    if(state.get("pc").isUndefined()) {
        state.set("images",JSValue::object());
        if(json.isUndefined() || !(json.get("length").toNumber()>0))return JSAsyncStep::complete(state.get("images"));
        const auto loader=imported("ImageLoader").construct({self.get("manager")});
        invoke(loader,"setCrossOrigin")({self.get("crossOrigin")});
        auto deserialize=createNativeAsyncStateFunction(deserializeImageStep,{self,loader},"deserializeImage",1,ObjectLoaderSource::callback8Source);
        state.set("deserialize",deserialize);state.set("i",0);state.set("il",json.get("length"));state.set("pc",1);
    } else {
        if(completion.kind==JSAsyncCompletionKind::Throw)return JSAsyncStep::abrupt(completion.value);
        const auto decoded=completion.value;
        if(state.get("pc").strictEquals(3)) {
            if(!decoded.isNull()) {
                const auto array=state.get("array");
                if(isImage(decoded))invoke(array,"push")({decoded});
                else invoke(array,"push")({imported("DataTexture").construct({decoded.get("data"),decoded.get("width"),decoded.get("height")})});
            }
            state.set("j",state.get("j").toNumber()+1);state.set("pc",2);
        } else {
            const auto key=state.get("image").get("uuid");
            auto images=state.get("images");images.set(key,imported("TextureSource").construct({decoded}));
            state.set("i",state.get("i").toNumber()+1);state.set("pc",1);
        }
    }
    while(true) {
        if(state.get("pc").strictEquals(1)) {
            if(!(state.get("i").toNumber()<state.get("il").toNumber()))return JSAsyncStep::complete(state.get("images"));
            const auto image=index(json,state.get("i").toNumber()), url=image.get("url");
            state.set("image",image);
            if(isArray(url)) {
                state.set("url",url);state.set("array",JSValue::array());state.set("j",0);state.set("jl",url.get("length"));state.set("pc",2);
            } else {
                state.set("pc",4);
                return JSAsyncStep::awaitValue(state.get("deserialize").call({}, {image.get("url")}));
            }
        }
        if(state.get("pc").strictEquals(2)) {
            if(state.get("j").toNumber()<state.get("jl").toNumber()) {
                const auto current=index(state.get("url"),state.get("j").toNumber());state.set("pc",3);
                return JSAsyncStep::awaitValue(state.get("deserialize").call({}, {current}));
            }
            const auto key=state.get("image").get("uuid");auto images=state.get("images");
            images.set(key,imported("TextureSource").construct({state.get("array")}));
            state.set("i",state.get("i").toNumber()+1);state.set("pc",1);
        }
    }
}

JSValue parseConstant(const JSValue& value,const JSValue& type) {
    if(value.type()==JSValue::Type::Number)return value;
    ThreeUtils::warn({"ObjectLoader.parseTexture: Constant should be in numeric form.",value});
    return type.get(value);
}
JSValue parseTextures(const JSValue&,const Args& args,const Args&) {
    auto textures=JSValue::object();const auto json=arg(args,0),images=arg(args,1);
    if(!json.isUndefined()) {
        const auto length=json.get("length");
        for(double i=0;i<length.toNumber();++i) {
            const auto data=index(json,i);
            if(data.get("image").isUndefined())ThreeUtils::warn({"ObjectLoader: No \"image\" specified for",data.get("uuid")});
            if(images.get(data.get("image")).isUndefined())ThreeUtils::warn({"ObjectLoader: Undefined image",data.get("image")});
            const auto source=images.get(data.get("image")), image=source.get("data");
            JSValue texture;
            if(isArray(image)) {
                texture=imported("CubeTexture").construct();
                if(image.get("length").strictEquals(6))texture.set("needsUpdate",true);
            } else {
                if(image.toBoolean() && image.get("data").toBoolean())texture=imported("DataTexture").construct();
                else texture=imported("Texture").construct();
                if(image.toBoolean())texture.set("needsUpdate",true);
            }
            texture.set("source",source);texture.set("uuid",data.get("uuid"));
            const auto copy=[&](const char* name){if(!data.get(name).isUndefined())texture.set(name,data.get(name));};
            copy("name");
            if(!data.get("mapping").isUndefined())texture.set("mapping",parseConstant(data.get("mapping"),imported("TEXTURE_MAPPING")));
            copy("channel");
            for(const auto* name:{"offset","repeat","center"})if(!data.get(name).isUndefined())invoke(texture.get(name),"fromArray")({data.get(name)});
            copy("rotation");
            if(!data.get("wrap").isUndefined()) {
                texture.set("wrapS",parseConstant(data.get("wrap").get(JSValue(0)),imported("TEXTURE_WRAPPING")));
                texture.set("wrapT",parseConstant(data.get("wrap").get(JSValue(1)),imported("TEXTURE_WRAPPING")));
            }
            for(const auto* name:{"format","internalFormat","type","colorSpace"})copy(name);
            for(const auto* name:{"minFilter","magFilter"})if(!data.get(name).isUndefined())texture.set(name,parseConstant(data.get(name),imported("TEXTURE_FILTER")));
            for(const auto* name:{"anisotropy","flipY","generateMipmaps","mipmapsAutoUpdate","premultiplyAlpha","unpackAlignment","compareFunction","normalized","userData"})copy(name);
            textures.set(data.get("uuid"),texture);
        }
    }
    return textures;
}

JSValue bindSkeletons(const JSValue&,const Args& args,const Args&) {
    const auto object=arg(args,0), skeletons=arg(args,1);
    if(invoke(nativeGlobalBinding("Object"),"keys")({skeletons}).get("length").strictEquals(0))return {};
    auto callback=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
        const auto child=arg(a,0);
        if(child.get("isSkinnedMesh").strictEquals(true) && !child.get("skeleton").isUndefined()) {
            const auto skeleton=c[0].get(child.get("skeleton"));
            if(skeleton.isUndefined())ThreeUtils::warn({"ObjectLoader: No skeleton found with UUID:",child.get("skeleton")});
            else invoke(child,"bind")({skeleton,child.get("bindMatrix")});
        }
        return {};
    },{skeletons},"",1);
    callback.setFunctionSourceText(ObjectLoaderSource::callback14Source);
    invoke(object,"traverse")({callback});return {};
}
JSValue bindLightTargets(const JSValue&,const Args& args,const Args&) {
    const auto object=arg(args,0);
    auto callback=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
        auto child=arg(a,0);
        if(child.get("isDirectionalLight").toBoolean() || child.get("isSpotLight").toBoolean()) {
            const auto uuid=child.get("target");const auto target=invoke(c[0],"getObjectByProperty")({"uuid",uuid});
            child.set("target",target.isUndefined()?imported("Object3D").construct():target);
        }
        return {};
    },{object},"",1);
    callback.setFunctionSourceText(ObjectLoaderSource::callback15Source);
    invoke(object,"traverse")({callback});return {};
}

JSValue parseObject(const JSValue& self,const Args& args,const Args&) {
    const auto data=arg(args,0), geometries=arg(args,1), materials=arg(args,2), textures=arg(args,3), animations=arg(args,4);
    const auto getGeometry=[&](const JSValue& name) {
        if(geometries.get(name).isUndefined())ThreeUtils::warn({"ObjectLoader: Undefined geometry",name});
        return geometries.get(name);
    };
    const auto getMaterial=[&](const JSValue& name)->JSValue {
        if(name.isUndefined())return {};
        if(isArray(name)) {
            auto result=JSValue::array();const auto length=name.get("length");
            for(double i=0;i<length.toNumber();++i) {
                const auto uuid=index(name,i);
                if(materials.get(uuid).isUndefined())ThreeUtils::warn({"ObjectLoader: Undefined material",uuid});
                invoke(result,"push")({materials.get(uuid)});
            }
            return result;
        }
        if(materials.get(name).isUndefined())ThreeUtils::warn({"ObjectLoader: Undefined material",name});
        return materials.get(name);
    };
    const auto getTexture=[&](const JSValue& uuid) {
        if(textures.get(uuid).isUndefined())ThreeUtils::warn({"ObjectLoader: Undefined texture",uuid});
        return textures.get(uuid);
    };
    JSValue object, geometry, material;
    const auto type=data.get("type");
    if(type.strictEquals("Scene")) {
        object=imported("Scene").construct();
        if(!data.get("background").isUndefined()) {
            if(invoke(nativeGlobalBinding("Number"),"isInteger")({data.get("background")}).toBoolean())
                object.set("background",imported("Color").construct({data.get("background")}));
            else object.set("background",getTexture(data.get("background")));
        }
        if(!data.get("environment").isUndefined())object.set("environment",getTexture(data.get("environment")));
        if(!data.get("fog").isUndefined()) {
            if(data.get("fog").get("type").strictEquals("Fog"))
                object.set("fog",imported("Fog").construct({data.get("fog").get("color"),data.get("fog").get("near"),data.get("fog").get("far")}));
            else if(data.get("fog").get("type").strictEquals("FogExp2"))
                object.set("fog",imported("FogExp2").construct({data.get("fog").get("color"),data.get("fog").get("density")}));
            if(!data.get("fog").get("name").strictEquals("")) {
                auto fog=object.get("fog");fog.set("name",data.get("fog").get("name"));
            }
        }
        for(const auto* name:{"backgroundBlurriness","backgroundIntensity"})
            if(!data.get(name).isUndefined())object.set(name,data.get(name));
        if(!data.get("backgroundRotation").isUndefined())invoke(object.get("backgroundRotation"),"fromArray")({data.get("backgroundRotation")});
        if(!data.get("environmentIntensity").isUndefined())object.set("environmentIntensity",data.get("environmentIntensity"));
        if(!data.get("environmentRotation").isUndefined())invoke(object.get("environmentRotation"),"fromArray")({data.get("environmentRotation")});
    } else if(type.strictEquals("PerspectiveCamera")) {
        object=imported("PerspectiveCamera").construct({data.get("fov"),data.get("aspect"),data.get("near"),data.get("far")});
        for(const auto* name:{"focus","zoom","filmGauge","filmOffset"})if(!data.get(name).isUndefined())object.set(name,data.get(name));
        if(!data.get("view").isUndefined())object.set("view",invoke(nativeGlobalBinding("Object"),"assign")({JSValue::object(),data.get("view")}));
    } else if(type.strictEquals("OrthographicCamera")) {
        object=imported("OrthographicCamera").construct({data.get("left"),data.get("right"),data.get("top"),data.get("bottom"),data.get("near"),data.get("far")});
        if(!data.get("zoom").isUndefined())object.set("zoom",data.get("zoom"));
        if(!data.get("view").isUndefined())object.set("view",invoke(nativeGlobalBinding("Object"),"assign")({JSValue::object(),data.get("view")}));
    } else if(type.strictEquals("AmbientLight") || type.strictEquals("DirectionalLight")) {
        object=imported(type.string().c_str()).construct({data.get("color"),data.get("intensity")});
        if(type.strictEquals("DirectionalLight")) {
            const auto target=data.get("target");object.set("target",target.toBoolean()?target:JSValue(""));
        }
    } else if(type.strictEquals("PointLight")) {
        object=imported("PointLight").construct({data.get("color"),data.get("intensity"),data.get("distance"),data.get("decay")});
    } else if(type.strictEquals("RectAreaLight")) {
        object=imported("RectAreaLight").construct({data.get("color"),data.get("intensity"),data.get("width"),data.get("height")});
    } else if(type.strictEquals("SpotLight")) {
        object=imported("SpotLight").construct({data.get("color"),data.get("intensity"),data.get("distance"),data.get("angle"),data.get("penumbra"),data.get("decay")});
        const auto target=data.get("target");object.set("target",target.toBoolean()?target:JSValue(""));
    } else if(type.strictEquals("HemisphereLight")) {
        object=imported("HemisphereLight").construct({data.get("color"),data.get("groundColor"),data.get("intensity")});
    } else if(type.strictEquals("LightProbe")) {
        const auto sh=invoke(imported("SphericalHarmonics3").construct(),"fromArray")({data.get("sh")});
        object=imported("LightProbe").construct({sh,data.get("intensity")});
    } else if(type.strictEquals("SkinnedMesh") || type.strictEquals("Mesh")) {
        geometry=getGeometry(data.get("geometry"));material=getMaterial(data.get("material"));
        object=imported(type.string().c_str()).construct({geometry,material});
        if(type.strictEquals("SkinnedMesh")) {
            if(!data.get("bindMode").isUndefined())object.set("bindMode",data.get("bindMode"));
            if(!data.get("bindMatrix").isUndefined())invoke(object.get("bindMatrix"),"fromArray")({data.get("bindMatrix")});
            if(!data.get("skeleton").isUndefined())object.set("skeleton",data.get("skeleton"));
        }
    } else if(type.strictEquals("InstancedMesh")) {
        geometry=getGeometry(data.get("geometry"));material=getMaterial(data.get("material"));
        const auto count=data.get("count"), instanceMatrix=data.get("instanceMatrix"), instanceColor=data.get("instanceColor");
        object=imported("InstancedMesh").construct({geometry,material,count});
        object.set("instanceMatrix",imported("InstancedBufferAttribute").construct({nativeGlobalBinding("Float32Array").construct({instanceMatrix.get("array")}),16}));
        if(!instanceColor.isUndefined())object.set("instanceColor",imported("InstancedBufferAttribute").construct({nativeGlobalBinding("Float32Array").construct({instanceColor.get("array")}),instanceColor.get("itemSize")}));
    } else if(type.strictEquals("BatchedMesh")) {
        geometry=getGeometry(data.get("geometry"));material=getMaterial(data.get("material"));
        object=imported("BatchedMesh").construct({data.get("maxInstanceCount"),data.get("maxVertexCount"),data.get("maxIndexCount"),material});
        object.set("geometry",geometry);
        for(const auto* name:{"perObjectFrustumCulled","sortObjects"})object.set(name,data.get(name));
        object.set("_drawRanges",data.get("drawRanges"));object.set("_reservedRanges",data.get("reservedRanges"));
        auto map=JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue {
            const auto info=arg(a,0);JSValue box=nullptr,sphere=nullptr;
            if(!info.get("boundingBox").isUndefined())box=invoke(imported("Box3").construct(),"fromJSON")({info.get("boundingBox")});
            if(!info.get("boundingSphere").isUndefined())sphere=invoke(imported("Sphere").construct(),"fromJSON")({info.get("boundingSphere")});
            auto result=JSValue::object();
            if(!info.isNull() && !info.isUndefined()) {
                const auto source=info.toObject();
                for(const auto& key:source.ownPropertyKeys()) {
                    const auto d=source.getOwnPropertyDescriptor(key);
                    if(d && d->enumerable.value_or(false)) {
                        JSPropertyDescriptor field;field.value=source.get(key);field.writable=field.enumerable=field.configurable=true;result.defineProperty(key,field);
                    }
                }
            }
            own(result,"boundingBox",box);own(result,"boundingSphere",sphere);return result;
        },{},"",1);
        map.setFunctionSourceText(ObjectLoaderSource::callback13Source);
        object.set("_geometryInfo",invoke(data.get("geometryInfo"),"map")({map}));
        for(const auto* name:{"instanceInfo","availableInstanceIds","availableGeometryIds","nextIndexStart","nextVertexStart","geometryCount","maxInstanceCount","maxVertexCount","maxIndexCount","geometryInitialized"})
            object.set(std::string("_")+name,data.get(name));
        object.set("_matricesTexture",getTexture(data.get("matricesTexture").get("uuid")));
        object.set("_indirectTexture",getTexture(data.get("indirectTexture").get("uuid")));
        if(!data.get("colorsTexture").isUndefined())object.set("_colorsTexture",getTexture(data.get("colorsTexture").get("uuid")));
        if(!data.get("boundingSphere").isUndefined())object.set("boundingSphere",invoke(imported("Sphere").construct(),"fromJSON")({data.get("boundingSphere")}));
        if(!data.get("boundingBox").isUndefined())object.set("boundingBox",invoke(imported("Box3").construct(),"fromJSON")({data.get("boundingBox")}));
    } else if(type.strictEquals("Line") || type.strictEquals("LineLoop") || type.strictEquals("LineSegments") || type.strictEquals("Points") || type.strictEquals("PointCloud")) {
        const auto constructor=imported(type.strictEquals("PointCloud")?"Points":type.string().c_str());
        // List initialization fixes the left-to-right evaluation of both resource resolutions.
        object=constructor.construct({getGeometry(data.get("geometry")),getMaterial(data.get("material"))});
    } else if(type.strictEquals("Sprite"))object=imported("Sprite").construct({getMaterial(data.get("material"))});
    else if(type.strictEquals("LOD") || type.strictEquals("Group") || type.strictEquals("Bone"))object=imported(type.string().c_str()).construct();
    else object=imported("Object3D").construct();

    object.set("uuid",data.get("uuid"));
    if(!data.get("name").isUndefined())object.set("name",data.get("name"));
    if(!data.get("pivot").isUndefined())object.set("pivot",invoke(imported("Vector3").construct(),"fromArray")({data.get("pivot")}));
    if(!data.get("matrix").isUndefined()) {
        invoke(object.get("matrix"),"fromArray")({data.get("matrix")});
        if(!data.get("matrixAutoUpdate").isUndefined())object.set("matrixAutoUpdate",data.get("matrixAutoUpdate"));
        if(object.get("matrixAutoUpdate").toBoolean()) {
            invoke(object.get("matrix"),"decompose")({object.get("position"),object.get("quaternion"),object.get("scale")});
            if(!object.get("pivot").isNull()) {
                const auto copied=invoke(object.get("position"),"copy")({object.get("pivot")});
                const auto applied=invoke(copied,"applyMatrix4")({object.get("matrix")});invoke(applied,"sub")({object.get("pivot")});
            }
        }
    } else {
        for(const auto* name:{"position","rotation","quaternion","scale"})if(!data.get(name).isUndefined())invoke(object.get(name),"fromArray")({data.get(name)});
    }
    if(!data.get("up").isUndefined())invoke(object.get("up"),"fromArray")({data.get("up")});
    if(!data.get("morphTargetDictionary").isUndefined())object.set("morphTargetDictionary",invoke(nativeGlobalBinding("Object"),"assign")({JSValue::object(),data.get("morphTargetDictionary")}));
    if(!data.get("morphTargetInfluences").isUndefined())object.set("morphTargetInfluences",invoke(data.get("morphTargetInfluences"),"slice")({}));
    for(const auto* name:{"castShadow","receiveShadow"})if(!data.get(name).isUndefined())object.set(name,data.get(name));
    if(data.get("shadow").toBoolean()) {
        for(const auto* name:{"intensity","bias","normalBias","radius","blurSamples","focus","aspect"})
            if(!data.get("shadow").get(name).isUndefined()) {auto shadow=object.get("shadow");shadow.set(name,data.get("shadow").get(name));}
        if(!data.get("shadow").get("mapSize").isUndefined())invoke(object.get("shadow").get("mapSize"),"fromArray")({data.get("shadow").get("mapSize")});
        if(!data.get("shadow").get("camera").isUndefined()) {
            auto shadow=object.get("shadow");shadow.set("camera",invoke(self,"parseObject")({data.get("shadow").get("camera")}));
        }
    }
    for(const auto* name:{"visible","frustumCulled","renderOrder","static","userData"})if(!data.get(name).isUndefined())object.set(name,data.get(name));
    if(!data.get("layers").isUndefined()) {auto layers=object.get("layers");layers.set("mask",data.get("layers"));}
    if(!data.get("children").isUndefined()) {
        const auto children=data.get("children");
        for(double i=0;i<children.get("length").toNumber();++i)
            invoke(object,"add")({invoke(self,"parseObject")({index(children,i),geometries,materials,textures,animations})});
    }
    if(!data.get("animations").isUndefined()) {
        const auto objectAnimations=data.get("animations");
        for(double i=0;i<objectAnimations.get("length").toNumber();++i) {
            const auto uuid=index(objectAnimations,i);invoke(object.get("animations"),"push")({animations.get(uuid)});
        }
    }
    if(data.get("type").strictEquals("LOD")) {
        if(!data.get("autoUpdate").isUndefined())object.set("autoUpdate",data.get("autoUpdate"));
        const auto levels=data.get("levels");
        for(double i=0;i<levels.get("length").toNumber();++i) {
            const auto level=index(levels,i);const auto child=invoke(object,"getObjectByProperty")({"uuid",level.get("object")});
            if(!child.isUndefined())invoke(object,"addLevel")({child,level.get("distance"),level.get("hysteresis")});
        }
    }
    return object;
}

JSValue defaulted(const Args& args,std::size_t i,const JSValue& value) {
    const auto v=arg(args,i);return v.isUndefined()?value:v;
}
// Source constructors required by ObjectLoader, sharing the managed Object3D graph.
// Independent class methods outside deserialization remain separate ports.
JSValue objectConstructor(const char* name,unsigned arity,const JSValue& base) {
    auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& a,const Args& c)->JSValue {
        const auto name=c[0].string();const auto parent=c[1].get("constructor").getPrototype();Args baseArgs;
        JSValue geometry,material;
        if(name=="Line" || name=="Points") {
            geometry=arg(a,0).isUndefined()?nativeBufferGeometryConstructor().construct():arg(a,0);
            material=arg(a,1).isUndefined()?MaterialLoader::createMaterialFromType(name=="Points"?"PointsMaterial":"LineBasicMaterial"):arg(a,1);
        } else if(name=="LineLoop" || name=="LineSegments")baseArgs={arg(a,0),arg(a,1)};
        else if(name=="LightProbe")baseArgs={{},defaulted(a,1,1)};
        else if(name!="Group" && name!="LOD" && name!="Sprite")baseArgs={arg(a,0),arg(a,1)};
        if(name=="Sprite")material=arg(a,0).isUndefined()?MaterialLoader::createMaterialFromType("SpriteMaterial"):arg(a,0);
        const auto harmonics=name=="LightProbe"?(arg(a,0).isUndefined()?imported("SphericalHarmonics3").construct():arg(a,0)):JSValue{};
        auto self=parent.construct(baseArgs,target);
        self.set("is"+name,true);
        if(name=="LOD")self.set("_currentLevel",0);
        self.set("type",name);
        if(name=="LOD") {
            JSPropertyDescriptor levels;levels.value=JSValue::array();levels.enumerable=true;self.defineProperty("levels",levels);self.set("autoUpdate",true);
        } else if(name=="Line" || name=="Points") {
            self.set("geometry",geometry);self.set("material",material);self.set("morphTargetDictionary",{});self.set("morphTargetInfluences",{});
            invoke(self,"updateMorphTargets")({});
        } else if(name=="Sprite") {
            auto module=environment();auto geometry=module.get("_spriteGeometry");
            if(geometry.isUndefined()) {
                geometry=nativeBufferGeometryConstructor().construct();
                auto values=JSValue::array();for(const auto number:{-0.5,-0.5,0.,0.,0.,0.5,-0.5,0.,1.,0.,0.5,0.5,0.,1.,1.,-0.5,0.5,0.,0.,1.})invoke(values,"push")({number});
                const auto array=nativeGlobalBinding("Float32Array").construct({values});
                const auto buffer=InterleavedBuffer(array,5).object();
                auto indices=JSValue::array();for(const auto i:{0,1,2,0,2,3})invoke(indices,"push")({i});
                invoke(geometry,"setIndex")({indices});
                invoke(geometry,"setAttribute")({"position",InterleavedBufferAttribute::constructor().construct({buffer,3,0,false})});
                invoke(geometry,"setAttribute")({"uv",InterleavedBufferAttribute::constructor().construct({buffer,2,3,false})});
                module.set("_spriteGeometry",geometry);
            }
            self.set("geometry",geometry);self.set("material",material);self.set("center",nativeVector2Constructor().construct({0.5,0.5}));self.set("count",1);
        } else if(name=="RectAreaLight") {self.set("width",defaulted(a,2,10));self.set("height",defaulted(a,3,10));}
        else if(name=="LightProbe")self.set("sh",harmonics);
        return self;
    },{name,holder},name,arity);
    holder.set("constructor",ctor);ctor.setPrototype(base);ctor.get("prototype").setPrototype(base.get("prototype"));
    if(std::string(name)=="Line" || std::string(name)=="Points") {
        JSPropertyDescriptor d;d.writable=d.configurable=true;
        // The source bodies are identical; their native Mesh counterpart performs the same graph operations.
        d.value=nativeMeshConstructor().get("prototype").get("updateMorphTargets");ctor.get("prototype").defineProperty("updateMorphTargets",d);
    }
    if(std::string(name)=="LOD") {
        JSPropertyDescriptor d;d.writable=d.configurable=true;
        d.value=JSValue::function(+[](const JSValue& self,const Args& a,const Args&)->JSValue {
            const auto object=arg(a,0);const auto distance=invoke(nativeGlobalBinding("Math"),"abs")({defaulted(a,1,0)});const auto hysteresis=defaulted(a,2,0);
            const auto levels=self.get("levels");double i=0;
            for(;i<levels.get("length").toNumber();++i)if(distance.toNumber()<index(levels,i).get("distance").toNumber())break;
            auto level=JSValue::object();own(level,"distance",distance);own(level,"hysteresis",hysteresis);own(level,"object",object);
            invoke(levels,"splice")({i,0,level});invoke(self,"add")({object});return self;
        },{},"addLevel",1);ctor.get("prototype").defineProperty("addLevel",d);
    }
    return ctor;
}
JSValue fogConstructor(const char* name) {
    return JSValue::constructor(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
        auto result=self;result.set(c[0].strictEquals("Fog")?"isFog":"isFogExp2",true);result.set("name","");
        result.set("color",nativeColorConstructor().construct({arg(a,0)}));
        if(c[0].strictEquals("Fog")) {
            result.set("near",arg(a,1).isUndefined()?JSValue(1):arg(a,1));result.set("far",arg(a,2).isUndefined()?JSValue(1000):arg(a,2));
        } else result.set("density",arg(a,1).isUndefined()?JSValue(0.00025):arg(a,1));
        return result;
    }, {name},name,1);
}
JSValue harmonicsConstructor() {
    auto constructor=JSValue::constructor(+[](const JSValue& self,const Args&,const Args&)->JSValue {
        const auto harmonics=std::make_shared<SphericalHarmonics3>();
        auto result=JSValue::hostObject(harmonics,harmonics.get(),"three.ObjectLoaderSphericalHarmonics3");
        result.setPrototype(self.getPrototype());result.set("isSphericalHarmonics3",true);
        auto coefficients=JSValue::array();
        for(auto& coefficient:harmonics->coefficients) {
            const auto vector=std::shared_ptr<Vector3>(harmonics,&coefficient);invoke(coefficients,"push")({nativeVector(vector)});
        }
        result.set("coefficients",coefficients);return result;
    },{},"SphericalHarmonics3",0);
    JSPropertyDescriptor descriptor;descriptor.writable=descriptor.configurable=true;
    descriptor.value=JSValue::function(+[](const JSValue& self,const Args& a,const Args&)->JSValue {
        const auto array=arg(a,0);auto offset=arg(a,1).isUndefined()?JSValue(0):arg(a,1);
        const auto coefficients=self.get("coefficients");
        for(double i=0;i<9;++i) {
            invoke(index(coefficients,i),"fromArray")({array,offset});offset=plus(offset,3);
        }
        return self;
    },{},"fromArray",1);
    constructor.get("prototype").defineProperty("fromArray",descriptor);return constructor;
}

} // namespace

JSValue nativeObjectLoaderModule() {
    static thread_local JSWeakValue weak;
    if(const auto value=weak.lock();value && value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ObjectLoader module");
    auto bindings=JSValue::object(nullptr), holder=JSValue::object(nullptr);
    own(bindings,"FileLoader",nativeFileLoaderConstructor());
    own(bindings,"ImageLoader",nativeImageLoaderConstructor());
    own(bindings,"LoadingManager",nativeLoadingManagerConstructor());
    own(bindings,"BufferGeometryLoader",nativeBufferGeometryLoaderConstructor());
    own(bindings,"MaterialLoader",nativeMaterialLoaderConstructor());
    own(bindings,"Object3D",nativeObject3DConstructor());own(bindings,"Scene",nativeSceneConstructor());
    own(bindings,"Light",nativeLightConstructor());
    own(bindings,"AmbientLight",nativeAmbientLightConstructor());
    own(bindings,"DirectionalLight",nativeDirectionalLightConstructor());
    own(bindings,"HemisphereLight",nativeHemisphereLightConstructor());
    own(bindings,"PointLight",nativePointLightConstructor());
    own(bindings,"SpotLight",nativeSpotLightConstructor());
    own(bindings,"PerspectiveCamera",nativePerspectiveCameraConstructor());own(bindings,"OrthographicCamera",nativeOrthographicCameraConstructor());
    own(bindings,"Mesh",nativeMeshConstructor());own(bindings,"SkinnedMesh",nativeSkinnedMeshConstructor());
    own(bindings,"InstancedMesh",nativeInstancedMeshConstructor());own(bindings,"BatchedMesh",nativeBatchedMeshConstructor());
    own(bindings,"Bone",nativeBoneConstructor());own(bindings,"Skeleton",nativeSkeletonConstructor());
    own(bindings,"Color",nativeColorConstructor());own(bindings,"Vector3",nativeVector3Constructor());
    own(bindings,"Box3",nativeBox3Constructor());own(bindings,"Sphere",nativeSphereConstructor());
    own(bindings,"InstancedBufferAttribute",nativeInstancedBufferAttributeConstructor());
    own(bindings,"Texture",nativeTextureConstructor());own(bindings,"CubeTexture",nativeCubeTextureConstructor());
    own(bindings,"DataTexture",nativeDataTextureConstructor());own(bindings,"TextureSource",nativeTextureSourceConstructor());
    own(bindings,"Geometries",nativeObjectLoaderGeometries());own(bindings,"_customGeometries",JSValue::object());
    own(bindings,"Shape",nativeObjectLoaderShapeConstructor());
    auto animation=JSValue::object();
    own(animation,"parse",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue {return nativeAnimationClip(AnimationClip::parse(arg(a,0)));},{},"parse",1));
    own(bindings,"AnimationClip",animation);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"Group",0},{"LOD",0},{"Line",0},{"LineLoop",2},{"LineSegments",2},{"Points",0},{"Sprite",0},{"RectAreaLight",2},{"LightProbe",0}}) {
        const std::string type=name;
        const auto base=type=="LineLoop"||type=="LineSegments"?bindings.get("Line"):
            type.ends_with("Light")||type=="LightProbe"?bindings.get("Light"):bindings.get("Object3D");
        own(bindings,name,objectConstructor(name,arity,base));
    }
    own(bindings,"Fog",fogConstructor("Fog"));own(bindings,"FogExp2",fogConstructor("FogExp2"));own(bindings,"SphericalHarmonics3",harmonicsConstructor());
    auto typedArrays=JSValue::object();
    for(const auto* name:{"Int8Array","Uint8Array","Uint8ClampedArray","Int16Array","Uint16Array","Int32Array","Uint32Array","Float32Array","Float64Array"})own(typedArrays,name,nativeGlobalBinding(name));
    own(bindings,"TypedArrays",typedArrays);
    for(const auto& [name,values]:std::initializer_list<std::pair<const char*,std::vector<std::pair<const char*,double>>>>{
        {"TEXTURE_MAPPING",{{"UVMapping",UVMapping},{"CubeReflectionMapping",CubeReflectionMapping},{"CubeRefractionMapping",CubeRefractionMapping},{"EquirectangularReflectionMapping",EquirectangularReflectionMapping},{"EquirectangularRefractionMapping",EquirectangularRefractionMapping}}},
        {"TEXTURE_WRAPPING",{{"RepeatWrapping",RepeatWrapping},{"ClampToEdgeWrapping",ClampToEdgeWrapping},{"MirroredRepeatWrapping",MirroredRepeatWrapping}}},
        {"TEXTURE_FILTER",{{"NearestFilter",NearestFilter},{"NearestMipmapNearestFilter",NearestMipmapNearestFilter},{"NearestMipmapLinearFilter",NearestMipmapLinearFilter},{"LinearFilter",LinearFilter},{"LinearMipmapNearestFilter",LinearMipmapNearestFilter},{"LinearMipmapLinearFilter",LinearMipmapLinearFilter}}}}) {
        auto dictionary=JSValue::object();for(const auto& [key,value]:values)own(dictionary,key,value);own(bindings,name,dictionary);
    }
    auto constructor=JSValue::builtinConstructor(+[](const JSValue& newTarget,const Args& a,const Args& c)->JSValue {
        return c[0].get("constructor").getPrototype().construct({arg(a,0)},newTarget);
    },{holder},"ObjectLoader",1);
    holder.set("constructor",constructor);constructor.setPrototype(nativeLoaderConstructor());
    auto prototype=constructor.get("prototype");prototype.setPrototype(Loader::prototype());constructor.setFunctionSourceText(ObjectLoaderSource::classSource);
    const auto install=[&](const char* name,unsigned arity,JSValue::NativeFunction body,const char* source) {
        auto function=JSValue::function(body,{},name,arity);function.setFunctionSourceText(source);
        JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty(name,d);
    };
    install("load",4,load,ObjectLoaderSource::loadSource);
    const auto async=[&](const char* name,unsigned arity,JSAsyncStateBody body,const char* source) {
        JSPropertyDescriptor d;d.value=createNativeAsyncStateFunction(body,{},name,arity,source);d.writable=d.configurable=true;prototype.defineProperty(name,d);
    };
    async("loadAsync",2,loadAsyncStep,ObjectLoaderSource::loadAsyncSource);
    install("parse",2,parse,ObjectLoaderSource::parseSource);
    async("parseAsync",1,parseAsyncStep,ObjectLoaderSource::parseAsyncSource);
    auto registerFunction=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {auto custom=c[0];custom.set(arg(a,0),arg(a,1));return {};},{bindings.get("_customGeometries")},"registerGeometry",2);
    registerFunction.setFunctionSourceText(ObjectLoaderSource::registerGeometrySource);
    JSPropertyDescriptor reg;reg.value=registerFunction;reg.writable=reg.configurable=true;constructor.defineProperty("registerGeometry",reg);
    install("parseShapes",1,parseShapes,ObjectLoaderSource::parseShapesSource);
    install("parseSkeletons",2,parseSkeletons,ObjectLoaderSource::parseSkeletonsSource);
    install("parseGeometries",2,parseGeometries,ObjectLoaderSource::parseGeometriesSource);
    install("parseMaterials",2,parseMaterials,ObjectLoaderSource::parseMaterialsSource);
    install("parseAnimations",1,parseAnimations,ObjectLoaderSource::parseAnimationsSource);
    install("parseImages",2,parseImages,ObjectLoaderSource::parseImagesSource);
    async("parseImagesAsync",1,parseImagesAsyncStep,ObjectLoaderSource::parseImagesAsyncSource);
    install("parseTextures",2,parseTextures,ObjectLoaderSource::parseTexturesSource);
    install("parseObject",5,parseObject,ObjectLoaderSource::parseObjectSource);
    install("bindSkeletons",2,bindSkeletons,ObjectLoaderSource::bindSkeletonsSource);
    install("bindLightTargets",1,bindLightTargets,ObjectLoaderSource::bindLightTargetsSource);
    own(bindings,"ObjectLoader",constructor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"ObjectLoader",constructor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeObjectLoaderBinding(const std::string& name) {const auto keeper=nativeObjectLoaderModule();return lexical().lock()->get(name);}
JSValue nativeObjectLoaderConstructor() {return nativeObjectLoaderBinding("ObjectLoader");}

ObjectLoader::ObjectLoader(JSValue manager):object_(nativeObjectLoaderConstructor().construct({manager})) {}
JSValue ObjectLoader::prototype() {return nativeObjectLoaderConstructor().get("prototype");}
JSValue ObjectLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError) {return invoke(object_,"load")({url,onLoad,onProgress,onError});}
JSValue ObjectLoader::loadAsync(JSValue url,JSValue onProgress) {return invoke(object_,"loadAsync")({url,onProgress});}
JSValue ObjectLoader::parseManaged(JSValue json,JSValue onLoad) {return invoke(object_,"parse")({json,onLoad});}
JSValue ObjectLoader::parseAsync(JSValue json) {return invoke(object_,"parseAsync")({json});}
ObjectLoader& ObjectLoader::setCrossOrigin(JSValue value) {invoke(object_,"setCrossOrigin")({value});return *this;}
ObjectLoader& ObjectLoader::setWithCredentials(JSValue value) {invoke(object_,"setWithCredentials")({value});return *this;}
ObjectLoader& ObjectLoader::setPath(JSValue value) {invoke(object_,"setPath")({value});return *this;}
ObjectLoader& ObjectLoader::setResourcePath(JSValue value) {invoke(object_,"setResourcePath")({value});return *this;}
ObjectLoader& ObjectLoader::setRequestHeader(JSValue value) {invoke(object_,"setRequestHeader")({value});return *this;}
void ObjectLoader::registerGeometry(JSValue type,JSValue geometryClass) {invoke(nativeObjectLoaderConstructor(),"registerGeometry")({type,geometryClass});}
