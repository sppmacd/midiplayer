#pragma once

#include <utility>

// Based on SerenityOS's <AK/Try.h>, but with std::optional

// std::optional doesn't have release_value()!
#define TRY_OPTIONAL(expression)                        \
    ({                                                  \
        auto _temporary_result = (expression);          \
        if(!_temporary_result.has_value()) [[unlikely]] \
            return {};                                  \
        std::move(_temporary_result.value());           \
    })

#define TRY(expression)                                \
    ({                                                 \
        auto _temporary_result = (expression);         \
        if(_temporary_result.has_error()) [[unlikely]] \
            return _temporary_result.release_error();  \
        _temporary_result.release_value();             \
    })
