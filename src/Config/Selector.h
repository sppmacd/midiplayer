#pragma once

#include <istream>
#include <memory>
#include <variant>

#include "../Event.h"
#include "AttributeValue.h"

class MIDIPlayer;
class NoteEvent;

namespace Config
{

class Selector
{
public:
    Selector() = default;
    Selector(Selector const&) = delete;
    Selector& operator=(Selector const&) = delete;
    virtual ~Selector() = default;

    virtual bool matches(NoteEvent::TransitionUnit, NoteEvent const*) const = 0;

    static std::unique_ptr<Selector> read(std::istream&);
};

class AttributeSelector : public Selector
{
public:
    enum class Attribute
    {
        Channel,
        Note,
        WhiteKey,
        BlackKey,
        Time
    };

    AttributeSelector(Attribute attr, AttributeValue&& value)
    : m_attribute(attr), m_value(std::move(value)) {}

    virtual bool matches(NoteEvent::TransitionUnit, NoteEvent const*) const override;

private:
    Attribute m_attribute {};
    AttributeValue m_value;
};

}
