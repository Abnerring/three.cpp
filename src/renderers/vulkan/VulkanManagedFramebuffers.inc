// Device attachment storage. Source target/texture identities stay in the
// original WebGLRenderer/WebGLTextures; only native command handles reach here.
struct ManagedRenderbufferImage {
    Image image;
    VkFormat format=VK_FORMAT_UNDEFINED;
    VkImageAspectFlags aspect=VK_IMAGE_ASPECT_DEPTH_BIT;
    std::uint32_t width=0,height=0;
    bool initialized=false;
};
struct ManagedRenderbufferStorage final:VulkanTextureCommands::RenderbufferStorage {
    std::weak_ptr<Lifecycle> lifecycle;
    std::shared_ptr<ManagedRenderbufferImage> image;
    explicit ManagedRenderbufferStorage(const std::shared_ptr<Lifecycle>& token):lifecycle(token){}
    Impl& owner()const{const auto token=lifecycle.lock();if(!token||!token->owner)throw JSTypeError("Native renderbuffer owner has been retired");return *token->owner;}
    void allocate(std::uint32_t internalFormat,int width,int height,int samples)override{
        auto& r=owner();if(width<=0||height<=0)throw std::invalid_argument("Empty Vulkan renderbuffer storage is not implemented");
        if(samples>1)throw std::logic_error("Native multisampled renderbuffer storage is not implemented");
        VkFormat format;VkImageAspectFlags aspect=VK_IMAGE_ASPECT_DEPTH_BIT;
        switch(internalFormat){
        case 0x81a5:format=VK_FORMAT_D16_UNORM;break;
        case 0x81a6:format=VK_FORMAT_X8_D24_UNORM_PACK32;break;
        case 0x8cac:format=VK_FORMAT_D32_SFLOAT;break;
        case 0x88f0:format=VK_FORMAT_D24_UNORM_S8_UINT;aspect|=VK_IMAGE_ASPECT_STENCIL_BIT;break;
        case 0x8cad:format=VK_FORMAT_D32_SFLOAT_S8_UINT;aspect|=VK_IMAGE_ASPECT_STENCIL_BIT;break;
        default:throw std::logic_error("Native color/stencil-only renderbuffer storage is not implemented");
        }
        VkFormatProperties properties{};r.api.vkGetPhysicalDeviceFormatProperties(r.physicalDevice,format,&properties);
        if(!(properties.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
            throw std::invalid_argument("Vulkan device does not support the requested depth renderbuffer format");
        if(r.recording)r.endFrame();
        auto replacement=std::shared_ptr<ManagedRenderbufferImage>(new ManagedRenderbufferImage{},[&r](auto* value){r.destroyImage(value->image);delete value;});
        replacement->format=format;replacement->aspect=aspect;replacement->width=std::uint32_t(width);replacement->height=std::uint32_t(height);
        r.createImage(replacement->image,format,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,aspect,1,replacement->width,replacement->height);
        image=std::move(replacement);
    }
};
struct ManagedFramebufferStorage final:VulkanTextureCommands::FramebufferStorage {
    struct Attachment {std::shared_ptr<ManagedTextureStorage> texture;std::shared_ptr<ManagedRenderbufferStorage> renderbuffer;std::uint32_t target=0;int level=0,layer=0;};
    std::weak_ptr<Lifecycle> lifecycle;
    std::map<std::uint32_t,Attachment> attachments;
    std::shared_ptr<GpuTarget> cached;
    TextureRef cachedColor,cachedDepthTexture;
    std::shared_ptr<ManagedRenderbufferImage> cachedDepth;
    explicit ManagedFramebufferStorage(const std::shared_ptr<Lifecycle>& token):lifecycle(token){}
    Impl& owner()const{const auto token=lifecycle.lock();if(!token||!token->owner)throw JSTypeError("Native framebuffer owner has been retired");return *token->owner;}
    void texture(std::uint32_t attachment,const std::shared_ptr<VulkanTextureCommands::Storage>& value,std::uint32_t target,int level,int layer)override{
        Attachment next;next.target=target;next.level=level;next.layer=layer;
        if(value){next.texture=std::dynamic_pointer_cast<ManagedTextureStorage>(value);if(!next.texture)throw JSTypeError("Foreign Vulkan framebuffer texture");}
        for(const auto point:attachment==0x821a?std::vector<std::uint32_t>{0x8d00,0x8d20}:std::vector<std::uint32_t>{attachment})
            if(value)attachments[point]=next;else attachments.erase(point);
        cached.reset();cachedColor.reset();cachedDepth.reset();cachedDepthTexture.reset();
    }
    void renderbuffer(std::uint32_t attachment,const std::shared_ptr<VulkanTextureCommands::RenderbufferStorage>& value)override{
        Attachment next;if(value){next.renderbuffer=std::dynamic_pointer_cast<ManagedRenderbufferStorage>(value);if(!next.renderbuffer)throw JSTypeError("Foreign Vulkan framebuffer renderbuffer");}
        for(const auto point:attachment==0x821a?std::vector<std::uint32_t>{0x8d00,0x8d20}:std::vector<std::uint32_t>{attachment})
            if(value)attachments[point]=next;else attachments.erase(point);
        cached.reset();cachedColor.reset();cachedDepth.reset();cachedDepthTexture.reset();
    }
    const Attachment* color()const{const auto found=attachments.find(0x8ce0);return found==attachments.end()?nullptr:&found->second;}
    const Attachment* depth()const{for(const auto point:{0x8d00u,0x821au})if(const auto found=attachments.find(point);found!=attachments.end())return &found->second;return nullptr;}
    std::uint32_t status(std::uint32_t)override{
        if(attachments.empty())return 0x8cd7;
        auto& renderer=owner();
        const auto mipExtent=[](std::uint32_t size,std::uint32_t level){return level<32?std::max(1u,size>>level):1u;};
        const auto formatAspect=[](VkFormat format)->VkImageAspectFlags{
            switch(format){
            case VK_FORMAT_UNDEFINED:return 0;
            case VK_FORMAT_D16_UNORM:case VK_FORMAT_X8_D24_UNORM_PACK32:case VK_FORMAT_D32_SFLOAT:return VK_IMAGE_ASPECT_DEPTH_BIT;
            case VK_FORMAT_S8_UINT:return VK_IMAGE_ASPECT_STENCIL_BIT;
            case VK_FORMAT_D16_UNORM_S8_UINT:case VK_FORMAT_D24_UNORM_S8_UINT:case VK_FORMAT_D32_SFLOAT_S8_UINT:return VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT;
            default:return VK_IMAGE_ASPECT_COLOR_BIT;
            }
        };
        struct AttachedImage {const void* storage=nullptr;bool texture=false;std::uint32_t level=0,layer=0;};
        const auto sameImage=[](const AttachedImage& a,const AttachedImage& b){return a.storage==b.storage&&a.texture==b.texture&&a.level==b.level&&a.layer==b.layer;};
        AttachedImage depthImage,stencilImage;std::vector<AttachedImage> colorImages;
        std::uint32_t width=0,height=0;
        for(const auto& [point,attachment]:attachments){
            AttachedImage image;std::uint32_t imageWidth=0,imageHeight=0;VkFormat format=VK_FORMAT_UNDEFINED;
            if(attachment.texture){
                const auto& t=*attachment.texture;
                if(!t.resource||!t.resource->image.image||!t.config.width||!t.config.height||!t.shape.depth||attachment.level<0||std::uint32_t(attachment.level)>=t.config.levels)return 0x8cd6;
                const auto level=std::uint32_t(attachment.level);std::uint32_t definedLayer=0,layer=0;
                switch(t.shape.target){
                case 0x0de1:if(attachment.target!=0x0de1)return 0x8cd6;break;
                case 0x8513:
                    if(attachment.target<0x8515||attachment.target>0x851a)return 0x8cd6;
                    definedLayer=layer=attachment.target-0x8515;break;
                case 0x8c1a:
                    if(attachment.target!=0x8c1a||attachment.layer<0||std::uint32_t(attachment.layer)>=t.shape.layers())return 0x8cd6;
                    definedLayer=layer=std::uint32_t(attachment.layer);break;
                case 0x806f:
                    // A 3D mip is one defined volume; its selected z slice must
                    // fit that mip's depth, rather than the image's array layers.
                    if(attachment.target!=0x806f||attachment.layer<0||std::uint32_t(attachment.layer)>=mipExtent(t.shape.depth,level))return 0x8cd6;
                    layer=std::uint32_t(attachment.layer);break;
                default:return 0x8cd6;
                }
                if(!t.isDefined(definedLayer,level))return 0x8cd6;
                if(!t.immutable){
                    // ES 3.0.6 4.4.4.1: mutable non-base attachments require
                    // the complete mip chain, independently of MIN_FILTER.
                    const auto base=t.sampler.baseLevel;
                    if(base>=t.config.levels||t.sampler.maxLevel<base)return 0x8cd6;
                    auto size=std::max(mipExtent(t.config.width,base),mipExtent(t.config.height,base));
                    if(t.shape.target==0x806f)size=std::max(size,mipExtent(t.shape.depth,base));
                    auto last=base;while(size>1){size>>=1;++last;}last=std::min(last,t.sampler.maxLevel);
                    if(level<base||level>last)return 0x8cd6;
                    if(level!=base){
                        if(last>=t.config.levels||(t.shape.target==0x8513&&mipExtent(t.config.width,base)!=mipExtent(t.config.height,base)))return 0x8cd6;
                        for(std::uint32_t face=0;face<t.shape.layers();++face)for(auto mip=base;mip<=last;++mip)
                            if(!t.isDefined(face,mip))return 0x8cd6;
                    }
                }
                image={attachment.texture.get(),true,level,layer};format=t.config.format;
                imageWidth=mipExtent(t.config.width,level);imageHeight=mipExtent(t.config.height,level);
            }else if(attachment.renderbuffer&&attachment.renderbuffer->image){
                const auto& r=*attachment.renderbuffer->image;if(!r.image.image||!r.width||!r.height)return 0x8cd6;
                image={attachment.renderbuffer.get(),false,0,0};format=r.format;imageWidth=r.width;imageHeight=r.height;
            }else return 0x8cd6;
            const auto aspect=formatAspect(format);VkFormatFeatureFlags required=0;
            if(point>=0x8ce0&&point<0x8d00){
                if(!(aspect&VK_IMAGE_ASPECT_COLOR_BIT))return 0x8cd6;
                // These sized texture formats are not color-renderable in
                // WebGL 2, even when Vulkan can attach the corresponding image.
                if(format==VK_FORMAT_R8G8B8_SRGB||format==VK_FORMAT_R16G16B16_SFLOAT||format==VK_FORMAT_R32G32B32_SFLOAT)return 0x8cd6;
                // Sampling alone does not enable floating color attachments.
                if(!renderer.managedFloatColorEnabled&&(format==VK_FORMAT_R16_SFLOAT||format==VK_FORMAT_R16G16_SFLOAT||format==VK_FORMAT_R16G16B16A16_SFLOAT||
                   format==VK_FORMAT_R32_SFLOAT||format==VK_FORMAT_R32G32_SFLOAT||format==VK_FORMAT_R32G32B32A32_SFLOAT||format==VK_FORMAT_B10G11R11_UFLOAT_PACK32))return 0x8cd6;
                required=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
                for(const auto& other:colorImages)if(sameImage(other,image))return 0x8cdd;
                colorImages.push_back(image);
            }else{
                const auto requiredAspect=point==0x8d00?VK_IMAGE_ASPECT_DEPTH_BIT:point==0x8d20?VK_IMAGE_ASPECT_STENCIL_BIT:point==0x821a?VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT:0;
                if(!requiredAspect||(aspect&requiredAspect)!=requiredAspect)return 0x8cd6;
                required=VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
                if(point==0x8d00||point==0x821a)depthImage=image;
                if(point==0x8d20||point==0x821a)stencilImage=image;
            }
            VkFormatProperties properties{};renderer.api.vkGetPhysicalDeviceFormatProperties(renderer.physicalDevice,format,&properties);
            if(!(properties.optimalTilingFeatures&required))return 0x8cd6;
            // framebufferTextureLayer selects a two-dimensional image. The
            // containing 3D depth/array size is not an attachment dimension.
            if(width&&(width!=imageWidth||height!=imageHeight))return 0x8cd9;
            width=imageWidth;height=imageHeight;
        }
        if(depthImage.storage&&stencilImage.storage&&!sameImage(depthImage,stencilImage))return 0x8cdd;
        return 0x8cd5;
    }
    std::shared_ptr<GpuTarget> target(){
        auto& r=owner();if(status(0x8d40)!=0x8cd5)throw std::invalid_argument("Native Vulkan framebuffer is incomplete");
        if(attachments.contains(0x8d20))throw std::logic_error("Native Vulkan framebuffer stencil execution is not implemented");
        if(!color()||!color()->texture)throw std::logic_error("Native framebuffer rendering requires a texture at COLOR_ATTACHMENT0");
        const auto& c=*color();const auto storage=c.texture;const auto d=depth();
        const auto depthImage=d&&d->renderbuffer?d->renderbuffer->image:std::shared_ptr<ManagedRenderbufferImage>{};
        const auto depthTexture=d&&d->texture?d->texture->resource:TextureRef{};
        const auto depthFormat=depthTexture?d->texture->config.format:depthImage?depthImage->format:VK_FORMAT_UNDEFINED;
        if(attachments.size()>(depthFormat!=VK_FORMAT_UNDEFINED?2u:1u))throw std::logic_error("Native multiple framebuffer color attachments are not implemented");
        if(cached&&cachedColor==storage->resource&&cachedDepth==depthImage&&cachedDepthTexture==depthTexture)return cached;
        r.requireColorFeatures(storage->config.format,VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
        if(storage->shape.target==0x806f||(depthTexture&&d->texture->shape.target==0x806f))throw std::logic_error("Native 3D texture framebuffer slices are not implemented");
        auto result=std::shared_ptr<GpuTarget>(new GpuTarget{},[&r](auto* value){
            if(value->framebuffer)r.api.vkDestroyFramebuffer(r.device,value->framebuffer,nullptr);
            if(value->faceView)r.api.vkDestroyImageView(r.device,value->faceView,nullptr);
            if(value->managedDepthView)r.api.vkDestroyImageView(r.device,value->managedDepthView,nullptr);
            delete value;
        });
        result->managedAttachment=true;result->color=storage->resource->image;result->colorFormat=storage->config.format;
        result->imageMip=std::uint32_t(c.level);result->imageLayer=storage->shape.target==0x8513?c.target-0x8515:storage->shape.target==0x8c1a?std::uint32_t(c.layer):0u;
        result->width=std::max(1u,storage->config.width>>c.level);result->height=std::max(1u,storage->config.height>>c.level);
        result->attachmentOwners.push_back(storage->resource);
        if(depthTexture){
            result->managedDepthTexture=true;result->depth=depthTexture->image;result->attachmentOwners.push_back(depthTexture);
            result->managedDepthMip=std::uint32_t(d->level);
            result->managedDepthLayer=d->texture->shape.target==0x8513?d->target-0x8515:d->texture->shape.target==0x8c1a?std::uint32_t(d->layer):0u;
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=result->depth.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=depthFormat;
            view.subresourceRange={VK_IMAGE_ASPECT_DEPTH_BIT,result->managedDepthMip,1,result->managedDepthLayer,1};
            check(r.api.vkCreateImageView(r.device,&view,nullptr,&result->managedDepthView),"vkCreateImageView(native depth attachment)");
        }else if(depthImage){result->depth=depthImage->image;result->attachmentOwners.push_back(depthImage);}
        result->nativeRenderPass=r.renderPassFor(result->colorFormat,depthFormat);
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=result->color.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;view.format=result->colorFormat;
        view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,result->imageMip,1,result->imageLayer,1};
        check(r.api.vkCreateImageView(r.device,&view,nullptr,&result->faceView),"vkCreateImageView(native framebuffer)");
        const VkImageView views[]={result->faceView,depthTexture?result->managedDepthView:depthImage?depthImage->image.view:VK_NULL_HANDLE};
        VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};info.renderPass=result->nativeRenderPass;info.attachmentCount=depthFormat!=VK_FORMAT_UNDEFINED?2:1;info.pAttachments=views;
        info.width=result->width;info.height=result->height;info.layers=1;
        check(r.api.vkCreateFramebuffer(r.device,&info,nullptr,&result->framebuffer),"vkCreateFramebuffer(native framebuffer)");
        result->pixels.resize(std::size_t(result->width)*result->height*colorPixelBytes(result->colorFormat));
        cached=std::move(result);cachedColor=storage->resource;cachedDepth=depthImage;cachedDepthTexture=depthTexture;return cached;
    }
};
bool managedFramebufferColorOutput=true;
void requireTypedFramebuffer(){
    if(managedTextureCommands&&!managedTextureCommands->boundFramebuffer(0x8ca9).isNull())
        throw std::logic_error("Bind the default framebuffer before using a typed framebuffer operation");
    if(!active||!active->managedAttachment)return;
    if(!rendering){selectManagedFramebuffer();return;}
    throw std::logic_error("A typed framebuffer operation cannot interrupt an active managed render");
}
unsigned managedFramebufferDepthBits(){
    const auto handle=managedTextureCommands->boundFramebuffer(0x8ca9);
    if(handle.isNull()){
        switch(depthFormat){case VK_FORMAT_D16_UNORM:return 16;case VK_FORMAT_X8_D24_UNORM_PACK32:case VK_FORMAT_D24_UNORM_S8_UINT:return 24;case VK_FORMAT_UNDEFINED:return 0;default:return 32;}
    }
    const auto fb=std::dynamic_pointer_cast<ManagedFramebufferStorage>(managedTextureCommands->framebufferStorage(handle));
    if(!fb)return 0;const auto depth=fb->depth();if(!depth)return 0;
    const auto format=depth->texture?depth->texture->config.format:depth->renderbuffer&&depth->renderbuffer->image?depth->renderbuffer->image->format:VK_FORMAT_UNDEFINED;
    if(format==VK_FORMAT_UNDEFINED)return 0;
    switch(format){case VK_FORMAT_D16_UNORM:return 16;case VK_FORMAT_X8_D24_UNORM_PACK32:case VK_FORMAT_D24_UNORM_S8_UINT:return 24;default:return 32;}
}
unsigned managedFramebufferStencilBits(){
    const auto handle=managedTextureCommands->boundFramebuffer(0x8ca9);if(handle.isNull())return 0;
    const auto fb=std::dynamic_pointer_cast<ManagedFramebufferStorage>(managedTextureCommands->framebufferStorage(handle));
    if(!fb)return 0;const auto found=fb->attachments.find(0x8d20);if(found==fb->attachments.end())return 0;
    const auto storage=found->second.renderbuffer;return storage&&storage->image&&(storage->image->aspect&VK_IMAGE_ASPECT_STENCIL_BIT)?8:0;
}
bool selectManagedFramebuffer(){
    const auto handle=managedTextureCommands->boundFramebuffer(0x8ca9);
    std::shared_ptr<GpuTarget> next;
    if(!handle.isNull()){
        const auto fb=std::dynamic_pointer_cast<ManagedFramebufferStorage>(managedTextureCommands->framebufferStorage(handle));
        if(!fb)throw JSTypeError("Native draw framebuffer is unavailable");
        if(fb->status(0x8d40)!=0x8cd5){managedNativeError(0x0506);return false;}
        next=fb->target();const auto outputs=managedTextureCommands->framebufferDrawBuffers(handle);
        if(outputs.size()>1)throw std::logic_error("Native multiple draw buffers are not implemented");
        managedFramebufferColorOutput=!outputs.empty()&&outputs[0]!=0;
    }else{
        const auto outputs=managedTextureCommands->framebufferDrawBuffers(handle);
        managedFramebufferColorOutput=!outputs.empty()&&outputs[0]!=0;
    }
    if((next&&next==active)||(!next&&!active->managedAttachment))return true;
    if(recording)endFrame();
    if(next){if(!active->managedAttachment)managedScreenTarget=active;active=std::move(next);}
    else {if(!managedScreenTarget)throw std::logic_error("Native default framebuffer is unavailable");active=std::move(managedScreenTarget);}
    width=active->width;height=active->height;activeCubeFace=0;
    const auto size=std::size_t(width)*height*colorPixelBytes(active->colorFormat);
    if(readback.size!=size){auto replacement=createBuffer(size,VK_BUFFER_USAGE_TRANSFER_DST_BIT);destroyBuffer(readback);readback=replacement;}
    return true;
}
void managedFramebufferBarrier(VkImage image,VkImageAspectFlags aspect,VkImageLayout before,VkImageLayout after,VkAccessFlags source,VkAccessFlags destination,VkPipelineStageFlags sourceStage,VkPipelineStageFlags destinationStage,std::uint32_t mip=0,std::uint32_t layer=0){
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcAccessMask=source;barrier.dstAccessMask=destination;
    barrier.oldLayout=before;barrier.newLayout=after;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.image=image;barrier.subresourceRange={aspect,mip,1,layer,1};
    api.vkCmdPipelineBarrier(commandBuffer,sourceStage,destinationStage,0,0,nullptr,0,nullptr,1,&barrier);
}
void beginManagedFramebufferFrame(bool clearColor,bool clearDepth,const Color& color,HIGH_PRECISION alpha,bool srgb){
    for(auto pool:descriptorPools)check(api.vkResetDescriptorPool(device,pool,0),"vkResetDescriptorPool");
    check(api.vkResetCommandBuffer(commandBuffer,0),"vkResetCommandBuffer(native framebuffer)");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(api.vkBeginCommandBuffer(commandBuffer,&begin),"vkBeginCommandBuffer(native framebuffer)");
    managedFramebufferBarrier(active->color.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,active->imageMip,active->imageLayer);
    bool initializeDepth=false;
    if(active->depth.image&&active->managedDepthTexture){
        managedFramebufferBarrier(active->depth.image,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT,VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            active->managedDepthMip,active->managedDepthLayer);
    }else if(active->depth.image){
        auto depth=std::static_pointer_cast<ManagedRenderbufferImage>(active->attachmentOwners[1]);initializeDepth=!depth->initialized;
        if(initializeDepth)managedFramebufferBarrier(depth->image.image,depth->aspect,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,0,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    }
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};pass.renderPass=activeRenderPass();pass.framebuffer=active->framebuffer;pass.renderArea.extent={width,height};
    api.vkCmdBeginRenderPass(commandBuffer,&pass,VK_SUBPASS_CONTENTS_INLINE);recording=true;++recordingEpoch;
    if(initializeDepth)clearAttachments(false,true,Color(0,0,0),0,false,false);
    clearAttachments(clearColor&&managedFramebufferColorOutput,clearDepth,color,alpha,srgb);applyViewportAndScissor();
}
void endManagedFramebufferFrame(){
    api.vkCmdEndRenderPass(commandBuffer);
    if(active->depth.image&&active->managedDepthTexture)
        managedFramebufferBarrier(active->depth.image,VK_IMAGE_ASPECT_DEPTH_BIT,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            active->managedDepthMip,active->managedDepthLayer);
    managedFramebufferBarrier(active->color.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,VK_ACCESS_TRANSFER_READ_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,active->imageMip,active->imageLayer);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,active->imageMip,active->imageLayer,1};copy.imageExtent={width,height,1};
    api.vkCmdCopyImageToBuffer(commandBuffer,active->color.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback.buffer,1,&copy);
    managedFramebufferBarrier(active->color.image,VK_IMAGE_ASPECT_COLOR_BIT,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_READ_BIT,VK_ACCESS_SHADER_READ_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,active->imageMip,active->imageLayer);
    VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    api.vkCmdPipelineBarrier(commandBuffer,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
    check(api.vkEndCommandBuffer(commandBuffer),"vkEndCommandBuffer(native framebuffer)");recording=false;
    check(api.vkResetFences(device,1,&fence),"vkResetFences(native framebuffer)");VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&commandBuffer;
    check(api.vkQueueSubmit(queue,1,&submit,fence),"vkQueueSubmit(native framebuffer)");targetSubmissionPending=true;
    check(api.vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX),"vkWaitForFences(native framebuffer)");targetSubmissionPending=false;
    void* mapped=nullptr;check(api.vkMapMemory(device,readback.memory,0,readback.size,0,&mapped),"vkMapMemory(native framebuffer)");
    std::memcpy(active->pixels.data(),mapped,active->pixels.size());api.vkUnmapMemory(device,readback.memory);active->initialized=true;
    if(active->depth.image&&!active->managedDepthTexture)std::static_pointer_cast<ManagedRenderbufferImage>(active->attachmentOwners[1])->initialized=true;
    collectRetiredTargets();
}
bool managedFeedbackLoop(std::uint32_t unit,std::uint32_t target)const{
    const auto framebuffer=managedTextureCommands->boundFramebuffer(0x8ca9);if(framebuffer.isNull())return false;
    const auto fb=std::dynamic_pointer_cast<ManagedFramebufferStorage>(managedTextureCommands->framebufferStorage(framebuffer));
    const auto handle=managedTextureCommands->boundTexture(unit,target);if(!fb||handle.isNull())return false;
    const auto storage=std::dynamic_pointer_cast<ManagedTextureStorage>(managedTextureCommands->storage(handle));if(!storage||!storage->resource)return false;
    const auto [base,last]=storage->samplingRange();
    for(const auto& [point,attachment]:fb->attachments)
        if(attachment.texture==storage&&attachment.level>=0&&std::uint32_t(attachment.level)>=base&&std::uint32_t(attachment.level)<=last)return true;
    return false;
}
