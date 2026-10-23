#pragma once
#include <string>

// Port of the rules in src/nodes/display/FrontFacingNode.js at the pinned revision. The module
// says whether the fragment being drawn faces the camera, and it also offers a way to turn a
// vector over when the answer is no.
class FrontFacing {
  public:
    static constexpr const char* kTypeName = "FrontFacingNode";

    // The three ways a material may be sided, named as the constants module names them.
    enum class Side { Front, Back, Double };

    // What the node generates as text. Two stages and one side decide it without asking
    // anything: a stage other than the fragment stage always says true, since facing is a
    // question only the fragment stage can answer; and a material sided to the back says false
    // everywhere, since every fragment drawn is on the back. Only the rest ask the builder.
    enum class Answer { True, False, AskBuilder };

    static Answer answer(bool fragmentStage, Side side);

    // The number the facing is turned into. True becomes one and false becomes minus one, which
    // is done by doubling and taking one away rather than by a choice.
    static double faceDirection(bool frontFacing);

    // The factor a vector is multiplied by when it must be turned over on the back side. A
    // material sided to the front leaves it alone; one sided to the back always turns it over,
    // whatever the fragment says; and one sided both ways turns it over only where the fragment
    // is on the back, which the facing number already carries.
    static double backSideFactor(Side side, bool frontFacing);

    // The sentence the renamed function prints when it is called.
    static std::string deprecatedRenameWarning();
};
