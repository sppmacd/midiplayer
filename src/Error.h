/*
 * Copyright (c) 2021-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// Most of code here is taken from SerenityOS, but ported to STL.
// https://github.com/SerenityOS/serenity/blob/master/AK/Error.h
// https://github.com/SerenityOS/serenity/blob/master/AK/Try.h

#pragma once

#include "Config.h"
#include "CppSourceLocation.h"
#include <cassert>
#include <cerrno>
#include <fmt/core.h>
#include <fmt/format.h>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

namespace Util {

template<typename T, typename... ErrorTypes> class [[nodiscard]] ErrorOr;

#define TRY(...)                                                                                \
    ({                                                                                          \
        auto _temporary_result = (__VA_ARGS__);                                                 \
        if (_temporary_result.is_error()) [[unlikely]]                                          \
            return { _temporary_result.release_error_variant(), _temporary_result.location() }; \
        _temporary_result.release_value();                                                      \
    })

#define MUST(...)                                        \
    ({                                                   \
        auto _temporary_result = (__VA_ARGS__);          \
        if (_temporary_result.is_error()) [[unlikely]] { \
            _temporary_result.dump("Unhandled error");   \
            abort();                                     \
        }                                                \
        _temporary_result.release_value();               \
    })

template<class... ErrorTypes> using ErrorVariant = std::variant<ErrorTypes...>;

template<typename T, typename... ErrorTypes> class [[nodiscard]] ErrorOr : public std::variant<T, ErrorTypes...> {
public:
    using Variant = std::variant<T, ErrorTypes...>;

    using Error = First<ErrorTypes...>;

    template<class E> static constexpr bool IsConvertibleToError = { (std::is_convertible_v<E, ErrorTypes> || ...) };

    template<class E> static constexpr bool ContainsError = { (std::is_same_v<E, ErrorTypes> || ...) };

    template<typename U>
    ESSA_ALWAYS_INLINE ErrorOr(U&& value, CppSourceLocation loc = CppSourceLocation::current())
        requires(!std::is_same_v<std::remove_cvref_t<U>, ErrorOr<T, ErrorTypes...>> && (IsConvertibleToError<U>))
        : Variant(std::forward<U>(value))
        , m_location(loc) { }

    template<typename U>
    ESSA_ALWAYS_INLINE ErrorOr(U&& value)
        requires(!std::is_same_v<std::remove_cvref_t<U>, ErrorOr<T, ErrorTypes...>> && (std::is_convertible_v<U, T>))
        : Variant(std::forward<U>(value)) { }

    // Construction from ErrorOr containing one of errors
    template<class U, class ET>
    ESSA_ALWAYS_INLINE ErrorOr(ErrorOr<U, ET>&& value, CppSourceLocation loc = CppSourceLocation::current())
        requires(!std::is_same_v<ET, First<ErrorTypes...>> && (std::is_convertible_v<U, T> && IsConvertibleToError<ET>))
        : Variant(value.is_error() ? Variant { value.release_error() } : Variant { value.release_value() })
        , m_location(loc) { }

    // Construction from error variant, used by TRY()
    template<class... ETs>
    ESSA_ALWAYS_INLINE ErrorOr(ErrorVariant<ETs...>&& pack, CppSourceLocation loc)
        requires(IsSubsetOf<std::tuple<ETs...>, std::tuple<ErrorTypes...>>)
        : Variant(std::visit([](auto&& v) -> Variant { return v; }, pack))
        , m_location(loc) { }

    T& value() { return std::get<T>(*this); }
    T const& value() const { return std::get<T>(*this); }

    First<ErrorTypes...>& error()
        requires(sizeof...(ErrorTypes) == 1)
    {
        return std::get<ErrorTypes...>(*this);
    }

    First<ErrorTypes...> const& error() const
        requires(sizeof...(ErrorTypes) == 1)
    {
        return std::get<ErrorTypes...>(*this);
    }

    template<class E>
    E& error_of_type()
        requires(ContainsError<E>)
    {
        return std::get<E>(*this);
    }

    template<class E>
    E const& error_of_type() const
        requires(ContainsError<E>)
    {
        return std::get<E>(*this);
    }

    bool is_error() const { return !std::holds_alternative<T>(*this); }

    ErrorVariant<ErrorTypes...> release_error_variant() {
        return std::visit(
            [](auto&& v) -> ErrorVariant<ErrorTypes...> {
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(v)>, T>) {
                    ESSA_UNREACHABLE;
                }
                else {
                    return v;
                }
            },
            *this
        );
    }

    template<class E>
        requires(ContainsError<E>) bool
    is_error_of_type() const {
        return std::holds_alternative<E>(*this);
    }

    T release_value() { return std::move(value()); }

    template<class E> E release_error_of_type() { return std::move(error_of_type<E>()); }

    auto release_error() { return std::move(error()); }

    T release_value_but_fixme_should_propagate_errors(CppSourceLocation loc = CppSourceLocation::current()) {
        if (is_error()) [[unlikely]] {
            dump("FIXME: Should propagate error", loc);
            abort();
        }
        return release_value();
    }

    // TODO: Implement this for multiple errors
    auto map_error(
        auto callback, CppSourceLocation loc = CppSourceLocation::current()
    ) && -> ErrorOr<T, std::remove_reference_t<decltype(callback(std::declval<ErrorTypes>()...))>>
        requires(sizeof...(ErrorTypes) == 1)
    {
        if (!is_error()) {
            return release_value();
        }
        return { callback(release_error()), loc };
    }

    auto location() const { return m_location; }

    void dump(std::string_view header, CppSourceLocation loc = CppSourceLocation::current()) const {
        if (is_error()) {
            fmt::print("\e[31;1m{}\e[m", header);
            std::visit(
                [](auto const& t) {
                    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(t)>, T>) { }
                    else if constexpr (fmt::is_formattable<decltype(t)>()) {
                        fmt::print("\e[m: {}", t);
                    }
                },
                *this
            );
            fmt::print("\e[m\n");
            fmt::print("  at {}\n", CppSourceLocation { location() });
            fmt::print("  ...\n");
            fmt::print("  at {}\n", loc);
        }
    }

private:
    CppSourceLocation m_location;
};

// Partial specialization for void value type
template<typename... ErrorTypes> class [[nodiscard]] ErrorOr<void, ErrorTypes...> final : public ErrorOr<std::monostate, ErrorTypes...> {
public:
    using Base = ErrorOr<std::monostate, ErrorTypes...>;

    template<class E> static constexpr bool IsConvertibleToError = { (std::is_convertible_v<E, ErrorTypes> || ...) };

    ErrorOr()
        : Base { std::monostate {} } { }

    template<typename U>
    ErrorOr(U&& value, CppSourceLocation loc = CppSourceLocation::current())
        requires(IsConvertibleToError<U>)
        : Base(std::forward<U>(value), loc) { }

    // Construction from ErrorOr containing one of errors
    template<class ET>
    ESSA_ALWAYS_INLINE ErrorOr(ErrorOr<void, ET>&& value, CppSourceLocation loc = CppSourceLocation::current())
        requires(!std::is_same_v<ET, First<ErrorTypes...>> && (IsConvertibleToError<ET>))
        : Base(std::forward<decltype(value)>(value), loc) { }

    // Construction from error variant, used by TRY()
    template<class... ETs>
    ESSA_ALWAYS_INLINE ErrorOr(ErrorVariant<ETs...>&& pack, CppSourceLocation loc)
        requires(IsSubsetOf<std::tuple<ETs...>, std::tuple<ErrorTypes...>>)
        : Base(std::forward<decltype(pack)>(pack), loc) { }
};

struct OsError {
    error_t error;
    std::string_view function;
};

template<class T> using OsErrorOr = ErrorOr<T, OsError>;

}

namespace fmt {

template<> struct formatter<Util::OsError> : public formatter<std::string_view> {
    template<typename FormatContext> constexpr auto format(Util::OsError const& p, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}: {}", p.function, strerror(p.error));
    }
};

}
