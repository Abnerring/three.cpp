#include "renderers/vulkan/VulkanTextureCommands.h"
#include "core/NativeArray.h"
#include "core/JSIterator.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using Args=std::vector<JSValue>;
using Storage=VulkanTextureCommands::Storage;
using FramebufferStorage=VulkanTextureCommands::FramebufferStorage;
using RenderbufferStorage=VulkanTextureCommands::RenderbufferStorage;
using PixelStore=VulkanTextureCommands::PixelStore;
constexpr const char* stateTag="VulkanTextureCommands.State";
constexpr const char* textureTag="VulkanTextureCommands.Texture";
constexpr const char* framebufferTag="VulkanTextureCommands.Framebuffer";
constexpr const char* renderbufferTag="VulkanTextureCommands.Renderbuffer";
constexpr std::uint32_t texture2D=0x0de1,textureCube=0x8513,texture3D=0x806f,textureArray=0x8c1a;
constexpr std::uint32_t firstFace=0x8515,lastFace=0x851a,texture0=0x84c0;
constexpr std::uint32_t invalidEnum=0x0500,invalidValue=0x0501,invalidOperation=0x0502,outOfMemory=0x0505;
constexpr std::uint32_t framebuffer=0x8d40,drawFramebuffer=0x8ca9,readFramebuffer=0x8ca8;
constexpr std::uint32_t renderbufferTarget=0x8d41,colorAttachment0=0x8ce0,depthAttachment=0x8d00,stencilAttachment=0x8d20,depthStencilAttachment=0x821a;
constexpr std::uint32_t backBuffer=0x0405;
struct State;
struct Level { int width=0,height=0,depth=1;std::uint32_t internalFormat=0; };
struct Texture {
    std::weak_ptr<State> owner;
    std::shared_ptr<Storage> storage;
    std::optional<std::uint32_t> target;
    std::map<std::pair<std::uint32_t,int>,Level> levels;
    std::map<std::uint32_t,double> parameters;
    std::uint64_t revision=0,generation=0;
    std::size_t attachments=0;
    bool immutable=false,deleted=false;
};
struct TextureBinding { JSWeakValue object;std::shared_ptr<Texture> texture; };
struct Renderbuffer {
    std::weak_ptr<State> owner;
    std::shared_ptr<RenderbufferStorage> storage;
    std::uint32_t internalFormat=0x8056;
    int width=0,height=0,samples=0;
    std::size_t attachments=0;
    bool bound=false,allocated=false,deleted=false;
};
struct RenderbufferBinding {JSWeakValue object;std::shared_ptr<Renderbuffer> renderbuffer;};
struct Attachment {
    JSWeakValue object;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Renderbuffer> renderbuffer;
    std::uint32_t target=0;int level=0,layer=-1;
};
struct Framebuffer {
    std::weak_ptr<State> owner;
    JSWeakValue object;
    std::shared_ptr<FramebufferStorage> storage;
    std::map<std::uint32_t,Attachment> attachments;
    std::vector<std::uint32_t> drawBuffers{colorAttachment0};
    std::uint32_t readBuffer=colorAttachment0;
    std::uint64_t revision=0;
    bool bound=false,deleted=false;
    void clearAttachments() {
        for(auto& [_,a]:attachments) {
            if(a.texture && --a.texture->attachments==0 && a.texture->deleted)a.texture->storage.reset();
            if(a.renderbuffer && --a.renderbuffer->attachments==0 && a.renderbuffer->deleted)a.renderbuffer->storage.reset();
        }
        attachments.clear();
    }
    ~Framebuffer(){clearAttachments();}
};
struct FramebufferBinding {JSWeakValue object;std::shared_ptr<Framebuffer> framebuffer;};
struct State {
    VulkanTextureCommands::Factory factory;
    VulkanTextureCommands::FramebufferFactory framebufferFactory;
    VulkanTextureCommands::RenderbufferFactory renderbufferFactory;
    VulkanTextureCommands::Limits limits;
    // Descriptor limits can exceed a million units. Only bound units belong in
    // the ownership graph; querying or unbinding an empty unit allocates none.
    std::map<std::uint32_t,std::array<TextureBinding,4>> bindings;
    std::vector<std::weak_ptr<Texture>> textures;
    std::vector<std::weak_ptr<Framebuffer>> framebuffers;
    std::vector<std::weak_ptr<Renderbuffer>> renderbuffers;
    FramebufferBinding draw,read;
    RenderbufferBinding renderbuffer;
    std::vector<std::uint32_t> defaultDrawBuffers{backBuffer};
    std::uint32_t defaultReadBuffer=backBuffer;
    std::uint64_t defaultFramebufferRevision=0;
    PixelStore unpack;
    int packAlignment=4;
    std::uint32_t activeUnit=0;
    std::uint64_t nextGeneration=1;
    std::array<bool,6> errors{};
    bool closed=false;
    State(VulkanTextureCommands::Factory f,VulkanTextureCommands::FramebufferFactory fb,VulkanTextureCommands::RenderbufferFactory rb,VulkanTextureCommands::Limits l)
        :factory(std::move(f)),framebufferFactory(std::move(fb)),renderbufferFactory(std::move(rb)),limits(l){}
    void requireOpen()const {if(closed)throw JSTypeError("Vulkan texture commands are shut down");}
    void error(std::uint32_t value){errors[value-invalidEnum]=true;}
    std::uint32_t getError(){for(std::size_t i=0;i<errors.size();++i)if(errors[i]){errors[i]=false;return invalidEnum+std::uint32_t(i);}return 0;}
    void close(){
        if(closed)return;closed=true;
        for(const auto& weak:framebuffers)if(auto f=weak.lock()){f->deleted=true;f->clearAttachments();f->storage.reset();}
        for(const auto& weak:textures)if(auto t=weak.lock()){t->deleted=true;t->storage.reset();}
        for(const auto& weak:renderbuffers)if(auto r=weak.lock()){r->deleted=true;r->storage.reset();}
        bindings.clear();draw={};read={};renderbuffer={};textures.clear();framebuffers.clear();renderbuffers.clear();factory={};framebufferFactory={};renderbufferFactory={};
    }
    ~State(){close();}
};
const JSValue& arg(const Args& args,std::size_t i){static const JSValue undefined;return i<args.size()?args[i]:undefined;}
void requireArgs(const Args& args,std::size_t n){if(args.size()<n)throw JSTypeError("Insufficient texture command arguments");}
std::uint32_t unsignedInteger(const JSValue& value){
    const double x=value.toNumber();if(!std::isfinite(x)||x==0)return 0;
    const auto magnitude=std::uint64_t(std::fmod(std::trunc(std::abs(x)),4294967296.0));
    return std::uint32_t(x<0 ? std::uint64_t(0)-magnitude : magnitude);
}
int integer(const JSValue& value){const auto n=unsignedInteger(value);return n<=0x7fffffffu?int(n):int(std::int64_t(n)-INT64_C(4294967296));}
std::shared_ptr<State> state(const JSValue& value){static_cast<void>(JSWeakValue(value));return std::static_pointer_cast<State>(value.hostPayload(stateTag));}
std::shared_ptr<Texture> texture(const JSValue& value,bool nullable=false){
    static_cast<void>(JSWeakValue(value));if(nullable&&(value.isNull()||value.isUndefined()))return {};
    if(!value.isHostObject(textureTag))throw JSTypeError("Expected a native Vulkan texture handle");
    return std::static_pointer_cast<Texture>(value.hostPayload(textureTag));
}
std::shared_ptr<Framebuffer> framebufferHandle(const JSValue& value,bool nullable=false){
    static_cast<void>(JSWeakValue(value));if(nullable&&(value.isNull()||value.isUndefined()))return {};
    if(!value.isHostObject(framebufferTag))throw JSTypeError("Expected a native Vulkan framebuffer handle");
    return std::static_pointer_cast<Framebuffer>(value.hostPayload(framebufferTag));
}
std::shared_ptr<Renderbuffer> renderbufferHandle(const JSValue& value,bool nullable=false){
    static_cast<void>(JSWeakValue(value));if(nullable&&(value.isNull()||value.isUndefined()))return {};
    if(!value.isHostObject(renderbufferTag))throw JSTypeError("Expected a native Vulkan renderbuffer handle");
    return std::static_pointer_cast<Renderbuffer>(value.hostPayload(renderbufferTag));
}
std::optional<std::size_t> bindingIndex(std::uint32_t target){
    switch(target){case texture2D:return 0;case textureCube:return 1;case texture3D:return 2;case textureArray:return 3;default:return {};}
}
std::uint32_t bindingTarget(std::uint32_t target){return target>=firstFace&&target<=lastFace?textureCube:target;}
const TextureBinding* findBinding(const State& c,std::uint32_t unit,std::size_t index){
    const auto it=c.bindings.find(unit);return it==c.bindings.end()?nullptr:&it->second[index];
}
bool emptyBindings(const std::array<TextureBinding,4>& unit){
    return std::none_of(unit.begin(),unit.end(),[](const auto& binding){return bool(binding.texture);});
}
std::string bindingKey(std::uint32_t unit,std::size_t index){return std::to_string(unit)+":"+std::to_string(index);}
std::shared_ptr<Texture> bound(State& c,std::uint32_t target){
    const auto i=bindingIndex(bindingTarget(target));if(!i){c.error(invalidEnum);return {};}
    const auto b=findBinding(c,c.activeUnit,*i);const auto t=b?b->texture:nullptr;
    if(!t||t->deleted||!t->storage){c.error(invalidOperation);return {};}return t;
}
void revisionAvailable(const Texture& t){if(t.revision==std::numeric_limits<std::uint64_t>::max())throw JSRangeError("Vulkan texture revision exhausted");}
void property(JSValue object,const char* key,JSValue value){JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void slot(JSValue object,const char* key,const JSValue& value){object.setInternalSlot(key,value);}
JSValue weakObject(const JSWeakValue& value){const auto v=value.lock();return v&&!v->isUndefined()?*v:JSValue(nullptr);}
JSValue bindingObject(const State& c,std::uint32_t unit,std::size_t index){
    const auto binding=findBinding(c,unit,index);return binding?weakObject(binding->object):JSValue(nullptr);
}
void setBinding(const JSValue& wrapper,State& c,std::size_t index,const JSValue& object,const std::shared_ptr<Texture>& t){
    auto roots=wrapper.getInternalSlot("textureBindings");const auto key=bindingKey(c.activeUnit,index);
    if(t){
        roots.set(key,object);c.bindings[c.activeUnit][index]={JSWeakValue(object),t};
    }else{
        roots.erase(key);
        if(const auto it=c.bindings.find(c.activeUnit);it!=c.bindings.end()){
            it->second[index]={};if(emptyBindings(it->second))c.bindings.erase(it);
        }
    }
}
JSNativeOwnerNode textureNode(const std::shared_ptr<Texture>& t){JSNativeOwnerNode n{t,{},{}};if(t->storage)n.edges.push_back({&t->storage,t->storage});return n;}
void appendTexture(std::vector<JSNativeOwnerNode>& nodes,const std::shared_ptr<Texture>& t){nodes.push_back(textureNode(t));if(t->storage)nodes.push_back({t->storage,{},{}});}
void appendRenderbuffer(std::vector<JSNativeOwnerNode>& nodes,const std::shared_ptr<Renderbuffer>& r){
    JSNativeOwnerNode n{r,{},{}};if(r->storage){n.edges.push_back({&r->storage,r->storage});nodes.push_back({r->storage,{},{}});}nodes.push_back(std::move(n));
}
void appendFramebuffer(std::vector<JSNativeOwnerNode>& nodes,const std::shared_ptr<Framebuffer>& f){
    JSNativeOwnerNode n{f,{},{}};if(f->storage){n.edges.push_back({&f->storage,f->storage});nodes.push_back({f->storage,{},{}});}
    for(const auto& [_,a]:f->attachments){
        if(a.texture){n.edges.push_back({&a.texture,a.texture});appendTexture(nodes,a.texture);}
        if(a.renderbuffer){n.edges.push_back({&a.renderbuffer,a.renderbuffer});appendRenderbuffer(nodes,a.renderbuffer);}
    }
    nodes.push_back(std::move(n));
}
JSValue createTexture(const std::shared_ptr<State>& c){
    if(c->nextGeneration==std::numeric_limits<std::uint64_t>::max())throw JSRangeError("Vulkan texture generation exhausted");
    auto t=std::make_shared<Texture>();t->owner=c;const auto factory=c->factory;
    try{t->storage=factory();}catch(const std::bad_alloc&){c->requireOpen();c->error(outOfMemory);return nullptr;}
    c->requireOpen();if(!t->storage){c->error(outOfMemory);return nullptr;}t->generation=c->nextGeneration++;
    auto v=JSValue::hostObject(t,t.get(),textureTag);
    static const auto hooks=[]{auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=+[](const JSValue& v){std::vector<JSNativeOwnerNode> n;appendTexture(n,texture(v));return n;};return h;}();
    v.setHostHooks(hooks);if(c->textures.size()%64==0)std::erase_if(c->textures,[](const auto& w){return w.expired();});c->textures.emplace_back(t);return v;
}
JSValue createFramebuffer(const std::shared_ptr<State>& c){
    if(!c->framebufferFactory)throw JSTypeError("Vulkan framebuffer storage is not installed");
    auto f=std::make_shared<Framebuffer>();f->owner=c;const auto factory=c->framebufferFactory;
    try{f->storage=factory();}catch(const std::bad_alloc&){c->requireOpen();c->error(outOfMemory);return nullptr;}
    c->requireOpen();if(!f->storage){c->error(outOfMemory);return nullptr;}
    auto v=JSValue::hostObject(f,f.get(),framebufferTag);f->object=JSWeakValue(v);v.setInternalSlot("textureAttachments",JSValue::object());
    static const auto hooks=[]{auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=+[](const JSValue& v){std::vector<JSNativeOwnerNode> n;appendFramebuffer(n,framebufferHandle(v));return n;};return h;}();
    v.setHostHooks(hooks);if(c->framebuffers.size()%64==0)std::erase_if(c->framebuffers,[](const auto& w){return w.expired();});c->framebuffers.emplace_back(f);return v;
}
JSValue createRenderbuffer(const std::shared_ptr<State>& c){
    if(!c->renderbufferFactory)throw JSTypeError("Vulkan renderbuffer storage is not installed");
    auto r=std::make_shared<Renderbuffer>();r->owner=c;const auto factory=c->renderbufferFactory;
    try{r->storage=factory();}catch(const std::bad_alloc&){c->requireOpen();c->error(outOfMemory);return nullptr;}
    c->requireOpen();if(!r->storage){c->error(outOfMemory);return nullptr;}
    auto v=JSValue::hostObject(r,r.get(),renderbufferTag);
    static const auto hooks=[]{auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=+[](const JSValue& v){std::vector<JSNativeOwnerNode> n;appendRenderbuffer(n,renderbufferHandle(v));return n;};return h;}();
    v.setHostHooks(hooks);if(c->renderbuffers.size()%64==0)std::erase_if(c->renderbuffers,[](const auto& w){return w.expired();});c->renderbuffers.emplace_back(r);return v;
}
struct View {InterpolantArray value;std::size_t bytes,elementSize;std::uint32_t type;};
View view(const JSValue& v){
    static_cast<void>(JSWeakValue(v));auto a=NativeArray_detail::unwrap(v);
    return std::visit([&](const auto& values)->View{
        using T=std::decay_t<decltype(values)>;
        if constexpr(std::is_same_v<T,JSValue>)throw JSTypeError("Texture upload requires a native ArrayBufferView");
        else {
            constexpr std::uint32_t type=std::is_same_v<T,Int8Array>?0x1400:
                std::is_same_v<T,Uint8Array>||std::is_same_v<T,Uint8ClampedArray>?0x1401:
                std::is_same_v<T,Int16Array>?0x1402:std::is_same_v<T,Uint16Array>?0x1403:
                std::is_same_v<T,Int32Array>?0x1404:std::is_same_v<T,Uint32Array>?0x1405:
                std::is_same_v<T,Float32Array>?0x1406:0;
            return {a,std::size_t(values.byteLength),T::BYTES_PER_ELEMENT,type};
        }
    },a.storage());
}
std::vector<std::byte> copyBytes(const View& source,std::size_t offset){
    return std::visit([&](const auto& values)->std::vector<std::byte>{
        using T=std::decay_t<decltype(values)>;
        if constexpr(std::is_same_v<T,JSValue>)throw JSTypeError("Texture upload requires an ArrayBufferView");
        else{const Uint8Array bytes(values.buffer,double(std::size_t(values.byteOffset)),double(std::size_t(values.byteLength)));std::vector<std::byte> out(source.bytes-offset);for(std::size_t i=offset;i<source.bytes;++i)out[i-offset]=std::byte(std::uint8_t(bytes[i]));return out;}
    },source.value.storage());
}
std::optional<std::size_t> pixelSize(std::uint32_t format,std::uint32_t type){
    std::size_t components;
    switch(format){case 0x1902:case 0x1903:case 0x1906:case 0x1909:case 0x8d94:components=1;break;
        case 0x8227:case 0x8228:case 0x190a:components=2;break;case 0x1907:case 0x8d98:components=3;break;
        case 0x1908:case 0x8d99:components=4;break;case 0x84f9:components=1;break;default:return {};}
    switch(type){case 0x1400:case 0x1401:return components;case 0x1402:case 0x1403:case 0x140b:return components*2;
        case 0x1404:case 0x1405:case 0x1406:return components*4;
        case 0x8033:case 0x8034:return format==0x1908?std::optional<std::size_t>(2):std::nullopt;
        case 0x8363:return format==0x1907?std::optional<std::size_t>(2):std::nullopt;
        case 0x8368:return format==0x1908||format==0x8d99?std::optional<std::size_t>(4):std::nullopt;
        case 0x8c3b:case 0x8c3e:return format==0x1907?std::optional<std::size_t>(4):std::nullopt;
        case 0x84fa:return format==0x84f9?std::optional<std::size_t>(4):std::nullopt;
        case 0x8dad:return format==0x84f9?std::optional<std::size_t>(8):std::nullopt;default:return {};}
}
bool addSize(std::size_t a,std::size_t b,std::size_t& out){if(b>std::numeric_limits<std::size_t>::max()-a)return false;out=a+b;return true;}
bool mulSize(std::size_t a,std::size_t b,std::size_t& out){if(a&&b>std::numeric_limits<std::size_t>::max()/a)return false;out=a*b;return true;}
std::optional<std::size_t> requiredBytes(const PixelStore& p,int width,int height,int depth,std::size_t pixelBytes){
    if(!width||!height||!depth)return 0;
    std::size_t row,rounded,stride,skip,images,last;
    if(!mulSize(std::size_t(p.rowLength?p.rowLength:width),pixelBytes,row)||!addSize(row,std::size_t(p.alignment-1),rounded))return {};
    row=rounded/std::size_t(p.alignment)*std::size_t(p.alignment);
    if(!mulSize(row,std::size_t(p.imageHeight?p.imageHeight:height),stride)||!mulSize(stride,std::size_t(p.skipImages),skip))return {};
    if(!mulSize(row,std::size_t(p.skipRows),images)||!addSize(skip,images,skip)||!mulSize(std::size_t(p.skipPixels),pixelBytes,images)||!addSize(skip,images,skip))return {};
    if(!mulSize(stride,std::size_t(depth-1),images)||!addSize(skip,images,skip)||!mulSize(row,std::size_t(height-1),last)||!addSize(skip,last,skip)||!mulSize(std::size_t(width),pixelBytes,last)||!addSize(skip,last,skip))return {};
    return skip;
}
bool validDimensions(State& c,std::uint32_t target,int level,int width,int height,int depth){
    if(level<0||width<0||height<0||depth<0){c.error(invalidValue);return false;}
    const auto base=bindingTarget(target);
    const int limit=base==textureCube?c.limits.maxCubeMapSize:base==texture3D?c.limits.max3DTextureSize:c.limits.maxTextureSize;
    if(level>=31||width>(limit>>level)||height>(limit>>level)||(base==textureCube&&width!=height)||(base==texture3D&&depth>(limit>>level))||(base==textureArray&&depth>c.limits.maxArrayLayers)){
        c.error(invalidValue);return false;
    }return true;
}
bool isImageTarget(std::uint32_t target,bool is3D){return is3D?(target==texture3D||target==textureArray):(target==texture2D||(target>=firstFace&&target<=lastFace));}
JSValue pixelStore(State& c,const Args& a){
    requireArgs(a,2);const auto name=unsignedInteger(a[0]);const int v=integer(a[1]);c.requireOpen();
    switch(name){
        case 0x0cf5:case 0x0d05:if(v!=1&&v!=2&&v!=4&&v!=8){c.error(invalidValue);break;}if(name==0x0cf5)c.unpack.alignment=v;else c.packAlignment=v;break;
        case 0x0cf2:case 0x806e:case 0x0cf4:case 0x0cf3:case 0x806d:
            if(v<0){c.error(invalidValue);break;}
            if(name==0x0cf2)c.unpack.rowLength=v;else if(name==0x806e)c.unpack.imageHeight=v;else if(name==0x0cf4)c.unpack.skipPixels=v;else if(name==0x0cf3)c.unpack.skipRows=v;else c.unpack.skipImages=v;break;
        case 0x9240:c.unpack.flipY=v!=0;break;case 0x9241:c.unpack.premultiplyAlpha=v!=0;break;
        case 0x9243:if(v!=0&&v!=0x9244)c.error(invalidValue);else c.unpack.colorSpaceConversion=std::uint32_t(v);break;
        default:c.error(invalidEnum);break;
    }return {};
}
JSValue storageCommand(const std::shared_ptr<State>& c,const Args& a,bool is3D){
    requireArgs(a,is3D?6:5);const auto target=unsignedInteger(a[0]);const int levels=integer(a[1]);const auto format=unsignedInteger(a[2]);
    const int width=integer(a[3]),height=integer(a[4]),depth=is3D?integer(a[5]):1;c->requireOpen();
    if(is3D ? (target!=texture3D&&target!=textureArray):(target!=texture2D&&target!=textureCube)){c->error(invalidEnum);return {};}
    const auto t=bound(*c,target);if(!t)return {};
    if(t->immutable){c->error(invalidOperation);return {};}
    if(!validDimensions(*c,target,0,width,height,depth))return {};
    const int largest=std::max(width,std::max(height,target==texture3D?depth:1));
    if(width==0||height==0||depth==0||levels<=0){c->error(invalidValue);return {};}
    if(levels>int(std::floor(std::log2(double(largest))))+1){c->error(invalidOperation);return {};}
    // Sized internal formats are validated by the GPU implementation. Unknown
    // formats must throw there; a generic command layer cannot infer extensions.
    revisionAvailable(*t);VulkanTextureCommands::Allocation allocation{target,format,0,levels,width,height,depth,true};
    t->storage->allocate(allocation);c->requireOpen();
    t->levels.clear();for(int i=0;i<levels;++i){const Level l{std::max(1,width>>i),std::max(1,height>>i),target==texture3D?std::max(1,depth>>i):depth,format};if(target==textureCube)for(auto f=firstFace;f<=lastFace;++f)t->levels[{f,i}]=l;else t->levels[{target,i}]=l;}
    t->immutable=true;++t->revision;return {};
}
JSValue imageCommand(const std::shared_ptr<State>& c,const Args& a,bool is3D,bool sub){
    const std::size_t explicitCount=is3D?10:9;
    if(a.size()<explicitCount)throw JSTypeError("Native texture commands do not support browser image-source overloads");
    const auto target=unsignedInteger(a[0]);const int level=integer(a[1]);
    int x=0,y=0,z=0,width,height,depth=1;std::uint32_t internalFormat=0,format,type;std::size_t dataIndex;
    if(sub){
        x=integer(a[2]);y=integer(a[3]);if(is3D){z=integer(a[4]);width=integer(a[5]);height=integer(a[6]);depth=integer(a[7]);format=unsignedInteger(a[8]);type=unsignedInteger(a[9]);dataIndex=10;}
        else{width=integer(a[4]);height=integer(a[5]);format=unsignedInteger(a[6]);type=unsignedInteger(a[7]);dataIndex=8;}
    }else{
        internalFormat=unsignedInteger(a[2]);width=integer(a[3]);height=integer(a[4]);if(is3D)depth=integer(a[5]);
        const int border=integer(a[is3D?6:5]);format=unsignedInteger(a[is3D?7:6]);type=unsignedInteger(a[is3D?8:7]);dataIndex=is3D?9:8;
        if(border!=0){c->error(invalidValue);return {};}
    }
    requireArgs(a,dataIndex+1);
    const bool nullData=arg(a,dataIndex).isNull();std::optional<View> source;
    if(!nullData)source=view(arg(a,dataIndex));
    const auto sourceOffset=a.size()>dataIndex+1?unsignedInteger(a[dataIndex+1]):0u;c->requireOpen();
    if(!isImageTarget(target,is3D)){c->error(invalidEnum);return {};}
    const auto t=bound(*c,target);if(!t)return {};
    if(!validDimensions(*c,target,level,width,height,depth))return {};
    if(x<0||y<0||z<0){c->error(invalidValue);return {};}
    if(!sub&&t->immutable){c->error(invalidOperation);return {};}
    if(sub){const auto it=t->levels.find({target,level});if(it==t->levels.end()){c->error(invalidOperation);return {};}
        const auto& l=it->second;if(x>l.width||width>l.width-x||y>l.height||height>l.height-y||z>l.depth||depth>l.depth-z){c->error(invalidValue);return {};}
        if(nullData){c->error(invalidValue);return {};}}
    const auto pixelBytes=pixelSize(format,type);if(!pixelBytes){c->error(invalidEnum);return {};}
    std::vector<std::byte> bytes;
    const auto unpack=c->unpack;
    if(source){
        const auto requiredType=type==0x140b||type==0x8033||type==0x8034||type==0x8363?0x1403u:
            type==0x8368||type==0x8c3b||type==0x8c3e||type==0x84fa||type==0x8dad?0x1405u:type;
        if(source->type!=requiredType){c->error(invalidOperation);return {};}
        std::size_t first;if(!mulSize(std::size_t(sourceOffset),source->elementSize,first)||first>source->bytes){c->error(invalidOperation);return {};}
        const auto required=requiredBytes(unpack,width,height,depth,*pixelBytes);if(!required||*required>source->bytes-first){c->error(invalidOperation);return {};}
        bytes=copyBytes(*source,first);}
    revisionAvailable(*t);
    if(!sub){t->storage->allocate({target,internalFormat,level,1,width,height,depth,false});c->requireOpen();t->levels[{target,level}]={width,height,depth,internalFormat};++t->revision;}
    if(width==0||height==0||depth==0)return {};
    revisionAvailable(*t);VulkanTextureCommands::Upload upload{target,format,type,level,x,y,z,width,height,depth,bytes,unpack,nullData,false};
    t->storage->upload(upload);c->requireOpen();++t->revision;return {};
}
bool validParameter(State& c,std::uint32_t name,double value){
    switch(name){case 0x2800:if(value==0x2600||value==0x2601)return true;break;
        case 0x2801:if(value==0x2600||value==0x2601||(value>=0x2700&&value<=0x2703&&std::trunc(value)==value))return true;break;
        case 0x2802:case 0x2803:case 0x8072:if(value==0x2901||value==0x812f||value==0x8370)return true;break;
        case 0x884c:if(value==0||value==0x884e)return true;break;
        case 0x884d:if(value>=0x0200&&value<=0x0207&&std::trunc(value)==value)return true;break;
        case 0x813c:case 0x813d:if(value>=0&&value<=double(std::numeric_limits<int>::max()))return true;c.error(invalidValue);return false;
        case 0x813a:case 0x813b:return true;
        case 0x84fe:if(std::isfinite(value)&&value>=1)return true;c.error(invalidValue);return false;
        default:c.error(invalidEnum);return false;
    }c.error(invalidEnum);return false;
}
struct FormatBits {int red=0,green=0,blue=0,alpha=0,depth=0,stencil=0;bool integer=false;};
std::optional<FormatBits> renderbufferFormat(std::uint32_t format){
    switch(format){
        case 0x8229:return FormatBits{8};case 0x822b:return FormatBits{8,8};case 0x8051:return FormatBits{8,8,8};
        case 0x8058:case 0x8c43:return FormatBits{8,8,8,8};case 0x8d62:return FormatBits{5,6,5};
        case 0x8056:return FormatBits{4,4,4,4};case 0x8057:return FormatBits{5,5,5,1};case 0x8059:return FormatBits{10,10,10,2};
        case 0x822d:return FormatBits{16};case 0x822f:return FormatBits{16,16};case 0x881a:return FormatBits{16,16,16,16};
        case 0x822e:return FormatBits{32};case 0x8230:return FormatBits{32,32};case 0x8814:return FormatBits{32,32,32,32};case 0x8c3a:return FormatBits{11,11,10};
        case 0x8231:case 0x8232:return FormatBits{8,0,0,0,0,0,true};case 0x8233:case 0x8234:return FormatBits{16,0,0,0,0,0,true};
        case 0x8235:case 0x8236:return FormatBits{32,0,0,0,0,0,true};case 0x8237:case 0x8238:return FormatBits{8,8,0,0,0,0,true};
        case 0x8239:case 0x823a:return FormatBits{16,16,0,0,0,0,true};case 0x823b:case 0x823c:return FormatBits{32,32,0,0,0,0,true};
        case 0x8d8e:case 0x8d7c:return FormatBits{8,8,8,8,0,0,true};case 0x8d88:case 0x8d76:return FormatBits{16,16,16,16,0,0,true};
        case 0x8d82:case 0x8d70:return FormatBits{32,32,32,32,0,0,true};case 0x906f:return FormatBits{10,10,10,2,0,0,true};
        case 0x81a5:return FormatBits{0,0,0,0,16};case 0x81a6:return FormatBits{0,0,0,0,24};case 0x8cac:return FormatBits{0,0,0,0,32};
        case 0x8d48:return FormatBits{0,0,0,0,0,8};case 0x84f9:case 0x88f0:return FormatBits{0,0,0,0,24,8};case 0x8cad:return FormatBits{0,0,0,0,32,8};
        default:return {};
    }
}
void framebufferRevisionAvailable(const Framebuffer& f){if(f.revision==std::numeric_limits<std::uint64_t>::max())throw JSRangeError("Vulkan framebuffer revision exhausted");}
bool validFramebufferTarget(std::uint32_t target){return target==framebuffer||target==drawFramebuffer||target==readFramebuffer;}
bool validAttachment(const State& c,std::uint32_t name){return (name>=colorAttachment0&&name-colorAttachment0<std::uint32_t(c.limits.maxColorAttachments))||name==depthAttachment||name==stencilAttachment||name==depthStencilAttachment;}
std::vector<std::uint32_t> attachmentNames(std::uint32_t attachment){return attachment==depthStencilAttachment?std::vector<std::uint32_t>{depthAttachment,stencilAttachment}:std::vector<std::uint32_t>{attachment};}
void clearAttachment(Framebuffer& f,std::uint32_t name){
    const auto it=f.attachments.find(name);if(it==f.attachments.end())return;
    auto& a=it->second;
    if(a.texture&&--a.texture->attachments==0&&a.texture->deleted)a.texture->storage.reset();
    if(a.renderbuffer&&--a.renderbuffer->attachments==0&&a.renderbuffer->deleted)a.renderbuffer->storage.reset();
    f.attachments.erase(it);
}
void attachmentRoot(const Framebuffer& f,std::uint32_t name,const JSValue& object){
    if(const auto wrapper=f.object.lock())wrapper->getInternalSlot("textureAttachments").set(std::to_string(name),object);
}
std::vector<std::uint32_t> enumSequence(const JSValue& value){
    std::vector<std::uint32_t> result;auto iterator=getJSIterator(value);
    while(const auto item=stepJSIterator(iterator))result.push_back(unsignedInteger(*item));
    return result;
}
JSValue getParameter(State& c,std::uint32_t name){
    switch(name){case 0x84e0:return texture0+c.activeUnit;case 0x0cf5:return c.unpack.alignment;case 0x0d05:return c.packAlignment;
        case 0x0cf2:return c.unpack.rowLength;case 0x806e:return c.unpack.imageHeight;case 0x0cf4:return c.unpack.skipPixels;case 0x0cf3:return c.unpack.skipRows;case 0x806d:return c.unpack.skipImages;
        case 0x9240:return c.unpack.flipY;case 0x9241:return c.unpack.premultiplyAlpha;case 0x9243:return c.unpack.colorSpaceConversion;
        case 0x0d33:return c.limits.maxTextureSize;case 0x851c:return c.limits.maxCubeMapSize;case 0x8073:return c.limits.max3DTextureSize;case 0x88ff:return c.limits.maxArrayLayers;
        case 0x8872:case 0x8b4d:return c.limits.maxTextureUnits;case 0x8d57:return c.limits.maxSamples;
        case 0x84e8:return c.limits.maxRenderbufferSize;case 0x8cdf:return c.limits.maxColorAttachments;case 0x8824:return c.limits.maxDrawBuffers;
        case 0x8069:return bindingObject(c,c.activeUnit,0);case 0x8514:return bindingObject(c,c.activeUnit,1);case 0x806a:return bindingObject(c,c.activeUnit,2);case 0x8c1d:return bindingObject(c,c.activeUnit,3);
        case 0x8ca6:return weakObject(c.draw.object);case 0x8caa:return weakObject(c.read.object);case 0x8ca7:return weakObject(c.renderbuffer.object);
        case 0x0c02:return c.read.framebuffer?c.read.framebuffer->readBuffer:c.defaultReadBuffer;
        default:
            if(name>=0x8825&&name-0x8825<std::uint32_t(c.limits.maxDrawBuffers)){
                const auto& buffers=c.draw.framebuffer?c.draw.framebuffer->drawBuffers:c.defaultDrawBuffers;
                const auto index=name-0x8825;return index<buffers.size()?buffers[index]:0;
            }
            c.error(invalidEnum);return nullptr;
    }
}
std::shared_ptr<Framebuffer> boundFramebuffer(State& c,std::uint32_t target){
    if(target!=framebuffer&&target!=drawFramebuffer&&target!=readFramebuffer){c.error(invalidEnum);return {};}
    const auto f=(target==readFramebuffer?c.read:c.draw).framebuffer;if(!f||f->deleted||!f->storage){c.error(invalidOperation);return {};}return f;
}
JSValue attachTexture(const std::shared_ptr<State>& c,const Args& a,bool layered){
    requireArgs(a,5);const auto target=unsignedInteger(a[0]),attachment=unsignedInteger(a[1]);
    const auto imageTarget=layered?0u:unsignedInteger(a[2]);const auto& object=a[layered?2:3];const auto t=texture(object,true);
    const int level=integer(a[layered?3:4]),layer=layered?integer(a[4]):-1;c->requireOpen();
    const auto f=boundFramebuffer(*c,target);if(!f)return {};
    if(!validAttachment(*c,attachment)){c->error(invalidEnum);return {};}
    if(!layered&&!isImageTarget(imageTarget,false)){c->error(invalidEnum);return {};}
    if(level<0||(layered&&layer<0)){c->error(invalidValue);return {};}
    if(t&&(t->owner.lock()!=c||t->deleted||!t->storage)){c->error(invalidOperation);return {};}
    if(t){
        if(!t->target){c->error(invalidOperation);return {};}
        if(layered&&*t->target!=texture3D&&*t->target!=textureArray){c->error(invalidOperation);return {};}
        if(!layered&&(!isImageTarget(imageTarget,false)||bindingTarget(imageTarget)!=*t->target)){c->error(invalidOperation);return {};}
    }
    const auto actualTarget=t?(layered?*t->target:imageTarget):imageTarget;
    framebufferRevisionAvailable(*f);f->storage->texture(attachment,t?t->storage:nullptr,actualTarget,level,layer);c->requireOpen();
    for(const auto name:attachmentNames(attachment)){
        clearAttachment(*f,name);attachmentRoot(*f,name,t?object:JSValue(nullptr));
        if(t){++t->attachments;f->attachments.emplace(name,Attachment{JSWeakValue(object),t,{},actualTarget,level,layer});}
    }
    ++f->revision;return {};
}
JSValue allocateRenderbuffer(const std::shared_ptr<State>& c,const Args& a,bool multisample){
    requireArgs(a,multisample?5:4);const auto target=unsignedInteger(a[0]);
    const int samples=multisample?integer(a[1]):0;const auto format=unsignedInteger(a[multisample?2:1]);
    const int width=integer(a[multisample?3:2]),height=integer(a[multisample?4:3]);c->requireOpen();
    if(target!=renderbufferTarget){c->error(invalidEnum);return {};}
    const auto bits=renderbufferFormat(format);if(!bits){c->error(invalidEnum);return {};}
    if(width<0||height<0||width>c->limits.maxRenderbufferSize||height>c->limits.maxRenderbufferSize||samples<0||samples>c->limits.maxSamples){c->error(invalidValue);return {};}
    if(samples>0&&(bits->integer||format==0x84f9)){c->error(invalidOperation);return {};}
    const auto r=c->renderbuffer.renderbuffer;if(!r||r->deleted||!r->storage){c->error(invalidOperation);return {};}
    const auto actualFormat=format==0x84f9?0x88f0u:format;
    try{r->storage->allocate(actualFormat,width,height,samples);}catch(const std::bad_alloc&){c->requireOpen();c->error(outOfMemory);return {};}
    c->requireOpen();r->internalFormat=actualFormat;r->width=width;r->height=height;r->samples=samples;r->allocated=true;return {};
}
JSValue renderbufferParameter(const std::shared_ptr<State>& c,const Args& a){
    requireArgs(a,2);const auto target=unsignedInteger(a[0]),pname=unsignedInteger(a[1]);c->requireOpen();
    if(target!=renderbufferTarget){c->error(invalidEnum);return nullptr;}
    const auto r=c->renderbuffer.renderbuffer;if(!r||r->deleted||!r->storage){c->error(invalidOperation);return nullptr;}
    const auto bits=r->allocated?renderbufferFormat(r->internalFormat).value():FormatBits{};
    switch(pname){case 0x8d42:return r->width;case 0x8d43:return r->height;case 0x8d44:return r->internalFormat;case 0x8cab:return r->samples;
        case 0x8d50:return bits.red;case 0x8d51:return bits.green;case 0x8d52:return bits.blue;case 0x8d53:return bits.alpha;case 0x8d54:return bits.depth;case 0x8d55:return bits.stencil;
        default:c->error(invalidEnum);return nullptr;}
}
JSValue attachRenderbuffer(const std::shared_ptr<State>& c,const Args& a){
    requireArgs(a,4);const auto target=unsignedInteger(a[0]),attachment=unsignedInteger(a[1]),bufferTarget=unsignedInteger(a[2]);
    const auto r=renderbufferHandle(a[3],true);c->requireOpen();const auto f=boundFramebuffer(*c,target);if(!f)return {};
    if(!validAttachment(*c,attachment)||bufferTarget!=renderbufferTarget){c->error(invalidEnum);return {};}
    if(r&&(r->owner.lock()!=c||r->deleted||!r->storage||!r->bound)){c->error(invalidOperation);return {};}
    framebufferRevisionAvailable(*f);f->storage->renderbuffer(attachment,r?r->storage:nullptr);c->requireOpen();
    for(const auto name:attachmentNames(attachment)){
        clearAttachment(*f,name);attachmentRoot(*f,name,r?a[3]:JSValue(nullptr));
        if(r){++r->attachments;f->attachments.emplace(name,Attachment{JSWeakValue(a[3]),{},r,renderbufferTarget,0,-1});}
    }
    ++f->revision;return {};
}
JSValue drawBuffers(const std::shared_ptr<State>& c,const Args& a){
    requireArgs(a,1);auto buffers=enumSequence(a[0]);c->requireOpen();
    if(buffers.size()>std::size_t(c->limits.maxDrawBuffers)){c->error(invalidValue);return {};}
    for(const auto value:buffers)if(value!=0&&value!=backBuffer&&!(value>=colorAttachment0&&value-colorAttachment0<std::uint32_t(c->limits.maxColorAttachments))){c->error(invalidEnum);return {};}
    const auto f=c->draw.framebuffer;
    if(!f){
        if(buffers.size()!=1||(buffers[0]!=0&&buffers[0]!=backBuffer)){c->error(invalidOperation);return {};}
        if(c->defaultFramebufferRevision==std::numeric_limits<std::uint64_t>::max())throw JSRangeError("Vulkan framebuffer revision exhausted");
        c->defaultDrawBuffers=std::move(buffers);++c->defaultFramebufferRevision;
    }else{
        for(std::size_t i=0;i<buffers.size();++i)if(buffers[i]!=0&&buffers[i]!=colorAttachment0+i){c->error(invalidOperation);return {};}
        framebufferRevisionAvailable(*f);f->drawBuffers=std::move(buffers);++f->revision;
    }
    return {};
}
JSValue readBuffer(const std::shared_ptr<State>& c,const Args& a){
    requireArgs(a,1);const auto value=unsignedInteger(a[0]);c->requireOpen();
    if(value!=0&&value!=backBuffer&&!(value>=colorAttachment0&&value-colorAttachment0<std::uint32_t(c->limits.maxColorAttachments))){c->error(invalidEnum);return {};}
    const auto f=c->read.framebuffer;
    if(!f){
        if(value!=0&&value!=backBuffer){c->error(invalidOperation);return {};}
        if(c->defaultFramebufferRevision==std::numeric_limits<std::uint64_t>::max())throw JSRangeError("Vulkan framebuffer revision exhausted");
        c->defaultReadBuffer=value;++c->defaultFramebufferRevision;
    }else{
        if(value==backBuffer){c->error(invalidOperation);return {};}
        framebufferRevisionAvailable(*f);f->readBuffer=value;++f->revision;
    }
    return {};
}
JSValue attachmentParameter(const std::shared_ptr<State>& c,const Args& a){
    requireArgs(a,3);const auto target=unsignedInteger(a[0]),attachment=unsignedInteger(a[1]),pname=unsignedInteger(a[2]);c->requireOpen();
    if(!validFramebufferTarget(target)){c->error(invalidEnum);return nullptr;}
    const auto f=(target==readFramebuffer?c->read:c->draw).framebuffer;
    if(!f)throw JSTypeError("Default framebuffer attachment queries require the renderer surface boundary");
    if(!validAttachment(*c,attachment)){c->error(invalidEnum);return nullptr;}
    const auto it=f->attachments.find(attachment==depthStencilAttachment?depthAttachment:attachment);
    const Attachment* image=it==f->attachments.end()?nullptr:&it->second;
    if(attachment==depthStencilAttachment){
        const auto stencil=f->attachments.find(stencilAttachment);const auto other=stencil==f->attachments.end()?nullptr:&stencil->second;
        if(bool(image)!=bool(other)||(image&&(image->texture!=other->texture||image->renderbuffer!=other->renderbuffer||image->target!=other->target||image->level!=other->level||image->layer!=other->layer))){c->error(invalidOperation);return nullptr;}
    }
    if(pname==0x8cd0)return image?(image->texture?0x1702:renderbufferTarget):0;
    if(pname==0x8cd1)return image?weakObject(image->object):JSValue(nullptr);
    if(!image){c->error(invalidOperation);return nullptr;}
    if(pname==0x8cd2||pname==0x8cd3||pname==0x8cd4){
        if(!image->texture){c->error(invalidOperation);return nullptr;}
        return pname==0x8cd2?image->level:pname==0x8cd3?(image->target>=firstFace&&image->target<=lastFace?image->target:0):std::max(0,image->layer);
    }
    c->error(invalidEnum);return nullptr;
}
JSValue command(const JSValue&,const Args& a,const Args& captures){
    const auto c=state(captures[0]);c->requireOpen();const auto name=captures[1].string();
    if(name=="createTexture")return createTexture(c);
    if(name=="createFramebuffer")return createFramebuffer(c);
    if(name=="createRenderbuffer")return createRenderbuffer(c);
    if(name=="getError")return c->getError();
    if(name=="activeTexture"){requireArgs(a,1);const auto unit=unsignedInteger(a[0]);c->requireOpen();if(unit<texture0||unit-texture0>=std::uint32_t(c->limits.maxTextureUnits))c->error(invalidEnum);else c->activeUnit=unit-texture0;return {};}
    if(name=="bindTexture"){
        requireArgs(a,2);const auto target=unsignedInteger(a[0]);const auto t=texture(a[1],true);c->requireOpen();const auto i=bindingIndex(target);
        if(!i){c->error(invalidEnum);return {};}if(t&&(t->owner.lock()!=c||t->deleted||(t->target&&*t->target!=target))){c->error(invalidOperation);return {};}
        if(t)t->target=target;
        setBinding(captures[0],*c,*i,a[1],t);return {};
    }
    if(name=="deleteTexture"){
        requireArgs(a,1);const auto t=texture(a[0],true);if(!t)return {};if(t->owner.lock()!=c){c->error(invalidOperation);return {};}
        if(t->deleted)return {};if(c->nextGeneration==std::numeric_limits<std::uint64_t>::max())throw JSRangeError("Vulkan texture generation exhausted");
        t->deleted=true;t->generation=c->nextGeneration++;auto roots=captures[0].getInternalSlot("textureBindings");
        for(auto it=c->bindings.begin();it!=c->bindings.end();){
            for(std::size_t i=0;i<4;++i)if(it->second[i].texture==t){roots.erase(bindingKey(it->first,i));it->second[i]={};}
            if(emptyBindings(it->second))it=c->bindings.erase(it);else ++it;
        }
        // Deleting from the currently bound framebuffer detaches the attachment.
        // Other framebuffer attachments retain the GL object's backing storage.
        for(auto f:{c->draw.framebuffer,c->read.framebuffer})if(f){std::vector<std::uint32_t> names;for(const auto& [n,a]:f->attachments)if(a.texture==t)names.push_back(n);
            for(const auto n:names){const auto old=f->attachments.at(n);framebufferRevisionAvailable(*f);f->storage->texture(n,nullptr,old.target,old.level,old.layer);clearAttachment(*f,n);attachmentRoot(*f,n,nullptr);++f->revision;}}
        if(!t->attachments)t->storage.reset();return {};
    }
    if(name=="isTexture"){requireArgs(a,1);if(!a[0].isHostObject(textureTag))return false;const auto t=texture(a[0]);return t->owner.lock()==c&&!t->deleted&&bool(t->target);}
    if(name=="pixelStorei")return pixelStore(*c,a);
    if(name=="renderbufferStorage"||name=="renderbufferStorageMultisample")return allocateRenderbuffer(c,a,name=="renderbufferStorageMultisample");
    if(name=="getRenderbufferParameter")return renderbufferParameter(c,a);
    if(name=="framebufferRenderbuffer")return attachRenderbuffer(c,a);
    if(name=="drawBuffers")return drawBuffers(c,a);
    if(name=="readBuffer")return readBuffer(c,a);
    if(name=="getFramebufferAttachmentParameter")return attachmentParameter(c,a);
    if(name=="getParameter"){requireArgs(a,1);const auto pname=unsignedInteger(a[0]);c->requireOpen();return getParameter(*c,pname);}
    if(name=="texStorage2D"||name=="texStorage3D")return storageCommand(c,a,name=="texStorage3D");
    if(name=="texImage2D"||name=="texImage3D"||name=="texSubImage2D"||name=="texSubImage3D")return imageCommand(c,a,name.ends_with("3D"),name.starts_with("texSub"));
    if(name=="texParameteri"||name=="texParameterf"){
        requireArgs(a,3);const auto target=unsignedInteger(a[0]),pname=unsignedInteger(a[1]);const auto value=name=="texParameteri"?double(integer(a[2])):a[2].toNumber();c->requireOpen();
        if(!bindingIndex(target)){c->error(invalidEnum);return {};}const auto t=bound(*c,target);if(!t||!validParameter(*c,pname,value))return {};
        revisionAvailable(*t);t->storage->parameter(pname,value);c->requireOpen();t->parameters[pname]=value;++t->revision;return {};
    }
    if(name=="generateMipmap"){
        requireArgs(a,1);const auto target=unsignedInteger(a[0]);c->requireOpen();if(!bindingIndex(target)){c->error(invalidEnum);return {};}
        const auto t=bound(*c,target);if(!t)return {};if(t->levels.empty()){c->error(invalidOperation);return {};}
        revisionAvailable(*t);t->storage->generateMipmap(target);c->requireOpen();
        // gl.generateMipmap defines the derived mutable images. Subsequent
        // subimage commands must resolve those actual levels, including odd
        // base dimensions; immutable storage already defines its exact levels.
        if(!t->immutable){
            const auto baseParameter=t->parameters.find(0x813c),maxParameter=t->parameters.find(0x813d);
            const int base=baseParameter==t->parameters.end()?0:int(baseParameter->second);
            const int maximum=maxParameter==t->parameters.end()?1000:int(maxParameter->second);
            const auto fill=[&](std::uint32_t imageTarget){
                const auto it=t->levels.find({imageTarget,base});if(it==t->levels.end())return;
                const auto image=it->second;
                const int largest=std::max(image.width,std::max(image.height,target==texture3D?image.depth:1));
                if(largest<=0)return;
                const int last=std::min(maximum,base+int(std::floor(std::log2(double(largest)))));
                for(int level=base+1;level<=last;++level){const auto shift=level-base;
                    t->levels[{imageTarget,level}]={std::max(1,image.width>>shift),std::max(1,image.height>>shift),target==texture3D?std::max(1,image.depth>>shift):image.depth,image.internalFormat};
                }
            };
            if(target==textureCube)for(auto face=firstFace;face<=lastFace;++face)fill(face);else fill(target);
        }
        ++t->revision;return {};
    }
    if(name=="bindFramebuffer"){
        requireArgs(a,2);const auto target=unsignedInteger(a[0]);const auto f=framebufferHandle(a[1],true);c->requireOpen();
        if(target!=framebuffer&&target!=drawFramebuffer&&target!=readFramebuffer){c->error(invalidEnum);return {};}
        if(f&&(f->owner.lock()!=c||f->deleted)){c->error(invalidOperation);return {};}
        if(f)f->bound=true;
        const auto value=f?a[1]:JSValue(nullptr);const FramebufferBinding b{JSWeakValue(value),f};
        if(target!=readFramebuffer){slot(captures[0],"drawFramebuffer",value);c->draw=b;}
        if(target!=drawFramebuffer){slot(captures[0],"readFramebuffer",value);c->read=b;}return {};
    }
    if(name=="deleteFramebuffer"){
        requireArgs(a,1);const auto f=framebufferHandle(a[0],true);if(!f)return {};if(f->owner.lock()!=c){c->error(invalidOperation);return {};}
        if(f->deleted)return {};f->deleted=true;f->clearAttachments();f->storage.reset();
        slot(a[0],"textureAttachments",nullptr);
        if(c->draw.framebuffer==f){slot(captures[0],"drawFramebuffer",nullptr);c->draw={};}
        if(c->read.framebuffer==f){slot(captures[0],"readFramebuffer",nullptr);c->read={};}return {};
    }
    if(name=="isFramebuffer"){requireArgs(a,1);if(!a[0].isHostObject(framebufferTag))return false;const auto f=framebufferHandle(a[0]);return f->owner.lock()==c&&!f->deleted&&f->bound;}
    if(name=="bindRenderbuffer"){
        requireArgs(a,2);const auto target=unsignedInteger(a[0]);const auto r=renderbufferHandle(a[1],true);c->requireOpen();
        if(target!=renderbufferTarget){c->error(invalidEnum);return {};}
        if(r&&(r->owner.lock()!=c||r->deleted)){c->error(invalidOperation);return {};}
        if(r)r->bound=true;const auto value=r?a[1]:JSValue(nullptr);
        slot(captures[0],"renderbuffer",value);c->renderbuffer={JSWeakValue(value),r};return {};
    }
    if(name=="deleteRenderbuffer"){
        requireArgs(a,1);const auto r=renderbufferHandle(a[0],true);if(!r)return {};if(r->owner.lock()!=c){c->error(invalidOperation);return {};}
        if(r->deleted)return {};r->deleted=true;
        if(c->renderbuffer.renderbuffer==r){slot(captures[0],"renderbuffer",nullptr);c->renderbuffer={};}
        for(auto f:{c->draw.framebuffer,c->read.framebuffer})if(f){
            std::vector<std::uint32_t> names;for(const auto& [name,attachment]:f->attachments)if(attachment.renderbuffer==r)names.push_back(name);
            for(const auto name:names){framebufferRevisionAvailable(*f);f->storage->renderbuffer(name,nullptr);clearAttachment(*f,name);attachmentRoot(*f,name,nullptr);++f->revision;}
        }
        if(!r->attachments)r->storage.reset();return {};
    }
    if(name=="isRenderbuffer"){requireArgs(a,1);if(!a[0].isHostObject(renderbufferTag))return false;const auto r=renderbufferHandle(a[0]);return r->owner.lock()==c&&!r->deleted&&r->bound;}
    if(name=="framebufferTexture2D"||name=="framebufferTextureLayer")return attachTexture(c,a,name=="framebufferTextureLayer");
    if(name=="checkFramebufferStatus"){
        requireArgs(a,1);const auto target=unsignedInteger(a[0]);c->requireOpen();
        if(target!=framebuffer&&target!=drawFramebuffer&&target!=readFramebuffer){c->error(invalidEnum);return 0;}
        const auto f=(target==readFramebuffer?c->read:c->draw).framebuffer;
        if(!f)throw JSTypeError("Default framebuffer status requires the renderer surface boundary");return f->storage->status(target);
    }
    throw JSTypeError("Vulkan texture command is not implemented by this GPU boundary: "+name);
}
void defineEnums(JSValue object);
std::shared_ptr<Texture> readable(const JSValue& stateValue,const JSValue& value){
    const auto c=state(stateValue);c->requireOpen();const auto t=texture(value);
    if(t->owner.lock()!=c||t->deleted||!t->storage)throw JSTypeError("Vulkan texture is foreign, deleted, or unavailable");return t;
}
}
VulkanTextureCommands::VulkanTextureCommands(Factory factory,FramebufferFactory framebuffers)
    :VulkanTextureCommands(std::move(factory),std::move(framebuffers),Limits{}){}
