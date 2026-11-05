// Included inside VulkanRenderer::Impl, after TextureResource and the GPU
// helpers. This storage implements GL commands; it never reads a Texture,
// Source, application version, cache key, UUID, or material.
std::unique_ptr<VulkanTextureCommands> managedTextureCommands;

struct ManagedTextureShape {
    std::uint32_t target=0x0de1,depth=1;
    std::uint32_t layers()const{return target==0x8513?6:target==0x8c1a?depth:1;}
    std::uint32_t mipDepth(std::uint32_t level)const{return target==0x806f?std::max(1u,depth>>level):1;}
    VkImageType type()const{return target==0x806f?VK_IMAGE_TYPE_3D:VK_IMAGE_TYPE_2D;}
    VkImageViewType view()const{return target==0x8513?VK_IMAGE_VIEW_TYPE_CUBE:target==0x806f?VK_IMAGE_VIEW_TYPE_3D:target==0x8c1a?VK_IMAGE_VIEW_TYPE_2D_ARRAY:VK_IMAGE_VIEW_TYPE_2D;}
};
struct ManagedTextureSampler {
    VkSamplerAddressMode wrapR=VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float minLod=-1000,maxLod=1000;
    std::uint32_t baseLevel=0,maxLevel=1000;
};
struct ManagedTexturePatch {
    std::uint32_t level=0,layer=0,x=0,y=0,z=0,width=0,height=0,depth=1,layerCount=1;
    std::vector<std::byte> pixels;
};
static std::uint32_t managedTextureTarget(std::uint32_t target){return target>=0x8515&&target<=0x851a?0x8513:target;}
static VkSamplerAddressMode managedTextureWrap(double value){
    if(value==0x2901)return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if(value==0x812f)return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if(value==0x8370)return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    throw std::invalid_argument("Unsupported native GL texture wrapping token");
}
static TextureConfig managedTextureFormat(std::uint32_t format){
    TextureConfig c;
    switch(format){
        case 0x1903:case 0x8229:c.components=1;c.componentBytes=1;c.format=VK_FORMAT_R8_UNORM;c.type=UnsignedByteType;break;
        case 0x8227:case 0x822b:c.components=2;c.componentBytes=1;c.format=VK_FORMAT_R8G8_UNORM;c.type=UnsignedByteType;break;
        case 0x1907:case 0x8051:c.components=3;c.componentBytes=1;c.format=VK_FORMAT_R8G8B8_UNORM;c.type=UnsignedByteType;break;
        case 0x1908:case 0x8058:c.components=4;c.componentBytes=1;c.format=VK_FORMAT_R8G8B8A8_UNORM;c.type=UnsignedByteType;break;
        case 0x8c41:c.components=3;c.componentBytes=1;c.format=VK_FORMAT_R8G8B8_SRGB;c.type=UnsignedByteType;break;
        case 0x8c43:c.components=4;c.componentBytes=1;c.format=VK_FORMAT_R8G8B8A8_SRGB;c.type=UnsignedByteType;break;
        case 0x822d:c.components=1;c.componentBytes=2;c.format=VK_FORMAT_R16_SFLOAT;c.type=HalfFloatType;break;
        case 0x822f:c.components=2;c.componentBytes=2;c.format=VK_FORMAT_R16G16_SFLOAT;c.type=HalfFloatType;break;
        case 0x881b:c.components=3;c.componentBytes=2;c.format=VK_FORMAT_R16G16B16_SFLOAT;c.type=HalfFloatType;break;
        case 0x881a:c.components=4;c.componentBytes=2;c.format=VK_FORMAT_R16G16B16A16_SFLOAT;c.type=HalfFloatType;break;
        case 0x822e:c.components=1;c.componentBytes=4;c.format=VK_FORMAT_R32_SFLOAT;c.type=FloatType;break;
        case 0x8230:c.components=2;c.componentBytes=4;c.format=VK_FORMAT_R32G32_SFLOAT;c.type=FloatType;break;
        case 0x8815:c.components=3;c.componentBytes=4;c.format=VK_FORMAT_R32G32B32_SFLOAT;c.type=FloatType;break;
        case 0x8814:c.components=4;c.componentBytes=4;c.format=VK_FORMAT_R32G32B32A32_SFLOAT;c.type=FloatType;break;
        default:throw std::invalid_argument("Native Vulkan GL texture internalFormat is not implemented: "+std::to_string(format));
    }
    return c;
}
void validateManagedTexture(const TextureConfig& c,const ManagedTextureShape& shape,bool generate)const{
    if(!c.width||!c.height||!c.levels||!shape.depth)throw std::invalid_argument("Native Vulkan texture storage requires positive dimensions");
    VkFormatProperties p{};api.vkGetPhysicalDeviceFormatProperties(physicalDevice,c.format,&p);
    VkFormatFeatureFlags required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if(c.mag==VK_FILTER_LINEAR||c.min==VK_FILTER_LINEAR||c.mipmapMode==VK_SAMPLER_MIPMAP_MODE_LINEAR||generate)required|=VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    if(generate&&c.levels>1)required|=VK_FORMAT_FEATURE_BLIT_SRC_BIT|VK_FORMAT_FEATURE_BLIT_DST_BIT;
    if((p.optimalTilingFeatures&required)!=required)throw std::invalid_argument("Native GL texture format/filter/mipmap exceeds actual Vulkan capabilities");
    const auto flags=shape.target==0x8513?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:VkImageCreateFlags(0);
    VkImageFormatProperties limits{};
    const auto result=api.vkGetPhysicalDeviceImageFormatProperties(physicalDevice,c.format,shape.type(),VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,flags,&limits);
    if(result==VK_ERROR_FORMAT_NOT_SUPPORTED)throw std::invalid_argument("Native GL texture image shape is not supported by this Vulkan device");
    check(result,"vkGetPhysicalDeviceImageFormatProperties(managed texture)");
    if(c.width>limits.maxExtent.width||c.height>limits.maxExtent.height||shape.mipDepth(0)>limits.maxExtent.depth||shape.layers()>limits.maxArrayLayers||c.levels>limits.maxMipLevels)
        throw std::invalid_argument("Native GL texture dimensions exceed actual Vulkan image format limits");
    if(c.anisotropy>1&&(!samplerAnisotropy||c.anisotropy>maxAnisotropy))throw std::invalid_argument("Native GL texture anisotropy exceeds actual Vulkan capabilities");
}
TextureRef createManagedTextureResource(const TextureConfig& config,const ManagedTextureShape& shape,
    const ManagedTextureSampler& settings,const TextureRef& previous,const std::vector<ManagedTexturePatch>& patches,
    const std::vector<VkImageSubresourceRange>& clears={},bool generate=false){
    validateManagedTexture(config,shape,generate);
    auto result=TextureRef(new TextureResource{},[this](TextureResource* texture){
        if(texture->sampler)api.vkDestroySampler(device,texture->sampler,nullptr);
        destroyImage(texture->image);delete texture;--info.textureImages;
    });++info.textureImages;result->config=config;
    auto& image=result->image;
    VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    create.flags=shape.target==0x8513?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
    create.imageType=shape.type();create.format=config.format;create.extent={config.width,config.height,shape.mipDepth(0)};
    create.mipLevels=config.levels;create.arrayLayers=shape.layers();create.samples=VK_SAMPLE_COUNT_1_BIT;
    create.tiling=VK_IMAGE_TILING_OPTIMAL;create.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    check(api.vkCreateImage(device,&create,nullptr,&image.image),"vkCreateImage(managed texture)");
    VkMemoryRequirements requirements{};api.vkGetImageMemoryRequirements(device,image.image,&requirements);
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;
    allocation.memoryTypeIndex=memoryType(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    check(api.vkAllocateMemory(device,&allocation,nullptr,&image.memory),"vkAllocateMemory(managed texture)");
    check(api.vkBindImageMemory(device,image.image,image.memory,0),"vkBindImageMemory(managed texture)");
    const auto baseLevel=std::min(settings.baseLevel,config.levels-1);
    const auto lastLevel=std::max(baseLevel,std::min(settings.maxLevel,config.levels-1));
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=image.image;view.viewType=shape.view();view.format=config.format;
    view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,baseLevel,lastLevel-baseLevel+1,0,shape.layers()};
    check(api.vkCreateImageView(device,&view,nullptr,&image.view),"vkCreateImageView(managed texture)");
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sampler.magFilter=config.mag;sampler.minFilter=config.min;sampler.mipmapMode=config.mipmapMode;
    sampler.addressModeU=config.wrapS;sampler.addressModeV=config.wrapT;sampler.addressModeW=settings.wrapR;
    sampler.anisotropyEnable=config.anisotropy>1;sampler.maxAnisotropy=config.anisotropy;
    sampler.maxLod=config.mipFilter?std::max(0.f,std::min(float(lastLevel-baseLevel),settings.maxLod)):0.f;
    sampler.minLod=std::min(sampler.maxLod,std::max(0.f,settings.minLod));
    check(api.vkCreateSampler(device,&sampler,nullptr,&result->sampler),"vkCreateSampler(managed texture)");
    std::vector<std::byte> bytes;std::vector<VkBufferImageCopy> copies;
    const auto pixelBytes=config.components*config.componentBytes;
    const std::size_t alignment=pixelBytes%4==0?pixelBytes:pixelBytes*4;
    for(const auto& patch:patches){
        bytes.resize((bytes.size()+alignment-1)/alignment*alignment);
        VkBufferImageCopy copy{};copy.bufferOffset=bytes.size();copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,patch.level,patch.layer,patch.layerCount};
        copy.imageOffset={int(patch.x),int(patch.y),int(patch.z)};copy.imageExtent={patch.width,patch.height,patch.depth};copies.push_back(copy);
        bytes.insert(bytes.end(),patch.pixels.begin(),patch.pixels.end());
    }
    const auto staging=bytes.empty()?BufferRef{}:sharedBuffer(bytes.size(),VK_BUFFER_USAGE_TRANSFER_SRC_BIT,bytes.data());
    // This independent command buffer submits synchronously. All resources are
    // new images, and previous images are restored to SHADER_READ_ONLY before
    // returning. Main-pass recording and descriptor pools remain untouched.
    check(api.vkResetCommandBuffer(textureCommandBuffer,0),"vkResetCommandBuffer(managed texture)");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(api.vkBeginCommandBuffer(textureCommandBuffer,&begin),"vkBeginCommandBuffer(managed texture)");
    const auto barrier=[&](VkImage target,std::uint32_t first,std::uint32_t levels,VkImageLayout oldLayout,VkImageLayout newLayout,
        VkAccessFlags source,VkAccessFlags destination,VkPipelineStageFlags sourceStage,VkPipelineStageFlags destinationStage){
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};b.srcAccessMask=source;b.dstAccessMask=destination;b.oldLayout=oldLayout;b.newLayout=newLayout;
        b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.image=target;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,first,levels,0,shape.layers()};
        api.vkCmdPipelineBarrier(textureCommandBuffer,sourceStage,destinationStage,0,0,nullptr,0,nullptr,1,&b);
    };
    barrier(image.image,0,config.levels,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,0,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    const VkClearColorValue zero{};const VkImageSubresourceRange all{VK_IMAGE_ASPECT_COLOR_BIT,0,config.levels,0,shape.layers()};
    api.vkCmdClearColorImage(textureCommandBuffer,image.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&zero,1,&all);
    barrier(image.image,0,config.levels,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    if(previous){
        barrier(previous->image.image,0,previous->config.levels,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
        std::vector<VkImageCopy> regions;
        for(std::uint32_t level=0;level<std::min(config.levels,previous->config.levels);++level){VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level,0,shape.layers()};copy.extent={std::max(1u,config.width>>level),std::max(1u,config.height>>level),shape.mipDepth(level)};regions.push_back(copy);}
        api.vkCmdCopyImage(textureCommandBuffer,previous->image.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,image.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,std::uint32_t(regions.size()),regions.data());
        barrier(previous->image.image,0,previous->config.levels,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        barrier(image.image,0,config.levels,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    }
    for(const auto& clear:clears)api.vkCmdClearColorImage(textureCommandBuffer,image.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&zero,1,&clear);
    if(!clears.empty())barrier(image.image,0,config.levels,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
    if(staging)api.vkCmdCopyBufferToImage(textureCommandBuffer,staging->buffer,image.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,std::uint32_t(copies.size()),copies.data());
    for(std::uint32_t level=1;generate&&level<config.levels;++level){
        barrier(image.image,level-1,1,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageBlit blit{};blit.srcSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level-1,0,shape.layers()};blit.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,level,0,shape.layers()};
        blit.srcOffsets[1]={int(std::max(1u,config.width>>(level-1))),int(std::max(1u,config.height>>(level-1))),int(shape.mipDepth(level-1))};
        blit.dstOffsets[1]={int(std::max(1u,config.width>>level)),int(std::max(1u,config.height>>level)),int(shape.mipDepth(level))};
        api.vkCmdBlitImage(textureCommandBuffer,image.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,image.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,VK_FILTER_LINEAR);
        barrier(image.image,level-1,1,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    barrier(image.image,generate?config.levels-1:0,generate?1:config.levels,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    check(api.vkEndCommandBuffer(textureCommandBuffer),"vkEndCommandBuffer(managed texture)");
    check(api.vkResetFences(device,1,&textureFence),"vkResetFences(managed texture)");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&textureCommandBuffer;
    check(api.vkQueueSubmit(queue,1,&submit,textureFence),"vkQueueSubmit(managed texture)");
    check(api.vkWaitForFences(device,1,&textureFence,VK_TRUE,UINT64_MAX),"vkWaitForFences(managed texture)");return result;
}

struct ManagedTextureStorage final:VulkanTextureCommands::Storage {
    std::weak_ptr<Lifecycle> lifecycle;
    TextureRef resource;
    TextureConfig config;
    ManagedTextureShape shape;
    ManagedTextureSampler sampler;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> defined;
    bool immutable=false;
    explicit ManagedTextureStorage(const std::shared_ptr<Lifecycle>& token):lifecycle(token){
        // WebGL object defaults. WebGLTextures explicitly supplies the texture's
        // own sampler parameters before allocating/uploading its first image.
        config.mag=VK_FILTER_LINEAR;config.min=VK_FILTER_NEAREST;config.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
        config.mipFilter=true;config.wrapS=config.wrapT=VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    Impl& owner()const{const auto token=lifecycle.lock();if(!token||!token->owner||!token->owner->device)throw JSTypeError("Native Vulkan texture storage has been retired");return *token->owner;}
    void define(std::uint32_t layer,std::uint32_t level){const auto key=std::pair{layer,level};if(std::find(defined.begin(),defined.end(),key)==defined.end())defined.push_back(key);}
    bool isDefined(std::uint32_t layer,std::uint32_t level)const{return std::find(defined.begin(),defined.end(),std::pair{layer,level})!=defined.end();}
    void allocate(const VulkanTextureCommands::Allocation& a)override{
        auto& renderer=owner();auto next=*this;
        const auto target=managedTextureTarget(a.target);
        if(target!=0x0de1&&target!=0x8513&&target!=0x806f&&target!=0x8c1a)throw std::invalid_argument("Unsupported native Vulkan texture target");
        if(a.level<0||a.level>=31||a.width<=0||a.height<=0||a.depth<=0||a.levels<=0)throw std::invalid_argument("Unsupported empty/invalid native Vulkan texture allocation");
        const auto format=managedTextureFormat(a.internalFormat);
        auto width=std::uint64_t(a.width)<<a.level,height=std::uint64_t(a.height)<<a.level;
        if(width>UINT32_MAX||height>UINT32_MAX)throw std::invalid_argument("Native Vulkan texture base dimensions overflow");
        next.shape={target,std::uint32_t(a.depth)};
        if(target==0x806f){const auto depth=std::uint64_t(a.depth)<<a.level;if(depth>UINT32_MAX)throw std::invalid_argument("Native Vulkan texture depth overflows");next.shape.depth=std::uint32_t(depth);}
        // floor(base / 2^level) cannot recover an odd base by shifting back.
        // Preserve the existing physical base whenever this image matches its
        // actual mip extent (3x3 -> 1x1 must keep the 3x3 level-zero storage).
        if(resource&&shape.target==target&&a.level>0){
            const bool matching=config.format==format.format&&std::uint32_t(a.width)==std::max(1u,config.width>>a.level)&&
                std::uint32_t(a.height)==std::max(1u,config.height>>a.level)&&
                (target==0x806f?std::uint32_t(a.depth)==shape.mipDepth(std::uint32_t(a.level)):std::uint32_t(a.depth)==shape.depth);
            if(!matching)throw std::invalid_argument("Native Vulkan mutable mip allocation with inconsistent base dimensions is not implemented");
            width=config.width;height=config.height;next.shape.depth=shape.depth;
        }
        const bool compatible=resource&&shape.target==target&&shape.depth==next.shape.depth&&config.width==width&&config.height==height&&config.format==format.format;
        next.config.width=std::uint32_t(width);next.config.height=std::uint32_t(height);next.config.components=format.components;next.config.componentBytes=format.componentBytes;next.config.format=format.format;next.config.type=format.type;
        next.config.levels=a.immutable?std::uint32_t(a.levels):std::max(compatible?config.levels:1u,std::uint32_t(a.level+1));next.immutable=a.immutable;
        if(!compatible||a.immutable)next.defined.clear();
        std::vector<VkImageSubresourceRange> clears;
        if(a.immutable){for(std::uint32_t layer=0;layer<next.shape.layers();++layer)for(std::uint32_t level=0;level<next.config.levels;++level)next.define(layer,level);}
        else{
            const auto layer=target==0x8513?a.target-0x8515:0u;const auto count=target==0x8c1a?next.shape.layers():1u;
            for(std::uint32_t i=0;i<count;++i)next.define(layer+i,std::uint32_t(a.level));
            clears.push_back({VK_IMAGE_ASPECT_COLOR_BIT,std::uint32_t(a.level),1,layer,count});
        }
        next.resource=renderer.createManagedTextureResource(next.config,next.shape,next.sampler,compatible&&!a.immutable?resource:TextureRef{}, {},clears);
        *this=std::move(next);
    }
    void upload(const VulkanTextureCommands::Upload& u)override{
        auto& renderer=owner();if(!resource)throw std::logic_error("Native Vulkan texture upload has no allocated storage");
        if(u.compressed)throw std::invalid_argument("Compressed native Vulkan texture upload is not implemented");
        const auto target=managedTextureTarget(u.target);if(target!=shape.target)throw std::invalid_argument("Native Vulkan texture upload target does not match storage");
        const auto components=u.format==0x1903?1u:u.format==0x8227?2u:u.format==0x1907?3u:u.format==0x1908?4u:0u;
        const auto type=u.type==0x1401?UnsignedByteType:u.type==0x140b?HalfFloatType:u.type==0x1406?FloatType:-1;
        if(components!=config.components||type!=config.type)throw std::invalid_argument("Native Vulkan texture upload format/type conversion is not implemented");
        if(u.nullData||!u.width||!u.height||!u.depth)return;
        const std::size_t pixelBytes=std::size_t(config.components)*config.componentBytes;
        const auto& p=u.unpack;
        const std::size_t rowLength=p.rowLength?std::size_t(p.rowLength):std::size_t(u.width);
        const std::size_t sourceRow=(rowLength*pixelBytes+std::size_t(p.alignment)-1)/std::size_t(p.alignment)*std::size_t(p.alignment);
        const std::size_t imageHeight=p.imageHeight?std::size_t(p.imageHeight):std::size_t(u.height);
        const std::size_t sourceImage=sourceRow*imageHeight;
        const bool layered=shape.target==0x806f||shape.target==0x8c1a;
        const std::size_t first=std::size_t(p.skipRows)*sourceRow+std::size_t(p.skipPixels)*pixelBytes+(layered?std::size_t(p.skipImages)*sourceImage:0);
        const std::size_t rowBytes=std::size_t(u.width)*pixelBytes;
        if(std::size_t(u.height)>SIZE_MAX/std::size_t(u.depth)||std::size_t(u.height)*std::size_t(u.depth)>SIZE_MAX/rowBytes)throw std::invalid_argument("Native Vulkan texture upload byte count overflows");
        ManagedTexturePatch patch;patch.level=std::uint32_t(u.level);patch.x=std::uint32_t(u.x);patch.y=std::uint32_t(u.y);patch.width=std::uint32_t(u.width);patch.height=std::uint32_t(u.height);
        patch.layer=shape.target==0x8513?u.target-0x8515:shape.target==0x8c1a?std::uint32_t(u.z):0u;
        patch.z=shape.target==0x806f?std::uint32_t(u.z):0u;patch.depth=shape.target==0x806f?std::uint32_t(u.depth):1u;patch.layerCount=shape.target==0x8c1a?std::uint32_t(u.depth):1u;
        patch.pixels.resize(rowBytes*std::size_t(u.height)*std::size_t(u.depth));
        for(int z=0;z<u.depth;++z)for(int row=0;row<u.height;++row){const auto sourceRowIndex=p.flipY?u.height-row-1:row;
            const auto offset=first+std::size_t(z)*sourceImage+std::size_t(sourceRowIndex)*sourceRow;
            if(offset>u.pixels.size()||rowBytes>u.pixels.size()-offset)throw std::invalid_argument("Native Vulkan texture upload exceeds the actual pixel view");
            std::memcpy(patch.pixels.data()+(std::size_t(z)*std::size_t(u.height)+std::size_t(row))*rowBytes,u.pixels.data()+offset,rowBytes);
        }
        if(p.premultiplyAlpha&&components==4)for(std::size_t offset=0;offset<patch.pixels.size();offset+=pixelBytes){
            if(type==UnsignedByteType){const float alpha=float(std::to_integer<unsigned>(patch.pixels[offset+3]))/255.f;for(unsigned c=0;c<3;++c)patch.pixels[offset+c]=std::byte(std::uint8_t(float(std::to_integer<unsigned>(patch.pixels[offset+c]))*alpha));}
            else if(type==FloatType){float pixel[4];std::memcpy(pixel,patch.pixels.data()+offset,sizeof(pixel));for(unsigned c=0;c<3;++c)pixel[c]*=pixel[3];std::memcpy(patch.pixels.data()+offset,pixel,sizeof(pixel));}
            else{std::uint16_t pixel[4];std::memcpy(pixel,patch.pixels.data()+offset,sizeof(pixel));for(unsigned c=0;c<3;++c)pixel[c]=textureHalfFloat(float(fromHalfFloat(pixel[c]))*float(fromHalfFloat(pixel[3])));std::memcpy(patch.pixels.data()+offset,pixel,sizeof(pixel));}
        }
        resource=renderer.createManagedTextureResource(config,shape,sampler,resource,{patch});++renderer.info.textureUploads;
    }
    void parameter(std::uint32_t name,double value)override{
        auto& renderer=owner();auto next=*this;
        switch(name){
            case 0x2800:next.config.mag=value==0x2601?VK_FILTER_LINEAR:VK_FILTER_NEAREST;break;
            case 0x2801:next.config.mipFilter=value>=0x2700;next.config.min=value==0x2601||value==0x2701||value==0x2703?VK_FILTER_LINEAR:VK_FILTER_NEAREST;next.config.mipmapMode=value==0x2702||value==0x2703?VK_SAMPLER_MIPMAP_MODE_LINEAR:VK_SAMPLER_MIPMAP_MODE_NEAREST;break;
            case 0x2802:next.config.wrapS=managedTextureWrap(value);break;case 0x2803:next.config.wrapT=managedTextureWrap(value);break;case 0x8072:next.sampler.wrapR=managedTextureWrap(value);break;
            case 0x84fe:next.config.anisotropy=float(value);break;case 0x813a:next.sampler.minLod=float(value);break;case 0x813b:next.sampler.maxLod=float(value);break;
            case 0x813c:next.sampler.baseLevel=std::uint32_t(value);break;case 0x813d:next.sampler.maxLevel=std::uint32_t(value);break;
            case 0x884c:if(value!=0)throw std::invalid_argument("Native Vulkan shadow texture comparison requires depth storage");break;
            case 0x884d:break; // A disabled comparison function cannot affect color sampling.
            default:throw std::invalid_argument("Native Vulkan texture parameter is not implemented");
        }
        if(next.sampler.minLod>next.sampler.maxLod)throw std::invalid_argument("Native Vulkan inverted texture LOD clamps are not implemented");
        if(resource)next.resource=renderer.createManagedTextureResource(next.config,next.shape,next.sampler,resource,{});
        *this=std::move(next);
    }
    void generateMipmap(std::uint32_t target)override{
        auto& renderer=owner();if(!resource||target!=shape.target)throw std::invalid_argument("Native Vulkan mipmap generation has no matching texture storage");
        if(sampler.baseLevel!=0)throw std::invalid_argument("Native Vulkan mipmap generation with a nonzero base level is not implemented");
        for(std::uint32_t layer=0;layer<shape.layers();++layer)if(!isDefined(layer,0))throw std::invalid_argument("Native Vulkan mipmap generation requires every base image");
        auto next=*this;
        if(!immutable)next.config.levels=std::uint32_t(std::floor(std::log2(std::max(config.width,std::max(config.height,shape.mipDepth(0))))))+1;
        next.resource=renderer.createManagedTextureResource(next.config,shape,sampler,resource,{}, {},true);
        for(std::uint32_t layer=0;layer<shape.layers();++layer)for(std::uint32_t level=0;level<next.config.levels;++level)next.define(layer,level);
        *this=std::move(next);
    }
    TextureRef sampled()const{
        static_cast<void>(owner());if(!resource)throw std::invalid_argument("Native Vulkan sampling of unallocated GL texture storage is not implemented");
        if(sampler.baseLevel>=config.levels||sampler.maxLevel<sampler.baseLevel)throw std::invalid_argument("Native Vulkan sampling of an incomplete texture mip range is not implemented");
        const auto last=immutable?config.levels-1:std::min(sampler.maxLevel,std::uint32_t(std::floor(std::log2(std::max(config.width,std::max(config.height,shape.mipDepth(0)))))));
        const auto requiredLast=config.mipFilter?last:sampler.baseLevel;
        for(std::uint32_t layer=0;layer<shape.layers();++layer)for(std::uint32_t level=sampler.baseLevel;level<=requiredLast;++level)
            if(!isDefined(layer,level))throw std::invalid_argument("Native Vulkan sampling of incomplete GL texture images is not implemented");
        return resource;
    }
};
void initializeManagedTextureCommands(){
    if(managedTextureCommands)return;
    VkPhysicalDeviceProperties properties{};api.vkGetPhysicalDeviceProperties(physicalDevice,&properties);
    VulkanTextureCommands::Limits limits;
    limits.maxTextureUnits=int(std::min({properties.limits.maxPerStageDescriptorSamplers,properties.limits.maxPerStageDescriptorSampledImages,
        properties.limits.maxDescriptorSetSamplers,properties.limits.maxDescriptorSetSampledImages,std::uint32_t(INT32_MAX)}));
    limits.maxTextureSize=int(std::min(properties.limits.maxImageDimension2D,std::uint32_t(INT32_MAX)));
    limits.maxCubeMapSize=int(std::min(properties.limits.maxImageDimensionCube,std::uint32_t(INT32_MAX)));
    limits.max3DTextureSize=int(std::min(properties.limits.maxImageDimension3D,std::uint32_t(INT32_MAX)));
    limits.maxArrayLayers=int(std::min(properties.limits.maxImageArrayLayers,std::uint32_t(INT32_MAX)));limits.maxSamples=1;
    const std::weak_ptr<Lifecycle> weak=lifecycle;
    managedTextureCommands=std::make_unique<VulkanTextureCommands>([weak]()->std::shared_ptr<VulkanTextureCommands::Storage>{
        const auto token=weak.lock();if(!token||!token->owner)throw JSTypeError("Native Vulkan texture command owner has been retired");
        return std::make_shared<ManagedTextureStorage>(token);
    },VulkanTextureCommands::FramebufferFactory{},limits);
}
TextureRef managedSampledTexture(std::uint32_t unit,std::uint32_t target){
    if(!managedTextureCommands)throw std::logic_error("Native Vulkan texture commands have not been initialized");
    const auto handle=managedTextureCommands->boundTexture(unit,target);
    if(handle.isNull())throw std::invalid_argument("Native Vulkan draw has no actual GL texture bound at its sampler unit");
    const auto storage=std::dynamic_pointer_cast<ManagedTextureStorage>(managedTextureCommands->storage(handle));
    if(!storage||storage->shape.target!=target)throw std::logic_error("Native Vulkan sampler does not reference its own texture storage");
    return storage->sampled();
}
