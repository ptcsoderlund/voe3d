cmake_minimum_required(VERSION 3.28)
project(voe_app C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(app DEPENDS render platform math base)
