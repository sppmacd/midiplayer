#pragma once

#include <istream>
#include <memory>
#include <variant>

class MIDIPlayer;
class NoteEvent;

class Selector
{
public:
    Selector() = default;
    Selector(Selector const&) = delete;
    Selector& operator=(Selector const&) = delete;

    virtual bool matches(MIDIPlayer const& player, NoteEvent const& event) = 0;

    static std::unique_ptr<Selector> read(std::istream&);
};

struct AttributeValue : public std::variant<std::string, int>
{
    AttributeValue(std::string const& value) : std::variant<std::string, int>(value) {}
    AttributeValue(int value) : std::variant<std::string, int>(value) {}

    std::string as_string() const { return get<std::string>(*this); }
    int as_int() const { return get<int>(*this); }
};

class AttributeSelector : public Selector
{
public:
    virtual bool matches(MIDIPlayer const& player, NoteEvent const& event) override;

    static std::unique_ptr<Selector> read(std::istream&);

private:
    enum class Attribute
    {
        Channel,
        Note
    };

    Attribute m_attribute {};
    AttributeValue m_value;

    AttributeSelector(Attribute attr, AttributeValue const& value)
    : m_attribute(attr), m_value(value) {}
};
