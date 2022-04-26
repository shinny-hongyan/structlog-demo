#pragma once
#include <cstddef>
#include <string>
#include <mutex>

#include "structlog/fastbuffer.h"
#include "structlog/string.h"

/*
该接口设计允许使用者自己增加特殊化 With 模板，例如为了将 class Foo 打入日志可以在某个 cpp 文件中加入以下代码

    template<>
    void Logger::Append(const Foo& v) {
        StringFmt(fields_, v.to_string());
    }

Logger 是有状态的，因此不能跨线程使用，需要使用 Clone 创建一个新 Logger
使用 Clone 创建一个新 logger, 继承了 parent 的字段，但是之后其状态和 parent 完全独立
Clone 时会继承 parent 的上下文字段及临时字段，并将这些字段作为新 logger 的上下文字段, 并将 parent 的的临时字段清空
Logger 内的状态包括从 parent 继承下来的上下文字段和临时的字段
使用 With 函数将一个字段加入临时集合
使用 Panic/Fatal/Error/Warning/Info/Debug 将输出所有字段，并将临时字段清空
*/

namespace structlog {

// 日志等级，在 SetLevel 时用到了该枚举
enum LogLevel { Panic, Fatal, Error, Warning, Info, Debug };

class Logger {
 public:
  ~Logger() {}
  // 返回一个全新的不带上下文信息的 Logger
  // 线程安全
  static Logger& Root();

  // 增加字段到该 logger 中, k 的类型必须是 std::string/const char*/char [N]
  // 不应多次加入相同 k， 如果出现 k 重复，则输出的日志也会有重复 k, 该函数不做去重处理, rational:
  // https://tools.ietf.org/html/rfc8259#section-4
  //   The names within an object SHOULD be unique.
  //   When the names within an object are not unique, the behavior of software that receives such an object is
  //   unpredictable
  //
  // https://www.ecma-international.org/ecma-262/#sec-json.parse
  //   In the case where there are duplicate name Strings within an object, lexically preceding values for the same key
  //   shall be overwritten.
  template <typename U, typename T>
  Logger& With(const U& k, const T& v) {
    auto bg = FastBufferGuard(fields_, 2);
    Append(k);
    bg.append(':');
    Append(v);
    bg.append(',');
    return *this;
  }

  // 返回一个新 Logger, 状态和之前的 Logger 完全独立
  Logger Clone();

  // 输出日志
  template <typename T>
  void Panic(const T& msg) {
    With("level", "panic").With("msg", msg).Emit(LogLevel::Panic);
  }
  template <typename T>
  void Fatal(const T& msg) {
    With("level", "fatal").With("msg", msg).Emit(LogLevel::Fatal);
  }
  template <typename T>
  void Error(const T& msg) {
    With("level", "error").With("msg", msg).Emit(LogLevel::Error);
  }
  template <typename T>
  void Warning(const T& msg) {
    With("level", "warning").With("msg", msg).Emit(LogLevel::Warning);
  }
  template <typename T>
  void Info(const T& msg) {
    With("level", "info").With("msg", msg).Emit(LogLevel::Info);
  }
  template <typename T>
  void Debug(const T& msg) {
    With("level", "debug").With("msg", msg).Emit(LogLevel::Debug);
  }

 private:
  Logger(std::mutex* _lock, std::ostream** _out_stream, LogLevel* _out_level);
  explicit Logger(const FastBuffer& fields, std::mutex* _lock, std::ostream** _out_stream, LogLevel* _out_level);
  // 该函数没有实现，如果所有特殊化模板都匹配不到则编译会报错
  template <typename T>
  void Append(const T& v);
  template <typename T>
  void Append(std::shared_ptr<T> v) {
    if (!v) {
      auto bg = FastBufferGuard(fields_, 4);
      bg.append("null");
      return;
    }
    Append(*v);
  }
  // 特殊化模板在处理 Append("abc") 时需要特殊化的类型为 char [4],
  // 因此需要把所有字符串长度都特殊化一遍，因此这里使用了重载 ref:
  // https://en.cppreference.com/w/cpp/language/template_argument_deduction
  template <std::size_t N>
  void Append(const char (&v)[N]) {
    StringFmt(fields_, v, N - 1);
  }
  void Emit(const LogLevel level);
  FastBuffer fields_;
  std::size_t index_;

  std::mutex* m_lock;
  std::ostream** m_out_stream;
  LogLevel* m_out_level;
};

// 用于输出原始 json 字符串, 会去除其中的换行
template <typename T>
class JsonRawMessage {
 public:
  explicit JsonRawMessage(const T& json) : raw_message_(json) {}
  const T& raw_message_;
};

template <typename T>
JsonRawMessage<T> make_json(const T& json) {
  return JsonRawMessage<T>(json);
}

// 可以在运行中调用以调整输出
// nullptr 关闭输出
// 默认输出到 stderr
// 如果需要压缩可以指定一个满足 ostream 接口的 zlib
// 线程安全
void SetOutput(std::ostream* out);

// 设置日志等级，日志等级比 level 低的日志不会输出，Panic 为最高等级，Debug 为最低等级
// 线程安全
void SetLevel(const LogLevel level);

}  // namespace structlog
