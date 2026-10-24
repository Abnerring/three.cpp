#include "nodes/core/NodeBuilderData.h"

NodeBuilderData::CacheChoice NodeBuilderData::cacheFor(bool cacheWasNamed, bool nodeIsGlobal) {
    if (cacheWasNamed) return CacheChoice::Named;
    // The node's own answer decides which of the two holds it.
    if (nodeIsGlobal) return CacheChoice::Global;
    return CacheChoice::PerBuild;
}

int NodeBuilderData::levelCount(bool insideSubBuild) {
    // The node, the stage, and inside a sub-build the sub-build too.
    return insideSubBuild ? 3 : 2;
}

bool NodeBuilderData::subBuildLevelIsReached(bool hasSubBuildLayers) {
    return hasSubBuildLayers;
}

bool NodeBuilderData::propertiesAreMadeOnFirstAsk() {
    return true;
}

bool NodeBuilderData::startsWithAnEmptyOutputNode() {
    return true;
}
