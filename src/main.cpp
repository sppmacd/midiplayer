#include "Logger.h"
#include "MIDIDevice.h"
#include "MIDIFile.h"
#include "MIDIPlayer.h"
#include "Resources.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

using namespace std::literals;

std::string_view g_exec_name;

enum class Brief {
    Yes,
    No
};

static void print_usage_and_exit(Brief brief = Brief::Yes)
{
    std::cerr << "Usage: " << g_exec_name << " [options...] [list-midi-devices|<play <MIDI file>|<realtime <MIDI device>>>]" << std::endl;
    if (brief == Brief::No) {
        std::cerr << "Modes:" << std::endl;
        std::cerr << "    list-midi-devices  Print a list of MIDI input/output devices." << std::endl;
        std::cerr << "    play [file]        Play pre-recorded .mid file. Supports output to MIDI device with -m option." << std::endl;
        std::cerr << "    realtime [port]    Display MIDI device input in realtime. Supports output to .mid file with -m option." << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "    -c [path]          Specify alternative config file" << std::endl;
        std::cerr << "    -d                 Headless mode (don't open a window, works in text mode)" << std::endl;
        std::cerr << "    -f                 Force overwriting output files" << std::endl;
        std::cerr << "    -m [file/port]     Specify MIDI output (file in realtime mode, port number in play mode)" << std::endl;
        std::cerr << "    -o                 Print render to stdout (may be used for rendering with ffmpeg)" << std::endl;
        std::cerr << "    -r                 Remove empty MIDI file if nothing was written (only for realtime mode)" << std::endl;
        std::cerr << "    --config-help      Print help for Config Files" << std::endl;
        std::cerr << "    --debug            Enable debug info rendering" << std::endl;
        std::cerr << "    --help             Print this message" << std::endl;
        std::cerr << "    --markers [file]   Enable markers; save them to `file` (add them with number keys)" << std::endl;
        std::cerr << "    --version          Print MIDIPlayer version" << std::endl;
    }
    exit(1);
}

const char* VERSION_STRING = "dev";

static void print_version(Brief brief = Brief::Yes)
{
    // This is based on gcc's --version output
    if (brief == Brief::No)
        std::cerr << "Copyright (C) 2021-2023 sppmacd" << std::endl;
    else
        std::cerr << "\e[1m" << g_exec_name << " (MIDIPlayer) " << VERSION_STRING << "\e[0m (" << __DATE__ << " " << __TIME__ << ")" << std::endl;
}

int main(int argc, char* argv[])
{
    g_exec_name = argv[0];

    print_version();

    MIDIPlayer::Args args;
    MIDIPlayer player;

    // FIXME: This is very messy. Factor it out to a nice API or just get a library for argsparse :)
    if (argc < 2)
        print_usage_and_exit(Brief::No);
    else {
        for (int i = 1; i < argc; i++) {
            std::string_view arg_sv { argv[i] };
            if (arg_sv.starts_with('-')) {
                std::string_view opt_sv = arg_sv.substr(1);
                if (opt_sv == "c"sv) {
                    if (++i == argc) {
                        logger::error("-c requires an argument");
                        return 1;
                    }
                    args.config_file_path = argv[i];
                } else if (opt_sv == "d"sv) {
                    player.set_headless();
                } else if (opt_sv == "f"sv) {
                    args.force_overwrite = true;
                } else if (opt_sv == "m"sv) {
                    if (++i == argc) {
                        logger::error("-m requires an argument");
                        return 1;
                    }
                    args.midi_output = argv[i];
                } else if (opt_sv == "o"sv) {
                    args.render_to_stdout = true;
                } else if (opt_sv == "r"sv) {
                    args.remove_file_if_nothing_written = true;
                } else if (opt_sv == "-config-help") {
                    args.print_config_help = true;
                } else if (opt_sv == "-debug") {
                    args.should_render_debug_info_in_preview = true;
                } else if (opt_sv == "-help") {
                    print_usage_and_exit(Brief::No);
                } else if (opt_sv == "-markers") {
                    if (++i == argc) {
                        logger::error("--markers requires an argument");
                        return 1;
                    }
                    args.marker_file_name = argv[i];
                } else if (opt_sv == "-version") {
                    print_version(Brief::No);
                    return 0;
                } else
                    logger::warning("WARNING: Ignoring invalid option: {}", arg_sv);
            } else {
                if (player.is_initialized()) {
                    logger::error("Only one mode may be active at once");
                    return 1;
                }

                if (arg_sv == "realtime"sv || arg_sv == "play"sv) {
                    if (++i >= argc) {
                        logger::error("Expected name for {} mode", arg_sv);
                        return 1;
                    }
                    std::string_view filename_sv { argv[i] };
                    if (arg_sv == "realtime"sv) {
                        int value = 0;
                        try {
                            // FIXME: stdc++, make consistent std::string_view support finally!!
                            value = std::stoi(std::string { filename_sv });
                        } catch (...) {
                            logger::error("Expected port number. See midieditor list-midi-devices for a list of MIDI devices.");
                            return 1;
                        }

                        if (!args.force_overwrite && std::filesystem::exists(args.midi_output)) {
                            logger::error("Output file '{}' already exists. Use -f flag to force overwrite.", args.midi_output);
                            return 1;
                        }

                        if (!player.initialize(MIDIPlayer::RealTime::Yes, std::make_unique<MIDIDeviceInput>(value),
                                args.midi_output.empty() ? nullptr : std::make_unique<MIDIFileOutput>(args.midi_output)))
                            return 1;
                    } else if (arg_sv == "play"sv) {
                        std::ifstream stream(std::string { filename_sv }, std::ios::binary);
                        if (stream.fail()) {
                            logger::error("Failed to open file");
                            return 1;
                        }
                        auto midi_file = std::make_unique<MIDIFileInput>(stream);
                        if (!midi_file->is_valid()) {
                            logger::error("Failed to read MIDI");
                            return 1;
                        }

                        midi_file->for_each_track([&player](auto const& track) {
                            player.did_read_events(track.events().size());
                        });

                        int value = 0;
                        if (!args.midi_output.empty()) {
                            try {
                                value = std::stoi(args.midi_output);
                            } catch (...) {
                                logger::error("Expected port number. See midieditor list-midi-devices for a list of MIDI devices.");
                                return 1;
                            }
                        }
                        if (!player.initialize(MIDIPlayer::RealTime::No, std::move(midi_file),
                                args.midi_output.empty() ? nullptr : std::make_unique<MIDIDeviceOutput>(value)))
                            return 1;
                    }
                } else if (arg_sv == "list-midi-devices"sv) {
                    MIDI::print_available_ports();
                    return 0;
                } else {
                    logger::error("Unknown mode: {}", arg_sv);
                    print_usage_and_exit();
                }
            }
        }
    }

    if (args.print_config_help) {
        player.print_config_help();
        return 0;
    }

    if (!player.is_initialized()) {
        logger::error("No mode was given. You need to specify either 'play' or 'realtime'. See --help for details.");
        return 1;
    }

    player.setup();

    if (args.config_file_path.empty())
        player.load_config_file("config.cfg");
    else if (!player.load_config_file(args.config_file_path)) {
        logger::error("Failed to load config file: {}", args.config_file_path);
        return 1;
    }
    player.run(args);

    if (args.remove_file_if_nothing_written && player.real_time() && !args.midi_output.empty() && player.midi_input()->track(0).events().empty()) {
        logger::info("No events recorded, removing empty file.");
        std::filesystem::remove(args.midi_output);
    }

    return 0;
}
