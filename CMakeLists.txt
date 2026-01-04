cmake_minimum_required(VERSION 3.12)
project(Siv3DProject CXX)

# ==============================================================================
# Git Submodule Setup
# ==============================================================================
option(SCRAPRECOVER_AUTO_INIT_SUBMODULES "Auto-init git submodules when missing" ON)

if (SCRAPRECOVER_AUTO_INIT_SUBMODULES)
    find_package(Git QUIET)
    if (GIT_FOUND)
        set(SIV3D_SUBMODULE_DIR "${CMAKE_SOURCE_DIR}/external/Siv3D")
        set(JSON_SUBMODULE_DIR "${CMAKE_SOURCE_DIR}/external/json")

        if (NOT EXISTS "${SIV3D_SUBMODULE_DIR}/Linux/CMakeLists.txt" OR 
            NOT EXISTS "${JSON_SUBMODULE_DIR}/single_include/nlohmann/json.hpp")
            
            message(STATUS "Initializing git submodules...")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT
            )
            
            if (NOT GIT_SUBMOD_RESULT EQUAL 0)
                message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}")
            endif()
        endif()
    else()
        message(WARNING "Git not found; run: git submodule update --init --recursive")
    endif()
endif()


# ==============================================================================
# Compiler Settings
# ==============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_CONFIGURATION_TYPES AND 
    NOT CMAKE_NO_BUILD_TYPE AND
    NOT CMAKE_BUILD_TYPE AND
    CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "[!] Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release)
endif()


# ==============================================================================
# Output Directory
# ==============================================================================
# Set the output directory to Siv3D/App to enable asset access.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/external/Siv3D/Linux/App")


# ==============================================================================
# Target & Sources
# ==============================================================================
add_executable(Siv3DApp
    src/main.cpp
    src/utils/config.cpp
    src/utils/layout_utils.cpp
    src/utils/pattern_mask.cpp
    src/utils/geometry_utils.cpp
    src/utils/polygon_generator.cpp
    src/ui/ui_manager.cpp
    src/ui/ui_buttons.cpp
    src/ui/ui_unplaced.cpp
    src/ui/ui_canvas.cpp
    src/ui/ui_draw.cpp
    src/ui/ui_events.cpp
    src/ui/ui_io.cpp
    src/ui/ui_placed_list.cpp
    src/ui/ui_pattern.cpp
    src/core/polygon.cpp
    src/core/canvas_model.cpp
    src/core/canvas_state.cpp
    src/algorithm/initializer.cpp
    src/algorithm/optimizer.cpp
    src/algorithm/optimizer_operations.cpp
    src/algorithm/optimizer_transition.cpp
    src/algorithm/hyperparameter.cpp
    src/algorithm/eval_functions.cpp
)


# ==============================================================================
# Include Directories
# ==============================================================================
target_include_directories(Siv3DApp PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/external/json/single_include"
)


# ==============================================================================
# Dependencies (Siv3D)
# ==============================================================================
find_package(Siv3D REQUIRED)
target_link_libraries(Siv3DApp PRIVATE Siv3D::Siv3D)