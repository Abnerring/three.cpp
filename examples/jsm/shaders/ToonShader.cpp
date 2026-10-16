// Native port of the pinned examples/jsm/shaders/ToonShader.js (see upstream.json).
// The GLSL and defines strings are the byte-exact template literal values of that
// revision; the exported objects keep the upstream field order.
#include "shaders/ToonShader.h"
#include "core/NativeObject.h"
#include "math/Color.h"
#include "math/Vector3.h"
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

namespace ToonShader {
namespace {

// Byte-exact upstream strings.
constexpr std::string_view ToonShader_ToonShader1_vertexShader = R"jsm1(

		varying vec3 vNormal;
		varying vec3 vRefract;

		void main() {

			vec4 worldPosition = modelMatrix * vec4( position, 1.0 );
			vec4 mvPosition = modelViewMatrix * vec4( position, 1.0 );
			vec3 worldNormal = normalize ( mat3( modelMatrix[0].xyz, modelMatrix[1].xyz, modelMatrix[2].xyz ) * normal );

			vNormal = normalize( normalMatrix * normal );

			vec3 I = worldPosition.xyz - cameraPosition;
			vRefract = refract( normalize( I ), worldNormal, 1.02 );

			gl_Position = projectionMatrix * mvPosition;

		})jsm1";
constexpr std::string_view ToonShader_ToonShader1_fragmentShader = R"jsm2(

		uniform vec3 uBaseColor;

		uniform vec3 uDirLightPos;
		uniform vec3 uDirLightColor;

		uniform vec3 uAmbientLightColor;

		varying vec3 vNormal;

		varying vec3 vRefract;

		void main() {

			float directionalLightWeighting = max( dot( normalize( vNormal ), uDirLightPos ), 0.0);
			vec3 lightWeighting = uAmbientLightColor + uDirLightColor * directionalLightWeighting;

			float intensity = smoothstep( - 0.5, 1.0, pow( length(lightWeighting), 20.0 ) );
			intensity += length(lightWeighting) * 0.2;

			float cameraWeighting = dot( normalize( vNormal ), vRefract );
			intensity += pow( 1.0 - length( cameraWeighting ), 6.0 );
			intensity = intensity * 0.2 + 0.3;

			if ( intensity < 0.50 ) {

				gl_FragColor = vec4( 2.0 * intensity * uBaseColor, 1.0 );

			} else {

				gl_FragColor = vec4( 1.0 - 2.0 * ( 1.0 - intensity ) * ( 1.0 - uBaseColor ), 1.0 );

			}

			#include <colorspace_fragment>

		})jsm2";
constexpr std::string_view ToonShader_ToonShader2_vertexShader = R"jsm3(

		varying vec3 vNormal;

		void main() {

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
			vNormal = normalize( normalMatrix * normal );

		})jsm3";
constexpr std::string_view ToonShader_ToonShader2_fragmentShader = R"jsm4(

		uniform vec3 uBaseColor;
		uniform vec3 uLineColor1;
		uniform vec3 uLineColor2;
		uniform vec3 uLineColor3;
		uniform vec3 uLineColor4;

		uniform vec3 uDirLightPos;
		uniform vec3 uDirLightColor;

		uniform vec3 uAmbientLightColor;

		varying vec3 vNormal;

		void main() {

			float camera = max( dot( normalize( vNormal ), vec3( 0.0, 0.0, 1.0 ) ), 0.4);
			float light = max( dot( normalize( vNormal ), uDirLightPos ), 0.0);

			gl_FragColor = vec4( uBaseColor, 1.0 );

			if ( length(uAmbientLightColor + uDirLightColor * light) < 1.00 ) {

				gl_FragColor *= vec4( uLineColor1, 1.0 );

			}

			if ( length(uAmbientLightColor + uDirLightColor * camera) < 0.50 ) {

				gl_FragColor *= vec4( uLineColor2, 1.0 );

			}

			#include <colorspace_fragment>

		})jsm4";
