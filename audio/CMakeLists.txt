cmake_minimum_required(VERSION 3.28)
project(voe_audio C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(audio DEPENDS scene ecs math platform assets base)
