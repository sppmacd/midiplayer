#include "Parser.h"

#include "../Logger.h"
#include "../Try.h"
#include <fmt/color.h>
#include <fmt/format.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace Config
{

ParserErrorOr<Configuration> Parser::parse(Info& info, std::vector<Token> const& token_list)
{
    Parser parser { info, token_list };
    return parser.parse_configuration();
}

ParserErrorOr<float> Parser::get_number()
{
    auto token = get_next_token_of_type(Token::Type::Number);
    if(!token)
        return parser_error("expected number");
    try
    {
        return std::stod(token->value());
    }
    catch(...)
    {
        return parser_error("invalid number: {}", token->value());
    }
}

ParserErrorOr<std::string> Parser::get_string()
{
    auto token = get_next_token_of_type(Token::Type::String);
    if(!token)
        return parser_error("expected string");
    return token->value();
}

ParserErrorOr<int> Parser::get_int()
{
    auto number = TRY(get_number());
    if(number != std::round(number))
        return parser_error("expected integer, got {}", number);
    return number;
}

ParserErrorOr<sf::Color> Parser::parse_color(ColorAlphaMode alpha_mode)
{
    // TODO: Support hex (HTML) colors
    int r = TRY(get_number());
    int g = TRY(get_number());
    int b = TRY(get_number());

    if(r > 255 || g > 255 || b > 255 || r < 0 || g < 0 || b < 0)
        return parser_error("color requires components in range <0;255>, given ({}, {}, {})", r, g, b);

    switch(alpha_mode)
    {
        case ColorAlphaMode::DontAllow:
            return sf::Color(r, g, b, 255);
        case ColorAlphaMode::Allow:
        {
            auto maybe_a = get_number();
            if(maybe_a.has_error())
                return sf::Color(r, g, b, 255);
            auto a = maybe_a.release_value();
            if(a > 255 || a < 0)
                return parser_error("color requires alpha component in range <0;255>, given {}", a);
            return sf::Color(r, g, b, a);
        }
        case ColorAlphaMode::Require:
            return sf::Color(r, g, b, TRY(get_number()));
    }
    return parser_error("?? invalid alpha mode");
}

ParserErrorOr<SelectorList> Parser::parse_selector_list()
{
    SelectorList selectors;
    while(true)
    {
        auto maybe_left_bracket = peek_next_token();
        if(!maybe_left_bracket || maybe_left_bracket->type() != Token::Type::SquareBracketLeft)
            break;

        auto maybe_selector = parse_selector();
        if(maybe_selector.has_error())
            return maybe_selector.release_error();

        selectors.push_back(maybe_selector.release_value());
    }
    return selectors;
}

ParserErrorOr<AttributeValue> Parser::parse_selector_attribute_value()
{
    int v = TRY(get_int());

    auto next_token = peek_next_token();
    if(!next_token)
        return parser_error("unexpected EOF");
    if(next_token->type() == Token::Type::Hyphen)
    {
        // range
        get_next_token();
        int max = TRY(get_int());
        return AttributeValue { std::make_unique<Range>(v, max) };
    }
    if(next_token->type() == Token::Type::Identifier && next_token->value() == "n")
    {
        // linear equation
        get_next_token();

        auto op = get_next_token();
        if(!op)
            return parser_error("unexpected EOF");
        if(op->type() == Token::Type::Add)
        {
            auto b = TRY(get_int());
            return AttributeValue { std::make_unique<LinearEquation>(v, b) };
        }
        if(op->type() == Token::Type::Hyphen)
        {
            auto b = TRY(get_int());
            return AttributeValue { std::make_unique<LinearEquation>(v, -b) };
        }
        return AttributeValue { std::make_unique<LinearEquation>(v, 0) };
    }

    // just an int
    return v;
}

ParserErrorOr<std::shared_ptr<Selector>> Parser::parse_selector()
{
    auto brace_left = get_next_token_of_type(Token::Type::SquareBracketLeft);
    if(!brace_left)
        return parser_error("expected '['");

    auto attribute_name = get_next_token_of_type(Token::Type::Identifier);
    if(!attribute_name)
        return parser_error("expected attribute name");

    auto equals = get_next_token_of_type(Token::Type::EqualSign);
    if(!equals)
        return parser_error("expected '='");

    auto value = TRY(parse_selector_attribute_value());

    auto brace_right = get_next_token_of_type(Token::Type::SquareBracketRight);
    if(!brace_right)
        return parser_error("expected ']'");

    if(attribute_name->value() == "channel")
        return std::make_shared<AttributeSelector>(AttributeSelector::Attribute::Channel, std::move(value));
    if(attribute_name->value() == "note")
        return std::make_shared<AttributeSelector>(AttributeSelector::Attribute::Note, std::move(value));
    if(attribute_name->value() == "white_key")
        return std::make_shared<AttributeSelector>(AttributeSelector::Attribute::WhiteKey, std::move(value));
    if(attribute_name->value() == "black_key")
        return std::make_shared<AttributeSelector>(AttributeSelector::Attribute::BlackKey, std::move(value));
    return parser_error("invalid attribute: {}", attribute_name->value());
}

ParserErrorOr<std::vector<PropertyParameter>> Parser::parse_property_parameters(std::span<PropertyFormalParameter const> params)
{
    std::vector<PropertyParameter> result;
    for(auto const& formal_param : params)
    {
        auto param = TRY([&]() -> ParserErrorOr<PropertyParameter>
            {
                switch(formal_param.type())
                {
                    case PropertyType::Int:
                        return TRY(get_int());
                    case PropertyType::Float:
                        return TRY(get_number());
                    case PropertyType::String:
                        return TRY(get_string());
                    case PropertyType::ColorRGB:
                        return TRY(parse_color(ColorAlphaMode::DontAllow));
                    case PropertyType::ColorRGBA:
                        return TRY(parse_color(ColorAlphaMode::Allow));
                    case PropertyType::SelectorList:
                        return TRY(parse_selector_list());
                    default:
                        return parser_error("?? invalid property type");
                }
            }());
        if(!formal_param.value_matches(param))
            return parser_error("value for parameter '{}' must match {}", formal_param.name(), formal_param.match_expression()->to_string());
        result.push_back(std::move(param));
    }
    assert(result.size() == params.size());
    return result;
}

ParserErrorOr<Configuration> Parser::parse_configuration()
{
    Configuration config;
    while(true)
    {
        auto token = get_next_token();
        if(!token)
            return config;
        if(token->type() != Token::Type::Identifier)
            return parser_error("expected identifier");
        auto property_parameters = TRY(parse_property_parameters(m_info.property_formal_parameters(token->value())));
        config.add_statement(std::make_unique<PropertyStatement>(token->value(), std::move(property_parameters)));
    }
    return config;
}

}
