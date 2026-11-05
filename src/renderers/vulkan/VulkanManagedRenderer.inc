// Included inside VulkanRenderer::Impl after VulkanManagedDraw.inc. The source
// scene/background factories retain their original raw objects and callbacks.
// These services implement the native device boundary. Unsupported device
// features fail at that boundary instead of replacing objects with typed copies.
JSValue managedRenderer, managedScenePipeline, managedState, managedXR;
std::unique_ptr<WebGLRenderLists> managedRenderLists;
std::unique_ptr<WebGLRenderStates> managedRenderStates;
std::unique_ptr<WebGLBackground> managedBackground;
std::unique_ptr<WebGLExtensions> managedExtensions;
unsigned managedRenderDepth = 0;
bool managedAlpha = false, managedPremultipliedAlpha = true;
bool managedReversedDepth = false;
JSValue managedViewport;

static JSValue managedArg(const std::vector<JSValue>& args, std::size_t i) {
    return i < args.size() ? args[i] : JSValue{};
}
JSValue managedCell(const char* name) {
    return managedScenePipeline.getInternalSlot("WebGLRendererScene.get").call({}, {name});
}
void managedCell(const char* name, JSValue value) {
    managedScenePipeline.getInternalSlot("WebGLRendererScene.set").call({}, {name, value});
}
JSValue managedService(const char* name, unsigned arity = 0) {
    const auto token = JSValue::hostObject(lifecycle, lifecycle.get(), "VulkanRenderer.Lifecycle");
    const JSValue::NativeFunction callback = +[](const JSValue& receiver, const auto& args, const auto& captures) -> JSValue {
        const auto lifetime = std::static_pointer_cast<Lifecycle>(captures[0].hostPayload("VulkanRenderer.Lifecycle"));
        if (!lifetime || !lifetime->owner) throw JSTypeError("VulkanRenderer has been disposed");
        return lifetime->owner->managedDispatch(captures[1].string(), receiver, args);
    };
    if (std::string(name).find('.') == std::string::npos)
        return JSValue::constructor(callback, {token, name}, "", arity, JSValue::ConstructorKind::Callable);
    return JSValue::function(callback, {token, name}, name, arity);
}
void ensureManagedRecording() {
    reversedDepth = managedReversedDepth;
    if (!recording) beginFrame(false, false, activeClearColor, activeClearAlpha, false);
}
void syncManagedViewport() {
    if (managedScenePipeline.isUndefined()) return;
    managedCell("_height", height);
    managedGLCanvas.set("width",width);managedGLCanvas.set("height",height);
    const auto value = managedCell("_currentViewport");
    managedCall(value, "set")({currentViewport.x, currentViewport.y, currentViewport.z, currentViewport.w});
    managedCall(managedViewport, "set")({viewport.x, viewport.y, viewport.z, viewport.w});
}
JSValue managedDispatch(const std::string& name, const JSValue& receiver, const std::vector<JSValue>& args) {
    if(name.starts_with("gl."))return managedNativeGL(name.substr(3),args);
    const auto a = managedArg(args, 0), b = managedArg(args, 1);
    if(name=="getContext")return managedGL;
    if (name == "render") return renderManaged(a, b, receiver);
    if (name == "renderBufferDirect") {
        if (renderTarget) throw std::logic_error("Managed draws to typed render targets are not implemented");
        if (rendering && managedRenderDepth == 0) throw std::logic_error("Mixing typed and managed draw reentry is not implemented");
        const bool outer = managedRenderDepth++ == 0;
        if (outer) rendering = true;
        try {
        managedScenePipeline.get("renderBufferDirect").call(receiver,args);
        if (outer) { if (recording) endFrame(); clearManagedProgramFrame(); rendering = false; }
        --managedRenderDepth;
        return {};
        } catch (...) {
            --managedRenderDepth;
            if (outer) {
                try { if (recording) endFrame(); } catch (...) {}
                abortFrame(); clearManagedProgramFrame(); rendering = false;
            }
            throw;
        }
    }
    if (name == "getRenderTarget") return managedCell("_currentRenderTarget");
    if (name == "getActiveCubeFace") return managedCell("_currentActiveCubeFace");
    if (name == "getActiveMipmapLevel") return managedCell("_currentActiveMipmapLevel");
    if (name == "setRenderTarget") {
        if (!a.isNull()) throw std::logic_error("Managed Vulkan render-target allocation is not implemented");
        if (renderTarget) throw std::logic_error("Switching typed targets through the managed renderer is not implemented");
        const auto face = b.isUndefined() ? JSValue(0) : b;
        const auto mipArg = managedArg(args, 2), mip = mipArg.isUndefined() ? JSValue(0) : mipArg;
        if (!face.strictEquals(0) || !mip.strictEquals(0)) throw std::logic_error("Managed Vulkan target face/mipmap is not implemented");
        managedCell("_currentRenderTarget", nullptr);
        managedCell("_currentActiveCubeFace", face); managedCell("_currentActiveMipmapLevel", mip);
        return {};
    }
    if (name == "clear") {
        const auto color=a.isUndefined()?JSValue(true):a;
        const auto depth=b.isUndefined()?JSValue(true):b;
        const auto stencilArg=managedArg(args,2),stencil=stencilArg.isUndefined()?JSValue(true):stencilArg;
        std::uint32_t bits=0;
        if(color.toBoolean())bits|=0x4000;
        if(depth.toBoolean()){bits|=0x0100;managedCall(receiver.get("state").get("buffers").get("depth"),"setMask")({true});}
        if(stencil.toBoolean()){bits|=0x0400;managedCall(receiver.get("state").get("buffers").get("stencil"),"setMask")({4294967295.0});}
        if(bits)managedCall(managedGL,"clear")({double(bits)});
        return {};
    }
    if (name == "setClearColor") return managedBackground->setClearColor(a, b.isUndefined() ? JSValue(1) : b);
    if (name == "setClearAlpha") return managedBackground->setClearAlpha(a);
    if (name == "getClearColor") return managedCall(a, "copy")({managedBackground->getClearColor()});
    if (name == "getClearAlpha") return managedBackground->getClearAlpha();
    if (name == "getSize") return managedCall(a, "set")({double(width), double(height)});
    if (name == "getDrawingBufferSize") {
        auto result = managedCall(a, "set")({double(width), double(height)});
        return managedCall(result, "floor")();
    }
    if (name == "getViewport" || name == "getCurrentViewport") {
        const auto view = name == "getViewport" ? managedViewport : managedCell("_currentViewport");
        return managedCall(a, "copy")({view});
    }
    if (name == "getPixelRatio") return 1;
    if (name == "xr.getEnvironmentBlendMode") return {};
    if (name == "xr.hasDepthSensing") return false;
    if (name == "xr.getDepthSensingMesh") return nullptr;
    if (name == "xr.updateCamera" || name == "xr.getCamera")
        throw std::logic_error("Native Vulkan XR session integration is not implemented");
    if (name == "shadowMap.render") {
        const auto shadow = managedRenderer.get("shadowMap");
        // Original WebGLShadowMap early exits, before any GPU work.
        if (shadow.get("enabled").strictEquals(false)) return {};
        if (shadow.get("autoUpdate").strictEquals(false) && shadow.get("needsUpdate").strictEquals(false)) return {};
        if (a.get("length").strictEquals(0)) return {};
        throw std::logic_error("Managed Vulkan shadow rendering is not implemented");
    }
    throw std::logic_error("Unknown managed Vulkan service: " + name);
}
void initializeManagedRenderer() {
    if (!managedRenderer.isUndefined()) return;
#ifndef THREECPP_HAS_NATIVE_ESSL
    throw std::logic_error("Source GLSL rendering requires THREECPP_NATIVE_ESSL=ON");
#endif
    if (renderTarget) throw std::logic_error("Initialize the managed renderer with the screen target bound");
    managedRenderer = JSValue::object();
    const auto property = [&](const char* name, JSValue value) { managedOwn(managedRenderer, name, value); };
    property("isWebGLRenderer", true); property("isVulkanRenderer", true);
    property("autoClear", true); property("autoClearColor", true); property("autoClearDepth", true);
    property("autoClearStencil", true); property("sortObjects", true);
    property("localClippingEnabled", false); property("clippingPlanes", JSValue::array());
    property("coordinateSystem", WebGLCoordinateSystem); property("info", drawInfo->object());
    property("toneMapping", NoToneMapping); property("toneMappingExposure", 1);
    property("transmissionResolutionScale", 1); property("outputColorSpace", SRGBColorSpace);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
        {"render", 2}, {"renderBufferDirect", 6}, {"clear", 0}, {"getRenderTarget", 0}, {"getContext", 0},
        {"getActiveCubeFace", 0}, {"getActiveMipmapLevel", 0}, {"setRenderTarget", 1},
        {"getClearColor", 1}, {"getClearAlpha", 0}, {"setClearColor", 0}, {"setClearAlpha", 0},
        {"getSize", 1}, {"getDrawingBufferSize", 1}, {"getViewport", 1}, {"getCurrentViewport", 1}, {"getPixelRatio", 0}})
        property(name, managedService(name, arity));
    managedXR = JSValue::object(); managedOwn(managedXR, "enabled", false);
    managedOwn(managedXR, "isPresenting", false); managedOwn(managedXR, "cameraAutoUpdate", true);
    for (const char* name : {"getEnvironmentBlendMode", "hasDepthSensing", "getDepthSensingMesh", "updateCamera", "getCamera"})
        managedOwn(managedXR, name, managedService((std::string("xr.") + name).c_str()));
    property("xr", managedXR);
    auto shadow = JSValue::object(); managedOwn(shadow, "enabled", false); managedOwn(shadow, "autoUpdate", true);
    managedOwn(shadow, "needsUpdate", false); managedOwn(shadow, "type", PCFShadowMap);
    managedOwn(shadow, "render", managedService("shadowMap.render", 3)); property("shadowMap", shadow);
    auto debug=JSValue::object();managedOwn(debug,"checkShaderErrors",true);managedOwn(debug,"onShaderError",nullptr);property("debug",debug);
    managedViewport = nativeVector4Constructor().construct({viewport.x, viewport.y, viewport.z, viewport.w});
    initializeManagedContext();
    property("domElement",managedGLCanvas);property("state",managedState);
    property("capabilities",managedCapabilities->object());property("extensions",managedExtensions->object());
    property("properties",clippingProperties->object());
    auto infoObject=drawInfo->object();infoObject.set("programs",managedPrograms->object().get("programs"));
    managedRenderLists=std::make_unique<WebGLRenderLists>();
    managedRenderStates=std::make_unique<WebGLRenderStates>(managedExtensions->object());
    managedBackground=std::make_unique<WebGLBackground>(managedRenderer,managedEnvironments->object(),managedState,objects->object(),managedAlpha,managedPremultipliedAlpha);
    auto bindings = JSValue::object();
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, JSValue>>{
        {"_this", managedRenderer}, {"objects", objects->object()}, {"renderLists", managedRenderLists->object()},
        {"renderStates", managedRenderStates->object()}, {"clipping", clipping->object()},
        {"background", managedBackground->object()}, {"shadowMap", shadow}, {"textures", managedTextures->object()},
        {"state", managedState}, {"xr", managedXR}, {"bindingStates", bindingStates->object()},
        {"extensions", managedExtensions->object()}, {"capabilities", managedCapabilities->object()},
        {"_gl",managedGL},{"properties",clippingProperties->object()},{"programCache",managedPrograms->object()},
        {"environments",managedEnvironments->object()},{"materials",managedMaterials->object()},
        {"morphtargets",managedMorphtargets->object()},{"uniformsGroups",managedUniformsGroups->object()},
        {"geometries",geometries->object()},{"attributes",managedAttributes},
        {"bufferRenderer",managedBufferRenderer->object()},{"indexedBufferRenderer",managedIndexedBufferRenderer->object()},
        {"getDFGLUT",nativeGetDFGLUTFunction()},{"_height",height},{"stencil",false}})
        managedOwn(bindings, name, value);
    managedScenePipeline = nativeWebGLRendererScenePipeline(bindings);
    managedRenderer.get("render").setFunctionSourceText(managedScenePipeline.get("render").nativeFunctionSource());
    syncManagedViewport();
    managedRenderer.get("renderBufferDirect").setFunctionSourceText(managedScenePipeline.get("renderBufferDirect").nativeFunctionSource());
}
JSValue renderManaged(JSValue scene, JSValue camera, JSValue receiver) {
    initializeManagedRenderer();
    if (renderTarget) throw std::logic_error("Managed rendering to typed render targets is not implemented");
    if (rendering && managedRenderDepth == 0) throw std::logic_error("Mixing typed and managed render reentry is not implemented");
    const bool outer = managedRenderDepth++ == 0;
    if (outer) {
        rendering = true;
        pruneBoundInputs();
        std::erase_if(managedIndices, [&](const auto& entry) {
            const auto key = entry.buffer.lock(); return !key || !attributeCommands->hasStorage(*key);
        });
    }
    try {
        const auto result = managedScenePipeline.get("render").call(receiver, {scene, camera});
        --managedRenderDepth;
        if (outer) {
            if (recording) endFrame();
            clearManagedProgramFrame(); rendering = false;
        }
        return result;
    } catch (...) {
        // Vulkan owns command/resource cleanup. The source function's unfinished
        // stacks and temporary values deliberately retain their original state.
        if (managedRenderDepth) --managedRenderDepth;
        if (outer) {
            try { if (recording) endFrame(); } catch (...) { abortFrame(); }
            abortFrame();
            clearManagedProgramFrame(); rendering = false;
        }
        throw;
    }
}
