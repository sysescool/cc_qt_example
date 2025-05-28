cmake_minimum_required(VERSION 3.16)

project(TCallbackT LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets)

add_executable(TCallbackT
  main.cpp
  TCoreSession.h TCoreSession.cpp
  Models.h
)
target_link_libraries(TCallbackT Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets)

include(GNUInstallDirs)
install(TARGETS TCallbackT
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
