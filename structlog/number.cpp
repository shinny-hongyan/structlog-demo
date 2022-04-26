#include "structlog/number.h"

#include <cmath>

namespace structlog
{

// if neg is true, then v should be uint64_t(s) which s is the signed value
// this function can deal with full range of int64_t/uint64_t, because:
// Section 4.7 conv.integral
// If the destination type is unsigned, the resulting value is the least unsigned integer congruent to the source integer (modulo 2^n where n is
// the number of bits used to represent the unsigned type). [ Note: In a two's complement representation, this conversion is conceptual and there
// is no change in the bit pattern (if there is no truncation).  — end note ]
// so for s < 0, v = s + 2^64
// eob point to end of buffer, in other word *eob is non writable
char* IntegerFmt(char* eob, uint64_t v, bool neg)
{
    static constexpr const char int_digits[] =
        "00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";
    // 5.3.1c7:
    // The negative of an unsigned quantity is computed by subtracting its value from 2^n, where n is the number of bits in the promoted operand.
    // for s < 0, now v = 2^64 - (s + 2^64)
    char* pos = eob;
    if (neg)
        v = 0 - v;
    while (v >= 10) {
        auto index = (v % 100) * 2;
        v /= 100;
        *--pos = int_digits[index + 1];
        *--pos = int_digits[index];
    }
    if (v || pos == eob)
        *--pos = '0' + static_cast<char>(v);
    if (neg)
        *--pos = '-';
    return pos;
}

void Int64Fmt(FastBuffer& buf, int64_t v)
{
    auto bg = FastBufferGuard(buf, 24);
    char buffer[24];
    char* eob = buffer + sizeof(buffer);
    auto pos = IntegerFmt(eob, static_cast<uint64_t>(v), v < 0);
    bg.append(pos, eob - pos);
}

void Uint64Fmt(FastBuffer& buf, uint64_t v)
{
    auto bg = FastBufferGuard(buf, 24);
    char buffer[24];
    char* eob = buffer + sizeof(buffer);
    auto pos = IntegerFmt(eob, v, false);
    bg.append(pos, eob - pos);
}

static double power10[] = {
    1.0,
    10.0,
    100.0,
    1000.0,
    10000.0,
    100000.0,
    1000000.0,
    10000000.0,
    100000000.0,
    1000000000.0,
    10000000000.0,
    100000000000.0,
    1000000000000.0
};

// for 0 <= v < 1
static void SubDoubleFmt(FastBufferGuard& bg, double v, uint8_t p, bool trim)
{
    auto frac = static_cast<uint64_t>(v * power10[p]);
    if (frac == 0 && trim) {
        bg.append('0');
        return;
    }
    auto buffer = bg.data();
    auto pos = IntegerFmt(buffer + p, frac, false);
    if (trim)
        while (*(buffer + p - 1) == '0')
            --p;
    std::fill_n(buffer, pos - buffer, '0');
    bg.consume(p);
}

static double round_double[] = {
    0.5,
    0.05,
    0.005,
    0.0005,
    0.00005,
    0.000005,
    0.0000005,
    0.00000005,
    0.000000005,
    0.0000000005,
    0.00000000005,
    0.000000000005,
    0.0000000000005
};

static double div_double[] = {
    1,
    0.1,
    0.01,
    0.001,
    0.0001,
    0.00001,
    0.000001,
    0.0000001,
    0.00000001,
    0.000000001,
    0.0000000001,
    0.00000000001,
    0.000000000001
};

// v must in range of int64_t or is nan
// p must be in range  [0, 12]
// trim: remove extra trailing zero
void DoubleFmt(FastBuffer& buf, double v, uint8_t p, bool trim)
{
    if (v != v) {
        FastBufferGuard(buf, 8).append(R"("-")");
        return;
    }
    if (v > 0)
        v += round_double[p];
    else
        v -= round_double[p];
    auto frac = static_cast<int64_t>(v);
    if (p == 0) {
        Int64Fmt(buf, frac);
        return;
    }
    auto bg = FastBufferGuard(buf, 24);
    if (frac == 0 && v <= -div_double[p]) {
        bg.append("-0.");
    } else {
        Int64Fmt(buf, frac);
        bg.append('.');
    }
    SubDoubleFmt(bg, fabs(v - static_cast<double>(frac)), p, trim);
}

}  // namespace structlog
