#include "nodes/core/PropertyNode.h"

std::string PropertyNode::nodeType(const std::string& declaredType,
                                   const std::string& builderOutputType) {
    // The one name that is a request to look the type up rather than a type.
    if (declaredType == kOutputTypeName) return builderOutputType;
    return declaredType;
}

std::string PropertyNode::cacheKeyText(const std::string& type, bool hasName,
                                       const std::string& name, bool varying) {
    // Type, name, flag; the flag as a digit and an absent name as nothing.
    std::string key = type;
    key += ':';
    if (hasName) key += name;
    key += ':';
    key += varying ? '1' : '0';
    return key;
}

bool PropertyNode::isVarying(bool varying) {
    return varying;
}

bool PropertyNode::interpolationSetUnconditionally() {
    // The source assigns true without asking whether it is already set.
    return true;
}

bool PropertyNode::writesPlaceholder(bool hasPlaceholder, bool hasWriteUsage) {
    // Only with a placeholder and only when nothing writes to the property.
    if (!hasPlaceholder) return false;
    return !hasWriteUsage;
}

bool PropertyNode::isCacheable() {
    return false;
}
