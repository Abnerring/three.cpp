#pragma once
#include <string>
#include <vector>
#include "core/JSValue.h"

JSValue nativeColorSpaceNodeConstructor();
JSValue nativeColorSpaceNodeModule();
JSValue nativeColorSpaceNodeBinding(const std::string& name);

// Port of the decisions in src/nodes/display/ColorSpaceNode.js at the pinned revision. A
// conversion applies three steps in a fixed order and is skipped in four cases, and the way
// those four are tested is not uniform.
class ColorSpaceNode {
    JSValue object_;
  public:
    ColorSpaceNode(const JSValue& colorNode,const JSValue& source,const JSValue& target);
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeColorSpaceNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue resolveColorSpace(const JSValue& builder,const JSValue& colorSpace)const;
    JSValue setup(const JSValue& builder)const;
    static constexpr const char* kTypeName = "ColorSpaceNode";
    static constexpr const char* kNodeType = "vec4";

    // The two names that stand for a colour space decided later rather than being given.
    static constexpr const char* kWorkingName = "WorkingColorSpace";
    static constexpr const char* kOutputName = "OutputColorSpace";

    // Which of the three steps a conversion needs, in the order they run: the source's transfer
    // undone, then a matrix if the two spaces differ in their primaries, then the target's
    // transfer applied. The steps are named here so that a test can check their order rather
    // than only that they happened.
    enum class Step { UndoSourceTransfer, PrimariesMatrix, ApplyTargetTransfer };

    // Whether a conversion happens at all. Colour management being exactly off skips it, and so
    // does a source or a target that is equal to the other or that is empty. The empties are
    // tested for truth rather than for being present, so a name that is present but empty skips
    // the conversion just as an absent one does, which is the part worth writing down.
    static bool converts(bool managementEnabled, const std::string& source, const std::string& target);

    // The steps a conversion runs, in order.
    static std::vector<Step> steps(bool sourceIsSrgb, bool primariesDiffer, bool targetIsSrgb);

    // The output colour space to use. The context's own is taken when it is truthy, and the
    // renderer's otherwise, which is a fallback for a value that is empty as well as for one
    // that is missing.
    static std::string outputColorSpace(const std::string& contextColorSpace,
                                       const std::string& rendererColorSpace);

    // The sentence the module would give for a name it does not specially resolve, which is the
    // name itself.
    static std::string resolve(const std::string& name, const std::string& working,
                              const std::string& resolvedOutput);
};
