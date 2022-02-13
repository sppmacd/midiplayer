#include "Selector.h"

#include "../Event.h"
#include "../Logger.h"

#include <cctype>
#include <memory>
#include <optional>

using namespace std::literals;

namespace Config
{

bool AttributeSelector::matches(MIDIPlayer const& player, NoteEvent const& event)
{
    switch(m_attribute)
    {
        case Attribute::Channel:
            return m_value.matches(event.channel());
        case Attribute::Note:
            return m_value.matches(event.key());
        case Attribute::WhiteKey:
            return m_value.matches(event.key().white_key_index());
        case Attribute::BlackKey:
            return m_value.matches(event.key().black_key_index());
    }
    abort();
}

}
