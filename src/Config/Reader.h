#pragma once

#include "Action.h"
#include "Condition.h"
#include "Info.h"

#include <stack>

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

    // FIXME: Turn this into real "execution context"
    void push_transition_time(Time time) { m_transition_times.push(time); }
    void pop_transition_time() { m_transition_times.pop(); }
    Time current_transition_time() { return m_transition_times.top(); }
    bool has_transition_time() const { return !m_transition_times.empty(); }

    void add_transition(Time time, std::function<void(double)> handler);

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
    std::stack<Time> m_transition_times;

    struct Transition
    {
        size_t start_frame {};
        size_t length {};
        std::function<void(double)> handler;
    };

    std::vector<Transition> m_transitions;
};

}
