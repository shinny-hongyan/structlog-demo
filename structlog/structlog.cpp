#include "structlog/structlog.h"

#include <mutex>
#include <chrono>
#include <algorithm>
#include <iostream>
#include "structlog/number.h"

namespace structlog {

static std::mutex g_structlog_lock;
static std::ostream* g_structlog_out_stream = &std::cerr;
static LogLevel g_structlog_out_level = LogLevel::Info;

structlog::Logger& Logger::Root() {
  static Logger root_logger(&g_structlog_lock, &g_structlog_out_stream, &g_structlog_out_level);
  return root_logger;
}

// NRVO
Logger Logger::Clone() {
  Logger l(fields_, m_lock, m_out_stream, m_out_level);
  fields_.shrink(fields_.size() - index_);
  return l;
}

Logger::Logger(std::mutex* _lock, std::ostream** _out_stream, LogLevel* _out_level)
  : index_(1)
  , m_lock(_lock)
  , m_out_stream(_out_stream)
  , m_out_level(_out_level) {
  auto bg = FastBufferGuard(fields_, 256);
  bg.append('{');
}

Logger::Logger(const FastBuffer& fields, std::mutex* _lock, std::ostream** _out_stream, LogLevel* _out_level)
  : fields_(fields)
  , index_(fields_.size())
  , m_lock(_lock)
  , m_out_stream(_out_stream)
  , m_out_level(_out_level) {}

template <>
void Logger::Append(const int64_t& v) {
  Int64Fmt(fields_, v);
}

template <>
void Logger::Append(const int& v) {
  Int64Fmt(fields_, static_cast<int64_t>(v));
}

template <>
void Logger::Append(const short& v) {
  Int64Fmt(fields_, static_cast<int64_t>(v));
}

template <>
void Logger::Append(const double& v) {
  DoubleFmt(fields_, v, 12, true);
}

template <>
void Logger::Append(const bool& v) {
  auto bg = FastBufferGuard(fields_, 5);
  if (v)
    bg.append("true");
  else
    bg.append("false");
}

template <>
void Logger::Append(const std::string& v) {
  StringFmt(fields_, v);
}

template <>
void Logger::Append(const char& v) {
  StringFmt(fields_, &v, 1);
}

template <>
void Logger::Append(const JsonRawMessage<std::string>& v) {
  auto bg = FastBufferGuard(fields_, v.raw_message_.size());
  // all \n are whitespace, because \n need to be escaped in string
  auto dst = std::copy_if(v.raw_message_.begin(), v.raw_message_.end(), bg.data(), [](const char c) {
    return c != '\n';
  });
  bg.consume(dst - bg.data());
}

template <>
void Logger::Append(const JsonRawMessage<const char*>& v) {
  const std::size_t block_size = 128;
  auto bg = FastBufferGuard(fields_, block_size);
  const char* s = v.raw_message_;
  char c = *s;
  while (c) {
    // deal one block at a time
    std::size_t i = 0;
    char* data = bg.data();
    for (; i < block_size && (c = *s++); i++) {
      if (c != '\n')
        *data++ = c;
    }
    std::size_t more = data - bg.data();
    bg.consume(more);
    bg.reserve(more);
  }
}

template <>
void Logger::Append(const JsonRawMessage<char*>& v) {
  Append(JsonRawMessage<const char*>(v.raw_message_));
}

template <>
void Logger::Append(const char* const& v) {
  StringFmt(fields_, v);
}

template <>
void Logger::Append(const std::chrono::time_point<std::chrono::system_clock>& v) {
  // C++ standard doesn't have time zone support until C++2a
  // std::localtime from <ctime> is too slow to be useful because it has to read&parse timezone file every time and may
  // not be thread safe. localtime_r from <time.h> is thread safe and able to cache timezone info between invocation,
  // but it requires POSIX which isn't universal. so we roll our own, and always output in UTC+8
  static thread_local uint64_t second_begin = 0, second_end = 0;  // [second_begin, second_end)
  static thread_local char second_str[21];                        // p1:"1984-04-01T02:34:56. p2:999999999 p3:+08:00"
  uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(v.time_since_epoch()).count();
  if (now < second_begin || second_end <= now) {
    static constexpr const char int_digits[] =
      "0001020304050607080910111213141516171819202122232425262728293031323334353637383940414243444546474849505152535455"
      "56575859";
    second_str[0] = '"';
    auto t = now / 1000000000;  // seconds
    second_begin = now - now % 1000000000;
    second_end = second_begin + 1000000000;
    auto second = t % 60;
    t /= 60;  // minutes
    auto minute = t % 60;
    t = t / 60 + 8;  // hours, add 8 hour timezone
    auto hour = t % 24;
    t /= 24;  // days
    // ref: https://howardhinnant.github.io/date_algorithms.html#civil_from_days
    uint32_t const z = uint32_t(t + 719468);  // shift the epoch from 1970-01-01 to 0000-03-01
    auto const era = z / 146097;
    auto const doe = z % 146097;                                             // [0, 146096]
    auto const yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
    auto const y = yoe + era * 400;
    auto const doy = doe - (365 * yoe + yoe / 4 - yoe / 100);  // [0, 365]
    auto const mp = (5 * doy + 2) / 153;                       // [0, 11]
    auto const d = doy - (153 * mp + 2) / 5 + 1;               // [1, 31]
    auto const m = mp < 10 ? mp + 3 : mp - 9;                  // [1, 12]
    IntegerFmt(second_str + 5, y + (m <= 2), false);
    second_str[5] = '-';
    second_str[6] = int_digits[m * 2];
    second_str[7] = int_digits[m * 2 + 1];
    second_str[8] = '-';
    second_str[9] = int_digits[d * 2];
    second_str[10] = int_digits[d * 2 + 1];
    second_str[11] = 'T';
    second_str[12] = int_digits[hour * 2];
    second_str[13] = int_digits[hour * 2 + 1];
    second_str[14] = ':';
    second_str[15] = int_digits[minute * 2];
    second_str[16] = int_digits[minute * 2 + 1];
    second_str[17] = ':';
    second_str[18] = int_digits[second * 2];
    second_str[19] = int_digits[second * 2 + 1];
    second_str[20] = '.';
  }
  auto bg = FastBufferGuard(fields_, 48);
  auto data = bg.data();
  data = std::copy_n(second_str, 21, data);
  auto pos = IntegerFmt(data + 9, now - second_begin, false);
  std::fill_n(data, pos - data, '0');
  data = std::copy_n(R"(+08:00")", 7, data + 9);
  bg.consume(data - bg.data());
}

void Logger::Emit(const LogLevel level) {
  With("time", std::chrono::system_clock::now());
  auto bg = FastBufferGuard(fields_, 2);
  fields_.shrink(1);
  bg.append("}\n");
  {
    std::lock_guard<std::mutex> lg(*m_lock);
    if (*m_out_stream && level <= *m_out_level) {
      (*m_out_stream)->write(fields_.get(), fields_.size());
      (*m_out_stream)->flush();
    }
  }
  fields_.shrink(fields_.size() - index_);
}

void SetOutput(std::ostream* out) {
  std::lock_guard<std::mutex> lg(g_structlog_lock);
  g_structlog_out_stream = out;
}

void SetLevel(const LogLevel level) {
  std::lock_guard<std::mutex> lg(g_structlog_lock);
  g_structlog_out_level = level;
}

}  // namespace structlog
