#include "catch.hpp"
#include "renderers/vulkan/VulkanTextureCommands.h"
#include "renderers/webgl/WebGLTextures.h"
#include "renderers/webgl/WebGLState.h"
#include "renderers/webgl/WebGLProperties.h"
#include "renderers/webgl/WebGLUtils.h"
#include "textures/DataTexture.h"
#include "core/NativeArray.h"
#include "core/JSJobs.h"
#include "core/JSPromise.h"
#include "constants.h"
#include <limits>
#include <map>
#include <thread>

namespace {
using Args=std::vector<JSValue>;
JSValue call(JSValue self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue list(const Args& values={}){auto out=JSValue::array();for(const auto& v:values)out.set(JSValue(out.length()),v);return out;}
void push(JSValue values,JSValue v){values.set(JSValue(values.length()),v);}
struct Upload {
    VulkanTextureCommands::Upload command;
    std::vector<std::byte> bytes;
    explicit Upload(const VulkanTextureCommands::Upload& value):command(value),bytes(value.pixels.begin(),value.pixels.end()){command.pixels={};}
};
// This is explicitly a synchronous CPU device recorder. It verifies the real
// source resource manager and adapter without claiming to execute Vulkan.
struct Memory final:VulkanTextureCommands::Storage {
    std::vector<VulkanTextureCommands::Allocation> allocations;
    std::vector<Upload> uploads;
    std::map<std::uint32_t,double> parameters;
    int mipmaps=0;
    bool failAllocation=false,failUpload=false,failParameter=false,failMipmap=false;
    void allocate(const VulkanTextureCommands::Allocation& v)override{if(failAllocation)throw JSRangeError("allocation failed");allocations.push_back(v);}
    void upload(const VulkanTextureCommands::Upload& v)override{if(failUpload)throw JSRangeError("upload failed");uploads.emplace_back(v);}
    void parameter(std::uint32_t name,double v)override{if(failParameter)throw JSRangeError("parameter failed");parameters[name]=v;}
    void generateMipmap(std::uint32_t)override{if(failMipmap)throw JSRangeError("mipmap failed");++mipmaps;}
};
struct FramebufferMemory final:VulkanTextureCommands::FramebufferStorage {
    struct Attachment{std::shared_ptr<VulkanTextureCommands::Storage> texture;std::uint32_t target;int level,layer;};
    std::map<std::uint32_t,Attachment> attachments;
    void texture(std::uint32_t name,const std::shared_ptr<VulkanTextureCommands::Storage>& value,std::uint32_t target,int level,int layer)override{
        if(value)attachments[name]={value,target,level,layer};else attachments.erase(name);
    }
    std::uint32_t status(std::uint32_t)override{return attachments.empty()?0x8cd7:0x8cd5;}
};
struct Fixture {
    std::vector<std::weak_ptr<Memory>> created;
    std::vector<std::weak_ptr<FramebufferMemory>> framebuffers;
    VulkanTextureCommands commands{
        [this]{auto m=std::make_shared<Memory>();created.emplace_back(m);return m;},
        [this]{auto m=std::make_shared<FramebufferMemory>();framebuffers.emplace_back(m);return m;}};
    JSValue gl=commands.object();
    ~Fixture(){commands.shutdown();}
    JSValue create(std::uint32_t target=0x0de1){auto t=call(gl,"createTexture");call(gl,"bindTexture",{target,t});return t;}
    std::shared_ptr<Memory> memory(JSValue t){return std::dynamic_pointer_cast<Memory>(commands.storage(t));}
    JSValue allocate(int width=2,int height=2){auto t=create();call(gl,"texStorage2D",{0x0de1,1,0x8058,width,height});return t;}
};
std::vector<int> numbers(const std::vector<std::byte>& bytes){std::vector<int> out;for(auto b:bytes)out.push_back(std::to_integer<int>(b));return out;}

struct SourceFixture {
    Fixture device;
    JSValue gl=device.gl,extensions=JSValue::object(),state,properties=WebGLProperties().object(),manager,info=JSValue::object();
    JSValue nonTextureCommands=JSValue::array();
    SourceFixture(){
        extensions.set("has",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return false;}));
        extensions.set("get",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return nullptr;}));
        const auto getParameter=gl.get("getParameter");
        gl.set("getParameter",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
            if(a.at(0).strictEquals(0x1f02))return "OpenGL ES 3.0 CPU command recorder";
            if(a.at(0).strictEquals(0x0ba2)||a.at(0).strictEquals(0x0c10))return list({0,0,8,8});
            return c[0].call(c[1],a);
        },{getParameter,gl}));
        auto canvas=JSValue::object();canvas.set("width",8);canvas.set("height",8);gl.set("canvas",canvas);
        // Only unrelated low level state initialization is recorded here.
        // Texture methods all remain the real VulkanTextureCommands methods.
        for(const auto* name:{"clearColor","clearDepth","clearStencil","enable","disable","depthFunc","frontFace","cullFace"})
            gl.set(name,JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{auto event=list({c[1]});for(const auto& v:a)push(event,v);push(c[0],event);return {};},{nonTextureCommands,name}));
        state=nativeWebGLStateConstructor().construct({gl,extensions});
        auto caps=JSValue::object();caps.set("maxTextures",16);caps.set("maxTextureSize",4096);caps.set("maxCubemapSize",4096);caps.set("maxSamples",1);
        caps.set("getMaxAnisotropy",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return 1;}));
        auto memory=JSValue::object(),render=JSValue::object();memory.set("textures",0);render.set("frame",0);info.set("memory",memory);info.set("render",render);
        const auto utils=nativeWebGLUtilsConstructor().construct({gl,extensions});
        manager=nativeWebGLTexturesConstructor().construct({gl,extensions,state,properties,caps,utils,info});
        REQUIRE(call(gl,"getError").strictEquals(0));
    }
    ~SourceFixture(){call(manager,"dispose");}
    JSValue texture(int width=2,int height=2){
        Uint8Array data(std::size_t(width*height*4));for(std::size_t i=0;i<data.size();++i)data[i]=double(i+1);
        return nativeDataTextureConstructor().construct({NativeArray_detail::wrap(data),width,height});
    }
    void upload(JSValue t,int slot=0){call(manager,"setTexture2D",{t,slot});}
    JSValue handle(JSValue t){return call(properties,"get",{t}).get("__webglTexture");}
    std::shared_ptr<Memory> memory(JSValue t){return device.memory(handle(t));}
};
}

