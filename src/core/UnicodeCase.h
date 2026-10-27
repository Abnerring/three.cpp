#pragma once
#include <string>
#include <string_view>
// Unicode default full uppercase; no locale tailoring or normalization.
// UTF-16 surrogate pairs are decoded and lone surrogates are preserved.
std::u16string unicodeToUpperCase(std::u16string_view input);
// Unicode default full lowercase, including the Final_Sigma context rule.
std::u16string unicodeToLowerCase(std::u16string_view input);
