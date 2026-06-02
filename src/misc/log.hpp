#pragma once
#include <string>
#include <cstdarg>
#include <chrono>

namespace util {

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };

} // namespace util

// Debug builds
#ifdef GAME_ENGINE_DEBUG_MODE
    #define LOG_TRACE(...)          util::log(util::LogLevel::Trace, __VA_ARGS__)
    #define LOG_DEBUG(...)          util::log(util::LogLevel::Debug, __VA_ARGS__)
    #define LOG_INFO(...)           util::log(util::LogLevel::Info, __VA_ARGS__)
    #define LOG_WARN(...)           util::log(util::LogLevel::Warn, __VA_ARGS__)
    #define LOG_ERROR(...)          util::log(util::LogLevel::Error, __VA_ARGS__)
    #define LOG_CRIT(...)           util::log(util::LogLevel::Critical, __VA_ARGS__)
    #define LOG_FILE_START(file)    util::setFileLogging(file, true)
    #define LOG_FILE_STOP(file)     util::setFileLogging(file, false)
// Release builds - compile to nothing
#else
    #define LOG_TRACE(...)          ((void)0)
    #define LOG_DEBUG(...)          ((void)0)
    #define LOG_INFO(...)           ((void)0)
    #define LOG_WARN(...)           ((void)0)
    #define LOG_ERROR(...)          ((void)0)
    #define LOG_CRIT(...)           ((void)0)
    #define LOG_FILE_START(file)    ((void)0)
    #define LOG_FILE_STOP(file)     ((void)0)
#endif

namespace util {

    /**
     * @brief Enable or disable logging to a file
     * @param filename Path to the log file
     * @param status True to enable file logging, False to disable. True by default.
     */
    void setFileLogging(const std::string &filename, bool status);

    /**
     * @brief Log a formatted message with the specified level
     * @param level Severity level of the message
     * @param fmt Printf-style format string
     * @param ... Variable arguments for format string
     */
    void log(LogLevel level, const char *fmt, ...);

} // namespace util
