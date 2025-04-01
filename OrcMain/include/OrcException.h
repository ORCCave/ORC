#pragma once

#include "OrcStdHeaders.h"

namespace Orc
{
    class OrcException : public std::runtime_error {
    public:
        OrcException(const std::string& message)
            : std::runtime_error(message), location(std::source_location::current()), trace(std::stacktrace::current())
        {
            fullMessage = std::format(
                "Error: {}\nFile: {}, Line: {}\nFunction: {}\nStack trace:\n{}",
                message,
                location.file_name(),
                location.line(),
                location.function_name(),
                std::to_string(trace)
            );
        }

        const char* what() const noexcept override { return fullMessage.c_str(); }

    private:
        std::source_location location;
        std::stacktrace trace;
        std::string fullMessage;
    };
}