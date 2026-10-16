// Native port of the pinned examples/jsm/shaders/PoissonDenoiseShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/PoissonDenoiseShader.h"
#include "core/NativeObject.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

namespace PoissonDenoiseShader {
namespace {

// generatePdSamplePointInitializer( samples, rings, radiusExponent ) and its
// module-local poisson disk generator. The GLSL text keeps the pinned template
// literal interpolation, so the numbers use the ES Number::toString spelling.
std::vector<Vector3> generateDenoiseSamples(double numSamples, double numRings, double radiusExponent) {
    std::vector<Vector3> samples;
    for (double i = 0; i < numSamples; i += 1) {
        const double angle = 2 * MathUtils::PI * numRings * i / numSamples;
        const double radius = JSNumberMath::pow(i / (numSamples - 1), radiusExponent);
        samples.push_back(Vector3(JSNumberMath::cos(angle), JSNumberMath::sin(angle), radius));
    }
    return samples;
}
JSValue generatePdSamplePointInitializer(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    const double numSamples = args[0].toNumber(), numRings = args[1].toNumber();
    const double radiusExponent = args[2].toNumber();
    const std::vector<Vector3> poissonDisk = generateDenoiseSamples(numSamples, numRings, radiusExponent);
    std::string glslCode = "vec3[SAMPLES](";
    for (double i = 0; i < numSamples; i += 1) {
        const Vector3& sample = poissonDisk[std::size_t(i)];
        glslCode += "vec3(" + JSValue(sample.x).toString() + ", " + JSValue(sample.y).toString() + ", " +
            JSValue(sample.z).toString() + ")" + (i < numSamples - 1 ? "," : ")");
    }
    return JSValue(glslCode);
}

// Byte-exact upstream strings.
constexpr std::string_view PoissonDenoiseShader_PoissonDenoiseShader_defines_SAMPLE_VECTORS = R"jsm1(vec3[SAMPLES](vec3(1, 0, 0),vec3(0.7071067811865476, 0.7071067811865475, 0.06666666666666667),vec3(6.123233995736766e-17, 1, 0.13333333333333333),vec3(-0.7071067811865475, 0.7071067811865476, 0.2),vec3(-1, 1.2246467991473532e-16, 0.26666666666666666),vec3(-0.7071067811865477, -0.7071067811865475, 0.3333333333333333),vec3(-1.8369701987210297e-16, -1, 0.4),vec3(0.7071067811865474, -0.7071067811865477, 0.4666666666666667),vec3(1, -2.4492935982947064e-16, 0.5333333333333333),vec3(0.7071067811865477, 0.7071067811865474, 0.6),vec3(3.061616997868383e-16, 1, 0.6666666666666666),vec3(-0.7071067811865467, 0.7071067811865483, 0.7333333333333333),vec3(-1, 3.6739403974420594e-16, 0.8),vec3(-0.7071067811865471, -0.7071067811865479, 0.8666666666666667),vec3(-4.286263797015736e-16, -1, 0.9333333333333333),vec3(0.7071067811865466, -0.7071067811865485, 1)))jsm1";
constexpr std::string_view PoissonDenoiseShader_PoissonDenoiseShader_vertexShader = R"jsm2(

		varying vec2 vUv;

		void main() {
			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
		})jsm2";
constexpr std::string_view PoissonDenoiseShader_PoissonDenoiseShader_fragmentShader = R"jsm3(

		varying vec2 vUv;

		uniform sampler2D tDiffuse;
		uniform sampler2D tNormal;
		uniform sampler2D tDepth;
		uniform sampler2D tNoise;
		uniform vec2 resolution;
		uniform mat4 cameraProjectionMatrixInverse;
		uniform float lumaPhi;
		uniform float depthPhi;
		uniform float normalPhi;
		uniform float radius;
		uniform int index;

		#include <common>
		#include <packing>

