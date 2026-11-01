#pragma once
#include <string>
#include <vector>

// Port of the material grouping inside examples/jsm/loaders/OBJLoader.js at the pinned
// revision. An object accumulates one material per usemtl run, each carrying the vertex range
// its faces cover; those ranges become the geometry groups. Building three.js objects and
// geometries from this is the layer above.
class OBJMaterialGroups {
  public:
    struct Material {
        int index = 0;
        std::string name;
        std::string mtllib;
        bool smooth = true;
        int groupStart = 0;
        int groupEnd = -1;
        int groupCount = -1;
        bool inherited = false;
    };

    // Opens a material. The previous one is finalised first, and it is dropped again when it
    // was inherited or covered no vertices, which is how upstream keeps multi material runs
    // contiguous.
    Material& startMaterial(const std::string& name, const std::vector<std::string>& libraries,
                             int vertexCount = 0);

    // The most recently opened material, or nullptr when there is none.
    Material* currentMaterial();
    const Material* currentMaterial() const;

    // Closes the open material at the current vertex count. With end set, materials that
    // covered nothing are removed, and an empty placeholder is added when none remain.
    Material* finalize(bool end, int vertexCount);

    const std::vector<Material>& materials() const { return _materials; }
    // Inherit a material from the previous object, as upstream does across declarations.
    void inheritSmooth(bool smooth) { _smooth = smooth; }
    bool smooth() const { return _smooth; }

  private:
    std::vector<Material> _materials;
    bool _smooth = true;
};
