#include "geometries/CircleGeometry.h"
#include "geometries/BoxGeometry.h"
#include "geometries/PlaneGeometry.h"
#include "geometries/RingGeometry.h"
#include "geometries/SphereGeometry.h"
#include "geometries/CylinderGeometry.h"
#include "geometries/ConeGeometry.h"
#include "geometries/TorusGeometry.h"
#include "geometries/TorusKnotGeometry.h"
#include "geometries/LatheGeometry.h"
#include "geometries/CapsuleGeometry.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <cmath>

namespace {
JsonValue number(double n) {
    if (std::isnan(n)) return "NaN";
    if (std::isinf(n)) return n < 0 ? "-Infinity" : "Infinity";
    if (n == 0 && std::signbit(n)) return "-0";
    return n;
}
JsonValue encode(const JsonValue& value) {
    if (std::holds_alternative<double>(value.storage())) return number(value.get<double>());
    if (value.isArray()) {
        JsonValue::Array result;
        for (const auto& item : value.get<JsonValue::Array>()) result.push_back(encode(item));
        return result;
    }
    if (value.isObject()) {
        JsonValue::Object result;
        for (const auto& [key, item] : value.get<JsonValue::Object>()) result[key] = encode(item);
        return result;
    }
    return value;
}
JsonValue serialized(const BufferGeometry& geometry) {
    auto data = geometry.toJSON();
    data.get<JsonValue::Object>().erase("uuid");
    return encode(data);
}
JsonValue state(const BufferGeometry& geometry) {
    JsonValue::Array keys, groups;
    JsonValue::Object attrs;
    for (const auto& [key, attr] : geometry.attributes) {
        keys.emplace_back(key);
        attrs[key] = encode(attr->toJSON());
    }
    for (const auto& group : geometry.groups)
        groups.emplace_back(JsonValue::Array{group.start, group.count, group.materialIndex});
    return JsonValue::Object{{"type", geometry.type}, {"keys", keys}, {"attributes", attrs},
        {"index", encode(geometry.index->toJSON())}, {"groups", groups}, {"json", serialized(geometry)}};
}
template<class G> std::shared_ptr<BufferGeometry> make(const std::vector<double>& a) {
    if (a.empty()) return std::make_shared<G>();
    if constexpr (std::is_same_v<G,BoxGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3],a[4],a[5]);
    if constexpr (std::is_same_v<G,PlaneGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3]);
    if constexpr (std::is_same_v<G,CircleGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3]);
    if constexpr (std::is_same_v<G,RingGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3],a[4],a[5]);
    if constexpr (std::is_same_v<G,SphereGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
    if constexpr (std::is_same_v<G,CylinderGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3],a[4],a[5]!=0,a[6],a[7]);
    if constexpr (std::is_same_v<G,ConeGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3],a[4]!=0,a[5],a[6]);
    if constexpr (std::is_same_v<G,TorusGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
    if constexpr (std::is_same_v<G,TorusKnotGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3],a[4],a[5]);
    if constexpr (std::is_same_v<G,CapsuleGeometry>) return std::make_shared<G>(a[0],a[1],a[2],a[3],a[4]);
    if constexpr (std::is_same_v<G,LatheGeometry>) {
        auto points = std::make_shared<LatheGeometry::Points>();
        for (std::size_t i=3; i+1<a.size(); i+=2) points->push_back(std::make_shared<Vector2>(a[i],a[i+1]));
        return std::make_shared<G>(points,a[0],a[1],a[2]);
    }
}
template<class G> JsonValue run(const std::vector<double>& args) {
    auto geometry = make<G>(args);
    const auto original = state(*geometry);
    auto clone = geometry->clone();
    G copied;
    const bool copyReturn = &copied.copy(*geometry) == &copied;
    auto restored = G::fromJSON(geometry->toJSON());
    auto defaults = G::fromJSON(JsonValue::Object{});
    JsonValue::Object result{{"geometry", original}, {"cloneEqual", state(*clone).dump()==original.dump()},
        {"copyEqual",state(copied).dump()==original.dump()}, {"copyReturn",copyReturn},
        {"cloneUuid",clone->uuid!=geometry->uuid}, {"cloneIndex",clone->index!=geometry->index},
        {"cloneAttribute",clone->getAttribute("position")!=geometry->getAttribute("position")},
        {"roundTripEqual",state(*restored).dump()==original.dump()}, {"defaults",serialized(*defaults)}};
    if constexpr(std::is_same_v<G,LatheGeometry>) {
        auto source = std::dynamic_pointer_cast<G>(geometry), copiedLathe = std::dynamic_pointer_cast<G>(clone);
        result["pointsShared"] = source->parameters.points == copiedLathe->parameters.points;
    }
    geometry->translate(.25,-.5,1);
    result["transformed"] = serialized(*geometry);
    result["transformedCloneEqual"] = serialized(*geometry->clone()).dump()==serialized(*geometry).dump();
    return result;
}
}
int main() {
    std::string line;
    while(std::getline(std::cin,line)) {
        std::istringstream input(line);
        std::string kind, token;
        input >> kind;
        std::vector<double> args;
        while(input >> token) args.push_back(std::stod(token));
        try {
            JsonValue result;
            if (kind == "CircleGeometry") result = run<CircleGeometry>(args);
            else if (kind == "BoxGeometry") result = run<BoxGeometry>(args);
            else if (kind == "PlaneGeometry") result = run<PlaneGeometry>(args);
            else if (kind == "RingGeometry") result = run<RingGeometry>(args);
            else if (kind == "SphereGeometry") result = run<SphereGeometry>(args);
            else if (kind == "CylinderGeometry") result = run<CylinderGeometry>(args);
            else if (kind == "ConeGeometry") result = run<ConeGeometry>(args);
            else if (kind == "TorusGeometry") result = run<TorusGeometry>(args);
            else if (kind == "TorusKnotGeometry") result = run<TorusKnotGeometry>(args);
            else if (kind == "LatheGeometry") result = run<LatheGeometry>(args);
            else if (kind == "CapsuleGeometry") result = run<CapsuleGeometry>(args);
            else throw std::invalid_argument("Unknown geometry");
            std::cout << result.dump() << '\n';
        } catch(const JSTypeError&) { std::cout << "{\"error\":\"TypeError\"}\n"; }
        catch(const JSRangeError&) { std::cout << "{\"error\":\"RangeError\"}\n"; }
        catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    }
}
