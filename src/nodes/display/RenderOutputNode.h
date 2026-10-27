#pragma once
#include <string>
#include <vector>
#include "core/JSValue.h"

JSValue nativeRenderOutputNodeConstructor();
JSValue nativeRenderOutputNodeModule();
JSValue nativeRenderOutputNodeBinding(const std::string& name);

class RenderOutputNode {
    JSValue object_;
public:
    RenderOutputNode(const JSValue& colorNode={},const JSValue& toneMapping={},const JSValue& outputColorSpace={});
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeRenderOutputNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue setToneMapping(const JSValue& value)const{return object_.get("setToneMapping").call(object_,{value});}
    JSValue getToneMapping()const{return object_.get("getToneMapping").call(object_);}
    JSValue setup(const JSValue& builder)const{return object_.get("setup").call(object_,{builder});}
};

// Port of the ordering and the two fallbacks in src/nodes/display/RenderOutputNode.js at the
// pinned revision. The node runs five steps when it is built, and two of the five undo one
// another, which leaves the step between them as the only lasting effect of the pair.
class RenderOutput {
  public:
    static constexpr const char* kTypeName = "RenderOutputNode";

    // The steps the node runs, in order.
    enum class Step {
        ClampAlpha,
        Unpremultiply,
        ToneMapping,
        WorkingToOutput,
        Premultiply
    };

    static std::vector<Step> steps(bool premultipliedAlpha, bool appliesToneMapping,
                                   bool convertsColorSpace);

    // The tone mapping to use. The node's own is taken when it is not null, and the context's
    // otherwise; and the answer then falls back to the none value when it is falsy. Since the
    // none value is nought, a tone mapping of nought is indistinguishable from an absent one,
    // which the two tests together produce.
    static int toneMapping(bool hasOwn, int own, int context, int noneValue);

    // The colour space to use, by the same two tests.
    static std::string outputColorSpace(bool hasOwn, const std::string& own,
                                        const std::string& context, const std::string& noneValue);

    // Whether the conversion to the output colour space happens. It is skipped when the output
    // is the none space and also when it is already the working space, so two conditions guard
    // it rather than one.
    static bool converts(bool isNoneSpace, bool isWorkingSpace);

    // Whether the alpha is clamped. It is, before anything else, and the clamp is what the pair
    // of alpha steps leaves behind: they undo one another, so a colour that came in
    // premultiplied would leave in the same state were it not for this.
    static double clampAlpha(double alpha);
};
