#include "renderers/vulkan/VulkanUniformBuffer.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>

namespace {
std::size_t add(std::size_t a, std::size_t b) {
    if(b>std::numeric_limits<std::size_t>::max()-a)throw std::invalid_argument("Block offset overflow");
    return a+b;
}
std::size_t multiply(std::size_t a, std::size_t b) {
    if(a && b>std::numeric_limits<std::size_t>::max()/a)throw std::invalid_argument("Block size overflow");
    return a*b;
}
using Block=VulkanShaderInterface::Block;
using Scalar=VulkanUniformBuffer::Scalar;
}
VulkanUniformBuffer::VulkanUniformBuffer(const Block& block,std::size_t byteSize,
                                       const Dimensions& dimensions)
    :storage_(byteSize?byteSize:std::max(block.size,block.paddedSize)) {
    if(block.members.empty())throw std::invalid_argument("A block must contain reflected members");
    for(const auto& child:block.members)expand(child,child.name,child.offset,dimensions);
}
void VulkanUniformBuffer::expand(const Block& node,const std::string& name,std::size_t base,
                                const Dimensions& dimensions) {
    auto dims=node.array.dimensions;
    bool resolvedDimensions=false;
    if(const auto override=dimensions.find(name);override!=dimensions.end()) {
        resolvedDimensions=true;
        if(override->second.size()!=dims.size())throw std::invalid_argument("Array dimension count mismatch");
        for(std::size_t i=0;i<dims.size();++i) {
            const bool specialized=i<node.array.specializationIds.size() && node.array.specializationIds[i]!=~0u;
            if(dims[i] && !specialized && dims[i]!=override->second[i])
                throw std::invalid_argument("Cannot override a fixed array dimension");
        }
        dims=override->second;
    }
    auto strides=node.array.strides;
    if(strides.empty() && dims.size()==1)strides.push_back(node.array.stride);
    if(strides.size()!=dims.size())throw std::invalid_argument("Missing per-dimension array strides");
    const bool emptyArray=std::find(dims.begin(),dims.end(),0u)!=dims.end();
    std::size_t end=base;
    for(std::size_t i=0;i<dims.size();++i) {
        if((!dims[i] && !resolvedDimensions) || !strides[i])throw std::invalid_argument("Unresolved array dimension or stride");
        if(emptyArray)continue;
        end=add(end,multiply(dims[i]-1,strides[i]));
    }
    if(end>storage_.size())throw std::invalid_argument("Array exceeds block storage");
    const auto& numeric=node.numeric;
    const auto width=numeric.scalar==Scalar::Boolean && !numeric.width?32:numeric.width;
    const auto scalarSize=width/8;
    const auto columns=numeric.columns,rows=numeric.rows;
    const bool matrix=columns || rows;
    const auto components=matrix?multiply(columns,rows):std::max(1u,numeric.components);
    if(node.members.empty() && (numeric.scalar==Scalar::Unknown || !width || width%8 ||
                               (matrix && (!columns || !rows || !numeric.matrixStride))))
        throw std::invalid_argument("Unsupported reflected scalar or matrix layout");
    auto leaf=[&](const std::string& path,std::size_t at) -> std::vector<std::size_t> {
        if(!node.members.empty()) {
            for(const auto& member:node.members)
                expand(member,path+"."+member.name,add(at,member.offset),dimensions);
            return {};
        }
        std::vector<std::size_t> offsets;
        offsets.reserve(components);
        for(std::size_t i=0;i<components;++i) {
            const auto offset=matrix?
                add(at,add(multiply(numeric.rowMajor?i%rows:i/rows,numeric.matrixStride),
                           multiply(numeric.rowMajor?i/rows:i%rows,scalarSize))):
                add(at,multiply(i,scalarSize));
            if(add(offset,scalarSize)>storage_.size())throw std::invalid_argument("Member exceeds block storage");
            offsets.push_back(offset);
        }
        if(!plans_.emplace(path,Plan{numeric.scalar,width,components,
                                   std::make_shared<const std::vector<std::size_t>>(offsets)}).second)
            throw std::invalid_argument("Duplicate reflected member name");
        return offsets;
    };
    std::function<std::vector<std::size_t>(std::size_t,const std::string&,std::size_t)> array;
    array=[&](std::size_t dimension,const std::string& path,std::size_t at) {
        if(dimension==dims.size())return leaf(path,at);
        std::vector<std::size_t> offsets;
        std::vector<std::size_t> starts;
        for(std::size_t i=0;i<dims[dimension];++i) {
            starts.push_back(offsets.size());
            auto child=array(dimension+1,path+"["+std::to_string(i)+"]",add(at,multiply(i,strides[dimension])));
            offsets.insert(offsets.end(),child.begin(),child.end());
        }
        if(node.members.empty()) {
            const auto shared=std::make_shared<const std::vector<std::size_t>>(offsets);
            plans_.insert_or_assign(path,Plan{numeric.scalar,width,components,shared});
            for(std::size_t i=0;i<dims[dimension];++i) {
                auto first=path+"["+std::to_string(i)+"]";
                for(std::size_t j=dimension+1;j<dims.size();++j)first+="[0]";
                plans_.insert_or_assign(first,Plan{numeric.scalar,width,components,shared,starts[i]});
            }
        }
        return offsets;
    };
    array(0,name,base);
}
void VulkanUniformBuffer::writeRaw(const std::string& name,Scalar scalar,std::uint32_t width,
                                  std::span<const std::byte> packed) {
    const auto found=plans_.find(name);
    if(found==plans_.end())throw std::invalid_argument("Unknown block member: "+name);
    const auto& plan=found->second;
    if(scalar!=plan.scalar || width!=plan.width)throw std::invalid_argument("Uniform scalar type mismatch");
    const auto size=width/8;
    if(packed.size()%multiply(size,plan.components) || packed.size()/size>plan.offsets->size()-plan.first)
        throw std::invalid_argument("Uniform component count mismatch");
    // Captured input can alias an earlier region of this block; all reads precede writes.
    const std::vector<std::byte> snapshot(packed.begin(),packed.end());
    for(std::size_t i=0;i<snapshot.size()/size;++i)
        std::memcpy(storage_.data()+(*plan.offsets)[plan.first+i],snapshot.data()+i*size,size);
}
void VulkanUniformBuffer::write(const std::string& n,std::span<const float> v) {
    writeRaw(n,Scalar::FloatingPoint,32,std::as_bytes(v));
}
void VulkanUniformBuffer::write(const std::string& n,std::span<const double> v) {
    writeRaw(n,Scalar::FloatingPoint,64,std::as_bytes(v));
}
void VulkanUniformBuffer::write(const std::string& n,std::span<const std::int32_t> v) {
    writeRaw(n,Scalar::SignedInteger,32,std::as_bytes(v));
}
void VulkanUniformBuffer::write(const std::string& n,std::span<const std::uint32_t> v) {
    writeRaw(n,Scalar::UnsignedInteger,32,std::as_bytes(v));
}
void VulkanUniformBuffer::writeBooleans(const std::string& n,std::span<const std::int32_t> v) {
    const auto found=plans_.find(n);
    if(found==plans_.end())throw std::invalid_argument("Unknown block member: "+n);
    const auto& plan=found->second;
    if(plan.width!=32 || (plan.scalar!=Scalar::Boolean && plan.scalar!=Scalar::SignedInteger &&
                         plan.scalar!=Scalar::UnsignedInteger))
        throw std::invalid_argument("Boolean storage type mismatch");
    std::vector<std::uint32_t> values;values.reserve(v.size());
    for(const auto value:v)values.push_back(value?1u:0u);
    writeRaw(n,plan.scalar,32,std::as_bytes(std::span(values)));
}
