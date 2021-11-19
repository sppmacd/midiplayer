#include "Selector.h"

#include "Event.h"

#include <cctype>
#include <optional>

using namespace std::literals;

inline void parse_error(std::string_view text)
{
    std::cout << "    ERROR: Syntax error in selector: " << text << std::endl;
}

static void ignore_whitespace(std::istream& in)
{
    while(isspace(in.peek()))
        in.get();
}

static std::optional<std::string> read_identifier(std::istream& in)
{
    std::string out;
    if(!isalpha(in.peek()))
        return {};
    while(isalnum(in.peek()))
        out += in.get();
    return out;
}

static std::optional<std::string> read_literal(std::istream& in)
{
    std::string out;
    while(isalnum(in.peek()))
        out += in.get();
    return out;
}

std::unique_ptr<Selector> Selector::read(std::istream& in)
{
    ignore_whitespace(in);
    if(in.peek() == '[')
        return AttributeSelector::read(in);
    return nullptr;
}

std::unique_ptr<Selector> AttributeSelector::read(std::istream& in)
{
    in.get(); // '['
    ignore_whitespace(in);

    auto attribute_name = read_identifier(in);
    if(!attribute_name.has_value())
    {
        parse_error("Expected identifier");
        return nullptr;
    }

    ignore_whitespace(in);
    if(in.get() != '=')
    {
        parse_error("Expected '='");
        return nullptr;
    }

    ignore_whitespace(in);
    auto value = read_literal(in);
    if(!value.has_value())
    {
        parse_error("Expected value");
        return nullptr;
    }

    ignore_whitespace(in);
    if(in.get() != ']')
    {
        parse_error("Expected ']' after attribute definition");
        return nullptr; 
    }
    ignore_whitespace(in);

    if(attribute_name == "channel"sv)
    {
        try
        {
            return std::unique_ptr<AttributeSelector>{ new AttributeSelector{Attribute::Channel, std::stoi(value.value())} };
        }
        catch(...)
        {
            parse_error("Invalid number for 'channel' attribute");
            return nullptr;
        }
    }
    if(attribute_name == "note"sv)
    {
        try
        {
            return std::unique_ptr<AttributeSelector>{ new AttributeSelector{Attribute::Note, std::stoi(value.value())} };
        }
        catch(...)
        {
            parse_error("Invalid number for 'note' attribute");
            return nullptr;
        }
    }
    parse_error("Unknown attribute");
    return nullptr;
}

bool AttributeSelector::matches(MIDIPlayer const& player, NoteEvent const& event)
{
    switch(m_attribute)
    {
        case Attribute::Channel:
            return event.channel() == m_value.as_int();
        case Attribute::Note:
            return event.key() == m_value.as_int();
    }
    abort();
}
