# NeuroDraft - Advanced Novel Writing Application
# Concept and Development: Ryon Shane Hall
# Built with Qt6 and C++20 for Linux

cmake_minimum_required(VERSION 3.16)
project(NeuroDraft VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Enable Qt6 MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories (current directory for headers)
include_directories(.)

# Source files
set(SOURCES
    main.cpp
    MainWindow.cpp
    ProjectManager.cpp
    EditorWidget.cpp
    PaneManager.cpp
    ProjectDialog.cpp
    ProjectTreeWidget.cpp
    AutoSaveManager.cpp
    DraggableTabWidget.cpp
    UpdateManager.cpp
)

# Header files
set(HEADERS
    MainWindow.h
    ProjectManager.h
    EditorWidget.h
    PaneManager.h
    ProjectDialog.h
    ProjectTreeWidget.h
    AutoSaveManager.h
    DraggableTabWidget.h
    UpdateManager.h
)

# Create executable
add_executable(NeuroDraft ${SOURCES} ${HEADERS})

# Link Qt6 libraries
target_link_libraries(NeuroDraft Qt6::Core Qt6::Widgets Qt6::Gui)

# Set output directory
set_target_properties(NeuroDraft PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)