constexpr std::string_view ToonShader_ToonShaderDotted_vertexShader = R"jsm5(

		varying vec3 vNormal;

		void main() {

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
			vNormal = normalize( normalMatrix * normal );

		})jsm5";
constexpr std::string_view ToonShader_ToonShaderDotted_fragmentShader = R"jsm6(

		uniform vec3 uBaseColor;
		uniform vec3 uLineColor1;
		uniform vec3 uLineColor2;
		uniform vec3 uLineColor3;
		uniform vec3 uLineColor4;

		uniform vec3 uDirLightPos;
		uniform vec3 uDirLightColor;

		uniform vec3 uAmbientLightColor;

		varying vec3 vNormal;

		void main() {

			float directionalLightWeighting = max( dot( normalize(vNormal), uDirLightPos ), 0.0);
			vec3 lightWeighting = uAmbientLightColor + uDirLightColor * directionalLightWeighting;

			gl_FragColor = vec4( uBaseColor, 1.0 );

			if ( length(lightWeighting) < 1.00 ) {

				if ( ( mod(gl_FragCoord.x, 4.001) + mod(gl_FragCoord.y, 4.0) ) > 6.00 ) {

					gl_FragColor = vec4( uLineColor1, 1.0 );

				}

			}

			if ( length(lightWeighting) < 0.50 ) {

				if ( ( mod(gl_FragCoord.x + 2.0, 4.001) + mod(gl_FragCoord.y + 2.0, 4.0) ) > 6.00 ) {

					gl_FragColor = vec4( uLineColor1, 1.0 );

				}

			}

			#include <colorspace_fragment>

		})jsm6";
constexpr std::string_view ToonShader_ToonShaderHatching_vertexShader = R"jsm7(

		varying vec3 vNormal;

		void main() {

			gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
			vNormal = normalize( normalMatrix * normal );

		})jsm7";
constexpr std::string_view ToonShader_ToonShaderHatching_fragmentShader = R"jsm8(

		uniform vec3 uBaseColor;
		uniform vec3 uLineColor1;
		uniform vec3 uLineColor2;
		uniform vec3 uLineColor3;
		uniform vec3 uLineColor4;

		uniform vec3 uDirLightPos;
		uniform vec3 uDirLightColor;

		uniform vec3 uAmbientLightColor;

		varying vec3 vNormal;

		void main() {

			float directionalLightWeighting = max( dot( normalize(vNormal), uDirLightPos ), 0.0);
			vec3 lightWeighting = uAmbientLightColor + uDirLightColor * directionalLightWeighting;

			gl_FragColor = vec4( uBaseColor, 1.0 );

			if ( length(lightWeighting) < 1.00 ) {

				if ( mod(gl_FragCoord.x + gl_FragCoord.y, 10.0) == 0.0) {

					gl_FragColor = vec4( uLineColor1, 1.0 );

				}

			}

			if ( length(lightWeighting) < 0.75 ) {

				if (mod(gl_FragCoord.x - gl_FragCoord.y, 10.0) == 0.0) {

					gl_FragColor = vec4( uLineColor2, 1.0 );

				}

			}

			if ( length(lightWeighting) < 0.50 ) {

				if (mod(gl_FragCoord.x + gl_FragCoord.y - 5.0, 10.0) == 0.0) {

					gl_FragColor = vec4( uLineColor3, 1.0 );

				}

			}

			if ( length(lightWeighting) < 0.3465 ) {

				if (mod(gl_FragCoord.x - gl_FragCoord.y - 5.0, 10.0) == 0.0) {

					gl_FragColor = vec4( uLineColor4, 1.0 );

			}

			}

			#include <colorspace_fragment>

		})jsm8";
} // namespace

