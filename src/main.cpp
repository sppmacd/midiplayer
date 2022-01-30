#include <cstring>
#include <fstream>

#include "ConfigFileReader.h"
#include "Logger.h"
#include "MIDIDevice.h"
#include "MIDIFile.h"
#include "MIDIPlayer.h"

#include <sys/stat.h>

using namespace std::literals;

std::string_view g_exec_name;

enum class Brief
{
    Yes,
    No
};

static void print_usage_and_exit(Brief brief = Brief::Yes)
{
    std::cerr << "Usage: " << g_exec_name << " [options...] [<realtime <MIDI device>>|<play <MIDI file>>]" << std::endl;
    if(brief == Brief::No)
    {
        std::cerr << "Options:" << std::endl;
        std::cerr << "    -m [file]       Save input to specified MIDI file (applicable only in realtime mode)" << std::endl;
        std::cerr << "    -o              Print render to stdout (may be used for rendering with ffmpeg)" << std::endl;
        std::cerr << "    --config-help   Print help for Config Files" << std::endl;
        std::cerr << "    --debug         Enable debug info rendering" << std::endl;
        std::cerr << "    --help          Print this message" << std::endl;
        std::cerr << "    --version       Print MIDIPlayer version" << std::endl;
    }
    exit(1);
}

const char* VERSION_STRING = "dev";

static void print_version(Brief brief = Brief::Yes)
{
    // This is based on gcc's --version output
    std::cerr << "\e[1m" << g_exec_name << " (MIDIPlayer) " << VERSION_STRING << "\e[0m" << std::endl;
    if(brief == Brief::No)
        std::cerr << "Copyright (C) 2021-2022 sppmacd" << std::endl;
}

enum class Mode
{
    Realtime,
    Play
};

