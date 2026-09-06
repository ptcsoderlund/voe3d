cmake_minimum_required(VERSION 3.28)
project(voe_sprite C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(sprite DEPENDS 3d render math base)
