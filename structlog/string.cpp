#include "structlog/string.h"

#include <cstdint>
#include <string>
#include <algorithm>

namespace structlog
{

static const std::string escape_table[] = { "",
                                            R"(\u0000)", R"(\u0001)", R"(\u0002)", R"(\u0003)", R"(\u0004)", R"(\u0005)", R"(\u0006)", R"(\u0007)",
                                            R"(\b)", R"(\t)", R"(\n)", R"(\u000B)", R"(\f)", R"(\r)", R"(\u000E)", R"(\u000F)",
                                            R"(\u0010)", R"(\u0011)", R"(\u0012)", R"(\u0013)", R"(\u0014)", R"(\u0015)", R"(\u0016)", R"(\u0017)",
                                            R"(\u0018)", R"(\u0019)", R"(\u001A)", R"(\u001B)", R"(\u001C)", R"(\u001D)", R"(\u001E)", R"(\u001F)",
                                            R"(\")", R"(\\)",
                                          };

static constexpr const uint8_t escape_flag[256] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    0, 0, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 34, 0, 0, 0,
};

// @todo: optimize for compile time string, skip all the checks and generate the resulting string at compile time
void StringFmt(FastBuffer& buf, const char* s, std::size_t n)
{
    // worst case json string escape
    auto bg = FastBufferGuard(buf, n * 6 + 2);
    char* dst = bg.data();
    *dst++ = '"';
    const char* end = s + n;
    while (s < end) {
        char c = *s++;
        // since most part of string do not need escape, we need a fast way to find out whether escape is needed.
        // we can afford going down the slow path if escape is needed, otherwise we should keep on the fast path.
        if (c == 0)
          break;
        uint8_t flag = escape_flag[static_cast<uint8_t>(c)];
        if (flag) {
            const std::string& esc = escape_table[flag];
            dst = std::copy_n(esc.c_str(), esc.size(), dst);
        } else *dst++ = c;
    }
    *dst++ = '"';
    bg.consume(dst - bg.data());
}

void StringFmt(FastBuffer& buf, const char* s)
{
    auto bgq = FastBufferGuard(buf, 2);  // for quotes
    bgq.append('"');
    const std::size_t block_size = 32;
    // deal one block at a time
    auto bg = FastBufferGuard(buf, block_size * 6);
    char c = *s;
    while (c) {
        char* dst = bg.data();
        for (std::size_t i = 0; i < block_size && (c = *s++); i++) {
            uint8_t flag = escape_flag[static_cast<uint8_t>(c)];
            if (flag) {
                const std::string& esc = escape_table[flag];
                dst = std::copy_n(esc.c_str(), esc.size(), dst);
            } else *dst++ = c;
        }
        std::size_t more = dst - bg.data();
        bg.consume(more);
        bg.reserve(more);
    }
    bgq.append('"');
}

void StringFmt(FastBuffer& buf, const std::string& s)
{
    StringFmt(buf, s.c_str(), s.size());
}

}  // namespace structlog