		#ifndef SAMPLE_LUMINANCE
		#define SAMPLE_LUMINANCE dot(vec3(0.2125, 0.7154, 0.0721), a)
		#endif

		#ifndef FRAGMENT_OUTPUT
		#define FRAGMENT_OUTPUT vec4(denoised, 1.)
		#endif

		float getLuminance(const in vec3 a) {
			return SAMPLE_LUMINANCE;
		}

		const vec3 poissonDisk[SAMPLES] = SAMPLE_VECTORS;

		vec3 getViewPosition( const in vec2 screenPosition, const in float depth ) {
			#ifdef USE_REVERSED_DEPTH_BUFFER
				vec4 clipSpacePosition = vec4( vec2( screenPosition ) * 2.0 - 1.0, depth, 1.0 );
			#else
				vec4 clipSpacePosition = vec4( vec3( screenPosition, depth ) * 2.0 - 1.0, 1.0 );
			#endif
			vec4 viewSpacePosition = cameraProjectionMatrixInverse * clipSpacePosition;
			return viewSpacePosition.xyz / viewSpacePosition.w;
		}

		float getDepth(const vec2 uv) {
			return textureLod(tDepth, uv.xy, 0.0).r;
		}

		float fetchDepth(const ivec2 uv) {
			return texelFetch(tDepth, uv.xy, 0).r;
		}

		vec3 computeNormalFromDepth(const vec2 uv) {
			vec2 size = vec2(textureSize(tDepth, 0));
			ivec2 p = ivec2(uv * size);
			float c0 = fetchDepth(p);
			float l2 = fetchDepth(p - ivec2(2, 0));
			float l1 = fetchDepth(p - ivec2(1, 0));
			float r1 = fetchDepth(p + ivec2(1, 0));
			float r2 = fetchDepth(p + ivec2(2, 0));
			float b2 = fetchDepth(p - ivec2(0, 2));
			float b1 = fetchDepth(p - ivec2(0, 1));
			float t1 = fetchDepth(p + ivec2(0, 1));
			float t2 = fetchDepth(p + ivec2(0, 2));
			float dl = abs((2.0 * l1 - l2) - c0);
			float dr = abs((2.0 * r1 - r2) - c0);
			float db = abs((2.0 * b1 - b2) - c0);
			float dt = abs((2.0 * t1 - t2) - c0);
			vec3 ce = getViewPosition(uv, c0).xyz;
			vec3 dpdx = (dl < dr) ?  ce - getViewPosition((uv - vec2(1.0 / size.x, 0.0)), l1).xyz
									: -ce + getViewPosition((uv + vec2(1.0 / size.x, 0.0)), r1).xyz;
			vec3 dpdy = (db < dt) ?  ce - getViewPosition((uv - vec2(0.0, 1.0 / size.y)), b1).xyz
									: -ce + getViewPosition((uv + vec2(0.0, 1.0 / size.y)), t1).xyz;
			return normalize(cross(dpdx, dpdy));
		}

		vec3 getViewNormal(const vec2 uv) {
		#if NORMAL_VECTOR_TYPE == 2
			return normalize(textureLod(tNormal, uv, 0.).rgb);
		#elif NORMAL_VECTOR_TYPE == 1
			return unpackRGBToNormal(textureLod(tNormal, uv, 0.).rgb);
		#else
			return computeNormalFromDepth(uv);
		#endif
		}

