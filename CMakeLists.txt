cmake_minimum_required(VERSION 3.9)
project(gameoflife)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set(SOURCE_FILES board.h board.c gameoflife.c vtkwrite.c vtkwrite.h)
add_executable(gameoflife ${SOURCE_FILES} board.h board.c gameoflife.c)
target_link_libraries(gameoflife m)