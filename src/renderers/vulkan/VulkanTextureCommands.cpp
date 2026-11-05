#include "renderers/vulkan/VulkanTextureCommands.h"
#include "core/NativeArray.h"
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
using PixelStore=VulkanTextureCommands::PixelStore;
constexpr const char* stateTag="VulkanTextureCommands.State";
constexpr const char* textureTag="VulkanTextureCommands.Texture";
constexpr const char* framebufferTag="VulkanTextureCommands.Framebuffer";
constexpr std::uint32_t texture2D=0x0de1,textureCube=0x8513,texture3D=0x806f,textureArray=0x8c1a;
constexpr std::uint32_t firstFace=0x8515,lastFace=0x851a,texture0=0x84c0;
constexpr std::uint32_t invalidEnum=0x0500,invalidValue=0x0501,invalidOperation=0x0502,outOfMemory=0x0505;
constexpr std::uint32_t framebuffer=0x8d40,drawFramebuffer=0x8ca9,readFramebuffer=0x8ca8;
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
struct Attachment {
    JSWeakValue object;
    std::shared_ptr<Texture> texture;
    std::uint32_t target=0;int level=0,layer=-1;
};
struct Framebuffer {
    std::weak_ptr<State> owner;
    JSWeakValue object;
    std::shared_ptr<FramebufferStorage> storage;
    std::map<std::uint32_t,Attachment> attachments;
    bool deleted=false;
    void clearAttachments() {
        for(auto& [_,a]:attachments) if(a.texture) {
            if(--a.texture->attachments==0 && a.texture->deleted)a.texture->storage.reset();
        }
        attachments.clear();
    }
    ~Framebuffer(){clearAttachments();}
};
struct FramebufferBinding {JSWeakValue object;std::shared_ptr<Framebuffer> framebuffer;};
struct State {
    VulkanTextureCommands::Factory factory;
    VulkanTextureCommands::FramebufferFactory framebufferFactory;
    VulkanTextureCommands::Limits limits;
    // Descriptor limits can exceed a million units. Only bound units belong in
    // the ownership graph; querying or unbinding an empty unit allocates none.
    std::map<std::uint32_t,std::array<TextureBinding,4>> bindings;
    std::vector<std::weak_ptr<Texture>> textures;
    std::vector<std::weak_ptr<Framebuffer>> framebuffers;
    FramebufferBinding draw,read;
    PixelStore unpack;
    int packAlignment=4;
    std::uint32_t activeUnit=0;
    std::uint64_t nextGeneration=1;
    std::array<bool,6> errors{};
    bool closed=false;
    State(VulkanTextureCommands::Factory f,VulkanTextureCommands::FramebufferFactory fb,VulkanTextureCommands::Limits l)
        :factory(std::move(f)),framebufferFactory(std::move(fb)),limits(l){}
    void requireOpen()const {if(closed)throw JSTypeError("Vulkan texture commands are shut down");}
    void error(std::uint32_t value){errors[value-invalidEnum]=true;}
    std::uint32_t getError(){for(std::size_t i=0;i<errors.size();++i)if(errors[i]){errors[i]=false;return invalidEnum+std::uint32_t(i);}return 0;}
    void close(){
        if(closed)return;closed=true;
        for(const auto& weak:framebuffers)if(auto f=weak.lock()){f->deleted=true;f->clearAttachments();f->storage.reset();}
        for(const auto& weak:textures)if(auto t=weak.lock()){t->deleted=true;t->storage.reset();}
        bindings.clear();draw={};read={};textures.clear();framebuffers.clear();factory={};framebufferFactory={};
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
void appendFramebuffer(std::vector<JSNativeOwnerNode>& nodes,const std::shared_ptr<Framebuffer>& f){
    JSNativeOwnerNode n{f,{},{}};if(f->storage){n.edges.push_back({&f->storage,f->storage});nodes.push_back({f->storage,{},{}});}
    for(const auto& [_,a]:f->attachments)if(a.texture){n.edges.push_back({&a.texture,a.texture});appendTexture(nodes,a.texture);}
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
JSValue getParameter(State& c,std::uint32_t name){
    switch(name){case 0x84e0:return texture0+c.activeUnit;case 0x0cf5:return c.unpack.alignment;case 0x0d05:return c.packAlignment;
        case 0x0cf2:return c.unpack.rowLength;case 0x806e:return c.unpack.imageHeight;case 0x0cf4:return c.unpack.skipPixels;case 0x0cf3:return c.unpack.skipRows;case 0x806d:return c.unpack.skipImages;
        case 0x9240:return c.unpack.flipY;case 0x9241:return c.unpack.premultiplyAlpha;case 0x9243:return c.unpack.colorSpaceConversion;
        case 0x0d33:return c.limits.maxTextureSize;case 0x851c:return c.limits.maxCubeMapSize;case 0x8073:return c.limits.max3DTextureSize;case 0x88ff:return c.limits.maxArrayLayers;
        case 0x8872:case 0x8b4d:return c.limits.maxTextureUnits;case 0x8d57:return c.limits.maxSamples;
        case 0x8069:return bindingObject(c,c.activeUnit,0);case 0x8514:return bindingObject(c,c.activeUnit,1);case 0x806a:return bindingObject(c,c.activeUnit,2);case 0x8c1d:return bindingObject(c,c.activeUnit,3);
        case 0x8ca6:return weakObject(c.draw.object);case 0x8caa:return weakObject(c.read.object);default:c.error(invalidEnum);return nullptr;
    }
}
std::shared_ptr<Framebuffer> boundFramebuffer(State& c,std::uint32_t target){
    if(target!=framebuffer&&target!=drawFramebuffer&&target!=readFramebuffer){c.error(invalidEnum);return {};}
    const auto f=(target==readFramebuffer?c.read:c.draw).framebuffer;if(!f||f->deleted||!f->storage){c.error(invalidOperation);return {};}return f;
}
bool validAttachment(std::uint32_t name){return (name>=0x8ce0&&name<0x8d00)||name==0x8d00||name==0x8d20||name==0x821a;}
JSValue attachTexture(const std::shared_ptr<State>& c,const Args& a,bool layered){
    requireArgs(a,5);const auto target=unsignedInteger(a[0]),attachment=unsignedInteger(a[1]);
    const auto imageTarget=layered?0u:unsignedInteger(a[2]);const auto& object=a[layered?2:3];const auto t=texture(object,true);
    const int level=integer(a[layered?3:4]),layer=layered?integer(a[4]):-1;c->requireOpen();
    const auto f=boundFramebuffer(*c,target);if(!f)return {};
    if(!validAttachment(attachment)){c->error(invalidEnum);return {};}
    if(level<0||(layered&&layer<0)){c->error(invalidValue);return {};}
    if(t&&(t->owner.lock()!=c||t->deleted||!t->storage)){c->error(invalidOperation);return {};}
    if(t){
        if(!t->target){c->error(invalidOperation);return {};}
        if(layered&&*t->target!=texture3D&&*t->target!=textureArray){c->error(invalidOperation);return {};}
        if(!layered&&(!isImageTarget(imageTarget,false)||bindingTarget(imageTarget)!=*t->target)){c->error(invalidOperation);return {};}
    }
    const auto actualTarget=t?(layered?*t->target:imageTarget):imageTarget;
    f->storage->texture(attachment,t?t->storage:nullptr,actualTarget,level,layer);c->requireOpen();
    if(const auto it=f->attachments.find(attachment);it!=f->attachments.end()){
        auto old=it->second.texture;if(old&&--old->attachments==0&&old->deleted)old->storage.reset();f->attachments.erase(it);
    }
    if(const auto wrapper=f->object.lock())wrapper->getInternalSlot("textureAttachments").set(std::to_string(attachment),t?object:JSValue(nullptr));
    if(t){++t->attachments;f->attachments.emplace(attachment,Attachment{JSWeakValue(object),t,actualTarget,level,layer});}return {};
}
JSValue command(const JSValue&,const Args& a,const Args& captures){
    const auto c=state(captures[0]);c->requireOpen();const auto name=captures[1].string();
    if(name=="createTexture")return createTexture(c);
    if(name=="createFramebuffer")return createFramebuffer(c);
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
            for(const auto n:names){const auto old=f->attachments.at(n);f->storage->texture(n,nullptr,old.target,old.level,old.layer);f->attachments.erase(n);--t->attachments;if(const auto wrapper=f->object.lock())wrapper->getInternalSlot("textureAttachments").set(std::to_string(n),nullptr);}}
        if(!t->attachments)t->storage.reset();return {};
    }
    if(name=="isTexture"){requireArgs(a,1);if(!a[0].isHostObject(textureTag))return false;const auto t=texture(a[0]);return t->owner.lock()==c&&!t->deleted&&bool(t->target);}
    if(name=="pixelStorei")return pixelStore(*c,a);
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
    if(name=="isFramebuffer"){requireArgs(a,1);if(!a[0].isHostObject(framebufferTag))return false;const auto f=framebufferHandle(a[0]);return f->owner.lock()==c&&!f->deleted;}
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
VulkanTextureCommands::VulkanTextureCommands(Factory factory,FramebufferFactory framebuffers,Limits limits):object_(JSValue::object()){
    if(!factory)throw std::invalid_argument("Vulkan texture storage factory is required");
    if(limits.maxTextureUnits<=0||limits.maxTextureSize<=0||limits.maxCubeMapSize<=0||limits.max3DTextureSize<=0||limits.maxArrayLayers<=0||limits.maxSamples<0)throw std::invalid_argument("Invalid Vulkan texture limits");
    auto c=std::make_shared<State>(std::move(factory),std::move(framebuffers),limits);state_=JSValue::hostObject(c,c.get(),stateTag);
    state_.setInternalSlot("textureBindings",JSValue::object(nullptr));state_.setInternalSlot("drawFramebuffer",nullptr);state_.setInternalSlot("readFramebuffer",nullptr);
    static const auto hooks=[]{auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=+[](const JSValue& v){
        const auto c=state(v);JSNativeOwnerNode n{c,{},{}};std::vector<JSNativeOwnerNode> nodes;
        for(const auto& [_,unit]:c->bindings)for(const auto& b:unit)if(b.texture){n.edges.push_back({&b.texture,b.texture});appendTexture(nodes,b.texture);}
        for(const auto* b:{&c->draw,&c->read})if(b->framebuffer){n.edges.push_back({&b->framebuffer,b->framebuffer});appendFramebuffer(nodes,b->framebuffer);}
        nodes.push_back(std::move(n));return nodes;};return h;}();state_.setHostHooks(hooks);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
        {"createTexture",0},{"bindTexture",2},{"deleteTexture",1},{"isTexture",1},{"activeTexture",1},{"pixelStorei",2},{"getParameter",1},{"getError",0},
        {"texStorage2D",5},{"texStorage3D",6},{"texImage2D",9},{"texImage3D",10},{"texSubImage2D",9},{"texSubImage3D",11},
        {"texParameteri",3},{"texParameterf",3},{"generateMipmap",1},
        {"createFramebuffer",0},{"bindFramebuffer",2},{"deleteFramebuffer",1},{"isFramebuffer",1},{"framebufferTexture2D",5},{"framebufferTextureLayer",5},{"checkFramebufferStatus",1},
        {"createRenderbuffer",0},{"bindRenderbuffer",2},{"deleteRenderbuffer",1},{"renderbufferStorage",4},{"renderbufferStorageMultisample",5},{"framebufferRenderbuffer",4},
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
void VulkanTextureCommands::shutdown(){
    const auto c=state(state_);
    for(const auto& weak:c->framebuffers)if(const auto f=weak.lock())if(const auto wrapper=f->object.lock())slot(*wrapper,"textureAttachments",nullptr);
    c->close();state_.setInternalSlot("textureBindings",nullptr);state_.setInternalSlot("drawFramebuffer",nullptr);state_.setInternalSlot("readFramebuffer",nullptr);
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
        {"DST_ALPHA",0x0304},
        {"DST_COLOR",0x0306},
        {"EQUAL",0x0202},
        {"FLOAT",0x1406},
        {"FRAMEBUFFER",0x8D40},
        {"FRAMEBUFFER_BINDING",0x8CA6},
        {"FRAMEBUFFER_COMPLETE",0x8CD5},
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
        {"RED",0x1903},
        {"RED_INTEGER",0x8D94},
        {"RENDERBUFFER",0x8D41},
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
