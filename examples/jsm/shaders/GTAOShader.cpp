// Native port of the pinned examples/jsm/shaders/GTAOShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/GTAOShader.h"
#include "constants.h"
#include "core/NativeObject.h"
#include "core/TypedArray.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "textures/DataTexture.h"
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <memory>
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

namespace GTAOShader {
namespace {

// generateMagicSquareNoise( size = 5 ) and its module-local magic square helper.
std::vector<double> generateMagicSquare(double size) {
    const double floored = std::floor(size);
    const double noiseSize = std::fmod(floored, 2) == 0 ? floored + 1 : floored;
    const std::size_t noiseSquareSize = std::size_t(noiseSize * noiseSize);
    std::vector<double> magicSquare(noiseSquareSize, 0);
    double i = std::floor(noiseSize / 2);
    double j = noiseSize - 1;
    for (double number = 1; number <= double(noiseSquareSize);) {
        if (i == -1 && j == noiseSize) {
            j = noiseSize - 2;
            i = 0;
        } else {
            if (j == noiseSize) j = 0;
            if (i < 0) i = noiseSize - 1;
        }
        if (magicSquare[std::size_t(i * noiseSize + j)] != 0) {
            j -= 2;
            i += 1;
            continue;
        }
        magicSquare[std::size_t(i * noiseSize + j)] = number;
        number += 1;
        j += 1;
        i -= 1;
    }
    return magicSquare;
}
JSValue generateMagicSquareNoise(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    const double size = args.empty() || args[0].isUndefined() ? 5 : args[0].toNumber();
    const double floored = std::floor(size);
    const double noiseSize = std::fmod(floored, 2) == 0 ? floored + 1 : floored;
    const std::vector<double> magicSquare = generateMagicSquare(noiseSize);
    const std::size_t noiseSquareSize = magicSquare.size();
    Uint8Array data(noiseSquareSize * 4);
    for (std::size_t inx = 0; inx < noiseSquareSize; ++inx) {
        const double iAng = magicSquare[inx];
        const double angle = (2 * MathUtils::PI * iAng) / double(noiseSquareSize);
        Vector3 randomVec = Vector3(JSNumberMath::cos(angle), JSNumberMath::sin(angle), 0);
        randomVec.normalize();
        data[inx * 4] = (randomVec.x * 0.5 + 0.5) * 255;
        data[inx * 4 + 1] = (randomVec.y * 0.5 + 0.5) * 255;
        data[inx * 4 + 2] = 127;
        data[inx * 4 + 3] = 255;
    }
    auto noiseTexture = std::make_shared<DataTexture>(TextureImage::Data(data), noiseSize, noiseSize);
    noiseTexture->wrapS = RepeatWrapping;
    noiseTexture->wrapT = RepeatWrapping;
    noiseTexture->needsUpdate = true;
    return nativeTexture(noiseTexture);
}

// Byte-exact upstream strings.
constexpr std::string_view GTAOShader_GTAOBlendShader_vertexShader = R"jsm1(
		varying vec2 vUv;

		void main() {
			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
		})jsm1";
constexpr std::string_view GTAOShader_GTAOBlendShader_fragmentShader = R"jsm2(
		uniform float intensity;
		uniform sampler2D tDiffuse;
		varying vec2 vUv;

		void main() {
			vec4 texel = texture2D( tDiffuse, vUv );
			gl_FragColor = vec4(mix(vec3(1.), texel.rgb, intensity), texel.a);
		})jsm2";
constexpr std::string_view GTAOShader_GTAODepthShader_vertexShader = R"jsm3(
		varying vec2 vUv;

		void main() {
			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
		})jsm3";
constexpr std::string_view GTAOShader_GTAODepthShader_fragmentShader = R"jsm4(
		uniform sampler2D tDepth;
		uniform float cameraNear;
		uniform float cameraFar;
		varying vec2 vUv;

		#include <packing>

		float getLinearDepth( const in vec2 screenPosition ) {
			#if PERSPECTIVE_CAMERA == 1
				float fragCoordZ = texture2D( tDepth, screenPosition ).x;
				float viewZ = perspectiveDepthToViewZ( fragCoordZ, cameraNear, cameraFar );
				return viewZToOrthographicDepth( viewZ, cameraNear, cameraFar );
			#else
				return texture2D( tDepth, screenPosition ).x;
			#endif
		}

		void main() {
			float depth = getLinearDepth( vUv );
			gl_FragColor = vec4( vec3( 1.0 - depth ), 1.0 );

		})jsm4";
constexpr std::string_view GTAOShader_GTAOShader_vertexShader = R"jsm5(

		varying vec2 vUv;

		void main() {
			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
		})jsm5";