TEST_CASE("Vulkan texture commands expose real GL tokens and stable opaque resource identity","[vulkan-texture-commands]"){
    Fixture f;REQUIRE(f.gl.get("TEXTURE_2D").strictEquals(0x0de1));REQUIRE(f.gl.get("RGBA8").strictEquals(0x8058));
    REQUIRE(f.gl.get("UNPACK_ROW_LENGTH").strictEquals(0x0cf2));REQUIRE(f.gl.get("UNPACK_FLIP_Y_WEBGL").strictEquals(0x9240));
    auto t=f.create(),other=f.create();t.set("uuid","same");other.set("uuid","same");t.set("id",1);other.set("id",1);
    REQUIRE_FALSE(t.sameValue(other));REQUIRE(f.commands.storage(t)!=f.commands.storage(other));
    REQUIRE(f.commands.generation(t)!=f.commands.generation(other));
    REQUIRE(f.commands.boundTexture(0,0x0de1).sameValue(other));REQUIRE(call(f.gl,"getParameter",{0x8069}).sameValue(other));
    call(f.gl,"activeTexture",{0x84c2});call(f.gl,"bindTexture",{0x0de1,t});
    REQUIRE(f.commands.boundTexture(0,0x0de1).sameValue(other));REQUIRE(f.commands.boundTexture(2,0x0de1).sameValue(t));
    REQUIRE(call(f.gl,"getParameter",{0x84e0}).strictEquals(0x84c2));
    call(f.gl,"bindTexture",{0x8513,t});REQUIRE(call(f.gl,"getError").strictEquals(0x0502));
    REQUIRE(f.commands.boundTexture(2,0x8513).isNull());REQUIRE(call(f.gl,"isTexture",{t}).strictEquals(true));
}
TEST_CASE("Vulkan immutable texture allocation and uploads honor actual typed view bytes and element offsets","[vulkan-texture-commands]"){
    Fixture f;auto t=f.allocate(2,1);Uint8Array backing{99,99,1,2,3,4,5,6,7,8,99,99};auto view=NativeArray_detail::wrap(backing.subarray(2,10));
    JSPropertyDescriptor fake;fake.value=999;fake.configurable=true;fake.writable=true;
    for(const auto* key:{"byteOffset","byteLength","length","BYTES_PER_ELEMENT"})view.defineProperty(key,fake);
    call(f.gl,"texSubImage2D",{0x0de1,0,0,0,2,1,0x1908,0x1401,view});
    auto m=f.memory(t);REQUIRE(m->allocations.size()==1);REQUIRE(m->allocations[0].immutable);REQUIRE(m->allocations[0].internalFormat==0x8058);
    REQUIRE(numbers(m->uploads[0].bytes)==std::vector<int>{1,2,3,4,5,6,7,8});REQUIRE(f.commands.revision(t)==2);
    Uint16Array words{99,0x3c00,0x4000,0x4200,0x4400};
    call(f.gl,"texSubImage2D",{0x0de1,0,0,0,1,1,0x1908,0x140b,NativeArray_detail::wrap(words),1});
    REQUIRE(numbers(m->uploads.back().bytes)==std::vector<int>{0,60,0,64,0,66,0,68});
    REQUIRE(std::uint8_t(backing[0])==99);REQUIRE(std::uint8_t(backing[10])==99);REQUIRE(call(f.gl,"getError").strictEquals(0));
}
TEST_CASE("Vulkan texture unit bindings stay sparse with the full reported descriptor limit","[vulkan-texture-commands]"){
    VulkanTextureCommands::Limits limits;limits.maxTextureUnits=std::numeric_limits<int>::max();
    std::size_t created=0;VulkanTextureCommands commands([&]{++created;return std::make_shared<Memory>();},{},limits);auto gl=commands.object();
    const auto last=std::uint32_t(limits.maxTextureUnits-1),middle=last/2;
    REQUIRE(created==0);REQUIRE(call(gl,"getParameter",{0x8872}).strictEquals(limits.maxTextureUnits));
    REQUIRE(call(gl,"getParameter",{0x8b4d}).strictEquals(limits.maxTextureUnits));
    for(const auto unit:{0u,middle,last}){
        call(gl,"activeTexture",{0x84c0u+unit});REQUIRE(call(gl,"getParameter",{0x84e0}).strictEquals(0x84c0u+unit));
        for(const auto& [target,pname]:std::initializer_list<std::pair<std::uint32_t,std::uint32_t>>{{0x0de1,0x8069},{0x8513,0x8514},{0x806f,0x806a},{0x8c1a,0x8c1d}}){
            REQUIRE(commands.boundTexture(unit,target).isNull());REQUIRE(call(gl,"getParameter",{pname}).isNull());call(gl,"bindTexture",{target,nullptr});
        }
    }
    REQUIRE(created==0);
    auto shared=call(gl,"createTexture"),cube=call(gl,"createTexture"),array=call(gl,"createTexture");
    std::weak_ptr<VulkanTextureCommands::Storage> sharedStorage=commands.storage(shared);
    call(gl,"bindTexture",{0x0de1,shared});call(gl,"bindTexture",{0x8513,cube});
    call(gl,"activeTexture",{0x84c0u+middle});call(gl,"bindTexture",{0x8c1a,array});
    call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,shared});
    REQUIRE(commands.boundTexture(last,0x0de1).sameValue(shared));REQUIRE(commands.boundTexture(last,0x8513).sameValue(cube));
    REQUIRE(commands.boundTexture(middle,0x8c1a).sameValue(array));REQUIRE(commands.boundTexture(0,0x0de1).sameValue(shared));
    call(gl,"deleteTexture",{shared});REQUIRE(commands.isDeleted(shared));REQUIRE(sharedStorage.expired());
    REQUIRE(commands.boundTexture(0,0x0de1).isNull());REQUIRE(commands.boundTexture(last,0x0de1).isNull());
    REQUIRE(commands.boundTexture(last,0x8513).sameValue(cube));REQUIRE(commands.boundTexture(middle,0x8c1a).sameValue(array));
    call(gl,"activeTexture",{0x84c0u+last});call(gl,"activeTexture",{0x84c0u+std::uint32_t(limits.maxTextureUnits)});
    REQUIRE(call(gl,"getParameter",{0x84e0}).strictEquals(0x84c0u+last));REQUIRE(call(gl,"getParameter",{0x8514}).sameValue(cube));
    REQUIRE(call(gl,"getError").strictEquals(0x0500));REQUIRE(call(gl,"getError").strictEquals(0));
    REQUIRE_THROWS_AS(commands.boundTexture(std::uint32_t(limits.maxTextureUnits),0x0de1),JSRangeError);
    call(gl,"bindTexture",{0x8513,nullptr});REQUIRE(commands.boundTexture(last,0x8513).isNull());
    call(gl,"texParameteri",{0x8513,0x2801,0x2600});REQUIRE(call(gl,"getError").strictEquals(0x0502));
    commands.shutdown();REQUIRE_THROWS_AS(call(gl,"activeTexture",{0x84c0}),JSTypeError);
    REQUIRE_THROWS_AS(commands.boundTexture(middle,0x8c1a),JSTypeError);
}
TEST_CASE("Vulkan texture pixel-store uses alignment rows skips images and keeps unpack flags","[vulkan-texture-commands]"){
    Fixture f;auto t=f.allocate(2,2);Uint8Array data(48);for(std::size_t i=0;i<data.size();++i)data[i]=double(i);
    for(const auto& [name,value]:std::initializer_list<std::pair<int,int>>{{0x0cf2,3},{0x0cf3,1},{0x0cf4,1},{0x9240,1},{0x9241,1},{0x9243,0}})call(f.gl,"pixelStorei",{name,value});
    call(f.gl,"texSubImage2D",{0x0de1,0,0,0,2,2,0x1908,0x1401,NativeArray_detail::wrap(data)});
    auto m=f.memory(t);REQUIRE(m->uploads.size()==1);const auto& u=m->uploads[0];
    REQUIRE(u.command.unpack.rowLength==3);REQUIRE(u.command.unpack.skipRows==1);REQUIRE(u.command.unpack.skipPixels==1);
    REQUIRE(u.command.unpack.flipY);REQUIRE(u.command.unpack.premultiplyAlpha);REQUIRE(u.command.unpack.colorSpaceConversion==0);
    REQUIRE(numbers(u.bytes).at(16)==16);REQUIRE(call(f.gl,"getParameter",{0x0cf2}).strictEquals(3));
    call(f.gl,"pixelStorei",{0x0cf5,3});REQUIRE(call(f.gl,"getError").strictEquals(0x0501));REQUIRE(call(f.gl,"getParameter",{0x0cf5}).strictEquals(4));
    call(f.gl,"texSubImage2D",{0x0de1,0,0,0,2,2,0x1908,0x1401,NativeArray_detail::wrap(data.subarray(0,31))});
    REQUIRE(call(f.gl,"getError").strictEquals(0x0502));REQUIRE(m->uploads.size()==1);
}
TEST_CASE("Vulkan texture command API errors are sticky no-ops while unsupported operations throw","[vulkan-texture-commands]"){
    Fixture f;auto t=f.allocate();auto m=f.memory(t);const auto revision=f.commands.revision(t);auto pixels=NativeArray_detail::wrap(Uint8Array(16));
    call(f.gl,"texStorage2D",{0x0de1,1,0x8058,2,2});call(f.gl,"texSubImage2D",{0x0de1,0,1,1,2,2,0x1908,0x1401,pixels});
    call(f.gl,"activeTexture",{0});call(f.gl,"pixelStorei",{0x0cf5,7});
    REQUIRE(call(f.gl,"getError").strictEquals(0x0500));REQUIRE(call(f.gl,"getError").strictEquals(0x0501));REQUIRE(call(f.gl,"getError").strictEquals(0x0502));REQUIRE(call(f.gl,"getError").strictEquals(0));
    REQUIRE(f.commands.revision(t)==revision);REQUIRE(m->uploads.empty());
    call(f.gl,"texSubImage2D",{0x0de1,0,0,0,2,2,0x1908,0x1401,NativeArray_detail::wrap(Float32Array(16))});REQUIRE(call(f.gl,"getError").strictEquals(0x0502));
    REQUIRE_THROWS_AS(call(f.gl,"texImage2D",{0x0de1,0,0x1908,0x1908,0x1401,JSValue::object()}),JSTypeError);
    REQUIRE_THROWS_AS(call(f.gl,"compressedTexImage2D",{}),JSTypeError);REQUIRE_THROWS_AS(call(f.gl,"createRenderbuffer"),JSTypeError);
    REQUIRE_THROWS_AS(call(f.gl,"readPixels",{}),JSTypeError);REQUIRE(call(f.gl,"getError").strictEquals(0));
}
TEST_CASE("Vulkan texture conversion reentry resolves binding and live bytes after WebIDL conversions","[vulkan-texture-commands]"){
    Fixture f;auto first=f.allocate(1,1),second=f.allocate(1,1),pixels=NativeArray_detail::wrap(Uint8Array{1,2,3,4});
    call(f.gl,"bindTexture",{0x0de1,first});auto width=JSValue::object();
    width.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"bindTexture",{0x0de1,c[1]});auto data=c[2];data.set("0",9);return 1;},{f.gl,second,pixels}));
    call(f.gl,"texSubImage2D",{0x0de1,0,0,0,width,1,0x1908,0x1401,pixels});
    REQUIRE(f.memory(first)->uploads.empty());REQUIRE(numbers(f.memory(second)->uploads[0].bytes)==std::vector<int>{9,2,3,4});
    REQUIRE(call(f.gl,"getError").strictEquals(0));
}
TEST_CASE("Vulkan immutable storage distinguishes invalid sizes from an excessive mip chain","[vulkan-texture-commands]"){
    Fixture f;auto t=f.create();auto m=f.memory(t);
    for(const auto& args:std::initializer_list<Args>{{0x0de1,0,0x8058,2,2},{0x0de1,-1,0x8058,2,2},{0x0de1,1,0x8058,0,2},{0x0de1,1,0x8058,2,0}}){
        call(f.gl,"texStorage2D",args);REQUIRE(call(f.gl,"getError").strictEquals(0x0501));REQUIRE(m->allocations.empty());REQUIRE(f.commands.revision(t)==0);
    }
    call(f.gl,"texStorage2D",{0x0de1,3,0x8058,2,2});REQUIRE(call(f.gl,"getError").strictEquals(0x0502));REQUIRE(m->allocations.empty());
    auto volume=f.create(0x806f);call(f.gl,"texStorage3D",{0x806f,1,0x8058,2,2,0});
    REQUIRE(call(f.gl,"getError").strictEquals(0x0501));REQUIRE(f.memory(volume)->allocations.empty());
    call(f.gl,"texStorage3D",{0x806f,2,0x8058,2,2,2});REQUIRE(call(f.gl,"getError").strictEquals(0));REQUIRE(f.memory(volume)->allocations.size()==1);
}
TEST_CASE("Vulkan texture storage exceptions preserve previously committed allocation and resource revision","[vulkan-texture-commands]"){
    Fixture f;auto t=f.create();auto m=f.memory(t);m->failAllocation=true;
    REQUIRE_THROWS_AS(call(f.gl,"texStorage2D",{0x0de1,1,0x8058,1,1}),JSRangeError);REQUIRE(f.commands.revision(t)==0);
    m->failAllocation=false;call(f.gl,"texStorage2D",{0x0de1,1,0x8058,1,1});m->failUpload=true;
    REQUIRE_THROWS_AS(call(f.gl,"texSubImage2D",{0x0de1,0,0,0,1,1,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array{1,2,3,4})}),JSRangeError);
    REQUIRE(f.commands.revision(t)==1);REQUIRE(m->uploads.empty());m->failParameter=true;
    REQUIRE_THROWS_AS(call(f.gl,"texParameteri",{0x0de1,0x2801,0x2600}),JSRangeError);REQUIRE(f.commands.revision(t)==1);
    m->failParameter=false;call(f.gl,"texParameteri",{0x0de1,0x2801,0x2600});call(f.gl,"generateMipmap",{0x0de1});
    REQUIRE(m->mipmaps==1);REQUIRE(f.commands.revision(t)==3);REQUIRE(call(f.gl,"getError").strictEquals(0));
}
TEST_CASE("Vulkan texture commands dispatch cube faces and three dimensional storage without typed textures","[vulkan-texture-commands]"){
    Fixture f;auto cube=f.create(0x8513);call(f.gl,"texStorage2D",{0x8513,2,0x8058,2,2});
    for(int i=0;i<6;++i)call(f.gl,"texSubImage2D",{0x8515+i,1,0,0,1,1,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array{std::uint8_t(i),0,0,255})});
    REQUIRE(f.memory(cube)->uploads.size()==6);REQUIRE(f.memory(cube)->uploads.back().command.target==0x851a);
    auto volume=f.create(0x806f);call(f.gl,"texStorage3D",{0x806f,2,0x8058,2,2,2});
    call(f.gl,"texSubImage3D",{0x806f,0,0,0,1,2,2,1,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array(16))});
    const auto& u=f.memory(volume)->uploads.back().command;REQUIRE(u.z==1);REQUIRE(u.depth==1);REQUIRE(f.commands.revision(volume)==2);
    auto fallback=f.create(0x8c1a);call(f.gl,"texImage3D",{0x8c1a,0,0x1908,1,1,1,0,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array(4))});
    REQUIRE_FALSE(f.memory(fallback)->allocations[0].immutable);REQUIRE(call(f.gl,"getError").strictEquals(0));
}
TEST_CASE("Vulkan generated mutable mip levels permit later subimage uploads with odd base dimensions","[vulkan-texture-commands]"){
    Fixture f;auto t=f.create();call(f.gl,"texImage2D",{0x0de1,0,0x8058,3,3,0,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array(36))});auto m=f.memory(t);
    m->failMipmap=true;REQUIRE_THROWS_AS(call(f.gl,"generateMipmap",{0x0de1}),JSRangeError);
    call(f.gl,"texSubImage2D",{0x0de1,1,0,0,1,1,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array{9,8,7,255})});
    REQUIRE(call(f.gl,"getError").strictEquals(0x0502));REQUIRE(m->uploads.size()==1);
    m->failMipmap=false;call(f.gl,"generateMipmap",{0x0de1});const auto revision=f.commands.revision(t);
    call(f.gl,"texSubImage2D",{0x0de1,1,0,0,1,1,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array{9,8,7,255})});
    REQUIRE(call(f.gl,"getError").strictEquals(0));REQUIRE(f.commands.revision(t)==revision+1);REQUIRE(m->uploads.back().command.level==1);
    REQUIRE(numbers(m->uploads.back().bytes)==std::vector<int>{9,8,7,255});
    call(f.gl,"texSubImage2D",{0x0de1,1,0,0,2,1,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array(8))});
    REQUIRE(call(f.gl,"getError").strictEquals(0x0501));REQUIRE(f.commands.revision(t)==revision+1);
    auto volume=f.create(0x806f);call(f.gl,"texImage3D",{0x806f,0,0x8058,3,3,3,0,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array(108))});
    call(f.gl,"generateMipmap",{0x806f});call(f.gl,"texSubImage3D",{0x806f,1,0,0,0,1,1,1,0x1908,0x1401,NativeArray_detail::wrap(Uint8Array(4))});
    REQUIRE(call(f.gl,"getError").strictEquals(0));REQUIRE(f.memory(volume)->uploads.back().command.level==1);
}
TEST_CASE("Vulkan framebuffer attachments retain deleted texture storage until detach and report device status","[vulkan-texture-commands]"){
    Fixture f;auto t=f.allocate(),frame=call(f.gl,"createFramebuffer");const auto weak=f.created.back();
    call(f.gl,"bindFramebuffer",{0x8d40,frame});call(f.gl,"framebufferTexture2D",{0x8d40,0x8ce0,0x0de1,t,0});
    REQUIRE(call(f.gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd5));
    call(f.gl,"bindFramebuffer",{0x8d40,nullptr});call(f.gl,"deleteTexture",{t});REQUIRE_FALSE(weak.expired());REQUIRE(f.commands.isDeleted(t));
    REQUIRE_THROWS_AS(f.commands.storage(t),JSTypeError);REQUIRE(f.commands.boundTexture(0,0x0de1).isNull());
    call(f.gl,"bindFramebuffer",{0x8d40,frame});call(f.gl,"framebufferTexture2D",{0x8d40,0x8ce0,0x0de1,nullptr,0});
    REQUIRE(weak.expired());REQUIRE(call(f.gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd7));
    call(f.gl,"deleteFramebuffer",{frame});REQUIRE(f.framebuffers.back().expired());REQUIRE(call(f.gl,"isFramebuffer",{frame}).strictEquals(false));
    REQUIRE_THROWS_AS(call(f.gl,"checkFramebufferStatus",{0x8d40}),JSTypeError);
}
TEST_CASE("Vulkan texture shutdown retires device resources and rejects escaped callbacks safely","[vulkan-texture-commands]"){
    Fixture f;auto t=f.allocate();auto escaped=f.gl.get("createTexture");auto oldGeneration=f.commands.generation(t);call(f.gl,"deleteTexture",{t});
    REQUIRE(f.commands.generation(t)>oldGeneration);REQUIRE(f.created.back().expired());REQUIRE(call(f.gl,"isTexture",{t}).strictEquals(false));
    auto live=f.allocate();const auto weak=f.created.back();f.commands.shutdown();f.commands.shutdown();REQUIRE(weak.expired());
    REQUIRE_THROWS_AS(escaped.call(nullptr),JSTypeError);REQUIRE_THROWS_AS(f.commands.storage(live),JSTypeError);
}

