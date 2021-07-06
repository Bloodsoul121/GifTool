cmake_minimum_required(VERSION 3.10.2)

project("giftool")

include_directories("/gif")

add_library(
        native-lib
        SHARED
        native-lib.cpp
        gif/dgif_lib.c
        gif/gifalloc.c
        gif/openbsd-reallocarray.c
)

find_library(
        log-lib
        log
)

find_library(
        jnigraphics-lib
        jnigraphics)


target_link_libraries(
        native-lib
        ${log-lib}
        ${jnigraphics-lib}
)