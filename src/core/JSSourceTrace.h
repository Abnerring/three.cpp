#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class JSValue;

// Source locations describe executed translated source, never a synthetic
// JavaScript engine stack. The recorder owns plain metadata only.
namespace JSSourceTrace {
struct FunctionIdentity {
    std::uintptr_t heap = 0;
    std::uint64_t record = 0;
    bool operator==(const FunctionIdentity&) const = default;
    explicit operator bool() const { return heap != 0 && record != 0; }
};
struct Location {
    std::string file;
    int line = 0;
    int column = 0;
    int position = -1;
    std::string scriptHash;
};
struct Frame {
    std::string functionName;
    Location location;
    std::string typeName;
    std::string methodName;
    bool isConstructor = false;
    bool isAsync = false;
    bool isNative = false;
    Location enclosingLocation;
    FunctionIdentity functionIdentity;
};

class NativeCallScope {
  public:
    explicit NativeCallScope(FunctionIdentity identity);
    ~NativeCallScope();
    NativeCallScope(const NativeCallScope&) = delete;
    NativeCallScope& operator=(const NativeCallScope&) = delete;
  private:
    std::size_t depth_;
};
class FrameScope {
  public:
    explicit FrameScope(Frame frame);
    ~FrameScope();
    FrameScope(const FrameScope&) = delete;
    FrameScope& operator=(const FrameScope&) = delete;
  private:
    std::size_t depth_;
};
class PositionScope {
  public:
    explicit PositionScope(Location location);
    ~PositionScope();
    PositionScope(const PositionScope&) = delete;
    PositionScope& operator=(const PositionScope&) = delete;
  private:
    std::size_t index_;
    std::uint64_t serial_ = 0;
    Location previous_;
};

// Newest first. A constructor filter removes that actual function and younger
// frames; absence of the requested function produces no frames, as in V8.
std::vector<Frame> capture(std::size_t limit = 10,
                          FunctionIdentity constructorFilter = {});
std::string receiverTypeName(const JSValue& receiver);
std::string format(const Frame& frame);
JSValue callSites(const std::vector<Frame>& frames);
} // namespace JSSourceTrace
