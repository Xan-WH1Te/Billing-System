cmake_minimum_required(VERSION 3.16)
project(BillingSystem LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(billing
    src/main.c
    src/menu.c
    src/service.c
    src/card_service.c
    src/billing_service.c
    src/card_file.c
    src/billing_file.c
    src/tool.c
)

target_include_directories(billing PRIVATE include)