#include "Action.h"

#include "../Logger.h"
#include "../MIDIInput.h"
#include "../MIDIPlayer.h"
#include "Reader.h"
#include "Statement.h"

namespace Config
{

void SetAction::execute(Reader& reader) const
{
    logger::info("Executing SetAction with {} statements", m_statements.size());
    reader.push_transition(m_transition);
    for(auto& it : m_statements)
        it->execute(reader);
    reader.pop_transition();
}

void AddEventAction::execute(Reader& reader) const
{
    // NOTE: This adds events directly to input, so that it will
    //       be both displayed and sent to MIDI output!
    auto input = reader.player().midi_input();
    if(!input)
    {
        logger::warning("No MIDI input to add event to!");
        return;
    }
    if(input->track_count() < 1)
    {
        logger::warning("No track to add event to!");
        return;
    }
    auto new_event = m_event->clone();

    // NOTE: We need to offset tick by 1 because the event won't be executed
    //       if it is added in the current tick because events are updated
    //       before actions.
    new_event->set_tick(reader.player().current_tick() + 1);
    input->track(0).add_event(std::move(new_event));
}

}
