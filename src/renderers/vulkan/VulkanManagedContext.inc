// Native GL command protocol used by the unchanged source renderer factories.
// Application getters/callbacks finish before the draw command captures device
// state. These values are native GL state, not a second material/scene model.
std::unique_ptr<VulkanProgramCommands> managedProgramCommands;
JSValue managedGL, managedGLCanvas;
std::unique_ptr<WebGLState> managedSourceState;
std::unique_ptr<WebGLCapabilities> managedCapabilities;
std::unique_ptr<WebGLUtils> managedUtils;
std::unique_ptr<WebGLPrograms> managedPrograms;
std::unique_ptr<WebGLMaterials> managedMaterials;
std::unique_ptr<WebGLTextures> managedTextures;
std::unique_ptr<WebGLUniformsGroups> managedUniformsGroups;
std::unique_ptr<WebGLBufferRenderer> managedBufferRenderer;
std::unique_ptr<WebGLIndexedBufferRenderer> managedIndexedBufferRenderer;
std::unique_ptr<WebGLEnvironments> managedEnvironments;
std::unique_ptr<WebGLMorphtargets> managedMorphtargets;
PipelineState managedGLFixed{false,true,true,false,NoBlending,false,LessDepth,false,false,0,0};
float managedGLClearDepth = 1;
std::uint32_t managedGLError = 0;
std::unordered_map<std::uint32_t, bool> managedGLEnabled;

