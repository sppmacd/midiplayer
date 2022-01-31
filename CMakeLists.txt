project(MIDIPlayer)
cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

option(MIDIPLAYER_PORTABLE_INSTALL "Generate local/portable installation, that will not use absolute paths.")

if(MIDIPLAYER_PORTABLE_INSTALL)
    message(STATUS "Creating portable installation. CMAKE_INSTALL_PREFIX will be overridden.")
    set(CMAKE_INSTALL_PREFIX root)
endif()

find_package(fmt 8.0.0 REQUIRED)
find_package(SFML 2.5.1 COMPONENTS graphics audio REQUIRED)

add_executable(midiplayer
    src/Config.cpp
    src/Config/Parser.cpp
    src/Config/Selector.cpp
    src/Event.cpp 
    src/FileWatcher.cpp
    src/MIDIDevice.cpp
    src/MIDIFile.cpp
    src/MIDIInput.cpp
    src/MIDIKey.cpp
    src/MIDIPlayer.cpp
    src/Resources.cpp
    src/RoundedEdgeRectangleShape.cpp
    src/Track.cpp
    src/main.cpp
)
target_link_libraries(midiplayer pthread sfml-graphics sfml-audio fmt rtmidi)
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
