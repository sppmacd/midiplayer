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

class Range
{
public:
    Range(int min, int max)
    : m_min(min), m_max(max)
    {
        if(m_max < m_min)
            std::swap(m_min, m_max);
    }

    bool contains(int value) const { return value >= m_min && value <= m_max; }

private:
    int m_min {};
    int m_max {};
};

using AttributeValueBase = std::variant<std::monostate, std::string, int, Range>;
struct AttributeValue : public AttributeValueBase
{
    AttributeValue() : AttributeValueBase() {}
    AttributeValue(std::string const& value) : AttributeValueBase(value) {}
    AttributeValue(int value) : AttributeValueBase(value) {}
    AttributeValue(Range value) : AttributeValueBase(value) {}

    bool is_empty() const { return holds_alternative<std::monostate>(*this); }

    std::string as_string() const { return get<std::string>(*this); }
    bool is_string() const { return holds_alternative<std::string>(*this); }
    int as_int() const { return get<int>(*this); }
    bool is_int() const { return holds_alternative<int>(*this); }
    Range as_range() const { return get<Range>(*this); }
    bool is_range() const { return holds_alternative<Range>(*this); }

    bool matches(int a) const { return is_int() && as_int() == a || is_range() && as_range().contains(a); }
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
