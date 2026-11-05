// Integer vertex fetch consumes the already-uploaded native buffer. Keep the
// 32-bit representation throughout; converting uint32 through float loses bits.
struct ManagedIntegerInput {
    JSWeakValue owner;
    VulkanBindingCommands::Attribute pointer;
    unsigned components=0;
    std::uint64_t revision=UINT64_MAX,rows=0;
    std::vector<std::uint32_t> values;
    BufferRef buffer;
};
std::vector<ManagedIntegerInput> managedIntegerInputs;

static bool managedUnsignedPointer(std::uint32_t type) {
    return type==0x1401||type==0x1403||type==0x1405;
}
static bool managedSignedPointer(std::uint32_t type) {
    return type==0x1400||type==0x1402||type==0x1404;
}
static VkFormat managedIntegerVertexFormat(unsigned components,bool signedValue) {
    static constexpr VkFormat signedFormats[]={VK_FORMAT_R32_SINT,VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32B32_SINT,VK_FORMAT_R32G32B32A32_SINT};
    static constexpr VkFormat unsignedFormats[]={VK_FORMAT_R32_UINT,VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32G32B32_UINT,VK_FORMAT_R32G32B32A32_UINT};
    if(!components||components>4)throw std::invalid_argument("Invalid native integer vertex width");
    return signedValue?signedFormats[components-1]:unsignedFormats[components-1];
}
ManagedIntegerInput& managedIntegerInput(const VulkanBindingCommands::Attribute& pointer,unsigned components) {
    std::erase_if(managedIntegerInputs,[&](const auto& entry){
        if(!entry.pointer.enabled)return false;
        const auto key=entry.owner.lock();return !key||!attributeCommands->hasStorage(*key);
    });
    ManagedIntegerInput* cached=nullptr;
    for(auto& entry:managedIntegerInputs) {
        if(entry.components!=components||!samePointer(entry.pointer,pointer))continue;
        const auto key=entry.owner.lock();
        if(!pointer.enabled||(key&&key->sameValue(pointer.buffer))){cached=&entry;break;}
    }
    if(!cached) {
        managedIntegerInputs.emplace_back();cached=&managedIntegerInputs.back();
        cached->pointer=pointer;cached->pointer.buffer=nullptr;cached->components=components;
        if(pointer.enabled)cached->owner=JSWeakValue(pointer.buffer);
    }
    const auto revision=pointer.enabled?attributeCommands->revision(pointer.buffer):0;
    if(cached->buffer&&cached->revision==revision)return *cached;
    std::shared_ptr<AttributeSnapshot> source;
    if(pointer.enabled) {
        auto actual=pointer;actual.normalized=false;
        source=std::make_shared<AttributeSnapshot>(uploadedArray(attributeCommands->read(pointer.buffer),pointer.type),actual,components);
        cached->rows=std::uint64_t(source->count);
    } else cached->rows=1;
    if(cached->rows>SIZE_MAX/components/sizeof(std::uint32_t))
        throw std::overflow_error("Native integer vertex data exceeds addressable memory");
    cached->values.resize(std::size_t(cached->rows)*components);
    for(std::uint64_t row=0;row<cached->rows;++row)for(unsigned component=0;component<components;++component) {
        std::uint32_t value;
        if(!pointer.enabled)value=pointer.constantType==VulkanBindingCommands::Attribute::ConstantType::SignedInteger?
            std::bit_cast<std::uint32_t>(pointer.signedValue[component]):pointer.unsignedValue[component];
        else {
            const auto number=source->getComponent(double(row),component);
            value=managedSignedPointer(pointer.type)?std::bit_cast<std::uint32_t>(std::int32_t(number)):std::uint32_t(number);
        }
        cached->values[std::size_t(row)*components+component]=value;
    }
    if(cached->buffer)updateBuffer(cached->buffer,cached->values,{{0,cached->values.size()}},VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    else cached->buffer=sharedBuffer(std::max<std::size_t>(cached->values.size(),1)*sizeof(std::uint32_t),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,cached->values.empty()?nullptr:cached->values.data(),true);
    cached->revision=revision;return *cached;
}
