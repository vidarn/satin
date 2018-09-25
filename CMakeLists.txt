cmake_minimum_required(VERSION 2.6)

set(SATIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/engine.c)
add_library(satin ${SATIN_SRC})
