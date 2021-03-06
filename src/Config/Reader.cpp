#include "Reader.h"

#include "../Logger.h"
#include "../MIDIPlayer.h"

#include <fstream>
#include <iostream>

namespace Config {

void Reader::clear()
{
    m_conditional_actions.clear();
    m_periodic_actions.clear();
}

static double apply_timing_function(double x, Transition::Function function)
{
    switch (function) {
        case Transition::Function::Constant0:
            return 0;
        case Transition::Function::Constant1:
            return 1;
        case Transition::Function::Linear:
            return x;
        case Transition::Function::EaseInOutQuad:
            return x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;
        default:
            return x;
    }
}

void Reader::register_conditional_action(std::shared_ptr<Condition> condition, std::shared_ptr<Action> action)
{
    m_conditional_actions.push_back({ condition, action, m_player.current_frame() });

    // Also try to execute because some actions have its condition met
    // in the tick they were added (e.g on[time=0s])
    if (!m_player.is_in_loop())
        return;

    m_time_offset = m_player.current_frame();
    if (condition->is_met(*this))
        action->execute(*this);
}

void Reader::register_periodic_action(Time interval, std::shared_ptr<Action> action)
{
    m_periodic_actions.push_back({ interval, action, m_player.current_frame() });
}

void Reader::update()
{
    auto player_frame = m_player.current_frame();

    // NOTE: We need to actually execute in separate pass because it may
    //       modify the array!
    std::list<Action*> actions_to_run;
    for (auto const& action : m_conditional_actions) {
        m_time_offset = action.add_frame;
        if (action.condition->is_met(*this))
            actions_to_run.push_back(action.action.get());
    }

    for (auto const& action : m_periodic_actions) {
        if (m_player.is_in_interval_frame(action.interval, action.add_frame))
            actions_to_run.push_back(action.action.get());
    }

    for (auto action : actions_to_run)
        action->execute(*this);

    for (auto const& transition : m_ongoing_transitions) {
        auto raw_factor = (double)(player_frame - transition.start_frame) / transition.length;
        // NOTE: AnimatableProperty uses '1' value as indication that transition
        //       has finished, so it needs to be applied even if timing functions
        //       says otherwise.
        transition.handler(raw_factor == 1 ? 1 : apply_timing_function(raw_factor, transition.function));
    }
    std::erase_if(m_ongoing_transitions, [&](auto const& e) { return player_frame >= e.start_frame + e.length; });
    std::erase_if(m_conditional_actions, [&](auto const& action) {
        m_time_offset = action.add_frame;
        return action.condition->has_expired(*this);
    });
}

void Reader::add_transition(Transition transition, std::function<void(double)> handler)
{
    logger::info("Adding transition");
    OngoingTransition ongoing_transition {
        .function = transition.function(),
        .start_frame = m_player.current_frame(),
        .length = m_player.frame_count_for_time(transition.time(), 0),
        .handler = std::move(handler),
    };
    m_ongoing_transitions.push_back(ongoing_transition);
}

void Reader::dump_stats(std::ostream& out) const
{
    out << "ConditionalActions: " << m_conditional_actions.size() << std::endl;
    out << "PeriodicActions: " << m_periodic_actions.size() << std::endl;
    out << "OngoingTransitions: " << m_ongoing_transitions.size() << std::endl;
}

}
