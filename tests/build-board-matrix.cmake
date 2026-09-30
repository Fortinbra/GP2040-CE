cmake_minimum_required(VERSION 3.24)
get_filename_component(source_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(build_root "${source_root}/build")
set(log_root "${build_root}/board-validation")
file(MAKE_DIRECTORY "${log_root}" "${build_root}/external packages" "${build_root}/external working directory")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/fixtures/boards/CustomPico" DESTINATION "${build_root}/external packages")
include("${CMAKE_CURRENT_LIST_DIR}/board-expectations.cmake")
if(NOT DEFINED GP2040_BUILD_TARGETS)
    set(GP2040_BUILD_TARGETS "${reference_targets}")
endif()
if(NOT DEFINED GP2040_BUILD_WITH_WEB)
    set(GP2040_BUILD_WITH_WEB TRUE)
endif()
find_program(NINJA_EXECUTABLE ninja REQUIRED)
set(clean_environment "")
foreach(variable GP2040_BOARD GP2040_BOARDCONFIG GP2040_BOARD_DIRS PICO_BOARD PICO_PLATFORM)
    list(APPEND clean_environment "--unset=${variable}")
endforeach()

foreach(board IN LISTS GP2040_BUILD_TARGETS)
    if(NOT board IN_LIST reference_targets)
        message(FATAL_ERROR "No reference expectations for '${board}'.")
    endif()
    set(skip_web TRUE)
    if(board STREQUAL "Pico" AND GP2040_BUILD_WITH_WEB)
        set(skip_web FALSE)
    endif()
    set(board_dirs "")
    if(board STREQUAL "CustomPico")
        set(board_dirs "${build_root}/external packages")
    endif()
    message(STATUS "Fresh configure: ${board}; SKIP_WEBBUILD=${skip_web}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env ${clean_environment} "SKIP_WEBBUILD=${skip_web}"
            "${CMAKE_COMMAND}" -G Ninja -S "${source_root}" -B "${build_root}" --fresh
            "-DGP2040_BOARD=${board}" "-DGP2040_BOARD_DIRS=${board_dirs}"
            -DGP2040_BOARD_TEST_EXPECTATIONS=ON -DCMAKE_BUILD_TYPE=Release
        WORKING_DIRECTORY "${build_root}/external working directory"
        RESULT_VARIABLE result
        OUTPUT_FILE "${log_root}/${board}-configure.log" ERROR_FILE "${log_root}/${board}-configure.log"
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${board} configure failed (${result}); see ${log_root}/${board}-configure.log")
    endif()
    execute_process(COMMAND "${NINJA_EXECUTABLE}" -C "${build_root}" -t clean RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${board} clean failed (${result}).")
    endif()
    execute_process(
        COMMAND "${NINJA_EXECUTABLE}" -C "${build_root}"
        RESULT_VARIABLE result
        OUTPUT_FILE "${log_root}/${board}-build.log" ERROR_FILE "${log_root}/${board}-build.log"
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${board} build failed (${result}); see ${log_root}/${board}-build.log")
    endif()
    file(GLOB artifacts "${build_root}/GP2040-CE_*_${board}.uf2")
    if(NOT artifacts)
        message(FATAL_ERROR "${board} build produced no target-named UF2.")
    endif()
    message(STATUS "PASS: ${board} clean full build, hardware/storage assertions, and target-named UF2")
endforeach()