TEST_CASE("Full WebGLTextures and WebGLState drive canonical DataTexture revision and live native uploads","[vulkan-texture-commands][webgl-textures]"){
    SourceFixture c;auto t=c.texture();REQUIRE(t.get("version").strictEquals(0));
    c.upload(t);REQUIRE(c.handle(t).isUndefined()); // Source requires needsUpdate before initial allocation.
    t.set("needsUpdate",true);REQUIRE(t.get("version").strictEquals(1));REQUIRE(t.get("source").get("version").strictEquals(1));
    c.upload(t,3);auto handle=c.handle(t);auto m=c.memory(t);REQUIRE(m->allocations.size()==1);REQUIRE(m->uploads.size()==1);
    REQUIRE(m->allocations[0].internalFormat==0x8058);REQUIRE(numbers(m->uploads[0].bytes)==std::vector<int>{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16});
    REQUIRE(c.device.commands.boundTexture(3,0x0de1).sameValue(handle));auto array=t.get("image").get("data");array.set("0",77);
    c.upload(t,3);REQUIRE(m->uploads.size()==1);t.set("needsUpdate",true);c.upload(t,3);
    REQUIRE(m->allocations.size()==1);REQUIRE(m->uploads.size()==2);REQUIRE(numbers(m->uploads.back().bytes)[0]==77);
    REQUIRE(call(c.properties,"get",{t}).get("__version").strictEquals(2));REQUIRE(call(c.device.gl,"getError").strictEquals(0));
}
TEST_CASE("Full WebGLTextures updates original range aliases and restores actual unpack state","[vulkan-texture-commands][webgl-textures]"){
    SourceFixture c;auto t=c.texture(4,2);t.set("needsUpdate",true);c.upload(t);auto m=c.memory(t);const auto before=m->uploads.size();
    call(t,"addUpdateRange",{8,4});call(t,"addUpdateRange",{0,4});call(t,"addUpdateRange",{3,5});
    const auto ranges=t.get("updateRanges"),merged=ranges.get("1");t.set("needsUpdate",true);c.upload(t);
    REQUIRE(t.get("updateRanges").sameValue(ranges));REQUIRE(ranges.length()==0);REQUIRE(merged.get("count").strictEquals(12));
    REQUIRE(m->uploads.size()==before+1);const auto& u=m->uploads.back().command;REQUIRE(u.width==3);REQUIRE(u.height==1);REQUIRE(u.unpack.rowLength==4);
    REQUIRE(call(c.device.gl,"getParameter",{0x0cf2}).strictEquals(0));REQUIRE(call(c.device.gl,"getParameter",{0x0cf4}).strictEquals(0));REQUIRE(call(c.device.gl,"getParameter",{0x0cf3}).strictEquals(0));
    REQUIRE(call(c.device.gl,"getError").strictEquals(0));
}
TEST_CASE("Full WebGLTextures keeps Source cache identity distinct from texture identity and UUID collisions","[vulkan-texture-commands][webgl-textures]"){
    SourceFixture c;auto a=c.texture(),b=c.texture();b.set("source",a.get("source"));a.set("uuid","collision");b.set("uuid","collision");
    a.set("needsUpdate",true);b.set("needsUpdate",true);c.upload(a,1);c.upload(b,2);auto shared=c.handle(a);auto weak=std::weak_ptr<Memory>(c.memory(a));
    REQUIRE(shared.sameValue(c.handle(b)));REQUIRE(c.info.get("memory").get("textures").strictEquals(1));
    call(a,"dispose");REQUIRE_FALSE(weak.expired());REQUIRE_FALSE(c.device.commands.isDeleted(shared));
    call(b,"dispose");REQUIRE(weak.expired());REQUIRE(c.device.commands.isDeleted(shared));REQUIRE(c.info.get("memory").get("textures").strictEquals(0));
    c.upload(b,2);REQUIRE_FALSE(c.handle(b).sameValue(shared));REQUIRE(c.info.get("memory").get("textures").strictEquals(1));
    auto separate=c.texture();separate.set("uuid","collision");separate.set("needsUpdate",true);c.upload(separate);REQUIRE_FALSE(c.handle(separate).sameValue(c.handle(b)));
    auto variant=c.texture();variant.set("source",b.get("source"));variant.set("wrapS",RepeatWrapping);variant.set("needsUpdate",true);c.upload(variant);REQUIRE_FALSE(c.handle(variant).sameValue(c.handle(b)));
    REQUIRE(call(c.device.gl,"getError").strictEquals(0));
}
TEST_CASE("Full WebGLTextures calls original onUpdate after GPU upload and before texture version assignment","[vulkan-texture-commands][webgl-textures]"){
    SourceFixture c;auto t=c.texture(),events=JSValue::array();t.set("needsUpdate",true);
    t.set("onUpdate",JSValue::function(+[](const JSValue& self,const Args& a,const Args& cap)->JSValue{
        const auto p=call(cap[1],"get",{self}),source=call(cap[1],"get",{self.get("source")});
        push(cap[0],list({self.sameValue(cap[2]),a.at(0).sameValue(self),p.get("__version"),source.get("__version")}));return {};
    },{events,c.properties,t}));
    c.upload(t);REQUIRE(events.length()==1);REQUIRE(events.get("0").get("0").strictEquals(true));REQUIRE(events.get("0").get("1").strictEquals(true));
    REQUIRE(events.get("0").get("2").isUndefined());REQUIRE(events.get("0").get("3").strictEquals(1));
    REQUIRE(call(c.properties,"get",{t}).get("__version").strictEquals(1));
    t.set("onUpdate",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSThrownValue("onUpdate sentinel");}));
    t.set("needsUpdate",true);REQUIRE_THROWS_AS(c.upload(t),JSThrownValue);REQUIRE(c.memory(t)->uploads.size()==2);
    REQUIRE(call(c.properties,"get",{t}).get("__version").strictEquals(1));REQUIRE(call(c.properties,"get",{t.get("source")}).get("__version").strictEquals(2));
    // Retrying the same source version updates texture bookkeeping without a
    // second GPU upload or a second callback, exactly as the source algorithm.
    c.upload(t);REQUIRE(c.memory(t)->uploads.size()==2);REQUIRE(call(c.properties,"get",{t}).get("__version").strictEquals(2));
}
TEST_CASE("Vulkan texture bindings expose actual native managed ownership to collection","[vulkan-texture-commands][gc]"){
    std::exception_ptr failure;std::thread worker([&]{try{
        auto keeper=JSValue::object();JSWeakValue weakTexture;std::weak_ptr<Memory> weakStorage;
        {
            Fixture f;{auto t=f.allocate();weakTexture=JSWeakValue(t);weakStorage=f.memory(t);}keeper.collectGarbage();
            REQUIRE_FALSE(weakTexture.expired());REQUIRE_FALSE(weakStorage.expired());REQUIRE(f.commands.boundTexture(0,0x0de1).sameValue(*weakTexture.lock()));
            call(f.gl,"bindTexture",{0x0de1,nullptr});keeper.collectGarbage();REQUIRE(weakTexture.expired());REQUIRE(weakStorage.expired());
        }
    }catch(...){failure=std::current_exception();}});worker.join();if(failure)std::rethrow_exception(failure);
}
TEST_CASE("Vulkan texture command owners release managed resource cycles and native device storage","[vulkan-texture-commands][gc]"){
    std::exception_ptr failure;std::thread worker([&]{try{
        auto keeper=JSValue::object();JSWeakValue weakGl,weakTexture;std::weak_ptr<Memory> weakStorage;
        {
            VulkanTextureCommands commands([]{return std::make_shared<Memory>();});auto gl=commands.object();auto t=call(gl,"createTexture");
            call(gl,"bindTexture",{0x0de1,t});gl.set("cycle",t);t.set("owner",gl);
            weakGl=JSWeakValue(gl);weakTexture=JSWeakValue(t);weakStorage=std::dynamic_pointer_cast<Memory>(commands.storage(t));
        }
        keeper.collectGarbage();REQUIRE(weakGl.expired());REQUIRE(weakTexture.expired());REQUIRE(weakStorage.expired());
    }catch(...){failure=std::current_exception();}});worker.join();if(failure)std::rethrow_exception(failure);
}
TEST_CASE("Sparse high texture unit bindings retain raw identity until the last unbind or shutdown","[vulkan-texture-commands][gc]"){
    std::exception_ptr failure;std::thread worker([&]{try{
        auto keeper=JSValue::object();VulkanTextureCommands::Limits limits;limits.maxTextureUnits=1048576;
        VulkanTextureCommands commands([]{return std::make_shared<Memory>();},{},limits);auto gl=commands.object();
        const auto last=std::uint32_t(limits.maxTextureUnits-1);JSWeakValue weakTexture;std::weak_ptr<VulkanTextureCommands::Storage> weakStorage;
        {
            auto t=call(gl,"createTexture");weakTexture=JSWeakValue(t);weakStorage=commands.storage(t);
            call(gl,"bindTexture",{0x0de1,t});call(gl,"activeTexture",{0x84c0u+last});call(gl,"bindTexture",{0x0de1,t});
        }
        keeper.collectGarbage();REQUIRE_FALSE(weakTexture.expired());REQUIRE_FALSE(weakStorage.expired());
        REQUIRE(commands.boundTexture(last,0x0de1).sameValue(*weakTexture.lock()));
        call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,nullptr});keeper.collectGarbage();
        REQUIRE_FALSE(weakTexture.expired());REQUIRE_FALSE(weakStorage.expired());
        REQUIRE(commands.boundTexture(last,0x0de1).sameValue(*weakTexture.lock()));
        call(gl,"activeTexture",{0x84c0u+last});call(gl,"bindTexture",{0x0de1,nullptr});keeper.collectGarbage();
        REQUIRE(weakTexture.expired());REQUIRE(weakStorage.expired());
        {
            auto t=call(gl,"createTexture");weakTexture=JSWeakValue(t);weakStorage=commands.storage(t);call(gl,"bindTexture",{0x8513,t});
        }
        keeper.collectGarbage();REQUIRE_FALSE(weakTexture.expired());REQUIRE_FALSE(weakStorage.expired());
        commands.shutdown();keeper.collectGarbage();REQUIRE(weakTexture.expired());REQUIRE(weakStorage.expired());
    }catch(...){failure=std::current_exception();}});worker.join();if(failure)std::rethrow_exception(failure);
}
