cmake_minimum_required(VERSION 3.19)
project(text_search LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(text_search
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    progressstatus.h progressstatus.cpp progressstatus.ui
    foundfilesmodel.h foundfilesmodel.cpp
    fileinfo.h
    filesview.h filesview.cpp
    minizip_wrapper.h minizip_wrapper.cpp
    progresswithcancel.h progresswithcancel.cpp progressbar.ui
)

target_link_libraries(text_search
    PRIVATE
        Qt::Core
        Qt::Widgets
)

include(GNUInstallDirs)

add_subdirectory(zlib-develop)
add_subdirectory(zlib-develop/contrib/minizip)

target_include_directories(text_search PRIVATE zlib-develop/contrib .)
target_link_libraries(text_search PRIVATE libminizipstatic)

install(TARGETS text_search
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET text_search
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
