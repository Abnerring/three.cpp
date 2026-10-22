#include "loaders/VOXLoader.h"

#include <stdexcept>

VOXLoader::Matrix VOXLoader::rotationMatrix(std::uint8_t byte) {
    // The low two bits give the column of the first row and the next two the second row.
    const int index1 = byte & 0x3;
    const int index2 = (byte >> 2) & 0x3;
    // The next three bits are the signs of each row.
    const double sign1 = ((byte >> 4) & 0x1) ? -1.0 : 1.0;
    const double sign2 = ((byte >> 5) & 0x1) ? -1.0 : 1.0;
    const double sign3 = ((byte >> 6) & 0x1) ? -1.0 : 1.0;

    if (index1 == index2)
        throw std::runtime_error("THREE.VOXLoader: Invalid rotation encoding");
    // Each row has one non zero entry, so the third row takes the column the others did not.
    const int index3 = 3 - index1 - index2;

    double r[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    r[0][index1] = sign1;
    r[1][index2] = sign2;
    r[2][index3] = sign3;

    // Change of basis from the VOX frame, where Z is up, to the port's Y up frame: the new
    // y is the old z and the new z is the negated old y.
    return {r[0][0], r[0][2], -r[0][1], 0.0,
            r[2][0], r[2][2], -r[2][1], 0.0,
            -r[1][0], -r[1][2], r[1][1], 0.0,
            0.0, 0.0, 0.0, 1.0};
}
