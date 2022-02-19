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

    // FIXME: Turn this into real "execution context"?
    void push_transition(Transition transition) { m_transition_stack.push(transition); }
    void pop_transition() { m_transition_stack.pop(); }
    Transition current_transition() { return m_transition_stack.top(); }
    bool has_transition() const { return !m_transition_stack.empty(); }

    void add_transition(Transition transition, std::function<void(double)> handler);

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
    std::stack<Transition> m_transition_stack;

    struct OngoingTransition
    {
        Transition::Function function;
        size_t start_frame {};
        size_t length {};
        std::function<void(double)> handler;
    };

    std::vector<OngoingTransition> m_ongoing_transitions;
};

}