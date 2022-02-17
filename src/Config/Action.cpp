#include "Action.h"

#include "../Logger.h"
#include "Reader.h"
#include "Statement.h"

namespace Config
{

void SetAction::execute(Reader& reader) const
{
    logger::info("Executing SetAction with {} statements", m_statements.size());
    reader.push_transition_time(m_transition_time);
    for(auto& it : m_statements)
        it->execute(reader);
    reader.pop_transition_time();
}

}
