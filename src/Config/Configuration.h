#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "Property.h"
#include "Statement.h"

namespace Config
{

class Reader;

class Configuration
{
public:
    void add_statement(std::unique_ptr<Statement>);

    bool execute(Reader&) const;

private:
    std::vector<std::unique_ptr<Statement>> m_statements;
};

}
