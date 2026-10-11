#pragma once
#include <cstdint>

class Layers {
  public:
    std::uint32_t mask = 1;
    void set(int layer) { mask = bit(layer); }
    void enable(int layer) { mask |= bit(layer); }
    void enableAll() { mask = 0xffffffffu; }
    void toggle(int layer) { mask ^= bit(layer); }
    void disable(int layer) { mask &= ~bit(layer); }
    void disableAll() { mask = 0; }
    bool test(const Layers& layers) const { return (mask & layers.mask) != 0; }
    bool isEnabled(int layer) const { return (mask & bit(layer)) != 0; }

  private:
    // JavaScript shifts mask the shift count to five bits, including negative counts.
    static std::uint32_t bit(int layer) {
        return std::uint32_t{1} << (static_cast<std::uint32_t>(layer) & 31u);
    }
};
