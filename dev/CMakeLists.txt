cmake_minimum_required(VERSION 3.28)
project(voe_dev C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_executable(dev DEPENDS platform base)
