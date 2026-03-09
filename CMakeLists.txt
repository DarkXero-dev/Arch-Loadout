cmake_minimum_required(VERSION 3.16)
project(arch-system-loadout VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)

qt_add_executable(arch-system-loadout
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/pagehelpers.h
    ${PROJECT_SOURCE_DIR}/src/mainwizard.cpp            ${PROJECT_SOURCE_DIR}/src/mainwizard.h
    ${PROJECT_SOURCE_DIR}/src/installworker.cpp         ${PROJECT_SOURCE_DIR}/src/installworker.h
    ${PROJECT_SOURCE_DIR}/src/pages/welcomepage.cpp     ${PROJECT_SOURCE_DIR}/src/pages/welcomepage.h
    ${PROJECT_SOURCE_DIR}/src/pages/chaoticaurpage.cpp  ${PROJECT_SOURCE_DIR}/src/pages/chaoticaurpage.h
    ${PROJECT_SOURCE_DIR}/src/pages/installpage.cpp    ${PROJECT_SOURCE_DIR}/src/pages/installpage.h
)

target_link_libraries(arch-system-loadout PRIVATE Qt6::Core Qt6::Widgets Qt6::Concurrent)

install(TARGETS arch-system-loadout RUNTIME DESTINATION /usr/local/bin)
