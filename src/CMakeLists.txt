cmake_minimum_required(VERSION 2.8.12)

project(DocSurf)

file(GLOB_RECURSE KDFM_SRCS *.cpp)
file(GLOB_RECURSE KDFM_HDRS *.h)
file(GLOB_RECURSE KDFM_UIS  *.ui)

qt5_wrap_ui(KDFM_UI_SRCS ${KDFM_UIS})

add_executable(DocSurf ${KDFM_SRCS} ${KDFM_HDRS} ${KDFM_UI_SRCS})

target_link_libraries(DocSurf
    Qt5::DBus
    Qt5::X11Extras
    Qt5::Xml
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::OpenGL
    KF5::ConfigCore
    KF5::CoreAddons
    KF5::Crash
    KF5::GlobalAccel
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::I18n
    KF5::Service
    KF5::Solid
    KF5::WindowSystem
    KF5::XmlGui
    KF5::Parts
    )

install(TARGETS DocSurf DESTINATION bin)
install(FILES DocSurf.desktop DESTINATION share/applications)
install(FILES kdfmui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/DocSurf)
