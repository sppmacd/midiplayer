#pragma once

#include "Property.h"
#include "Statement.h"

namespace Config
{

class Statement;
class Reader;

class Action
{
public:
    virtual void execute(Reader&) const = 0;
};

class SetAction : public Action
{
public:
    SetAction(std::vector<std::unique_ptr<Statement>> statements, Time transition_time)
    : m_statements(std::move(statements)), m_transition_time(transition_time) {}

    virtual void execute(Reader&) const override;

private:
    std::vector<std::unique_ptr<Statement>> m_statements;
    Time m_transition_time;
};

}
