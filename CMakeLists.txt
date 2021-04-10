project(mtm)
cmake_minimum_required(VERSION 3.10)

add_executable(${PROJECT_NAME}
    mtm.cpp
    pair.c
    vt/vtparser.c
    main.cpp
    selector.cpp
    node.cpp
    scrn.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _POSIX_C_SOURCE=200809L
    _XOPEN_SOURCE=600
    _XOPEN_SOURCE_EXTENDED
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ncursesw
    util
)
