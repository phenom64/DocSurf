cmake_minimum_required(VERSION 2.8.12)

project(DocSurf)

include(KDEInstallDirs)

file(GLOB_RECURSE DOCSURF_SRCS *.cpp)
file(GLOB_RECURSE DOCSURF_HDRS *.h)
file(GLOB_RECURSE DOCSURF_UIS  *.ui)

qt5_wrap_ui(DOCSURF_UI_SRCS ${DOCSURF_UIS})

add_executable(DocSurf ${DOCSURF_SRCS} ${DOCSURF_HDRS} ${DOCSURF_UI_SRCS})

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
# KXMLGUI expects ui.rc in a subdirectory named after the component
# e.g. share/kxmlgui5/DocSurf/DocSurfui.rc
install(FILES DocSurfui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/DocSurf)
