cmake_minimum_required(VERSION 3.16)
project(BillingSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(billing
    src/main.cpp
    src/card_system.cpp
)

target_include_directories(billing PRIVATE include)