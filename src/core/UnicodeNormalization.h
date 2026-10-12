#pragma once
#include <string>

// Unicode 17.0 NFC, preserving lone UTF-16 surrogates in JSValue's WTF-8 form.
std::string normalizeUnicodeNFC(const std::string& value);
