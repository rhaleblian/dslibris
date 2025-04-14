#pragma once

#include <iostream>

class Logger {
    public:
        void Log(const char *msg);
        void Log(const char *format, const int value);
        void Log(const char *format, const char *msg);
        void Log(const std::string msg);
        void Fatal(const char *msg);
};
