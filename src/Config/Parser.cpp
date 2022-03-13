#include "Parser.h"

#include "../Event.h"
#include "../Logger.h"
#include "../Try.h"
#include "Condition.h"
#include "Transition.h"

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
    if(token)
        return token->value();
    token = get_next_token_of_type(Token::Type::Identifier);
    if(token)
        return token->value();
    return parser_error("expected string");
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

ParserErrorOr<Time> Parser::parse_time()
{
    auto value = TRY(get_number());
    auto unit = get_next_token_of_type(Token::Type::Identifier);
    if(!unit || unit->value() == "s")
        return Time { value, Time::Unit::Seconds };
    if(unit->value() == "f")
        return Time { value, Time::Unit::Frames };
    if(unit->value() == "t")
        return Time { value, Time::Unit::Ticks };
    return parser_error("invalid time unit: {}", unit->value());
}

ParserErrorOr<NamedParameters> Parser::parse_named_parameters(NamedFormalParameters const& formal_params)
{
    if(!get_next_token_of_type(Token::Type::BracketLeft))
        return parser_error("expected '('");

    NamedParameters parameters;

    while(true)
    {
        auto name = get_next_token_of_type(Token::Type::Identifier);
        if(!name)
            return parser_error("expected identifier in named parameter list");
        if(get_next_token_of_type(Token::Type::EqualSign))
        {
            auto formal_param = formal_params.find(name->value());
            if(formal_param == formal_params.end())
                return parser_error("invalid named parameter: '{}'", name->value());

            parameters.insert({ name->value(), TRY(parse_property_parameter(formal_param->second)) });
        }
        if(!get_next_token_of_type(Token::Type::Comma))
            break;
    }

    if(!get_next_token_of_type(Token::Type::BracketRight))
        return parser_error("expected closing ')'");
    return parameters;
}

ParserErrorOr<SelectorList> Parser::parse_selector_list()
{
    SelectorList selectors;
    while(true)
    {
        auto maybe_left_bracket = peek_next_token();
        if(!maybe_left_bracket || maybe_left_bracket->type() != Token::Type::SquareBracketLeft)
            break;

        selectors.push_back(TRY(parse_selector()));
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
        auto param = TRY(parse_property_parameter(formal_param));
        if(!formal_param.value_matches(param))
            return parser_error("value for parameter '{}' must match {}", formal_param.name(), formal_param.match_expression()->to_string());
        result.push_back(std::move(param));
    }
    assert(result.size() == params.size());
    return result;
}

ParserErrorOr<PropertyParameter> Parser::parse_property_parameter(PropertyFormalParameter const& formal_param)
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
        case PropertyType::Time:
            return TRY(parse_time());
        default:
            return parser_error("?? invalid property type");
    }
}

ParserErrorOr<std::shared_ptr<Condition>> Parser::parse_condition()
{
    auto identifier = get_next_token();
    if(!identifier || identifier->type() != Token::Type::Identifier)
        return parser_error("expected identifier in condition");

    // TODO: Some kind of condition registry
    if(identifier->value() == "startup")
        return std::make_shared<StartupCondition>();
    if(identifier->value() == "time")
    {
        if(!get_next_token_of_type(Token::Type::EqualSign))
            return parser_error("expected '='");
        auto value = TRY(parse_time());
        return std::make_shared<TimeCondition>(value);
    }
    if(identifier->value() == "mode")
    {
        if(!get_next_token_of_type(Token::Type::EqualSign))
            return parser_error("expected '='");
        auto value = TRY(get_string());
        if(value == "realtime")
            return std::make_shared<ModeCondition>(ModeCondition::Mode::Realtime);
        if(value == "play")
            return std::make_shared<ModeCondition>(ModeCondition::Mode::Play);
        return parser_error("invalid mode: {}, valid are 'play', 'realtime'", value);
    }
    return parser_error("invalid condition: {}", identifier->value());
}

ParserErrorOr<std::shared_ptr<Action>> Parser::parse_action()
{
    auto identifier = peek_next_token();
    if(!identifier || identifier->type() != Token::Type::Identifier)
        return parser_error("expected action type (valid are 'set', 'add_event')");
    if(identifier->value() == "set")
        return parse_set_action();
    if(identifier->value() == "add_event")
        return parse_add_event_action();
    return parser_error("unknown action");
}

