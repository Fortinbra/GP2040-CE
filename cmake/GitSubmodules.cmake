# GitSubmodules.cmake
# Module for handling Git submodule initialization

function(setup_git_submodules)
    # Check if we should skip submodule update
    if(DEFINED ENV{SKIP_SUBMODULES})
        set(SKIP_SUBMODULES $ENV{SKIP_SUBMODULES})
    elseif(NOT DEFINED SKIP_SUBMODULES)
        set(SKIP_SUBMODULES FALSE)
    endif()

    if(SKIP_SUBMODULES)
        cmake_print_variables(SKIP_SUBMODULES)
        message(STATUS "Skipping submodule update")
        return()
    endif()

    # Find Git and update submodules
    find_package(Git)
    if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
        message(STATUS "Updating Git submodules...")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT
        )
        
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        else()
            message(STATUS "Submodules updated successfully")
        endif()
    else()
        if(NOT GIT_FOUND)
            message(WARNING "Git not found, cannot update submodules")
        else()
            message(WARNING "Not in a Git repository, cannot update submodules")
        endif()
    endif()
endfunction()
