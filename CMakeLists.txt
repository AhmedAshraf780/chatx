cmake_minimum_required(VERSION 3.14)
project(ChatApplication)

set(CMAKE_CXX_STANDARD 17)

# Find Qt
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# UI files
set(UI_FILES
    UI/Chat__Application.ui
    UI/login.ui
    UI/signup.ui
    UI/forgotpass.ui
)

# Header files
set(HEADER_FILES
    headers/Chat__Application.h
    headers/chatPage.h
    headers/login.h
    headers/forgotpass.h
    headers/signup.h
    server/server.h
    client/client.h
    client/message.h
    client/room.h

)

# Source files
set(SOURCE_FILES
    main.cpp
    sourceFiles/Chat__Application.cpp
    sourceFiles/chatPage.cpp
    sourceFiles/login.cpp
    sourceFiles/forgotpass.cpp
    sourceFiles/signup.cpp
    server/server.cpp
    client/client.cpp
    client/message.cpp
    client/room.cpp
)

# Automatically generate MOC and UI files
qt5_wrap_ui(UIS ${UI_FILES})
qt5_wrap_cpp(MOCS ${HEADER_FILES})

add_executable(ChatApp
    ${SOURCE_FILES}
    ${MOCS}
    ${UIS}
)

target_link_libraries(ChatApp Qt5::Widgets)

# Include directories
target_include_directories(ChatApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/UI
)
