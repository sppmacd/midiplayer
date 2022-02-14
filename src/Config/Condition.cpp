#include "Condition.h"

#include "../MIDIPlayer.h"
#include "Reader.h"

namespace Config
{

bool StartupCondition::is_met(Reader& reader) const
{
    std::cerr << reader.player().current_frame() << " == 0?" << std::endl;
    return reader.player().current_frame() == 0;
}

}
