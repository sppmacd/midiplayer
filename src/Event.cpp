#include "Event.h"
#include "Config/Property.h"
#include "Logger.h"
#include "MIDIPlayer.h"
#include "RoundedEdgeRectangleShape.hpp"

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

void EndOfTrackEvent::render(MIDIPlayer&, sf::RenderTarget& target)
{
    sf::RectangleShape rs({ target.getView().getSize().x, 5 });
    rs.setFillColor(sf::Color::White);
    rs.setPosition(0, tick());
    target.draw(rs);
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

void NoteEvent::render(MIDIPlayer& player, sf::RenderTarget& target)
{
    // FIXME: Bring back color caching
    auto size = target.getView().getSize();
    const float scale = player.scale();

    auto is_visible = [&](float y_start, float y_size) {
        float real_y_start = (y_start + (player.real_time() ? -static_cast<int64_t>(player.current_tick()) : static_cast<int64_t>(player.current_tick()))) * scale;
        return !(real_y_start > 0 || real_y_start + y_size < -size.y);
    };

    auto render_note = [&](float y_start, float y_size, sf::Color color) {
        float real_y_start = (y_start + (player.real_time() ? -static_cast<int64_t>(player.current_tick()) : static_cast<int64_t>(player.current_tick()))) * scale;
        auto black = m_key.is_black();
        float tile_width = black ? 0.5 : 1;
        sf::Vector2f tile_size { tile_width, y_size };
        sf::Vector2f tile_position { m_key.to_piano_position(), real_y_start };
        sf::Vector2f extent { 50.f, 50.f };
        sf::RectangleShape rs { tile_size + extent };
        rs.setPosition(tile_position - extent / 2.f);
        rs.setFillColor(color);
        auto key_position = m_key.to_piano_position();
        auto& shader = player.note_shader();

        constexpr float TileSpacing = 2;

        auto physical_tile_position = target.mapCoordsToPixel(tile_position);
        physical_tile_position.x += TileSpacing;
        physical_tile_position.y += TileSpacing;
        auto physical_tile_end_position = target.mapCoordsToPixel(tile_position + tile_size);
        physical_tile_end_position.x -= TileSpacing;
        physical_tile_end_position.y -= TileSpacing;

        sf::Vector2f physical_tile_size { physical_tile_end_position - physical_tile_position };

        shader.setUniform("uKeySize", physical_tile_size);
        shader.setUniform("uKeyPos", sf::Vector2f { static_cast<float>(physical_tile_position.x), static_cast<float>(target.getSize().y - physical_tile_position.y - physical_tile_size.y) });
        shader.setUniform("uIsBlack", black);
        target.draw(rs, sf::RenderStates { &shader });
    };

    if (!player.real_time()) {
        auto& start_note = player.started_notes()[m_key];

        if (m_type == Type::Off) {
            if (start_note.has_value()) {
                int note_size_y = tick() - start_note.value().tick();
                auto y_start = size.y * scale - static_cast<int>(tick());
                auto y_size = note_size_y * scale;
                if (is_visible(y_start, y_size)) {
                    auto color = player.resolve_color(*this);
                    if (player.current_tick() > start_note.value().tick() && player.current_tick() < tick())
                        player.spawn_random_particles(target, m_key, color, start_note.value().velocity());
                    render_note(y_start, y_size, color);
                }
                player.started_notes()[m_key].reset();
            }
        } else if (!start_note.has_value())
            player.started_notes()[m_key] = *this;
    } else {
        auto end_note = player.ended_notes()[m_key];
        if (m_type == Type::On) {
            auto note_size_y = static_cast<float>(player.current_tick() - tick());
            if (end_note.has_value() && end_note.value().has_value())
                note_size_y = std::min(static_cast<float>(end_note.value().value().tick() - tick()), note_size_y);
            auto y_start = tick();
            auto y_size = note_size_y * scale;
            if (is_visible(y_start, y_size)) {
                auto color = player.resolve_color(*this);
                render_note(tick(), note_size_y * scale, color);
                if (!end_note.has_value() || !end_note.value().has_value())
                    player.spawn_random_particles(target, m_key, color, velocity());
            }
            if (end_note.has_value())
                player.ended_notes()[m_key].reset();
        } else if (!end_note.has_value())
            player.ended_notes()[m_key] = *this;
    }
}

void NoteEvent::execute(MIDIPlayer& player)
{
    player.set_sound_playing(m_key, m_velocity, m_type == Type::On ? true : false, player.resolve_color(*this));
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