constexpr std::string_view GTAOShader_GTAOShader_fragmentShader = R"jsm6(
		varying vec2 vUv;
		uniform highp sampler2D tNormal;
		uniform highp sampler2D tDepth;
		uniform sampler2D tNoise;
		uniform vec2 resolution;
		uniform float cameraNear;
		uniform float cameraFar;
		uniform mat4 cameraProjectionMatrix;
		uniform mat4 cameraProjectionMatrixInverse;
		uniform mat4 cameraWorldMatrix;
		uniform float radius;
		uniform float distanceExponent;
		uniform float thickness;
		uniform float distanceFallOff;
		uniform float scale;
		#if SCENE_CLIP_BOX == 1
			uniform vec3 sceneBoxMin;
			uniform vec3 sceneBoxMax;
		#endif

		#include <common>
		#include <packing>

		#ifndef FRAGMENT_OUTPUT
		#define FRAGMENT_OUTPUT vec4(vec3(ao), 1.)
		#endif

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

		float getViewZ(const in float depth) {
			#if PERSPECTIVE_CAMERA == 1
				return perspectiveDepthToViewZ(depth, cameraNear, cameraFar);
			#else
				return orthographicDepthToViewZ(depth, cameraNear, cameraFar);
			#endif
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
			vec3 dpdx = (dl < dr) ? ce - getViewPosition((uv - vec2(1.0 / size.x, 0.0)), l1).xyz : -ce + getViewPosition((uv + vec2(1.0 / size.x, 0.0)), r1).xyz;
			vec3 dpdy = (db < dt) ? ce - getViewPosition((uv - vec2(0.0, 1.0 / size.y)), b1).xyz : -ce + getViewPosition((uv + vec2(0.0, 1.0 / size.y)), t1).xyz;
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

		vec3 getSceneUvAndDepth(vec3 sampleViewPos) {
			vec4 sampleClipPos = cameraProjectionMatrix * vec4(sampleViewPos, 1.);
			vec2 sampleUv = sampleClipPos.xy / sampleClipPos.w * 0.5 + 0.5;
			float sampleSceneDepth = getDepth(sampleUv);
			return vec3(sampleUv, sampleSceneDepth);
		}

		void main() {
			float depth = getDepth(vUv.xy);

			#ifdef USE_REVERSED_DEPTH_BUFFER
				if (depth <= 0.0) {
					discard;
					return;
				}
			#else
				if (depth >= 1.0) {
					discard;
					return;
				}
			#endif
			
			vec3 viewPos = getViewPosition(vUv, depth);
			vec3 viewNormal = getViewNormal(vUv);

			float radiusToUse = radius;
			float distanceFalloffToUse = thickness;
			#if SCREEN_SPACE_RADIUS == 1
				float radiusScale = getViewPosition(vec2(0.5 + float(SCREEN_SPACE_RADIUS_SCALE) / resolution.x, 0.0), depth).x;
				radiusToUse *= radiusScale;
				distanceFalloffToUse *= radiusScale;
			#endif

			#if SCENE_CLIP_BOX == 1
				vec3 worldPos = (cameraWorldMatrix * vec4(viewPos, 1.0)).xyz;
				float boxDistance = length(max(vec3(0.0), max(sceneBoxMin - worldPos, worldPos - sceneBoxMax)));
				if (boxDistance > radiusToUse) {
					discard;
					return;
				}
			#endif

			vec2 noiseResolution = vec2(textureSize(tNoise, 0));
			vec2 noiseUv = vUv * resolution / noiseResolution;
			vec4 noiseTexel = textureLod(tNoise, noiseUv, 0.0);
			vec3 randomVec = noiseTexel.xyz * 2.0 - 1.0;
			vec3 tangent = normalize(vec3(randomVec.xy, 0.));
			vec3 bitangent = vec3(-tangent.y, tangent.x, 0.);
			mat3 kernelMatrix = mat3(tangent, bitangent, vec3(0., 0., 1.));

			const int DIRECTIONS = SAMPLES < 30 ? 3 : 5;
			const int STEPS = (SAMPLES + DIRECTIONS - 1) / DIRECTIONS;
			float ao = 0.0;
			for (int i = 0; i < DIRECTIONS; ++i) {

				float angle = float(i) / float(DIRECTIONS) * PI;
				vec4 sampleDir = vec4(cos(angle), sin(angle), 0., 0.5 + 0.5 * noiseTexel.w);
				sampleDir.xyz = normalize(kernelMatrix * sampleDir.xyz);

				vec3 viewDir = normalize(-viewPos.xyz);
				vec3 sliceBitangent = normalize(cross(sampleDir.xyz, viewDir));
				vec3 sliceTangent = cross(sliceBitangent, viewDir);
				vec3 normalInSlice = normalize(viewNormal - sliceBitangent * dot(viewNormal, sliceBitangent));

				vec3 tangentToNormalInSlice = cross(normalInSlice, sliceBitangent);
				vec2 cosHorizons = vec2(dot(viewDir, tangentToNormalInSlice), dot(viewDir, -tangentToNormalInSlice));

				for (int j = 0; j < STEPS; ++j) {
					vec3 sampleViewOffset = sampleDir.xyz * radiusToUse * sampleDir.w * pow(float(j + 1) / float(STEPS), distanceExponent);

					vec3 sampleSceneUvDepth = getSceneUvAndDepth(viewPos + sampleViewOffset);
					vec3 sampleSceneViewPos = getViewPosition(sampleSceneUvDepth.xy, sampleSceneUvDepth.z);
					vec3 viewDelta = sampleSceneViewPos - viewPos;
					if (abs(viewDelta.z) < thickness) {
						float sampleCosHorizon = dot(viewDir, normalize(viewDelta));
						cosHorizons.x += max(0., (sampleCosHorizon - cosHorizons.x) * mix(1., 2. / float(j + 2), distanceFallOff));
					}

					sampleSceneUvDepth = getSceneUvAndDepth(viewPos - sampleViewOffset);
					sampleSceneViewPos = getViewPosition(sampleSceneUvDepth.xy, sampleSceneUvDepth.z);
					viewDelta = sampleSceneViewPos - viewPos;
					if (abs(viewDelta.z) < thickness) {
						float sampleCosHorizon = dot(viewDir, normalize(viewDelta));
						cosHorizons.y += max(0., (sampleCosHorizon - cosHorizons.y) * mix(1., 2. / float(j + 2), distanceFallOff));
					}
				}

				vec2 sinHorizons = sqrt(1. - cosHorizons * cosHorizons);
				float nx = dot(normalInSlice, sliceTangent);
				float ny = dot(normalInSlice, viewDir);
				float nxb = 1. / 2. * (acos(cosHorizons.y) - acos(cosHorizons.x) + sinHorizons.x * cosHorizons.x - sinHorizons.y * cosHorizons.y);
				float nyb = 1. / 2. * (2. - cosHorizons.x * cosHorizons.x - cosHorizons.y * cosHorizons.y);
				float occlusion = nx * nxb + ny * nyb;
				ao += occlusion;
			}

			ao = clamp(ao / float(DIRECTIONS), 0., 1.);
		#if SCENE_CLIP_BOX == 1
			ao = mix(ao, 1., smoothstep(0., radiusToUse, boxDistance));
		#endif
			ao = pow(ao, scale);

			gl_FragColor = FRAGMENT_OUTPUT;
		})jsm6";
} // namespace

