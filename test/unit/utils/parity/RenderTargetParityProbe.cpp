#include "core/RenderTarget.h"
#include "core/RenderTarget3D.h"
#include "core/NativeObject.h"
#include "textures/DepthTexture.h"
#include "textures/Data3DTexture.h"
#include <iostream>
#include <cmath>
#include <functional>
using A = JsonValue::Array;
using O = JsonValue::Object;
JsonValue num(double x) {
    if (std::isnan(x)) return "NaN";
    if (std::isinf(x)) return x < 0 ? "-Infinity" : "Infinity";
    if (x == 0 && std::signbit(x)) return "-0";
    return x;
}
JsonValue rect(const Vector4& x) { return A{num(x.x),num(x.y),num(x.z),num(x.w)}; }
JsonValue state(RenderTarget& t) {
    A attachments;
    for (const auto& x : t.textures) {
        auto image = x->image.get();
        attachments.push_back(O{{"size",A{num(image->width),num(image->height),num(image->depth)}},
            {"version",num(x->version)},{"sourceVersion",num(x->source->version)},
            {"flags",A{x->isRenderTargetTexture,x->isArrayTexture,x->isData3DTexture}},
            {"owner",x->renderTarget == &t},{"options",A{x->mapping,x->wrapS,x->wrapT,x->magFilter,x->minFilter,
                x->format,x->type,x->anisotropy,x->colorSpace,x->flipY,x->generateMipmaps,x->mipmapsAutoUpdate,
                x->internalFormat ? JsonValue(*x->internalFormat) : JsonValue(),x->unpackAlignment}}});
    }
    return O{{"size",A{num(t.width),num(t.height),num(t.depth)}},{"scissor",rect(t.scissor)},
        {"viewport",rect(t.viewport)},{"flags",A{t.scissorTest,t.depthBuffer,t.stencilBuffer,t.resolveColorBuffer,
            t.resolveDepthBuffer,t.resolveStencilBuffer,t.storeMultisampledColorBuffer,t.storeMultisampledDepthBuffer,
            t.storeMultisampledStencilBuffer,t.multiview,t.useArrayDepthTexture,t.isRenderTarget3D}},
        {"samples",num(t.samples)},{"attachments",attachments},
        {"sameSource",t.textures.size() > 1 && t.textures[0]->source == t.textures[1]->source},
        {"sameImage",t.textures.size() > 1 && t.textures[0]->image.get() == t.textures[1]->image.get()}};
}
namespace {
struct CallbackTexture : Texture {
    std::function<std::shared_ptr<Texture>()> onClone;
    std::function<void(JSValue)> onSetValues;
    using Texture::Texture;
    std::shared_ptr<Texture> clone() const override { return onClone ? onClone() : Texture::clone(); }
    void setValues(JSValue values) override { if (onSetValues) onSetValues(values); else Texture::setValues(values); }
};
JsonValue compact(RenderTarget& target, RenderTarget* source = nullptr, RenderTarget* destination = nullptr) {
    A textures;
    for (std::size_t i = 0; i < target.textures.size(); ++i) {
        if (target.textures.storage()->holes.contains(i)) { textures.emplace_back("hole"); continue; }
        const auto texture = target.textures[i]; if (!texture) { textures.emplace_back(nullptr); continue; }
        const auto image = texture->image.get(); const auto flags = target.textures.storage()->flags.contains(i) ? target.textures.storage()->flags.at(i) : SharedArray<std::shared_ptr<Texture>>::Flags{};
        textures.emplace_back(O{{"name",texture->name},{"image", image ? JsonValue(A{num(image->width),num(image->height),num(image->depth)}) : JsonValue()},
            {"version",num(texture->version)},{"sourceVersion",num(texture->source->version)},
            {"owner",texture->renderTarget==nullptr?"none":texture->renderTarget==source?"source":texture->renderTarget==destination?"destination":"other"},
            {"flags",A{texture->isRenderTargetTexture,texture->isArrayTexture,texture->isData3DTexture}},
            {"descriptor",A{flags.writable,flags.enumerable,flags.configurable}}});
    }
    return O{{"size",A{num(target.width),num(target.height),num(target.depth)}},{"scissor",rect(target.scissor)},{"viewport",rect(target.viewport)},
        {"samples",num(target.samples)},{"lengthWritable",target.textures.storage()->lengthWritable},{"textures",textures}};
}
JsonValue copyReentry(int mode) {
    RenderTarget source(4,5,JSValue::fromJSON(O{{"count",2},{"depth",2}})), destination(9,10,JSValue::fromJSON(O{{"count",3}}));
    source.samples = 7; source.scissor.set(1,2,3,4); source.viewport.set(5,6,7,8);
    auto first = std::make_shared<CallbackTexture>(std::make_shared<TextureImage>(4,5,2)); first->name = "callback"; source.textures[0] = first;
    auto sourceArray = source.textures, destinationArray = destination.textures; A trace;
    first->onClone = [&]() -> std::shared_ptr<Texture> {
        trace.emplace_back(O{{"stage","clone"},{"source",compact(source,&source,&destination)},{"destination",compact(destination,&source,&destination)}});
        if (mode == 1) { source.textures[0] = std::make_shared<Texture>(std::make_shared<TextureImage>(13,14,15)); source.textures[0]->name = "replacement"; }
        if (mode == 2) source.textures.clear();
        if (mode == 3) { auto added = std::make_shared<Texture>(); added->name = "appended"; source.textures.push_back(added); }
        if (mode == 4) destination.textures = {std::make_shared<Texture>()};
        if (mode == 5) return first;
        if (mode == 6) { destination.width = 31; source.samples = 23; throw JSTypeError("stop"); }
        if (mode == 7) destination.textures.storage()->lengthWritable = false;
        if (mode == 8) { destination.textures.push_back(std::make_shared<Texture>()); destination.textures.storage()->flags[0].writable = false; }
        if (mode == 9) { source.textures[1] = nullptr; source.textures.storage()->holes.insert(1); }
        if (mode == 10) destination.copy(destination);
        if (mode == 11) destination.textures = SharedArray<std::shared_ptr<Texture>>();
        if (mode == 12) { destination.textures = SharedArray<std::shared_ptr<Texture>>(); destination.textures.storage()->lengthWritable = false; }
        if (mode == 13) { destinationArray.storage()->lengthWritable = false; destination.textures = {std::make_shared<Texture>(),std::make_shared<Texture>()}; }
        if (mode == 14) { destination.textures = {std::make_shared<Texture>()}; destination.textures.storage()->flags[0].writable = false; }
        return first->Texture::clone();
    };
    std::string error;
    try { destination.copy(source); } catch (const JSTypeError&) { error = "TypeError"; }
    return O{{"audit","copy-reentry"},{"mode",mode},{"trace",trace},{"error",error},{"source",compact(source,&source,&destination)},
        {"destination",compact(destination,&source,&destination)},{"sourceArraySame",sourceArray.sameIdentity(source.textures)},
        {"destinationArraySame",destinationArray.sameIdentity(destination.textures)},{"oldDestinationLength",destinationArray.size()}};
}
JsonValue setValuesReentry(int mode) {
    RenderTarget target(3,4,JSValue::fromJSON(O{{"count",2}})); A trace;
    auto first = std::make_shared<CallbackTexture>(std::make_shared<TextureImage>(3,4,1)); first->name = "callback"; target.textures[0] = first;
    auto extra = std::make_shared<CallbackTexture>(std::make_shared<TextureImage>(7,8,1)); extra->name = "extra";
    extra->onSetValues = [&](JSValue values) { trace.emplace_back("extra"); extra->Texture::setValues(values); };
    first->onSetValues = [&](JSValue values) {
        trace.emplace_back(*values.toJSON()); first->Texture::setValues(values);
        if (mode == 1) target.textures.clear();
        if (mode == 2) target.textures.push_back(extra);
        if (mode == 3) target.textures = {first,extra};
        if (mode == 4) { target.textures[1] = nullptr; target.textures.storage()->holes.insert(1); }
        if (mode == 5) throw JSTypeError("stop");
    };
    std::string error; try { target._setTextureOptions(JSValue::fromJSON(O{{"mapping",123},{"flipY",true},{"generateMipmaps",true},{"internalFormat","R8"}})); } catch(const JSTypeError&) { error="TypeError"; }
    return O{{"audit","setValues-reentry"},{"mode",mode},{"trace",trace},{"error",error},{"target",compact(target)}};
}
JsonValue descriptorCase(int mode) {
    RenderTarget source(4,5,JSValue::fromJSON(O{{"count",2}})), destination(9,10,JSValue::fromJSON(O{{"count",3}}));
    if (mode == 0) destination.textures.storage()->lengthWritable = false;
    if (mode == 1) destination.textures.storage()->flags[1].configurable = false;
    if (mode == 2) destination.textures.storage()->flags[2].configurable = false;
    if (mode == 3) { destination.textures.storage()->flags[0].configurable = false; destination.textures.storage()->holes.insert(2); destination.textures[2] = nullptr; }
    if (mode == 4) { source.textures.storage()->holes.insert(1); source.textures[1] = nullptr; }
    if (mode == 5) source.textures[1] = nullptr;
    if (mode == 6) destination.textures = source.textures;
    std::string error; try { destination.copy(source); } catch(const JSTypeError&) { error="TypeError"; }
    return O{{"audit","descriptors"},{"mode",mode},{"error",error},{"source",compact(source,&source,&destination)},{"destination",compact(destination,&source,&destination)}};
}
JsonValue getters(int mode) {
    auto options = JSValue::object(), trace = JSValue::array(), counter = JSValue::object(); counter.set("reads",0);
    JSPropertyDescriptor filter; filter.enumerable = filter.configurable = true;
    filter.getter = JSValue::function([](const JSValue&,const auto&,const auto& captures) {
        auto options=captures[0],trace=captures[1],counter=captures[2];const int mode=int(captures[3].number()),n=int(counter.get("reads").number())+1;counter.set("reads",n);trace.set(std::to_string(trace.length()),"min:"+std::to_string(n));
        if(mode==1&&n==1){options.erase("flipY");options.set("magFilter",NearestFilter);}
        if(mode==2&&n==1)return JSValue();
        if(mode==3&&n==2)return JSValue();
        return JSValue(n%2?NearestFilter:LinearFilter);
    },{options,trace,counter,mode});options.defineProperty("minFilter",filter);
    JSPropertyDescriptor flip;flip.enumerable=flip.configurable=true;
    flip.getter=JSValue::function([](const JSValue&,const auto&,const auto& captures){auto trace=captures[0];trace.set(std::to_string(trace.length()),"flip");return JSValue(true);},{trace});options.defineProperty("flipY",flip);
    options.set("count",2); options.set("wrapR",RepeatWrapping);
    std::shared_ptr<RenderTarget> target;
    if(mode<4) {target=std::make_shared<RenderTarget>();target->_setTextureOptions(options);}
    else if(mode==4) target=std::make_shared<RenderTarget>(3,4,options);
    else target=std::make_shared<RenderTarget3D>(3,4,5,options);
    A wraps;for(const auto& texture:target->textures)if(const auto t=std::dynamic_pointer_cast<Data3DTexture>(texture))wraps.push_back(t->wrapR);else wraps.emplace_back("undefined");
    return O{{"audit","getters"},{"mode",mode},{"trace",*trace.toJSON()},{"reads",counter.get("reads").number()},{"wrapR",wraps},{"state",state(*target)}};
}
JsonValue resizeFailure(int mode) {
    std::shared_ptr<RenderTarget> target = mode >= 5 ? std::make_shared<RenderTarget3D>(3,4,2,JSValue::fromJSON(O{{"count",2}})) : std::make_shared<RenderTarget>(3,4,JSValue::fromJSON(O{{"count",2}}));
    target->viewport.set(1,2,3,4); target->scissor.set(5,6,7,8); A trace;
    target->addEventListener("dispose",[&](Event&){trace.emplace_back(compact(*target)); if(mode==7)target->textures.clear();});
    if(mode==0){target->textures[1]=nullptr;target->textures.storage()->holes.insert(1);}
    if(mode==1)target->textures[1]=nullptr;
    if(mode==2)target->textures[1]->image=nullptr;
    if(mode==3)target->textures[0]->image=nullptr;
    if(mode==4){target->textures.storage()->flags[0].writable=false;target->textures.storage()->lengthWritable=false;}
    if(mode==5)target->textures[0]->isArrayTexture=true;
    if(mode==6)target->textures[0]->isData3DTexture=false;
    std::string error;try{target->setSize(7,8,3);}catch(const JSTypeError&){error="TypeError";}
    return O{{"audit","resize-partial"},{"mode",mode},{"error",error},{"trace",trace},{"state",compact(*target)}};
}
JsonValue pixelsCopy() {
    RenderTarget source(2,2,JSValue::fromJSON(O{{"count",2}}));
    auto pixels=Float32Array({1,2,3,4});source.textures[0]->image.get()->data=pixels;
    auto clone=source.clone(); auto first=clone->textures[0]->image.get(),second=clone->textures[1]->image.get();
    auto& copiedPixels=std::get<Float32Array>(first->data);copiedPixels.setIndex(1,19);
    return O{{"audit","image-data-identity"},{"imageDifferent",first!=source.textures[0]->image.get()},
        {"cloneImagesDifferent",first!=second},{"cloneSourcesDifferent",clone->textures[0]->source!=clone->textures[1]->source},
        {"pixelsShared",pixels.sameIdentity(copiedPixels)},{"sourceValue",pixels.get(1).number()},{"secondValue",std::get<Float32Array>(second->data).get(1).number()},
        {"sourceVersion",source.textures[0]->source->version},{"cloneVersions",A{clone->textures[0]->source->version,clone->textures[1]->source->version}}};
}
}
int main() {
    A result;
    for (bool volume : {false,true}) for (double count : {0.,1.,2.,2.5,-1.,double(NAN)}) {
        auto options = JSValue::fromJSON(O{{"count",count},{"depth",3},{"samples",2.5},
            {"depthBuffer",false},{"resolveColorBuffer",false},{"storeMultisampledDepthBuffer",false},
            {"multiview",true},{"useArrayDepthTexture",true},{"magFilter",NearestFilter},
            {"wrapS",RepeatWrapping},{"colorSpace",SRGBColorSpace}});
        std::shared_ptr<RenderTarget> target = volume ? std::make_shared<RenderTarget3D>(4,5,7,options) : std::make_shared<RenderTarget>(4,5,options);
        A steps{state(*target)};
        A events;
        auto listener = target->addEventListener("dispose",[&](Event& e) { events.push_back(O{{"target",e.target == target.get()},{"state",state(*target)}}); });
        target->viewport.set(1,2,3,4); target->scissor.set(4,3,2,1);
        target->setSize(4,5,volume ? 7 : 3); steps.push_back(state(*target));
        target->setSize(-2.5,0,1); steps.push_back(state(*target));
        target->setSize(NAN,-0.,2); target->setSize(NAN,-0.,2); steps.push_back(state(*target));
        auto copy = target->clone(); steps.push_back(state(*copy));
        A identities;
        for (std::size_t i = 0; i < copy->textures.size(); ++i) identities.push_back(A{
            copy->textures[i] == target->textures[i],copy->textures[i]->source == target->textures[i]->source,
            copy->textures[i]->image.get() == target->textures[i]->image.get(),copy->textures[i]->mipmaps == target->textures[i]->mipmaps});
        result.push_back(O{{"steps",steps},{"events",events},{"cloneIdentity",identities}});
    }
    {
        RenderTarget a,b,c;
        auto depth = std::make_shared<DepthTexture>(3,4);
        A trace;
        a.depthTexture = depth; b.depthTexture = depth;
        trace.push_back(A{depth->renderTarget == &a,a.depthTexture.get() == b.depthTexture.get()});
        auto ac = a.clone(), bc = b.clone();
        trace.push_back(A{ac->depthTexture.get() == depth,ac->depthTexture->renderTarget == ac.get(),
            ac->depthTexture->source == depth->source,bc->depthTexture.get() == depth,bc->depthTexture->renderTarget == &a});
        b.depthTexture = nullptr; trace.push_back(depth->renderTarget == &a);
        a.depthTexture = nullptr; trace.push_back(depth->renderTarget == nullptr);
        b.depthTexture = depth; c.depthTexture = depth; b.copy(c);
        trace.push_back(A{depth->renderTarget == &b,b.depthTexture.get() == c.depthTexture.get()});
        result.push_back(trace);
    }
    {
        RenderTarget target(3,4,JSValue::fromJSON(O{{"count",2}}));
        auto array = target.textures;
        auto old = target.texture.get(); auto replacement = std::make_shared<Texture>();
        target.texture = replacement;
        result.push_back(A{array[0] == replacement,replacement->renderTarget == nullptr,!replacement->isRenderTargetTexture,
            old->renderTarget == &target});
        target.copy(target); result.push_back(state(target));
    }
    {
        RenderTarget target(2,3); A events; bool nested = false;
        target.addEventListener("dispose",[&](Event&) {
            events.push_back(state(target));
            if (!nested) { nested = true; target.setSize(9,8,4); }
        });
        target.setSize(5,6,2); result.push_back(O{{"events",events},{"final",state(target)}});
    }
    {
        RenderTarget target(2,3);
        target.addEventListener("dispose",[](Event&) { throw JSTypeError("stop"); });
        try { target.setSize(5,6,2); } catch (const JSTypeError&) {}
        result.push_back(state(target));
    }
    {
        RenderTarget target; auto trace = JSValue::array();
        auto options = JSValue::object();
        for (const char* key : {"minFilter","flipY","samples"}) {
            JSPropertyDescriptor descriptor; descriptor.enumerable = true;
            descriptor.getter = JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures) {
                auto trace = captures[1]; trace.set(std::to_string(trace.length()), captures[0]); return captures[0].string() == "flipY" ? JSValue(true) : JSValue(NearestFilter);
            },{key,trace}); options.defineProperty(key,descriptor);
        }
        target._setTextureOptions(options); result.push_back(O{{"trace",*trace.toJSON()},{"state",state(target)}});
        trace.set("length", 0); RenderTarget constructed(2,3,options); result.push_back(O{{"trace",*trace.toJSON()},{"state",state(constructed)}});
    }
    for(int mode=0;mode<15;++mode)result.push_back(copyReentry(mode));
    for(int mode=0;mode<6;++mode)result.push_back(setValuesReentry(mode));
    for(int mode=0;mode<7;++mode)result.push_back(descriptorCase(mode));
    for(int mode=0;mode<6;++mode)result.push_back(getters(mode));
    for(int mode=0;mode<8;++mode)result.push_back(resizeFailure(mode));
    result.push_back(pixelsCopy());
    std::cout << JsonValue(result).dump();
}
