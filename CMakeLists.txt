cmake_minimum_required(VERSION 3.20)
project(THREECPP VERSION 0.1.0 LANGUAGES CXX)
include(src/renderers/shaders/ShaderSources.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <charconv>
int main() {
    char text[64] = {'1'};
    double value = 0;
    const auto parsed = std::from_chars(text, text + 1, value, std::chars_format::general);
    const auto formatted = std::to_chars(text, text + sizeof(text), value, std::chars_format::general);
    return parsed.ec == formatted.ec ? 0 : 1;
}" THREECPP_HAS_FLOATING_CHARCONV)
if(NOT THREECPP_HAS_FLOATING_CHARCONV)
    message(FATAL_ERROR "three.cpp requires a C++20 standard library with floating-point std::from_chars and std::to_chars. Select a toolchain with floating-point charconv support.")
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
option(THREECPP_BUILD_TESTS "Build CPU tests and three.js differential checks" ON)
option(THREECPP_BUILD_VULKAN "Build the Vulkan renderer and rendering checks" ON)
option(THREECPP_BUILD_ADDONS "Build ported examples/jsm addons" ON)
set(THREEJS_SOURCE_DIR "" CACHE PATH "Pinned upstream three.js checkout for differential tests")
find_package(Threads REQUIRED)

add_library(threecpp
    src/math/MathUtils.cpp src/math/Vector2.cpp src/math/Vector3.cpp src/math/Vector4.cpp
    src/math/Matrix3.cpp src/math/Matrix4.cpp src/math/Quaternion.cpp src/math/Euler.cpp
    src/math/Color.cpp src/math/ColorManagement.cpp src/math/Box3.cpp src/math/Sphere.cpp
    src/math/Box2.cpp src/math/Cylindrical.cpp src/math/Spherical.cpp src/math/Matrix2.cpp
    src/math/Plane.cpp src/math/Line3.cpp src/math/Triangle.cpp src/math/Ray.cpp src/math/Frustum.cpp
    src/math/Interpolant.cpp src/math/JSNumberMath.cpp
    src/math/interpolants/DiscreteInterpolant.cpp src/math/interpolants/LinearInterpolant.cpp
    src/math/interpolants/CubicInterpolant.cpp src/math/interpolants/QuaternionLinearInterpolant.cpp
    src/math/interpolants/BezierInterpolant.cpp
    src/core/EventDispatcher.cpp src/core/Object3D.cpp src/core/JSValue.cpp src/core/NativeObject.cpp
    src/audio/AudioContext.cpp
    src/audio/PositionalAudio.cpp
    src/audio/AudioAnalyser.cpp
    src/audio/AudioListener.cpp
    src/audio/Audio.cpp
    src/core/NativeRenderingObject.cpp src/core/Layers.cpp src/core/JSObject.cpp src/core/JSNumber.cpp
    src/core/JSWeakRef.cpp src/core/JSFinalizationRegistry.cpp
    src/core/BufferAttribute.cpp src/core/BufferGeometry.cpp src/core/JSDataSnapshot.cpp
    src/core/Raycaster.cpp
    src/cameras/Camera.cpp src/cameras/PerspectiveCamera.cpp src/cameras/OrthographicCamera.cpp
    src/cameras/CubeCamera.cpp
    src/scenes/Scene.cpp src/geometries/PlaneGeometry.cpp src/geometries/BoxGeometry.cpp
    src/geometries/CircleGeometry.cpp
    src/geometries/RingGeometry.cpp
    src/geometries/SphereGeometry.cpp
    src/geometries/CylinderGeometry.cpp
    src/geometries/ConeGeometry.cpp
    src/geometries/TorusGeometry.cpp
    src/geometries/TorusKnotGeometry.cpp
    src/geometries/LatheGeometry.cpp
    src/geometries/CapsuleGeometry.cpp
    src/geometries/PolyhedronGeometry.cpp
    src/geometries/TetrahedronGeometry.cpp
    src/geometries/OctahedronGeometry.cpp
    src/geometries/IcosahedronGeometry.cpp
    src/geometries/DodecahedronGeometry.cpp
    src/geometries/ShapeGeometry.cpp
    src/geometries/ExtrudeGeometry.cpp
    src/geometries/TubeGeometry.cpp
    src/geometries/EdgesGeometry.cpp
    src/geometries/WireframeGeometry.cpp
    src/materials/Material.cpp src/materials/MeshBasicMaterial.cpp src/objects/Mesh.cpp
    src/textures/TextureSource.cpp src/textures/Texture.cpp src/textures/DataTexture.cpp
    src/textures/CanvasTexture.cpp
    src/textures/VideoTexture.cpp
    src/textures/VideoFrameTexture.cpp
    src/textures/HTMLTexture.cpp
    src/textures/DataArrayTexture.cpp src/textures/Data3DTexture.cpp src/textures/CubeTexture.cpp
    src/textures/DepthTexture.cpp src/textures/CubeDepthTexture.cpp src/textures/FramebufferTexture.cpp
    src/textures/CompressedTexture.cpp src/textures/CompressedArrayTexture.cpp
    src/textures/CompressedCubeTexture.cpp src/textures/ExternalTexture.cpp
    src/animation/AnimationUtils.cpp src/animation/AnimationJSON.cpp src/animation/KeyframeTrack.cpp
    src/animation/AnimationClip.cpp src/animation/PropertyMixer.cpp src/animation/PropertyBinding.cpp
    src/animation/AnimationAction.cpp src/animation/AnimationMixer.cpp
    src/animation/AnimationObjectGroup.cpp src/animation/AnimationRuntime.cpp
    src/animation/tracks/BooleanKeyframeTrack.cpp src/animation/tracks/ColorKeyframeTrack.cpp
    src/animation/tracks/NumberKeyframeTrack.cpp src/animation/tracks/QuaternionKeyframeTrack.cpp
    src/animation/tracks/StringKeyframeTrack.cpp src/animation/tracks/VectorKeyframeTrack.cpp
    src/extras/core/Curve.cpp
    src/extras/core/Interpolations.cpp src/extras/core/CurvePath.cpp
    src/extras/core/Path.cpp src/extras/core/Shape.cpp src/extras/core/ShapePath.cpp
    src/extras/ShapeUtils.cpp src/extras/DataUtils.cpp src/extras/TextureUtils.cpp src/extras/lib/earcut.cpp
    src/extras/Earcut.cpp
    src/extras/Controls.cpp
    src/extras/curves/LineCurve.cpp src/extras/curves/LineCurve3.cpp
    src/extras/curves/QuadraticBezierCurve.cpp src/extras/curves/QuadraticBezierCurve3.cpp
    src/extras/curves/CubicBezierCurve.cpp src/extras/curves/CubicBezierCurve3.cpp
    src/extras/curves/EllipseCurve.cpp src/extras/curves/ArcCurve.cpp
    src/extras/curves/SplineCurve.cpp src/extras/curves/CatmullRomCurve3.cpp
)
target_sources(threecpp PRIVATE
    src/core/JSJobs.cpp src/core/JSPromise.cpp src/core/AbortController.cpp
    src/core/NativeEventLoop.cpp
    src/core/JSWeakMap.cpp
    src/core/JSMap.cpp src/core/JSSet.cpp src/core/JSIterator.cpp src/core/JSString.cpp
    src/core/JSRegExp.cpp src/core/JSStringEncoding.cpp src/core/UnicodeCase.cpp
    src/utils.cpp
    src/core/GLBufferAttribute.cpp
    src/core/UnicodeNormalization.cpp
    src/core/Clock.cpp src/core/Timer.cpp src/core/InstancedBufferGeometry.cpp
    src/cameras/ArrayCamera.cpp src/cameras/StereoCamera.cpp
    src/math/FrustumArray.cpp src/objects/ClippingGroup.cpp
    src/helpers/ArrowHelper.cpp src/helpers/AxesHelper.cpp
    src/helpers/Box3Helper.cpp src/helpers/BoxHelper.cpp src/helpers/CameraHelper.cpp
    src/helpers/DirectionalLightHelper.cpp src/helpers/GridHelper.cpp
    src/helpers/HemisphereLightHelper.cpp src/helpers/PlaneHelper.cpp
    src/helpers/PointLightHelper.cpp src/helpers/PolarGridHelper.cpp
    src/helpers/SkeletonHelper.cpp src/helpers/SpotLightHelper.cpp
    src/loaders/Cache.cpp src/loaders/Loader.cpp src/loaders/LoaderUtils.cpp
    src/loaders/LoadingManager.cpp src/loaders/AnimationLoader.cpp
    src/loaders/BufferGeometryLoader.cpp
    src/loaders/ImageLoader.cpp
    src/loaders/AudioLoader.cpp
    src/loaders/ImageBitmapLoader.cpp
    src/loaders/TextureLoader.cpp
    src/loaders/LUTCubeLoader.cpp
    src/loaders/LUT3dlLoader.cpp
    src/loaders/MDDLoader.cpp
    src/loaders/GaussianSplatUtils.cpp
    src/loaders/SPLATLoader.cpp
    src/loaders/PDBLoader.cpp
    src/loaders/GCodeLoader.cpp
    src/loaders/HDRLoader.cpp
    src/loaders/RGBELoader.cpp
    src/loaders/MTLLoader.cpp
    src/loaders/OBJLoader.cpp
    src/loaders/OBJMaterialGroups.cpp
    src/loaders/PLYLoader.cpp
    src/loaders/FontLoader.cpp
    src/loaders/HDRCubeTextureLoader.cpp
    src/loaders/MD2Loader.cpp
    src/loaders/BVHLoader.cpp
    src/loaders/PCDLoader.cpp
    src/loaders/TGALoader.cpp
    src/loaders/NRRDLoader.cpp
    src/loaders/AMFLoader.cpp
    src/loaders/XmlDocument.cpp
    src/loaders/VOXLoader.cpp
    src/loaders/SPZLoader.cpp
    src/loaders/IESLoader.cpp
    src/loaders/TIFFLoader.cpp
    src/loaders/GaussianSplatPLYLoader.cpp
    src/loaders/KSPLATLoader.cpp
    src/loaders/UltraHDRLoader.cpp
    src/loaders/PVRLoader.cpp
    src/loaders/DDSLoader.cpp
    src/loaders/KTXLoader.cpp
    src/loaders/LUTImageLoader.cpp
    src/loaders/GLTFGaussianSplatLoaderExtension.cpp
    src/loaders/VTKLoader.cpp
    src/loaders/LWOLoader.cpp
    src/loaders/TDSLoader.cpp
    src/loaders/LDrawLoader.cpp
    src/loaders/SVGLoader.cpp
    src/loaders/GLTFLoader.cpp
    src/loaders/EXRLoader.cpp
    src/loaders/Zlib.cpp
    src/loaders/Zip.cpp
    src/loaders/KMZLoader.cpp
    src/loaders/ThreeMFLoader.cpp
    src/loaders/CubeTextureLoader.cpp
    src/extras/ImageUtils.cpp
    src/extras/PMREMGenerator.cpp
    src/nodes/core/StackTrace.cpp
    src/nodes/core/NodeCode.cpp
    src/nodes/core/NodeFunctionInput.cpp
    src/nodes/core/NodeCache.cpp
    src/nodes/core/NodeVar.cpp
    src/nodes/core/NodeUtils.cpp
    src/nodes/core/NodeFrame.cpp
    src/nodes/core/ConstNode.cpp
    src/nodes/core/TempNode.cpp
    src/nodes/core/AccessorNames.cpp
    src/nodes/core/PointUVNode.cpp
    src/nodes/core/TangentUtils.cpp
    src/nodes/core/BuiltinNode.cpp
    src/nodes/core/SceneProperties.cpp
    src/nodes/core/Tangent.cpp
    src/nodes/core/Normal.cpp
    src/nodes/core/ReferenceElementNode.cpp
    src/nodes/core/CameraNames.cpp
    src/nodes/core/MaterialProperties.cpp
    src/nodes/core/ModelNode.cpp
    src/nodes/core/Object3DNode.cpp
    src/nodes/core/FrontFacing.cpp
    src/nodes/core/ColorSpaceFunctions.cpp
    src/nodes/core/ToneMapping.cpp
    src/nodes/core/NormalMap.cpp
    src/nodes/core/BumpMap.cpp
    src/nodes/core/PremultiplyAlpha.cpp
    src/nodes/core/Packing.cpp
    src/nodes/core/MatcapUV.cpp
    src/nodes/core/EquirectUV.cpp
    src/nodes/core/SpriteSheetUV.cpp
    src/nodes/core/Oscillators.cpp
    src/nodes/core/ViewportUtils.cpp
    src/nodes/core/DebugNode.cpp
    src/nodes/core/Discard.cpp
    src/nodes/core/ScreenNode.cpp
    src/nodes/core/DepthConversion.cpp
    src/nodes/core/ViewportDepthTexture.cpp
    src/nodes/core/ColorSpaceNode.cpp
    src/nodes/core/BlendModes.cpp
    src/nodes/core/ToneMappingFunctions.cpp
    src/nodes/core/ColorAdjustment.cpp
    src/nodes/core/ViewportTexture.cpp
    src/nodes/core/RenderOutput.cpp
    src/nodes/core/ToonOutlinePass.cpp
    src/nodes/core/PassTexture.cpp
    src/nodes/core/UVUtils.cpp
    src/nodes/core/ConvertNode.cpp
    src/nodes/core/MaxMipLevel.cpp
    src/nodes/core/Remap.cpp
    src/nodes/core/SampleNode.cpp
    src/nodes/core/RotateNode.cpp
    src/nodes/core/FlipNode.cpp
    src/nodes/core/SplitNode.cpp
    src/nodes/core/JoinNode.cpp
    src/nodes/core/MemberNode.cpp
    src/nodes/core/ArrayElementNode.cpp
    src/nodes/core/StorageArrayElementNode.cpp
    src/nodes/core/SpriteUtils.cpp
    src/nodes/core/SetNode.cpp
    src/nodes/core/TriplanarTextures.cpp
    src/nodes/core/FunctionOverloadingNode.cpp
    src/nodes/core/CubeMapNode.cpp
    src/nodes/core/EventNode.cpp
    src/nodes/core/PostProcessingUtils.cpp
    src/nodes/core/NodeParser.cpp
    src/nodes/core/BypassNode.cpp
    src/nodes/core/SubBuildNode.cpp
    src/nodes/core/NodeConstantsTables.cpp
    src/nodes/core/NodeBaseFields.cpp
    src/nodes/core/NodeBeforeChain.cpp
    src/nodes/core/NodeAccessors.cpp
    src/nodes/core/NodeTypeCache.cpp
    src/nodes/core/NodeShared.cpp
    src/nodes/core/NodeSetup.cpp
    src/nodes/core/NodeBuilderContext.cpp
    src/nodes/core/NodeBuilderData.cpp
    src/nodes/core/NodeBuilderHash.cpp
    src/nodes/core/NodeBuilderTypes.cpp
    src/nodes/core/OutputStructNode.cpp
    src/nodes/core/LightingModel.cpp
    src/nodes/core/IsolateNode.cpp
    src/nodes/core/InspectorNode.cpp
    src/nodes/core/StructNode.cpp
    src/nodes/core/InputNode.cpp
    src/nodes/core/AttributeNode.cpp
    src/nodes/core/StructTypeNode.cpp
    src/nodes/core/UniformGroupNode.cpp
    src/nodes/core/ArrayNode.cpp
    src/nodes/core/OverrideContextNode.cpp
    src/nodes/core/AssignNode.cpp
    src/nodes/core/VaryingNode.cpp
    src/nodes/core/MRTNode.cpp
    src/nodes/core/UniformNode.cpp
    src/nodes/core/IndexNode.cpp
    src/nodes/core/VarNode.cpp
    src/nodes/core/ContextNode.cpp
    src/nodes/core/StackNode.cpp
    src/nodes/core/PropertyNode.cpp
    src/nodes/core/LoopNode.cpp
    src/nodes/core/RTTNode.cpp
    src/nodes/core/ReflectorNode.cpp
    src/nodes/core/ViewportSharedTextureNode.cpp
    src/nodes/core/ViewportDepthTextureNode.cpp
    src/nodes/core/ViewportDepthNode.cpp
    src/loaders/USDZLoader.cpp
    src/loaders/TTFLoader.cpp
    src/loaders/ColladaLoader.cpp
    src/loaders/USDLoader.cpp
    src/loaders/MaterialXLoader.cpp
    src/loaders/DRACOLoader.cpp
    src/loaders/KTX2Loader.cpp
    src/loaders/ThreeDMLoader.cpp
    src/loaders/VRMLLoader.cpp
    src/loaders/FBXLoader.cpp
    src/nodes/core/ParameterNode.cpp
    src/nodes/core/NodeConstants.cpp
    src/nodes/core/Node.cpp
    src/loaders/DataTextureLoader.cpp
    src/loaders/CompressedTextureLoader.cpp src/loaders/MaterialLoader.cpp src/loaders/FileLoader.cpp
    src/loaders/ObjectLoader.cpp
    src/core/InterleavedBufferAttribute.cpp
    src/objects/Line.cpp
    src/objects/LineLoop.cpp
    src/objects/LineSegments.cpp
    src/objects/Points.cpp
    src/objects/Sprite.cpp
    src/objects/LOD.cpp
    src/renderers/shaders/UniformsUtils.cpp
    src/renderers/shaders/UniformsLib.cpp
    ${THREECPP_SHADER_STRING_SOURCES}
    src/renderers/webgl/WebGLShader.cpp
    src/renderers/webgl/WebGLShaderCache.cpp src/renderers/webgl/WebGLUtils.cpp
    src/renderers/webgl/WebGLProgram.cpp src/renderers/webgl/WebGLPrograms.cpp
    src/renderers/webgl/WebGLInfo.cpp
    src/renderers/webgl/WebGLAnimation.cpp
    src/renderers/webgl/WebGLExtensions.cpp
    src/renderers/webgl/WebGLCapabilities.cpp
    src/renderers/webgl/WebGLProperties.cpp
    src/renderers/webgl/WebGLAttributes.cpp
    src/renderers/webgl/WebGLBindingStates.cpp
    src/renderers/webgl/WebGLClipping.cpp
    src/renderers/webgl/WebGLMaterials.cpp
    src/renderers/webgl/WebGLGeometries.cpp
    src/renderers/webgl/WebGLObjects.cpp
    src/renderers/webgl/WebGLLights.cpp
    src/renderers/webgl/WebGLRenderStates.cpp
    src/renderers/webgl/WebGLRenderLists.cpp
    src/renderers/webgl/WebGLUniforms.cpp
    src/renderers/webgl/WebGLUniformsGroups.cpp
    src/renderers/webgl/WebGLState.cpp
    src/renderers/webgl/WebGLBufferRenderer.cpp
    src/renderers/webgl/WebGLIndexedBufferRenderer.cpp
    src/core/Uniform.cpp
    src/core/UniformsGroup.cpp
    src/core/NativeArray.cpp
    src/core/InterleavedBuffer.cpp
    src/core/InstancedInterleavedBuffer.cpp
    src/core/RenderTarget.cpp
    src/core/RenderTarget3D.cpp
    src/core/WebGLCubeRenderTarget.cpp
    src/core/InstancedBufferAttribute.cpp
    src/objects/Bone.cpp
    src/objects/Skeleton.cpp
    src/objects/BatchedMesh.cpp
    src/objects/SkinnedMesh.cpp
    src/objects/InstancedMesh.cpp
    src/math/SphericalHarmonics3.cpp
    src/lights/AmbientLight.cpp
    src/lights/DirectionalLight.cpp
    src/lights/DirectionalLightShadow.cpp
    src/lights/HemisphereLight.cpp
    src/lights/Light.cpp
    src/lights/LightProbe.cpp
    src/lights/LightShadow.cpp
    src/lights/PointLight.cpp
    src/lights/PointLightShadow.cpp
    src/lights/RectAreaLight.cpp
    src/lights/SpotLight.cpp
    src/lights/SpotLightShadow.cpp
    src/lights/webgpu/IESSpotLight.cpp
    src/lights/webgpu/ProjectorLight.cpp
    src/materials/LineBasicMaterial.cpp
    src/materials/LineDashedMaterial.cpp
    src/materials/MeshDepthMaterial.cpp
    src/materials/MeshDistanceMaterial.cpp
    src/materials/MeshLambertMaterial.cpp
    src/materials/MeshMatcapMaterial.cpp
    src/materials/MeshNormalMaterial.cpp
    src/materials/MeshPhongMaterial.cpp
    src/materials/MeshPhysicalMaterial.cpp
    src/materials/MeshStandardMaterial.cpp
    src/materials/MeshToonMaterial.cpp
    src/materials/PointsMaterial.cpp
    src/materials/RawShaderMaterial.cpp
    src/materials/ShaderMaterial.cpp
    src/materials/ShadowMaterial.cpp
    src/materials/SpriteMaterial.cpp
)
add_library(threecpp::threecpp ALIAS threecpp)
target_include_directories(threecpp PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")
target_compile_features(threecpp PUBLIC cxx_std_20)
target_link_libraries(threecpp PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(threecpp PUBLIC /utf-8 /permissive-)
    target_compile_options(threecpp PRIVATE /W4 /fp:strict)
else()
    target_compile_options(threecpp PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
endif()
if(THREECPP_BUILD_ADDONS)
    add_library(threecpp_addons
        examples/jsm/helpers/VertexNormalsHelper.cpp
        examples/jsm/helpers/VertexTangentsHelper.cpp
        examples/jsm/utils/SortUtils.cpp
        examples/jsm/utils/SkeletonUtils.cpp
        examples/jsm/utils/BufferGeometryUtils.cpp
        examples/jsm/math/ImprovedNoise.cpp
        examples/jsm/math/ColorConverter.cpp
        examples/jsm/math/ColorSpaces.cpp
        examples/jsm/math/Capsule.cpp
        examples/jsm/math/OBB.cpp
        examples/jsm/math/SimplexNoise.cpp
        examples/jsm/math/MeshSurfaceSampler.cpp
        examples/jsm/utils/CameraUtils.cpp
        examples/jsm/animation/AnimationClipCreator.cpp
        examples/jsm/lights/RectAreaLightTexturesLib.cpp
        examples/jsm/lights/RectAreaLightUniformsLib.cpp
        examples/jsm/curves/NURBSUtils.cpp
        examples/jsm/curves/NURBSCurve.cpp
        examples/jsm/curves/NURBSSurface.cpp
        examples/jsm/curves/NURBSVolume.cpp
        examples/jsm/curves/CurveExtras.cpp
        examples/jsm/geometries/ParametricFunctions.cpp
        examples/jsm/geometries/ParametricGeometry.cpp
        examples/jsm/geometries/BoxLineGeometry.cpp
        examples/jsm/geometries/RoundedBoxGeometry.cpp
        examples/jsm/math/ConvexHull.cpp
        examples/jsm/geometries/ConvexGeometry.cpp
        examples/jsm/geometries/LoftGeometry.cpp
        examples/jsm/geometries/DecalGeometry.cpp
        examples/jsm/geometries/TeapotGeometry.cpp
    examples/jsm/loaders/XYZLoader.cpp
  examples/jsm/loaders/STLLoader.cpp
        examples/jsm/shaders/ACESFilmicToneMappingShader.cpp
        examples/jsm/shaders/AfterimageShader.cpp
        examples/jsm/shaders/BasicShader.cpp
        examples/jsm/shaders/BleachBypassShader.cpp
        examples/jsm/shaders/BlendShader.cpp
        examples/jsm/shaders/BokehShader.cpp
        examples/jsm/shaders/BokehShader2.cpp
        examples/jsm/shaders/BrightnessContrastShader.cpp
        examples/jsm/shaders/ColorCorrectionShader.cpp
        examples/jsm/shaders/ColorifyShader.cpp
        examples/jsm/shaders/ConvolutionShader.cpp
        examples/jsm/shaders/CopyShader.cpp
        examples/jsm/shaders/DOFMipMapShader.cpp
        examples/jsm/shaders/DepthLimitedBlurShader.cpp
        examples/jsm/shaders/DigitalGlitch.cpp
        examples/jsm/shaders/DotScreenShader.cpp
        examples/jsm/shaders/ExposureShader.cpp
        examples/jsm/shaders/FXAAShader.cpp
        examples/jsm/shaders/FilmShader.cpp
        examples/jsm/shaders/FocusShader.cpp
        examples/jsm/shaders/FreiChenShader.cpp
        examples/jsm/shaders/GTAOShader.cpp
        examples/jsm/shaders/GammaCorrectionShader.cpp
        examples/jsm/shaders/HalftoneShader.cpp
        examples/jsm/shaders/HorizontalBlurShader.cpp
        examples/jsm/shaders/HorizontalTiltShiftShader.cpp
        examples/jsm/shaders/HueSaturationShader.cpp
        examples/jsm/shaders/KaleidoShader.cpp
        examples/jsm/shaders/LuminosityHighPassShader.cpp
        examples/jsm/shaders/LuminosityShader.cpp
        examples/jsm/shaders/MirrorShader.cpp
        examples/jsm/shaders/NormalMapShader.cpp
        examples/jsm/shaders/OutputShader.cpp
        examples/jsm/shaders/PoissonDenoiseShader.cpp
        examples/jsm/shaders/RGBShiftShader.cpp
        examples/jsm/shaders/SAOShader.cpp
        examples/jsm/shaders/SMAAShader.cpp
        examples/jsm/shaders/SSAOShader.cpp
        examples/jsm/shaders/SSRShader.cpp
        examples/jsm/shaders/SepiaShader.cpp
        examples/jsm/shaders/SobelOperatorShader.cpp
        examples/jsm/shaders/SubsurfaceScatteringShader.cpp
        examples/jsm/shaders/TechnicolorShader.cpp
        examples/jsm/shaders/ToonShader.cpp
        examples/jsm/shaders/TriangleBlurShader.cpp
        examples/jsm/shaders/UnpackDepthRGBAShader.cpp
        examples/jsm/shaders/VelocityShader.cpp
        examples/jsm/shaders/VolumeShader.cpp
        examples/jsm/shaders/VerticalBlurShader.cpp
        examples/jsm/shaders/VerticalTiltShiftShader.cpp
        examples/jsm/shaders/VignetteShader.cpp
        examples/jsm/shaders/WaterRefractionShader.cpp)
    add_library(threecpp::addons ALIAS threecpp_addons)
    target_link_libraries(threecpp_addons PUBLIC threecpp::threecpp)
    target_include_directories(threecpp_addons PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/examples/jsm>")
    if(MSVC)
        target_compile_options(threecpp_addons PRIVATE /W4 /fp:strict)
    else()
        target_compile_options(threecpp_addons PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
    endif()
endif()
add_executable(threecpp_example examples/math_basic.cpp)
set_target_properties(threecpp_example PROPERTIES OUTPUT_NAME THREECPP)
target_link_libraries(threecpp_example PRIVATE threecpp::threecpp)
add_executable(animation_keyframes examples/animation_keyframes.cpp)
target_link_libraries(animation_keyframes PRIVATE threecpp::threecpp)

if(THREECPP_BUILD_VULKAN)
    # The SDK may also be vendored in the tree (third_party/vulkan), so a checkout
    # builds without installing anything or setting an environment variable.
    set(THREECPP_VENDORED_VULKAN "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vulkan")
    find_path(THREECPP_VULKAN_INCLUDE_DIR vulkan/vulkan.h
        HINTS "${THREECPP_VENDORED_VULKAN}/include" "$ENV{VULKAN_SDK}"
        PATH_SUFFIXES Include include)
    find_program(THREECPP_GLSLANG_VALIDATOR NAMES glslangValidator glslang
        HINTS "${THREECPP_VENDORED_VULKAN}/bin" "$ENV{VULKAN_SDK}"
        PATH_SUFFIXES Bin bin)
    if(NOT THREECPP_VULKAN_INCLUDE_DIR OR NOT THREECPP_GLSLANG_VALIDATOR)
        message(FATAL_ERROR "Vulkan requires headers and glslangValidator. Set THREECPP_VULKAN_INCLUDE_DIR and THREECPP_GLSLANG_VALIDATOR, or install the Vulkan SDK.")
    endif()
    set(THREECPP_SHADER_OUTPUTS)
    # Every built-in program is compiled the same way; PMREM needs its own pair.
    foreach(program basic pmrem_equirect pmrem_blur)
    # The basic program is sourced from meshbasic.glsl but keeps its historical spv name.
    if(program STREQUAL "basic")
        set(program_source meshbasic)
    else()
        set(program_source "${program}")
    endif()
    foreach(stage vert frag)
        set(shader "${program}.${stage}")
        set(shader_source "${CMAKE_CURRENT_SOURCE_DIR}/src/renderers/shaders/ShaderLib/${program_source}.${stage}.glsl")
        set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv")
        add_custom_command(OUTPUT "${output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            COMMAND "${THREECPP_GLSLANG_VALIDATOR}" -V --target-env vulkan1.1 -S "${stage}" "${shader_source}" -o "${output}"
            DEPENDS "${shader_source}" VERBATIM)
        list(APPEND THREECPP_SHADER_OUTPUTS "${output}")
    endforeach()
    endforeach()
    add_custom_target(threecpp_shaders DEPENDS ${THREECPP_SHADER_OUTPUTS})
    add_library(threecpp_spirv_reflect STATIC src/renderers/vulkan/lib/spirv_reflect/spirv_reflect.cpp)
    target_include_directories(threecpp_spirv_reflect PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src/renderers/vulkan/lib/spirv_reflect")
    set_target_properties(threecpp_spirv_reflect PROPERTIES POSITION_INDEPENDENT_CODE ON)
    add_library(threecpp_vulkan src/renderers/VulkanRenderer.cpp
        src/renderers/vulkan/VulkanBufferCommands.cpp src/renderers/vulkan/VulkanBindingCommands.cpp
        src/renderers/vulkan/VulkanShaderCompiler.cpp
        src/renderers/vulkan/VulkanShaderInterface.cpp src/renderers/vulkan/VulkanPipelineLayout.cpp
        src/renderers/vulkan/VulkanShaderModules.cpp src/renderers/vulkan/VulkanUniformBuffer.cpp
        src/renderers/vulkan/VulkanProgramUniforms.cpp src/renderers/vulkan/CubeCameraVulkan.cpp)
    add_library(threecpp::vulkan ALIAS threecpp_vulkan)
    target_link_libraries(threecpp_vulkan PUBLIC threecpp::threecpp
        PRIVATE threecpp_spirv_reflect ${CMAKE_DL_LIBS})
    target_include_directories(threecpp_vulkan PRIVATE "${THREECPP_VULKAN_INCLUDE_DIR}")
    target_compile_definitions(threecpp_vulkan PRIVATE THREECPP_SHADER_DIRECTORY="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    target_compile_definitions(threecpp_vulkan PUBLIC THREECPP_HAS_VULKAN=1)
    add_dependencies(threecpp_vulkan threecpp_shaders)
    option(THREECPP_NATIVE_ESSL "Build the pinned native ESSL AST compiler candidate" OFF)
    if(THREECPP_NATIVE_ESSL)
        include(cmake/essl/NativeEssl.cmake)
        target_sources(threecpp_vulkan PRIVATE src/renderers/vulkan/VulkanShaderCompilerESSL.cpp)
        target_link_libraries(threecpp_vulkan PRIVATE threecpp_essl_native)
        target_compile_definitions(threecpp_vulkan PUBLIC THREECPP_HAS_NATIVE_ESSL=1)
    endif()
    add_executable(vulkan_basic examples/vulkan_basic.cpp)
    target_link_libraries(vulkan_basic PRIVATE threecpp::vulkan)
endif()
if(THREECPP_BUILD_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()
