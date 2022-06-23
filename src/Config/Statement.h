#pragma once

#include <memory>
#include <vector>

#include "Condition.h"
#include "Property.h"

namespace Config {

class Reader;

class Statement {
public:
    virtual ~Statement() = default;
    virtual bool execute(Reader& reader) const = 0;
};

class PropertyStatement : public Statement {
public:
    PropertyStatement(std::string name, std::vector<PropertyParameter> args)
        : m_name(std::move(name))
        , m_args(std::move(args))
    {
    }

    virtual bool execute(Reader& reader) const override;

private:
    std::string m_name;
    std::vector<PropertyParameter> m_args;
};

class Action;

class OnStatement : public Statement {
public:
    OnStatement(std::shared_ptr<Condition> condition, std::shared_ptr<Action> action)
        : m_condition(std::move(condition))
        , m_action(std::move(action))
    {
    }

    virtual bool execute(Reader& reader) const override;

private:
    std::shared_ptr<Condition> m_condition;
    std::shared_ptr<Action> m_action;
};

class EveryStatement : public Statement {
public:
    EveryStatement(Time interval, std::shared_ptr<Action> action)
        : m_interval(std::move(interval))
        , m_action(std::move(action))
    {
    }

    virtual bool execute(Reader& reader) const override;

private:
    Time m_interval;
    std::shared_ptr<Action> m_action;
};

class IfStatement : public Statement {
public:
    IfStatement(std::shared_ptr<Condition> condition, std::shared_ptr<Action> action)
        : m_condition(std::move(condition))
        , m_action(std::move(action))
    {
    }

    virtual bool execute(Reader& reader) const override;

private:
    std::shared_ptr<Condition> m_condition;
    std::shared_ptr<Action> m_action;
};

}
