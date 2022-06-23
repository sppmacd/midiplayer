#include "Statement.h"

#include "Action.h"
#include "Reader.h"

#include "../Logger.h"
#include "../MIDIPlayer.h"

namespace Config {

bool PropertyStatement::execute(Reader& reader) const
{
    auto transition = reader.has_transition() ? reader.current_transition() : Transition();
    if (transition.time().value() == 0)
        reader.info().set_property(m_name, m_args, 1);
    else
        reader.add_transition(transition, [&reader, name = m_name, args = m_args](double factor) { reader.info().set_property(name, args, factor); });
    return true;
}

bool OnStatement::execute(Reader& reader) const
{
    reader.register_conditional_action(m_condition, m_action);
    return true;
}

bool EveryStatement::execute(Reader& reader) const
{
    reader.register_periodic_action(m_interval, m_action);
    return true;
}

bool IfStatement::execute(Reader& reader) const
{
    reader.set_time_offset(reader.player().current_frame());
    if (m_condition->is_met(reader))
        m_action->execute(reader);
    return true;
}

}
