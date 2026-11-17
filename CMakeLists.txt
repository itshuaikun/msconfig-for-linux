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
    src/bootloader.cpp
    src/bootloader.h
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

include(GNUInstallDirs)
install(TARGETS msconfig RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES msconfig-linux.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

include(CTest)
if(BUILD_TESTING)
    add_executable(core_test tests/core_test.cpp src/bootloader.cpp src/grub.cpp src/util.cpp)
    add_executable(autostart_test tests/autostart_test.cpp src/autostart.cpp)
    get_target_property(MSC_SOURCES msconfig SOURCES)
    list(REMOVE_ITEM MSC_SOURCES src/main.cpp)
    add_executable(ui_test tests/ui_test.cpp ${MSC_SOURCES})
    foreach(test IN ITEMS core_test autostart_test ui_test)
        target_include_directories(${test} PRIVATE src)
        target_link_libraries(${test} PRIVATE ${MSC_QT_TARGETS})
        add_test(NAME ${test} COMMAND ${test})
        set_tests_properties(${test} PROPERTIES TIMEOUT 60)
    endforeach()
    set_tests_properties(ui_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
