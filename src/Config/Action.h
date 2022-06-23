#pragma once

#include "../Event.h"
#include "Property.h"
#include "Statement.h"
#include "Transition.h"

namespace Config {

class Statement;
class Reader;

class Action {
public:
    virtual ~Action() = default;
    virtual void execute(Reader&) const = 0;
};

class SetAction : public Action {
public:
    SetAction(std::vector<std::unique_ptr<Statement>> statements, Transition transition)
        : m_statements(std::move(statements))
        , m_transition(transition)
    {
    }

    virtual void execute(Reader&) const override;

private:
    std::vector<std::unique_ptr<Statement>> m_statements;
    Transition m_transition;
};

class AddEventAction : public Action {
public:
    explicit AddEventAction(std::unique_ptr<Event> event)
        : m_event(std::move(event))
    {
    }

    virtual void execute(Reader&) const override;

private:
    std::unique_ptr<Event> m_event;
};

}