VulkanTextureCommands::VulkanTextureCommands(Factory factory,FramebufferFactory framebuffers,Limits limits)
    :VulkanTextureCommands(std::move(factory),std::move(framebuffers),{},limits){}
VulkanTextureCommands::VulkanTextureCommands(Factory factory,FramebufferFactory framebuffers,RenderbufferFactory renderbuffers,Limits limits):object_(JSValue::object()){
    if(!factory)throw std::invalid_argument("Vulkan texture storage factory is required");
    if(limits.maxTextureUnits<=0||limits.maxTextureSize<=0||limits.maxCubeMapSize<=0||limits.max3DTextureSize<=0||limits.maxArrayLayers<=0||limits.maxSamples<0||limits.maxRenderbufferSize<=0||limits.maxColorAttachments<=0||limits.maxColorAttachments>32||limits.maxDrawBuffers<=0||limits.maxDrawBuffers>limits.maxColorAttachments)throw std::invalid_argument("Invalid Vulkan texture limits");
    auto c=std::make_shared<State>(std::move(factory),std::move(framebuffers),std::move(renderbuffers),limits);state_=JSValue::hostObject(c,c.get(),stateTag);
    state_.setInternalSlot("textureBindings",JSValue::object(nullptr));state_.setInternalSlot("drawFramebuffer",nullptr);state_.setInternalSlot("readFramebuffer",nullptr);
    state_.setInternalSlot("renderbuffer",nullptr);
    static const auto hooks=[]{auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=+[](const JSValue& v){
        const auto c=state(v);JSNativeOwnerNode n{c,{},{}};std::vector<JSNativeOwnerNode> nodes;
        for(const auto& [_,unit]:c->bindings)for(const auto& b:unit)if(b.texture){n.edges.push_back({&b.texture,b.texture});appendTexture(nodes,b.texture);}
        for(const auto* b:{&c->draw,&c->read})if(b->framebuffer){n.edges.push_back({&b->framebuffer,b->framebuffer});appendFramebuffer(nodes,b->framebuffer);}
        if(c->renderbuffer.renderbuffer){n.edges.push_back({&c->renderbuffer.renderbuffer,c->renderbuffer.renderbuffer});appendRenderbuffer(nodes,c->renderbuffer.renderbuffer);}
        nodes.push_back(std::move(n));return nodes;};return h;}();state_.setHostHooks(hooks);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
        {"createTexture",0},{"bindTexture",2},{"deleteTexture",1},{"isTexture",1},{"activeTexture",1},{"pixelStorei",2},{"getParameter",1},{"getError",0},
        {"texStorage2D",5},{"texStorage3D",6},{"texImage2D",9},{"texImage3D",10},{"texSubImage2D",9},{"texSubImage3D",11},
        {"texParameteri",3},{"texParameterf",3},{"generateMipmap",1},
        {"createFramebuffer",0},{"bindFramebuffer",2},{"deleteFramebuffer",1},{"isFramebuffer",1},{"framebufferTexture2D",5},{"framebufferTextureLayer",5},{"checkFramebufferStatus",1},
        {"createRenderbuffer",0},{"bindRenderbuffer",2},{"deleteRenderbuffer",1},{"isRenderbuffer",1},{"renderbufferStorage",4},{"renderbufferStorageMultisample",5},{"framebufferRenderbuffer",4},{"getRenderbufferParameter",2},{"getFramebufferAttachmentParameter",3},
        {"compressedTexImage2D",7},{"compressedTexImage3D",8},{"compressedTexSubImage2D",8},{"compressedTexSubImage3D",10},
        {"copyTexSubImage2D",8},{"copyTexSubImage3D",9},{"copyTexImage2D",8},{"blitFramebuffer",10},{"invalidateFramebuffer",2},{"drawBuffers",1},{"readBuffer",1},{"readPixels",7}})
        property(object_,name,JSValue::function(command,{state_,name},name,arity));
    defineEnums(object_);
}
JSValue VulkanTextureCommands::object()const{static_cast<void>(JSWeakValue(object_));return object_;}
JSValue VulkanTextureCommands::boundTexture(std::uint32_t unit,std::uint32_t target)const{
    const auto c=state(state_);c->requireOpen();const auto i=bindingIndex(target);
    if(!i||unit>=std::uint32_t(c->limits.maxTextureUnits))throw JSRangeError("Invalid Vulkan sampler binding");return bindingObject(*c,unit,*i);
}
std::shared_ptr<VulkanTextureCommands::Storage> VulkanTextureCommands::storage(const JSValue& value)const{return readable(state_,value)->storage;}
std::uint64_t VulkanTextureCommands::revision(const JSValue& value)const{return readable(state_,value)->revision;}
std::uint64_t VulkanTextureCommands::generation(const JSValue& value)const{const auto c=state(state_);c->requireOpen();const auto t=texture(value);if(t->owner.lock()!=c)throw JSTypeError("Foreign Vulkan texture handle");return t->generation;}
bool VulkanTextureCommands::isDeleted(const JSValue& value)const{const auto c=state(state_);c->requireOpen();const auto t=texture(value);if(t->owner.lock()!=c)throw JSTypeError("Foreign Vulkan texture handle");return t->deleted;}
JSValue VulkanTextureCommands::boundFramebuffer(std::uint32_t target)const{
    const auto c=state(state_);c->requireOpen();if(!validFramebufferTarget(target))throw JSRangeError("Invalid Vulkan framebuffer binding target");
    return weakObject((target==readFramebuffer?c->read:c->draw).object);
}
std::shared_ptr<VulkanTextureCommands::FramebufferStorage> VulkanTextureCommands::framebufferStorage(const JSValue& value)const{
    const auto c=state(state_);c->requireOpen();const auto f=framebufferHandle(value,true);if(!f)return {};
    if(f->owner.lock()!=c||f->deleted||!f->storage)throw JSTypeError("Vulkan framebuffer is foreign, deleted, or unavailable");return f->storage;
}
std::vector<std::uint32_t> VulkanTextureCommands::framebufferDrawBuffers(const JSValue& value)const{
    const auto c=state(state_);c->requireOpen();const auto f=framebufferHandle(value,true);if(!f)return c->defaultDrawBuffers;
    if(f->owner.lock()!=c||f->deleted||!f->storage)throw JSTypeError("Vulkan framebuffer is foreign, deleted, or unavailable");return f->drawBuffers;
}
std::uint32_t VulkanTextureCommands::framebufferReadBuffer(const JSValue& value)const{
    const auto c=state(state_);c->requireOpen();const auto f=framebufferHandle(value,true);if(!f)return c->defaultReadBuffer;
    if(f->owner.lock()!=c||f->deleted||!f->storage)throw JSTypeError("Vulkan framebuffer is foreign, deleted, or unavailable");return f->readBuffer;
}
std::uint64_t VulkanTextureCommands::framebufferRevision(const JSValue& value)const{
    const auto c=state(state_);c->requireOpen();const auto f=framebufferHandle(value,true);if(!f)return c->defaultFramebufferRevision;
    if(f->owner.lock()!=c||f->deleted||!f->storage)throw JSTypeError("Vulkan framebuffer is foreign, deleted, or unavailable");return f->revision;
}
JSValue VulkanTextureCommands::boundRenderbuffer()const{const auto c=state(state_);c->requireOpen();return weakObject(c->renderbuffer.object);}
std::shared_ptr<VulkanTextureCommands::RenderbufferStorage> VulkanTextureCommands::renderbufferStorage(const JSValue& value)const{
    const auto c=state(state_);c->requireOpen();const auto r=renderbufferHandle(value);
    if(r->owner.lock()!=c||r->deleted||!r->storage)throw JSTypeError("Vulkan renderbuffer is foreign, deleted, or unavailable");return r->storage;
}
void VulkanTextureCommands::shutdown(){
    const auto c=state(state_);
    for(const auto& weak:c->framebuffers)if(const auto f=weak.lock())if(const auto wrapper=f->object.lock())slot(*wrapper,"textureAttachments",nullptr);
    c->close();state_.setInternalSlot("textureBindings",nullptr);state_.setInternalSlot("drawFramebuffer",nullptr);state_.setInternalSlot("readFramebuffer",nullptr);state_.setInternalSlot("renderbuffer",nullptr);
}

