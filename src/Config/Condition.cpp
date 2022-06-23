#include "Condition.h"

#include "../MIDIPlayer.h"
#include "Reader.h"

namespace Config {

bool StartupCondition::is_met(Reader& reader) const
{
    return reader.player().current_frame() == 0;
}

bool StartupCondition::has_expired(Reader& reader) const
{
    return true;
}

bool TimeCondition::is_met(Reader& reader) const
{
    return reader.player().current_time_is(m_time, reader.time_offset());
}

bool TimeCondition::has_expired(Reader& reader) const
{
    return reader.player().current_frame() >= reader.player().frame_count_for_time(m_time, reader.time_offset());
}

bool ModeCondition::is_met(Reader& reader) const
{
    return (m_mode == Mode::Play && !reader.player().real_time())
        || (m_mode == Mode::Realtime && reader.player().real_time());
}

bool ModeCondition::has_expired(Reader& reader) const
{
    return reader.player().real_time() != (m_mode == Mode::Realtime);
}

}
