#include "renderers/vulkan/VulkanShaderInterface.h"
#include "spirv_reflect.h"
#include <stdexcept>
#include <utility>
#include <map>
#include <limits>
#include <unordered_set>
#include <optional>
namespace {
using Interface = VulkanShaderInterface;
std::string text(const char* value) { return value ? value : ""; }
void checked(SpvReflectResult result) {
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        throw std::runtime_error("SPIR-V reflection failed: " + std::to_string(result));
}
struct Module {
    SpvReflectShaderModule value{};
    bool initialized=false;
    ~Module() { if(initialized)spvReflectDestroyShaderModule(&value); }
};

// The bundled SPIRV-Reflect does not resolve OpDecorationGroup. Expand legal
// grouped annotations in a private reflection copy; the caller's module and
// the exact original type/member decorations remain untouched.
std::vector<std::uint32_t> reflectionCopy(std::span<const std::uint32_t> words) {
    std::unordered_set<std::uint32_t> groupIds;
    for(std::size_t i=5;i<words.size();i+=words[i]>>16)
        if(SpvOp(words[i]&0xffff)==SpvOpDecorationGroup&&(words[i]>>16)>=2)groupIds.insert(words[i+1]);
    if(groupIds.empty())return {};
    std::map<std::uint32_t,std::vector<std::vector<std::uint32_t>>> decorations;
    for(std::size_t i=5;i<words.size();i+=words[i]>>16)
        if(SpvOp(words[i]&0xffff)==SpvOpDecorate&&(words[i]>>16)>=3&&groupIds.contains(words[i+1]))
            decorations[words[i+1]].emplace_back(words.begin()+i+2,words.begin()+i+(words[i]>>16));
    std::vector<std::uint32_t> result(words.begin(),words.begin()+5);
    for(std::size_t i=5;i<words.size();i+=words[i]>>16) {
        const auto count=words[i]>>16;const auto op=SpvOp(words[i]&0xffff);
        if(op==SpvOpDecorationGroup||(op==SpvOpDecorate&&count>=3&&groupIds.contains(words[i+1])))continue;
        if((op==SpvOpGroupDecorate&&count>=3)||(op==SpvOpGroupMemberDecorate&&count>=4)) {
            const bool member=op==SpvOpGroupMemberDecorate;
            const auto found=decorations.find(words[i+1]);
            if(!groupIds.contains(words[i+1]))throw std::invalid_argument("Missing SPIR-V decoration group");
            // OpDecorationGroup with no OpDecorate is a legal no-op.
            if(found==decorations.end())continue;
            for(std::size_t target=i+2;target+(member?1:0)<i+count;target+=member?2:1)
                for(const auto& decoration:found->second) {
                    const auto size=std::uint32_t(decoration.size())+(member?3:2);
                    result.push_back((size<<16)|std::uint32_t(member?SpvOpMemberDecorate:SpvOpDecorate));
                    result.push_back(words[target]);if(member)result.push_back(words[target+1]);
                    result.insert(result.end(),decoration.begin(),decoration.end());
                }
            continue;
        }
        result.insert(result.end(),words.begin()+i,words.begin()+i+count);
    }
    return result;
}

// SPIRV-Reflect exposes only the innermost array stride. Preserve the actual
// decoration on every OpTypeArray instead of inferring outer padding.
struct TypeIndex {
    struct MatrixLayout {std::optional<std::uint32_t> stride;std::optional<bool> rowMajor;};
    std::map<std::uint32_t, std::uint32_t> arrays, pointers, strides;
    std::map<std::pair<std::uint32_t,std::uint32_t>,MatrixLayout> matrices;
    static void decorateMatrix(MatrixLayout& layout,SpvDecoration decoration,std::optional<std::uint32_t> value) {
        if(decoration==SpvDecorationMatrixStride&&value)layout.stride=*value;
        if(decoration==SpvDecorationRowMajor)layout.rowMajor=true;
        if(decoration==SpvDecorationColMajor)layout.rowMajor=false;
    }
    explicit TypeIndex(std::span<const std::uint32_t> words) {
        std::vector<std::vector<std::uint32_t>> groups;
        std::vector<std::vector<std::uint32_t>> memberGroups;
        std::map<std::uint32_t,MatrixLayout> matrixGroups;
        for (std::size_t i=5;i<words.size();i+=words[i]>>16) {
            const auto count=words[i]>>16;
            switch (SpvOp(words[i]&0xffff)) {
            case SpvOpTypeArray:
                if(count>=4)arrays[words[i+1]]=words[i+2];break;
            case SpvOpTypeRuntimeArray:
                if(count>=3)arrays[words[i+1]]=words[i+2];break;
            case SpvOpTypePointer:
                if(count>=4)pointers[words[i+1]]=words[i+3];break;
            case SpvOpDecorate:
                if(count>=4 && words[i+2]==SpvDecorationArrayStride)
                    strides[words[i+1]]=words[i+3];
                if(count>=3)decorateMatrix(matrixGroups[words[i+1]],SpvDecoration(words[i+2]),count>=4?std::optional(words[i+3]):std::nullopt);
                break;
            case SpvOpMemberDecorate:
                if(count>=4)decorateMatrix(matrices[{words[i+1],words[i+2]}],SpvDecoration(words[i+3]),count>=5?std::optional(words[i+4]):std::nullopt);
                break;
            case SpvOpGroupDecorate:
                if(count>=3)groups.emplace_back(words.begin()+i+1,words.begin()+i+count);break;
            case SpvOpGroupMemberDecorate:
                if(count>=4)memberGroups.emplace_back(words.begin()+i+1,words.begin()+i+count);break;
            default: break;
            }
        }
        for(const auto& group:groups)
            if(const auto found=strides.find(group[0]);found!=strides.end())
                for(std::size_t i=1;i<group.size();++i)strides[group[i]]=found->second;
        for(const auto& group:memberGroups)
            if(const auto found=matrixGroups.find(group[0]);found!=matrixGroups.end())
                for(std::size_t i=1;i+1<group.size();i+=2) {
                    auto& layout=matrices[{group[i],group[i+1]}];
                    if(found->second.stride)layout.stride=found->second.stride;
                    if(found->second.rowMajor)layout.rowMajor=found->second.rowMajor;
                }
    }
    std::vector<std::uint32_t> arrayStrides(std::uint32_t id) const {
        std::vector<std::uint32_t> result;std::unordered_set<std::uint32_t> seen;
        for(;;) {
            if(!seen.insert(id).second)throw std::invalid_argument("Cyclic SPIR-V array type");
            if(const auto p=pointers.find(id);p!=pointers.end()){id=p->second;continue;}
            const auto element=arrays.find(id);if(element==arrays.end())break;
            const auto stride=strides.find(id);result.push_back(stride==strides.end()?0:stride->second);
            id=element->second;
        }
        return result;
    }
};

Interface::Numeric numeric(const SpvReflectNumericTraits& source,
                           const SpvReflectTypeDescription* type,
                           SpvReflectDecorationFlags flags) {
    Interface::Numeric result;
    if(type) {
        if(type->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)result.scalar=Interface::Numeric::Scalar::Boolean;
        else if(type->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)result.scalar=Interface::Numeric::Scalar::FloatingPoint;
        else if(type->type_flags & SPV_REFLECT_TYPE_FLAG_INT)result.scalar=source.scalar.signedness
            ?Interface::Numeric::Scalar::SignedInteger:Interface::Numeric::Scalar::UnsignedInteger;
    }
    result.width=source.scalar.width;result.components=source.vector.component_count;
    result.columns=source.matrix.column_count;result.rows=source.matrix.row_count;
    result.matrixStride=source.matrix.stride;result.rowMajor=(flags & SPV_REFLECT_DECORATION_ROW_MAJOR)!=0;
    return result;
}
Interface::Array array(const SpvReflectArrayTraits& source, const SpvReflectTypeDescription* type,
                       const TypeIndex& index) {
    Interface::Array result;
    result.dimensions.assign(source.dims,source.dims+source.dims_count);
    result.specializationIds.assign(source.spec_constant_op_ids,source.spec_constant_op_ids+source.dims_count);
    result.stride=source.stride;
    if(type)result.strides=index.arrayStrides(type->id);
    return result;
}
Interface::Block block(const SpvReflectBlockVariable& source, const TypeIndex& index,
        std::optional<std::pair<std::uint32_t,std::uint32_t>> owner=std::nullopt) {
    Interface::Block result;
    result.name=text(source.name);result.typeName=source.type_description?text(source.type_description->type_name):"";
    result.offset=source.offset;result.absoluteOffset=source.absolute_offset;
    result.size=source.size;result.paddedSize=source.padded_size;
    result.used=(source.flags & SPV_REFLECT_VARIABLE_FLAGS_UNUSED)==0;
    result.numeric=numeric(source.numeric,source.type_description,source.decoration_flags);
    // Matrix arrays can lose their member decoration in SPIRV-Reflect's
    // flattened numeric traits. MatrixStride and major order belong to the
    // owning struct member, including when that member is an array of matrices.
    if(owner&&result.numeric.columns)if(const auto m=index.matrices.find(*owner);m!=index.matrices.end()) {
        if(m->second.stride)result.numeric.matrixStride=*m->second.stride;
        if(m->second.rowMajor)result.numeric.rowMajor=*m->second.rowMajor;
    }
    // SPIRV-Reflect leaves BlockVariable::array empty for OpTypeRuntimeArray;
    // the complete dimensions and stride are available on its type description.
    result.array=array(source.array.dims_count || !source.type_description
        ? source.array : source.type_description->traits.array,source.type_description,index);
    const auto* type=source.type_description;
    if(type&&type->struct_type_description)type=type->struct_type_description;
    for(std::uint32_t i=0;i<source.member_count;++i)
        result.members.push_back(block(source.members[i],index,type?std::optional(std::pair(type->id,i)):std::nullopt));
    // An explicitly padded outer dimension can be larger than the product of
    // the dimension counts and SPIRV-Reflect's innermost stride.
    std::uint64_t extent=result.size;
    if(!result.array.dimensions.empty()) {
        if(result.array.dimensions.front() && !result.array.strides.empty())
            extent=std::max(extent,std::uint64_t(result.array.dimensions.front())*result.array.strides.front());
    } else {
        if(result.numeric.columns&&result.numeric.rows&&result.numeric.matrixStride)
            extent=std::max(extent,std::uint64_t(result.numeric.rowMajor?result.numeric.rows:result.numeric.columns)*result.numeric.matrixStride);
        for(const auto& child:result.members)
            extent=std::max(extent,std::uint64_t(child.offset)+child.size);
    }
    if(extent>std::numeric_limits<std::uint32_t>::max())throw std::invalid_argument("Reflected block extent overflow");
    result.size=std::uint32_t(extent);result.paddedSize=std::max(result.paddedSize,result.size);
    return result;
}
Interface::Variable variable(const SpvReflectInterfaceVariable& source, const TypeIndex& index) {
    Interface::Variable result;
    result.name=text(source.name);result.typeName=source.type_description?text(source.type_description->type_name):"";
    result.location=source.location;result.component=source.component;result.format=source.format;
    result.builtIn=source.built_in;result.numeric=numeric(source.numeric,source.type_description,source.decoration_flags);
    result.array=array(source.array,source.type_description,index);
    for(std::uint32_t i=0;i<source.member_count;++i)result.members.push_back(variable(source.members[i],index));
    return result;
}
}
VulkanShaderInterface VulkanShaderInterface::read(std::span<const std::uint32_t> words,
                                                const std::string& entryPoint) {
    if(words.size()<5 || words[0]!=0x07230203 || words[4]!=0)
        throw std::invalid_argument("Invalid SPIR-V header");
    for(std::size_t i=5;i<words.size();) {
        const auto count=words[i]>>16;
        if(!count || count>words.size()-i)throw std::invalid_argument("Truncated SPIR-V instruction");
        i+=count;
    }
    if(entryPoint.find('\0')!=std::string::npos)throw std::invalid_argument("Invalid SPIR-V entry point name");
    const TypeIndex index(words);
    const auto expanded=reflectionCopy(words);
    const auto reflectionWords=expanded.empty()?words:std::span<const std::uint32_t>(expanded);
    Module module;checked(spvReflectCreateShaderModule(reflectionWords.size_bytes(),reflectionWords.data(),&module.value));
    module.initialized=true;
    const auto* entry=spvReflectGetEntryPoint(&module.value,entryPoint.c_str());
    if(!entry)throw std::invalid_argument("SPIR-V entry point was not found");
    VulkanShaderInterface result;
    result.entryPoint=entryPoint;result.stage=entry->shader_stage;
    result.localSize={entry->local_size.x,entry->local_size.y,entry->local_size.z};
    std::uint32_t count=0;
    checked(spvReflectEnumerateEntryPointDescriptorBindings(&module.value,entryPoint.c_str(),&count,nullptr));
    std::vector<SpvReflectDescriptorBinding*> bindings(count);
    if(count)checked(spvReflectEnumerateEntryPointDescriptorBindings(&module.value,entryPoint.c_str(),&count,bindings.data()));
    for(const auto* source:bindings) {
        Descriptor descriptor;
        descriptor.name=text(source->name);descriptor.typeName=source->type_description?text(source->type_description->type_name):"";
        descriptor.set=source->set;descriptor.binding=source->binding;descriptor.type=source->descriptor_type;
        descriptor.count=source->count;
        descriptor.arrayDimensions.assign(source->array.dims,source->array.dims+source->array.dims_count);
        descriptor.imageDimension=source->image.dim;descriptor.imageFormat=source->image.image_format;
        descriptor.imageArray=source->image.arrayed!=0;descriptor.imageMultisampled=source->image.ms!=0;
        descriptor.imageDepth=source->image.depth;descriptor.imageSampled=source->image.sampled;
        descriptor.block=block(source->block,index);
        result.descriptors.push_back(std::move(descriptor));
    }
    for(std::uint32_t i=0;i<entry->input_variable_count;++i)result.inputs.push_back(variable(*entry->input_variables[i],index));
    for(std::uint32_t i=0;i<entry->output_variable_count;++i)result.outputs.push_back(variable(*entry->output_variables[i],index));
    count=0;checked(spvReflectEnumerateEntryPointPushConstantBlocks(&module.value,entryPoint.c_str(),&count,nullptr));
    std::vector<SpvReflectBlockVariable*> constants(count);
    if(count)checked(spvReflectEnumerateEntryPointPushConstantBlocks(&module.value,entryPoint.c_str(),&count,constants.data()));
    for(const auto* source:constants)result.pushConstants.push_back(block(*source,index));
    return result;
}
