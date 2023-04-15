cmake_minimum_required(VERSION 3.25)
project(EE450 C)

set(CMAKE_C_STANDARD 11)

add_executable(EE450 serverM.c serverA.c serverB.c client.c)
