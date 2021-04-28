SET(TARGET_NAME vtlog)

add_executable(${TARGET_NAME}
    vtlog.cpp
)
target_link_libraries(${TARGET_NAME} PRIVATE
    sub
)
