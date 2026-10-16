// Native port of the pinned examples/jsm/shaders/SSAOShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/SSAOShader.h"
#include "core/NativeObject.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
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

namespace SSAOShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view SSAOShader_SSAOBlurShader_vertexShader = R"jsm1(varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm1";
constexpr std::string_view SSAOShader_SSAOBlurShader_fragmentShader = R"jsm2(uniform sampler2D tDiffuse;

		uniform vec2 resolution;

		varying vec2 vUv;

		void main() {

			vec2 texelSize = ( 1.0 / resolution );
			float result = 0.0;

			for ( int i = - 2; i <= 2; i ++ ) {

				for ( int j = - 2; j <= 2; j ++ ) {

					vec2 offset = ( vec2( float( i ), float( j ) ) ) * texelSize;
					result += texture2D( tDiffuse, vUv + offset ).r;

				}

			}

			gl_FragColor = vec4( vec3( result / ( 5.0 * 5.0 ) ), 1.0 );

		})jsm2";
constexpr std::string_view SSAOShader_SSAODepthShader_vertexShader = R"jsm3(varying vec2 vUv;

		void main() {

			vUv = uv;
			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm3";
constexpr std::string_view SSAOShader_SSAODepthShader_fragmentShader = R"jsm4(uniform sampler2D tDepth;

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
constexpr std::string_view SSAOShader_SSAOShader_vertexShader = R"jsm5(

		varying vec2 vUv;

		void main() {

			vUv = uv;

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

		})jsm5";
constexpr std::string_view SSAOShader_SSAOShader_fragmentShader = R"jsm6(
		uniform highp sampler2D tNormal;
		uniform highp sampler2D tDepth;
		uniform sampler2D tNoise;

		uniform vec3 kernel[ KERNEL_SIZE ];

		uniform vec2 resolution;

		uniform float cameraNear;
		uniform float cameraFar;
		uniform mat4 cameraProjectionMatrix;
		uniform mat4 cameraInverseProjectionMatrix;

		uniform float kernelRadius;
		uniform float minDistance; // avoid artifacts caused by neighbour fragments with minimal depth difference
		uniform float maxDistance; // avoid the influence of fragments which are too far away

		varying vec2 vUv;

		#include <packing>

		#ifdef USE_REVERSED_DEPTH_BUFFER

			const float depthThreshold = 0.0;

		#else

			const float depthThreshold = 1.0;

		#endif

		float getDepth( const in vec2 screenPosition ) {

			return texture2D( tDepth, screenPosition ).x;

		}

		float getLinearDepth( const in vec2 screenPosition ) {

			#if PERSPECTIVE_CAMERA == 1

				float fragCoordZ = texture2D( tDepth, screenPosition ).x;
				float viewZ = perspectiveDepthToViewZ( fragCoordZ, cameraNear, cameraFar );
				return viewZToOrthographicDepth( viewZ, cameraNear, cameraFar );

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

		vec3 getViewPosition( const in vec2 screenPosition, const in float depth, const in float viewZ ) {

			float clipW = cameraProjectionMatrix[2][3] * viewZ + cameraProjectionMatrix[3][3];

			vec4 clipPosition = vec4( ( vec3( screenPosition, depth ) - 0.5 ) * 2.0, 1.0 );

			clipPosition *= clipW; // unprojection.

			return ( cameraInverseProjectionMatrix * clipPosition ).xyz;

		}

		vec3 getViewNormal( const in vec2 screenPosition ) {

			return unpackRGBToNormal( texture2D( tNormal, screenPosition ).xyz );

		}

		void main() {

			float depth = getDepth( vUv );

			if ( depth == depthThreshold ) {

				gl_FragColor = vec4( 1.0 ); // don't influence background

			} else {

				float viewZ = getViewZ( depth );

				vec3 viewPosition = getViewPosition( vUv, depth, viewZ );
				vec3 viewNormal = getViewNormal( vUv );

				vec2 noiseScale = vec2( resolution.x / 4.0, resolution.y / 4.0 );
				vec3 random = vec3( texture2D( tNoise, vUv * noiseScale ).r );

				// compute matrix used to reorient a kernel vector

				vec3 tangent = normalize( random - viewNormal * dot( random, viewNormal ) );
				vec3 bitangent = cross( viewNormal, tangent );
				mat3 kernelMatrix = mat3( tangent, bitangent, viewNormal );

				float occlusion = 0.0;

				for ( int i = 0; i < KERNEL_SIZE; i ++ ) {

					vec3 sampleVector = kernelMatrix * kernel[ i ]; // reorient sample vector in view space
					vec3 samplePoint = viewPosition + ( sampleVector * kernelRadius ); // calculate sample point

					vec4 samplePointNDC = cameraProjectionMatrix * vec4( samplePoint, 1.0 ); // project point and calculate NDC
					samplePointNDC /= samplePointNDC.w;

					vec2 samplePointUv = samplePointNDC.xy * 0.5 + 0.5; // compute uv coordinates

					float realDepth = getLinearDepth( samplePointUv ); // get linear depth from depth texture
					float sampleDepth = viewZToOrthographicDepth( samplePoint.z, cameraNear, cameraFar ); // compute linear depth of the sample view Z value
					float delta = sampleDepth - realDepth;

					if ( delta > minDistance && delta < maxDistance ) { // if fragment is before sample point, increase occlusion

						occlusion += 1.0;

					}

				}

				occlusion = clamp( occlusion / float( KERNEL_SIZE ), 0.0, 1.0 );

				gl_FragColor = vec4( vec3( 1.0 - occlusion ), 1.0 );

			}

		})jsm6";
} // namespace

