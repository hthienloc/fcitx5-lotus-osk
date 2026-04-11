cmake_minimum_required(VERSION 3.16)

project(fcitx5-lotus-osk VERSION 0.1.0)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets DBus REQUIRED)
find_package(LayerShellQt)

add_executable(fcitx5-lotus-osk
    src/main.cpp
    src/lotus-key-command.h
    src/osk_theme.h
    src/osk_window.cpp
    src/osk_window.h
    src/osk_controller.cpp
    src/osk_controller.h
)

target_link_libraries(fcitx5-lotus-osk
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
)

if (LayerShellQt_FOUND)
    target_compile_definitions(fcitx5-lotus-osk PRIVATE HAVE_LAYER_SHELL)
    target_link_libraries(fcitx5-lotus-osk PRIVATE LayerShellQt::Interface)
endif()

target_include_directories(fcitx5-lotus-osk PRIVATE src)

install(TARGETS fcitx5-lotus-osk DESTINATION ${CMAKE_INSTALL_BINDIR})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/app.lotus.Osk.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/app.lotus.Osk.service
    @ONLY
)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/app.lotus.Osk.service
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)
