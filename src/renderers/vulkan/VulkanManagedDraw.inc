// Included inside VulkanRenderer::Impl. This is the raw managed resource/draw
// boundary: source traversal has already called the real WebGLObjects.update.
// No scene, geometry, material, or attribute is reconstructed as a typed facade.
struct ManagedIndex {
    JSWeakValue buffer;
    std::uint64_t revision = UINT64_MAX;
    double type = 0;
    std::vector<std::uint32_t> values;
    BufferRef resource;
};
std::vector<ManagedIndex> managedIndices;

struct ManagedCall {
    JSValue receiver, function;
    JSValue operator()(const std::vector<JSValue>& args = {}) const {
        return function.call(receiver, args);
    }
};
static ManagedCall managedCall(JSValue receiver, const char* name) {
    return {receiver, receiver.get(name)};
}
static void managedOwn(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor descriptor; descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(name, descriptor);
}
ManagedIndex& managedIndex(const JSValue& buffer, double type) {
    std::erase_if(managedIndices, [&](const auto& entry) {
        const auto value = entry.buffer.lock(); return !value || !attributeCommands->hasStorage(*value);
    });
    ManagedIndex* entry = nullptr;
    for (auto& candidate : managedIndices) {
        const auto key = candidate.buffer.lock();
        if (key && key->sameValue(buffer) && candidate.type == type) { entry = &candidate; break; }
    }
    if (!entry) { managedIndices.emplace_back(); entry = &managedIndices.back(); entry->buffer = JSWeakValue(buffer); entry->type = type; }
    const auto revision = attributeCommands->revision(buffer);
    if (entry->resource && entry->revision == revision) return *entry;
    const auto values = uploadedArray(attributeCommands->read(buffer), type);
    entry->values = std::visit([](const auto& array) -> std::vector<std::uint32_t> {
        using T = typename std::decay_t<decltype(array)>::value_type;
        if constexpr (std::is_same_v<T, std::uint8_t> || std::is_same_v<T, std::uint16_t> || std::is_same_v<T, std::uint32_t>)
            return {array.begin(), array.end()};
        else throw std::invalid_argument("Managed Vulkan index requires unsigned byte, short, or int storage");
    }, values);
    if (entry->resource)
        updateBuffer(entry->resource, entry->values, {{0, entry->values.size()}}, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    else entry->resource = sharedBuffer(std::max<std::size_t>(entry->values.size(), 1) * sizeof(std::uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, entry->values.empty() ? nullptr : entry->values.data(), true);
    entry->revision = revision; return *entry;
}
