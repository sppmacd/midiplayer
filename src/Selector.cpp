#include "Selector.h"

#include "Event.h"

#include <cctype>
#include <memory>
#include <optional>

using namespace std::literals;

inline void parse_error(std::string_view text)
{
    std::cerr << "    ERROR: Syntax error in selector: " << text << std::endl;
}

static void ignore_whitespace(std::istream& in)
{
    while(isspace(in.peek()))
        in.get();
}

bool is_identifier_character(char ch)
{
    return isalnum(ch) || ch == '_';
}

static std::optional<std::string> read_identifier(std::istream& in)
{
    std::string out;

    if(auto ch = in.peek(); !is_identifier_character(ch) || isdigit(ch))
        return {};
    while(is_identifier_character(in.peek()))
        out += in.get();
    return out;
}

static std::string read_literal(std::istream& in)
{
    std::string out;
    while(isalnum(in.peek()))
        out += in.get();
    return out;
}

static AttributeValue read_value(std::istream& in)
{
    int c = in.peek();
    if(isdigit(c))
    {
        int v;
        if(!(in >> v))
            return {};

        if(in.peek() == '-')
        {
            // Range
            in.get(); // '-'
            int max;
            if(!(in >> max))
                return {};
            return {std::make_unique<Range>(v, max)};
        }
        if(in.peek() == 'n')
        {
            // LinearEquation
            in.get(); // 'n'
            int next_c = in.peek();
            if(next_c == '+')
            {
                int b;
                if(!(in >> b))
                    return {};
                return {std::make_unique<LinearEquation>(v, b)};
            }
            if(next_c == '-')
            {
                int b;
                if(!(in >> b))
                    return {};
                return {std::make_unique<LinearEquation>(v, -b)};
            }
            return {std::make_unique<LinearEquation>(v, 0)};
        }
        return v;
    }
    return read_literal(in);
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
    auto value = read_value(in);
    if(value.is_empty())
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
        return std::unique_ptr<AttributeSelector>{ new AttributeSelector{Attribute::Channel, std::move(value)} };
    if(attribute_name == "note"sv)
        return std::unique_ptr<AttributeSelector>{ new AttributeSelector{Attribute::Note, std::move(value)} };
    if(attribute_name == "white_key"sv)
        return std::unique_ptr<AttributeSelector>{ new AttributeSelector{Attribute::WhiteKey, std::move(value)} };
    if(attribute_name == "black_key"sv)
        return std::unique_ptr<AttributeSelector>{ new AttributeSelector{Attribute::BlackKey, std::move(value)} };
    parse_error("Unknown attribute: " + attribute_name.value());
    return nullptr;
}

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
