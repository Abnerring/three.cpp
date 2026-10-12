#include "math/JSNumberMath.h"
#include "animation/PropertyMixer.h"
#include <array>
#include <cmath>

namespace {
void requireLoop(double start, double end) {
    // Upstream uses i !== end. Reject only an entered loop that cannot reach it.
    if (start != end && (!std::isfinite(start) || !std::isfinite(end) || end < start ||
                        std::trunc(end - start) != end - start || end > 9007199254740991.))
        throw JSRangeError("PropertyMixer loop cannot reach its endpoint");
}
void mix(PropertyMixer& self, PropertyMixer::MixFunction fn, AnimationArray buffer,
         double destination, double source, JSValue weight, double size) {
    if (!fn) throw JSTypeError("PropertyMixer mix function is not callable");
    fn(self, std::move(buffer), destination, source, std::move(weight), size);
}
void identity(PropertyMixer& self, PropertyMixer::IdentityFunction fn) {
    if (!fn) throw JSTypeError("PropertyMixer identity function is not callable");
    fn(self);
}
void requireBinding(const std::shared_ptr<PropertyBindingInterface>& binding) {
    if (!binding) throw JSTypeError("Cannot read properties of null property binding");
}
// Flat quaternion operations preserve the upstream reads, coercions and writes.
// Using Quaternion's native vector<double> overload here would lose ordinary
// array accessors, undefined values, and shared-view aliasing.
void slerpFlat(AnimationArray buffer, double dst, double a, double b, JSValue t) {
    std::array<JSValue, 4> first{buffer.get(a), buffer.get(a + 1), buffer.get(a + 2), buffer.get(a + 3)};
    std::array<JSValue, 4> second{buffer.get(b), buffer.get(b + 1), buffer.get(b + 2), buffer.get(b + 3)};
    if (!first[3].strictEquals(second[3]) || !first[0].strictEquals(second[0]) ||
        !first[1].strictEquals(second[1]) || !first[2].strictEquals(second[2])) {
        auto dot = Interpolant_detail::multiply(first[0], second[0]);
        dot += Interpolant_detail::multiply(first[1], second[1]);
        dot += Interpolant_detail::multiply(first[2], second[2]);
        dot += Interpolant_detail::multiply(first[3], second[3]);
        if (dot < 0) { for (auto& component : second) component = -component.toNumber(); dot = -dot; }
        auto s = 1 - t.toNumber();
        const bool spherical = dot < .9995;
        if (spherical) {
            const auto theta = JSNumberMath::acos(dot), sine = JSNumberMath::sin(theta);
            s = JSNumberMath::sin(s * theta) / sine;
            t = JSNumberMath::sin(t.toNumber() * theta) / sine;
        }
        for (std::size_t i = 0; i < 4; ++i) {
            const auto left = Interpolant_detail::multiply(first[i], s);
            const auto right = Interpolant_detail::multiply(second[i], t);
            first[i] = left + right;
        }
        if (!spherical) {
            const auto x = first[0].number(), y = first[1].number(), z = first[2].number(), w = first[3].number();
            const auto f = 1 / std::sqrt(x * x + y * y + z * z + w * w);
            for (auto& component : first) component = component.number() * f;
        }
    }
    for (std::size_t i = 0; i < 4; ++i) buffer.set(dst + double(i), first[i]);
}
void multiplyFlat(AnimationArray buffer, double dst, double a, double b) {
    const std::array<JSValue, 4> first{buffer.get(a), buffer.get(a + 1), buffer.get(a + 2), buffer.get(a + 3)};
    const std::array<JSValue, 4> second{buffer.get(b), buffer.get(b + 1), buffer.get(b + 2), buffer.get(b + 3)};
    const auto mul = [&](int x, int y) { return Interpolant_detail::multiply(first[x], second[y]); };
    auto x = mul(0, 3); x += mul(3, 0); x += mul(1, 2); x -= mul(2, 1); buffer.set(dst, x);
    auto y = mul(1, 3); y += mul(3, 1); y += mul(2, 0); y -= mul(0, 2); buffer.set(dst + 1, y);
    auto z = mul(2, 3); z += mul(3, 2); z += mul(0, 1); z -= mul(1, 0); buffer.set(dst + 2, z);
    auto w = mul(3, 3); w -= mul(0, 0); w -= mul(1, 1); w -= mul(2, 2); buffer.set(dst + 3, w);
}
}

