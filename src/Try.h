#pragma once

#include <utility>

// Based on SerenityOS's <AK/Try.h>, but with std::optional

#define TRY_OPTIONAL(expression)                       \
    ({                                                 \
        auto _temporary_result = (expression);         \
        if (!_temporary_result.has_value()) [[unlikely]] \
            return {};  \
        std::move(_temporary_result.value());          \
    })
