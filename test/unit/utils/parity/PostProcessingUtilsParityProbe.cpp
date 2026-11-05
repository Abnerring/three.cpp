#include "nodes/utils/PostProcessingUtils.h"

#include "core/JSValue.h"

#include <iostream>

int main() {
    try {
        const auto module = nativePostProcessingUtilsModule();
        auto output = JSValue::object(nullptr);
        auto names = JSValue::array();
        auto kinds = JSValue::object(nullptr);
        for (const char* name : {"isBackgroundDepth", "getViewPosition", "getScreenPosition",
                                 "getScreenPositionFromClip", "getNormalFromDepth",
                                 "interleavedGradientNoise", "vogelDiskSample"}) {
            const auto value = module.get(name);
            names.set(names.length(), JSValue(name));
            kinds.set(name, JSValue(value.isCallable() ? "function" : "other"));
        }
        output.set("names", names);
        output.set("kinds", kinds);
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
