#pragma once
#include <string>

// Port of the per-node data bookkeeping in src/nodes/core/NodeBuilder.js at the pinned revision. A
// node's scratch space is not one place but a nesting of places, and which of two caches holds it
// depends on a question the node itself answers.
class NodeBuilderData {
  public:
    // Which of the builder's two caches holds a node's data. A node that calls itself global has
    // its data kept in the global cache, which is shared where the per-build one is not, and one
    // that does not has it kept in the per-build cache; so the node decides where its own scratch
    // space lives by answering a question about itself. A caller may name a cache and override
    // both.
    enum class CacheChoice { PerBuild, Global, Named };

    static CacheChoice cacheFor(bool cacheWasNamed, bool nodeIsGlobal);

    // How deeply a node's data is nested. It is keyed by the node, then by the shader stage, and
    // then, but only inside a sub-build, by that sub-build; so the third level exists in some
    // contexts and not others, and a caller that assumed it everywhere would either miss data or
    // make entries that are never reached.
    static int levelCount(bool insideSubBuild);

    // Whether the sub-build level is reached at all when the builder has no sub-build layers,
    // which it is not: the data is answered at the stage level before that.
    static bool subBuildLevelIsReached(bool hasSubBuildLayers);

    // The stage that asks for a node's properties when nobody names one. It is the word for any
    // rather than the stage the builder happens to be in, so a property belongs to the node as a
    // whole rather than to one stage of it; asking from inside a stage still reaches the same
    // record.
    static constexpr const char* kDefaultPropertiesStage = "any";

    // What a node's properties are the first time they are asked for. The record is made then and
    // given one entry holding no output node, so a caller always receives something with the field
    // it expects rather than an empty record it has to test.
    static bool propertiesAreMadeOnFirstAsk();
    static bool startsWithAnEmptyOutputNode();
};
