// Generated from three.js src/renderers/shaders/ShaderChunk/lights_phong_fragment.glsl.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk/lights_phong_fragment.glsl.h"
namespace ShaderSource::Chunk::lights_phong_fragment {
std::string_view defaultSource() { return R"three_glsl_0(
BlinnPhongMaterial material;
material.diffuseColor = diffuseColor.rgb;
material.specularColor = specular;
material.specularShininess = shininess;
material.specularStrength = specularStrength;
)three_glsl_0"; }
}
