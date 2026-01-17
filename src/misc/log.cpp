#include <SDL3/SDL.h>

#include <cstdio>
#include <mutex>

#include <misc/log.hpp>

static bool log_to_file = true;
static std::string log_filename;
static std::mutex log_mutex;
static FILE* log_file = nullptr;

namespace util {

    static const char* getLogPrefix(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:    return "[TRACE] ";
        case LogLevel::Debug:    return "[DEBUG] ";
        case LogLevel::Info:     return "[INFO ] ";
        case LogLevel::Warn:     return "[WARN ] ";
        case LogLevel::Error:    return "[ERROR] ";
        case LogLevel::Critical: return "[CRIT ] ";
        default:                 return "";
        }
    }

    static const char* getTimeStamp() {
        static char time_buffer[32];
        const auto current_time = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(current_time);
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", localtime(&time));
        return time_buffer;
    }

    void setFileLogging(const std::string& filename, bool status = true) {
        /* 
        Keep file handle open for better performance instead of opening/closing on each write.
        Need mutex since multiple threads could write logs simultaneously.
        */
        std::lock_guard<std::mutex> lock(log_mutex);
        
        if (log_file) {
            fclose(log_file);
            log_file = nullptr;
        }

        log_to_file = status;
        log_filename = filename;

        if (status && !filename.empty()) {
            log_file = fopen(filename.c_str(), "w");
            if (!log_file) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open log file: %s", filename.c_str());
                log_to_file = false;
            }
        }
    }

    void log(LogLevel level, const char* fmt, ...) {
        std::lock_guard<std::mutex> lock(log_mutex);

        va_list args;
        va_start(args, fmt);

        char message_buffer[1024];
        SDL_vsnprintf(message_buffer, sizeof(message_buffer), fmt, args);

        SDL_LogPriority sdl_priority = SDL_LOG_PRIORITY_INFO;
        switch (level) {
            case LogLevel::Warn:     sdl_priority = SDL_LOG_PRIORITY_WARN; break;
            case LogLevel::Error:    sdl_priority = SDL_LOG_PRIORITY_ERROR; break;
            case LogLevel::Critical: sdl_priority = SDL_LOG_PRIORITY_CRITICAL; break;
            case LogLevel::Debug:    sdl_priority = SDL_LOG_PRIORITY_DEBUG; break;
            case LogLevel::Trace:    sdl_priority = SDL_LOG_PRIORITY_VERBOSE; break;
            default:                 break;
        }

        SDL_LogMessage(
            SDL_LOG_CATEGORY_APPLICATION,
            sdl_priority,
            "[%s] %s%s",
            getTimeStamp(),
            getLogPrefix(level),
            message_buffer
        );

        if (log_to_file && log_file) {
            fprintf(log_file, "[%s] %s%s\n", getTimeStamp(), getLogPrefix(level), message_buffer);
            fflush(log_file);
        }

        va_end(args);
    }

} // namespace util