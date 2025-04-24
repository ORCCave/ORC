#pragma once

#include <format>
#include <source_location>
#include <stacktrace>
#include <stdexcept>
#include <string>

namespace Orc
{
    class OrcException : public std::runtime_error {
    public:
        OrcException(const std::string& message, const std::source_location& location = std::source_location::current(), const std::stacktrace& trace = std::stacktrace::current())
            : std::runtime_error(makeFullMessage(message, location, trace)) {}
    private:
        std::string makeFullMessage(const std::string& message, const std::source_location& location, const std::stacktrace& trace)
        {
            return std::format(
                "Error: {}\nFile: {}, Line: {}\nFunction: {}\nStack trace:\n{}",
                message,
                location.file_name(),
                location.line(),
                location.function_name(),
                std::to_string(trace)
            );
        }
    };
}