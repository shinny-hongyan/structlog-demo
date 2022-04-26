#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

#include "structlog/fastbuffer.h"

namespace structlog
{

void StringFmt(FastBuffer& buf, const char* s, std::size_t n);
void StringFmt(FastBuffer& buf, const char* s);
void StringFmt(FastBuffer& buf, const std::string& s);

}  // namespace structlog
