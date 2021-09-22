#pragma once

#include "Oblivion.h"

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <fmt/os.h>
#include <fmt/color.h>



namespace Logger
{

enum class LogLevel
{
    INFO = 0, WARNING = 1, FATAL = 2
};
static const char *LogLevelString[] =
{
    " INFO  ",
    "WARNING",
    " FATAL "
};

void Init();

template <typename... Args>
void Log(LogLevel level, const char *fileName, unsigned int lineNumber, const char *functionName, const char *format, const Args&... args)
{
    fmt::v8::text_style style;
    if (level == LogLevel::FATAL)
    {
        style = fg(fmt::color::crimson) | fmt::emphasis::bold;
    }
    else if (level == LogLevel::WARNING)
    {
        style = fg(fmt::color::yellow);
    }
    else if (level == LogLevel::INFO)
    {
        style = fg(fmt::color::white);
    }
    fmt::print(style, "[{}] {}:{} ({}) => ", LogLevelString[(uint32_t)level], fileName, lineNumber, functionName);
    fmt::print(style, format, std::forward<const Args &>(args)...);
    fmt::print("\n");
}

void Close();

};


constexpr const char *GetShortFileName(const char *str, uint32_t len)
{
    const char *result = str;
    for (uint32_t i = len - 1; i >= 0 && i < len; --i)
    {
        if (str[i] == '\\')
        {
            result = str + i + 1;
            break;
        }
    }
    return result;
}

constexpr const char *GetShortFunctionName(const char *str, uint32_t len)
{
    const char *result = str;
    for (uint32_t i = len - 1; i >= 0 && i < len; --i)
    {
        if (str[i] == ':')
        {
            result = str + i + 1;
            break;
        }
    }
    return result;
}

#define SHOWINFO(format, ...) {\
constexpr auto fileName = GetShortFileName(__FILE__, sizeof(__FILE__) - 1);\
constexpr auto functionName = GetShortFunctionName(__FUNCTION__, sizeof(__FUNCTION__) - 1);\
Log(Logger::LogLevel::INFO, fileName, __LINE__, functionName, format, __VA_ARGS__);\
}

#define SHOWWARNING(format, ...) {\
constexpr auto fileName = GetShortFileName(__FILE__, sizeof(__FILE__) - 1);\
constexpr auto functionName = GetShortFunctionName(__FUNCTION__, sizeof(__FUNCTION__) - 1);\
Log(Logger::LogLevel::WARNING, fileName, __LINE__, functionName, format, __VA_ARGS__);\
}

#define SHOWFATAL(format, ...) {\
constexpr auto fileName = GetShortFileName(__FILE__, sizeof(__FILE__) - 1);\
constexpr auto functionName = GetShortFunctionName(__FUNCTION__, sizeof(__FUNCTION__) - 1);\
Log(Logger::LogLevel::FATAL, fileName, __LINE__, functionName, format, __VA_ARGS__);\
}

#define CHECK(cond, retValue, format, ...) {\
if (!cond) {\
SHOWFATAL(format, __VA_ARGS__);\
return (retValue);\
}\
}

#define CHECKRET(cond, retValue, format, ...) {\
if (!cond) {\
SHOWFATAL(format, __VA_ARGS__);\
return;\
}\
}

#define CHECKSHOW(cond, format, ...) {\
if (!cond) {\
SHOWFATAL(format, __VA_ARGS__);\
}\
}

