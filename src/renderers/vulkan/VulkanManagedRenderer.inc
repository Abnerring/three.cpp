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
bool managedDepthMask = true, managedDepthTest = true, managedPolygonOffset = false;
JSValue managedTextureUnits = 0;
JSValue managedClearScratch, managedCurrentClear, managedViewport;

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
    if (std::string(name) == "render")
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
    const auto value = managedCell("_currentViewport");
    managedCall(value, "set")({currentViewport.x, currentViewport.y, currentViewport.z, currentViewport.w});
    managedCall(managedViewport, "set")({viewport.x, viewport.y, viewport.z, viewport.w});
}
JSValue managedDispatch(const std::string& name, const JSValue& receiver, const std::vector<JSValue>& args) {
    const auto a = managedArg(args, 0), b = managedArg(args, 1);
    if (name == "render") return renderManaged(a, b, receiver);
    if (name == "renderBufferDirect") {
        if (renderTarget) throw std::logic_error("Managed draws to typed render targets are not implemented");
        if (rendering && managedRenderDepth == 0) throw std::logic_error("Mixing typed and managed draw reentry is not implemented");
        const bool outer = managedRenderDepth++ == 0;
        if (outer) rendering = true;
        try {
        if (managedCell("_clippingEnabled").strictEquals(true)) {
            const auto previousCamera = managedCell("_currentCamera");
            if (managedCell("_localClippingEnabled").strictEquals(true) || !a.strictEquals(previousCamera)) {
                const auto material = managedArg(args, 3);
                const bool useCache = a.strictEquals(previousCamera) && material.get("id").strictEquals(managedCell("_currentMaterialId"));
                clipping->setState(material, a, useCache);
            }
        }
        ensureManagedRecording();
        managedOutputSRGB = managedRenderer.get("outputColorSpace").strictEquals(SRGBColorSpace);
        renderManagedBufferDirect(a, b, managedArg(args, 2), managedArg(args, 3), managedArg(args, 4), managedArg(args, 5));
        if (outer) { if (recording) endFrame(); managedPrepared.clear(); rendering = false; }
        --managedRenderDepth;
        return {};
        } catch (...) {
            --managedRenderDepth;
            if (outer) {
                try { if (recording) endFrame(); } catch (...) {}
                abortFrame(); managedPrepared.clear(); rendering = false;
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
        const bool color = a.isUndefined() || a.toBoolean(), depth = b.isUndefined() || b.toBoolean();
        if (depth) managedDepthMask = true;
        ensureManagedRecording();
        clearAttachments(color && colorWrite, depth, activeClearColor, activeClearAlpha, false);
        if (managedRenderDepth == 0) { endFrame(); managedPrepared.clear(); }
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
    if (name == "state.viewport") {
        const auto x = a.get("x"), y = a.get("y"), z = a.get("z"), w = a.get("w");
        const auto nx = x.toNumber(), ny = y.toNumber(), nz = z.toNumber(), nw = w.toNumber();
        currentViewport.set(nx, ny, nz, nw);
        retainValidRectangle(currentViewport, gpuViewport); applyViewportAndScissor(); return {};
    }
    if (name == "state.color.setMask") { colorWrite = a.toBoolean(); return {}; }
    if (name == "state.depth.setMask") { managedDepthMask = a.toBoolean(); return {}; }
    if (name == "state.depth.setTest") { managedDepthTest = a.toBoolean(); return {}; }
    if (name == "state.setPolygonOffset") { managedPolygonOffset = a.toBoolean(); return {}; }
    if (name == "state.color.setClear") {
        auto r = a, g = b, blue = managedArg(args, 2);
        const auto alpha = managedArg(args, 3);
        if (managedArg(args, 4).strictEquals(true)) {
            const auto redValue = r.toNumber(); const auto redAlpha = alpha.toNumber(); r = redValue * redAlpha;
            const auto greenValue = g.toNumber(); const auto greenAlpha = alpha.toNumber(); g = greenValue * greenAlpha;
            const auto blueValue = blue.toNumber(); const auto blueAlpha = alpha.toNumber(); blue = blueValue * blueAlpha;
        }
        managedCall(managedClearScratch, "set")({r, g, blue, alpha});
        if (!managedCall(managedCurrentClear, "equals")({managedClearScratch}).strictEquals(false)) return {};
        // Conversion is the native clearColor command boundary; preserve the
        // source's three separate alpha coercions above and its raw cache.
        const auto red = float(r.toNumber()); const auto green = float(g.toNumber());
        const auto blueValue = float(blue.toNumber()); const auto alphaValue = float(alpha.toNumber());
        activeClearColor.r = red; activeClearColor.g = green; activeClearColor.b = blueValue;
        activeClearAlpha = alphaValue;
        managedCall(managedCurrentClear, "copy")({managedClearScratch});
        // WebGLBackground already converts working colors to the selected
        // output space before calling this native color-buffer service.
        activeClearSRGB = false; return {};
    }
    if (name == "xr.getEnvironmentBlendMode") return {};
    if (name == "xr.hasDepthSensing") return false;
    if (name == "xr.getDepthSensingMesh") return nullptr;
    if (name == "xr.updateCamera" || name == "xr.getCamera")
        throw std::logic_error("Native Vulkan XR session integration is not implemented");
    if (name == "textures.getTextureUnits") return managedTextureUnits;
    if (name == "textures.setTextureUnits") { managedTextureUnits = a; return {}; }
    if (name == "textures.resetTextureUnits") { managedTextureUnits = 0; return {}; }
    if (name == "textures.updateMultisampleRenderTarget" || name == "textures.updateRenderTargetMipmap")
        throw std::logic_error("Managed Vulkan multisample/mipmap target updates are not implemented");
    if (name == "environments.get")
        throw std::logic_error("Managed Vulkan environment texture conversion is not implemented");
    if (name == "gl.getExtension") return nullptr; // No GL extensions on a Vulkan device.
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
        {"render", 2}, {"renderBufferDirect", 6}, {"clear", 0}, {"getRenderTarget", 0},
        {"getActiveCubeFace", 0}, {"getActiveMipmapLevel", 0}, {"setRenderTarget", 1},
        {"getClearColor", 1}, {"getClearAlpha", 0}, {"setClearColor", 1}, {"setClearAlpha", 1},
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
    managedState = JSValue::object(); auto buffers = JSValue::object();
    managedClearScratch = nativeVector4Constructor().construct();
    // WebGLState initializes the GL clear color to opaque black at startup.
    managedCurrentClear = nativeVector4Constructor().construct({0, 0, 0, 1});
    managedViewport = nativeVector4Constructor().construct({viewport.x, viewport.y, viewport.z, viewport.w});
    auto color = JSValue::object(), depth = JSValue::object();
    managedOwn(color, "setClear", managedService("state.color.setClear", 5));
    managedOwn(color, "setMask", managedService("state.color.setMask", 1));
    managedOwn(depth, "setMask", managedService("state.depth.setMask", 1));
    managedOwn(depth, "setTest", managedService("state.depth.setTest", 1));
    managedOwn(buffers, "color", color); managedOwn(buffers, "depth", depth);
    managedOwn(managedState, "buffers", buffers);
    managedOwn(managedState, "viewport", managedService("state.viewport", 1));
    managedOwn(managedState, "setPolygonOffset", managedService("state.setPolygonOffset", 3));
    property("state", managedState);
    auto gl = JSValue::object(); managedOwn(gl, "getExtension", managedService("gl.getExtension", 1));
    managedExtensions = std::make_unique<WebGLExtensions>(gl);
    managedRenderLists = std::make_unique<WebGLRenderLists>();
    managedRenderStates = std::make_unique<WebGLRenderStates>(managedExtensions->object());
    auto environments = JSValue::object(); managedOwn(environments, "get", managedService("environments.get", 2));
    managedBackground = std::make_unique<WebGLBackground>(managedRenderer, environments, managedState, objects->object(), managedAlpha, managedPremultipliedAlpha);
    auto textures = JSValue::object();
    for (const char* name : {"getTextureUnits", "setTextureUnits", "resetTextureUnits", "updateMultisampleRenderTarget", "updateRenderTargetMipmap"})
        managedOwn(textures, name, managedService((std::string("textures.") + name).c_str(), name == std::string("getTextureUnits") ? 0 : 1));
    auto capabilities = JSValue::object(); managedOwn(capabilities, "samples", 0);
    auto bindings = JSValue::object();
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, JSValue>>{
        {"_this", managedRenderer}, {"objects", objects->object()}, {"renderLists", managedRenderLists->object()},
        {"renderStates", managedRenderStates->object()}, {"clipping", clipping->object()},
        {"background", managedBackground->object()}, {"shadowMap", shadow}, {"textures", textures},
        {"state", managedState}, {"xr", managedXR}, {"bindingStates", bindingStates->object()},
        {"extensions", managedExtensions->object()}, {"capabilities", capabilities}})
        managedOwn(bindings, name, value);
    managedScenePipeline = nativeWebGLRendererScenePipeline(bindings);
    managedRenderer.get("render").setFunctionSourceText(managedScenePipeline.get("render").nativeFunctionSource());
    syncManagedViewport();
    managedEmptyScene = managedCell("_emptyScene");
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
            managedPrepared.clear(); rendering = false;
        }
        return result;
    } catch (...) {
        // Vulkan owns command/resource cleanup. The source function's unfinished
        // stacks and temporary values deliberately retain their original state.
        if (managedRenderDepth) --managedRenderDepth;
        if (outer) {
            try { if (recording) endFrame(); } catch (...) { abortFrame(); }
            abortFrame();
            managedPrepared.clear(); rendering = false;
        }
        throw;
    }
}
