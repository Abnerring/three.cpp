#include "nodes/core/ViewportDepthTextureNode.h"
#include "nodes/core/ViewportSharedTextureNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ViewportSharedTextureNode shares one texture across every instance") {
    // The texture is made once and reused, so there is no per-instance one at all; that is the
    // whole difference from the ordinary viewport texture node.
    REQUIRE(ViewportSharedTextureNode::createsTheSharedTextureOnce());
    // And the method that answers with it does not consult the node it is called on, since a
    // per-instance answer would defeat the point of sharing.
    REQUIRE(ViewportSharedTextureNode::textureIgnoresTheInstance());
    REQUIRE(std::string(ViewportSharedTextureNode::kTypeName) == "ViewportSharedTextureNode");
}

TEST_CASE("ViewportSharedTextureNode's reference update is a no-op") {
    // The ordinary node builds a new reference for itself; this one answers itself unchanged, so
    // the shared texture is never replaced and a caller that expected a new node gets the same
    // one back.
    REQUIRE(ViewportSharedTextureNode::updateReferenceIsANoOp());
    // The refresh is before the render.
    REQUIRE(std::string(ViewportSharedTextureNode::kUpdateType) == "BEFORE_RENDER");
}

TEST_CASE("ViewportDepthTextureNode uses a given texture as it stands") {
    using Source = ViewportDepthTextureNode::Source;
    // A texture passed in is respected.
    REQUIRE(ViewportDepthTextureNode::sourceFor(true) == Source::Given);
    // Only an absent one is replaced by the shared depth texture.
    REQUIRE(ViewportDepthTextureNode::sourceFor(false) == Source::Shared);
    REQUIRE(ViewportDepthTextureNode::sourceFor(true) != ViewportDepthTextureNode::sourceFor(false));
    REQUIRE(std::string(ViewportDepthTextureNode::kTypeName) == "ViewportDepthTextureNode");
}

TEST_CASE("ViewportDepthTextureNode shares one depth texture but not the sibling's override") {
    // The shared texture is made once and reused.
    REQUIRE(ViewportDepthTextureNode::sharesOneDepthTexture());
    // But this node keeps the ordinary reference behaviour, so only the texture it starts from
    // differs from the ordinary node.
    REQUIRE_FALSE(ViewportDepthTextureNode::overridesReferenceUpdate());
    REQUIRE(ViewportDepthTextureNode::overridesReferenceUpdate() !=
            ViewportSharedTextureNode::updateReferenceIsANoOp());
    REQUIRE(ViewportDepthTextureNode::kParameterCount == 3);
}