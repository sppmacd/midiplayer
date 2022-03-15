#include "Selector.h"

#include "../Event.h"
#include "../Logger.h"

#include <cctype>
#include <memory>
#include <optional>

using namespace std::literals;

namespace Config
{

bool AttributeSelector::matches(NoteEvent::TransitionUnit transition_unit) const
{
    switch(m_attribute)
    {
        case Attribute::Channel:
            return m_value.matches(transition_unit.channel);
        case Attribute::Note:
            return m_value.matches(transition_unit.key);
        case Attribute::WhiteKey:
            return m_value.matches(transition_unit.key.white_key_index());
        case Attribute::BlackKey:
            return m_value.matches(transition_unit.key.black_key_index());
    }
    abort();
}

}
