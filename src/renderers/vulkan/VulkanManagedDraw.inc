// Included inside VulkanRenderer::Impl. This is the raw managed resource/draw
// boundary: source traversal has already called the real WebGLObjects.update.
// No scene, geometry, material, or attribute is reconstructed as a typed facade.
bool managedOutputSRGB = true;
JSValue managedEmptyScene;
std::vector<Prepared> managedPrepared;
struct ManagedIndex {
    JSWeakValue buffer;
    std::uint64_t revision = UINT64_MAX;
    double type = 0;
    std::vector<std::uint32_t> values;
    BufferRef resource;
};
std::vector<ManagedIndex> managedIndices;

struct ManagedCall {
    JSValue receiver, function;
    JSValue operator()(const std::vector<JSValue>& args = {}) const {
        return function.call(receiver, args);
    }
};
static ManagedCall managedCall(JSValue receiver, const char* name) {
    return {receiver, receiver.get(name)};
}
static JSValue managedAdd(JSValue left, JSValue right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String) {
        const auto x = a.toString(), y = b.toString(); return x + y;
    }
    const auto x = a.toNumber(), y = b.toNumber(); return x + y;
}
static int managedEnum(const JSValue& value, const char* name) {
    const auto number = value.toNumber();
    if (!std::isfinite(number) || std::trunc(number) != number || number < INT32_MIN || number > INT32_MAX)
        throw std::invalid_argument(std::string("Unsupported managed Vulkan ") + name);
    return static_cast<int>(number);
}
static void managedOwn(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor descriptor; descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(name, descriptor);
}
JSValue managedBindingProgram(bool colors, bool vertexAlpha, bool instanced, bool instanceColors,
                              std::uint32_t planes, std::uint32_t intersections) {
    // This cache describes a shader interface. It intentionally shares programs
    // by capabilities, independently of user-controlled material/geometry IDs.
    const auto key = std::string("managed-basic:") + std::to_string(colors) + ":" +
        std::to_string(vertexAlpha) + ":" + std::to_string(instanced) + ":" +
        std::to_string(instanceColors) + ":" + std::to_string(planes) + ":" + std::to_string(intersections);
    if (const auto found = programs.find(key); found != programs.end()) return found->second;
    auto inputs = JSValue::object();
    const auto input = [&](const char* name, int location, int size) {
        auto value = JSValue::object(); managedOwn(value, "location", location); managedOwn(value, "locationSize", size);
        managedOwn(inputs, name, value);
    };
    input("position", 0, 1); if (colors) input("color", 1, 1);
    if (instanced) { input("instanceMatrix", 5, 4); if (instanceColors) input("instanceColor", 9, 1); }
    auto program = JSValue::object(); managedOwn(program, "id", double(programs.size()));
    managedOwn(program, "getAttributes", JSValue::function(+[](const JSValue&, const auto&, const auto& captures) {
        return captures[0];
    }, {inputs}));
    programs.emplace(key, program); return program;
}
ManagedIndex& managedIndex(const JSValue& buffer, double type) {
    std::erase_if(managedIndices, [&](const auto& entry) {
        const auto value = entry.buffer.lock(); return !value || !attributeCommands->hasStorage(*value);
    });
    ManagedIndex* entry = nullptr;
    for (auto& candidate : managedIndices) {
        const auto key = candidate.buffer.lock();
        if (key && key->sameValue(buffer) && candidate.type == type) { entry = &candidate; break; }
    }
    if (!entry) { managedIndices.emplace_back(); entry = &managedIndices.back(); entry->buffer = JSWeakValue(buffer); entry->type = type; }
    const auto revision = attributeCommands->revision(buffer);
    if (entry->resource && entry->revision == revision) return *entry;
    const auto values = uploadedArray(attributeCommands->read(buffer), type);
    entry->values = std::visit([](const auto& array) -> std::vector<std::uint32_t> {
        using T = typename std::decay_t<decltype(array)>::value_type;
        if constexpr (std::is_same_v<T, std::uint8_t> || std::is_same_v<T, std::uint16_t> || std::is_same_v<T, std::uint32_t>)
            return {array.begin(), array.end()};
        else throw std::invalid_argument("Managed Vulkan index requires unsigned byte, short, or int storage");
    }, values);
    if (entry->resource)
        updateBuffer(entry->resource, entry->values, {{0, entry->values.size()}}, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    else entry->resource = sharedBuffer(std::max<std::size_t>(entry->values.size(), 1) * sizeof(std::uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, entry->values.empty() ? nullptr : entry->values.data(), true);
    entry->revision = revision; return *entry;
}
void renderManagedBufferDirect(JSValue camera, JSValue scene, JSValue geometry,
                               JSValue material, JSValue object, JSValue group) {
    if (scene.isNull()) {
        if (managedEmptyScene.isUndefined()) throw std::logic_error("Source renderer empty scene is unavailable");
        scene = managedEmptyScene;
    }
    // The source queries determinant before selecting a program or a draw range.
    const bool frontFaceCW = object.get("isMesh").toBoolean() &&
        managedCall(object.get("matrixWorld"), "determinantAffine")().toNumber() < 0;
    if (!material.get("isMeshBasicMaterial").toBoolean())
        throw std::invalid_argument("Managed Vulkan rendering currently requires MeshBasicMaterial");
    if (!object.get("isMesh").toBoolean() || object.get("isSkinnedMesh").toBoolean() || object.get("isBatchedMesh").toBoolean())
        throw std::invalid_argument("Managed Vulkan Line, Points, Sprite, SkinnedMesh and BatchedMesh drawing is not implemented");
    for (const char* name : {"map", "alphaMap", "lightMap", "aoMap", "specularMap", "envMap"})
        if (material.get(name).toBoolean()) throw std::invalid_argument(std::string("Managed Vulkan raw texture binding is not implemented: ") + name);
    for (const char* name : {"alphaHash", "alphaToCoverage", "stencilWrite", "dithering"})
        if (material.get(name).toBoolean()) throw std::invalid_argument(std::string("Managed Vulkan material capability is not implemented: ") + name);
    const auto morph = geometry.get("morphAttributes");
    if (morph.get("position").toBoolean() || morph.get("color").toBoolean() || object.get("morphTexture").toBoolean())
        throw std::invalid_argument("Managed Vulkan morph resources are not implemented");
    const bool instanced = object.get("isInstancedMesh").strictEquals(true);
    const bool instanceColors = instanced && !object.get("instanceColor").isNull();
    const bool vertexColors = material.get("vertexColors").toBoolean();
    const auto colorAttribute = geometry.get("attributes").get("color");
    const bool vertexAlpha = material.get("vertexColors").strictEquals(true) && colorAttribute.toBoolean() &&
        colorAttribute.get("itemSize").strictEquals(4);
    const auto planes = clipping->numPlanes.toNumber(), intersections = clipping->numIntersection.toNumber();
    if (!std::isfinite(planes) || planes < 0 || std::floor(planes) != planes ||
        !std::isfinite(intersections) || intersections < 0 || intersections > planes || std::floor(intersections) != intersections ||
        16 + planes * 16 > maxStorageBufferRange)
        throw std::invalid_argument("Managed Vulkan clipping planes exceed device storage limits");
    const auto planeCount = static_cast<std::uint32_t>(planes), intersectionCount = static_cast<std::uint32_t>(intersections);
    const auto program = managedBindingProgram(vertexColors, vertexAlpha, instanced, instanceColors, planeCount, intersectionCount);
    const auto side = material.get("side");
    const auto sideValue = side.strictEquals(BackSide) ? BackSide : side.strictEquals(DoubleSide) ? DoubleSide : FrontSide;
    // setProgram uploads uniforms and state.setMaterial commits fixed state
    // before BindingStates.setup can upload an index and invoke onUpload.
    // Retain only the sampled GPU scalars; all source objects stay untouched.
    Prepared draw{};
    TextureUniforms uniforms;
    draw.colorWrite = material.get("colorWrite").toBoolean();
    const bool wireframe = material.get("wireframe").strictEquals(true);
    const auto modelView = object.get("modelViewMatrix").get("elements"), projection = camera.get("projectionMatrix").get("elements");
    std::array<double,16> view{}, clip{};
    for (unsigned i=0;i<16;++i) {view[i]=modelView.get(JSValue(double(i))).toNumber(); uniforms.modelView[i]=float(view[i]);}
    for (unsigned i=0;i<16;++i) clip[i]=projection.get(JSValue(double(i))).toNumber();
    // Camera projection metadata selects its clip-space range. Depth compare
    // and clear conventions belong to the renderer's independent device state.
    const bool cameraReversed = camera.get("reversedDepth").toBoolean();
    const bool reversed = managedReversedDepth;
    const bool openGL = camera.get("coordinateSystem").strictEquals(WebGLCoordinateSystem) && !cameraReversed;
    for (unsigned col=0;col<4;++col) { std::array<double,4> value{};
        for(unsigned row=0;row<4;++row)value[row]=((clip[row]*view[col*4]+clip[4+row]*view[col*4+1])+clip[8+row]*view[col*4+2])+clip[12+row]*view[col*4+3];
        draw.constants.mvp[col*4]=float(value[0]);draw.constants.mvp[col*4+1]=float(-value[1]);
        draw.constants.mvp[col*4+2]=float(openGL?value[2]*.5+value[3]*.5:value[2]);draw.constants.mvp[col*4+3]=float(value[3]);
    }
    const auto color = material.get("color");
    draw.constants.diffuse[0]=float(color.get("r").toNumber());draw.constants.diffuse[1]=float(color.get("g").toNumber());draw.constants.diffuse[2]=float(color.get("b").toNumber());draw.constants.diffuse[3]=float(material.get("opacity").toNumber());
    draw.constants.settings[0]=float(material.get("alphaTest").toNumber());
    draw.constants.settings[1]=material.get("transparent").strictEquals(false)&&material.get("blending").strictEquals(NormalBlending)&&material.get("alphaToCoverage").strictEquals(false)?1.f:0.f;
    draw.constants.settings[2]=material.get("premultipliedAlpha").toBoolean()?1.f:0.f;
    const bool outputSRGB=managedOutputSRGB&&!renderTarget&&active->colorFormat!=VK_FORMAT_R16G16B16A16_SFLOAT;
    draw.constants.settings[3]=outputSRGB?1.f:0.f;
    const auto fog=scene.get("fog");const bool fogEnabled=fog.toBoolean()&&material.get("fog").toBoolean();
    if(fogEnabled){
        if(fog.get("isFogExp2").toBoolean()){draw.constants.fogParams[0]=2;draw.constants.fogParams[1]=float(fog.get("density").toNumber());}
        else if(fog.get("isFog").toBoolean()){draw.constants.fogParams[0]=1;draw.constants.fogParams[1]=float(fog.get("near").toNumber());draw.constants.fogParams[2]=float(fog.get("far").toNumber());}
        else throw std::invalid_argument("Unknown managed scene fog type");
        const auto fogColor=fog.get("color");for(unsigned i=0;i<3;++i){const auto component=fogColor.get(i==0?"r":i==1?"g":"b").toNumber();draw.constants.fogColor[i]=outputSRGB?linearToSRGB(component):float(component);}
    }
    const PipelineState state{material.get("depthTest").toBoolean(),material.get("depthWrite").toBoolean(),draw.colorWrite,material.get("transparent").toBoolean(),
        managedEnum(material.get("blending"),"blending"),material.get("premultipliedAlpha").toBoolean(),managedEnum(material.get("depthFunc"),"depthFunc"),wireframe,
        material.get("polygonOffset").toBoolean(),material.get("polygonOffsetFactor").toNumber(),material.get("polygonOffsetUnits").toNumber()};
    if((state.blending==SubtractiveBlending||state.blending==MultiplyBlending)&&!state.premultipliedAlpha)
        throw std::invalid_argument("three.js requires premultipliedAlpha for subtractive and multiply blending");
    std::vector<std::byte> clippingData(16+planeCount*16);const std::uint32_t header[]={planeCount,planeCount-intersectionCount,0,0};std::memcpy(clippingData.data(),header,sizeof(header));
    if(planeCount){const auto source=NativeArray_detail::unwrap(clipping->uniform.value().get("value"));const auto* values=std::get_if<Float32Array>(&source.storage());
        if(!values||values->size()<planeCount*4)throw JSTypeError("Managed clipping uniform has no projected Float32 planes");
        for(std::size_t i=0;i<planeCount*4;++i){const float value=(*values)[i];std::memcpy(clippingData.data()+16+i*sizeof(float),&value,sizeof(value));}
    }
    managedCell("_currentCamera",camera);
    managedCell("_currentMaterialId",material.get("id"));
    colorWrite = draw.colorWrite;
    auto index = geometry.get("index");
    double rangeFactor = 1;
    if (wireframe) { index = geometries->getWireframeAttribute(geometry); if (index.isUndefined()) return; rangeFactor = 2; }
    const auto drawRange = geometry.get("drawRange"), position = geometry.get("attributes").get("position");
    double drawStart = drawRange.get("start").toNumber() * rangeFactor;
    const auto rawStart = drawRange.get("start"), rawCount = drawRange.get("count");
    double drawEnd = managedAdd(rawStart, rawCount).toNumber() * rangeFactor;
    if (!group.isNull()) {
        drawStart = managedCall(nativeGlobalBinding("Math"), "max")({drawStart, group.get("start").toNumber() * rangeFactor}).toNumber();
        const auto start = group.get("start"), count = group.get("count");
        drawEnd = managedCall(nativeGlobalBinding("Math"), "min")({drawEnd, managedAdd(start, count).toNumber() * rangeFactor}).toNumber();
    }
    if (!index.isNull()) {
        drawStart = managedCall(nativeGlobalBinding("Math"), "max")({drawStart, 0}).toNumber();
        drawEnd = managedCall(nativeGlobalBinding("Math"), "min")({drawEnd, index.get("count")}).toNumber();
    } else if (!position.isUndefined() && !position.isNull()) {
        drawStart = managedCall(nativeGlobalBinding("Math"), "max")({drawStart, 0}).toNumber();
        drawEnd = managedCall(nativeGlobalBinding("Math"), "min")({drawEnd, position.get("count")}).toNumber();
    }
    const auto drawCount = drawEnd - drawStart;
    if (drawCount < 0 || drawCount == std::numeric_limits<double>::infinity()) return;
    // Do not move setup above the source's range return. It owns the actual
    // index upload, VAO capture, divisor calculation, and onUpload callbacks.
    bindingStates->setup(object, material, program, geometry, index);
    auto indexData = JSValue{}, indexType = JSValue{}, indexBytes = JSValue{};
    if (!index.isNull()) {
        indexData = attributes->get(index);
        // WebGLIndexedBufferRenderer.setIndex captures these properties before
        // selecting the primitive mode or resolving the instance count.
        indexType = indexData.get("type"); indexBytes = indexData.get("bytesPerElement");
    }
    if (wireframe && material.get("wireframeLinewidth").toNumber() != 1)
        throw std::invalid_argument("Managed Vulkan wide wireframe lines are not implemented");
    draw.indexed = !index.isNull(); draw.wireframe = wireframe;
    draw.logicalCount = drawCount;
    if (!std::isfinite(drawStart) || !std::isfinite(drawCount) || drawStart < 0 || drawStart > UINT32_MAX || drawCount > UINT32_MAX)
        throw std::invalid_argument("Managed geometry exceeds Vulkan draw limits");
    if (draw.indexed && std::floor(drawStart) != drawStart)
        throw std::invalid_argument("Managed indexed draw offset is not aligned to index elements");
    draw.start = static_cast<std::uint32_t>(drawStart); draw.count = static_cast<std::uint32_t>(drawCount);
    if (instanced) draw.logicalInstanceCount = object.get("count");
    else if (geometry.get("isInstancedBufferGeometry").toBoolean()) {
        const auto available = geometry.get("_maxInstanceCount");
        const auto maximum = !available.isUndefined() ? geometry.get("_maxInstanceCount") : JSValue(std::numeric_limits<double>::infinity());
        draw.logicalInstanceCount = managedCall(nativeGlobalBinding("Math"), "min")({geometry.get("instanceCount"), maximum});
    }
    draw.skipInstances = draw.logicalInstanceCount.strictEquals(0);
    draw.instanceCount = draw.skipInstances ? 0 : unsignedLong(draw.logicalInstanceCount.toNumber());
    if (draw.instanceCount > INT32_MAX) throw std::invalid_argument("Negative WebGL instance count cannot be submitted to Vulkan");
    const auto bindings = bindingCommands->snapshot();
    const auto invalidDraw = [&]() {
        bindingCommands->invalidDraw(); draw.invalidBindings = true;
        managedPrepared.push_back(std::move(draw)); recordDraw(managedPrepared.back());
    };
    for (unsigned location = 0; location < bindings.attributes.size(); ++location) {
        const bool activeInput = location == 0 || (location == 1 && vertexColors) ||
            (instanced && location >= 5 && location <= 8) || (location == 9 && instanceColors);
        const auto& input = bindings.attributes[location];
        if (activeInput && input.enabled && (!attributeCommands->hasStorage(input.buffer) || input.integer)) { invalidDraw(); return; }
    }
    if (draw.indexed && !attributeCommands->hasStorage(bindings.index)) { invalidDraw(); return; }
    const std::vector<std::uint32_t>* indices = nullptr;
    if (draw.indexed) {
        const auto type = indexType.toNumber();
        auto& cached = managedIndex(bindings.index, type); indices = &cached.values; draw.indices = cached.resource;
        if (draw.start > indices->size() || draw.count > indices->size() - draw.start) { invalidDraw(); return; }
    }
    const auto positionCache = boundInput(bindings.attributes[0], std::max(3u, bindings.attributes[0].size));
    const auto colorCache = vertexColors ? boundInput(bindings.attributes[1], vertexAlpha ? 4 : 3) : nullptr;
    if (!emptyStorage) { constexpr float values[] = {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1}; emptyStorage = sharedBuffer(sizeof(values), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, values); }
    draw.instanceMatrices = draw.instanceColors = draw.morphPositions = emptyStorage;
    std::shared_ptr<AttributeCache> matrixCache, instanceColorCache;
    if (instanced) {
        matrixCache = boundMatrix(bindings); instanceColorCache = instanceColors ? boundInput(bindings.attributes[9], 3) : nullptr;
        draw.instanceMatrices = matrixCache->vertices[false];
        if (instanceColorCache) draw.instanceColors = instanceColorCache->vertices[false];
        uniforms.instance[0] = 1; uniforms.instance[1] = instanceColors ? 1 : 0;
        uniforms.instance[2] = bindings.attributes[5].divisor; uniforms.instance[3] = bindings.attributes[9].divisor;
        uniforms.instanceLayout[0] = 16; uniforms.instanceLayout[1] = instanceColorCache ? 3 : 0;
        for (unsigned col = 0; col < 4; ++col) { uniforms.matrixDivisors[col] = bindings.attributes[5+col].divisor; uniforms.matrixEnabled[col] = bindings.attributes[5+col].enabled; }
        if (!bindings.attributes[9].enabled) uniforms.instanceLayout[1] = 0;
    }
    const auto available = [](const auto& snapshot, std::size_t components) {
        if (snapshot.constantInput) return SIZE_MAX;
        if (snapshot.boundView) return std::size_t(snapshot.count);
        const auto count = componentCount(snapshot); return count < components ? std::size_t(0) :
            std::size_t(std::floor(double(count-components)/snapshot.itemSize)) + 1;
    };
    const auto row = [&](unsigned location, std::size_t vertex, std::uint32_t instance) {
        const auto& input = bindings.attributes[location];
        return !input.enabled ? std::size_t(0) : input.divisor ? std::size_t(instance/input.divisor) : vertex;
    };
    std::size_t vertexCount = 0; bool invalid = false;
    const auto fetchCount = draw.instanceCount == 0 ? 0 : draw.count;
    for (std::size_t offset = 0; offset < fetchCount; ++offset) {
        const auto vertex = indices ? (*indices)[std::size_t(draw.start)+offset] : std::size_t(draw.start)+offset;
        const auto instance = draw.instanceCount ? draw.instanceCount-1 : 0;
        if (row(0,vertex,instance) >= available(*positionCache->snapshot,3) ||
            (colorCache && row(1,vertex,instance) >= available(*colorCache->snapshot,vertexAlpha?4:3))) invalid = true;
        vertexCount = std::max(vertexCount,vertex+1);
    }
    if (instanced && fetchCount) {
        const auto check = [&](const auto& cache, std::uint32_t divisor, unsigned components) {
            if (!cache) return;
            const auto rows = divisor == 0 ? vertexCount : (std::uint64_t(draw.instanceCount)-1)/divisor+1;
            if (rows > (cache->snapshot->boundView ? cache->snapshot->count : componentCount(*cache->snapshot)/components)) invalid = true;
        };
        for (const auto divisor : uniforms.matrixDivisors) check(matrixCache,divisor,16);
        check(instanceColorCache,bindings.attributes[9].divisor,3);
    }
    if (invalid) { invalidDraw(); return; }
    if (!whiteColor) { constexpr float value[] = {1,1,1,1}; whiteColor = sharedBuffer(sizeof(value), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,value); }
    if (!noFog) { constexpr float value = 0; noFog = sharedBuffer(sizeof(value),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,&value); }
    if (!zeroUv) { constexpr float value[] = {0,0}; zeroUv = sharedBuffer(sizeof(value),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,value); }
    draw.positions = positionCache->vertices[false]; draw.colors = colorCache ? colorCache->vertices[false] : whiteColor;
    draw.fog = noFog; draw.mapUv = draw.alphaUv = zeroUv;
    draw.map = draw.alpha = draw.morphWeights = fallbackTexture(true);
    for (const auto& buffer : {draw.positions,draw.colors,draw.instanceMatrices,draw.instanceColors,draw.morphPositions})
        if (buffer->size > maxStorageBufferRange) throw std::invalid_argument("Managed vertex storage exceeds Vulkan limits");
    const std::array<unsigned,4> locations{0,1,3,4};
    const std::array<std::uint32_t,4> strides{std::uint32_t(positionCache->snapshot->itemSize),colorCache?std::uint32_t(colorCache->snapshot->itemSize):4u,2u,2u};
    for (unsigned i = 0; i < 4; ++i) { const auto& input = bindings.attributes[locations[i]];
        uniforms.inputLayout[i][0] = strides[i]; uniforms.inputLayout[i][1] = input.divisor;
        uniforms.inputLayout[i][2] = input.enabled && (i == 0 || (i == 1 && bool(colorCache)));
    }
    uniforms.inputLayout[1][3] = bindings.attributes[9].enabled;
    const auto positionStride=positionCache->snapshot->itemSize*sizeof(float);
    if(positionStride>UINT32_MAX)throw std::invalid_argument("Managed position itemSize exceeds Vulkan limits");
    draw.pipeline=pipeline(state,sideValue,frontFaceCW,reversed,std::uint32_t(positionStride),colorCache?std::uint32_t(colorCache->snapshot->itemSize):4,
        colorCache&&bindings.attributes[1].enabled,fogEnabled,2,2,false,false);
    const auto encoded=encodeTextureUniforms(uniforms);draw.textureUniforms=sharedBuffer(encoded.bytes().size(),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,encoded.bytes().data());
    draw.clippingPlanes=sharedBuffer(clippingData.size(),VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,clippingData.data());
    draw.descriptor=textureDescriptor(draw.map,draw.alpha,draw.textureUniforms,draw.instanceMatrices,draw.instanceColors,draw.morphPositions,draw.morphWeights,
        draw.positions,draw.colors,draw.mapUv,draw.alphaUv,draw.clippingPlanes);
    managedPrepared.push_back(std::move(draw));recordDraw(managedPrepared.back());
}
