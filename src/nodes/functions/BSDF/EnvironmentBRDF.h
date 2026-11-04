#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeEnvironmentBRDF();
JSValue nativeEnvironmentBRDFModule();
JSValue nativeEnvironmentBRDFBinding(const std::string& name);
