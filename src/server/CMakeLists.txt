﻿cmake_minimum_required(VERSION 3.2)
project(katarenga_server)

set(HEADER_FILES
    server.hpp
    server_utils.hpp
)

set(SOURCE_FILES
    server.cpp
    server_utils.cpp
    katarenga_server.cpp
)

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ZMQPP
    docopt
    katarenga_common
)
