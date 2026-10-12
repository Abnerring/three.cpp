#pragma once
#include "animation/PropertyMixer.h"
#include "core/SharedArray.h"

class PropertyBindingComposite;

class PropertyBinding : public PropertyBindingInterface {
  public:
    using Composite = PropertyBindingComposite;
    using Function = PropertyMixerCallback<PropertyBinding&, AnimationArray, JSValue>;
    JSValue path, parsedPath, node, rootNode, targetObject, resolvedProperty, propertyName, propertyIndex;
    JSValue BindingType, Versioning;
    SharedArray<Function> GetterByBindingType;
    SharedArray<SharedArray<Function>> SetterByBindingTypeAndVersioning;
    Function getValueFunction, setValueFunction;

    PropertyBinding(JSValue rootNode, JSValue path, JSValue parsedPath = {});
    static std::shared_ptr<PropertyBindingInterface> create(JSValue root, JSValue path, JSValue parsedPath = {});
    static std::string sanitizeNodeName(JSValue name);
    static JSValue parseTrackName(JSValue trackName);
    static JSValue findNode(JSValue root, JSValue nodeName);
    static std::string runtimeFunctionName(const Function& function);
    void getValue(AnimationArray buffer, double offset) override;
    void setValue(AnimationArray buffer, double offset) override;
    void getValueAt(AnimationArray buffer, JSValue offset);
    void setValueAt(AnimationArray buffer, JSValue offset);
    virtual void bind();
    virtual void unbind();

    virtual void _getValue_unavailable(AnimationArray, JSValue) {}
    virtual void _setValue_unavailable(AnimationArray, JSValue) {}
    virtual void _getValue_direct(AnimationArray buffer, JSValue offset);
    virtual void _getValue_array(AnimationArray buffer, JSValue offset);
    virtual void _getValue_arrayElement(AnimationArray buffer, JSValue offset);
    virtual void _getValue_toArray(AnimationArray buffer, JSValue offset);
    virtual void _getValue_unbound(AnimationArray buffer, JSValue offset);
    virtual void _setValue_unbound(AnimationArray buffer, JSValue offset);
    virtual void _setValue_direct(AnimationArray buffer, JSValue offset);
    virtual void _setValue_array(AnimationArray buffer, JSValue offset);
    virtual void _setValue_arrayElement(AnimationArray buffer, JSValue offset);
    virtual void _setValue_fromArray(AnimationArray buffer, JSValue offset);
    virtual void _setValue_direct_setNeedsUpdate(AnimationArray buffer, JSValue offset);
    virtual void _setValue_direct_setMatrixWorldNeedsUpdate(AnimationArray buffer, JSValue offset);
    virtual void _setValue_array_setNeedsUpdate(AnimationArray buffer, JSValue offset);
    virtual void _setValue_array_setMatrixWorldNeedsUpdate(AnimationArray buffer, JSValue offset);
    virtual void _setValue_arrayElement_setNeedsUpdate(AnimationArray buffer, JSValue offset);
    virtual void _setValue_arrayElement_setMatrixWorldNeedsUpdate(AnimationArray buffer, JSValue offset);
    virtual void _setValue_fromArray_setNeedsUpdate(AnimationArray buffer, JSValue offset);
    virtual void _setValue_fromArray_setMatrixWorldNeedsUpdate(AnimationArray buffer, JSValue offset);
};

class PropertyBindingComposite : public PropertyBindingInterface {
  public:
    JSValue _targetGroup, _bindings;
    PropertyBindingComposite(JSValue targetGroup, JSValue path, JSValue optionalParsedPath = {});
    void getValue(AnimationArray buffer, double offset) override;
    void setValue(AnimationArray buffer, double offset) override;
    void getValueAt(AnimationArray buffer, JSValue offset);
    void setValueAt(AnimationArray buffer, JSValue offset);
    virtual void bind();
    virtual void unbind();
};
