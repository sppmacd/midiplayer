#include <cstring>
#include <fstream>

#include "MIDIDevice.h"
#include "MIDIFile.h"
#include "MIDIPlayer.h"

#include <sstream>
#include <sys/stat.h>

using namespace std::literals;

std::string_view g_exec_name;

enum class BriefUsage
{
    Yes,
    No
};

static void print_usage_and_exit(BriefUsage brief_usage = BriefUsage::Yes)
{
    std::cerr << "Usage: " << g_exec_name << " [options...] [<realtime <MIDI device>>|<play <MIDI file>>]" << std::endl;
    if(brief_usage == BriefUsage::No)
    {
        std::cerr << "Options:" << std::endl;
        std::cerr << "    -o    Print render to stdout (may be used for rendering with ffmpeg)" << std::endl;
    }
    exit(1);
}

enum class Mode {
    Realtime,
    Play
};

int main(int argc, char* argv[])
{
    g_exec_name = argv[0];
    std::unique_ptr<MIDI> midi;

    Mode mode {};
    bool render_to_stdout = false;

    if(argc < 2)
        print_usage_and_exit(BriefUsage::No);
    else
    {
        for(int i = 1; i < argc; i++)
        {
            std::string_view arg_sv { argv[i] };
            if(arg_sv.starts_with('-'))
            {
                std::string_view opt_sv = arg_sv.substr(1);
                if(opt_sv == "o"sv)
                    render_to_stdout = true;
                else
                    std::cerr << "WARNING: Ignoring invalid option: " << arg_sv << std::endl;
            }
            else
            {
                if(midi)
                {
                    std::cerr << "ERROR: Only one mode may be active at once" << std::endl;
                    return 1;
                }

                if(arg_sv == "realtime"sv || arg_sv == "play"sv)
                {
                    if(++i >= argc)
                    {
                        std::cerr << "ERROR: Expected file name for 'realtime' mode" << std::endl;
                        return 1;
                    }
                    std::string_view filename_sv { argv[i] };
                    struct stat s;
                    if(stat(argv[i], &s) < 0)
                    {
                        std::cerr << "ERROR: Failed to stat file " << filename_sv << std::endl;
                        return 1;
                    }
                    if(arg_sv == "realtime"sv)
                    {
                        if(!S_ISCHR(s.st_mode))
                        {
                            std::cerr << "ERROR: Expected character device (e.g /dev/midi3), got " << filename_sv << std::endl;
                            return 1;
                        }
                        midi = std::make_unique<MIDIDevice>(std::string{filename_sv});
                        mode = Mode::Realtime;
                    }
                    else if(arg_sv == "play"sv)
                    {
                        if(!S_ISREG(s.st_mode))
                        {
                            std::cerr << "ERROR: Expected regular file, got " << filename_sv << std::endl;
                            return 1;
                        }
                        std::ifstream stream(std::string{filename_sv}, std::ios::binary);
                        if(stream.fail())
                        {
                            std::cerr << "Failed to open file." << std::endl;
                            return 1;
                        }
                        auto midi_file = std::make_unique<MIDIFile>(stream);
                        if(!midi_file->is_valid())
                        {
                            std::cerr << "Failed to read MIDI" << std::endl;
                            return 1;
                        }    
                        midi_file->dump();
                        mode = Mode::Play;
                        midi = std::move(midi_file);
                    }
                }
                else
                {
                    std::cerr << "ERROR: Unknown mode" << std::endl;
                    print_usage_and_exit();
                }
            }
        }
    }

    sf::RenderWindow window(sf::VideoMode(500, 500), "MIDI Player");

    std::unique_ptr<sf::RenderTexture> render_texture = [&]()->std::unique_ptr<sf::RenderTexture> {
        if(render_to_stdout)
        {
            if(isatty(STDOUT_FILENO))
            {
                std::cerr << "ERROR: stdout is a terminal, refusing to print binary data" << std::endl;
                return nullptr;
            }

            auto texture = std::make_unique<sf::RenderTexture>();
            // TODO: Support custom resolution/FPS/format/...
            if(!texture->create(1920, 1080))
            {
                std::cerr << "ERROR: Failed to create render texture, ignoring" << std::endl;
                return nullptr;
            }
            std::cerr << "INFO: Rendering to stdout (RGBA24 1920x1080 60fps)" << std::endl;
            if(mode == Mode::Realtime)
                std::cerr << "WARNING: Realtime mode is not recommended for rendering, consider recording it to MIDI file first and playing" << std::endl;
            return texture;
        }
        return nullptr;
    }();

    if(!render_texture)
        window.setFramerateLimit(60);

    sf::Font font;
    font.loadFromFile("res/font.ttf");

    MIDIPlayer player{*midi, mode == Mode::Realtime ? MIDIPlayer::RealTime::Yes : MIDIPlayer::RealTime::No};
    sf::Clock fps_clock;
    sf::Time last_fps_time;
    while(player.playing())
    {
        {
            sf::Event event;
            while(window.pollEvent(event))
            {
            }
        }
        player.update();

        auto render = [&](sf::RenderTarget& target, bool preview) {
            target.clear();

            float aspect = static_cast<float>(target.getSize().x) / target.getSize().y;
            constexpr float piano_size_px = 100.f; // TODO: Make it configurable??
            const float piano_size = piano_size_px * (128.f / aspect) / target.getSize().y;
            auto view = sf::View{sf::FloatRect(0, -128.f / aspect + piano_size, 128.f, 128.f / aspect)};
            target.setView(view);
            if(mode == Mode::Realtime)
            {
                player.ended_notes().clear();
                midi->for_each_event_backwards([&](Event& event) {
                    event.render(player, target);
                });
            }
            else
            {
                player.started_notes().clear();
                midi->for_each_event([&](Event& event) {
                    event.render(player, target);
                });
            }
            player.render_particles(target);
            player.render_piano(target);
            {
                sf::Vector2f target_size {target.getSize()};
                target.setView(sf::View({0, 0, target_size.x, target_size.y}));
                std::ostringstream oss;
                oss << player.current_tick();
                if(preview)
                    oss << "  " + std::to_string(1.f / last_fps_time.asSeconds()) + " fps";
                sf::Text text{oss.str(), font, 10};
                text.setPosition(5, 5);
                target.draw(text);
            }
        };
        render(window, true);
        window.display();
        if(render_texture)
        {
            render(*render_texture, false);
            render_texture->display();
            auto image = render_texture->getTexture().copyToImage();

            // NOTE: RGBA only!
            fwrite(image.getPixelsPtr(), 4, image.getSize().x * image.getSize().y, stdout);
        }

        last_fps_time = fps_clock.restart();
    }

    return 0;
}