namespace {
// Numeric tokens from Khronos OpenGL-Registry gl.xml; WebGL pixel-store
// tokens follow the WebGL specification. No synthetic parity values are used.
void defineEnums(JSValue object){
    for(const auto& [name,value]:std::initializer_list<std::pair<const char*,std::uint32_t>>{
        {"ACTIVE_TEXTURE",0x84E0},
        {"ALPHA",0x1906},
        {"ALWAYS",0x0207},
        {"BACK",0x0405},
        {"BLEND",0x0BE2},
        {"BROWSER_DEFAULT_WEBGL",0x9244},
        {"BYTE",0x1400},
        {"CCW",0x0901},
        {"CLAMP_TO_EDGE",0x812F},
        {"COLOR_ATTACHMENT0",0x8CE0},
        {"COLOR_BUFFER_BIT",0x00004000},
        {"COMPARE_REF_TO_TEXTURE",0x884E},
        {"CONSTANT_ALPHA",0x8003},
        {"CONSTANT_COLOR",0x8001},
        {"CULL_FACE",0x0B44},
        {"CW",0x0900},
        {"DEPTH24_STENCIL8",0x88F0},
        {"DEPTH32F_STENCIL8",0x8CAD},
        {"DEPTH_ATTACHMENT",0x8D00},
        {"DEPTH_BUFFER_BIT",0x00000100},
        {"DEPTH_COMPONENT",0x1902},
        {"DEPTH_COMPONENT16",0x81A5},
        {"DEPTH_COMPONENT24",0x81A6},
        {"DEPTH_COMPONENT32F",0x8CAC},
        {"DEPTH_STENCIL",0x84F9},
        {"DEPTH_STENCIL_ATTACHMENT",0x821A},
        {"DEPTH_TEST",0x0B71},
        {"DRAW_FRAMEBUFFER",0x8CA9},
        {"DRAW_FRAMEBUFFER_BINDING",0x8CA6},
        {"DRAW_BUFFER0",0x8825},
        {"DST_ALPHA",0x0304},
        {"DST_COLOR",0x0306},
        {"EQUAL",0x0202},
        {"FLOAT",0x1406},
        {"FRAMEBUFFER",0x8D40},
        {"FRAMEBUFFER_BINDING",0x8CA6},
        {"FRAMEBUFFER_COMPLETE",0x8CD5},
        {"FRAMEBUFFER_INCOMPLETE_ATTACHMENT",0x8CD6},
        {"FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT",0x8CD7},
        {"FRAMEBUFFER_UNSUPPORTED",0x8CDD},
        {"FRAMEBUFFER_INCOMPLETE_MULTISAMPLE",0x8D56},
        {"FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE",0x8CD0},
        {"FRAMEBUFFER_ATTACHMENT_OBJECT_NAME",0x8CD1},
        {"FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL",0x8CD2},
        {"FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE",0x8CD3},
        {"FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER",0x8CD4},
        {"FRONT",0x0404},
        {"FRONT_AND_BACK",0x0408},
        {"FUNC_ADD",0x8006},
        {"FUNC_REVERSE_SUBTRACT",0x800B},
        {"FUNC_SUBTRACT",0x800A},
        {"GEQUAL",0x0206},
        {"GREATER",0x0204},
        {"HALF_FLOAT",0x140B},
        {"INT",0x1404},
        {"INVALID_ENUM",0x0500},
        {"INVALID_OPERATION",0x0502},
        {"INVALID_VALUE",0x0501},
        {"KEEP",0x1E00},
        {"LEQUAL",0x0203},
        {"LESS",0x0201},
        {"LINEAR",0x2601},
        {"LINEAR_MIPMAP_LINEAR",0x2703},
        {"LINEAR_MIPMAP_NEAREST",0x2701},
        {"MAX",0x8008},
        {"MAX_3D_TEXTURE_SIZE",0x8073},
        {"MAX_ARRAY_TEXTURE_LAYERS",0x88FF},
        {"MAX_COMBINED_TEXTURE_IMAGE_UNITS",0x8B4D},
        {"MAX_COLOR_ATTACHMENTS",0x8CDF},
        {"MAX_DRAW_BUFFERS",0x8824},
        {"MAX_RENDERBUFFER_SIZE",0x84E8},
        {"MAX_CUBE_MAP_TEXTURE_SIZE",0x851C},
        {"MAX_SAMPLES",0x8D57},
        {"MAX_TEXTURE_IMAGE_UNITS",0x8872},
        {"MAX_TEXTURE_SIZE",0x0D33},
        {"MIN",0x8007},
        {"MIRRORED_REPEAT",0x8370},
        {"NEAREST",0x2600},
        {"NEAREST_MIPMAP_LINEAR",0x2702},
        {"NEAREST_MIPMAP_NEAREST",0x2700},
        {"NEVER",0x0200},
        {"NONE",0},
        {"NOTEQUAL",0x0205},
        {"NO_ERROR",0},
        {"ONE",1},
        {"ONE_MINUS_CONSTANT_ALPHA",0x8004},
        {"ONE_MINUS_CONSTANT_COLOR",0x8002},
        {"ONE_MINUS_DST_ALPHA",0x0305},
        {"ONE_MINUS_DST_COLOR",0x0307},
        {"ONE_MINUS_SRC_ALPHA",0x0303},
        {"ONE_MINUS_SRC_COLOR",0x0301},
        {"OUT_OF_MEMORY",0x0505},
        {"PACK_ALIGNMENT",0x0D05},
        {"PACK_ROW_LENGTH",0x0D02},
        {"PACK_SKIP_PIXELS",0x0D04},
        {"PACK_SKIP_ROWS",0x0D03},
        {"POLYGON_OFFSET_FILL",0x8037},
        {"R11F_G11F_B10F",0x8C3A},
        {"R16F",0x822D},
        {"R16I",0x8233},
        {"R16UI",0x8234},
        {"R32F",0x822E},
        {"R32I",0x8235},
        {"R32UI",0x8236},
        {"R8",0x8229},
        {"R8I",0x8231},
        {"R8UI",0x8232},
        {"READ_FRAMEBUFFER",0x8CA8},
        {"READ_FRAMEBUFFER_BINDING",0x8CAA},
        {"READ_BUFFER",0x0C02},
        {"RED",0x1903},
        {"RED_INTEGER",0x8D94},
        {"RENDERBUFFER",0x8D41},
        {"RENDERBUFFER_BINDING",0x8CA7},
        {"RENDERBUFFER_WIDTH",0x8D42},
        {"RENDERBUFFER_HEIGHT",0x8D43},
        {"RENDERBUFFER_INTERNAL_FORMAT",0x8D44},
        {"RENDERBUFFER_RED_SIZE",0x8D50},
        {"RENDERBUFFER_GREEN_SIZE",0x8D51},
        {"RENDERBUFFER_BLUE_SIZE",0x8D52},
        {"RENDERBUFFER_ALPHA_SIZE",0x8D53},
        {"RENDERBUFFER_DEPTH_SIZE",0x8D54},
        {"RENDERBUFFER_STENCIL_SIZE",0x8D55},
        {"RENDERBUFFER_SAMPLES",0x8CAB},
        {"REPEAT",0x2901},
        {"RG",0x8227},
        {"RG16F",0x822F},
        {"RG16I",0x8239},
        {"RG16UI",0x823A},
        {"RG32F",0x8230},
        {"RG32I",0x823B},
        {"RG32UI",0x823C},
        {"RG8",0x822B},
        {"RG8I",0x8237},
        {"RG8UI",0x8238},
        {"RGB",0x1907},
        {"RGB16I",0x8D89},
        {"RGB16UI",0x8D77},
        {"RGB32I",0x8D83},
        {"RGB32UI",0x8D71},
        {"RGB5_A1",0x8057},
        {"RGB8I",0x8D8F},
        {"RGB8UI",0x8D7D},
        {"RGB9_E5",0x8C3D},
        {"RGBA",0x1908},
        {"RGBA16F",0x881A},
        {"RGBA16I",0x8D88},
        {"RGBA16UI",0x8D76},
        {"RGBA32F",0x8814},
        {"RGBA32I",0x8D82},
        {"RGBA32UI",0x8D70},
        {"RGBA4",0x8056},
        {"RGBA8",0x8058},
        {"RGBA8I",0x8D8E},
        {"RGBA8UI",0x8D7C},
        {"RGBA_INTEGER",0x8D99},
        {"RGB_INTEGER",0x8D98},
        {"RG_INTEGER",0x8228},
        {"SAMPLE_ALPHA_TO_COVERAGE",0x809E},
        {"SCISSOR_BOX",0x0C10},
        {"SCISSOR_TEST",0x0C11},
        {"SHORT",0x1402},
        {"SRC_ALPHA",0x0302},
        {"SRC_ALPHA_SATURATE",0x0308},
        {"SRC_COLOR",0x0300},
        {"SRGB8_ALPHA8",0x8C43},
        {"STENCIL_BUFFER_BIT",0x00000400},
        {"STENCIL_ATTACHMENT",0x8D20},
        {"STENCIL_INDEX8",0x8D48},
        {"STENCIL_TEST",0x0B90},
        {"TEXTURE0",0x84C0},
        {"TEXTURE_2D",0x0DE1},
        {"TEXTURE_2D_ARRAY",0x8C1A},
        {"TEXTURE_3D",0x806F},
        {"TEXTURE_BINDING_2D",0x8069},
        {"TEXTURE_BINDING_2D_ARRAY",0x8C1D},
        {"TEXTURE_BINDING_3D",0x806A},
        {"TEXTURE_BINDING_CUBE_MAP",0x8514},
        {"TEXTURE_COMPARE_FUNC",0x884D},
        {"TEXTURE_COMPARE_MODE",0x884C},
        {"TEXTURE_CUBE_MAP",0x8513},
        {"TEXTURE_CUBE_MAP_NEGATIVE_Z",0x851A},
        {"TEXTURE_CUBE_MAP_POSITIVE_X",0x8515},
        {"TEXTURE_MAG_FILTER",0x2800},
        {"TEXTURE_MAX_LEVEL",0x813D},
        {"TEXTURE_MIN_FILTER",0x2801},
        {"TEXTURE_WRAP_R",0x8072},
        {"TEXTURE_WRAP_S",0x2802},
        {"TEXTURE_WRAP_T",0x2803},
        {"UNIFORM_BUFFER",0x8A11},
        {"UNPACK_ALIGNMENT",0x0CF5},
        {"UNPACK_COLORSPACE_CONVERSION_WEBGL",0x9243},
        {"UNPACK_FLIP_Y_WEBGL",0x9240},
        {"UNPACK_IMAGE_HEIGHT",0x806E},
        {"UNPACK_PREMULTIPLY_ALPHA_WEBGL",0x9241},
        {"UNPACK_ROW_LENGTH",0x0CF2},
        {"UNPACK_SKIP_IMAGES",0x806D},
        {"UNPACK_SKIP_PIXELS",0x0CF4},
        {"UNPACK_SKIP_ROWS",0x0CF3},
        {"UNSIGNED_BYTE",0x1401},
        {"UNSIGNED_INT",0x1405},
        {"UNSIGNED_INT_10F_11F_11F_REV",0x8C3B},
        {"UNSIGNED_INT_24_8",0x84FA},
        {"UNSIGNED_INT_5_9_9_9_REV",0x8C3E},
        {"UNSIGNED_SHORT",0x1403},
        {"UNSIGNED_SHORT_4_4_4_4",0x8033},
        {"UNSIGNED_SHORT_5_5_5_1",0x8034},
        {"VERSION",0x1F02},
        {"VIEWPORT",0x0BA2},
        {"ZERO",0},
    })property(object,name,value);
}
}