int main(int argc, char* argv[])
{
    g_exec_name = argv[0];
    print_version();

    Mode mode {};
    bool print_config_help = false;
    bool render_to_stdout = false;
    bool should_render_debug_info_in_preview = false;
    std::string midi_output;
    std::string config_file_path = "";

    MIDIPlayer player;

    // FIXME: This is very messy. Factor it out to a nice API or just get a library for argsparse :)
    if(argc < 2)
        print_usage_and_exit(Brief::No);
    else
    {
        for(int i = 1; i < argc; i++)
        {
            std::string_view arg_sv { argv[i] };
            if(arg_sv.starts_with('-'))
            {
                std::string_view opt_sv = arg_sv.substr(1);
                if(opt_sv == "c"sv)
                {
                    if(++i == argc)
                    {
                        logger::error("-c requires an argument");
                        return 1;
                    }
                    config_file_path = argv[i];
                }
                else if(opt_sv == "m"sv)
                {
                    if(++i == argc)
                    {
                        logger::error("-m requires an argument");
                        return 1;
                    }
                    midi_output = argv[i];
                }
                else if(opt_sv == "o"sv)
                    render_to_stdout = true;
                else if(opt_sv == "-config-help")
                    print_config_help = true;
                else if(opt_sv == "-debug")
                    should_render_debug_info_in_preview = true;
                else if(opt_sv == "-help")
                    print_usage_and_exit(Brief::No);
                else
                    logger::warning("WARNING: Ignoring invalid option: {}", arg_sv);
            }
            else
            {
                if(player.is_initialized())
                {
                    logger::error("Only one mode may be active at once");
                    return 1;
                }

                if(arg_sv == "realtime"sv || arg_sv == "play"sv)
                {
                    if(++i >= argc)
                    {
                        logger::error("Expected file name for 'realtime' mode");
                        return 1;
                    }
                    std::string_view filename_sv { argv[i] };
                    struct stat s;
                    if(stat(argv[i], &s) < 0)
                    {
                        logger::error("Failed to stat file '{}': {}", filename_sv, strerror(errno));
                        return 1;
                    }
                    if(arg_sv == "realtime"sv)
                    {
                        if(!S_ISCHR(s.st_mode))
                        {
                            logger::error("Expected character device (e.g /dev/midi3), got {}", filename_sv);
                            return 1;
                        }

                        player.initialize(MIDIPlayer::RealTime::Yes, std::make_unique<MIDIDeviceInput>(std::string { filename_sv }),
                            midi_output.empty() ? nullptr : std::make_unique<MIDIFileOutput>(midi_output));
                    }
                    else if(arg_sv == "play"sv)
                    {
                        if(!S_ISREG(s.st_mode))
                        {
                            logger::error("Expected regular file, got {}", filename_sv);
                            return 1;
                        }
                        std::ifstream stream(std::string { filename_sv }, std::ios::binary);
                        if(stream.fail())
                        {
                            logger::error("Failed to open file");
                            return 1;
                        }
                        auto midi_file = std::make_unique<MIDIFileInput>(stream);
                        if(!midi_file->is_valid())
                        {
                            logger::error("Failed to read MIDI");
                            return 1;
                        }
                        midi_file->dump();
                        player.initialize(MIDIPlayer::RealTime::No, std::move(midi_file), nullptr);
                    }
                }
                else
                {
                    logger::error("Unknown mode: {}", arg_sv);
                    print_usage_and_exit();
                }
            }
        }
    }

    if(print_config_help)
    {
        player.print_config_help();
        return 0;
    }

    if(!player.is_initialized())
    {
        logger::error("No mode was given. You need to specify either 'play' or 'realtime'. See --help for details.");
        return 1;
    }

    if(config_file_path.empty())
        player.load_config_file("config.cfg");
    else if(!player.load_config_file(config_file_path))
    {
        logger::error("Failed to load config file: {}", config_file_path);
        return 1;
    }

    std::unique_ptr<sf::RenderTexture> render_texture = [&]() -> std::unique_ptr<sf::RenderTexture>
    {
        if(render_to_stdout)
        {
            if(isatty(STDOUT_FILENO))
            {
                logger::error("stdout is a terminal, refusing to print binary data");
                return nullptr;
            }

            auto texture = std::make_unique<sf::RenderTexture>();
            // TODO: Support custom resolution/FPS/format/...
            if(!texture->create(1920, 1080))
            {
                logger::error("Failed to create render texture, ignoring");
                return nullptr;
            }
            logger::info("Rendering to stdout (RGBA24 1920x1080 60fps)");
            if(mode == Mode::Realtime)
                logger::warning("Realtime mode is not recommended for rendering, consider recording it to MIDI file first and playing");
            return texture;
        }
        return nullptr;
    }();

    bool is_fullscreen = false;
    sf::RenderWindow window;
    auto create_windowed = [&]()
    {
        is_fullscreen = false;
        window.create(sf::VideoMode::getDesktopMode(), "MIDI Player", sf::Style::Default, sf::ContextSettings { 0, 0, 1 });
        if(!render_texture)
            window.setFramerateLimit(60);
        window.setMouseCursorVisible(true);
    };
    auto create_fullscreen = [&]()
    {
        is_fullscreen = true;
        window.create(sf::VideoMode::getDesktopMode(), "MIDI Player", sf::Style::Fullscreen, sf::ContextSettings { 0, 0, 1 });
        if(!render_texture)
            window.setFramerateLimit(60);
        window.setMouseCursorVisible(false);
    };
    create_windowed();

    sf::Clock fps_clock;
    sf::Time last_fps_time;
    while(player.playing())
    {
        {
            sf::Event event;
            while(window.pollEvent(event))
            {
                if(event.type == sf::Event::Closed)
                    player.set_playing(false);
                else if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F11)
                {
                    if(is_fullscreen)
                        create_windowed();
                    else
                        create_fullscreen();
                }
            }
        }
        player.update();
        // FIXME: Last FPS should be stored in MIDIPlayer somehow!
        player.render(window, { .full_info = should_render_debug_info_in_preview, .last_fps_time = last_fps_time });
        window.display();
        if(render_texture)
        {
            player.render(*render_texture, { .full_info = false, .last_fps_time = last_fps_time });
            render_texture->display();
            auto image = render_texture->getTexture().copyToImage();

            // NOTE: RGBA only!
            fwrite(image.getPixelsPtr(), 4, image.getSize().x * image.getSize().y, stdout);
        }

        last_fps_time = fps_clock.restart();
    }

    return 0;
}
