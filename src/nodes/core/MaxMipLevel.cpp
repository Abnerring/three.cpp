#include "nodes/core/MaxMipLevel.h"

#include <cmath>

double MaxMipLevel::level(double width, double height) {
    // The larger dimension, then the base two logarithm of it.
    const double largest = width > height ? width : height;
    return std::log2(largest);
}

bool MaxMipLevel::usable(bool hasWidth) {
    // Against the absence of a width rather than for its truth, so nought passes.
    return hasWidth;
}

double MaxMipLevel::levelWithZeroWidth(double height) {
    // With a width of nought the larger dimension is the height, so the level follows it.
    return std::log2(height);
}

MaxMipLevel::ImageSource MaxMipLevel::imageSource(bool hasImages, bool hasFirstImage,
                                                  bool firstImageHasImage) {
    // The first of the three places the source can find an image in.
    if (hasImages) {
        if (hasFirstImage && firstImageHasImage) return ImageSource::FirstImageOrItself;
        return ImageSource::FirstImage;
    }
    return ImageSource::TextureImage;
}
