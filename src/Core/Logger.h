#pragma once

#include "Oblivion.h"

#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <fmt/os.h>
#include <fmt/color.h>
#include <fmt/xchar.h>

#include "TypeOutput.h"


namespace Logger
{

enum class LogLevel
{
    INFO = 0, WARNING = 1, FATAL = 2
};
static constexpr const char *LogLevelString[] =
{
    " INFO  ",
    "WARNING",
    " FATAL "
};

extern std::ofstream gOutputStream;

void Init();

template <typename... Args>
constexpr void Log(LogLevel level, const char *fileName, unsigned int lineNumber, const char *functionName, const char *format, const Args&... args)
{
#ifdef COLOR_LOGS
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
#endif
    std::string newFormat = fmt::format("[{}] {}:{} ({}) => {}\n", LogLevelString[(uint32_t)level], fileName, lineNumber, functionName, format);
    std::string stringToPrint = fmt::format(newFormat, std::forward<const Args &>(args)...);
    
#ifdef COLOR_LOGS
    fmt::print(style, stringToPrint);
#else
    fmt::print(stringToPrint);
#endif
    gOutputStream << stringToPrint;
    gOutputStream.flush();
}

void Close();

} // namespace Logger;


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
if (!(cond)) {\
SHOWFATAL(format, __VA_ARGS__);\
return (retValue);\
}\
}

#define CHECKSIMPLE(cond) {\
if (!(cond)) {\
return false;\
}\
}

#define CHECKRET(cond, format, ...) {\
if (!(cond)) {\
SHOWFATAL(format, __VA_ARGS__);\
return;\
}\
}

#define CHECKCONT(cond, format, ...) {\
if (!(cond)) {\
SHOWFATAL(format, __VA_ARGS__);\
continue;\
}\
}

#define CHECKBK(cond, format, ...) {\
if (!(cond)) {\
SHOWFATAL(format, __VA_ARGS__);\
break;\
}\
}

#define CHECKSHOW(cond, format, ...) {\
if (!(cond)) {\
SHOWFATAL(format, __VA_ARGS__);\
}\
}

#define ASSIGN_RESULT(var, result, retValue, format, ...) {\
auto __res = result;\
CHECK(__res.Valid(), retValue, format, __VA_ARGS__);\
var = __res.Get();\
}

#define ASSIGN_RESULTRET(var, result, retValue, format, ...) {\
auto __res = result;\
CHECKRET(__res.Valid(), format, __VA_ARGS__);\
var = __res.Get();\
}

#define RETURN_ERROR(ret, format, ...) {\
SHOWINFO(format, __VA_ARGS__);\
return (ret);\
}\

inline const TCHAR *GetStringFromHr(HRESULT hr)
{
    _com_error err(hr);
    return err.ErrorMessage();
}

inline auto ReturnFalseIfFailed(HRESULT hr) -> std::tuple<bool, const TCHAR *>
{
    if (FAILED(hr))
    {
        return { false, GetStringFromHr(hr) };
    }
    return { true, GetStringFromHr(hr) };
}

#define CHECK_HR(hr, retValue) {\
    auto [__result, __message] = ReturnFalseIfFailed(hr); \
    CHECK(__result, retValue, "Expression {} failed with reason: {}", #hr, __message); \
}

#define CHECKRET_HR(hr) {\
    auto [__result, __message] = ReturnFalseIfFailed(hr); \
    CHECKRET(__result, "Expression {} failed with reason: {}", #hr, __message); \
}
