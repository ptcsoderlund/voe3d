cmake_minimum_required(VERSION 3.28)
project(voe C)
include(cmake/voe.cmake)
add_subdirectory(base)
add_subdirectory(math)
add_subdirectory(ecs)
add_subdirectory(platform)
add_subdirectory(scene)
add_subdirectory(assets)
add_subdirectory(render)
add_subdirectory(3d)
add_subdirectory(text)
add_subdirectory(dev)

# The root is a project with a CMakeLists.txt too, so it gets its database beside
# it on the same terms every folder does. This is the only call that is not made
# from inside voe_module()/voe_executable(), because the root builds no target of
# its own for one to hang off.
voe_export_compile_commands(voe ${CMAKE_CURRENT_LIST_DIR})
