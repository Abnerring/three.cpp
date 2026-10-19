#include "catch.hpp"
#include "textures/HTMLTexture.h"
#include "textures/TextureSource.h"
#include <memory>

// An element backed texture turns mipmaps off and is ready on construction. The paint hooks upstream wires to
// the parent element are a host concern, so the host raises the update flag again when its pixels change.
TEST_CASE("HTMLTexture turns mipmaps off and marks itself ready") {
    auto image = std::make_shared<TextureImage>(8.0, 4.0);
    HTMLTexture texture(image);
    REQUIRE(texture.isHTMLTexture);
    REQUIRE(texture.generateMipmaps == false);
    REQUIRE(texture.image.get().get() == image.get());
    REQUIRE(texture.image.get().get()->height == 4.0);
    REQUIRE(texture.minFilter == LinearMipmapLinearFilter);
}