cmake_minimum_required(VERSION 3.28)
project(voe_scene C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(scene DEPENDS ecs math base)
