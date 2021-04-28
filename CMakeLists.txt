project(mtm)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

#
# lib
#
add_library(sub
    pair.cpp
    vtparser.cpp
    vthandler.cpp
    curses_term.cpp
    selector.cpp
    node.cpp
    scrn.cpp
    app.cpp
    vtcontext.cpp
)
target_compile_definitions(sub PUBLIC
    _POSIX_C_SOURCE=200809L
    _XOPEN_SOURCE=600
    _XOPEN_SOURCE_EXTENDED
)
target_link_libraries(sub PUBLIC
    ncursesw
    util
)

#
# mtm
#
add_executable(mtm
    main.cpp
)
target_link_libraries(mtm PRIVATE
    sub
)

#
# vtlog
#
add_executable(vtlog
    vtlog.cpp
)
target_link_libraries(vtlog PRIVATE
    sub
)