JSValue nativeGTAOShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "GTAOShader");
    JSValue result = object({
        {"GTAOBlendShader", object({
            {"name", "GTAOBlendShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"intensity", object({
                    {"value", 1}
                })}
            })},
            {"vertexShader", std::string(GTAOShader_GTAOBlendShader_vertexShader)},
            {"fragmentShader", std::string(GTAOShader_GTAOBlendShader_fragmentShader)}
        })},
        {"GTAODepthShader", object({
            {"name", "GTAODepthShader"},
            {"defines", object({
                {"PERSPECTIVE_CAMERA", 1}
            })},
            {"uniforms", object({
                {"tDepth", object({
                    {"value", nullptr}
                })},
                {"cameraNear", object({
                    {"value", nullptr}
                })},
                {"cameraFar", object({
                    {"value", nullptr}
                })}
            })},
            {"vertexShader", std::string(GTAOShader_GTAODepthShader_vertexShader)},
            {"fragmentShader", std::string(GTAOShader_GTAODepthShader_fragmentShader)}
        })},
        {"GTAOShader", object({
            {"name", "GTAOShader"},
            {"defines", object({
                {"PERSPECTIVE_CAMERA", 1},
                {"SAMPLES", 16},
                {"NORMAL_VECTOR_TYPE", 1},
                {"SCREEN_SPACE_RADIUS", 0},
                {"SCREEN_SPACE_RADIUS_SCALE", 100},
                {"SCENE_CLIP_BOX", 0}
            })},
            {"uniforms", object({
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
                {"cameraNear", object({
                    {"value", nullptr}
                })},
                {"cameraFar", object({
                    {"value", nullptr}
                })},
                {"cameraProjectionMatrix", object({
                    {"value", nativeMatrix(std::make_shared<Matrix4>())}
                })},
                {"cameraProjectionMatrixInverse", object({
                    {"value", nativeMatrix(std::make_shared<Matrix4>())}
                })},
                {"cameraWorldMatrix", object({
                    {"value", nativeMatrix(std::make_shared<Matrix4>())}
                })},
                {"radius", object({
                    {"value", 0.25}
                })},
                {"distanceExponent", object({
                    {"value", 1}
                })},
                {"thickness", object({
                    {"value", 1}
                })},
                {"distanceFallOff", object({
                    {"value", 1}
                })},
                {"scale", object({
                    {"value", 1}
                })},
                {"sceneBoxMin", object({
                    {"value", nativeVector(std::make_shared<Vector3>(-1, -1, -1))}
                })},
                {"sceneBoxMax", object({
                    {"value", nativeVector(std::make_shared<Vector3>(1, 1, 1))}
                })}
            })},
            {"vertexShader", std::string(GTAOShader_GTAOShader_vertexShader)},
            {"fragmentShader", std::string(GTAOShader_GTAOShader_fragmentShader)}
        })},
        {"generateMagicSquareNoise", JSValue::function(generateMagicSquareNoise, {}, "generateMagicSquareNoise", 0)}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace GTAOShader
