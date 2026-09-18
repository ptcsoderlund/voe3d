cmake_minimum_required(VERSION 3.28)
project(voe_theme C)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)
voe_module(theme DEPENDS ui text render assets math base)
