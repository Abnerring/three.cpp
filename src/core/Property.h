#pragma once
#include <functional>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

// Preserve the callback's mutable state and keep it alive if it replaces itself.
inline std::function<void()> makeChangeCallback(std::function<void()> callback) {
    auto target = std::make_shared<std::function<void()>>(
        callback ? std::move(callback) : std::function<void()>([] {}));
    return [target = std::move(target)] { (*target)(); };
}
inline void notifyPropertyChange(const std::function<void()>& onChange) {
    const auto callback = onChange;
    callback();
}

// One native registration has one persistent callable snapshot. Managed adapters
// may observe registrations and dispatch through their current property using
// weak graph handles; this record never requires a global object registry.
using PropertyChangeSnapshot = std::shared_ptr<std::function<void()>>;
struct PropertyChangeRecord {
    PropertyChangeSnapshot callback;
    bool registered = false;
    std::uint64_t registrationVersion = 0;
    std::function<bool()> managedNotify;
    std::function<void(const PropertyChangeSnapshot&)> managedRegister;
};
struct PropertyChangeDispatcher {
    std::shared_ptr<PropertyChangeRecord> record;
    void operator()() const {
        const auto managed=record->managedNotify;
        if(managed&&managed())return;
        const auto snapshot=record->callback;
        (*snapshot)();
    }
};
inline std::shared_ptr<PropertyChangeRecord> propertyChangeRecord(std::function<void()>& callback) {
    if(const auto dispatcher=callback.target<PropertyChangeDispatcher>())return dispatcher->record;
    auto record=std::make_shared<PropertyChangeRecord>();
    record->callback=std::make_shared<std::function<void()>>(callback?callback:std::function<void()>([]{}));
    callback=PropertyChangeDispatcher{record};
    return record;
}
inline void registerPropertyChangeCallback(std::function<void()>& current,std::function<void()> callback) {
    const auto record=propertyChangeRecord(current);
    const auto snapshot=std::make_shared<std::function<void()>>(makeChangeCallback(std::move(callback)));
    const auto previous=record->callback;const bool previouslyRegistered=record->registered;
    const auto observer=record->managedRegister;
    record->callback=snapshot;record->registered=true;const auto version=++record->registrationVersion;
    try {if(observer)observer(snapshot);}
    catch(...) {
        // Preserve nested registrations completed during an observable setter.
        if(record->registrationVersion==version) {
            record->callback=previous;record->registered=previouslyRegistered;
            ++record->registrationVersion;
        }
        throw;
    }
}

// A C++ accessor property. Bound instances never transfer their binding on assignment.
// Copying a property itself produces an independent value snapshot, without callbacks.
template <class T> class Property {
  public:
    explicit Property(T& value) noexcept : value_(&value) {}
    Property(T& value, std::function<void()>& onChange) noexcept
        : value_(&value), onChange_(&onChange) {}
    Property(const Property& source) : snapshot_(source.value()), value_(&snapshot_) {}
    Property(Property&& source) : Property(static_cast<const Property&>(source)) {}

    operator T() const { return value(); }
    T value() const { return *value_; }
    T operator=(T next) {
        *value_ = next;
        if (onChange_) notifyPropertyChange(*onChange_);
        return next;
    }
    T operator=(const Property& source) { return *this = source.value(); }
    T operator=(Property&& source) { return *this = source.value(); }
    T operator+=(T amount) requires std::is_arithmetic_v<T> {
        return *this = value() + amount;
    }
    T operator-=(T amount) requires std::is_arithmetic_v<T> {
        return *this = value() - amount;
    }
    T operator*=(T amount) requires std::is_arithmetic_v<T> {
        return *this = value() * amount;
    }
    T operator/=(T amount) requires std::is_arithmetic_v<T> {
        return *this = value() / amount;
    }
    T operator++() requires std::is_arithmetic_v<T> { return *this += T(1); }
    T operator++(int) requires std::is_arithmetic_v<T> { const T previous = value(); ++*this; return previous; }
    T operator--() requires std::is_arithmetic_v<T> { return *this -= T(1); }
    T operator--(int) requires std::is_arithmetic_v<T> { const T previous = value(); --*this; return previous; }

  private:
    T snapshot_{};
    T* value_;
    std::function<void()>* onChange_ = nullptr;
};
