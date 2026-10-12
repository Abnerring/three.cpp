#include "core/UnicodeNormalization.h"
#include "core/UnicodeNormalizationData.h"
#include <algorithm>
#include <iterator>
#include <vector>

namespace {
using namespace Unicode_detail;
unsigned combining(std::uint32_t code) {
    const auto found=std::lower_bound(std::begin(combiningClasses),std::end(combiningClasses),code,
        [](const auto& entry,auto value){return entry.code<value;});
    return found!=std::end(combiningClasses)&&found->code==code?found->value:0;
}
void decompose(std::uint32_t code,std::vector<std::uint32_t>& result) {
    if(code>=0xac00&&code<0xd7a4) {
        const auto index=code-0xac00;result.push_back(0x1100+index/588);result.push_back(0x1161+(index%588)/28);
        if(index%28)result.push_back(0x11a7+index%28);return;
    }
    const auto found=std::lower_bound(std::begin(decompositions),std::end(decompositions),code,
        [](const auto& entry,auto value){return entry.code<value;});
    if(found==std::end(decompositions)||found->code!=code){result.push_back(code);return;}
    decompose(found->first,result);if(found->second)decompose(found->second,result);
}
std::uint32_t compose(std::uint32_t first,std::uint32_t second) {
    if(first>=0x1100&&first<0x1113&&second>=0x1161&&second<0x1176)
        return 0xac00+((first-0x1100)*21+second-0x1161)*28;
    if(first>=0xac00&&first<0xd7a4&&(first-0xac00)%28==0&&second>0x11a7&&second<0x11c3)
        return first+second-0x11a7;
    const auto wanted=std::pair{first,second};
    const auto found=std::lower_bound(std::begin(compositions),std::end(compositions),wanted,
        [](const auto& entry,const auto& value){return std::pair{entry.first,entry.second}<value;});
    return found!=std::end(compositions)&&found->first==first&&found->second==second?found->result:0;
}
void encode(std::string& result,std::uint32_t code) {
    if(code<0x80)result+=char(code);
    else if(code<0x800){result+=char(0xc0|(code>>6));result+=char(0x80|(code&63));}
    else if(code<0x10000){result+=char(0xe0|(code>>12));result+=char(0x80|((code>>6)&63));result+=char(0x80|(code&63));}
    else{result+=char(0xf0|(code>>18));result+=char(0x80|((code>>12)&63));result+=char(0x80|((code>>6)&63));result+=char(0x80|(code&63));}
}
}
std::string normalizeUnicodeNFC(const std::string& value) {
    std::vector<std::uint32_t> decomposed;
    for(std::size_t i=0;i<value.size();) {
        const auto first=static_cast<unsigned char>(value[i++]);std::uint32_t code=first;
        unsigned remaining=first<0x80?0:first<0xe0?1:first<0xf0?2:3;
        if(remaining)code=first&((1u<<(6-remaining))-1);
        while(remaining--&&i<value.size())code=(code<<6)|(static_cast<unsigned char>(value[i++])&63);
        decompose(code,decomposed);
    }
    // Stable canonical ordering; starters delimit independent runs.
    for(std::size_t i=1;i<decomposed.size();++i) {
        const auto cls=combining(decomposed[i]);if(!cls)continue;
        auto j=i;while(j>0&&combining(decomposed[j-1])>cls){std::swap(decomposed[j],decomposed[j-1]);--j;}
    }
    std::vector<std::uint32_t> result;
    std::size_t starter=0;unsigned previousClass=0;
    for(const auto code:decomposed) {
        const auto cls=combining(code);const auto combined=result.empty()?0:compose(result[starter],code);
        if(combined&&(previousClass==0||previousClass<cls))result[starter]=combined;
        else {if(cls==0)starter=result.size();result.push_back(code);previousClass=cls;}
    }
    std::string encoded;for(const auto code:result)encode(encoded,code);return encoded;
}
