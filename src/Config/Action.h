#pragma once

#include "Property.h"
#include "Statement.h"
#include "Transition.h"

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
    SetAction(std::vector<std::unique_ptr<Statement>> statements, Transition transition)
    : m_statements(std::move(statements)), m_transition(transition) {}

    virtual void execute(Reader&) const override;

private:
    std::vector<std::unique_ptr<Statement>> m_statements;
    Transition m_transition;
};

}
