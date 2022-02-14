#include "Action.h"

#include "../Logger.h"
#include "Statement.h"

namespace Config
{

void SetAction::execute(Reader& reader) const
{
    logger::info("Executing SetAction with {} statements", m_statements.size());
    for(auto& it : m_statements)
        it->execute(reader);
}

}
