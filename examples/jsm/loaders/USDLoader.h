#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/USDLoader.js at the pinned revision. The loader reads
// both text and binary USD files and can also unwrap an archive of them, and three of its rules
// are worth writing down.
class USDLoader {
  public:
    static constexpr const char* kTypeName = "USDLoader";

    // The kind of data read from the file, which is raw bytes rather than text: the format may be
    // binary, so the loader cannot ask for a string and decode it afterwards.
    static constexpr const char* kResponseType = "arraybuffer";

    // The lower case extension of a name. Two questions are asked and either may answer with
    // nothing: a name with no dot has no extension, and a name whose last dot is followed by a
    // slash has none either, since the dot then belongs to a directory rather than to the file.
    // So a path like a dot-named directory followed by a bare file name has no extension, which
    // a reader taking the last dot alone would get wrong.
    static std::string extension(const std::string& filename);

    // The bytes of a buffer, whatever kind of view was handed over. A buffer that is already the
    // whole thing is passed through, a view covering all of its backing store is replaced by that
    // store, and a view covering part of it is copied out. So the answer is always a buffer whose
    // start is its own.
    enum class BufferCase { WholeBuffer, FullView, PartialView };

    static BufferCase bufferCase(bool isWholeBuffer, bool viewCoversAll);

    // The base path a load uses, which is the loader's own unless that is the empty string.
    static std::string basePath(const std::string& loaderPath, const std::string& urlDirectory);

    // Whether the parse answers synchronously while some of its textures arrive later, which it
    // does: the group comes back at once and the caller's handler is told when the rest is ready.
    static bool returnsBeforeTexturesLoaded();
};
