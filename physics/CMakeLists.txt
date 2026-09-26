cmake_minimum_required(VERSION 3.28)
project(voe_physics C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(physics DEPENDS scene ecs math base)
