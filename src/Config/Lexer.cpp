#include "Lexer.h"

#include <cctype>
#include <iostream>

namespace Config
{

char Lexer::consume_one()
{
    char next = m_input.get();
    if(next == EOF)
        return EOF;
    if(next == '\n')
    {
        m_location.line++;
        m_location.column = 1;
    }
    else
        m_location.column++;

    m_location.index++;
    return next;
}

std::vector<Token> Lexer::lex()
{
    std::vector<Token> tokens;

    while(true)
    {
        consume_while([](char c)
            { return std::isspace(c); });
        char next = m_input.peek();
        if(next == EOF)
            break;
        auto start_location = m_location;
        if(next == '#')
        {
            // Comment
            consume_while([](char c)
                { return c != '\n'; });
            continue;
        }
        else if(next == '[')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::SquareBracketLeft, start_location, 1, "["));
        }
        else if(next == ']')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::SquareBracketRight, start_location, 1, "]"));
        }
        else if(next == '(')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::BracketLeft, start_location, 1, "("));
        }
        else if(next == ')')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::BracketRight, start_location, 1, ")"));
        }
        else if(next == '{')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::CurlyLeft, start_location, 1, "{"));
        }
        else if(next == '}')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::CurlyRight, start_location, 1, "}"));
        }
        else if(next == '-')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::Hyphen, start_location, 1, "-"));
        }
        else if(next == '+')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::Add, start_location, 1, "+"));
        }
        else if(next == '=')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::EqualSign, start_location, 1, "="));
        }
        else if(next == ',')
        {
            consume_one();
            tokens.push_back(Token(Token::Type::Comma, start_location, 1, ","));
        }
        else if(std::isalpha(next) || next == '_')
        {
            std::string result = consume_while([&](char c)
                { return std::isalnum(c) || c == '_'; });
            tokens.push_back(Token(Token::Type::Identifier, start_location, m_location.index - start_location.index, result));
        }
        else if(next == '\"')
        {
            consume_one(); // "
            std::string result = consume_while([&](char c)
                { return c != '"'; });
            consume_one(); // "
            tokens.push_back(Token(Token::Type::String, start_location, m_location.index - start_location.index, result));
        }
        else if(std::isdigit(next))
        {
            std::string result = consume_while([&](char c)
                { return c == '-' || c == '+' || std::isdigit(c) || c == '.' || c == 'e' || c == 'E'; });

            tokens.push_back(Token(Token::Type::Number, start_location, m_location.index - start_location.index, result));
        }
        else
        {
            consume_one();
            tokens.push_back(Token(Token::Type::Invalid, start_location, m_location.index - start_location.index, { &next, 1 }));
        }
    }

    for(auto const& token: tokens)
        std::cout << (int)token.type() << " = " << token.value() << " at " << token.location().line << ":" << token.location().column << std::endl;
    
    return tokens;
}

}
