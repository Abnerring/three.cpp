#include "loaders/UltraHDRLoader.h"

#include "catch.hpp"

#include <string>

namespace {
const char* kGainMapPacket =
    "<?xpacket begin='' id='W5M0MpCehiHzreSzNTczkc9d'?>\n"
    "<x:xmpmeta xmlns:x='adobe:ns:meta/'>\n"
    " <rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
    "  <rdf:Description hdrgm:Version='1.0'"
    " xmlns:hdrgm='http://ns.adobe.com/hdr-gain-map/1.0/'"
    " hdrgm:BaseRenditionIsHDR='True'"
    " hdrgm:GainMapMin='-1'"
    " hdrgm:GainMapMax='2.5'"
    " hdrgm:Gamma='1.5'"
    " hdrgm:OffsetSDR='0.5'"
    " hdrgm:OffsetHDR='-0.25'"
    " hdrgm:HDRCapacityMin='0.5'"
    " hdrgm:HDRCapacityMax='3'/>\n"
    " </rdf:RDF>\n"
    "</x:xmpmeta>\n"
    "<?xpacket end='w'?>";
const char* kContainerPacket =
    "<x:xmpmeta><Container:Directory><rdf:Seq/></Container:Directory></x:xmpmeta>";
}  // namespace

TEST_CASE("UltraHDRLoader reads the XMP gain map descriptor") {
    UltraHDRLoader::IsoMetadata metadata;
    UltraHDRLoader::parseXmpMetadata(kGainMapPacket, metadata);
    REQUIRE(metadata.version == "1.0");
    REQUIRE(metadata.baseRenditionIsHDR);
    REQUIRE(metadata.gainMapMin == Approx(-1.0).epsilon(1e-12));
    REQUIRE(metadata.gainMapMax == Approx(2.5).epsilon(1e-12));
    REQUIRE(metadata.gamma == Approx(1.5).epsilon(1e-12));
    // The offsets are written in sixty fourths, so a half becomes thirty two.
    REQUIRE(metadata.offsetSdr == Approx(32.0).epsilon(1e-12));
    REQUIRE(metadata.offsetHdr == Approx(-16.0).epsilon(1e-12));
    REQUIRE(metadata.hdrCapacityMin == Approx(0.5).epsilon(1e-12));
    REQUIRE(metadata.hdrCapacityMax == Approx(3.0).epsilon(1e-12));
}

TEST_CASE("UltraHDRLoader falls back when the XMP attributes are absent") {
    const std::string text =
        "<x:xmpmeta><rdf:RDF><rdf:Description hdrgm:Version='1.0'/></rdf:RDF></x:xmpmeta>";
    UltraHDRLoader::IsoMetadata metadata;
    UltraHDRLoader::parseXmpMetadata(text, metadata);
    // The defaults are the ones the format documents.
    REQUIRE(metadata.gainMapMin == 0.0);
    REQUIRE(metadata.gainMapMax == 1.0);
    REQUIRE(metadata.gamma == 1.0);
    REQUIRE(metadata.offsetSdr == 0.0);
    REQUIRE(metadata.hdrCapacityMin == 0.0);
    REQUIRE(metadata.hdrCapacityMax == 1.0);
    // The flag is only true for the exact word.
    REQUIRE_FALSE(metadata.baseRenditionIsHDR);
}

TEST_CASE("UltraHDRLoader leaves the metadata alone for a container descriptor") {
    UltraHDRLoader::IsoMetadata metadata;
    metadata.gainMapMax = 7.0;
    UltraHDRLoader::parseXmpMetadata(kContainerPacket, metadata);
    // A container is not a gain map, so nothing is overwritten.
    REQUIRE(metadata.gainMapMax == 7.0);
}

TEST_CASE("UltraHDRLoader ignores XMP text without markup") {
    UltraHDRLoader::IsoMetadata metadata;
    metadata.gamma = 1.25;
    UltraHDRLoader::parseXmpMetadata("no markup here", metadata);
    // Nothing changed, and no exception escaped.
    REQUIRE(metadata.gamma == 1.25);
}