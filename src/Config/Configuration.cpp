#include "Configuration.h"

namespace Config
{

void Configuration::add_statement(std::unique_ptr<Statement> statement)
{
    m_statements.push_back(std::move(statement));
}

bool Configuration::execute(Reader& reader) const
{
    bool success = true;
    for(auto& statement : m_statements)
        success &= statement->execute(reader);
    return success;
}

}
