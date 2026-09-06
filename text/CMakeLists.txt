cmake_minimum_required(VERSION 3.28)
project(voe_text C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(text DEPENDS render math base)