static std::uint32_t managedGLUint(const JSValue& value) {
    const auto number=value.toNumber();
    if(!std::isfinite(number)||number==0)return 0;
    auto result=std::fmod(std::trunc(number),4294967296.0);
    if(result<0)result+=4294967296.0;
    return static_cast<std::uint32_t>(result);
}
static std::int32_t managedGLInt(const JSValue& value) {
    return std::bit_cast<std::int32_t>(managedGLUint(value));
}
static std::int64_t managedGLInt64(const JSValue& value) {
    const auto number=value.toNumber();
    if(!std::isfinite(number)||number==0)return 0;
    auto result=std::fmod(std::trunc(number),18446744073709551616.0);
    if(result>=9223372036854775808.0)result-=18446744073709551616.0;
    if(result< -9223372036854775808.0)result+=18446744073709551616.0;
    return static_cast<std::int64_t>(result);
}
void managedNativeError(std::uint32_t error) { if(!managedGLError)managedGLError=error; }
JSValue managedGLRectangle(const Vector4& value) {
    Int32Array data(4);
    data[0]=std::int32_t(value.x);data[1]=std::int32_t(value.y);
    data[2]=std::int32_t(value.z);data[3]=std::int32_t(value.w);
    return NativeArray_detail::wrap(data);
}
JSValue managedGLParameter(std::uint32_t name) {
    VkPhysicalDeviceProperties properties{};api.vkGetPhysicalDeviceProperties(physicalDevice,&properties);
    const auto& limits=properties.limits;
    switch(name) {
        case 0x1f02:return "WebGL 2.0 (three.cpp Vulkan)";
        case 0x8b8c:return "WebGL GLSL ES 3.00 (native ESSL to SPIR-V)";
        case 0x1f00:return "three.cpp";
        case 0x1f01:return properties.deviceName;
        case 0x0ba2:return managedGLRectangle(currentViewport);
        case 0x0c10:return managedGLRectangle(currentScissor);
        case 0x8b8d:return managedProgramCommands->currentProgram();
        case 0x8869:return double(limits.maxVertexInputAttributes);
        case 0x8b4c:return double(std::min(limits.maxPerStageDescriptorSamplers,limits.maxPerStageDescriptorSampledImages));
        case 0x8dfb:case 0x8dfd:return double(limits.maxUniformBufferRange/16);
        case 0x8dfc:return double(std::min(limits.maxVertexOutputComponents,limits.maxFragmentInputComponents)/4);
        case 0x8a2f:return double(limits.maxDescriptorSetUniformBuffers);
        case 0x8a30:return double(limits.maxUniformBufferRange);
        case 0x8a34:return double(limits.minUniformBufferOffsetAlignment);
        case 0x80a9:case 0x80a8:return 0; // The actual screen framebuffer is single-sampled.
        case 0x0d57:return 0; // The actual depth image has no stencil attachment.
        case 0x0d56:return depthFormat==VK_FORMAT_D32_SFLOAT?32:16;
        case 0x0b72:return managedGLFixed.depthWrite;
        case 0x0b74:return double(managedGLFixed.nativeDepthFunc);
        case 0x0b45:return double(managedGLFixed.nativeCullFace);
        case 0x0b46:return double(managedGLFixed.nativeFrontFace);
        case 0x0b21:return managedGLFixed.nativeLineWidth;
        case 0x8894:case 0x8895:case 0x85b5:
            return managedCall(bindingCommands->object(),"getParameter")({double(name)});
        case 0x84e0:case 0x0cf5:case 0x0d05:case 0x0cf2:case 0x806e:
        case 0x0cf4:case 0x0cf3:case 0x806d:case 0x9240:case 0x9241:case 0x9243:
        case 0x0d33:case 0x851c:case 0x8073:case 0x88ff:case 0x8872:case 0x8b4d:
        case 0x8d57:case 0x8069:case 0x8514:case 0x806a:case 0x8c1d:case 0x8ca6:case 0x8caa:
            return managedCall(managedTextureCommands->object(),"getParameter")({double(name)});
        default:managedNativeError(0x0500);return nullptr;
    }
}
JSValue managedNativeGL(const std::string& name,const std::vector<JSValue>& args) {
    const auto arg=[&](std::size_t i){return i<args.size()?args[i]:JSValue{};};
    const auto uint=[&](std::size_t i){return managedGLUint(arg(i));};
    const auto real=[&](std::size_t i){return float(arg(i).toNumber());};
    if(name=="getParameter")return managedGLParameter(uint(0));
    if(name=="getError") {
        if(managedGLError){const auto error=managedGLError;managedGLError=0;return double(error);}
        for(const auto& source:{managedProgramCommands->object(),managedTextureCommands->object(),bindingCommands->object(),attributeCommands->object()}) {
            const auto error=managedCall(source,"getError")();if(error.number()!=0)return error;
        }
        return 0;
    }
    if(name=="getExtension") {arg(0).toString();return nullptr;}
    if(name=="getSupportedExtensions")return JSValue::array();
    if(name=="getContextAttributes") {
        auto result=JSValue::object();
        managedOwn(result,"alpha",managedAlpha);managedOwn(result,"premultipliedAlpha",managedPremultipliedAlpha);
        managedOwn(result,"depth",true);managedOwn(result,"stencil",false);managedOwn(result,"antialias",false);
        return result;
    }
    if(name=="getShaderPrecisionFormat") {
        const auto stage=uint(0),precision=uint(1);
        if((stage!=0x8b31&&stage!=0x8b30)||precision<0x8df0||precision>0x8df5){managedNativeError(0x0500);return nullptr;}
        const bool floating=precision<=0x8df2;
        auto result=JSValue::object();managedOwn(result,"rangeMin",floating?127:31);
        managedOwn(result,"rangeMax",floating?127:30);managedOwn(result,"precision",floating?23:0);return result;
    }
    if(name=="isContextLost")return false;
    if(name=="enable"||name=="disable"||name=="isEnabled") {
        const auto capability=uint(0);const bool enabled=name=="enable";
        if(name=="isEnabled")return managedGLEnabled[capability];
        switch(capability) {
            case 0x0b71:managedGLFixed.depthTest=enabled;break;
            case 0x0be2:managedGLFixed.nativeBlend=enabled;break;
            case 0x0b44:managedGLFixed.nativeCull=enabled;break;
            case 0x0c11:currentScissorTest=enabled;applyViewportAndScissor();break;
            case 0x8037:managedGLFixed.polygonOffset=enabled;break;
            case 0x0bd0:if(enabled)throw std::logic_error("Vulkan GL dithering is not implemented");break;
            case 0x0b90:case 0x809e:case 0x80a0:case 0x8c89:
                if(enabled)throw std::logic_error("This Vulkan GL capability is not implemented");break;
            default:managedNativeError(0x0500);return {};
        }
        managedGLEnabled[capability]=enabled;return {};
    }
    if(name=="depthMask"){managedGLFixed.depthWrite=arg(0).toBoolean();return {};}
    if(name=="colorMask") {
        std::uint32_t mask=0;for(unsigned i=0;i<4;++i)if(arg(i).toBoolean())mask|=1u<<i;
        managedGLFixed.nativeColorMask=mask;managedGLFixed.colorWrite=mask!=0;return {};
    }
    if(name=="depthFunc") {
        const auto value=uint(0);if(value<0x0200||value>0x0207){managedNativeError(0x0500);return {};}
        managedGLFixed.nativeDepthFunc=value;return {};
    }
    if(name=="frontFace") {
        const auto value=uint(0);if(value!=0x0900&&value!=0x0901){managedNativeError(0x0500);return {};}
        managedGLFixed.nativeFrontFace=value;return {};
    }
    if(name=="cullFace") {
        const auto value=uint(0);if(value!=0x0404&&value!=0x0405&&value!=0x0408){managedNativeError(0x0500);return {};}
        managedGLFixed.nativeCullFace=value;return {};
    }
    if(name=="blendEquation"||name=="blendEquationSeparate") {
        const auto rgb=uint(0),alpha=name=="blendEquation"?rgb:uint(1);
        const auto valid=[](auto v){return v==0x8006||v==0x800a||v==0x800b||v==0x8007||v==0x8008;};
        if(!valid(rgb)||!valid(alpha)){managedNativeError(0x0500);return {};}
        managedGLFixed.nativeEquationRGB=rgb;managedGLFixed.nativeEquationAlpha=alpha;return {};
    }
    if(name=="blendFunc"||name=="blendFuncSeparate") {
        const auto src=uint(0),dst=uint(1),srcAlpha=name=="blendFunc"?src:uint(2),dstAlpha=name=="blendFunc"?dst:uint(3);
        const auto valid=[](auto v){return v<=1||(v>=0x0300&&v<=0x0308)||(v>=0x8001&&v<=0x8004);};
        if(!valid(src)||!valid(dst)||!valid(srcAlpha)||!valid(dstAlpha)){managedNativeError(0x0500);return {};}
        managedGLFixed.nativeSrcRGB=src;managedGLFixed.nativeDstRGB=dst;
        managedGLFixed.nativeSrcAlpha=srcAlpha;managedGLFixed.nativeDstAlpha=dstAlpha;return {};
    }
    if(name=="blendColor"||name=="clearColor") {
        std::array<float,4> values{};for(unsigned i=0;i<4;++i)values[i]=real(i);
        if(name=="blendColor") {for(auto& value:values)value=std::clamp(value,0.0f,1.0f);managedGLFixed.nativeBlendColor=values;}
        else {activeClearColor.r=values[0];activeClearColor.g=values[1];activeClearColor.b=values[2];activeClearAlpha=values[3];activeClearSRGB=false;}
        return {};
    }
    if(name=="clearDepth"){managedGLClearDepth=std::clamp(float(arg(0).toNumber()),0.0f,1.0f);return {};}
    if(name=="clearStencil"){managedGLInt(arg(0));return {};}
    // No stencil attachment exists on this framebuffer; masks/functions still
    // accept native conversions and the source state cache owns their values.
    if(name=="stencilMask"){uint(0);return {};}
    if(name=="stencilFunc"||name=="stencilOp"){uint(0);uint(1);uint(2);return {};}
    if(name=="polygonOffset") {const auto factor=real(0),units=real(1);managedGLFixed.polygonOffsetFactor=factor;managedGLFixed.polygonOffsetUnits=units;return {};}
    if(name=="lineWidth") {
        const auto width=real(0);if(!(width>0)){managedNativeError(0x0501);return {};}
        if(width!=1)throw std::logic_error("Vulkan wideLines feature is not enabled");
        managedGLFixed.nativeLineWidth=width;return {};
    }
    if(name=="viewport"||name=="scissor") {
        const auto x=managedGLInt(arg(0)),y=managedGLInt(arg(1)),w=managedGLInt(arg(2)),h=managedGLInt(arg(3));
        if(w<0||h<0){managedNativeError(0x0501);return {};}
        auto& target=name=="viewport"?currentViewport:currentScissor;
        target.set(x,y,w,h);retainValidRectangle(target,name=="viewport"?gpuViewport:gpuScissor);applyViewportAndScissor();return {};
    }
    if(name=="clear") {
        const auto mask=uint(0);if(mask&~0x4500u){managedNativeError(0x0501);return {};}
        const auto colorMask=managedGLFixed.nativeColorMask;
        if((mask&0x4000)&&colorMask&&colorMask!=15)throw std::logic_error("Vulkan clear with a partial color mask is not implemented");
        ensureManagedRecording();
        clearAttachments((mask&0x4000)&&colorMask,(mask&0x0100)&&managedGLFixed.depthWrite,
            activeClearColor,activeClearAlpha,false,true,managedGLClearDepth);
        if(managedRenderDepth==0){endFrame();clearManagedProgramFrame();}
        return {};
    }
    if(name=="drawArrays"||name=="drawArraysInstanced"||name=="drawElements"||name=="drawElementsInstanced") {
        const bool indexed=name=="drawElements"||name=="drawElementsInstanced";
        const bool instanced=name=="drawArraysInstanced"||name=="drawElementsInstanced";
        const auto mode=uint(0);
        const auto first=indexed?0:managedGLInt(arg(1));
        const auto count=managedGLInt(arg(indexed?1:2));
        const auto type=indexed?uint(2):0;
        const auto offset=indexed?managedGLInt64(arg(3)):0;
        const auto instances=instanced?managedGLInt(arg(indexed?4:3)):1;
        if(first<0||count<0||instances<0||offset<0){managedNativeError(0x0501);return {};}
        if(mode>6){managedNativeError(0x0500);return {};}
        if(mode==0&&!largePoints)throw std::logic_error("Native Vulkan points require the largePoints device feature");
        unsigned bytes=0;
        if(indexed){bytes=type==0x1401?1:type==0x1403?2:type==0x1405?4:0;if(!bytes){managedNativeError(0x0500);return {};}}
        if(indexed&&offset%bytes!=0){managedNativeError(0x0502);return {};}
        if(!managedProgramCommands->current()){managedNativeError(0x0502);return {};}
        if(indexed&&offset/bytes>UINT32_MAX)throw std::overflow_error("Native GL index offset exceeds Vulkan draw range");
        ensureManagedRecording();
        auto snapshot=captureManagedProgramDraw(managedGLFixed,FrontSide,false,false);
        recordManagedProgramDraw(std::move(snapshot),mode,indexed?std::uint32_t(offset/bytes):std::uint32_t(first),
            std::uint32_t(count),std::uint32_t(instances),indexed,double(type));
        return {};
    }
    if(name=="flush"||name=="finish") {
        if(recording){endFrame();clearManagedProgramFrame();if(managedRenderDepth)ensureManagedRecording();}return {};
    }
    throw std::logic_error("Native Vulkan GL command is not implemented: "+name);
}

