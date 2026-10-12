add_executable(THREECPPTests
    unit/three.source.unit.cpp
    unit/src/math/Vector2.tests.cpp
    unit/src/math/Vector3.tests.cpp
    unit/src/math/Vector4.tests.cpp
    unit/src/math/MathUtils.tests.cpp
    unit/src/math/Matrix3.tests.cpp
    unit/src/math/Matrix4.tests.cpp
    unit/src/math/Quaternion.tests.cpp
    unit/src/math/Euler.tests.cpp
    unit/src/math/Color.tests.cpp
    unit/src/math/ThreeJsParity.tests.cpp
    unit/src/math/Interpolant.tests.cpp unit/src/math/interpolants/Interpolants.tests.cpp
    unit/src/core/EventDispatcher.tests.cpp unit/src/core/Object3D.tests.cpp unit/src/core/Resources.tests.cpp
    unit/src/core/ResourceParity.tests.cpp
    unit/src/core/BufferAttributeRuntime.tests.cpp
    unit/src/core/JSValue.tests.cpp
    unit/src/core/OrderedMap.tests.cpp
    unit/src/core/JSPromise.tests.cpp
    unit/src/core/NativeEventLoop.tests.cpp
    unit/src/core/CoreCameraModules.tests.cpp
    unit/src/helpers/Helpers.tests.cpp
    unit/src/loaders/Loaders.tests.cpp
    unit/src/renderers/webgl/WebGLInfo.tests.cpp
    unit/src/renderers/webgl/WebGLBufferRenderers.tests.cpp
    unit/src/renderers/webgl/WebGLAttributes.tests.cpp
    unit/src/renderers/webgl/WebGLRenderLists.tests.cpp
    unit/src/core/JSConstruct.tests.cpp
    unit/src/core/Uniforms.tests.cpp
    unit/src/core/InterleavedBuffer.tests.cpp
    unit/src/core/InterleavedBufferAttribute.tests.cpp
    unit/src/core/JSHost.tests.cpp
    unit/src/core/RuntimeNativeOwnership.tests.cpp
    unit/src/core/ArraySort.tests.cpp
    unit/src/core/ArrayMethods.tests.cpp
    unit/src/core/NativeObject.tests.cpp
    unit/src/core/TypedArray.tests.cpp
    unit/src/extras/core/Curve.tests.cpp
    unit/src/extras/core/CurvePath.tests.cpp unit/src/extras/core/Path.tests.cpp
    unit/src/extras/core/Shape.tests.cpp unit/src/extras/core/ShapePath.tests.cpp
    unit/src/extras/ShapeUtils.tests.cpp
    unit/src/extras/DataUtils.tests.cpp
    unit/src/extras/curves/RemainingCurves.tests.cpp
    unit/src/extras/Earcut.tests.cpp
    unit/src/extras/curves/LineCurve.tests.cpp unit/src/extras/curves/LineCurve3.tests.cpp
    unit/src/extras/curves/QuadraticBezierCurve.tests.cpp unit/src/extras/curves/QuadraticBezierCurve3.tests.cpp
    unit/src/extras/curves/CubicBezierCurve.tests.cpp unit/src/extras/curves/CubicBezierCurve3.tests.cpp
    unit/src/cameras/Camera.tests.cpp unit/src/scenes/Scene.tests.cpp
    unit/src/textures/Texture.tests.cpp
    unit/src/lights/Lights.tests.cpp
    unit/src/materials/Materials.tests.cpp
    unit/src/objects/Skinning.tests.cpp
    unit/src/objects/InstancedMesh.tests.cpp
    unit/src/objects/LOD.tests.cpp
    unit/src/objects/PrimitiveObjects.tests.cpp
    unit/src/animation/Animation.tests.cpp
    unit/src/animation/PropertyMixer.tests.cpp
    unit/src/animation/PropertyBinding.tests.cpp
    unit/src/animation/AnimationAction.tests.cpp
    unit/src/animation/AnimationObjectGroup.tests.cpp
    unit/src/animation/AnimationRuntime.tests.cpp
    unit/src/animation/AnimationMixer.tests.cpp
    unit/src/geometries/PrimitiveGeometries.tests.cpp unit/src/geometries/AdvancedGeometries.tests.cpp
    unit/src/geometries/ExtrudeGeometry.tests.cpp
)

