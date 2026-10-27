#include "core/UnicodeCase.h"
#include "core/UnicodeCaseData.h"
#include "core/UnicodeLowerCaseData.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>
namespace {
void append(std::u16string& output, std::uint32_t point) {
    if (point <= 0xffff) output.push_back(static_cast<char16_t>(point));
    else {
        point -= 0x10000;
        output.push_back(static_cast<char16_t>(0xd800 + (point >> 10)));
        output.push_back(static_cast<char16_t>(0xdc00 + (point & 0x3ff)));
    }
}
template<std::size_t N>
bool hasProperty(const UnicodeCase_detail::CasePropertyRange (&ranges)[N],std::uint32_t point) {
    const auto found=std::lower_bound(std::begin(ranges),std::end(ranges),point,
        [](const auto& range,std::uint32_t value){return range.last<value;});
    return found!=std::end(ranges)&&found->first<=point;
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
std::u16string unicodeToLowerCase(std::u16string_view input) {
    using namespace UnicodeCase_detail;
    std::vector<std::uint32_t> points;
    points.reserve(input.size());
    for(std::size_t i=0;i<input.size();++i) {
        std::uint32_t point=input[i];
        if(point>=0xd800&&point<=0xdbff&&i+1<input.size()) {
            const std::uint32_t next=input[i+1];
            if(next>=0xdc00&&next<=0xdfff) {
                point=0x10000+((point-0xd800)<<10)+(next-0xdc00);
                ++i;
            }
        }
        points.push_back(point);
    }
    // Context is evaluated in the original string. Ignorable characters, including
    // characters that are also Cased, do not change the nearest significant case.
    std::vector<bool> casedFollowing(points.size()+1,false);
    for(std::size_t i=points.size();i-->0;)
        casedFollowing[i]=hasProperty(caseIgnorableRanges,points[i])
            ?casedFollowing[i+1]:hasProperty(casedRanges,points[i]);
    std::u16string output;
    output.reserve(input.size());
    bool casedPreceding=false;
    for(std::size_t i=0;i<points.size();++i) {
        const auto point=points[i];
        if(point==0x03a3&&casedPreceding&&!casedFollowing[i+1])append(output,0x03c2);
        else {
            const auto found=std::lower_bound(std::begin(lowerMappings),std::end(lowerMappings),point,
                [](const LowerMapping& mapping,std::uint32_t value){return mapping.point<value;});
            if(found!=std::end(lowerMappings)&&found->point==point)
                for(unsigned j=0;j<found->count;++j)append(output,found->result[j]);
            else append(output,point);
        }
        if(!hasProperty(caseIgnorableRanges,point))casedPreceding=hasProperty(casedRanges,point);
    }
    return output;
}
