# 指定 Cmake 的最低版本，确保兼容性
cmake_minimum_required(VERSION 3.16)

# 定义项目名称和使用的语言
project(httpserver LANGUAGES CXX)

# 手动启动 Qt 的元对象系统（MOC/UIC/RCC)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt 中相关的模块，并兼容 Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Core
    HttpServer
)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    HttpServer
)

# 添加可执行文件
add_executable(httpserver
  main.cpp
  SimpleHttpServer.qrc
  simplehttpserver.h simplehttpserver.cpp
)

# 连接相关的模块
target_link_libraries(httpserver
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::HttpServer
)

include(GNUInstallDirs)
install(TARGETS httpserver
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
