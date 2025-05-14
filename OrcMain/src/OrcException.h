#pragma once

#include <format>
#include <source_location>
#include <stdexcept>
#include <string>

namespace Orc
{
    class OrcException : public std::runtime_error {
    public:
        OrcException(const std::string& message, const std::source_location& location = std::source_location::current())
            : std::runtime_error(makeFullMessage(message, location)) {}
    private:
        std::string makeFullMessage(const std::string& message, const std::source_location& location)
        {
            return std::format(
                "Error: {}\nFile: {}, Line: {}\nFunction: {}\n",
                message,
                location.file_name(),
                location.line(),
                location.function_name()
            );
        }
    };
}