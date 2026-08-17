cmake_minimum_required(VERSION 3.16)
project(msconfig-linux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# 优先 Qt6，回退 Qt5（老发行版）
find_package(Qt6 6.2 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(MSC_QT_TARGETS Qt6::Widgets)
    message(STATUS "msconfig-linux: using Qt6")
else()
    find_package(Qt5 5.12 COMPONENTS Widgets REQUIRED)
    set(MSC_QT_TARGETS Qt5::Widgets)
    message(STATUS "msconfig-linux: using Qt5")
endif()

add_executable(msconfig
    src/main.cpp
    src/lang.cpp
    src/util.cpp
    src/grub.cpp
    src/services.cpp
    src/autostart.cpp
    src/mainwindow.cpp
    src/generaltab.cpp
    src/boottab.cpp
    src/servicestab.cpp
    src/startuptab.cpp
    src/toolstab.cpp
    src/lang.h
    src/util.h
    src/grub.h
    src/services.h
    src/autostart.h
    src/mainwindow.h
    src/configtab.h
    src/generaltab.h
    src/boottab.h
    src/servicestab.h
    src/startuptab.h
    src/toolstab.h
)

target_link_libraries(msconfig PRIVATE ${MSC_QT_TARGETS})
