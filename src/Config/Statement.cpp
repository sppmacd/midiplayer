#include "Statement.h"

#include "Action.h"
#include "Reader.h"

#include "../Logger.h"

namespace Config
{

bool PropertyStatement::execute(Reader& reader) const
{
    reader.info().set_property(m_name, m_args);
    return true;
}

bool OnStatement::execute(Reader& reader) const
{
    reader.register_conditional_actions(m_condition, m_action);
    return true;
}

}
