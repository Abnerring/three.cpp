#pragma once
#include <array>
#include <cstdint>

// Port of the rotation decoding in examples/jsm/loaders/VOXLoader.js at the pinned revision.
// A VOX transform stores a rotation as a single byte: two bits pick the column of the first
// two rows, three bits give their signs, and the third row takes the remaining column. The
// matrix is then turned from the VOX frame, where Z is up, into the port's Y up frame.
class VOXLoader {
  public:
    // The sixteen components of a column-major rotation matrix, ready for a Matrix4.
    using Matrix = std::array<double, 16>;

    // Decodes one rotation byte. Throws std::runtime_error when the two column indices are
    // equal, which leaves no third column to assign.
    static Matrix rotationMatrix(std::uint8_t byte);
};
