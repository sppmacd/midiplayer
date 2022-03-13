#pragma once

#include "Time.h"

namespace Config
{

class Reader;

class Condition
{
public:
    virtual ~Condition() = default;
    virtual bool is_met(Reader& reader) const = 0;
};

class StartupCondition : public Condition
{
public:
    virtual bool is_met(Reader& reader) const override;
};

class TimeCondition : public Condition
{
public:
    TimeCondition(Time time)
    : m_time(time) {}

    virtual bool is_met(Reader& reader) const override;

private:
    Time m_time;
};

}