target_include_directories(THREECPPTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils"
)
target_link_libraries(THREECPPTests PRIVATE threecpp::threecpp)
add_test(NAME threecpp.unit COMMAND THREECPPTests)
add_executable(JSWeakValueTests unit/src/core/JSWeakValue.tests.cpp)
target_link_libraries(JSWeakValueTests PRIVATE threecpp::threecpp)
add_test(NAME threecpp.runtime-weak-values COMMAND JSWeakValueTests)
add_executable(JSWeakMapTests
    unit/three.source.unit.cpp
    unit/src/core/JSWeakMap.tests.cpp
    unit/src/renderers/webgl/WebGLProperties.tests.cpp)
target_include_directories(JSWeakMapTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils")
target_link_libraries(JSWeakMapTests PRIVATE threecpp::threecpp)
# Keep heap lifetime checks independent of eagerly initialized renderer TLS globals.
add_test(NAME threecpp.runtime-weak-map COMMAND JSWeakMapTests)
add_executable(GLBufferAttributeTests
    unit/three.source.unit.cpp
    unit/src/core/GLBufferAttribute.tests.cpp)
target_include_directories(GLBufferAttributeTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils")
target_link_libraries(GLBufferAttributeTests PRIVATE threecpp::threecpp)
add_test(NAME threecpp.gl-buffer-attribute-unit COMMAND GLBufferAttributeTests)
add_executable(NativeEventLoopReentryTests
    unit/utils/NativeEventLoopReentryTestMain.cpp
    unit/src/core/NativeEventLoopReentry.tests.cpp)
target_include_directories(NativeEventLoopReentryTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils")
target_link_libraries(NativeEventLoopReentryTests PRIVATE threecpp::threecpp)
# Last-root finalization needs a process without eagerly initialized UniformsLib TLS roots.
add_test(NAME threecpp.native-event-loop-reentry COMMAND NativeEventLoopReentryTests)
if(THREECPP_BUILD_ADDONS)
    add_executable(THREECPPAddonTests
        unit/three.source.unit.cpp
        unit/addons/helpers/VertexNormalsHelper.tests.cpp
        unit/addons/helpers/VertexTangentsHelper.tests.cpp
        unit/addons/helpers/NativeObjectExtension.tests.cpp)
    target_include_directories(THREECPPAddonTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils")
    target_link_libraries(THREECPPAddonTests PRIVATE threecpp::addons)
    add_test(NAME threecpp.addons-unit COMMAND THREECPPAddonTests)
    add_executable(ThreeJsVertexHelpersParityProbe unit/utils/parity/VertexHelpersParityProbe.cpp)
    target_link_libraries(ThreeJsVertexHelpersParityProbe PRIVATE threecpp::addons)
endif()
if(THREECPP_BUILD_VULKAN)
    add_executable(VulkanBufferCommandsTests
        unit/three.source.unit.cpp unit/src/renderers/vulkan/VulkanBufferCommands.tests.cpp)
    target_include_directories(VulkanBufferCommandsTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils")
    target_link_libraries(VulkanBufferCommandsTests PRIVATE threecpp::vulkan)
    add_test(NAME threecpp.vulkan-buffer-commands-unit COMMAND VulkanBufferCommandsTests)
    add_executable(VulkanBufferCommandsParityProbe unit/utils/parity/VulkanBufferCommandsParityProbe.cpp)
    target_link_libraries(VulkanBufferCommandsParityProbe PRIVATE threecpp::vulkan)
    add_executable(VulkanRendererTests unit/src/renderers/VulkanRenderer.tests.cpp)
    target_link_libraries(VulkanRendererTests PRIVATE threecpp::vulkan)
    add_test(NAME threecpp.vulkan COMMAND VulkanRendererTests)
    set_tests_properties(threecpp.vulkan PROPERTIES LABELS "GPU;Vulkan")
endif()
add_executable(ThreeJsParityProbe unit/utils/parity/ParityProbe.cpp)
add_executable(ThreeJsPromiseParityProbe unit/utils/parity/PromiseParityProbe.cpp)
add_executable(ThreeJsWebGLInfoParityProbe unit/utils/parity/WebGLInfoParityProbe.cpp)
target_link_libraries(ThreeJsWebGLInfoParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsWebGLBufferRenderersParityProbe unit/utils/parity/WebGLBufferRenderersParityProbe.cpp)
target_link_libraries(ThreeJsWebGLBufferRenderersParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsNativeEventLoopParityProbe unit/utils/parity/NativeEventLoopParityProbe.cpp)
target_link_libraries(ThreeJsNativeEventLoopParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsWeakMapParityProbe unit/utils/parity/JSWeakMapParityProbe.cpp)
target_link_libraries(ThreeJsWeakMapParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsWebGLPropertiesParityProbe unit/utils/parity/WebGLPropertiesParityProbe.cpp)
target_link_libraries(ThreeJsWebGLPropertiesParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsWebGLAttributesParityProbe unit/utils/parity/WebGLAttributesParityProbe.cpp)
target_link_libraries(ThreeJsWebGLAttributesParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsNativeBufferAttributeUploadParityProbe unit/utils/parity/NativeBufferAttributeUploadParityProbe.cpp)
target_link_libraries(ThreeJsNativeBufferAttributeUploadParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsWebGLRenderListsParityProbe unit/utils/parity/WebGLRenderListsParityProbe.cpp)
target_link_libraries(ThreeJsWebGLRenderListsParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsGLBufferAttributeParityProbe unit/utils/parity/GLBufferAttributeParityProbe.cpp)
target_link_libraries(ThreeJsGLBufferAttributeParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsArrayMethodsParityProbe unit/utils/parity/ArrayMethodsParityProbe.cpp)
target_link_libraries(ThreeJsArrayMethodsParityProbe PRIVATE threecpp::threecpp)
target_link_libraries(ThreeJsPromiseParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsCoreCameraParityProbe unit/utils/parity/CoreCameraParityProbe.cpp)
target_link_libraries(ThreeJsCoreCameraParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsHelpersParityProbe unit/utils/parity/HelpersParityProbe.cpp)
target_link_libraries(ThreeJsHelpersParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsLoaderFoundationParityProbe unit/utils/parity/LoaderFoundationParityProbe.cpp)
target_link_libraries(ThreeJsLoaderFoundationParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsLoaderParsersParityProbe unit/utils/parity/LoaderParsersParityProbe.cpp)
target_link_libraries(ThreeJsLoaderParsersParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsInterleavedAttributeParityProbe unit/utils/parity/InterleavedAttributeParityProbe.cpp)
target_link_libraries(ThreeJsInterleavedAttributeParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsLODParityProbe unit/utils/parity/LODParityProbe.cpp)
target_link_libraries(ThreeJsLODParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsPrimitiveObjectsParityProbe unit/utils/parity/PrimitiveObjectsParityProbe.cpp)
target_link_libraries(ThreeJsPrimitiveObjectsParityProbe PRIVATE threecpp::threecpp)
target_link_libraries(ThreeJsParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsCoreParityProbe unit/utils/parity/CoreParityProbe.cpp)
target_link_libraries(ThreeJsCoreParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsHelperParityProbe unit/utils/parity/HelperParityProbe.cpp)
target_link_libraries(ThreeJsHelperParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsRaycastParityProbe unit/utils/parity/RaycastParityProbe.cpp)
target_link_libraries(ThreeJsRaycastParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsQueryParityProbe unit/utils/parity/QueryParityProbe.cpp)
target_link_libraries(ThreeJsQueryParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsRuntimeParityProbe unit/utils/parity/RuntimeParityProbe.cpp)
target_link_libraries(ThreeJsRuntimeParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsStorageParityProbe unit/utils/parity/StorageParityProbe.cpp)
target_link_libraries(ThreeJsStorageParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsCurveParityProbe unit/utils/parity/CurveParityProbe.cpp)
target_link_libraries(ThreeJsCurveParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsExtendedRuntimeParityProbe unit/utils/parity/ExtendedRuntimeParityProbe.cpp)
target_link_libraries(ThreeJsExtendedRuntimeParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsRemainingCurveParityProbe unit/utils/parity/RemainingCurveParityProbe.cpp)
target_link_libraries(ThreeJsRemainingCurveParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsPathParityProbe unit/utils/parity/PathParityProbe.cpp)
target_link_libraries(ThreeJsPathParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsInterpolantParityProbe unit/utils/parity/InterpolantParityProbe.cpp)
target_link_libraries(ThreeJsInterpolantParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsDataUtilsParityProbe unit/utils/parity/DataUtilsParityProbe.cpp)
target_link_libraries(ThreeJsDataUtilsParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsEarcutParityProbe unit/utils/parity/EarcutParityProbe.cpp)
target_link_libraries(ThreeJsEarcutParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsPrimitiveGeometryParityProbe unit/utils/parity/PrimitiveGeometryParityProbe.cpp)
target_link_libraries(ThreeJsPrimitiveGeometryParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsAdvancedGeometryParityProbe unit/utils/parity/AdvancedGeometryParityProbe.cpp)
target_link_libraries(ThreeJsAdvancedGeometryParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsExtrudeGeometryParityProbe unit/utils/parity/ExtrudeGeometryParityProbe.cpp)
target_link_libraries(ThreeJsExtrudeGeometryParityProbe PRIVATE threecpp::threecpp)
find_program(THREECPP_NODE_EXECUTABLE NAMES node)
add_executable(ThreeJsArraySortParityProbe unit/utils/parity/ArraySortParityProbe.cpp)
target_link_libraries(ThreeJsArraySortParityProbe PRIVATE threecpp::threecpp)
if(THREECPP_NODE_EXECUTABLE)
    add_test(NAME threecpp.javascript-array-methods-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-array-methods.mjs"
        "$<TARGET_FILE:ThreeJsArrayMethodsParityProbe>")
    add_test(NAME threecpp.javascript-weak-map-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-native-weakmap.mjs"
        "$<TARGET_FILE:ThreeJsWeakMapParityProbe>")
    add_test(NAME threecpp.native-event-loop-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-native-event-loop.mjs"
        "$<TARGET_FILE:ThreeJsNativeEventLoopParityProbe>")
    if(THREEJS_SOURCE_DIR)
        if(THREECPP_BUILD_VULKAN)
            add_test(NAME threecpp.vulkan-buffer-commands-parity
                COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-vulkan-buffer-commands.mjs"
                "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:VulkanBufferCommandsParityProbe>")
        endif()
        add_test(NAME threecpp.threejs-native-buffer-attribute-upload-parity
            COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-native-buffer-attribute-upload.mjs"
            "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsNativeBufferAttributeUploadParityProbe>")
        add_test(NAME threecpp.threejs-webgl-render-lists-parity
            COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-webgl-render-lists.mjs"
            "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsWebGLRenderListsParityProbe>")
        add_test(NAME threecpp.threejs-webgl-attributes-parity
            COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-webgl-attributes.mjs"
            "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsWebGLAttributesParityProbe>")
        add_test(NAME threecpp.threejs-gl-buffer-attribute-parity
            COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-gl-buffer-attribute.mjs"
            "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsGLBufferAttributeParityProbe>")
        add_test(NAME threecpp.threejs-webgl-properties-parity
            COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-webgl-properties.mjs"
            "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsWebGLPropertiesParityProbe>")
        add_test(NAME threecpp.threejs-webgl-buffer-renderers-parity
            COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-webgl-buffer-renderers.mjs"
            "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsWebGLBufferRenderersParityProbe>")
        if(THREECPP_BUILD_ADDONS)
            add_test(NAME threecpp.threejs-vertex-helpers-parity
                COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-vertex-helpers.mjs"
                "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsVertexHelpersParityProbe>")
        endif()
        add_test(NAME threecpp.threejs-webgl-info-parity
            COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-webgl-info.mjs"
            "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsWebGLInfoParityProbe>")
    endif()
    add_test(NAME threecpp.javascript-promises-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-promises.mjs"
        "$<TARGET_FILE:ThreeJsPromiseParityProbe>")
    add_test(NAME threecpp.v8-array-sort-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-array-sort.mjs"
        "$<TARGET_FILE:ThreeJsArraySortParityProbe>")
endif()
add_executable(ThreeJsAnimationParityProbe unit/utils/parity/AnimationParityProbe.cpp)
target_link_libraries(ThreeJsAnimationParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsPropertyMixerParityProbe unit/utils/parity/PropertyMixerParityProbe.cpp)
target_link_libraries(ThreeJsPropertyMixerParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsPropertyBindingParityProbe unit/utils/parity/PropertyBindingParityProbe.cpp)
target_link_libraries(ThreeJsPropertyBindingParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsNativeObjectParityProbe unit/utils/parity/NativeObjectParityProbe.cpp)
target_link_libraries(ThreeJsNativeObjectParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsTextureParityProbe unit/utils/parity/TextureParityProbe.cpp)
target_link_libraries(ThreeJsTextureParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsActionParityProbe unit/utils/parity/ActionParityProbe.cpp)
target_link_libraries(ThreeJsActionParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsGroupParityProbe unit/utils/parity/GroupParityProbe.cpp)
target_link_libraries(ThreeJsGroupParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsMixerCacheParityProbe unit/utils/parity/MixerCacheParityProbe.cpp)
target_link_libraries(ThreeJsMixerCacheParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsMixerParityProbe unit/utils/parity/MixerParityProbe.cpp)
target_link_libraries(ThreeJsMixerParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsAnimationRuntimeParityProbe unit/utils/parity/AnimationRuntimeParityProbe.cpp)
target_link_libraries(ThreeJsAnimationRuntimeParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsRuntimeCoreParityProbe unit/utils/parity/RuntimeCoreParityProbe.cpp)
target_link_libraries(ThreeJsRuntimeCoreParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsRenderTargetParityProbe unit/utils/parity/RenderTargetParityProbe.cpp)
target_link_libraries(ThreeJsRenderTargetParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsLightsParityProbe unit/utils/parity/LightsParityProbe.cpp)
target_link_libraries(ThreeJsLightsParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsMaterialParityProbe unit/utils/parity/MaterialParityProbe.cpp)
target_link_libraries(ThreeJsMaterialParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsSkinningParityProbe unit/utils/parity/SkinningParityProbe.cpp)
target_link_libraries(ThreeJsSkinningParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsUniformsParityProbe unit/utils/parity/UniformsParityProbe.cpp)
target_link_libraries(ThreeJsUniformsParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsUniformsLibParityProbe unit/utils/parity/UniformsLibParityProbe.cpp)
target_link_libraries(ThreeJsUniformsLibParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsConstructParityProbe unit/utils/parity/ConstructParityProbe.cpp)
target_link_libraries(ThreeJsConstructParityProbe PRIVATE threecpp::threecpp)
add_executable(ThreeJsInterleavedBufferParityProbe unit/utils/parity/InterleavedBufferParityProbe.cpp)
target_link_libraries(ThreeJsInterleavedBufferParityProbe PRIVATE threecpp::threecpp)
if(THREECPP_NODE_EXECUTABLE)
    add_test(NAME threecpp.javascript-construct-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-construct.mjs"
        "$<TARGET_FILE:ThreeJsConstructParityProbe>")
endif()
if(THREEJS_SOURCE_DIR AND THREECPP_NODE_EXECUTABLE)
    add_test(NAME threecpp.threejs-core-camera-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-core-camera.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsCoreCameraParityProbe>")
    add_test(NAME threecpp.threejs-helpers-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-helpers.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsHelpersParityProbe>")
    add_test(NAME threecpp.threejs-loader-foundation-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-loader-foundation.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsLoaderFoundationParityProbe>")
    add_test(NAME threecpp.threejs-loader-parsers-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-loader-parsers.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsLoaderParsersParityProbe>")
    add_test(NAME threecpp.threejs-interleaved-buffers-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-interleaved-buffers.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsInterleavedBufferParityProbe>")
    add_test(NAME threecpp.threejs-uniforms-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-uniforms.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsUniformsParityProbe>")
    add_test(NAME threecpp.threejs-uniforms-lib-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-uniforms-lib.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsUniformsLibParityProbe>")
    add_test(NAME threecpp.threejs-uniforms-source
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../utils/generate-uniforms.mjs"
        "${THREEJS_SOURCE_DIR}" --check)
    add_test(NAME threecpp.threejs-interleaved-attributes-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-interleaved-attributes.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsInterleavedAttributeParityProbe>")
    add_test(NAME threecpp.threejs-lod-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-lod.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsLODParityProbe>")
    add_test(NAME threecpp.threejs-primitive-objects-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-primitive-objects.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsPrimitiveObjectsParityProbe>")
    add_test(NAME threecpp.threejs-instancing-oracle
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/instancing-upstream-oracle.mjs"
        "${THREEJS_SOURCE_DIR}")
    add_test(NAME threecpp.threejs-lights-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-lights.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsLightsParityProbe>")
    add_test(NAME threecpp.threejs-materials-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-materials.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsMaterialParityProbe>")
    add_test(NAME threecpp.threejs-skinning-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-skinning.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsSkinningParityProbe>")
    add_test(NAME threecpp.threejs-render-targets-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-render-targets.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsRenderTargetParityProbe>")
    add_test(NAME threecpp.threejs-runtime-core-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-runtime-core.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsRuntimeCoreParityProbe>")
    add_test(NAME threecpp.threejs-animation-runtime-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" --expose-gc "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-animation-runtime.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsAnimationRuntimeParityProbe>")
    add_test(NAME threecpp.threejs-mixer-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-mixer.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsMixerParityProbe>")
    add_test(NAME threecpp.threejs-mixer-cache-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-mixer-cache.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsMixerCacheParityProbe>")
    add_test(NAME threecpp.threejs-action-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-action.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsActionParityProbe>")
    add_test(NAME threecpp.threejs-object-group-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-object-group.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsGroupParityProbe>")
    add_test(NAME threecpp.threejs-property-binding-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-property-binding.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsPropertyBindingParityProbe>")
    add_test(NAME threecpp.threejs-native-object-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-native-objects.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsNativeObjectParityProbe>")
    add_test(NAME threecpp.threejs-webgl-mipmaps-oracle
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/verify-webgl-mipmaps.mjs"
        "${THREEJS_SOURCE_DIR}")
    add_test(NAME threecpp.threejs-webgl-attributes-oracle
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/verify-webgl-attributes.mjs"
        "${THREEJS_SOURCE_DIR}")
    add_test(NAME threecpp.threejs-property-mixer-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-property-mixer.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsPropertyMixerParityProbe>")
    add_test(NAME threecpp.threejs-extrude-geometries-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-extrude-geometries.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsExtrudeGeometryParityProbe>")
    add_test(NAME threecpp.threejs-animation-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-animation.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsAnimationParityProbe>")
    add_test(NAME threecpp.threejs-primitive-geometries-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-primitive-geometries.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsPrimitiveGeometryParityProbe>")
    add_test(NAME threecpp.threejs-advanced-geometries-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-advanced-geometries.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsAdvancedGeometryParityProbe>")
    add_test(NAME threecpp.threejs-textures-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-textures.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsTextureParityProbe>")
    add_test(NAME threecpp.threejs-earcut-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-earcut.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsEarcutParityProbe>")
    add_test(NAME threecpp.threejs-data-utils-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-data-utils.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsDataUtilsParityProbe>")
    add_test(NAME threecpp.threejs-remaining-curves-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-remaining-curves.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsRemainingCurveParityProbe>")
    add_test(NAME threecpp.threejs-paths-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-paths.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsPathParityProbe>")
    add_test(NAME threecpp.threejs-interpolants-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-interpolants.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsInterpolantParityProbe>")
    add_test(NAME threecpp.port-audit
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/utils/port-audit.tests.mjs"
        "${THREEJS_SOURCE_DIR}")
    set_tests_properties(threecpp.port-audit PROPERTIES LABELS "Audit")
    add_test(NAME threecpp.threejs-extended-runtime-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-runtime-extended.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsExtendedRuntimeParityProbe>")
    add_test(NAME threecpp.threejs-curves-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-curves.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsCurveParityProbe>")
    add_test(NAME threecpp.threejs-storage-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-storage.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsStorageParityProbe>")
    add_test(NAME threecpp.threejs-runtime-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-runtime.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsRuntimeParityProbe>")
    add_test(NAME threecpp.threejs-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsParityProbe>")
    add_test(NAME threecpp.threejs-core-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-core.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsCoreParityProbe>")
    add_test(NAME threecpp.threejs-helper-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-helper.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsHelperParityProbe>")
    add_test(NAME threecpp.threejs-raycast-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-raycast.mjs"
        "${THREEJS_SOURCE_DIR}" "$<TARGET_FILE:ThreeJsRaycastParityProbe>")
    add_test(NAME threecpp.threejs-query-parity
        COMMAND "${THREECPP_NODE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/unit/utils/parity/compare-query.mjs"
        "$<TARGET_FILE:ThreeJsQueryParityProbe>" "${THREEJS_SOURCE_DIR}")
endif()