void initializeManagedContext() {
    managedProgramCommands=std::make_unique<VulkanProgramCommands>();
    initializeManagedTextureCommands();
    managedGL=JSValue::object();
    for(const auto& commands:{attributeCommands->object(),bindingCommands->object(),managedProgramCommands->object(),managedTextureCommands->object()})
        for(const auto& key:commands.ownPropertyKeys())
            managedGL.defineProperty(key,*commands.getOwnPropertyDescriptor(key));
    for(const auto& [name,token]:std::initializer_list<std::pair<const char*,std::uint32_t>>{
        {"VERSION",0x1f02},{"VENDOR",0x1f00},{"RENDERER",0x1f01},{"SHADING_LANGUAGE_VERSION",0x8b8c},
        {"CURRENT_PROGRAM",0x8b8d},{"VIEWPORT",0x0ba2},{"SCISSOR_BOX",0x0c10},
        {"MAX_VERTEX_TEXTURE_IMAGE_UNITS",0x8b4c},{"MAX_VERTEX_UNIFORM_VECTORS",0x8dfb},
        {"MAX_FRAGMENT_UNIFORM_VECTORS",0x8dfd},{"MAX_VARYING_VECTORS",0x8dfc},
        {"MAX_UNIFORM_BUFFER_BINDINGS",0x8a2f},{"MAX_UNIFORM_BLOCK_SIZE",0x8a30},{"UNIFORM_BUFFER_OFFSET_ALIGNMENT",0x8a34},
        {"LOW_FLOAT",0x8df0},{"MEDIUM_FLOAT",0x8df1},{"HIGH_FLOAT",0x8df2},
        {"LOW_INT",0x8df3},{"MEDIUM_INT",0x8df4},{"HIGH_INT",0x8df5},
        {"SAMPLES",0x80a9},{"SAMPLE_BUFFERS",0x80a8},{"STENCIL_BITS",0x0d57},{"DEPTH_BITS",0x0d56},
        {"POINTS",0},{"LINES",1},{"LINE_LOOP",2},{"LINE_STRIP",3},{"TRIANGLES",4},{"TRIANGLE_STRIP",5},{"TRIANGLE_FAN",6}})
        managedOwn(managedGL,name,double(token));
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
        {"getParameter",1},{"getError",0},{"getExtension",1},{"getSupportedExtensions",0},{"getContextAttributes",0},{"getShaderPrecisionFormat",2},{"isContextLost",0},
        {"enable",1},{"disable",1},{"isEnabled",1},{"depthMask",1},{"colorMask",4},{"depthFunc",1},{"frontFace",1},{"cullFace",1},
        {"blendEquation",1},{"blendEquationSeparate",2},{"blendFunc",2},{"blendFuncSeparate",4},{"blendColor",4},{"polygonOffset",2},{"lineWidth",1},
        {"clearColor",4},{"clearDepth",1},{"clearStencil",1},{"stencilMask",1},{"stencilFunc",3},{"stencilOp",3},{"viewport",4},{"scissor",4},{"clear",1},
        {"drawArrays",3},{"drawArraysInstanced",4},{"drawElements",4},{"drawElementsInstanced",5},{"flush",0},{"finish",0}})
        managedOwn(managedGL,name,managedService((std::string("gl.")+name).c_str(),arity));
    managedGLCanvas=JSValue::object();managedOwn(managedGLCanvas,"width",width);managedOwn(managedGLCanvas,"height",height);
    managedOwn(managedGL,"canvas",managedGLCanvas);
    managedGLFixed.nativeGL=true;
    managedExtensions=std::make_unique<WebGLExtensions>(managedGL);
    managedUtils=std::make_unique<WebGLUtils>(managedGL,managedExtensions->object());
    managedCapabilities=std::make_unique<WebGLCapabilities>(managedGL,managedExtensions->object(),JSValue::object(),managedUtils->object());
    managedSourceState=std::make_unique<WebGLState>(managedGL,managedExtensions->object());
    managedState=managedSourceState->object();
    managedTextures=std::make_unique<WebGLTextures>(managedGL,managedExtensions->object(),managedState,
        clippingProperties->object(),managedCapabilities->object(),managedUtils->object(),drawInfo->object());
    managedEnvironments=std::make_unique<WebGLEnvironments>(managedRenderer);
    managedMorphtargets=std::make_unique<WebGLMorphtargets>(managedGL,managedCapabilities->object(),managedTextures->object());
    managedPrograms=std::make_unique<WebGLPrograms>(managedRenderer,managedEnvironments->object(),managedExtensions->object(),
        managedCapabilities->object(),bindingStates->object(),clipping->object());
    managedMaterials=std::make_unique<WebGLMaterials>(managedRenderer,clippingProperties->object());
    managedUniformsGroups=std::make_unique<WebGLUniformsGroups>(managedGL,drawInfo->object(),managedCapabilities->object(),managedState);
    managedBufferRenderer=std::make_unique<WebGLBufferRenderer>(managedGL,managedExtensions->object(),drawInfo->object());
    managedIndexedBufferRenderer=std::make_unique<WebGLIndexedBufferRenderer>(managedGL,managedExtensions->object(),drawInfo->object());
}
