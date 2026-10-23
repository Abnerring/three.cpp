#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/accessors/Object3DNode.js at the pinned revision. The node
// reports one of five measurements of a 3D object; which one is fixed when it is made, and the
// type it reports follows from that choice rather than being given.
class Object3DNode {
  public:
    static constexpr const char* kTypeName = "Object3DNode";

    // The five scopes, named as the class names them.
    static const std::vector<std::string>& scopes();

    // The type each scope reports. Four of the five are three part vectors and only the world
    // matrix is a four by four, so the type is worth working out rather than storing per node.
    static std::string nodeType(const std::string& scope);

    // Whether the node may be cached. It may not, this being the third of the modules that
    // refuse outright rather than deciding from their usage.
    static bool isCacheable();

    // The kind of update the node asks for, which is unlike most nodes: it is updated for every
    // object rather than for every frame or render.
    static constexpr const char* kUpdateType = "OBJECT";
};
