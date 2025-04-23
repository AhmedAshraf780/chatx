
cmake_minimum_required(VERSION 3.5)

project(ChatxApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Qt5 components you need
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Gather all source, header, and UI files
set(SOURCES
    main.cpp
    login.cpp
    signup.cpp
    chatPage.cpp
    Chat__Application.cpp
)

set(HEADERS
    login.h
    signup.h
    chatPage.h
    Chat__Application.h
)

set(UIS
    login.ui
    signup.ui
    Chat__Application.ui
)

# Automatically generate MOC and UI files
qt5_wrap_ui(UI_HEADERS ${UIS})
qt5_wrap_cpp(MOC_SOURCES ${HEADERS})

add_executable(ChatxApp
    ${SOURCES}
    ${MOC_SOURCES}
    ${UI_HEADERS}
)

target_link_libraries(ChatxApp Qt5::Widgets)
