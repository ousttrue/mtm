SET(TARGET_NAME vtlog)

add_executable(${TARGET_NAME}
    vtlog.cpp
)
target_link_libraries(${TARGET_NAME} PRIVATE
    sub
)
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../_externals/plog/include
)