JSValue nativeSSAOShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "SSAOShader");
    JSValue result = object({
        {"SSAOBlurShader", object({
            {"name", "SSAOBlurShader"},
            {"uniforms", object({
                {"tDiffuse", object({
                    {"value", nullptr}
                })},
                {"resolution", object({
                    {"value", nativeVector(std::make_shared<Vector2>(0, 0))}
                })}
            })},
            {"vertexShader", std::string(SSAOShader_SSAOBlurShader_vertexShader)},
            {"fragmentShader", std::string(SSAOShader_SSAOBlurShader_fragmentShader)}
        })},
        {"SSAODepthShader", object({
            {"name", "SSAODepthShader"},
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
            {"vertexShader", std::string(SSAOShader_SSAODepthShader_vertexShader)},
            {"fragmentShader", std::string(SSAOShader_SSAODepthShader_fragmentShader)}
        })},
        {"SSAOShader", object({
            {"name", "SSAOShader"},
            {"defines", object({
                {"PERSPECTIVE_CAMERA", 1},
                {"KERNEL_SIZE", 32}
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
                {"kernel", object({
                    {"value", nullptr}
                })},
                {"cameraNear", object({
                    {"value", nullptr}
                })},
                {"cameraFar", object({
                    {"value", nullptr}
                })},
                {"resolution", object({
                    {"value", nativeVector(std::make_shared<Vector2>(0, 0))}
                })},
                {"cameraProjectionMatrix", object({
                    {"value", nativeMatrix(std::make_shared<Matrix4>())}
                })},
                {"cameraInverseProjectionMatrix", object({
                    {"value", nativeMatrix(std::make_shared<Matrix4>())}
                })},
                {"kernelRadius", object({
                    {"value", 8}
                })},
                {"minDistance", object({
                    {"value", 0.005}
                })},
                {"maxDistance", object({
                    {"value", 0.05}
                })}
            })},
            {"vertexShader", std::string(SSAOShader_SSAOShader_vertexShader)},
            {"fragmentShader", std::string(SSAOShader_SSAOShader_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace SSAOShader
