#include "core/JSValue.h"
#include "core/NativeObject.h"
#include "textures/Texture.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
    constexpr unsigned iterations = 10000, repetitions = 5;
    auto realm = JSValue::object();
    auto texture = nativeTexture(std::make_shared<Texture>());
    JSWeakValue weak(texture);
    std::vector<JSValue> ordinary;
    for (const unsigned records : {0u, 1000u, 10000u}) {
        while (ordinary.size() < records) ordinary.push_back(JSValue::object());
        for (const bool inside : {false, true}) {
            std::array<double, repetitions> samples{};
            for (auto& sample : samples) {
                unsigned live = 0;
                std::function<void()> loop = [&] {
                    for (unsigned i = 0; i < iterations; ++i)
                        if (weak.lock()) ++live;
                };
                JSValue callback;
                if (inside) {
                    auto capture = JSValue::hostObject(
                        std::shared_ptr<void>(&loop, +[](void*) {}), &loop, "test.PerfLoop");
                    callback = JSValue::function(
                        [](const JSValue&, const std::vector<JSValue>&,
                           const std::vector<JSValue>& captures) {
                            auto function = static_cast<std::function<void()>*>(
                                captures[0].hostPayload("test.PerfLoop").get());
                            (*function)();
                            return JSValue{};
                        }, {capture});
                }
                const auto begin = std::chrono::steady_clock::now();
                if (inside) callback.call(); else loop();
                sample = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - begin).count();
                if (live != iterations) throw std::runtime_error("weak cache lost live native resource");
            }
            const auto original = samples;
            std::sort(samples.begin(), samples.end());
            std::cout << (inside ? "native_call" : "host_access")
                      << " ordinary_records=" << records
                      << " iterations=" << iterations
                      << " repetitions=" << repetitions
                      << " median_ms=" << samples[repetitions / 2] << " samples_ms=";
            for (const auto sample : original) std::cout << sample << ',';
            std::cout << '\n';
        }
    }
}
