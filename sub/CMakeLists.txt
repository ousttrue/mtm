SET(TARGET_NAME sub)

add_library(${TARGET_NAME}
    pair.cpp
    vtparser.cpp
    vthandler.cpp
    curses_term.cpp
    selector.cpp
    node.cpp
    curses_window.cpp
    app.cpp
    vtcontext.cpp
)
target_compile_definitions(${TARGET_NAME} PUBLIC
    _POSIX_C_SOURCE=200809L
    _XOPEN_SOURCE=600
    _XOPEN_SOURCE_EXTENDED
)
target_link_libraries(${TARGET_NAME} PUBLIC
    ncursesw
    util
)
target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../_externals/plog/include
)
