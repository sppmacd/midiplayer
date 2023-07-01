#pragma once

#include <cstdint>
#include <filesystem>
#include <fmt/format.h>

#if __has_builtin(__builtin_source_location)
#    include <source_location>
#endif
namespace Util {

#if __has_builtin(__builtin_source_location)
using CppSourceLocation = std::source_location;
#else
// Yes. Dummy impl to satisfy clangD, next time.
struct CppSourceLocation {
    static CppSourceLocation current() { return {}; }
    constexpr uint_least32_t line() const noexcept { return 0; }
    constexpr uint_least32_t column() const noexcept { return 0; }
    constexpr const char* file_name() const noexcept { return ""; }
    constexpr const char* function_name() const noexcept { return ""; }
};
#endif

}

template<> class fmt::formatter<Util::CppSourceLocation> : public fmt::formatter<std::string_view> {
public:
    template<typename FormatContext> constexpr auto format(Util::CppSourceLocation const& p, FormatContext& ctx) const {
        fmt::format_to(
            ctx.out(), "\e[33m{}()\e[m ({}:{}:{})", p.function_name(), std::filesystem::path { p.file_name() }.string(), p.line(),
            p.column()
        );
        return ctx.out();
    }
};
