#pragma once

#include <optional>
#include <string>
#include <vector>

// Native port of examples/jsm/loaders/materialx/MaterialXLog.js from the pinned three.js revision
// (see upstream.json).
//
// The addon is a small diagnostics sink for the MaterialX loader: a table of nine log codes with
// their severity, and a log that collects entries and filters them by severity. The pinned class
// exposes errors and warnings through getters; C++ has no getters, so they are plain accessors
// returning the filtered vector.
namespace MaterialXLogCodes {

struct Code {
    const char* label;
    const char* severity;
};

inline constexpr Code UNSUPPORTED_NODE{"unsupported-node", "error"};
inline constexpr Code IGNORED_SURFACE_INPUT{"ignored-surface-input", "warning"};
inline constexpr Code MISSING_REFERENCE{"missing-reference", "error"};
inline constexpr Code MISSING_MATERIAL{"missing-material", "error"};
inline constexpr Code INVALID_VALUE{"invalid-value", "error"};
inline constexpr Code TEXTURE_LOAD_FAILED{"texture-load-failed", "warning"};
inline constexpr Code UNKNOWN_INPUT{"unknown-input", "error"};
inline constexpr Code INVALID_OUTPUT_CONNECTION{"invalid-output-connection", "error"};
inline constexpr Code TYPE_MISMATCH{"type-mismatch", "error"};

}  // namespace MaterialXLogCodes

struct MaterialXLogEntry {
    std::string code;
    std::string severity;
    std::string message;
    std::optional<std::string> nodeName;
};

class MaterialXLog {
  public:
    std::vector<MaterialXLogEntry> entries;

    // get errors() { return this.entries.filter( entry => entry.severity === 'error' ); }
    std::vector<MaterialXLogEntry> errors() const;
    // get warnings() { return this.entries.filter( entry => entry.severity === 'warning' ); }
    std::vector<MaterialXLogEntry> warnings() const;

    // add( code, message, nodeName ) { ... }
    void add(const MaterialXLogCodes::Code& code, std::string message);
    void add(const MaterialXLogCodes::Code& code, std::string message, std::string nodeName);
};
