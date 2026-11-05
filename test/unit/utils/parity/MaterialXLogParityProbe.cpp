#include "loaders/materialx/MaterialXLog.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>

namespace {
JSValue entryToValue(const MaterialXLogEntry& entry) {
    auto value = JSValue::object(nullptr);
    value.set("code", JSValue(entry.code));
    value.set("severity", JSValue(entry.severity));
    value.set("message", JSValue(entry.message));
    value.set("hasNodeName", JSValue(entry.nodeName.has_value()));
    if (entry.nodeName.has_value()) value.set("nodeName", JSValue(*entry.nodeName));
    return value;
}

JSValue entriesToArray(const std::vector<MaterialXLogEntry>& entries) {
    auto array = JSValue::array();
    std::size_t index = 0;
    for (const auto& entry : entries) array.set(index++, entryToValue(entry));
    return array;
}
}  // namespace

int main() {
    try {
        MaterialXLog log;
        const auto emptyErrors = log.errors().size();
        const auto emptyWarnings = log.warnings().size();

        log.add(MaterialXLogCodes::UNSUPPORTED_NODE, "node is unsupported");
        log.add(MaterialXLogCodes::IGNORED_SURFACE_INPUT, "surface input ignored", "surfaceShader");
        log.add(MaterialXLogCodes::TYPE_MISMATCH, "type mismatch");
        log.add(MaterialXLogCodes::MISSING_REFERENCE, "reference missing");
        log.add(MaterialXLogCodes::TEXTURE_LOAD_FAILED, "texture failed", "imageNode");

        auto codes = JSValue::object(nullptr);
        for (const auto& [name, code] : {std::pair<const char*, const MaterialXLogCodes::Code>{
                                             "UNSUPPORTED_NODE", MaterialXLogCodes::UNSUPPORTED_NODE},
                                         {"IGNORED_SURFACE_INPUT", MaterialXLogCodes::IGNORED_SURFACE_INPUT},
                                         {"MISSING_REFERENCE", MaterialXLogCodes::MISSING_REFERENCE},
                                         {"MISSING_MATERIAL", MaterialXLogCodes::MISSING_MATERIAL},
                                         {"INVALID_VALUE", MaterialXLogCodes::INVALID_VALUE},
                                         {"TEXTURE_LOAD_FAILED", MaterialXLogCodes::TEXTURE_LOAD_FAILED},
                                         {"UNKNOWN_INPUT", MaterialXLogCodes::UNKNOWN_INPUT},
                                         {"INVALID_OUTPUT_CONNECTION",
                                          MaterialXLogCodes::INVALID_OUTPUT_CONNECTION},
                                         {"TYPE_MISMATCH", MaterialXLogCodes::TYPE_MISMATCH}}) {
            auto value = JSValue::object(nullptr);
            value.set("label", JSValue(code.label));
            value.set("severity", JSValue(code.severity));
            codes.set(name, value);
        }

        auto output = JSValue::object(nullptr);
        output.set("emptyErrors", JSValue(static_cast<double>(emptyErrors)));
        output.set("emptyWarnings", JSValue(static_cast<double>(emptyWarnings)));
        output.set("codes", codes);
        output.set("entryCount", JSValue(static_cast<double>(log.entries.size())));
        output.set("entries", entriesToArray(log.entries));
        output.set("errors", entriesToArray(log.errors()));
        output.set("warnings", entriesToArray(log.warnings()));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
