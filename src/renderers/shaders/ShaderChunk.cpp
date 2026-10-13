// Generated from three.js src/renderers/shaders/ShaderChunk.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.
#include "renderers/shaders/ShaderChunk.h"
#include "renderers/shaders/ShaderChunk/alphahash_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/alphahash_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/alphamap_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/alphamap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/alphatest_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/alphatest_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/aomap_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/aomap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/batching_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/batching_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/begin_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/beginnormal_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/bsdfs.glsl.h"
#include "renderers/shaders/ShaderChunk/iridescence_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/bumpmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/clipping_planes_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/clipping_planes_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/clipping_planes_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/clipping_planes_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/color_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/color_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/color_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/color_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/common.glsl.h"
#include "renderers/shaders/ShaderChunk/defaultnormal_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/displacementmap_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/displacementmap_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/emissivemap_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/emissivemap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/colorspace_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/colorspace_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/envmap_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/envmap_common_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/envmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/envmap_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/envmap_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/fog_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/fog_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/fog_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/fog_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/gradientmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lightmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_lambert_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_lambert_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_pars_begin.glsl.h"
#include "renderers/shaders/ShaderChunk/envmap_physical_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_toon_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_toon_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_phong_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_phong_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_physical_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_physical_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_fragment_begin.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_fragment_maps.glsl.h"
#include "renderers/shaders/ShaderChunk/lights_fragment_end.glsl.h"
#include "renderers/shaders/ShaderChunk/lightprobes_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/logdepthbuf_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/logdepthbuf_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/logdepthbuf_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/logdepthbuf_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/map_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/map_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/map_particle_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/map_particle_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/metalnessmap_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/metalnessmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/morphinstance_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/morphcolor_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/morphnormal_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/morphtarget_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/morphtarget_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/normal_fragment_begin.glsl.h"
#include "renderers/shaders/ShaderChunk/normal_fragment_maps.glsl.h"
#include "renderers/shaders/ShaderChunk/normal_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/normal_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/normal_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/normalmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/clearcoat_normal_fragment_begin.glsl.h"
#include "renderers/shaders/ShaderChunk/clearcoat_normal_fragment_maps.glsl.h"
#include "renderers/shaders/ShaderChunk/clearcoat_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/iridescence_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/opaque_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/packing.glsl.h"
#include "renderers/shaders/ShaderChunk/premultiplied_alpha_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/project_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/dithering_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/dithering_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/roughnessmap_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/roughnessmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/shadowmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/shadowmap_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/shadowmap_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/shadowmask_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/skinbase_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/skinning_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/skinning_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/skinnormal_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/specularmap_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/specularmap_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/tonemapping_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/transmission_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/transmission_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/uv_pars_fragment.glsl.h"
#include "renderers/shaders/ShaderChunk/uv_pars_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/uv_vertex.glsl.h"
#include "renderers/shaders/ShaderChunk/worldpos_vertex.glsl.h"
#include "renderers/shaders/ShaderLib/background.glsl.h"
#include "renderers/shaders/ShaderLib/backgroundCube.glsl.h"
#include "renderers/shaders/ShaderLib/cube.glsl.h"
#include "renderers/shaders/ShaderLib/depth.glsl.h"
#include "renderers/shaders/ShaderLib/distance.glsl.h"
#include "renderers/shaders/ShaderLib/equirect.glsl.h"
#include "renderers/shaders/ShaderLib/linedashed.glsl.h"
#include "renderers/shaders/ShaderLib/meshbasic.glsl.h"
#include "renderers/shaders/ShaderLib/meshlambert.glsl.h"
#include "renderers/shaders/ShaderLib/meshmatcap.glsl.h"
#include "renderers/shaders/ShaderLib/meshnormal.glsl.h"
#include "renderers/shaders/ShaderLib/meshphong.glsl.h"
#include "renderers/shaders/ShaderLib/meshphysical.glsl.h"
#include "renderers/shaders/ShaderLib/meshtoon.glsl.h"
#include "renderers/shaders/ShaderLib/points.glsl.h"
#include "renderers/shaders/ShaderLib/shadow.glsl.h"
#include "renderers/shaders/ShaderLib/sprite.glsl.h"
#include <initializer_list>
#include <utility>
namespace {
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields) {
    auto result=JSValue::object();
    for(const auto& [key,value]:fields){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(key,d);}
    return result;
}
}
JSValue nativeShaderChunk() {
    static thread_local JSWeakValue cache;
    if(const auto existing=cache.lock();existing&&existing->isObject())return *existing;
    static thread_local bool initializing=false;
    JSModuleInitializationScope initialization(initializing,"ShaderChunk");
    auto result=object({
        {"alphahash_fragment",std::string(ShaderSource::Chunk::alphahash_fragment::defaultSource())},
        {"alphahash_pars_fragment",std::string(ShaderSource::Chunk::alphahash_pars_fragment::defaultSource())},
        {"alphamap_fragment",std::string(ShaderSource::Chunk::alphamap_fragment::defaultSource())},
        {"alphamap_pars_fragment",std::string(ShaderSource::Chunk::alphamap_pars_fragment::defaultSource())},
        {"alphatest_fragment",std::string(ShaderSource::Chunk::alphatest_fragment::defaultSource())},
        {"alphatest_pars_fragment",std::string(ShaderSource::Chunk::alphatest_pars_fragment::defaultSource())},
        {"aomap_fragment",std::string(ShaderSource::Chunk::aomap_fragment::defaultSource())},
        {"aomap_pars_fragment",std::string(ShaderSource::Chunk::aomap_pars_fragment::defaultSource())},
        {"batching_pars_vertex",std::string(ShaderSource::Chunk::batching_pars_vertex::defaultSource())},
        {"batching_vertex",std::string(ShaderSource::Chunk::batching_vertex::defaultSource())},
        {"begin_vertex",std::string(ShaderSource::Chunk::begin_vertex::defaultSource())},
        {"beginnormal_vertex",std::string(ShaderSource::Chunk::beginnormal_vertex::defaultSource())},
        {"bsdfs",std::string(ShaderSource::Chunk::bsdfs::defaultSource())},
        {"iridescence_fragment",std::string(ShaderSource::Chunk::iridescence_fragment::defaultSource())},
        {"bumpmap_pars_fragment",std::string(ShaderSource::Chunk::bumpmap_pars_fragment::defaultSource())},
        {"clipping_planes_fragment",std::string(ShaderSource::Chunk::clipping_planes_fragment::defaultSource())},
        {"clipping_planes_pars_fragment",std::string(ShaderSource::Chunk::clipping_planes_pars_fragment::defaultSource())},
        {"clipping_planes_pars_vertex",std::string(ShaderSource::Chunk::clipping_planes_pars_vertex::defaultSource())},
        {"clipping_planes_vertex",std::string(ShaderSource::Chunk::clipping_planes_vertex::defaultSource())},
        {"color_fragment",std::string(ShaderSource::Chunk::color_fragment::defaultSource())},
        {"color_pars_fragment",std::string(ShaderSource::Chunk::color_pars_fragment::defaultSource())},
        {"color_pars_vertex",std::string(ShaderSource::Chunk::color_pars_vertex::defaultSource())},
        {"color_vertex",std::string(ShaderSource::Chunk::color_vertex::defaultSource())},
        {"common",std::string(ShaderSource::Chunk::common::defaultSource())},
        {"defaultnormal_vertex",std::string(ShaderSource::Chunk::defaultnormal_vertex::defaultSource())},
        {"displacementmap_pars_vertex",std::string(ShaderSource::Chunk::displacementmap_pars_vertex::defaultSource())},
        {"displacementmap_vertex",std::string(ShaderSource::Chunk::displacementmap_vertex::defaultSource())},
        {"emissivemap_fragment",std::string(ShaderSource::Chunk::emissivemap_fragment::defaultSource())},
        {"emissivemap_pars_fragment",std::string(ShaderSource::Chunk::emissivemap_pars_fragment::defaultSource())},
        {"colorspace_fragment",std::string(ShaderSource::Chunk::colorspace_fragment::defaultSource())},
        {"colorspace_pars_fragment",std::string(ShaderSource::Chunk::colorspace_pars_fragment::defaultSource())},
        {"envmap_fragment",std::string(ShaderSource::Chunk::envmap_fragment::defaultSource())},
        {"envmap_common_pars_fragment",std::string(ShaderSource::Chunk::envmap_common_pars_fragment::defaultSource())},
        {"envmap_pars_fragment",std::string(ShaderSource::Chunk::envmap_pars_fragment::defaultSource())},
        {"envmap_pars_vertex",std::string(ShaderSource::Chunk::envmap_pars_vertex::defaultSource())},
        {"envmap_physical_pars_fragment",std::string(ShaderSource::Chunk::envmap_physical_pars_fragment::defaultSource())},
        {"envmap_vertex",std::string(ShaderSource::Chunk::envmap_vertex::defaultSource())},
        {"fog_vertex",std::string(ShaderSource::Chunk::fog_vertex::defaultSource())},
        {"fog_pars_vertex",std::string(ShaderSource::Chunk::fog_pars_vertex::defaultSource())},
        {"fog_fragment",std::string(ShaderSource::Chunk::fog_fragment::defaultSource())},
        {"fog_pars_fragment",std::string(ShaderSource::Chunk::fog_pars_fragment::defaultSource())},
        {"gradientmap_pars_fragment",std::string(ShaderSource::Chunk::gradientmap_pars_fragment::defaultSource())},
        {"lightmap_pars_fragment",std::string(ShaderSource::Chunk::lightmap_pars_fragment::defaultSource())},
        {"lights_lambert_fragment",std::string(ShaderSource::Chunk::lights_lambert_fragment::defaultSource())},
        {"lights_lambert_pars_fragment",std::string(ShaderSource::Chunk::lights_lambert_pars_fragment::defaultSource())},
        {"lights_pars_begin",std::string(ShaderSource::Chunk::lights_pars_begin::defaultSource())},
        {"lights_toon_fragment",std::string(ShaderSource::Chunk::lights_toon_fragment::defaultSource())},
        {"lights_toon_pars_fragment",std::string(ShaderSource::Chunk::lights_toon_pars_fragment::defaultSource())},
        {"lights_phong_fragment",std::string(ShaderSource::Chunk::lights_phong_fragment::defaultSource())},
        {"lights_phong_pars_fragment",std::string(ShaderSource::Chunk::lights_phong_pars_fragment::defaultSource())},
        {"lights_physical_fragment",std::string(ShaderSource::Chunk::lights_physical_fragment::defaultSource())},
        {"lights_physical_pars_fragment",std::string(ShaderSource::Chunk::lights_physical_pars_fragment::defaultSource())},
        {"lights_fragment_begin",std::string(ShaderSource::Chunk::lights_fragment_begin::defaultSource())},
        {"lights_fragment_maps",std::string(ShaderSource::Chunk::lights_fragment_maps::defaultSource())},
        {"lights_fragment_end",std::string(ShaderSource::Chunk::lights_fragment_end::defaultSource())},
        {"lightprobes_pars_fragment",std::string(ShaderSource::Chunk::lightprobes_pars_fragment::defaultSource())},
        {"logdepthbuf_fragment",std::string(ShaderSource::Chunk::logdepthbuf_fragment::defaultSource())},
        {"logdepthbuf_pars_fragment",std::string(ShaderSource::Chunk::logdepthbuf_pars_fragment::defaultSource())},
        {"logdepthbuf_pars_vertex",std::string(ShaderSource::Chunk::logdepthbuf_pars_vertex::defaultSource())},
        {"logdepthbuf_vertex",std::string(ShaderSource::Chunk::logdepthbuf_vertex::defaultSource())},
        {"map_fragment",std::string(ShaderSource::Chunk::map_fragment::defaultSource())},
        {"map_pars_fragment",std::string(ShaderSource::Chunk::map_pars_fragment::defaultSource())},
        {"map_particle_fragment",std::string(ShaderSource::Chunk::map_particle_fragment::defaultSource())},
        {"map_particle_pars_fragment",std::string(ShaderSource::Chunk::map_particle_pars_fragment::defaultSource())},
        {"metalnessmap_fragment",std::string(ShaderSource::Chunk::metalnessmap_fragment::defaultSource())},
        {"metalnessmap_pars_fragment",std::string(ShaderSource::Chunk::metalnessmap_pars_fragment::defaultSource())},
        {"morphinstance_vertex",std::string(ShaderSource::Chunk::morphinstance_vertex::defaultSource())},
        {"morphcolor_vertex",std::string(ShaderSource::Chunk::morphcolor_vertex::defaultSource())},
        {"morphnormal_vertex",std::string(ShaderSource::Chunk::morphnormal_vertex::defaultSource())},
        {"morphtarget_pars_vertex",std::string(ShaderSource::Chunk::morphtarget_pars_vertex::defaultSource())},
        {"morphtarget_vertex",std::string(ShaderSource::Chunk::morphtarget_vertex::defaultSource())},
        {"normal_fragment_begin",std::string(ShaderSource::Chunk::normal_fragment_begin::defaultSource())},
        {"normal_fragment_maps",std::string(ShaderSource::Chunk::normal_fragment_maps::defaultSource())},
        {"normal_pars_fragment",std::string(ShaderSource::Chunk::normal_pars_fragment::defaultSource())},
        {"normal_pars_vertex",std::string(ShaderSource::Chunk::normal_pars_vertex::defaultSource())},
        {"normal_vertex",std::string(ShaderSource::Chunk::normal_vertex::defaultSource())},
        {"normalmap_pars_fragment",std::string(ShaderSource::Chunk::normalmap_pars_fragment::defaultSource())},
        {"clearcoat_normal_fragment_begin",std::string(ShaderSource::Chunk::clearcoat_normal_fragment_begin::defaultSource())},
        {"clearcoat_normal_fragment_maps",std::string(ShaderSource::Chunk::clearcoat_normal_fragment_maps::defaultSource())},
        {"clearcoat_pars_fragment",std::string(ShaderSource::Chunk::clearcoat_pars_fragment::defaultSource())},
        {"iridescence_pars_fragment",std::string(ShaderSource::Chunk::iridescence_pars_fragment::defaultSource())},
        {"opaque_fragment",std::string(ShaderSource::Chunk::opaque_fragment::defaultSource())},
        {"packing",std::string(ShaderSource::Chunk::packing::defaultSource())},
        {"premultiplied_alpha_fragment",std::string(ShaderSource::Chunk::premultiplied_alpha_fragment::defaultSource())},
        {"project_vertex",std::string(ShaderSource::Chunk::project_vertex::defaultSource())},
        {"dithering_fragment",std::string(ShaderSource::Chunk::dithering_fragment::defaultSource())},
        {"dithering_pars_fragment",std::string(ShaderSource::Chunk::dithering_pars_fragment::defaultSource())},
        {"roughnessmap_fragment",std::string(ShaderSource::Chunk::roughnessmap_fragment::defaultSource())},
        {"roughnessmap_pars_fragment",std::string(ShaderSource::Chunk::roughnessmap_pars_fragment::defaultSource())},
        {"shadowmap_pars_fragment",std::string(ShaderSource::Chunk::shadowmap_pars_fragment::defaultSource())},
        {"shadowmap_pars_vertex",std::string(ShaderSource::Chunk::shadowmap_pars_vertex::defaultSource())},
        {"shadowmap_vertex",std::string(ShaderSource::Chunk::shadowmap_vertex::defaultSource())},
        {"shadowmask_pars_fragment",std::string(ShaderSource::Chunk::shadowmask_pars_fragment::defaultSource())},
        {"skinbase_vertex",std::string(ShaderSource::Chunk::skinbase_vertex::defaultSource())},
        {"skinning_pars_vertex",std::string(ShaderSource::Chunk::skinning_pars_vertex::defaultSource())},
        {"skinning_vertex",std::string(ShaderSource::Chunk::skinning_vertex::defaultSource())},
        {"skinnormal_vertex",std::string(ShaderSource::Chunk::skinnormal_vertex::defaultSource())},
        {"specularmap_fragment",std::string(ShaderSource::Chunk::specularmap_fragment::defaultSource())},
        {"specularmap_pars_fragment",std::string(ShaderSource::Chunk::specularmap_pars_fragment::defaultSource())},
        {"tonemapping_fragment",std::string(ShaderSource::Chunk::tonemapping_fragment::defaultSource())},
        {"tonemapping_pars_fragment",std::string(ShaderSource::Chunk::tonemapping_pars_fragment::defaultSource())},
        {"transmission_fragment",std::string(ShaderSource::Chunk::transmission_fragment::defaultSource())},
        {"transmission_pars_fragment",std::string(ShaderSource::Chunk::transmission_pars_fragment::defaultSource())},
        {"uv_pars_fragment",std::string(ShaderSource::Chunk::uv_pars_fragment::defaultSource())},
        {"uv_pars_vertex",std::string(ShaderSource::Chunk::uv_pars_vertex::defaultSource())},
        {"uv_vertex",std::string(ShaderSource::Chunk::uv_vertex::defaultSource())},
        {"worldpos_vertex",std::string(ShaderSource::Chunk::worldpos_vertex::defaultSource())},
        {"background_vert",std::string(ShaderSource::Lib::background::vertex())},
        {"background_frag",std::string(ShaderSource::Lib::background::fragment())},
        {"backgroundCube_vert",std::string(ShaderSource::Lib::backgroundCube::vertex())},
        {"backgroundCube_frag",std::string(ShaderSource::Lib::backgroundCube::fragment())},
        {"cube_vert",std::string(ShaderSource::Lib::cube::vertex())},
        {"cube_frag",std::string(ShaderSource::Lib::cube::fragment())},
        {"depth_vert",std::string(ShaderSource::Lib::depth::vertex())},
        {"depth_frag",std::string(ShaderSource::Lib::depth::fragment())},
        {"distance_vert",std::string(ShaderSource::Lib::distance::vertex())},
        {"distance_frag",std::string(ShaderSource::Lib::distance::fragment())},
        {"equirect_vert",std::string(ShaderSource::Lib::equirect::vertex())},
        {"equirect_frag",std::string(ShaderSource::Lib::equirect::fragment())},
        {"linedashed_vert",std::string(ShaderSource::Lib::linedashed::vertex())},
        {"linedashed_frag",std::string(ShaderSource::Lib::linedashed::fragment())},
        {"meshbasic_vert",std::string(ShaderSource::Lib::meshbasic::vertex())},
        {"meshbasic_frag",std::string(ShaderSource::Lib::meshbasic::fragment())},
        {"meshlambert_vert",std::string(ShaderSource::Lib::meshlambert::vertex())},
        {"meshlambert_frag",std::string(ShaderSource::Lib::meshlambert::fragment())},
        {"meshmatcap_vert",std::string(ShaderSource::Lib::meshmatcap::vertex())},
        {"meshmatcap_frag",std::string(ShaderSource::Lib::meshmatcap::fragment())},
        {"meshnormal_vert",std::string(ShaderSource::Lib::meshnormal::vertex())},
        {"meshnormal_frag",std::string(ShaderSource::Lib::meshnormal::fragment())},
        {"meshphong_vert",std::string(ShaderSource::Lib::meshphong::vertex())},
        {"meshphong_frag",std::string(ShaderSource::Lib::meshphong::fragment())},
        {"meshphysical_vert",std::string(ShaderSource::Lib::meshphysical::vertex())},
        {"meshphysical_frag",std::string(ShaderSource::Lib::meshphysical::fragment())},
        {"meshtoon_vert",std::string(ShaderSource::Lib::meshtoon::vertex())},
        {"meshtoon_frag",std::string(ShaderSource::Lib::meshtoon::fragment())},
        {"points_vert",std::string(ShaderSource::Lib::points::vertex())},
        {"points_frag",std::string(ShaderSource::Lib::points::fragment())},
        {"shadow_vert",std::string(ShaderSource::Lib::shadow::vertex())},
        {"shadow_frag",std::string(ShaderSource::Lib::shadow::fragment())},
        {"sprite_vert",std::string(ShaderSource::Lib::sprite::vertex())},
        {"sprite_frag",std::string(ShaderSource::Lib::sprite::fragment())}
    });
    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;
}
