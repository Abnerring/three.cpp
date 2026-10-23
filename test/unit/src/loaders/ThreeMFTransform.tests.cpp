#include "loaders/ThreeMFLoader.h"

#include "catch.hpp"

#include <cmath>
#include <string>
#include <vector>

TEST_CASE("ThreeMFLoader reads the twelve transform numbers") {
    // Twelve numbers separated by single spaces, as the format writes them.
    const auto numbers = ThreeMFLoader::transformNumbers("1 2 3 4 5 6 7 8 9 10 11 12");
    REQUIRE(numbers.size() == 12);
    REQUIRE(numbers[0] == 1);
    REQUIRE(numbers[1] == 2);
    REQUIRE(numbers[11] == 12);
    // The count the format requires is twelve.
    REQUIRE(ThreeMFLoader::kTransformNumberCount == 12);
    // Fewer are accepted as written; nothing pads them here.
    REQUIRE(ThreeMFLoader::transformNumbers("1 2 3").size() == 3);
    // Fractions survive.
    const auto fractional = ThreeMFLoader::transformNumbers("0.5 0.25 0 0 0 0 0 0 0 0 0 0");
    REQUIRE(fractional[0] == 0.5);
    REQUIRE(fractional[1] == 0.25);
}

TEST_CASE("ThreeMFLoader splits on one space only") {
    // Two spaces in a row give an empty piece, which reads as not a number rather than being
    // skipped. That is what splitting on a single space does.
    const auto doubled = ThreeMFLoader::transformNumbers("1  2");
    REQUIRE(doubled.size() == 3);
    REQUIRE(doubled[0] == 1);
    REQUIRE(std::isnan(doubled[1]));
    REQUIRE(doubled[2] == 2);
    // A leading or trailing space does the same at the end.
    REQUIRE(std::isnan(ThreeMFLoader::transformNumbers(" 1")[0]));
    REQUIRE(std::isnan(ThreeMFLoader::transformNumbers("1 ")[1]));
    // A value that is not a number at all is also not a number.
    REQUIRE(std::isnan(ThreeMFLoader::transformNumbers("x")[0]));
}

TEST_CASE("ThreeMFLoader transposes the twelve into the matrix") {
    // The document writes its numbers column by column while the matrix takes them row by row,
    // so the first row is the first, fourth, seventh and tenth.
    const auto matrix = ThreeMFLoader::transformMatrix("1 2 3 4 5 6 7 8 9 10 11 12");
    REQUIRE(matrix.size() == 16);
    REQUIRE(matrix[0] == 1);
    REQUIRE(matrix[1] == 4);
    REQUIRE(matrix[2] == 7);
    REQUIRE(matrix[3] == 10);
    // The second row continues from the second number.
    REQUIRE(matrix[4] == 2);
    REQUIRE(matrix[5] == 5);
    REQUIRE(matrix[8] == 3);
    REQUIRE(matrix[11] == 12);
    // The last row is always the constant one.
    REQUIRE(matrix[12] == 0);
    REQUIRE(matrix[13] == 0);
    REQUIRE(matrix[14] == 0);
    REQUIRE(matrix[15] == 1);
}

TEST_CASE("ThreeMFLoader leaves a short transform full of holes") {
    // Fewer than twelve numbers leave the missing ones not a number, which is what reading past
    // the end of an array does upstream rather than giving zeroes.
    const auto matrix = ThreeMFLoader::transformMatrix("1 2 3");
    REQUIRE(matrix.size() == 16);
    REQUIRE(matrix[0] == 1);
    // A not a number never equals itself, so it is asked about rather than compared.
    REQUIRE(std::isnan(matrix[1]));
    REQUIRE(std::isnan(matrix[7]));
    // The constant last row is still filled in, since it does not come from the document.
    REQUIRE(matrix[15] == 1);
    // An identity written out is the identity, however it is transposed.
    const auto identity = ThreeMFLoader::transformMatrix("1 0 0 0 1 0 0 0 1 0 0 0");
    REQUIRE(identity[0] == 1);
    REQUIRE(identity[5] == 1);
    REQUIRE(identity[10] == 1);
    REQUIRE(identity[15] == 1);
    REQUIRE(identity[1] == 0);
}
