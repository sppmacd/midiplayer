#include "Error.h"
#include "Logger.h"
#include "MIDIDevice.h"
#include "MIDIFile.h"
#include "MIDIPlayer.h"
#include "Resources.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string_view>
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
        std::cerr << "    -o                 Print render to stdout (may be c for rendering with ffmpeg)" << std::endl;
        std::cerr << "    -r                 Remove empty MIDI file if nothing was written (only for realtime mode)" << std::endl;
        std::cerr << "    --config-help      Print help for Config Files" << std::endl;
        std::cerr << "    --debug            Enable debug info rendering" << std::endl;
        std::cerr << "    --help             Print this message" << std::endl;
        std::cerr << "    --markers [file]   Enable markers; save them to `file` (add them with number keys)" << std::endl;
        std::cerr << "    --version          Print MIDIPlayer version" << std::endl;
    } else {
        std::cerr << "Use --help to print available options." << std::endl;
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

class ArgParser {
    using Result = Util::ErrorOr<void, std::string>;
    struct PositionalParameter {
        using Handler = std::function<Result(std::string_view argument)>;
        Handler handler;
        std::string_view name;
    };
    struct Option {
        using Handler = std::function<Result(std::string_view value)>;
        Handler handler;
        std::string_view name;
        bool is_boolean;
    };

public:
    explicit ArgParser(int argc, char* argv[])
    {
        for (int i = 0; i < argc; i++) {
            m_arguments.push_back(std::string_view(argv[i]));
        }
    }

    template<class T>
    void parameter(std::string_view name, T& target)
    {
        m_positional_parameters.push_back(parameter_handler(name, target));
    }

    template<class T>
    void parameter(std::string_view name, std::optional<T>& target)
    {
        m_optional_positional_parameters.push_back({
            .handler = [name, &target](std::string_view value) -> Result {
                T t;
                TRY(parameter_handler(name, t).handler(value));
                target = std::move(t);
                return {};
            },
            .name = name,
        });
    }

    template<class T>
    void option(std::string_view name, T& target)
    {
        m_options.insert({ name, option_handler(name, target) });
    }

    template<class T>
    void option(std::string_view name, std::optional<T>& target)
    {
        m_options.insert({
            name,
            Option {
                .handler = [name, &target](std::string_view value) -> Result {
                    T t;
                    TRY(option_handler(name, t).handler(value));
                    target = std::move(t);
                    return {};
                },
                .name = name,
                .is_boolean = std::is_same_v<T, bool>,
            },
        });
    }

    Result parse() const
    {
        int positional_arg_offset = 0;
        for (int i = 1; i < m_arguments.size(); i++) {
            if (m_arguments[i].starts_with("-")) {
                auto it = m_options.find(m_arguments[i]);
                if (it == m_options.end()) {
                    return fmt::format("Unknown option: '{}'", m_arguments[i]);
                }
                auto& option = it->second;
                if (option.is_boolean) {
                    TRY(it->second.handler(""));
                } else {
                    i++;
                    if (i >= m_arguments.size()) {
                        return fmt::format("'{}' requires an argument", option.name);
                    }
                    TRY(it->second.handler(m_arguments[i]));
                }
                continue;
            }

            auto& param = *TRY([&]() -> Util::ErrorOr<PositionalParameter const*, std::string> {
                int idx = positional_arg_offset;
                if (idx < m_positional_parameters.size()) {
                    return &m_positional_parameters[idx];
                }
                idx -= m_positional_parameters.size();
                if (idx < m_optional_positional_parameters.size()) {
                    return &m_optional_positional_parameters[idx];
                }
                return "Unused parameter(s)";
            }());
            positional_arg_offset++;

            TRY(param.handler(m_arguments[i]));
        }
        if (positional_arg_offset < m_positional_parameters.size()) {
            std::vector<std::string_view> out;
            std::ranges::transform(m_positional_parameters, std::back_inserter(out), [](auto& param) { return param.name; });
            return fmt::format("Missing required parameters: {}", fmt::join(out, ", "));
        }
        return {};
    }

private:
    static PositionalParameter parameter_handler(std::string_view name, int& target)
    {
        return {
            .handler = [name, &target](std::string_view param) -> Result {
                try {
                    target = std::stoi(std::string(param));
                    return {};
                } catch (...) {
                    return fmt::format("Failed to parse int for parameter '{}'", name);
                }
            },
            .name = name,
        };
    }

    static PositionalParameter parameter_handler(std::string_view name, std::string& target)
    {
        return {
            .handler = [name, &target](std::string_view param) -> Result {
                target = param;
                return {};
            },
            .name = name,
        };
    }

    static Option option_handler(std::string_view name, std::string& target)
    {
        return {
            .handler = [name, &target](std::string_view param) -> Result {
                target = param;
                return {};
            },
            .name = name,
            .is_boolean = false,
        };
    }

    static Option option_handler(std::string_view name, bool& target)
    {
        return {
            .handler = [name, &target](std::string_view param) -> Result {
                // Note: This handler will be called only if the option is given
                //       in arguments, so we can assume that it is true.
                target = true;
                return {};
            },
            .name = name,
            .is_boolean = true,
        };
    }

    std::vector<std::string_view> m_arguments;
    std::vector<PositionalParameter> m_positional_parameters;
    std::vector<PositionalParameter> m_optional_positional_parameters;
    std::map<std::string_view, Option> m_options;
};

int main(int argc, char* argv[])
{
    g_exec_name = argv[0];

    print_version();

    ArgParser parser { argc, argv };

    MIDIPlayer::Args args;

    std::optional<std::string> mode_string_opt;
    parser.parameter("mode", mode_string_opt);
    std::optional<std::string> filename;
    parser.parameter("filename", filename);

    parser.option("-c", args.config_file_path);
    bool headless = false;
    parser.option("-d", headless);
    parser.option("-f", args.force_overwrite);
    parser.option("-m", args.midi_output);
    parser.option("-o", args.render_to_stdout);
    parser.option("-r", args.remove_file_if_nothing_written);
    bool print_config_help = false;
    parser.option("--config-help", print_config_help);
    parser.option("--debug", args.should_render_debug_info_in_preview);
    bool help = false;
    parser.option("--help", help);
    parser.option("--markers", args.marker_file_name);
    bool version = false;
    parser.option("--version", version);

    auto result = parser.parse();
    if (result.is_error()) {
        logger::error("{}", result.release_error());
        print_usage_and_exit(Brief::Yes);
        return 1;
    }
    if (help) {
        print_usage_and_exit(Brief::No);
        return 0;
    }
    if (version) {
        print_version(Brief::No);
        return 0;
    }

    MIDIPlayer player;
    if (headless) {
        player.set_headless();
    }
    if (print_config_help) {
        player.print_config_help();
        return 0;
    }
    if (!mode_string_opt) {
        logger::error("No mode was given. You need to specify either 'play' or 'realtime'. See --help for details.");
        return 1;
    }
    std::string const& mode_string = *mode_string_opt;

    if (mode_string == "play") {
        args.mode = MIDIPlayer::Args::Mode::Play;
        if (!filename) {
            logger::error("Input filename required for play mode");
            return 1;
        }
        std::ifstream stream(std::string { *filename }, std::ios::binary);
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
                logger::error("Expected port number (Run `midieditor list-midi-devices` for a list of MIDI devices.)");
                return 1;
            }
        }
        if (!player.initialize(MIDIPlayer::RealTime::No, std::move(midi_file),
                args.midi_output.empty() ? nullptr : std::make_unique<MIDIDeviceOutput>(value)))
            return 1;
    } else if (mode_string == "realtime") {
        args.mode = MIDIPlayer::Args::Mode::Realtime;
        if (!filename) {
            logger::error("Input filename required for realtime mode");
            return 1;
        }
        int value = 0;
        try {
            // FIXME: stdc++, make consistent std::string_view support finally!!
            value = std::stoi(std::string { *filename });
        } catch (...) {
            logger::error("Port number must be an integer (Run `midieditor list-midi-devices` for a list of MIDI devices.)");
            return 1;
        }

        if (!args.force_overwrite && std::filesystem::exists(args.midi_output)) {
            logger::error("Output file '{}' already exists. Use -f flag to force overwrite.", args.midi_output);
            return 1;
        }

        if (!player.initialize(MIDIPlayer::RealTime::Yes, std::make_unique<MIDIDeviceInput>(value),
                args.midi_output.empty() ? nullptr : std::make_unique<MIDIFileOutput>(args.midi_output)))
            return 1;
    } else if (mode_string == "list-midi-devices") {
        MIDI::print_available_ports();
        return 0;
    } else {
        logger::error("Unknown mode: {}", mode_string);
        print_usage_and_exit();
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
