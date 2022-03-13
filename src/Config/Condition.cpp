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
    return reader.player().current_time_is(m_time, reader.time_offset());
}

bool ModeCondition::is_met(Reader& reader) const
{
    return reader.player().current_frame() == 0
        && ((m_mode == Mode::Play && !reader.player().real_time()) || m_mode == Mode::Realtime && reader.player().real_time());
}

}
