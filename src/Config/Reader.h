#pragma once

#include "Action.h"
#include "Condition.h"
#include "Info.h"
namespace Config
{

class Reader
{
public:
    explicit Reader(Info& info, MIDIPlayer& player)
    : m_info(info), m_player(player) {}

    Info& info() { return m_info; }
    MIDIPlayer& player() { return m_player; }

    void clear();

    void register_conditional_actions(std::shared_ptr<Condition> condition, std::shared_ptr<Action> action)
    {
        m_conditional_actions.push_back({ condition, action });
    }

    void update();

private:
    struct ConditionalAction
    {
        std::shared_ptr<Condition> condition;
        std::shared_ptr<Action> action;
    };

    Info& m_info;
    MIDIPlayer& m_player;
    std::vector<ConditionalAction> m_conditional_actions;
};

}
