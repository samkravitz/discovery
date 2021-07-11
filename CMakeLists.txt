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

# sdl 
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# fmt
find_package(fmt)

# imgui
find_package(imgui)

set(IMGUI_SOURCES
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
)

set(IMGUI_HEADERS
    third_party/imgui/imconfig.h
    third_party/imgui/imgui.h
    third_party/imgui/imgui_internal.h
    third_party/imgui/imstb_rectpack.h
    third_party/imgui/imstb_textedit.h
    third_party/imgui/imstb_truetype.h
)

include_directories(include)
include_directories(include/backup)
include_directories(third_party/imgui)

add_subdirectory(src)
add_subdirectory(include)

add_library("imgui" STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})

add_executable(discovery ${SOURCES} ${HEADERS})
target_link_libraries(discovery ${SDL2_LIBRARIES} stdc++fs fmt::fmt imgui)
