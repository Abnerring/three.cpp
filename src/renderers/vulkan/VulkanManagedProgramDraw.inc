// Included inside VulkanRenderer::Impl after the native texture/buffer helpers.
// Source GL draw commands supply already committed native program/fixed state.
// No application scene/material/geometry fields are sampled at this boundary.
struct ManagedProgramGpu {
    std::weak_ptr<VulkanProgramCommands::LinkedProgram> owner;
    std::uint64_t generation = 0;
    std::unique_ptr<VulkanShaderModules> modules;
    std::unique_ptr<VulkanPipelineLayout> layout;
    std::unordered_map<std::string, VkPipeline> pipelines;
    struct Uniform {
        std::vector<std::byte> bytes;
        BufferRef resource;
    };
    std::map<VulkanProgramUniforms::Key, Uniform> uniforms;
};
struct ManagedProgramDrawSnapshot {
    std::shared_ptr<VulkanProgramCommands::LinkedProgram> program;
    PipelineState state;
    int side = FrontSide;
    bool frontFaceCW = false, reversed = false;
    bool samplingFeedback = false;
    std::uint32_t firstInstance = 0;
    std::map<VulkanProgramUniforms::Key, std::vector<std::byte>> blocks;
    std::map<VulkanProgramUniforms::Key, std::vector<TextureRef>> samplers;
};
struct ManagedProgramPrepared {
    std::shared_ptr<VulkanProgramCommands::LinkedProgram> program;
    std::shared_ptr<ManagedProgramGpu> gpu;
    std::vector<BufferRef> buffers;
    std::vector<TextureRef> textures;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptors;
};
std::vector<std::shared_ptr<ManagedProgramGpu>> managedProgramGpus;
std::vector<std::shared_ptr<ManagedProgramPrepared>> managedProgramPrepared;

void clearManagedProgramFrame() {
    // Only after every submission using these descriptors/resources has finished.
    managedProgramPrepared.clear();
    std::erase_if(managedProgramGpus, [](const auto& gpu) { return gpu->owner.expired(); });
}
void resetManagedProgramDraw() {
    // Impl must wait for the device, then call this before destroying the device.
    managedProgramPrepared.clear();
    managedProgramGpus.clear();
}
ManagedProgramDrawSnapshot captureManagedProgramDraw(const PipelineState& state, int side,
        bool frontFaceCW, bool reversed, std::uint32_t firstInstance = 0) {
    if (!managedProgramCommands) throw std::logic_error("Native managed program commands are unavailable");
    auto program = managedProgramCommands->current();
    if (!program || !program->result.linked || !program->uniforms)
        throw std::logic_error("A managed Vulkan draw requires a linked current native program");
    ManagedProgramDrawSnapshot result;
    result.program=program;result.state=state;result.side=side;
    result.frontFaceCW=frontFaceCW;result.reversed=reversed;result.firstInstance=firstInstance;
    // Negative height selects the GL-oriented offscreen viewport in the ESSL adapter.
    program->uniforms->writeBackend(firstInstance, active->managedAttachment ? -float(height) : float(height));
    for (const auto& [key, block] : program->uniforms->blocks()) {
        // The native ESSL conversion owns binding zero for original standalone
        // uniforms. Explicit source UBOs require the actual bindBufferBase path.
        if (key.set != 0 || key.binding != 0)
            throw std::logic_error("Managed explicit uniform-buffer bindings are not implemented");
        const auto bytes = block.storage.bytes();
        result.blocks.emplace(key, std::vector<std::byte>(bytes.begin(), bytes.end()));
    }
    for (const auto& [name, sampler] : program->uniforms->samplers()) {
        const VulkanShaderInterface::Descriptor* descriptor = nullptr;
        for (const auto& stage : program->result.interfaces)
            for (const auto& candidate : stage.descriptors)
                if (candidate.set == sampler.key.set && candidate.binding == sampler.key.binding)
                    {
                        if (descriptor && (descriptor->imageDimension != candidate.imageDimension ||
                            descriptor->imageArray != candidate.imageArray || descriptor->imageDepth != candidate.imageDepth ||
                            descriptor->imageMultisampled != candidate.imageMultisampled || descriptor->count != candidate.count))
                            throw std::logic_error("Native sampler image shape differs between shader stages");
                        descriptor = &candidate;
                    }
        if (!descriptor || descriptor->type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            descriptor->imageMultisampled ||
            descriptor->imageDepth == 1 || descriptor->count != sampler.units.size())
            throw std::logic_error("Managed Vulkan comparison, multisample and separate image samplers are not implemented");
        // SPIR-V Dim and Arrayed describe the real image view. Original active
        // uniform types additionally distinguish ordinary from integer samplers.
        std::uint32_t target = 0;
        int glType = 0;
        if (descriptor->imageDimension == 1) {
            target = descriptor->imageArray ? 0x8c1a : 0x0de1;
            glType = descriptor->imageArray ? 0x8dc1 : 0x8b5e;
        } else if (descriptor->imageDimension == 2 && !descriptor->imageArray) {
            target = 0x806f; glType = 0x8b5f;
        } else if (descriptor->imageDimension == 3 && !descriptor->imageArray) {
            target = 0x8513; glType = 0x8b60;
        } else {
            throw std::logic_error("Managed Vulkan sampler image dimension is not implemented");
        }
        bool originalType = false;
        for (const auto& uniform : program->result.activeUniforms)
            if (uniform.name == name || uniform.name.starts_with(name + "[")) {
                originalType = true;
                if (uniform.glType != glType)
                    throw std::logic_error("Managed integer and comparison samplers are not implemented");
            }
        if (!originalType) throw std::logic_error("Native sampler lacks an original active uniform type");
        auto& textures = result.samplers[sampler.key];
        for (const auto unit : sampler.units) {
            if (unit < 0) throw std::invalid_argument("Negative native sampler texture unit");
            auto texture = managedSampledTexture(std::uint32_t(unit), target);
            result.samplingFeedback=result.samplingFeedback||managedFeedbackLoop(std::uint32_t(unit),target);
            if (!texture || !texture->sampler || !texture->image.view)
                throw std::logic_error("Native texture binding has no Vulkan sampled image");
            textures.push_back(std::move(texture));
        }
    }
    return result;
}

