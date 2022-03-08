#include "Statement.h"

#include "Action.h"
#include "Reader.h"

#include "../Logger.h"

namespace Config
{

bool PropertyStatement::execute(Reader& reader) const
{
    auto transition = reader.has_transition() ? reader.current_transition() : Transition();
    if(transition.time().value() == 0)
        reader.info().set_property(m_name, m_args, 1);
    else
        reader.add_transition(transition, [&reader, this](double factor) {
            reader.info().set_property(m_name, m_args, factor);
        });
    return true;
}

bool OnStatement::execute(Reader& reader) const
{
    reader.register_conditional_action(m_condition, m_action);
    return true;
}

}
