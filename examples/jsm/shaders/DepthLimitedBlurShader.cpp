// Native port of the pinned examples/jsm/shaders/DepthLimitedBlurShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/DepthLimitedBlurShader.h"
#include "core/NativeObject.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/Vector2.h"
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace {
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields) {
    auto result=JSValue::object();
    for(const auto& [key,value]:fields){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(key,d);}
    return result;
}
JSValue array(std::initializer_list<JSValue> values) {
    auto result=JSValue::array();std::size_t index=0;
    for(const auto& value:values){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(std::to_string(index++),d);}
    return result;
}
} // namespace

namespace DepthLimitedBlurShader {
namespace {

// BlurShaderUtils: the three helpers of the pinned module, including the module-local
// `gaussian` they call.
double gaussian(double x, double stdDev) {
    return JSNumberMath::exp(-(x * x) / (2.0 * (stdDev * stdDev))) / (std::sqrt(2.0 * MathUtils::PI) * stdDev);
}
JSValue createSampleWeights(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    const double kernelRadius = args[0].toNumber(), stdDev = args[1].toNumber();
    auto weights = array({});
    for (double i = 0; i <= kernelRadius; i += 1)
        weights.get("push").call(weights, {JSValue(gaussian(i, stdDev))});
    return weights;
}
JSValue createSampleOffsets(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    const double kernelRadius = args[0].toNumber();
    const JSValue uvIncrement = args[1];
    auto offsets = array({});
    for (double i = 0; i <= kernelRadius; i += 1) {
        const JSValue offset = uvIncrement.get("clone").call(uvIncrement);
        offset.get("multiplyScalar").call(offset, {JSValue(i)});
        offsets.get("push").call(offsets, {offset});
    }
    return offsets;
}
JSValue configure(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    JSValue material = args[0];
    const double kernelRadius = args[1].toNumber(), stdDev = args[2].toNumber();
    const JSValue uvIncrement = args[3];
    JSValue defines = material.get("defines");
    defines.set("KERNEL_RADIUS", kernelRadius);
    JSValue uniforms = material.get("uniforms");
    JSValue offsetsEntry = uniforms.get("sampleUvOffsets");
    offsetsEntry.set("value", createSampleOffsets(JSValue{}, {args[1], uvIncrement}, {}));
    JSValue weightsEntry = uniforms.get("sampleWeights");
    weightsEntry.set("value", createSampleWeights(JSValue{}, {args[1], args[2]}, {}));
    material.set("needsUpdate", true);
    return JSValue{};
}
JSValue blurShaderUtils() {
    auto result = object({});
    result.set("createSampleWeights", JSValue::function(createSampleWeights, {}, "createSampleWeights", 2));
    result.set("createSampleOffsets", JSValue::function(createSampleOffsets, {}, "createSampleOffsets", 2));
    result.set("configure", JSValue::function(configure, {}, "configure", 4));
    return result;
}

// Byte-exact upstream strings.
constexpr std::string_view DepthLimitedBlurShader_DepthLimitedBlurShader_vertexShader = R"jsm1(

		#include <common>

		uniform vec2 size;

		varying vec2 vUv;
		varying vec2 vInvSize;

		void main() {
			vUv = uv;
			vInvSize = 1.0 / size;

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
		})jsm1";
constexpr std::string_view DepthLimitedBlurShader_DepthLimitedBlurShader_fragmentShader = R"jsm2(

		#include <common>
		#include <packing>

		uniform sampler2D tDiffuse;
		uniform sampler2D tDepth;

		uniform float cameraNear;
		uniform float cameraFar;
		uniform float depthCutoff;

		uniform vec2 sampleUvOffsets[ KERNEL_RADIUS + 1 ];
		uniform float sampleWeights[ KERNEL_RADIUS + 1 ];

		varying vec2 vUv;
		varying vec2 vInvSize;

		float getDepth( const in vec2 screenPosition ) {
			#if DEPTH_PACKING == 1
			return unpackRGBAToDepth( texture2D( tDepth, screenPosition ) );
			#else
			return texture2D( tDepth, screenPosition ).x;
			#endif
		}

		float getViewZ( const in float depth ) {
			#if PERSPECTIVE_CAMERA == 1
			return perspectiveDepthToViewZ( depth, cameraNear, cameraFar );
			#else
			return orthographicDepthToViewZ( depth, cameraNear, cameraFar );
			#endif
		}

		void main() {
			float depth = getDepth( vUv );
			if( depth >= ( 1.0 - EPSILON ) ) {
				discard;
			}

			float centerViewZ = -getViewZ( depth );
			bool rBreak = false, lBreak = false;

			float weightSum = sampleWeights[0];
			vec4 diffuseSum = texture2D( tDiffuse, vUv ) * weightSum;

			for( int i = 1; i <= KERNEL_RADIUS; i ++ ) {

				float sampleWeight = sampleWeights[i];
				vec2 sampleUvOffset = sampleUvOffsets[i] * vInvSize;

				vec2 sampleUv = vUv + sampleUvOffset;
				float viewZ = -getViewZ( getDepth( sampleUv ) );

				if( abs( viewZ - centerViewZ ) > depthCutoff ) rBreak = true;

				if( ! rBreak ) {
					diffuseSum += texture2D( tDiffuse, sampleUv ) * sampleWeight;
					weightSum += sampleWeight;
				}

				sampleUv = vUv - sampleUvOffset;
				viewZ = -getViewZ( getDepth( sampleUv ) );

				if( abs( viewZ - centerViewZ ) > depthCutoff ) lBreak = true;

				if( ! lBreak ) {
					diffuseSum += texture2D( tDiffuse, sampleUv ) * sampleWeight;
					weightSum += sampleWeight;
				}

			}

			gl_FragColor = diffuseSum / weightSum;
		})jsm2";
} // namespace

JSValue nativeDepthLimitedBlurShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "DepthLimitedBlurShader");
    JSValue result = object({
        {"BlurShaderUtils", blurShaderUtils()},
        {"DepthLimitedBlurShader", object({
            {"name", "DepthLimitedBlurShader"},
            {"defines", object({
                {"KERNEL_RADIUS", 4},
                {"DEPTH_PACKING", 1},
                {"PERSPECTIVE_CAMERA", 1}
            })},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"size", object({
                    {"value", nativeVector(std::make_shared<Vector2>(512, 512))}
                })},
                {"sampleUvOffsets", object({
                    {"value", array({
                        nativeVector(std::make_shared<Vector2>(0, 0))
                    })}
                })},
                {"sampleWeights", object({
                    {"value", array({
                        1
                    })}
                })},
                {"tDepth", object({
                    {"value", nullptr}
                })},
                {"cameraNear", object({
                    {"value", 10}
                })},
                {"cameraFar", object({
                    {"value", 1000}
                })},
                {"depthCutoff", object({
                    {"value", 10}
                })}
            })},
            {"vertexShader", std::string(DepthLimitedBlurShader_DepthLimitedBlurShader_vertexShader)},
            {"fragmentShader", std::string(DepthLimitedBlurShader_DepthLimitedBlurShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace DepthLimitedBlurShader
