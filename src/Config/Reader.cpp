#include "Reader.h"

#include "../Logger.h"
#include "../MIDIPlayer.h"

#include <fstream>
#include <iostream>

namespace Config
{

void Reader::clear()
{
    m_conditional_actions.clear();
}

void Reader::update()
{
    for(auto const& action : m_conditional_actions)
    {
        if(action.condition->is_met(*this))
            action.action->execute(*this);
    }
    auto player_frame = m_player.current_frame();
    for(auto const& transition : m_transitions)
        transition.handler((double)(player_frame - transition.start_frame) / transition.length);
    std::erase_if(m_transitions, [&](auto const& e)
        { return player_frame >= e.start_frame + e.length; });
}

void Reader::add_transition(Time time, std::function<void(double)> handler)
{
    logger::info("Adding transition");
    Transition transition;
    transition.handler = std::move(handler);
    transition.start_frame = m_player.current_frame();
    transition.length = m_player.frame_count_for_time(time);
    m_transitions.push_back(std::move(transition));
}

}
