#include "Event.h"
#include "Config/Property.h"
#include "Logger.h"
#include "MIDIPlayer.h"
#include "RoundedEdgeRectangleShape.hpp"
#include "TileWorld.hpp"

#include <cstddef>
#include <optional>
#include <random>
#include <type_traits>

std::unique_ptr<Event> Event::create_event_from_name(std::string_view name)
{
    if (name == "Text")
        return std::make_unique<TextEvent>();
    return nullptr;
}

void TextEvent::execute(MIDIPlayer& player)
{
    switch (m_type) {
        case Type::TrackName:
            player.display_label(MIDIPlayer::LabelType::TrackName, m_text, 180);
            break;
        case Type::Lyric:
        case Type::Text:
        case Type::Copyright:
        case Type::Instrument:
        case Type::Marker:
        case Type::CuePoint:
            // TODO
            break;
    }
}

Config::NamedFormalParameters TextEvent::formal_parameters() const
{
    return {
        { "type", Config::PropertyFormalParameter(Config::PropertyType::String, "type") },
        { "text", Config::PropertyFormalParameter(Config::PropertyType::String, "text") },
    };
}

bool TextEvent::read_from_parameters(Config::NamedParameters const& params)
{
    auto type = params.find("type");
    if (type != params.end()) {
        if (type->first != "track_name") {
            logger::error("The only supported TextEvent type is 'track_name'");
            return false;
        }
    }
    auto text = params.find("text");
    if (text == params.end()) {
        logger::error("required argument: 'text'");
        return false;
    }
    m_text = text->second.as_string();
    m_type = Type::TrackName;
    return true;
}

void SetTempoEvent::execute(MIDIPlayer& player)
{
    player.set_tempo(m_microseconds_per_quarter_note);
}

void NoteEvent::execute(MIDIPlayer& player)
{
    if (player.real_time()) {
        player.tile_world().push_note_event(*this);
    }
    player.set_sound_playing(m_key, m_velocity, m_type == Type::On ? true : false,
        player.resolve_color(Tile { tick(), {}, transition_unit() }));
}

void ControlChangeEvent::execute(MIDIPlayer& player)
{
    switch (m_number) {
        case Number::DamperPedal:
            player.pedals().set_sustain(m_value > 0);
            break;
        case Number::Portamento:
            break;
        case Number::Sostenuto:
            player.pedals().set_sostenuto(m_value > 0);
            break;
        case Number::SoftPedal:
            player.pedals().set_soft(m_value > 0);
            break;
        case Number::LegatoFootswitch:
        case Number::Hold2:
        case Number::SoundController1:
        case Number::SoundController2:
        case Number::SoundController3:
        case Number::SoundController4:
        case Number::SoundController5:
        case Number::SoundController6:
        case Number::SoundController7:
        case Number::SoundController8:
        case Number::SoundController9:
        case Number::SoundController10:
        case Number::GeneralPurposeController5:
        case Number::GeneralPurposeController6:
        case Number::GeneralPurposeController7:
        case Number::GeneralPurposeController8:
        case Number::PortamentoControl:
        case Number::Effects1Depth:
        case Number::Effects2Depth:
        case Number::Effects3Depth:
        case Number::Effects4Depth:
        case Number::Effects5Depth:
        case Number::DataEntryPlus1:
        case Number::DataEntryMinus1:
        case Number::NonRegisteredParameterNumberLSB:
        case Number::NonRegisteredParameterNumberMSB:
        case Number::RegisteredParameterNumberLSB:
        case Number::RegisteredParameterNumberMSB:
        case Number::AllSoundOff:
        case Number::ResetAllControllers:
        case Number::LocalControlOnOff:
        case Number::AllNotesOff:
        case Number::OmniModeOff:
        case Number::OmniModeOn:
        case Number::PolyModeOn:
        case Number::PolyModeOnInclMono:
        case Number::Count:
            break;
    }
}
