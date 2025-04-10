cmake_minimum_required(VERSION 3.17)
project(MIDIPlayer)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

option(MIDIPLAYER_PORTABLE_INSTALL "Generate local/portable installation, that will not use absolute paths.")

if(MIDIPLAYER_PORTABLE_INSTALL)
    message(STATUS "Creating portable installation. CMAKE_INSTALL_PREFIX will be overridden.")
    set(CMAKE_INSTALL_PREFIX root)
endif()

include(FetchContent)
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

find_package(SFML 3.0.0 COMPONENTS Graphics Audio REQUIRED)

add_executable(midiplayer
    src/Config/Action.cpp
    src/Config/Condition.cpp
    src/Config/Configuration.cpp
    src/Config/Info.cpp
    src/Config/Lexer.cpp
    src/Config/Parser.cpp
    src/Config/Property.cpp
    src/Config/Reader.cpp
    src/Config/Selector.cpp
    src/Config/Statement.cpp
    src/AnimatableBackground.cpp
    src/Event.cpp 
    src/FileWatcher.cpp
    src/MIDIDevice.cpp
    src/MIDIFile.cpp
    src/MIDIInput.cpp
    src/MIDIKey.cpp
    src/MIDIPlayer.cpp
    src/MIDIPlayerConfig.cpp
    src/Resources.cpp
    src/RoundedEdgeRectangleShape.cpp
    src/TileWorld.cpp
    src/Track.cpp
    src/main.cpp
)
target_compile_options(midiplayer PUBLIC -Werror -Wnon-virtual-dtor -fdiagnostics-color=always)
target_link_libraries(midiplayer pthread SFML::Graphics SFML::Audio fmt rtmidi)
target_include_directories(midiplayer PUBLIC ${CMAKE_BINARY_DIR}/src)
install(TARGETS midiplayer DESTINATION bin)
if(MIDIPLAYER_PORTABLE_INSTALL)
    # This is a big HACK to support running executable from `bin` for local installations (but idk the proper solution)
    install(DIRECTORY res DESTINATION ".")
    set(MIDIPLAYER_RESOURCE_DIR "../res")
else()
    install(DIRECTORY res/ DESTINATION ${CMAKE_INSTALL_DATADIR}/midiplayer)
    set(MIDIPLAYER_RESOURCE_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/midiplayer)
endif()
message(STATUS "Using resource path: '${MIDIPLAYER_RESOURCE_DIR}'")
configure_file(src/ResourceDir.h.in src/ResourceDir.h)
