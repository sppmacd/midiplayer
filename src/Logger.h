#pragma once

#include <fmt/color.h>
#include <fmt/format.h>

namespace logger
{

template<class... Args>
void log(fmt::terminal_color style, std::string str, fmt::format_string<Args...> fmtstr, Args... args)
{
    // FIXME: fmtlib doesn't support printing with text style to another streams than stderr??
    std::string data;
    fmt::format_to(std::back_inserter(data), fmt::fg(style) | fmt::emphasis::bold, "{}: ", str);
    fmt::print(stderr, "{}", data);

    fmt::print(stderr, fmtstr, std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
}

template<class... Args>
void info(fmt::format_string<Args...> fmtstr, Args... args)
{
    log(fmt::terminal_color::bright_blue, "INFO", std::forward<fmt::format_string<Args...>>(fmtstr), std::forward<Args>(args)...);
}

template<class... Args>
void warning(fmt::format_string<Args...> fmtstr, Args... args)
{
    log(fmt::terminal_color::yellow, "WARNING", std::forward<fmt::format_string<Args...>>(fmtstr), std::forward<Args>(args)...);
}

template<class... Args>
void error(fmt::format_string<Args...> fmtstr, Args... args)
{
    log(fmt::terminal_color::red, "ERROR", std::forward<fmt::format_string<Args...>>(fmtstr), std::forward<Args>(args)...);
}

template<class... Args>
void error_note(fmt::format_string<Args...> fmtstr, Args... args)
{
    log(fmt::terminal_color::bright_cyan, "note", std::forward<fmt::format_string<Args...>>(fmtstr), std::forward<Args>(args)...);
}

}
