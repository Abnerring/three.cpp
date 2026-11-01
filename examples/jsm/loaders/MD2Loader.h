#pragma once
#include "animation/AnimationClip.h"
#include <array>
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

    // Twelve bytes per triangle: three uint16 vertex indices then three uint16 texture
    // coordinate indices, all little endian.
    struct Triangle {
        std::uint16_t vertex[3] = {0, 0, 0};
        std::uint16_t uv[3] = {0, 0, 0};
    };
    static std::vector<Triangle> parseTriangles(const std::vector<std::uint8_t>& buffer,
                                                const Header& header);

    // A frame starts with a scale and a translation, then a sixteen byte name that ends at
    // the first zero byte. The vertex data follows and is the layer above.
    struct FrameHeader {
        double scale[3] = {0, 0, 0};
        double translation[3] = {0, 0, 0};
        std::string name;
    };
    static std::vector<FrameHeader> parseFrameHeaders(const std::vector<std::uint8_t>& buffer,
                                                      const Header& header);

    // The 162 entry normal table, indexed by the byte stored with each vertex. It is the
    // Quake 2 table, extracted from the upstream source rather than retyped.
    static const std::array<std::array<double, 3>, 162>& normalTable();

    // One decoded vertex: a quantised position scaled and translated, then reordered to
    // Y-up by swapping the last two components, with a table normal treated the same way.
    struct Vertex {
        double position[3] = {0, 0, 0};
        double normal[3] = {0, 0, 0};
    };

    // Decodes num_vertices records starting at at: three bytes of position then one normal
    // index. Returns how many bytes were consumed.
    static std::size_t decodeVertices(const std::vector<std::uint8_t>& buffer, std::size_t at,
                                      int count, const double scale[3],
                                      const double translation[3], std::vector<Vertex>& out);

    // A decoded frame: the vertices in file order plus the frame name from the header.
    struct Frame {
        FrameHeader header;
        std::vector<Vertex> vertices;
    };

    // One morph target per frame, with the triangle vertex indices expanded out so the
    // attribute holds three vertices per triangle corner in triangle order.
    struct MorphTargets {
        std::vector<std::vector<double>> positions;
        std::vector<std::vector<double>> normals;
        std::vector<std::string> names;
    };
    static MorphTargets buildMorphTargets(const std::vector<Frame>& frames,
                                          const std::vector<Triangle>& triangles);

    // Groups the frame names into clips the way upstream does: names are split into a
    // prefix and trailing digits, and each prefix becomes one clip whose tracks are the
    // frames in name order. A name without trailing digits joins no group, so a model whose
    // frames are named "a" and "b" produces no clips at all, which is upstream behaviour.
    static AnimationClip::Clips createAnimations(const std::vector<std::string>& frameNames,
                                                 double fps = 10);
};