		void denoiseSample(in vec3 center, in vec3 viewNormal, in vec3 viewPos, in vec2 sampleUv, inout vec3 denoised, inout float totalWeight) {
			vec4 sampleTexel = textureLod(tDiffuse, sampleUv, 0.0);
			float sampleDepth = getDepth(sampleUv);
			vec3 sampleNormal = getViewNormal(sampleUv);
			vec3 neighborColor = sampleTexel.rgb;
			vec3 viewPosSample = getViewPosition(sampleUv, sampleDepth);

			float normalDiff = dot(viewNormal, sampleNormal);
			float normalSimilarity = pow(max(normalDiff, 0.), normalPhi);
			float lumaDiff = abs(getLuminance(neighborColor) - getLuminance(center));
			float lumaSimilarity = max(1.0 - lumaDiff / lumaPhi, 0.0);
			float depthDiff = abs(dot(viewPos - viewPosSample, viewNormal));
			float depthSimilarity = max(1. - depthDiff / depthPhi, 0.);
			float w = lumaSimilarity * depthSimilarity * normalSimilarity;

			denoised += w * neighborColor;
			totalWeight += w;
		}

		void main() {
			float depth = getDepth(vUv.xy);
			vec3 viewNormal = getViewNormal(vUv);
			if (depth == 1. || dot(viewNormal, viewNormal) == 0.) {
				discard;
				return;
			}
			vec4 texel = textureLod(tDiffuse, vUv, 0.0);
			vec3 center = texel.rgb;
			vec3 viewPos = getViewPosition(vUv, depth);

			vec2 noiseResolution = vec2(textureSize(tNoise, 0));
			vec2 noiseUv = vUv * resolution / noiseResolution;
			vec4 noiseTexel = textureLod(tNoise, noiseUv, 0.0);
      		vec2 noiseVec = vec2(sin(noiseTexel[index % 4] * 2. * PI), cos(noiseTexel[index % 4] * 2. * PI));
    		mat2 rotationMatrix = mat2(noiseVec.x, -noiseVec.y, noiseVec.x, noiseVec.y);

			float totalWeight = 1.0;
			vec3 denoised = texel.rgb;
			for (int i = 0; i < SAMPLES; i++) {
				vec3 sampleDir = poissonDisk[i];
				vec2 offset = rotationMatrix * (sampleDir.xy * (1. + sampleDir.z * (radius - 1.)) / resolution);
				vec2 sampleUv = vUv + offset;
				denoiseSample(center, viewNormal, viewPos, sampleUv, denoised, totalWeight);
			}

			if (totalWeight > 0.) {
				denoised /= totalWeight;
			}
			gl_FragColor = FRAGMENT_OUTPUT;
		})jsm3";
} // namespace

JSValue nativePoissonDenoiseShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "PoissonDenoiseShader");
    JSValue result = object({
        {"PoissonDenoiseShader", object({
            {"name", "PoissonDenoiseShader"},
            {"defines", object({
                {"SAMPLES", 16},
                {"SAMPLE_VECTORS", std::string(PoissonDenoiseShader_PoissonDenoiseShader_defines_SAMPLE_VECTORS)},
                {"NORMAL_VECTOR_TYPE", 1}
            })},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"tNormal", object({
                    {"value", nullptr}
                })},
                {"tDepth", object({
                    {"value", nullptr}
                })},
                {"tNoise", object({
                    {"value", nullptr}
                })},
                {"resolution", object({
                    {"value", nativeVector(std::make_shared<Vector2>(0, 0))}
                })},
                {"cameraProjectionMatrixInverse", object({
                    {"value", nativeMatrix(std::make_shared<Matrix4>())}
                })},
                {"lumaPhi", object({
                    {"value", 5}
                })},
                {"depthPhi", object({
                    {"value", 5}
                })},
                {"normalPhi", object({
                    {"value", 5}
                })},
                {"radius", object({
                    {"value", 4}
                })},
                {"index", object({
                    {"value", 0}
                })}
            })},
            {"vertexShader", std::string(PoissonDenoiseShader_PoissonDenoiseShader_vertexShader)},
            {"fragmentShader", std::string(PoissonDenoiseShader_PoissonDenoiseShader_fragmentShader)}
        })},
        {"generatePdSamplePointInitializer", JSValue::function(generatePdSamplePointInitializer, {}, "generatePdSamplePointInitializer", 3)}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace PoissonDenoiseShader
