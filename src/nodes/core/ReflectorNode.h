#pragma once
#include <string>

// Port of the rules in src/nodes/utils/ReflectorNode.js at the pinned revision. The node renders a
// mirrored view of the scene for a flat surface, and four of its rules are worth writing down.
class ReflectorNode {
  public:
    static constexpr const char* kTypeName = "ReflectorNode";

    // Whether the reflector is built for a given object. A quad mesh is skipped, which is how the
    // node stays out of post processing, where everything is drawn on a quad and a reflection
    // would be meaningless; every other object builds it.
    static bool buildsFor(bool objectIsQuadMesh);

    // Whether a depth node may be asked for. Only a reflector made with the depth option may
    // answer one, and asking otherwise throws rather than answering with a default.
    static bool mayAskForDepth(bool depthOption);

    // The sentence thrown in that case. It ends with a space before the closing quote, which is
    // part of the text rather than a mistake of this port.
    static std::string depthNotEnabledMessage();

    // Whether the texture node updates its matrix. The reflector turns that off when it is made,
    // because the reflection is sampled in screen space and a matrix would be the wrong thing to
    // apply.
    static bool updatesMatrix();

    // How a reflector avoids rendering itself: a flag outside the class is raised while one is
    // being rendered, so a second asked for during that time is declined. The flag belongs to the
    // module rather than to an instance, so it is shared by every reflector.
    static bool skipsWhileOneIsBeingRendered(bool alreadyRendering);

    // The default configuration, which is what a caller who says nothing gets.
    static constexpr bool kDefaultGenerateMipmaps = false;
    static constexpr bool kDefaultBounces = true;
    static constexpr bool kDefaultDepth = false;
    static constexpr int kDefaultResolutionScale = 1;
};
