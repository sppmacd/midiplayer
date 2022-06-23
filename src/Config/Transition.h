#pragma once

#include "Time.h"

namespace Config {

class Transition {
public:
    // TODO: Add here all CSS timing functions
    enum class Function {
        Constant0,
        Constant1,
        Linear,
        EaseInOutQuad,
    };

    Transition() = default;

    Transition(Time time, Function function)
        : m_time(time)
        , m_function(function)
    {
    }

    Time time() const { return m_time; }
    Function function() const { return m_function; }

private:
    Time m_time;
    Function m_function { Function::Linear };
};

}
