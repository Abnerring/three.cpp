#pragma once
#include "core/JSValue.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

// Command/state boundary used by the original WebGLTextures and WebGLState.
// Factories and resource implementations must not conceal managed JSValue roots.
// Each successful GPU operation commits immediately; throwing leaves the prior
// resource state intact. The owner must shutdown before destroying its device.
class VulkanTextureCommands {
  public:
    struct PixelStore {
        int alignment=4,rowLength=0,imageHeight=0,skipPixels=0,skipRows=0,skipImages=0;
        bool flipY=false,premultiplyAlpha=false;
        std::uint32_t colorSpaceConversion=0x9244; // BROWSER_DEFAULT_WEBGL
    };
    struct Allocation {
        std::uint32_t target=0,internalFormat=0;
        // Mutable texImage specifies one level; immutable storage specifies all.
        int level=0,levels=1,width=0,height=0,depth=1;
        bool immutable=true;
    };
    struct Upload {
        std::uint32_t target=0,format=0,type=0;
        int level=0,x=0,y=0,z=0,width=0,height=0,depth=1;
        std::span<const std::byte> pixels;
        PixelStore unpack;
        bool nullData=false,compressed=false;
    };
    struct Storage {
        virtual ~Storage()=default;
        virtual void allocate(const Allocation& allocation)=0;
        virtual void upload(const Upload& upload)=0;
        virtual void parameter(std::uint32_t name,double value)=0;
        virtual void generateMipmap(std::uint32_t target)=0;
    };
    struct FramebufferStorage {
        virtual ~FramebufferStorage()=default;
        virtual void texture(std::uint32_t attachment,const std::shared_ptr<Storage>& texture,
                             std::uint32_t target,int level,int layer)=0;
        virtual std::uint32_t status(std::uint32_t target)=0;
    };
    struct Limits {
        int maxTextureUnits=16,maxTextureSize=4096,maxCubeMapSize=4096;
        int max3DTextureSize=256,maxArrayLayers=256,maxSamples=1;
    };
    using Factory=std::function<std::shared_ptr<Storage>()>;
    using FramebufferFactory=std::function<std::shared_ptr<FramebufferStorage>()>;
    explicit VulkanTextureCommands(Factory factory,FramebufferFactory framebuffers={});
    VulkanTextureCommands(Factory factory,FramebufferFactory framebuffers,Limits limits);
    JSValue object() const;
    // Sampling snapshots resolve the actual handle selected by native bindTexture,
    // independently of texture UUID/id, source objects, or uploaded pixel copies.
    JSValue boundTexture(std::uint32_t unit,std::uint32_t target) const;
    std::shared_ptr<Storage> storage(const JSValue& handle) const;
    std::uint64_t revision(const JSValue& handle) const;
    // Includes deleted handles; generations change at disposal, and newly
    // created handles receive a distinct generation within this command owner.
    std::uint64_t generation(const JSValue& handle) const;
    bool isDeleted(const JSValue& handle) const;
    void shutdown();
  private:
    JSValue state_,object_;
};
