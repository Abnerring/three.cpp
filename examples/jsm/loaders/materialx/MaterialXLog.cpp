#include "loaders/materialx/MaterialXLog.h"

#include <utility>

std::vector<MaterialXLogEntry> MaterialXLog::errors() const {
    std::vector<MaterialXLogEntry> result;
    for (const auto& entry : entries) {
        if (entry.severity == "error") result.push_back(entry);
    }
    return result;
}

std::vector<MaterialXLogEntry> MaterialXLog::warnings() const {
    std::vector<MaterialXLogEntry> result;
    for (const auto& entry : entries) {
        if (entry.severity == "warning") result.push_back(entry);
    }
    return result;
}

void MaterialXLog::add(const MaterialXLogCodes::Code& code, std::string message) {
    // const entry = { code: code.label, severity: code.severity, message }
    MaterialXLogEntry entry;
    entry.code = code.label;
    entry.severity = code.severity;
    entry.message = std::move(message);
    entries.push_back(std::move(entry));
}

void MaterialXLog::add(const MaterialXLogCodes::Code& code, std::string message,
                       std::string nodeName) {
    // if ( nodeName !== undefined && nodeName !== null ) entry.nodeName = nodeName
    add(code, std::move(message));
    entries.back().nodeName = std::move(nodeName);
}
