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

class MatchExpression
{
public:
    virtual bool matches(int value) const = 0;
};

class Range : public MatchExpression
{
public:
    Range(int min, int max)
    : m_min(min), m_max(max)
    {
        if(m_max < m_min)
            std::swap(m_min, m_max);
    }

    virtual bool matches(int value) const override { return value >= m_min && value <= m_max; }

private:
    int m_min {};
    int m_max {};
};

// an + b
class LinearEquation : public MatchExpression
{
public:
    LinearEquation(int a, int b)
    : m_a(a), m_b(b) {}

    virtual bool matches(int value) const override { return value - m_b >= 0 && (value - m_b) % m_a == 0; }

private:
    int m_a {};
    int m_b {};
};

using AttributeValueBase = std::variant<std::monostate, std::string, int, std::unique_ptr<MatchExpression>>;
struct AttributeValue : public AttributeValueBase
{
    AttributeValue() : AttributeValueBase() {}
    AttributeValue(std::string const& value) : AttributeValueBase(value) {}
    AttributeValue(int value) : AttributeValueBase(value) {}
    AttributeValue(std::unique_ptr<MatchExpression>&& value) : AttributeValueBase(std::move(value)) {}

    bool is_empty() const { return holds_alternative<std::monostate>(*this); }

    std::string as_string() const { return get<std::string>(*this); }
    bool is_string() const { return holds_alternative<std::string>(*this); }
    int as_int() const { return get<int>(*this); }
    bool is_int() const { return holds_alternative<int>(*this); }
    MatchExpression const& as_match_expression() const { return *get<std::unique_ptr<MatchExpression>>(*this); }
    bool is_match_expression() const { return holds_alternative<std::unique_ptr<MatchExpression>>(*this); }

    bool matches(int a) const { return is_int() && as_int() == a || is_match_expression() && as_match_expression().matches(a); }
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

    AttributeSelector(Attribute attr, AttributeValue&& value)
    : m_attribute(attr), m_value(std::move(value)) {}
};