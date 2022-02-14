#include "Condition.h"

#include "../MIDIPlayer.h"
#include "Reader.h"

namespace Config
{

bool StartupCondition::is_met(Reader& reader) const
{
    return reader.player().current_frame() == 0;
}

bool TimeCondition::is_met(Reader& reader) const
{
    return reader.player().current_time_is(m_time);
}

}
