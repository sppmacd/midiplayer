#pragma once

#include <memory>
#include <vector>

#include "Property.h"

namespace Config
{

class Reader;

class Statement
{
public:
    virtual bool execute(Reader& reader) const = 0;
};

class PropertyStatement : public Statement
{
public:
    PropertyStatement(std::string name, std::vector<PropertyParameter> args)
    : m_name(std::move(name)), m_args(std::move(args)) {}

    virtual bool execute(Reader& reader) const override;

private:
    std::string m_name;
    std::vector<PropertyParameter> m_args;
};

}
