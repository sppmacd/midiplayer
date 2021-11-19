#include <cstring>
#include <fstream>

#include "MIDIDevice.h"
#include "MIDIFile.h"
#include "MIDIPlayer.h"

#include <sys/stat.h>

int main(int argc, char* argv[])
{
    bool real_time = false;
    std::unique_ptr<MIDI> midi;
    if(argc < 2)
    {
        std::cout << "Usage: " << basename(argv[0]) << " [path to MIDI file or MIDI device]" << std::endl;
        return 1;
    }
    else
    {
        struct stat s;
        if(stat(argv[1], &s) < 0)
        {
            std::cout << "Failed to stat file" << std::endl;
            return 1;
        }
        
        if(S_ISCHR(s.st_mode))
        {
            midi = std::make_unique<MIDIDevice>(argv[1]);
            real_time = true;
        }
        else if(S_ISREG(s.st_mode))
        {
            std::ifstream stream(argv[1], std::ios::binary);
            if(stream.fail())
            {
                std::cout << "Failed to open file." << std::endl;
                return 1;
            }
            auto midi_file = std::make_unique<MIDIFile>(stream);
            if(!midi_file->is_valid())
            {
                std::cout << "Failed to read MIDI" << std::endl;
                return 1;
            }    
            midi_file->dump();
            real_time = false;
            midi = std::move(midi_file);
        }
        else
        {
            std::cout << "Invalid file type (should be regular file or character device)" << std::endl;
            return 1;
        }
    }

    sf::RenderWindow window(sf::VideoMode(500, 500), "MIDI Player");
    window.setFramerateLimit(60);

    sf::Font font;
    font.loadFromFile("res/font.ttf");

    MIDIPlayer player{*midi, real_time ? MIDIPlayer::RealTime::Yes : MIDIPlayer::RealTime::No};
    sf::Clock fps_clock;
    while(player.playing())
    {
        {
            sf::Event event;
            while(window.pollEvent(event))
            {
            }
        }
        player.update();
        window.clear();

        float aspect = static_cast<float>(window.getSize().x) / window.getSize().y;
        auto view = sf::View{sf::FloatRect(0, -128.f / aspect, 128.f, 128.f / aspect)};
        window.setView(view);

        if(real_time)
        {
            player.ended_notes().clear();
            midi->for_each_event_backwards([&](Event& event) {
                event.render(player, window);
            });
        }
        else
        {
            player.started_notes().clear();
            midi->for_each_event([&](Event& event) {
                event.render(player, window);
            });
        }
        player.render_particles(window);
        {
            sf::Vector2f window_size {window.getSize()};
            window.setView(sf::View({0, 0, window_size.x, window_size.y}));
            sf::Text text{std::to_string(player.current_tick()) + "  " + std::to_string(1.f / fps_clock.restart().asSeconds()) + " fps", font, 10};
            text.setPosition(5, 5);
            window.draw(text);
        }
        window.display();
    }

    return 0;
}
