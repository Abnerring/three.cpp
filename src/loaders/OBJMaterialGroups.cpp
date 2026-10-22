#include "loaders/OBJMaterialGroups.h"

OBJMaterialGroups::Material& OBJMaterialGroups::startMaterial(
    const std::string& name, const std::vector<std::string>& libraries, int vertexCount) {
    // The previous run ends at the current vertex count, which the caller supplies because a
    // zero here would make every earlier run look empty and get dropped.
    Material* previous = finalize(false, vertexCount);

    // An inherited material, or one that covered no faces, is replaced by this declaration.
    if (previous != nullptr && (previous->inherited || previous->groupCount <= 0)) {
        _materials.erase(_materials.begin() + previous->index);
    }

    Material material;
    material.index = static_cast<int>(_materials.size());
    material.name = name;
    // The last library wins, as upstream takes libraries[ length - 1 ].
    material.mtllib = libraries.empty() ? std::string() : libraries.back();
    material.smooth = previous != nullptr ? previous->smooth : _smooth;
    material.groupStart = previous != nullptr ? previous->groupEnd : 0;
    material.groupEnd = -1;
    material.groupCount = -1;
    material.inherited = false;

    _materials.push_back(material);
    return _materials.back();
}

OBJMaterialGroups::Material* OBJMaterialGroups::currentMaterial() {
    if (_materials.empty()) return nullptr;
    return &_materials.back();
}

const OBJMaterialGroups::Material* OBJMaterialGroups::currentMaterial() const {
    if (_materials.empty()) return nullptr;
    return &_materials.back();
}

OBJMaterialGroups::Material* OBJMaterialGroups::finalize(bool end, int vertexCount) {
    Material* last = currentMaterial();
    if (last != nullptr && last->groupEnd == -1) {
        // The open material ends where the vertices currently end.
        last->groupEnd = vertexCount / 3;
        last->groupCount = last->groupEnd - last->groupStart;
        last->inherited = false;
    }

    // Trailing materials that covered nothing are dropped when the object is closed.
    if (end && _materials.size() > 1) {
        for (int index = static_cast<int>(_materials.size()) - 1; index >= 0; --index) {
            if (_materials[static_cast<std::size_t>(index)].groupCount <= 0)
                _materials.erase(_materials.begin() + index);
        }
    }

    // At least one material is guaranteed so the caller has something to attach group data to.
    if (end && _materials.empty()) {
        Material placeholder;
        placeholder.name.clear();
        placeholder.smooth = _smooth;
        _materials.push_back(placeholder);
    }
    return last;
}
