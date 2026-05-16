#include "utils/logger.hpp"

#include <ctime>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace to_gif {

namespace {

const char* level_str(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::kDebug: return "DEBUG";
        case LogLevel::kInfo:  return "INFO";
        case LogLevel::kWarn:  return "WARN";
        case LogLevel::kError: return "ERROR";
        case LogLevel::kFatal: return "FATAL";
        default:               return "UNKNOWN";
    }
}

std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;

    std::tm tm{};
    {
        static std::mutex localtime_mtx;
        std::lock_guard<std::mutex> lock(localtime_mtx);
        tm = *std::localtime(&t);
    }

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

} // namespace

// ------------------------------------------------------------------
// Singleton
// ------------------------------------------------------------------

Logger& Logger::instance() noexcept {
    static Logger inst;
    return inst;
}

// ------------------------------------------------------------------
// Configuration
// ------------------------------------------------------------------

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mtx_);
    level_ = level;
}

void Logger::set_output(std::ostream& os) {
    std::lock_guard<std::mutex> lock(mtx_);
    out_ = &os;
}

// ------------------------------------------------------------------
// Logging
// ------------------------------------------------------------------

void Logger::log(LogLevel level,
                 const char* file,
                 int line,
                 const char* func,
                 const std::string& msg) {
    if (level < level_) {
        return;
    }

    // Keep only filename (strip path)
    const char* basename = std::strrchr(file, '/');
    if (!basename) {
        basename = std::strrchr(file, '\\');
    }
    basename = basename ? basename + 1 : file;

    std::lock_guard<std::mutex> lock(mtx_);

    std::ostream& os = out_ ? *out_ : std::cerr;

    os << '[' << timestamp() << "]"
       << '[' << level_str(level) << "]"
       << '[' << basename << ':' << line << "]"
       << '[' << func << "] "
       << msg
       << '\n';
}



} // namespace to_gif
