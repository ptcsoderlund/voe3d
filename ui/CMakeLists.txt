cmake_minimum_required(VERSION 3.28)
project(voe_ui C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(ui DEPENDS math base)
