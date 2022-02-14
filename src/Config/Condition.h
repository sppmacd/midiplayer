#pragma once

namespace Config
{

class Reader;

class Condition
{
public:
    virtual bool is_met(Reader& reader) const = 0;
};

class StartupCondition : public Condition
{
public:
    virtual bool is_met(Reader& reader) const override;
};

}
