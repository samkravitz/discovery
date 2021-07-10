# This file has been modified from https://github.com/mmorse1217/cmake-project-template
# Adapted for discovery

cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 17)

project(discovery)

set(CMAKE_CXX_FLAGS "-lSDL2")

if (OPT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
endif()

FUNCTION(PREPEND var )
   SET(listVar "")
   FOREACH(f ${${var}})
       LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(fmt)

include_directories(include)
include_directories(include/backup)

add_subdirectory(src)
add_subdirectory(include)

add_executable(discovery ${SOURCES} ${HEADERS})
target_link_libraries(discovery ${SDL2_LIBRARIES} stdc++fs fmt::fmt)
