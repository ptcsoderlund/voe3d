cmake_minimum_required(VERSION 3.28)
project(voe_3d C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(3d DEPENDS render scene ecs assets math base)