ParserErrorOr<std::shared_ptr<SetAction>> Parser::parse_set_action()
{
    get_next_token(); // "set"

    std::map<std::string, PropertyParameter> params;

    auto next = peek_next_token();
    if(next && next->type() == Token::Type::BracketLeft)
    {
        params = TRY(parse_named_parameters({ { "transition", PropertyFormalParameter(PropertyType::Time, "transition") },
            { "function", PropertyFormalParameter(PropertyType::String, "function") } }));
    }

    auto get_or = [&](std::string key, auto default_value) -> PropertyParameter
    {
        auto it = params.find(key);
        if(it == params.end())
            return default_value;
        return it->second;
    };

    Time transition_time = get_or("transition", Time()).as_time();
    std::string transition_function = get_or("function", "linear").as_string();

    if(!get_next_token_of_type(Token::Type::CurlyLeft))
        return parser_error("expected '{{' in 'set' action");

    std::vector<std::unique_ptr<Statement>> statements;
    while(true)
    {
        auto token = peek_next_token();
        if(token && token->type() == Token::Type::CurlyRight)
            break;
        statements.push_back(TRY(parse_statement()));
    }

    if(!get_next_token_of_type(Token::Type::CurlyRight))
        return parser_error("expected closing '}}'");

    auto transition_function_from_string = [this](std::string const& str)->ParserErrorOr<Transition::Function>
    {
        if(str == "constant-0")
            return Transition::Function::Constant0;
        if(str == "constant-1")
            return Transition::Function::Constant1;
        if(str == "linear")
            return Transition::Function::Linear;
        if(str == "ease-in-out-quad")
            return Transition::Function::EaseInOutQuad;
        return parser_error("invalid transition function: '{}'", str);
    };

    return std::make_shared<SetAction>(std::move(statements), Transition(transition_time, TRY(transition_function_from_string(transition_function))));
}

ParserErrorOr<std::shared_ptr<AddEventAction>> Parser::parse_add_event_action()
{
    get_next_token(); // "add_event"

    auto name = get_next_token_of_type(Token::Type::Identifier);
    if(!name)
        return parser_error("expected event type name");
    auto event = Event::create_event_from_name(name->value());
    if(!event)
        return parser_error("invalid event name");
    auto formal_parameters = event->formal_parameters();
    auto parameters = TRY(parse_named_parameters(formal_parameters));
    if(!event->read_from_parameters(parameters))
    {
        // FIXME: More detailed error info
        return parser_error("failed to read parameters");
    }
    return std::make_shared<AddEventAction>(std::move(event));
}

ParserErrorOr<std::unique_ptr<Statement>> Parser::parse_statement()
{
    auto identifier = peek_next_token();
    if(!identifier || identifier->type() != Token::Type::Identifier)
        return parser_error("expected statement");
    if(identifier->value() == "on")
        return parse_on_statement();
    if(identifier->value() == "every")
        return parse_every_statement();
    if(identifier->value() == "if")
        return parse_if_statement();
    return parse_property_statement();
}

ParserErrorOr<std::unique_ptr<OnStatement>> Parser::parse_on_statement()
{
    get_next_token(); // "on"

    auto bl = get_next_token_of_type(Token::Type::BracketLeft);
    if(!bl)
        return parser_error("expected '(' in 'on' statement");

    // TODO: Multiple conditions
    auto condition = TRY(parse_condition());

    auto br = get_next_token_of_type(Token::Type::BracketRight);
    if(!br)
        return parser_error("expected closing ')'");

    auto action = TRY(parse_action());
    return std::make_unique<OnStatement>(condition, action);
}

ParserErrorOr<std::unique_ptr<EveryStatement>> Parser::parse_every_statement()
{
    get_next_token(); // "every"

    auto bl = get_next_token_of_type(Token::Type::BracketLeft);
    if(!bl)
        return parser_error("expected '(' in 'on' statement");

    auto interval = TRY(parse_time());

    auto br = get_next_token_of_type(Token::Type::BracketRight);
    if(!br)
        return parser_error("expected closing ')'");

    auto action = TRY(parse_action());
    return std::make_unique<EveryStatement>(interval, action);
}

ParserErrorOr<std::unique_ptr<IfStatement>> Parser::parse_if_statement()
{
    get_next_token(); // "if"

    auto bl = get_next_token_of_type(Token::Type::BracketLeft);
    if(!bl)
        return parser_error("expected '(' in 'if' statement");

    // TODO: Multiple conditions
    auto condition = TRY(parse_condition());

    auto br = get_next_token_of_type(Token::Type::BracketRight);
    if(!br)
        return parser_error("expected closing ')'");

    auto action = TRY(parse_action());
    return std::make_unique<IfStatement>(condition, action);
}

ParserErrorOr<std::unique_ptr<PropertyStatement>> Parser::parse_property_statement()
{
    auto identifier = get_next_token_of_type(Token::Type::Identifier);
    if(!identifier)
        return parser_error("expected identifier");
    auto formal_parameters = m_info.property_formal_parameters(identifier->value());
    auto property_parameters = TRY(parse_property_parameters(formal_parameters));
    return { std::make_unique<PropertyStatement>(identifier->value(), std::move(property_parameters)) };
}

ParserErrorOr<Configuration> Parser::parse_configuration()
{
    Configuration config;
    while(peek_next_token())
        config.add_statement(TRY(parse_statement()));
    return config;
}

}
