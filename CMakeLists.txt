cmake_minimum_required(VERSION 3.20)

project(idu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(idu
    src/main.cpp
)

target_include_directories(idu PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(idu SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_options(idu PRIVATE
    $<$<CONFIG:Debug>:
        -O2 
        -g3 -ggdb    
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        -Wformat=2 
        -Wnull-dereference  
        -fno-omit-frame-pointer 
        -fsanitize=address,undefined,leak 
        -D_GLIBCXX_DEBUG
    >

    $<$<CONFIG:Release>:
        -O3 -DNDEBUG
        -march=native -mtune=native 
        -flto=auto 
        -fipa-pta 
        -funroll-loops
        -fno-semantic-interposition 
        -ftracer 
        -fno-exceptions
        -fno-rtti
        -ffast-math
        -fmerge-all-constants
        -ffunction-sections -fdata-sections
        -fvisibility=hidden -fvisibility-inlines-hidden 
        -fno-plt 
        -pipe  
    >
    -Wno-interference-size
)

target_link_options(idu PRIVATE
    $<$<CONFIG:Debug>:
        -fsanitize=address,undefined,leak
    >
    $<$<CONFIG:Release>:
       -flto=auto 
            -Wl,--gc-sections 
            -Wl,--as-needed 
            -Wl,-O1
            -Wl,-s             
            -fuse-ld=mold 
    >
)
