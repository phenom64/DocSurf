cmake_minimum_required(VERSION 2.8.12)

project(DocSurf)

set(QT_MIN_VERSION "5.4.0")
set(KF5_MIN_VERSION "5.9.0")

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Gui
    Core
    DBus
    UiTools
    Widgets
    X11Extras
    OpenGL
    Network
    Xml
)

execute_process(COMMAND kf5-config --prefix OUTPUT_VARIABLE INCLUDEPATH OUTPUT_STRIP_TRAILING_WHITESPACE)
include_directories(${INCLUDEPATH}/include)

include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5X11Extras_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS} ${Qt5Xml_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

# required frameworks by Core
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    Crash
    GlobalAccel
    I18n
    KIO
    Service
    Init
    Notifications
    Service
    WidgetsAddons
    WindowSystem
    XmlGui
    Parts
)

add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

#this is quite some magic, no need to use qtX_wrap_cpp anymore
set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#we need to first handle plugins...
add_subdirectory(src)

# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach(dir ${dirs})
#   message(STATUS "dir='${dir}'")
# endforeach()
