#include "catch.hpp"
#include "animation/AnimationRuntime.h"
#include "animation/AnimationClip.h"
#include "animation/AnimationMixer.h"
#include "animation/PropertyBinding.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "core/NativeObject.h"
#include "core/Object3D.h"
#include "math/interpolants/LinearInterpolant.h"
#include "math/interpolants/CubicInterpolant.h"
#include "objects/Mesh.h"
#include "textures/Texture.h"

namespace {
JSValue call(const JSValue& self,const char* method,const std::vector<JSValue>& args={}) { return self.get(method).call(self,args); }
JSValue rootObject() {auto root=JSValue::object();root.set("x",2);return root;}
}
TEST_CASE("Animation runtime binding methods share prototypes and mutate native metadata", "[animationruntime]") {
    auto root=rootObject();auto binding=std::make_shared<PropertyBinding>(root,".x");auto value=nativePropertyBinding(binding);
    const auto second=nativePropertyBinding(std::make_shared<PropertyBinding>(root,".x"));
    REQUIRE(value.sameValue(nativePropertyBinding(binding)));
    REQUIRE(value.getPrototype().sameValue(second.getPrototype()));
    REQUIRE(value.get("getValue").sameValue(value.get("_getValue_unbound")));
    REQUIRE(value.get("getValue").get("length").number()==2);
    REQUIRE(value.get("GetterByBindingType").get("0").sameValue(value.get("_getValue_direct")));
    REQUIRE(value.get("SetterByBindingTypeAndVersioning").get("0").get("0").sameValue(value.get("_setValue_direct")));
    REQUIRE(value.get("BindingType").get("Direct").number()==0);
    REQUIRE_FALSE(value.hasOwn("targetObject"));
    auto buffer=JSValue::array(1);call(value,"getValue",{buffer,0});
    REQUIRE(buffer.get("0").number()==2);
    REQUIRE(value.get("targetObject").sameValue(root));
    REQUIRE(value.get("getValue").sameValue(value.get("_getValue_direct")));
    buffer.set("0",7);call(value,"setValue",{buffer,0});REQUIRE(root.get("x").number()==7);
    binding->path=".changed";REQUIRE(value.get("path").string()==".changed");
    value.set("propertyName","y");buffer.set("0",9);binding->setValue(AnimationArray(buffer),0);REQUIRE(root.get("y").number()==9);
    call(value,"unbind");REQUIRE(value.get("node").isNull());
}
TEST_CASE("Animation runtime composite exposes only the upstream own metadata", "[animationruntime]") {
    auto group=JSValue::object();group.set("nCachedObjects_",0);
    group.set("subscribe_",JSValue::function([](const JSValue&,const auto&,const auto&){return JSValue::array();}));
    auto value=nativePropertyBinding(std::make_shared<PropertyBindingComposite>(group,".x"));
    REQUIRE(value.ownPropertyNames()==std::vector<std::string>{"_targetGroup","_bindings"});
    REQUIRE(value.get("rootNode").isUndefined());REQUIRE(value.get("path").isUndefined());
    call(value,"bind");call(value,"unbind");
}
TEST_CASE("Animation runtime mixer shares buffers and dispatches managed callbacks", "[animationruntime]") {
    auto root=rootObject();auto binding=std::make_shared<PropertyBinding>(root,".x");
    auto mixer=std::make_shared<PropertyMixer>(binding,"number",1);auto value=nativePropertyMixer(mixer);
    REQUIRE(value.get("_cacheIndex").isUndefined());REQUIRE_FALSE(value.hasOwn("_cacheIndex"));
    value.set("_cacheIndex",3);REQUIRE(mixer->_cacheIndex.number()==3);
    REQUIRE(value.get("_mixBufferRegion").sameValue(value.get("_lerp")));
    auto buffer=value.get("buffer");REQUIRE(buffer.sameValue(value.get("buffer")));
    call(value,"saveOriginalState");buffer.set("0",10);call(value,"accumulate",{0,1});call(value,"apply",{0});
    REQUIRE(root.get("x").number()==10);call(value,"restoreOriginalState");REQUIRE(root.get("x").number()==2);
    auto trace=JSValue::array();auto fn=JSValue::function([](const JSValue& self,const auto& args,const auto& captures){
        auto trace=captures[0];trace.set("0",self.get("valueSize"));trace.set("1",args[3]);
        auto b=args[0];b.set(args[1].toString(),42);return JSValue();
    },{trace});
    value.set("_mixBufferRegion",fn);REQUIRE(value.get("_mixBufferRegion").sameValue(fn));
    mixer->cumulativeWeight=.5;mixer->accumulate(0,.5);
    REQUIRE(trace.get("0").number()==1);REQUIRE(trace.get("1").number()==.5);REQUIRE(buffer.get("1").number()==42);
    auto replacement=JSValue::array(5);value.set("buffer",replacement);REQUIRE(nativeArray(mixer->buffer).sameValue(replacement));
}
TEST_CASE("Animation runtime interpolants share state and invoke native evaluate overrides", "[animationruntime]") {
    class Derived final:public LinearInterpolant {public:using LinearInterpolant::LinearInterpolant;int calls=0;
        int interpolations=0,intervals=0;
        Array evaluate(double time)override{++calls;return LinearInterpolant::evaluate(time);}
        Array interpolate_(std::ptrdiff_t i,JSValue t0,double t,JSValue t1)override{++interpolations;return LinearInterpolant::interpolate_(i,t0,t,t1);}
        void intervalChanged_(std::ptrdiff_t,JSValue,JSValue)override{++intervals;}};
    auto v=std::make_shared<Derived>(Float32Array({0,1}),Float32Array({2,6}),1);auto value=nativeInterpolant(v);
    const auto result=value.get("resultBuffer");const auto evaluated=call(value,"evaluate",{.5});
    REQUIRE(evaluated.sameValue(result));REQUIRE(evaluated.get("0").number()==4);REQUIRE(v->calls==1);
    REQUIRE(v->interpolations==1);REQUIRE(v->intervals==1);
    auto settings=JSValue::object();settings.set("endingStart",2402);value.set("settings",settings);
    REQUIRE(v->settings.sameValue(settings));REQUIRE(call(value,"getSettings_").sameValue(settings));
    value.set("__cacheIndex",4);REQUIRE(v->__cacheIndex.number()==4);
    auto buffer=JSValue::array(1);value.set("resultBuffer",buffer);REQUIRE(call(value,"evaluate",{1}).sameValue(buffer));
    REQUIRE(buffer.get("0").number()==6);
}
TEST_CASE("Animation runtime tracks and clips retain live container and factory identity", "[animationruntime]") {
    auto t=std::make_shared<NumberKeyframeTrack>(".x",Float32Array({0,1}),Float32Array({0,8}));
    auto track=nativeKeyframeTrack(t);REQUIRE(track.get("ValueTypeName").string()=="number");
    REQUIRE(track.get("createInterpolant").sameValue(track.get("InterpolantFactoryMethodLinear")));
    auto curve=call(track,"createInterpolant");REQUIRE(call(curve,"evaluate",{.25}).get("0").number()==2);
    REQUIRE(call(track,"shift",{2}).sameValue(track));REQUIRE(t->times.number(0)==2);
    auto list=std::make_shared<AnimationClip::Tracks>(AnimationClip::Tracks{t});
    auto c=std::make_shared<AnimationClip>("clip",-1,list);auto clip=nativeAnimationClip(c);
    auto tracks=clip.get("tracks");REQUIRE(tracks.sameValue(clip.get("tracks")));REQUIRE(tracks.get("0").sameValue(track));
    auto t2=std::make_shared<NumberKeyframeTrack>(".x",Float32Array({0,4}),Float32Array({1,3}));
    tracks.set("0",nativeKeyframeTrack(t2));REQUIRE(list->at(0)==t2);call(clip,"resetDuration");REQUIRE(c->duration==4);
    c->name="native";REQUIRE(clip.get("name").string()=="native");
    const auto clone=call(clip,"clone");REQUIRE_FALSE(clone.sameValue(clip));REQUIRE_FALSE(clone.get("tracks").sameValue(tracks));
}
TEST_CASE("Animation runtime GC distinguishes internal native roots from external owners", "[animationruntime][gc]") {
    auto anchor=JSValue::object();std::weak_ptr<AnimationClip> weak;
    {
        auto clip=std::make_shared<AnimationClip>("cycle");weak=clip;auto wrapper=nativeAnimationClip(clip);
        clip->userData.set("self",wrapper);wrapper.set("keep",17);
        wrapper={};anchor.collectGarbage();
        REQUIRE(clip->userData.get("self").get("keep").number()==17);
    }
    anchor.collectGarbage();REQUIRE(weak.expired());
    std::weak_ptr<PropertyMixer> mixerWeak;
    {
        auto root=rootObject();auto binding=std::make_shared<PropertyBinding>(root,".x");
        auto mixer=std::make_shared<PropertyMixer>(binding,"string",1);mixerWeak=mixer;
        auto value=nativePropertyMixer(mixer);auto buffer=value.get("buffer");buffer.set("0",value);
        value.set("_setIdentity",JSValue::function([](const JSValue&,const auto&,const auto&){return JSValue();},{value}));
    }
    anchor.collectGarbage();REQUIRE(mixerWeak.expired());
}
TEST_CASE("Animation runtime reachable native views preserve extensions without retaining replaced buffers", "[animationruntime][gc]") {
    auto v=std::make_shared<LinearInterpolant>(Float32Array({0,1}),Float32Array({2,6}),1);
    auto value=nativeInterpolant(v);auto buffer=value.get("resultBuffer");buffer.set("extension",7);buffer={};
    value.collectGarbage();REQUIRE(value.get("resultBuffer").get("extension").number()==7);
    std::weak_ptr<AnimationClip> weak;
    {auto c=std::make_shared<AnimationClip>();weak=c;auto clip=nativeAnimationClip(c);value.get("resultBuffer").set("clip",clip);}
    v->resultBuffer=Float32Array(1);value.collectGarbage();REQUIRE(weak.expired());
}
TEST_CASE("Animation runtime exposes live Object3D animation arrays", "[animationruntime]") {
    auto object=std::make_shared<Object3D>();auto clip=std::make_shared<AnimationClip>("a");object->animations.push_back(clip);
    auto root=nativeObject(object);auto clips=root.get("animations");REQUIRE(clips.sameValue(root.get("animations")));
    REQUIRE(clips.get("0").sameValue(nativeAnimationClip(clip)));clips.set("length",0);REQUIRE(object->animations.empty());
    clips.set("0",nativeAnimationClip(clip));REQUIRE(object->animations.front()==clip);
}
TEST_CASE("Animation runtime collects native Mesh resource wrapper cycles", "[animationruntime][gc]") {
    auto anchor=JSValue::object();
    std::weak_ptr<Mesh> meshWeak;
    std::weak_ptr<Material> materialWeak;
    std::weak_ptr<Texture> textureWeak;
    std::weak_ptr<BufferAttribute> attributeWeak;
    {
        auto material=std::make_shared<MeshBasicMaterial>();materialWeak=material;
        auto texture=std::make_shared<Texture>();textureWeak=texture;
        material->map=texture;material->alphaMap=texture;
        auto mesh=std::make_shared<Mesh>(std::make_shared<BufferGeometry>(),material);meshWeak=mesh;
        auto attribute=std::make_shared<Float32BufferAttribute>(std::vector<float>{0,1,2},3);attributeWeak=attribute;
        mesh->geometry->setAttribute("position",attribute);mesh->geometry->index=attribute;
        mesh->geometry->morphAttributes["position"]={attribute,attribute};
        auto root=nativeObject(mesh);AnimationMixer mixer(root);
        SECTION("single material extension") { nativeMaterial(material).set("mixer",mixer.object()); }
        SECTION("shared materials array extension") {
            mesh->materials.push_back(material);mesh->materials.push_back(material);mesh->materialIsArray=true;
            root.get("material").set("mixer",mixer.object());
            nativeMaterial(material).set("mixer",mixer.object());
        }
        SECTION("texture wrapper and native userData") {
            nativeTexture(texture).set("mixer",mixer.object());texture->userData.set("root",root);
        }
        SECTION("image ordinary data held through source and mipmaps") {
            auto image=std::make_shared<TextureImage>();image->data=JSValue::object();
            std::get<JSValue>(image->data).set("mixer",mixer.object());
            texture->image=image;texture->mipmaps->push_back(image);
        }
        SECTION("geometry attribute wrapper shared in several native slots") {
            root.get("geometry").get("morphAttributes").get("position").get("0").set("mixer",mixer.object());
        }
    }
    anchor.collectGarbage();
    REQUIRE(meshWeak.expired());REQUIRE(materialWeak.expired());
    REQUIRE(textureWeak.expired());REQUIRE(attributeWeak.expired());
}
TEST_CASE("Animation runtime retains resource wrappers for external native owners", "[animationruntime][gc]") {
    auto anchor=JSValue::object();auto material=std::make_shared<MeshBasicMaterial>();
    auto texture=std::make_shared<Texture>();material->map=texture;
    std::weak_ptr<Mesh> meshWeak;
    {
        auto mesh=std::make_shared<Mesh>(std::make_shared<BufferGeometry>(),material);meshWeak=mesh;
        auto root=nativeObject(mesh);AnimationMixer mixer(root);
        nativeMaterial(material).set("mixer",mixer.object());nativeTexture(texture).set("marker",7);
    }
    anchor.collectGarbage();
    REQUIRE_FALSE(meshWeak.expired());
    REQUIRE(nativeMaterial(material).get("mixer").get("_root").get("isMesh").toBoolean());
    REQUIRE(nativeTexture(texture).get("marker").number()==7);
    material.reset();texture.reset();anchor.collectGarbage();REQUIRE(meshWeak.expired());
}
TEST_CASE("Animation native vector arrays reject deletion that cannot preserve sparse storage", "[animationruntime]") {
    auto track=std::make_shared<NumberKeyframeTrack>(".x",Float32Array({0,1}),Float32Array({2,3}));
    auto clip=std::make_shared<AnimationClip>("a",1,std::make_shared<AnimationClip::Tracks>(AnimationClip::Tracks{track}));
    auto clips=nativeAnimationClips(std::make_shared<AnimationClip::Clips>(AnimationClip::Clips{clip}));
    auto tracks=nativeAnimationClip(clip).get("tracks");
    for(auto array:{clips,tracks}) {
        const auto original=array.get("0");REQUIRE_THROWS_AS(array.erase("0"),JSTypeError);
        REQUIRE(array.get("0").sameValue(original));REQUIRE_FALSE(array.erase("length"));
        REQUIRE(array.erase("7"));
    }
}
TEST_CASE("Animation runtime interpolant internal calls read current methods and preserve virtual fallback", "[animationruntime][gc]") {
    auto curve=std::make_shared<LinearInterpolant>(Float32Array({0,1,2}),Float32Array({2,6,10}),1);
    auto value=nativeInterpolant(curve);auto trace=JSValue::array();
    auto interpolate=JSValue::function([](const JSValue& self,const auto& args,const auto& captures){
        auto trace=captures[0];trace.set(std::to_string(trace.length()),"interpolate:"+args[0].toString());
        self.collectGarbage();auto result=self.get("resultBuffer");result.set("0",42);return result;
    },{trace});
    JSPropertyDescriptor descriptor;descriptor.configurable=true;
    descriptor.getter=JSValue::function([](const JSValue& self,const auto&,const auto& captures){
        auto trace=captures[0];trace.set(std::to_string(trace.length()),"getInterval");self.collectGarbage();return captures[1];
    },{trace,JSValue::function([](const JSValue& self,const auto&,const auto& captures){
        auto trace=captures[0];trace.set(std::to_string(trace.length()),"interval");auto receiver=self;
        receiver.set("interpolate_",captures[1]);return JSValue();
    },{trace,interpolate})});
    value.defineProperty("intervalChanged_",descriptor);
    REQUIRE(curve->evaluate(.5).number(0)==42);
    REQUIRE(trace.get("0").string()=="getInterval");REQUIRE(trace.get("1").string()=="interval");
    REQUIRE(trace.get("2").string()=="interpolate:1");
    REQUIRE(curve->evaluate(.75).number(0)==42);REQUIRE(trace.length()==4);
    auto returned=JSValue::array(1);returned.set("0",71);
    value.set("copySampleValue_",JSValue::function([](const JSValue& self,const auto&,const auto& captures){self.collectGarbage();return captures[0];},{returned}));
    REQUIRE(call(value,"evaluate",{-1}).sameValue(returned));REQUIRE(call(value,"evaluate",{3}).sameValue(returned));
    value.set("interpolate_",nullptr);value.set("_cachedIndex",2);REQUIRE_THROWS_AS(call(value,"evaluate",{1.5}),JSTypeError);
}
TEST_CASE("Animation runtime cubic settings resolve twice and release callback cycles", "[animationruntime][gc]") {
    auto anchor=JSValue::object();std::weak_ptr<Interpolant> weak;
    {
        auto curve=std::make_shared<CubicInterpolant>(Float32Array({0,1}),Float32Array({2,6}),1);weak=curve;
        auto value=nativeInterpolant(curve);auto trace=JSValue::array();auto settings=JSValue::object();
        settings.set("endingStart",ZeroSlopeEnding);settings.set("endingEnd",ZeroSlopeEnding);
        JSPropertyDescriptor descriptor;descriptor.configurable=true;
        descriptor.getter=JSValue::function([](const JSValue& self,const auto&,const auto& captures){
            auto trace=captures[0];trace.set(std::to_string(trace.length()),"get");self.collectGarbage();return captures[1];
        },{trace,JSValue::function([](const JSValue& self,const auto&,const auto& captures){
            auto trace=captures[0];trace.set(std::to_string(trace.length()),"call");self.collectGarbage();return captures[1];
        },{trace,settings,value})});
        value.defineProperty("getSettings_",descriptor);
        REQUIRE(call(value,"evaluate",{.5}).get("0").number()==3.75);
        REQUIRE(trace.length()==4);REQUIRE(trace.get("0").string()=="get");REQUIRE(trace.get("1").string()=="call");
        REQUIRE(trace.get("2").string()=="get");REQUIRE(trace.get("3").string()=="call");
        value={};anchor.collectGarbage();REQUIRE_FALSE(weak.expired());
    }
    anchor.collectGarbage();REQUIRE(weak.expired());
}
