#include "core/ArraySort.h"
#include "core/JsonValue.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

namespace {
struct Item { int id; double key; };
std::uint32_t next(std::uint32_t& state) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state;
}
}
int main() {
    std::size_t count;int mode;std::uint32_t seed;
    while(std::cin>>count>>mode>>seed){
        std::vector<std::shared_ptr<Item>> values;
        for(std::size_t i=0;i<count;++i){std::string key;std::cin>>key;values.push_back(std::make_shared<Item>(Item{int(i),std::stod(key)}));}
        JsonValue::Array comparisons;bool threw=false;
        try{v8ArraySort(values,[&](const auto& a,const auto& b)->double{
            comparisons.emplace_back(JsonValue::Array{a->id,b->id});
            if(mode==7 && comparisons.size()==17)throw std::runtime_error("comparison");
            if(mode==6 && comparisons.size()==1){values.push_back(std::make_shared<Item>(Item{int(count),-9}));values[0]->key=123;}
            if(mode==1){const auto difference=(a->id%3-b->id%3+3)%3;return difference==0?0:difference==1?-1:1;}
            if(mode==2){const auto code=next(seed)%7;return code==0?NAN:double(int(code)-3);}
            if(mode==3 && (a->id%7==0 || b->id%7==0))return NAN;
            if(mode==5)return NAN;
            return mode==4?b->key-a->key:a->key-b->key;
        });}catch(const std::runtime_error&){threw=true;}
        JsonValue::Array order;for(const auto& item:values)order.emplace_back(item->id);
        std::cout<<JsonValue(JsonValue::Object{{"order",order},{"comparisons",comparisons},{"threw",threw}}).dump()<<'\n';
    }
}
