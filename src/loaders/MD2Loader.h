#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the header and texture coordinate half of examples/jsm/loaders/MD2Loader.js at the
// pinned revision. MD2 is a little endian Quake 2 model: a fixed header of seventeen int32
// fields, then skins, texture coordinates, triangles, frames and OpenGL commands at offsets
// the header names. Reading triangles and the keyframe animation is the layer above.
class MD2Loader {
  public:
    // The header fields in file order, all little endian int32.
    struct Header {
        std::int32_t ident = 0;
        std::int32_t version = 0;
        std::int32_t skinwidth = 0;
        std::int32_t skinheight = 0;
        std::int32_t framesize = 0;
        std::int32_t num_skins = 0;
        std::int32_t num_vertices = 0;
        std::int32_t num_st = 0;
        std::int32_t num_tris = 0;
        std::int32_t num_glcmds = 0;
        std::int32_t num_frames = 0;
        std::int32_t offset_skins = 0;
        std::int32_t offset_st = 0;
        std::int32_t offset_tris = 0;
        std::int32_t offset_frames = 0;
        std::int32_t offset_glcmds = 0;
        std::int32_t offset_end = 0;
    };

    // The magic value upstream compares against, which spells IDP2.
    static constexpr std::int32_t kIdent = 844121161;
    static constexpr std::int32_t kVersion = 8;
    static constexpr std::size_t kHeaderBytes = 17 * 4;

    // Reads the seventeen fields. Throws std::runtime_error with the upstream message when
    // the data is too short.
    static Header parseHeader(const std::vector<std::uint8_t>& buffer);

    // Whether the header identifies an MD2 file of version eight.
    static bool isValid(const Header& header);
    // Whether the file length matches the offset_end field, which catches truncation.
    static bool isConsistent(const Header& header, std::size_t byteLength);

    // Reads the texture coordinates as pairs, dividing by the skin size and flipping v.
    // Throws when the data ends before the coordinates do.
    static std::vector<double> parseUVs(const std::vector<std::uint8_t>& buffer,
                                        const Header& header);
};