JSValue nativeToonShader() {
    static thread_local JSWeakValue cache;
    if (const auto existing = cache.lock(); existing && existing->isObject()) return *existing;
    static thread_local bool initializing = false;
    JSModuleInitializationScope initialization(initializing, "ToonShader");
    JSValue result = object({
        {"ToonShader1", object({
            {"name", "ToonShader1"},
            {"uniforms", object({
                {"uDirLightPos", object({
                    {"value", nativeVector(std::make_shared<Vector3>(0, 0, 0))}
                })},
                {"uDirLightColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.85499260812105, 0.85499260812105, 0.85499260812105))}
                })},
                {"uAmbientLightColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.0015176349176470587, 0.0015176349176470587, 0.0015176349176470587))}
                })},
                {"uBaseColor", object({
                    {"value", nativeColor(std::make_shared<Color>(1, 1, 1))}
                })}
            })},
            {"vertexShader", std::string(ToonShader_ToonShader1_vertexShader)},
            {"fragmentShader", std::string(ToonShader_ToonShader1_fragmentShader)}
        })},
        {"ToonShader2", object({
            {"uniforms", object({
                {"uDirLightPos", object({
                    {"value", nativeVector(std::make_shared<Vector3>(0, 0, 0))}
                })},
                {"uDirLightColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.85499260812105, 0.85499260812105, 0.85499260812105))}
                })},
                {"uAmbientLightColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.0015176349176470587, 0.0015176349176470587, 0.0015176349176470587))}
                })},
                {"uBaseColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.85499260812105, 0.85499260812105, 0.85499260812105))}
                })},
                {"uLineColor1", object({
                    {"value", nativeColor(std::make_shared<Color>(0.21586050010324417, 0.21586050010324417, 0.21586050010324417))}
                })},
                {"uLineColor2", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })},
                {"uLineColor3", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })},
                {"uLineColor4", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })}
            })},
            {"vertexShader", std::string(ToonShader_ToonShader2_vertexShader)},
            {"fragmentShader", std::string(ToonShader_ToonShader2_fragmentShader)}
        })},
        {"ToonShaderDotted", object({
            {"uniforms", object({
                {"uDirLightPos", object({
                    {"value", nativeVector(std::make_shared<Vector3>(0, 0, 0))}
                })},
                {"uDirLightColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.85499260812105, 0.85499260812105, 0.85499260812105))}
                })},
                {"uAmbientLightColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.0015176349176470587, 0.0015176349176470587, 0.0015176349176470587))}
                })},
                {"uBaseColor", object({
                    {"value", nativeColor(std::make_shared<Color>(1, 1, 1))}
                })},
                {"uLineColor1", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })}
            })},
            {"vertexShader", std::string(ToonShader_ToonShaderDotted_vertexShader)},
            {"fragmentShader", std::string(ToonShader_ToonShaderDotted_fragmentShader)}
        })},
        {"ToonShaderHatching", object({
            {"uniforms", object({
                {"uDirLightPos", object({
                    {"value", nativeVector(std::make_shared<Vector3>(0, 0, 0))}
                })},
                {"uDirLightColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.85499260812105, 0.85499260812105, 0.85499260812105))}
                })},
                {"uAmbientLightColor", object({
                    {"value", nativeColor(std::make_shared<Color>(0.0015176349176470587, 0.0015176349176470587, 0.0015176349176470587))}
                })},
                {"uBaseColor", object({
                    {"value", nativeColor(std::make_shared<Color>(1, 1, 1))}
                })},
                {"uLineColor1", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })},
                {"uLineColor2", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })},
                {"uLineColor3", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })},
                {"uLineColor4", object({
                    {"value", nativeColor(std::make_shared<Color>(0, 0, 0))}
                })}
            })},
            {"vertexShader", std::string(ToonShader_ToonShaderHatching_vertexShader)},
            {"fragmentShader", std::string(ToonShader_ToonShaderHatching_fragmentShader)}
        })}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
} // namespace ToonShader
