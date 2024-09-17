cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TP1)

set( PROJECT_IMPL  TriangularMesh.cpp TP1.cpp)

add_executable(TP1 ${PROJECT_IMPL})
target_include_directories(TP1 PUBLIC .)