#pragma once
// copied from golang "shinnytech.com/easyfuture/lib/utility/number.go"
#include <cstdint>

#include "structlog/fastbuffer.h"

namespace structlog
{

void Int64Fmt(FastBuffer& buf, int64_t v);
void Uint64Fmt(FastBuffer& buf, uint64_t v);
// v must be in range of int64_t
void DoubleFmt(FastBuffer& buf, double v, uint8_t p, bool trim);

char* IntegerFmt(char* eob, uint64_t v, bool neg);

}  // namespace structlog
