project(MIDIPlayer)
cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 20)
include(GNUInstallDirs)

option(MIDIPLAYER_GLOBAL_INSTALL "Use absolute resource path (${CMAKE_INSTALL_PREFIX}/share/midiplayer). By default, cwd-relative path is used.")

find_package(SFML 2.5.1 COMPONENTS graphics audio REQUIRED)

add_executable(midiplayer
    src/ConfigFileReader.cpp
    src/Event.cpp 
    src/FileWatcher.cpp
    src/MIDIDevice.cpp
    src/MIDIFile.cpp
    src/MIDIInput.cpp
    src/MIDIKey.cpp
    src/MIDIPlayer.cpp
    src/Resources.cpp
    src/RoundedEdgeRectangleShape.cpp
    src/Selector.cpp
    src/Track.cpp
    src/main.cpp
)
target_link_libraries(midiplayer pthread sfml-graphics sfml-audio)
target_include_directories(midiplayer PUBLIC ${CMAKE_BINARY_DIR}/src)
install(TARGETS midiplayer DESTINATION bin)
install(DIRECTORY res/ DESTINATION ${CMAKE_INSTALL_DATADIR}/midiplayer)
set(MIDIPLAYER_RESOURCE_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/midiplayer)
message(STATUS "Using resource path: '${MIDIPLAYER_RESOURCE_PATH}'")
configure_file(src/ResourceDir.h.in src/ResourceDir.h)
