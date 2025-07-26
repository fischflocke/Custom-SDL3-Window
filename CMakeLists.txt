cmake_minimum_required(VERSION 3.16)

project(Demo-Window LANGUAGES C)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(SDL3_image REQUIRED CONFIG REQUIRED COMPONENTS SDL3_image-shared)
find_package(SDL3_ttf REQUIRED CONFIG REQUIRED COMPONENTS SDL3_ttf-shared)

add_executable(Demo-Window main.c shadow.h)

target_link_libraries(Demo-Window PRIVATE m)
target_link_libraries(Demo-Window PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf)
