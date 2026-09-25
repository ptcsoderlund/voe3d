cmake_minimum_required(VERSION 3.28)
project(voe_editor C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_executable(editor DEPENDS game app theme ui text 3d physics scene ecs render platform authoring math base)
