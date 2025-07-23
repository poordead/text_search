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

    foundfilesmodel.h foundfilesmodel.cpp
    fileinfo.h
    filesview.h filesview.cpp
    minizip_wrapper.h minizip_wrapper.cpp
    progresswithcancel.h progresswithcancel.cpp progressbar.ui
    conanfile.txt
)

target_link_libraries(text_search
    PRIVATE
        Qt::Core
        Qt::Widgets
)

include(GNUInstallDirs)

find_package(minizip REQUIRED)

target_link_libraries(text_search PRIVATE minizip::minizip)

target_include_directories(text_search PRIVATE .)

install(TARGETS text_search
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(NOT LINUX)
	qt_generate_deploy_app_script(
		TARGET text_search
		OUTPUT_SCRIPT deploy_script
		NO_UNSUPPORTED_PLATFORM_ERROR
	)
	install(SCRIPT ${deploy_script})
endif()
