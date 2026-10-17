// Differential probe for the native src/loaders/BufferGeometryLoader.js port.
// One serialized geometry per stdin line; the output is the shape of what was parsed plus the raw
// float32 bits of each attribute, because a bit pattern seen as a number is exactly the mistake this
// loader made once.
#include "animation/AnimationUtils.h"
#include "core/JsonValue.h"
#include "core/NativeObject.h"
#include "core/JSValue.h"
#include "loaders/BufferGeometryLoader.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
int main() {
    const auto realm=JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while(std::getline(std::cin,line)) {
        try {
            BufferGeometryLoader loader;
            const auto typed=loader.parseGeometry(Animation_detail::parseJSON(line));
            const auto geometry=nativeGeometry(typed);
            const auto attributes=geometry.get("attributes");
            JsonValue::Array encoded;
            for(const auto& name:attributes.keys()) {
                const auto attribute=attributes.get(name);
                const auto array=attribute.get("array");
                const double length=array.get("length").toNumber();
                std::string bits;
                for(double index=0;index<length;index+=1) {
                    const float value=static_cast<float>(array.get(std::to_string(static_cast<long long>(index))).toNumber());
                    std::uint32_t word=0;
                    std::memcpy(&word,&value,sizeof(word));
                    char hex[16];
                    std::snprintf(hex,sizeof(hex),"%08x ",word);
                    bits+=hex;
                }
                while(!bits.empty()&&bits.back()==' ')bits.pop_back();
                encoded.emplace_back(JsonValue::Object{{"name",name},
                    {"itemSize",attribute.get("itemSize").toNumber()},
                    {"count",attribute.get("count").toNumber()},
                    {"bits",bits}});
            }
            const auto groups=geometry.get("groups");
            const auto morphAttributes=geometry.get("morphAttributes");
            double morphCount=0;
            if(!morphAttributes.isUndefined()) {
                const auto list=morphAttributes.get("position");
                if(!list.isUndefined())morphCount=list.get("length").toNumber();
            }
            std::cout << JsonValue(JsonValue::Object{
                {"attributes",JsonValue(encoded)},
                {"hasIndex",!geometry.get("index").isUndefined()&&!geometry.get("index").isNull()},
                {"morph",morphCount},
                {"groups",static_cast<double>(typed->groups.size())},
                {"morphRelative",typed->morphTargetsRelative}}).dump() << '\n';
        } catch(const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError",error.what()}}).dump() << '\n';
        }
    }
    return 0;
}