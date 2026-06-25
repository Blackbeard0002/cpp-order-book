cmake_minimum_required(VERSION 3.16)
project(cpp_orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ob
    main.cpp
    order_book.cpp
    order_book_engine.cpp
)

target_include_directories(ob PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ob PRIVATE
        -O3
        -march=native
        -flto
        -Wall
        -Wextra
    )
    target_link_options(ob PRIVATE -flto)
endif()

if(MSVC)
    target_compile_options(ob PRIVATE /O2 /GL)
    target_link_options(ob PRIVATE /LTCG)
endif()
