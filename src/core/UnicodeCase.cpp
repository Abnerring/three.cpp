#include "core/UnicodeCase.h"
#include "core/UnicodeCaseData.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
namespace {
void append(std::u16string& output, std::uint32_t point) {
    if (point <= 0xffff) output.push_back(static_cast<char16_t>(point));
    else {
        point -= 0x10000;
        output.push_back(static_cast<char16_t>(0xd800 + (point >> 10)));
        output.push_back(static_cast<char16_t>(0xdc00 + (point & 0x3ff)));
    }
}
}
std::u16string unicodeToUpperCase(std::u16string_view input) {
    std::u16string output;
    output.reserve(input.size());
    for (std::size_t i=0; i<input.size(); ++i) {
        std::uint32_t point=input[i];
        if (point>=0xd800 && point<=0xdbff && i+1<input.size()) {
            const std::uint32_t next=input[i+1];
            if (next>=0xdc00 && next<=0xdfff) {
                point=0x10000 + ((point-0xd800)<<10) + (next-0xdc00);
                ++i;
            }
        }
        using namespace UnicodeCase_detail;
        const auto found=std::lower_bound(std::begin(upperMappings),std::end(upperMappings),point,
            [](const UpperMapping& mapping, std::uint32_t p){return mapping.point<p;});
        if (found!=std::end(upperMappings) && found->point==point)
            for (unsigned j=0;j<found->count;++j) append(output,found->result[j]);
        else append(output,point);
    }
    return output;
}