std::shared_ptr<ManagedProgramGpu> managedProgramGpu(
        const std::shared_ptr<VulkanProgramCommands::LinkedProgram>& program) {
    std::erase_if(managedProgramGpus, [](const auto& gpu) { return gpu->owner.expired(); });
    for (const auto& gpu : managedProgramGpus)
        if (gpu->generation == program->generation && gpu->owner.lock() == program) return gpu;
    auto gpu = std::shared_ptr<ManagedProgramGpu>(new ManagedProgramGpu{}, [this](auto* value) {
        for (const auto& [key, pipeline] : value->pipelines)
            api.vkDestroyPipeline(device, pipeline, nullptr);
        delete value;
    });
    gpu->owner = program; gpu->generation = program->generation;
    const VulkanShaderModules::Stage stages[] = {
        {program->result.modules[0].words, "main"}, {program->result.modules[1].words, "main"}};
    gpu->modules = std::make_unique<VulkanShaderModules>(device,
        VulkanShaderModules::Dispatch{api.vkCreateShaderModule, api.vkDestroyShaderModule}, stages);
    VkPhysicalDeviceProperties properties{}; api.vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    gpu->layout = std::make_unique<VulkanPipelineLayout>(device,
        VulkanPipelineLayout::Dispatch{api.vkCreateDescriptorSetLayout, api.vkDestroyDescriptorSetLayout,
            api.vkCreatePipelineLayout, api.vkDestroyPipelineLayout},
        gpu->modules->interfaces(), properties.limits, VulkanPipelineLayout::DescriptorCounts{}, 1);
    if (!gpu->layout->description().pushConstants.empty())
        throw std::logic_error("Original managed GLSL unexpectedly requires push constants");
    managedProgramGpus.push_back(gpu);
    return gpu;
}
static VkBlendFactor managedBlendFactor(std::uint32_t value) {
    switch (value) {
    case 0: return VK_BLEND_FACTOR_ZERO; case 1: return VK_BLEND_FACTOR_ONE;
    case 0x0300: return VK_BLEND_FACTOR_SRC_COLOR; case 0x0301: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case 0x0302: return VK_BLEND_FACTOR_SRC_ALPHA; case 0x0303: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case 0x0304: return VK_BLEND_FACTOR_DST_ALPHA; case 0x0305: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case 0x0306: return VK_BLEND_FACTOR_DST_COLOR; case 0x0307: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case 0x0308: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case 0x8001: return VK_BLEND_FACTOR_CONSTANT_COLOR; case 0x8002: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case 0x8003: return VK_BLEND_FACTOR_CONSTANT_ALPHA; case 0x8004: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    default: throw std::invalid_argument("Unsupported native GL blend factor");
    }
}
static VkBlendOp managedBlendEquation(std::uint32_t value) {
    switch (value) {
    case 0x8006: return VK_BLEND_OP_ADD; case 0x8007: return VK_BLEND_OP_MIN; case 0x8008: return VK_BLEND_OP_MAX;
    case 0x800a: return VK_BLEND_OP_SUBTRACT; case 0x800b: return VK_BLEND_OP_REVERSE_SUBTRACT;
    default: throw std::invalid_argument("Unsupported native GL blend equation");
    }
}
static VkPrimitiveTopology managedTopology(std::uint32_t mode) {
    switch (mode) {
    case 0: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case 1: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case 2: case 3: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case 4: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case 5: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case 6: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    default: throw std::invalid_argument("Unsupported native GL draw mode");
    }
}
static VkFormat managedFloatVertexFormat(unsigned components) {
    switch (components) {
    case 1: return VK_FORMAT_R32_SFLOAT; case 2: return VK_FORMAT_R32G32_SFLOAT;
    case 3: return VK_FORMAT_R32G32B32_SFLOAT; case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    default: throw std::invalid_argument("Invalid managed floating vertex shape");
    }
}
VkPipeline managedProgramPipeline(const ManagedProgramDrawSnapshot& draw, ManagedProgramGpu& gpu,
        std::uint32_t mode, const std::vector<VkVertexInputBindingDescription>& bindings,
        const std::vector<VkVertexInputAttributeDescription>& attributes) {
    const auto& state = draw.state;
    if (state.nativeGL && state.nativeLineWidth != 1)
        throw std::logic_error("Managed Vulkan wide line rasterization is not implemented");
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL; raster.lineWidth = 1;
    raster.depthBiasEnable = state.polygonOffset && mode >= 4;
    raster.depthBiasConstantFactor = float(state.polygonOffsetUnits);
    raster.depthBiasSlopeFactor = float(state.polygonOffsetFactor);
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = state.depthTest; depth.depthWriteEnable = state.depthWrite;
    VkPipelineColorBlendAttachmentState blend{};
    VkPipelineColorBlendStateCreateInfo color{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color.attachmentCount = 1; color.pAttachments = &blend;
    if (state.nativeGL) {
        if (!state.nativeCull) raster.cullMode = VK_CULL_MODE_NONE;
        else if (state.nativeCullFace == 0x0404) raster.cullMode = VK_CULL_MODE_FRONT_BIT;
        else if (state.nativeCullFace == 0x0405) raster.cullMode = VK_CULL_MODE_BACK_BIT;
        else if (state.nativeCullFace == 0x0408) raster.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
        else throw std::invalid_argument("Unsupported native GL cull face");
        if (state.nativeFrontFace != 0x0900 && state.nativeFrontFace != 0x0901)
            throw std::invalid_argument("Unsupported native GL front face");
        raster.frontFace = state.nativeFrontFace == 0x0900 ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
        if (state.nativeDepthFunc < 0x0200 || state.nativeDepthFunc > 0x0207)
            throw std::invalid_argument("Unsupported native GL depth function");
        static constexpr VkCompareOp compare[] = {VK_COMPARE_OP_NEVER, VK_COMPARE_OP_LESS, VK_COMPARE_OP_EQUAL,
            VK_COMPARE_OP_LESS_OR_EQUAL, VK_COMPARE_OP_GREATER, VK_COMPARE_OP_NOT_EQUAL,
            VK_COMPARE_OP_GREATER_OR_EQUAL, VK_COMPARE_OP_ALWAYS};
        depth.depthCompareOp = compare[state.nativeDepthFunc - 0x0200];
        blend.colorWriteMask = state.nativeColorMask & 15;
        blend.blendEnable = state.nativeBlend;
        blend.colorBlendOp = managedBlendEquation(state.nativeEquationRGB);
        blend.alphaBlendOp = managedBlendEquation(state.nativeEquationAlpha);
        blend.srcColorBlendFactor = managedBlendFactor(state.nativeSrcRGB);
        blend.dstColorBlendFactor = managedBlendFactor(state.nativeDstRGB);
        blend.srcAlphaBlendFactor = managedBlendFactor(state.nativeSrcAlpha);
        blend.dstAlphaBlendFactor = managedBlendFactor(state.nativeDstAlpha);
        std::copy(state.nativeBlendColor.begin(), state.nativeBlendColor.end(), color.blendConstants);
    } else {
        if (draw.side < 0 || draw.side > 2 || state.blending < 0 || state.blending > 4)
            throw std::invalid_argument("Unsupported managed material state");
        raster.cullMode = draw.side == DoubleSide ? VK_CULL_MODE_NONE : draw.side == BackSide ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT;
        raster.frontFace = draw.frontFaceCW ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
        depth.depthCompareOp = depthCompare(state.depthFunc, draw.reversed);
        blend.colorWriteMask = state.colorWrite ? 15 : 0;
        blend.blendEnable = state.blending != NoBlending && !(state.blending == NormalBlending && !state.transparent);
        blend.colorBlendOp = blend.alphaBlendOp = VK_BLEND_OP_ADD;
        blend.srcColorBlendFactor = state.premultipliedAlpha ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        if (state.blending == AdditiveBlending) blend.dstColorBlendFactor = blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        if (state.blending == SubtractiveBlending) {
            if (!state.premultipliedAlpha) throw std::invalid_argument("Subtractive blending requires premultiplied alpha");
            blend.srcColorBlendFactor = blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        }
        if (state.blending == MultiplyBlending) {
            if (!state.premultipliedAlpha) throw std::invalid_argument("Multiply blending requires premultiplied alpha");
            blend.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        }
    }
    if (blend.blendEnable) requireColorFeatures(active->colorFormat, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT);
    std::string key;
    const auto add = [&](auto value) { key.append(reinterpret_cast<const char*>(&value), sizeof(value)); };
    if(active->managedAttachment)
        raster.frontFace = raster.frontFace == VK_FRONT_FACE_CLOCKWISE ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    add(mode); add(active->colorFormat); add(activeRenderPass()); add(raster.cullMode); add(raster.frontFace); add(raster.depthBiasEnable);
    add(raster.depthBiasConstantFactor); add(raster.depthBiasSlopeFactor); add(depth.depthTestEnable);
    add(depth.depthWriteEnable); add(depth.depthCompareOp); add(blend.colorWriteMask); add(blend.blendEnable);
    add(blend.colorBlendOp); add(blend.alphaBlendOp); add(blend.srcColorBlendFactor); add(blend.dstColorBlendFactor);
    add(blend.srcAlphaBlendFactor); add(blend.dstAlphaBlendFactor);
    for (const auto value : color.blendConstants) add(value);
    add(std::uint32_t(bindings.size())); add(std::uint32_t(attributes.size()));
    for (const auto& binding : bindings) { add(binding.binding); add(binding.stride); add(binding.inputRate); }
    for (const auto& attribute : attributes) { add(attribute.location); add(attribute.binding); add(attribute.format); add(attribute.offset); }
    if (const auto found = gpu.pipelines.find(key); found != gpu.pipelines.end()) return found->second;
    const auto stages = gpu.modules->pipelineStages();
    VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex.vertexBindingDescriptionCount = std::uint32_t(bindings.size()); vertex.pVertexBindingDescriptions = bindings.data();
    vertex.vertexAttributeDescriptionCount = std::uint32_t(attributes.size()); vertex.pVertexAttributeDescriptions = attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = managedTopology(mode);
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = viewport.scissorCount = 1;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 2; dynamic.pDynamicStates = dynamicStates;
    VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    create.stageCount = std::uint32_t(stages.size()); create.pStages = stages.data();
    create.pVertexInputState = &vertex; create.pInputAssemblyState = &assembly; create.pViewportState = &viewport;
    create.pRasterizationState = &raster; create.pMultisampleState = &multisample;
    create.pDepthStencilState = &depth; create.pColorBlendState = &color; create.pDynamicState = &dynamic;
    create.layout = gpu.layout->handle(); create.renderPass = activeRenderPass();
    VkPipeline pipeline = VK_NULL_HANDLE;
    check(api.vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create, nullptr, &pipeline), "vkCreateGraphicsPipelines(native program)");
    gpu.pipelines.emplace(std::move(key), pipeline);
    return pipeline;
}

