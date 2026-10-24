#include "nodes/core/NodeBaseFields.h"

int NodeBaseFields::versionAfterAssigning(int version, bool valueIsExactlyTrue) {
    // Only exactly true raises it, so assigning false leaves the version where it was.
    if (valueIsExactlyTrue) return version + 1;
    return version;
}

bool NodeBaseFields::identifierComesFromACounter() {
    return true;
}

bool NodeBaseFields::traceIsAClassLevelSetting() {
    return true;
}
