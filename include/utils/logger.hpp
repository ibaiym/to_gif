#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>

namespace to_gif {

enum class LogLevel : uint8_t {
    kDebug = 0,
    kInfo = 1,
    kWarn = 2,
    kError = 3,
    kFatal = 4,
    kOff = 5,
};

class Logger {
public:
    static Logger& instance() noexcept;

    void set_level(LogLevel level);
    void set_output(std::ostream& os);

    void log(LogLevel level,
             const char* file,
             int line,
             const char* func,
             const std::string& msg);

private:
    Logger() = default;



    LogLevel level_ = LogLevel::kInfo;
    std::ostream* out_ = nullptr;
    std::mutex mtx_;
};

// ------------------------------------------------------------------
// 用于 operator<< 风格日志的流辅助类
// ------------------------------------------------------------------

class NullLogStream {
public:
    template <typename T>
    NullLogStream& operator<<(const T&) { return *this; }
};

class LogStream {
public:
    LogStream(LogLevel level, const char* file, int line, const char* func)
        : level_(level), file_(file), line_(line), func_(func) {}

    ~LogStream() {
        Logger::instance().log(level_, file_, line_, func_, oss_.str());
    }

    template <typename T>
    LogStream& operator<<(const T& value) {
        oss_ << value;
        return *this;
    }

private:
    LogLevel level_;
    const char* file_;
    int line_;
    const char* func_;
    std::ostringstream oss_;
};

} // namespace to_gif

// ------------------------------------------------------------------
// 带编译时优化的便捷宏
// ------------------------------------------------------------------

#ifndef TO_GIF_DISABLE_LOGGING
#define LOG(level) \
    to_gif::LogStream(to_gif::LogLevel::level, __FILE__, __LINE__, __func__)
#else
#define LOG(level) \
    to_gif::NullLogStream()
#endif
