
cmake_minimum_required(VERSION 3.19)

project(StockServer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

qt_add_executable(StockServer
    main.cpp
)

target_link_libraries(StockServer
    PRIVATE
        Qt6::Core
        Qt6::Network
)

install(TARGETS StockServer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