BufferRef managedProgramUniform(ManagedProgramGpu& gpu, VulkanProgramUniforms::Key key,
        const std::vector<std::byte>& bytes) {
    if (bytes.empty()) throw std::logic_error("Empty reflected uniform block");
    auto& cached = gpu.uniforms[key];
    if (!cached.resource || cached.bytes != bytes) {
        if (!cached.resource || cached.resource->size != bytes.size() ||
            (recording && cached.resource->usedEpoch == recordingEpoch))
            cached.resource = sharedBuffer(bytes.size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, bytes.data());
        else {
            void* mapped = nullptr;
            check(api.vkMapMemory(device, cached.resource->memory, 0, bytes.size(), 0, &mapped), "vkMapMemory(native uniform)");
            std::memcpy(mapped, bytes.data(), bytes.size()); api.vkUnmapMemory(device, cached.resource->memory);
        }
        cached.bytes = bytes;
    }
    return cached.resource;
}
void managedProgramDescriptors(const ManagedProgramDrawSnapshot& snapshot, ManagedProgramPrepared& draw) {
    const auto layouts = draw.gpu->layout->setLayouts();
    if (layouts.empty()) return;
    std::map<VkDescriptorType, std::uint32_t> counts;
    for (const auto& set : draw.gpu->layout->description().sets)
        for (const auto& binding : set) counts[binding.descriptorType] += binding.descriptorCount;
    std::vector<VkDescriptorPoolSize> sizes;
    for (const auto& [type, count] : counts) sizes.push_back({type, count});
    VkDescriptorPoolCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    create.maxSets = std::uint32_t(layouts.size()); create.poolSizeCount = std::uint32_t(sizes.size()); create.pPoolSizes = sizes.data();
    check(api.vkCreateDescriptorPool(device, &create, nullptr, &draw.descriptorPool), "vkCreateDescriptorPool(native program)");
    draw.descriptors.resize(layouts.size());
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate.descriptorPool = draw.descriptorPool; allocate.descriptorSetCount = std::uint32_t(layouts.size()); allocate.pSetLayouts = layouts.data();
    check(api.vkAllocateDescriptorSets(device, &allocate, draw.descriptors.data()), "vkAllocateDescriptorSets(native program)");
    for (std::size_t set = 0; set < draw.gpu->layout->description().sets.size(); ++set) {
        for (const auto& binding : draw.gpu->layout->description().sets[set]) {
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}; write.dstSet = draw.descriptors[set];
            write.dstBinding = binding.binding; write.descriptorType = binding.descriptorType; write.descriptorCount = binding.descriptorCount;
            const VulkanProgramUniforms::Key key{std::uint32_t(set), binding.binding};
            if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && binding.descriptorCount == 1) {
                const auto found = snapshot.blocks.find(key);
                if (found == snapshot.blocks.end()) throw std::logic_error("Missing original uniform block snapshot");
                auto buffer = managedProgramUniform(*draw.gpu, key, found->second);
                const VkDescriptorBufferInfo info{buffer->buffer, 0, found->second.size()};
                write.pBufferInfo = &info; api.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
                draw.buffers.push_back(std::move(buffer));
            } else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                const auto found = snapshot.samplers.find(key);
                if (found == snapshot.samplers.end() || found->second.size() != binding.descriptorCount)
                    throw std::logic_error("Missing original sampler-unit snapshot");
                std::vector<VkDescriptorImageInfo> images;
                for (const auto& texture : found->second) {
                    images.push_back({texture->sampler, texture->image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                    draw.textures.push_back(texture);
                }
                write.pImageInfo = images.data(); api.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
            } else throw std::logic_error("Managed original shader resource kind is not implemented");
        }
    }
}

