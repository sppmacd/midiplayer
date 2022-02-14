#pragma once

#include "Configuration.h"
#include "Info.h"
#include "Lexer.h"
#include "Property.h"
#include "Selector.h"
#include "Action.h"

#include <SFML/Graphics.hpp>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <type_traits>

namespace Config
{

struct SourceRange
{
    SourceLocation location;
    size_t size;
};

struct ParserError
{
    SourceRange range;
    std::string message;
};

template<class T>
class [[nodiscard]] ParserErrorOr
{
public:
    ParserErrorOr(T&& t)
    : m_value(std::move(t)) {}

    ParserErrorOr(ParserError&& t)
    : m_error(std::move(t)) {}

    template<class U>
    ParserErrorOr(U&& u)
    : m_value(std::move(u)) {}

    template<class U>
    ParserErrorOr(ParserErrorOr<U>&& u)
    {
        if(u.has_error())
            m_error = u.release_error();
        else
            m_value = u.release_value();
    }

    bool has_value() const { return m_value.has_value(); }
    bool has_error() const { return m_error.has_value(); }
    T& value() { return m_value.value(); }
    T const& value() const { return m_value.value(); }
    T release_value() { return std::move(value()); }
    ParserError& error() { return m_error.value(); }
    ParserError const& error() const { return m_error.value(); }
    ParserError release_error() { return std::move(error()); }

private:
    std::optional<T> m_value;
    std::optional<ParserError> m_error;
};

class Parser
{
public:
    enum class ColorAlphaMode
    {
        DontAllow, // #(<r> <g> <b>)
        Allow,     // #(<r> <g> <b> [a])
        Require    // #(<r> <g> <b> <a>)
    };

    static ParserErrorOr<Configuration> parse(Info&, std::vector<Token> const&);

private:
    ParserErrorOr<float> get_number();
    ParserErrorOr<std::string> get_string();
    ParserErrorOr<int> get_int();
    ParserErrorOr<Configuration> parse_configuration();
    ParserErrorOr<sf::Color> parse_color(ColorAlphaMode);
    ParserErrorOr<Time> parse_time();

    // [name=value]...
    ParserErrorOr<SelectorList> parse_selector_list();
    ParserErrorOr<AttributeValue> parse_selector_attribute_value();
    ParserErrorOr<std::shared_ptr<Selector>> parse_selector();
    ParserErrorOr<std::vector<PropertyParameter>> parse_property_parameters(std::span<PropertyFormalParameter const>);
    ParserErrorOr<std::shared_ptr<Condition>> parse_condition();
    ParserErrorOr<std::shared_ptr<Action>> parse_action();
    ParserErrorOr<std::shared_ptr<SetAction>> parse_set_action();
    ParserErrorOr<std::unique_ptr<Statement>> parse_statement();
    ParserErrorOr<std::unique_ptr<OnStatement>> parse_on_statement();
    ParserErrorOr<std::unique_ptr<PropertyStatement>> parse_property_statement();

    explicit Parser(Info& info, std::vector<Token> const& in)
    : m_info(info), m_tokens(in) {}

    Token const* peek_next_token()
    {
        if(m_offset >= m_tokens.size())
            return nullptr;
        return &m_tokens[m_offset];
    }

    Token const* get_next_token()
    {
        auto token = peek_next_token();
        m_offset++;
        return token;
    }

    Token const* get_next_token_of_type(Token::Type type)
    {
        auto token = peek_next_token();
        if(token->type() == type)
        {
            m_offset++;
            return token;
        }
        return nullptr;
    }

    SourceRange current_source_range() const
    {
        auto token = m_offset == m_tokens.size() ? m_tokens[m_offset - 1] : m_tokens[m_offset];
        return m_offset == m_tokens.size() ? SourceRange { token.location(), 1 } : SourceRange { token.location(), token.size() };
    }

    template<class... Args>
    [[nodiscard]] ParserError parser_error(fmt::format_string<Args...> message, Args&&... args) const
    {
        return ParserError { current_source_range(), fmt::format(message, std::forward<Args>(args)...) };
    }

    size_t m_offset = 0;
    Info& m_info;
    std::vector<Token> const& m_tokens;
};

}