PropertyMixer::PropertyMixer(std::shared_ptr<PropertyBindingInterface> target, JSValue type, double size)
    : binding(std::move(target)), valueSize(size) {
    static const MixFunction slerp = [](PropertyMixer& m, Array b, double d, double s, JSValue t, double) { m._slerp(b, d, s, t); };
    static const MixFunction slerpAdditive = [](PropertyMixer& m, Array b, double d, double s, JSValue t, double n) { m._slerpAdditive(b, d, s, t, n); };
    static const MixFunction select = [](PropertyMixer& m, Array b, double d, double s, JSValue t, double n) { m._select(b, d, s, t, n); };
    static const MixFunction lerp = [](PropertyMixer& m, Array b, double d, double s, JSValue t, double n) { m._lerp(b, d, s, t, n); };
    static const MixFunction lerpAdditive = [](PropertyMixer& m, Array b, double d, double s, JSValue t, double n) { m._lerpAdditive(b, d, s, t, n); };
    static const IdentityFunction quaternionIdentity = [](PropertyMixer& m) { m._setAdditiveIdentityQuaternion(); };
    static const IdentityFunction otherIdentity = [](PropertyMixer& m) { m._setAdditiveIdentityOther(); };
    static const IdentityFunction numericIdentity = [](PropertyMixer& m) { m._setAdditiveIdentityNumeric(); };
    if (type.strictEquals("quaternion")) {
        buffer = Float64Array(size * 6); _workIndex = 5;
        _mixBufferRegion = slerp.named("_slerp");
        _mixBufferRegionAdditive = slerpAdditive.named("_slerpAdditive");
        _setIdentity = quaternionIdentity.named("_setAdditiveIdentityQuaternion");
    } else if (type.strictEquals("string") || type.strictEquals("bool")) {
        const auto n = size * 5;
        if (!std::isfinite(n) || n < 0 || n > UINT32_MAX || std::trunc(n) != n) throw JSRangeError("Invalid array length");
        buffer = JSValue::array(static_cast<std::uint32_t>(n));
        _mixBufferRegion = _mixBufferRegionAdditive = select.named("_select");
        _setIdentity = otherIdentity.named("_setAdditiveIdentityOther");
    } else {
        buffer = Float64Array(size * 5);
        _mixBufferRegion = lerp.named("_lerp");
        _mixBufferRegionAdditive = lerpAdditive.named("_lerpAdditive");
        _setIdentity = numericIdentity.named("_setAdditiveIdentityNumeric");
    }
}
void PropertyMixer::accumulate(JSValue accuIndex, JSValue weight) {
    auto target = buffer; const auto stride = valueSize;
    const auto offset = Interpolant_detail::multiply(accuIndex, stride) + stride;
    auto current = cumulativeWeight;
    if (current.strictEquals(0)) {
        requireLoop(0, stride);
        for (double i = 0; i != stride; ++i) target.set(offset + i, target.get(i));
        current = weight;
    } else {
        current = Interpolant_detail::add(current, weight);
        const auto numerator = weight.toNumber(), denominator = current.toNumber();
        mix(*this, _mixBufferRegion, target, offset, 0, numerator / denominator, stride);
    }
    cumulativeWeight = current;
}
void PropertyMixer::accumulateAdditive(JSValue weight) {
    auto target = buffer; const auto stride = valueSize, offset = stride * _addIndex;
    if (cumulativeWeightAdditive.strictEquals(0)) identity(*this, _setIdentity);
    mix(*this, _mixBufferRegionAdditive, target, offset, 0, weight, stride);
    const auto current = cumulativeWeightAdditive;
    cumulativeWeightAdditive = Interpolant_detail::add(current, weight);
}
void PropertyMixer::apply(JSValue accuIndex) {
    const auto stride = valueSize; auto target = buffer;
    const auto offset = Interpolant_detail::multiply(accuIndex, stride) + stride;
    const auto weight = cumulativeWeight, additive = cumulativeWeightAdditive;
    const auto receiver = binding;
    cumulativeWeight = 0; cumulativeWeightAdditive = 0;
    if (weight.toNumber() < 1) {
        const auto original = stride * _origIndex;
        // Retrieve the selected callback before coercing its argument.
        const auto callback = _mixBufferRegion;
        const auto amount = 1 - weight.toNumber();
        mix(*this, callback, target, offset, original, amount, stride);
    }
    if (additive.toNumber() > 0)
        mix(*this, _mixBufferRegionAdditive, target, offset, _addIndex * stride, 1, stride);
    const auto end = stride + stride;
    for (double i = stride; i != end; ++i) {
        const auto left = target.get(i), right = target.get(i + stride);
        if (!left.strictEquals(right)) { requireBinding(receiver); receiver->setValue(target, offset); break; }
        // A fractional or negative stride can still terminate at the first
        // differing entry. Do not reject it before that observable comparison.
        if (!std::isfinite(i) || i + 1 == i)
            throw JSRangeError("PropertyMixer comparison loop cannot progress");
    }
}
void PropertyMixer::saveOriginalState() {
    const auto receiver = binding; auto target = buffer;
    const auto stride = valueSize, original = stride * _origIndex;
    requireBinding(receiver); receiver->getValue(target, original);
    requireLoop(stride, original);
    for (double i = stride; i != original; ++i) target.set(i, target.get(original + std::fmod(i, stride)));
    identity(*this, _setIdentity);
    cumulativeWeight = 0; cumulativeWeightAdditive = 0;
}
void PropertyMixer::restoreOriginalState() {
    const auto offset = valueSize * 3;
    const auto receiver = binding; const auto target = buffer;
    requireBinding(receiver); receiver->setValue(target, offset);
}
void PropertyMixer::_setAdditiveIdentityNumeric() {
    const auto start = _addIndex * valueSize, end = start + valueSize;
    for (double i = start; i < end; ++i) {
        if (!std::isfinite(i) || !std::isfinite(end) || i >= 9007199254740991.) throw JSRangeError("PropertyMixer identity loop cannot progress");
        auto target = buffer; target.set(i, 0);
    }
}
void PropertyMixer::_setAdditiveIdentityQuaternion() {
    _setAdditiveIdentityNumeric();
    auto target = buffer; const auto offset = _addIndex * valueSize + 3;
    target.set(offset, 1);
}
void PropertyMixer::_setAdditiveIdentityOther() {
    const auto start = _origIndex * valueSize, destination = _addIndex * valueSize;
    for (double i = 0; i < valueSize; ++i) {
        if (i >= 9007199254740991.) throw JSRangeError("PropertyMixer identity loop cannot progress");
        auto target = buffer;
        const auto source = buffer;
        target.set(destination + i, source.get(start + i));
    }
}
void PropertyMixer::_select(Array target, double destination, double source, JSValue t, double stride) {
    if (t.toNumber() >= .5) {
        requireLoop(0, stride);
        for (double i = 0; i != stride; ++i) target.set(destination + i, target.get(source + i));
    }
}
void PropertyMixer::_slerp(Array target, double destination, double source, JSValue t) {
    slerpFlat(std::move(target), destination, destination, source, std::move(t));
}
void PropertyMixer::_slerpAdditive(Array target, double destination, double source, JSValue t, double stride) {
    const auto work = _workIndex.toNumber() * stride;
    multiplyFlat(target, work, destination, source);
    slerpFlat(target, destination, destination, work, std::move(t));
}
void PropertyMixer::_lerp(Array target, double destination, double source, JSValue t, double stride) {
    const auto s = 1 - t.toNumber();
    requireLoop(0, stride);
    for (double i = 0; i != stride; ++i) {
        const auto j = destination + i;
        const auto left = Interpolant_detail::multiply(target.get(j), s);
        const auto right = Interpolant_detail::multiply(target.get(source + i), t);
        target.set(j, left + right);
    }
}
void PropertyMixer::_lerpAdditive(Array target, double destination, double source, JSValue t, double stride) {
    requireLoop(0, stride);
    for (double i = 0; i != stride; ++i) {
        const auto j = destination + i;
        const auto left = target.get(j), right = target.get(source + i);
        const auto product = Interpolant_detail::multiply(right, t);
        target.set(j, Interpolant_detail::add(left, product));
    }
}
