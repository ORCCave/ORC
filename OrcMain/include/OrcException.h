#pragma once

#include "OrcStdHeaders.h"

namespace Orc
{
    class OrcException : public std::exception {
    public:
        OrcException(const std::string& message) :msg(message) {}
        const char* what() const noexcept override { return msg.c_str(); }
    private:
        std::string msg;
    };
}