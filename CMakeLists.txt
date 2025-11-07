cmake_minimum_required(VERSION 3.10)

project(DecodificadorPRT7)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Solo un archivo fuente
add_executable(DecodificadorPRT7 main.cpp)

message(STATUS "Proyecto configurado correctamente")