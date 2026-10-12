#include "Three.h"
#include <cmath>
#include <iostream>

namespace {
JsonValue number(double v) {
    if (std::isnan(v))
        return "NaN";
    if (std::isinf(v))
        return v > 0 ? "Infinity" : "-Infinity";
    return v;
}
JsonValue vec(const Vector3& v) {
    return JsonValue::Array{number(v.x), number(v.y), number(v.z)};
}
JsonValue attribute(const VertexAttribute& a) {
    JsonValue::Array values;
    const auto array=a.getArray();
    for(std::size_t i=0;i<array.size();++i) values.push_back(number(array.number(double(i))));
    return JsonValue::Object{{"array",values},{"itemSize",a.getItemSize()},{"count",a.getCount()},
        {"normalized",a.getNormalized()},{"version",a.getProperty("version").toNumber()}};
}
JsonValue geometry(BufferGeometry& g) {
    JsonValue as = JsonValue::Object{}, groups = JsonValue::Array{};
    for (const auto& [name, a] : g.attributes)
        as[name] = attribute(*a);
    for (const auto& group : g.groups)
        groups.get<JsonValue::Array>().push_back(
            JsonValue::Object{{"start", group.start},
                              {"count", group.count},
                              {"materialIndex", group.materialIndex}});
    g.computeBoundingBox();
    g.computeBoundingSphere();
    return JsonValue::Object{
        {"type", g.type},
        {"index", g.index ? attribute(*g.index) : JsonValue()},
        {"attributes", as},
        {"groups", groups},
        {"boundingBox",
         JsonValue::Object{{"min", vec(g.boundingBox->min)}, {"max", vec(g.boundingBox->max)}}},
        {"boundingSphere", JsonValue::Object{{"center", vec(g.boundingSphere->center)},
                                             {"radius", number(g.boundingSphere->radius)}}}};
}
JsonValue material(const MeshBasicMaterial& m) {
    return JsonValue::Object{{"type", m.type},
                             {"color", JsonValue::Array{m.color.r, m.color.g, m.color.b}},
                             {"side", m.side},
                             {"blending", m.blending},
                             {"opacity", m.opacity},
                             {"transparent", m.transparent},
                             {"depthFunc", m.depthFunc},
                             {"depthTest", m.depthTest},
                             {"depthWrite", m.depthWrite},
                             {"alphaTest", static_cast<double>(m.alphaTest)},
                             {"version", m.version},
                             {"vertexColors", m.vertexColors},
                             {"wireframe", m.wireframe},
                             {"fog", m.fog},
                             {"reflectivity", m.reflectivity},
                             {"refractionRatio", m.refractionRatio}};
}
} // namespace
int main() {
    JsonValue output = JsonValue::Object{};
    JsonValue geoms = JsonValue::Array{};
    for (const auto& params : std::vector<std::vector<double>>{
             {1, 1, 1, 1}, {3, 2, 3, 2}, {-2, 3, 2.9, 1.9}, {0, 0, 1, 1}, {1, 1, 0, 0}}) {
        PlaneGeometry g(params[0], params[1], params[2], params[3]);
        geoms.get<JsonValue::Array>().push_back(geometry(g));
    }
    for (const auto& params : std::vector<std::vector<double>>{{1, 1, 1, 1, 1, 1},
                                                               {2, 3, 4, 2, 3, 2},
                                                               {-2, 3, -4, 2.9, 1.9, 2},
                                                               {0, 2, 0, 1, 1, 1}}) {
        BoxGeometry g(params[0], params[1], params[2], params[3], params[4], params[5]);
        geoms.get<JsonValue::Array>().push_back(geometry(g));
    }
    BoxGeometry transformed(2, 3, 4, 2, 1, 3);
    transformed.computeBoundingBox();
    transformed.computeBoundingSphere();
    transformed.rotateY(.37).scale(-2, .5, 3).translate(4, -3, 2);
    geoms.get<JsonValue::Array>().push_back(geometry(transformed));
    PlaneGeometry normalPlane(3, 2, 3, 2);
    normalPlane.deleteAttribute("normal");
    normalPlane.computeVertexNormals();
    geoms.get<JsonValue::Array>().push_back(geometry(normalPlane));
    auto expanded = normalPlane.toNonIndexed();
    expanded->computeVertexNormals();
    geoms.get<JsonValue::Array>().push_back(geometry(*expanded));
    output["geometries"] = geoms;
    BufferGeometry indexBoundary;
    indexBoundary.setIndex(std::vector<std::uint32_t>{0, 65535});
    output["indexBoundaryUint32"] =
        std::holds_alternative<Uint32Array>(indexBoundary.index->array);

    Int8BufferAttribute signedAttribute({-128, -127, -64, 0, 64, 127}, 1, true);
    JsonValue normalized = JsonValue::Array{};
    for (std::size_t i = 0; i < signedAttribute.count; ++i)
        normalized.get<JsonValue::Array>().push_back(signedAttribute.getX(i));
    signedAttribute.setX(0, -.5).setX(1, .5);
    signedAttribute.needsUpdate = false;
    signedAttribute.needsUpdate = true;
    signedAttribute.setUsage(DynamicDrawUsage);
    signedAttribute.addUpdateRange(1, 2);
    auto attrClone = signedAttribute.clone();
    signedAttribute.setX(2, 1);
    output["attribute"] = JsonValue::Object{{"normalized", normalized},
                                            {"written", attribute(signedAttribute)},
                                            {"clone", attribute(*attrClone)},
                                            {"cloneUsage", attrClone->usage.toNumber()},
                                            {"cloneRanges", attrClone->updateRanges.value().length()}};
    Uint8BufferAttribute unsignedAttribute({0, 0, 0, 0}, 1);
    unsignedAttribute.set(std::vector<double>{-1, 256, 511.9, NAN});
    output["integerWrap"] = attribute(unsignedAttribute);
    BufferAttribute source(std::vector<std::uint16_t>{65535, 32768}, 1, true),
        target(std::vector<std::uint8_t>{0, 0}, 1, true);
    target.copyAt(0, source, 0).copyAt(1, source, 1);
    output["copyAtRaw"] = attribute(target);

    auto morphGeometry = std::make_shared<PlaneGeometry>(2, 2);
    auto morph = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 1, 0, 0, 2, 0, 0, 3, 0, 0, 4}, 3);
    morph->name = "raise";
    morphGeometry->morphAttributes["position"] = {morph};
    morphGeometry->morphTargetsRelative = true;
    Mesh mesh(morphGeometry);
    mesh.morphTargetInfluences->at(0) = .25;
    Vector3 v;
    output["morph"] = JsonValue::Object{{"geometry", geometry(*morphGeometry)},
                                        {"position", vec(mesh.getVertexPosition(2, v))},
                                        {"name", mesh.morphTargetDictionary->at("raise")}};
    auto copy = std::dynamic_pointer_cast<Mesh>(mesh.clone());
    output["meshCloneShares"] = copy->geometry == mesh.geometry &&
                                copy->material == mesh.material &&
                                copy->morphTargetInfluences == mesh.morphTargetInfluences;

    MeshBasicMaterial m;
    output["materialDefault"] = material(m);
    m.setValues(JsonValue::Object{{"color", 0x4080c0},
                                  {"opacity", .5},
                                  {"transparent", true},
                                  {"side", DoubleSide},
                                  {"wireframe", true}});
    m.alphaTest = .1;
    m.alphaTest = .2;
    m.alphaTest = 0;
    m.needsUpdate = true;
    m.needsUpdate = false;
    output["materialConfigured"] = material(m);
    auto mc = std::dynamic_pointer_cast<MeshBasicMaterial>(m.clone());
    output["materialClone"] = material(*mc);
    Box3 empty;
    Vector3 center, size;
    output["emptyBox"] = JsonValue::Object{{"empty", empty.isEmpty()},
                                           {"center", vec(empty.getCenter(center))},
                                           {"size", vec(empty.getSize(size))}};
    Sphere sphere(Vector3(2, 0, 0), 1);
    sphere.expandByPoint(Vector3(-2, 0, 0)).unionWith(Sphere(Vector3(0, 3, 0), 2));
    output["sphereUnion"] =
        JsonValue::Object{{"center", vec(sphere.center)}, {"radius", sphere.radius}};
    BufferGeometry aliasedGeometry;
    aliasedGeometry.userData["nested"] = JsonValue::Object{{"count", 1}};
    auto aliasedCopy = aliasedGeometry.clone();
    aliasedGeometry.userData["nested"]["count"] = 3;
    aliasedCopy->userData["nested"]["count"] = 7;
    MeshBasicMaterial taggedMaterial;
    taggedMaterial.userData["nested"] = JsonValue::Object{{"count", 1}};
    auto taggedCopy = taggedMaterial.clone();
    taggedMaterial.userData["nested"]["count"] = 2;
    taggedCopy->userData["nested"]["count"] = 3;
    output["userDataCopySemantics"] = JsonValue::Object{
        {"geometryShared", &aliasedGeometry.userData.storage() == &aliasedCopy->userData.storage()},
        {"geometryCounts", JsonValue::Array{aliasedGeometry.userData.at("nested").at("count"),
                                            aliasedCopy->userData.at("nested").at("count")}},
        {"materialCounts", JsonValue::Array{taggedMaterial.userData.at("nested").at("count"),
                                            taggedCopy->userData.at("nested").at("count")}}};
    JsonValue morphFirstKeys = JsonValue::Array{};
    for (const bool positionFirst : {true, false}) {
        auto geometry = std::make_shared<BufferGeometry>();
        auto position = std::make_shared<Float32BufferAttribute>(std::vector<float>{0, 0, 0}, 3);
        auto normal = std::make_shared<Float32BufferAttribute>(std::vector<float>{0, 0, 1}, 3);
        position->name = "position-shape";
        normal->name = "normal-shape";
        geometry->morphAttributes[positionFirst ? "position" : "normal"] = {positionFirst ? position
                                                                                          : normal};
        geometry->morphAttributes[positionFirst ? "normal" : "position"] = {
            positionFirst ? normal : position};
        Mesh named(geometry);
        morphFirstKeys.get<JsonValue::Array>().push_back(
            named.morphTargetDictionary->begin()->first);
    }
    output["morphFirstKeys"] = std::move(morphFirstKeys);
    BufferGeometry pointsGeometry;
    pointsGeometry.setFromPoints({Vector3(1, 2, NAN), Vector3(3, 4, -0.0)});
    output["pointTruthyInitial"] = attribute(*pointsGeometry.getAttribute("position"));
    pointsGeometry.setFromPoints({Vector3(5, 6, NAN), Vector3(7, 8, INFINITY)});
    output["pointTruthyUpdated"] = attribute(*pointsGeometry.getAttribute("position"));
    Sphere overflowing(Vector3(0, 0, 0), 1e200);
    overflowing.expandByPoint(Vector3(1e300, 0, 0));
    output["sphereOverflow"] = JsonValue::Object{{"center", vec(overflowing.center)},
                                                 {"radius", number(overflowing.radius)}};
    std::cout << output.dump() << '\n';
}
