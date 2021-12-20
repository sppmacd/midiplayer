project(MIDI)
cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 20)
find_package(SFML 2.5.1 COMPONENTS graphics audio REQUIRED)
add_executable(midi
    src/Event.cpp 
    src/main.cpp
    src/MIDIDevice.cpp
    src/MIDIFile.cpp
    src/MIDIInput.cpp
    src/MIDIKey.cpp
    src/MIDIPlayer.cpp
    src/Selector.cpp
    src/Track.cpp
)
target_link_libraries(midi pthread sfml-graphics sfml-audio)
