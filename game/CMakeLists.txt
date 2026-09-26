cmake_minimum_required(VERSION 3.28)
project(voe_game C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(game DEPENDS app 3d physics render ui text scene ecs audio platform math base)
