#include "catch.hpp"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include "renderers/vulkan/VulkanShaderModules.h"
#include <array>
#include <cstring>
#include <type_traits>

namespace {
template<class T> T handle(std::uintptr_t value) {
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}
struct State {
    int calls = 0, failAt = -1;
    std::vector<VkShaderModule> destroyed;
} state;
VKAPI_ATTR VkResult VKAPI_CALL create(VkDevice, const VkShaderModuleCreateInfo* info,
                                    const VkAllocationCallbacks*, VkShaderModule* result) {
    REQUIRE(info->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    REQUIRE(info->codeSize >= 20);
    REQUIRE(info->codeSize % 4 == 0);
    REQUIRE(info->pCode[0] == 0x07230203);
    const auto n = state.calls++;
    if (n == state.failAt) {
        *result = handle<VkShaderModule>(0xdead);
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }
    *result = handle<VkShaderModule>(n + 1);
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL destroy(VkDevice, VkShaderModule module, const VkAllocationCallbacks*) {
    state.destroyed.push_back(module);
}
const auto device = handle<VkDevice>(0x10);
const VulkanShaderModules::Dispatch dispatch{create, destroy};
VulkanShaderCompiler::Result compiled() {
    VulkanShaderCompiler compiler(THREECPP_GLSLANG_EXECUTABLE);
    auto value = compiler.compile(
        "#version 450\nlayout(location=0)in vec3 position;void main(){gl_Position=vec4(position,1);}",
        "#version 450\nlayout(location=0)out vec4 color;void main(){color=vec4(1);}");
    INFO(value.diagnostics);
    REQUIRE(value.linked);
    return value;
}
}
TEST_CASE("Shader module creation owns only successful native handles", "[vulkan][shader-modules]") {
    const auto source = compiled();
    const std::array<VulkanShaderModules::Stage, 2> stages{{{source.vertex}, {source.fragment}}};
    for (int failure = 0; failure < 2; ++failure) {
        state = {}; state.failAt = failure;
        REQUIRE_THROWS_AS(VulkanShaderModules(device, dispatch, stages), std::runtime_error);
        REQUIRE(state.calls == failure + 1);
        REQUIRE(state.destroyed.size() == static_cast<std::size_t>(failure));
        if (failure) REQUIRE(state.destroyed[0] == handle<VkShaderModule>(1));
    }
    state = {};
    {
        VulkanShaderModules modules(device, dispatch, stages);
        REQUIRE(modules.handles().size() == 2);
        REQUIRE(modules.interfaces()[0].stage == VK_SHADER_STAGE_VERTEX_BIT);
        REQUIRE(modules.interfaces()[1].stage == VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    REQUIRE(state.destroyed == std::vector<VkShaderModule>{handle<VkShaderModule>(2), handle<VkShaderModule>(1)});
}
TEST_CASE("Shader modules reflect all stages before allocating native resources", "[vulkan][shader-modules]") {
    const auto source = compiled();
    const std::array<VulkanShaderModules::Stage, 2> missing{{{source.vertex}, {source.fragment, "missing"}}};
    state = {};
    REQUIRE_THROWS(VulkanShaderModules(device, dispatch, missing));
    REQUIRE(state.calls == 0);
    const std::array<VulkanShaderModules::Stage, 1> truncated{{{{source.vertex.data(), 4}}}};
    REQUIRE_THROWS(VulkanShaderModules(device, dispatch, truncated));
    REQUIRE_THROWS_AS(VulkanShaderModules(VK_NULL_HANDLE, dispatch, missing), std::invalid_argument);
    REQUIRE_THROWS_AS(VulkanShaderModules(device, {}, missing), std::invalid_argument);
    REQUIRE_THROWS_AS(VulkanShaderModules(device, dispatch, {}), std::invalid_argument);
    REQUIRE(state.calls == 0);
}
TEST_CASE("Moving shader modules rebuilds entry pointers and releases previous resources", "[vulkan][shader-modules]") {
    auto source = compiled();
    const std::array<VulkanShaderModules::Stage, 2> stages{{{source.vertex}, {source.fragment}}};
    state = {};
    VulkanShaderModules first(device, dispatch, stages);
    source.vertex.clear(); source.fragment.clear();
    VulkanShaderModules second(std::move(first));
    REQUIRE(first.handles().empty());
    REQUIRE(second.handles().size() == 2);
    auto pipeline = second.pipelineStages();
    REQUIRE(std::strcmp(pipeline[0].pName, "main") == 0);
    REQUIRE(pipeline[0].module == handle<VkShaderModule>(1));
    REQUIRE(pipeline[1].stage == VK_SHADER_STAGE_FRAGMENT_BIT);
    second = std::move(second);
    REQUIRE(second.handles().size() == 2);
    auto replacement = compiled();
    const std::array<VulkanShaderModules::Stage, 1> another{{{replacement.vertex}}};
    VulkanShaderModules third(device, dispatch, another);
    third = std::move(second);
    REQUIRE(state.destroyed == std::vector<VkShaderModule>{handle<VkShaderModule>(3)});
    REQUIRE(second.pipelineStages().empty());
    REQUIRE(std::strcmp(third.pipelineStages()[0].pName, "main") == 0);
    third.reset(); third.reset();
    REQUIRE(state.destroyed == std::vector<VkShaderModule>{handle<VkShaderModule>(3), handle<VkShaderModule>(2), handle<VkShaderModule>(1)});
}
