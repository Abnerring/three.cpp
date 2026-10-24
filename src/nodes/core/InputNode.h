#pragma once
#include "core/JSValue.h"
#include <string>

JSValue nativeInputNodeConstructor();

// Port of the rules in src/nodes/core/InputNode.js at the pinned revision. The node holds a value
// and reports two types for it, and four of its rules are worth writing down, one of them a round
// trip that does not come back.
class InputNode {
    JSValue object_;
  public:
    explicit InputNode(const JSValue& value = {}, const JSValue& nodeType = nullptr);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeInputNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    static constexpr bool isInputNode = true;
    static constexpr const char* kTypeName = "InputNode";

    // The node's type. It is whatever was given when the node was made, or derived from the value
    // when nothing was: so a node made with a number and no type reports a number's type rather
    // than nothing.
    static std::string nodeType(bool typeWasGiven, const std::string& givenType,
                                const std::string& typeFromValue);

    // The input type. By default it is the node type, and a derived class may answer otherwise;
    // a texture is the example, where the input is a texture and the node's value is a vector of
    // four parts. The two are therefore not always the same, and a caller that used one for the
    // other would be right for most kinds and wrong for textures.
    static std::string inputType(bool derivedOverrides, const std::string& nodeTypeValue,
                                 const std::string& derivedInputType);

    // What serialising an array buffer produces. It is turned into text, which is what
    // arrayBufferToBase64 does.
    static bool serialisesArrayBufferAsText();

    // Whether deserialising turns that text back into an array buffer. It does not: the value is
    // only reconstructed when it is an array, and the text is not one, so it is taken as it
    // stands. The round trip therefore gives back the text rather than the buffer, which is a
    // fault in the source rather than something the port introduced.
    static bool deserialisesTextBackToArrayBuffer();

    // Whether a serialised value needs reconstructing from its parts, which is decided by asking
    // whether it is an array. A text is not, so it is taken whole.
    static bool reconstructsFromParts(bool deserialisedValueIsArray);

    // The precision after deserialising, which falls back to nothing when the stored one is
    // falsy: the test is a short circuiting or rather than a test against the absence.
    static bool precisionIsSetAfterDeserialising(bool hasStoredPrecision, bool storedPrecisionTruthy);
};