void recordManagedProgramDraw(ManagedProgramDrawSnapshot snapshot, std::uint32_t mode,
        std::uint32_t start, std::uint32_t count, std::uint32_t instanceCount, bool indexed, double indexType) {
    if (!recording) throw std::logic_error("Native managed draw has no recording frame");
    if (!count || !instanceCount) return;
    if (snapshot.firstInstance != 0)
        throw std::logic_error("Managed base-instance vertex input is not implemented");
    const auto bindings = bindingCommands->snapshot();
    const auto invalid = [&] { bindingCommands->invalidDraw(); };
    BufferRef indices;
    const std::vector<std::uint32_t>* indexValues = nullptr;
    std::uint64_t vertexRows = std::uint64_t(start) + count;
    if (indexed) {
        if (!attributeCommands->hasStorage(bindings.index)) { invalid(); return; }
        auto& cache = managedIndex(bindings.index, indexType); indexValues = &cache.values; indices = cache.resource;
        if (start > indexValues->size() || count > indexValues->size() - start) { invalid(); return; }
        vertexRows = 0;
        const auto restart = indexType == 0x1401 ? 0xffu : indexType == 0x1403 ? 0xffffu : UINT32_MAX;
        for (std::size_t i = start; i < std::size_t(start) + count; ++i) {
            if ((*indexValues)[i] == restart)
                throw std::logic_error("Managed fixed-index primitive restart is not implemented");
            vertexRows = std::max(vertexRows, std::uint64_t((*indexValues)[i]) + 1);
        }
    }
    auto draw = std::shared_ptr<ManagedProgramPrepared>(new ManagedProgramPrepared{}, [this](auto* value) {
        if (value->descriptorPool) api.vkDestroyDescriptorPool(device, value->descriptorPool, nullptr);
        delete value;
    });
    draw->program = snapshot.program; draw->gpu = managedProgramGpu(snapshot.program);
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    std::vector<VkBuffer> vertexBuffers;
    std::vector<VkDeviceSize> offsets;
    VkPhysicalDeviceProperties properties{}; api.vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    for (const auto& attribute : snapshot.program->result.attributeMappings) {
        if (attribute.builtIn >= 0) continue;
        const bool floating=attribute.numeric.scalar==VulkanShaderInterface::Numeric::Scalar::FloatingPoint;
        const bool signedInteger=attribute.numeric.scalar==VulkanShaderInterface::Numeric::Scalar::SignedInteger;
        const bool unsignedInteger=attribute.numeric.scalar==VulkanShaderInterface::Numeric::Scalar::UnsignedInteger;
        if(attribute.component!=0||attribute.numeric.width!=32||(!floating&&!signedInteger&&!unsignedInteger))
            throw std::logic_error("Managed shader requires an unsupported packed or non-32-bit vertex input: "+attribute.sourceName);
        const auto columns = std::max(attribute.numeric.columns, 1u);
        const auto components = attribute.numeric.columns ? attribute.numeric.rows : std::max(attribute.numeric.components, 1u);
        std::uint64_t locations = columns;
        for (const auto size : attribute.array.dimensions) {
            if (!size || locations > UINT32_MAX / size) throw std::invalid_argument("Managed shader attribute array is too large");
            locations *= size;
        }
        for (std::uint32_t column = 0; column < locations; ++column) {
            const auto location = std::uint64_t(attribute.location) + column;
            if (location >= bindings.attributes.size() || location >= properties.limits.maxVertexInputAttributes ||
                vertexBindings.size() >= properties.limits.maxVertexInputBindings)
                throw std::invalid_argument("Managed source shader exceeds native vertex input limits");
            const auto& pointer = bindings.attributes[std::size_t(location)];
            if(pointer.enabled&&!attributeCommands->hasStorage(pointer.buffer)){invalid();return;}
            const auto required=!pointer.enabled?1:pointer.divisor?
                (std::uint64_t(instanceCount)-1)/pointer.divisor+1:vertexRows;
            BufferRef buffer;
            VkFormat format;
            if(floating) {
                if((pointer.enabled&&pointer.integer)||(!pointer.enabled&&pointer.constantType!=VulkanBindingCommands::Attribute::ConstantType::FloatingPoint)){invalid();return;}
                const auto cache=boundInput(pointer,components);
                if(pointer.enabled&&double(required)>cache->snapshot->count){invalid();return;}
                buffer=cache->vertices[false];format=managedFloatVertexFormat(components);
                if(pointer.enabled&&pointer.divisor>1) {
                    if(std::uint64_t(instanceCount)*components>SIZE_MAX/sizeof(float))throw std::overflow_error("Managed instance data is too large");
                    std::vector<float> expanded(std::size_t(instanceCount)*components);
                    for(std::uint32_t row=0;row<instanceCount;++row)for(std::uint32_t component=0;component<components;++component)
                        expanded[std::size_t(row)*components+component]=float(cache->snapshot->getComponent(row/pointer.divisor,component));
                    buffer=sharedBuffer(expanded.size()*sizeof(float),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,expanded.data());
                }
            } else {
                // WebGL2 requires signedness and pointer/constant category to
                // match the active shader input, even for disabled arrays.
                const bool matches=pointer.enabled?pointer.integer&&(signedInteger?managedSignedPointer(pointer.type):managedUnsignedPointer(pointer.type)):
                    pointer.constantType==(signedInteger?VulkanBindingCommands::Attribute::ConstantType::SignedInteger:VulkanBindingCommands::Attribute::ConstantType::UnsignedInteger);
                if(!matches){invalid();return;}
                auto& input=managedIntegerInput(pointer,components);
                if(required>input.rows){invalid();return;}
                buffer=input.buffer;format=managedIntegerVertexFormat(components,signedInteger);
                if(pointer.enabled&&pointer.divisor>1) {
                    if(std::uint64_t(instanceCount)*components>SIZE_MAX/sizeof(std::uint32_t))throw std::overflow_error("Native integer instance data is too large");
                    std::vector<std::uint32_t> expanded(std::size_t(instanceCount)*components);
                    for(std::uint32_t row=0;row<instanceCount;++row)for(std::uint32_t component=0;component<components;++component)
                        expanded[std::size_t(row)*components+component]=input.values[std::size_t(row/pointer.divisor)*components+component];
                    buffer=sharedBuffer(expanded.size()*sizeof(std::uint32_t),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,expanded.data());
                }
            }
            const auto instance=pointer.enabled&&pointer.divisor!=0;
            const auto binding = std::uint32_t(vertexBindings.size());
            vertexBindings.push_back({binding, pointer.enabled ? components * 4 : 0, instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX});
            vertexAttributes.push_back({std::uint32_t(location), binding, format, 0});
            vertexBuffers.push_back(buffer->buffer); offsets.push_back(0); draw->buffers.push_back(std::move(buffer));
        }
    }
    // GL_LINE_LOOP has no Vulkan topology. Its closing index belongs to this
    // one GPU draw; source range/count/statistics remain in the source renderer.
    auto gpuStart = start, gpuCount = count;
    if (mode == 2) {
        if (count == UINT32_MAX) throw std::invalid_argument("Managed line loop index count is too large");
        std::vector<std::uint32_t> loop(std::size_t(count) + 1);
        for (std::uint32_t i = 0; i < count; ++i) loop[i] = indexValues ? (*indexValues)[std::size_t(start) + i] : start + i;
        loop[count] = loop[0];
        indices = sharedBuffer(loop.size() * sizeof(std::uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, loop.data(), true);
        indexed = true; gpuStart = 0; gpuCount = count + 1;
    }
    if (indices) draw->buffers.push_back(indices);
    managedProgramDescriptors(snapshot, *draw);
    const auto pipeline = managedProgramPipeline(snapshot, *draw->gpu, mode, vertexBindings, vertexAttributes);
    // Commit retention before emitting any command which refers to these owners.
    managedProgramPrepared.push_back(draw);
    for (const auto& buffer : draw->buffers) buffer->usedEpoch = recordingEpoch;
    colorWrite = snapshot.state.nativeGL ? snapshot.state.nativeColorMask != 0 : snapshot.state.colorWrite;
    api.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    if (!vertexBuffers.empty()) api.vkCmdBindVertexBuffers(commandBuffer, 0, std::uint32_t(vertexBuffers.size()), vertexBuffers.data(), offsets.data());
    if (!draw->descriptors.empty()) api.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        draw->gpu->layout->handle(), 0, std::uint32_t(draw->descriptors.size()), draw->descriptors.data(), 0, nullptr);
    if (indexed) {
        api.vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT32);
        api.vkCmdDrawIndexed(commandBuffer, gpuCount, instanceCount, gpuStart, 0, snapshot.firstInstance);
    } else api.vkCmdDraw(commandBuffer, gpuCount, instanceCount, gpuStart, snapshot.firstInstance);
    // The actual WebGLBufferRenderer updates source statistics after gl.draw.
}
