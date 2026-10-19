#include "audio/AudioAnalyser.h"

#include "core/NativeObject.h"
#include "core/TypedArray.h"
#include "core/JSValue.h"

AudioAnalyser::AudioAnalyser(JSValue audio, double fftSize) {
    object_ = JSValue::object();
    const auto context = audio.get("context");
    auto analyser = context.get("createAnalyser").call(context, {});
    analyser.set("fftSize", JSValue(fftSize));
    const auto bins = analyser.get("frequencyBinCount").toNumber();
    // The data array is allocated from the bin count and then kept for every read, as upstream does.
    auto data = nativeArray(Uint8Array(std::vector<std::uint8_t>(static_cast<std::size_t>(bins), 0)));
    audio.get("getOutput").call(audio, {}).get("connect").call(audio.get("getOutput").call(audio, {}), {analyser});
    object_.set("analyser", analyser);
    object_.set("data", data);
}

JSValue AudioAnalyser::getFrequencyData() {
    auto analyser = object_.get("analyser");
    auto data = object_.get("data");
    analyser.get("getByteFrequencyData").call(analyser, {data});
    return data;
}

double AudioAnalyser::getAverageFrequency() {
    const auto data = getFrequencyData();
    double value = 0.0;
    // A native typed array reports its length as a property, not through the runtime array accessor.
    const auto length = data.get("length").toNumber();
    for (double i = 0; i < length; i += 1) value += data.get(i).toNumber();
    return length > 0 ? value / length : 0.0;
}