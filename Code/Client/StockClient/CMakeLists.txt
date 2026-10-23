cmake_minimum_required(VERSION 3.19)
project(StockClient LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

qt_add_executable(StockClient
    WIN32 MACOSX_BUNDLE
    main.cpp
    stockclient.cpp
    stockclient.h
    stockclient.ui
    mainwindow.h
    stockclient.cpp
    mainwindow.cpp
)

target_link_libraries(StockClient
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
)

install(TARGETS StockClient
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET StockClient
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
