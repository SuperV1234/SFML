add_subdirectory(install)
set_target_warnings(test-sfml-install)

set(DOCTEST_NO_INSTALL ON CACHE BOOL "")

CPMAddPackage(
    NAME doctest
    GITHUB_REPOSITORY doctest/doctest
    GIT_TAG v2.4.11
    SYSTEM TRUE
)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

# include_directories(${PROJECT_SOURCE_DIR}/tracy/public/tracy/)
# add_definitions(-include Tracy.hpp)
# add_definitions(-DTRACY_ENABLE)
#
# add_library(tracyclient SHARED ${PROJECT_SOURCE_DIR}/tracy/public/TracyClient.cpp)
# target_link_libraries(tracyclient PRIVATE ws2_32 winmm gdi32 dbghelp)



# Build doctest in C++20 mode to enable C++20 features
target_compile_features(doctest INTERFACE cxx_std_20)

# Ensure that doctest sources and headers are not analyzed by any tools
set_target_properties(doctest PROPERTIES COMPILE_OPTIONS "" EXPORT_COMPILE_COMMANDS OFF)
set_target_properties(doctest_with_main PROPERTIES EXPORT_COMPILE_COMMANDS OFF)
set_target_properties(doctest doctest_with_main PROPERTIES FOLDER "Dependencies")
get_target_property(DOCTEST_INCLUDE_DIRS doctest INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(doctest SYSTEM INTERFACE ${DOCTEST_INCLUDE_DIRS})
target_compile_options(doctest_with_main PRIVATE "-w")

# add flags required for emscripten support (e.g. pthread)
if(SFML_OS_EMSCRIPTEN)
    target_compile_definitions(doctest INTERFACE DOCTEST_CONFIG_COLORS_NONE)
    target_compile_options(doctest INTERFACE ${SFML_EMSCRIPTEN_TARGET_COMPILE_OPTIONS})
    target_link_options(doctest INTERFACE ${SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS})
endif()

set(TEST_SRCROOT ${PROJECT_SOURCE_DIR}/test)
file(GLOB TEST_UTILITIES_SRC CONFIGURE_DEPENDS "${TEST_SRCROOT}/TestUtilities/*.cpp" "${TEST_SRCROOT}/TestUtilities/*.hpp" "${TEST_SRCROOT}/TestUtilities/*.inl")

add_library(sfml-test-main STATIC ${TEST_UTILITIES_SRC})
target_include_directories(sfml-test-main PUBLIC TestUtilities)
target_compile_definitions(sfml-test-main PUBLIC DOCTEST_CONFIG_REQUIRE_STRINGIFICATION_FOR_ALL_USED_TYPES)

set_target_warnings(sfml-test-main)

# enable precompiled headers
if (SFML_ENABLE_PCH)
    message(VERBOSE "enabling PCH for doctest")

    target_precompile_headers(doctest_with_main REUSE_FROM sfml-system)
    target_precompile_headers(sfml-test-main REUSE_FROM sfml-system)

    target_include_directories(sfml-system PUBLIC ${DOCTEST_INCLUDE_DIRS})
    target_link_libraries(sfml-system PUBLIC $<BUILD_INTERFACE:doctest::doctest>)
    target_link_libraries(sfml-test-main PUBLIC SFML::System doctest::doctest)
else()
    target_link_libraries(sfml-test-main PUBLIC SFML::System doctest::doctest_with_main)
endif()

# set the target flags to use the appropriate C++ standard library
sfml_set_stdlib(doctest)
sfml_set_stdlib(doctest_with_main)
sfml_set_stdlib(sfml-test-main)

sfml_set_option(SFML_RUN_DISPLAY_TESTS ON BOOL "ON to run tests that require a display, OFF to ignore it")
if(SFML_RUN_DISPLAY_TESTS)
    add_definitions(-DSFML_RUN_DISPLAY_TESTS)
    target_compile_definitions(sfml-test-main PRIVATE SFML_RUN_DISPLAY_TESTS)
endif()

sfml_set_option(SFML_RUN_AUDIO_DEVICE_TESTS ON BOOL "ON to run tests that require an audio device, OFF to ignore it")
if(SFML_RUN_AUDIO_DEVICE_TESTS)
    add_definitions(-DSFML_RUN_AUDIO_DEVICE_TESTS)
    target_compile_definitions(sfml-test-main PRIVATE SFML_RUN_AUDIO_DEVICE_TESTS)
endif()

file(GLOB BASE_SRC CONFIGURE_DEPENDS "${TEST_SRCROOT}/Base/*.cpp" "${TEST_SRCROOT}/Base/*.hpp" "${TEST_SRCROOT}/Base/*.inl")
sfml_add_test(test-sfml-base "${BASE_SRC}" "")

file(GLOB SYSTEM_SRC CONFIGURE_DEPENDS "${TEST_SRCROOT}/System/*.cpp" "${TEST_SRCROOT}/System/*.hpp" "${TEST_SRCROOT}/System/*.inl")
sfml_add_test(test-sfml-system "${SYSTEM_SRC}" "")
target_compile_definitions(test-sfml-system PRIVATE
    EXPECTED_SFML_VERSION_MAJOR=${SFML_VERSION_MAJOR}
    EXPECTED_SFML_VERSION_MINOR=${SFML_VERSION_MINOR}
    EXPECTED_SFML_VERSION_PATCH=${SFML_VERSION_PATCH}
    EXPECTED_SFML_VERSION_IS_RELEASE=$<IF:$<BOOL:${VERSION_IS_RELEASE}>,true,false>
)

file(GLOB WINDOW_SRC CONFIGURE_DEPENDS "${TEST_SRCROOT}/Window/*.cpp" "${TEST_SRCROOT}/Window/*.hpp" "${TEST_SRCROOT}/Window/*.inl")
sfml_add_test(test-sfml-window "${WINDOW_SRC}" SFML::Window)

file(GLOB GRAPHICS_SRC CONFIGURE_DEPENDS "${TEST_SRCROOT}/Graphics/*.cpp" "${TEST_SRCROOT}/Graphics/*.hpp" "${TEST_SRCROOT}/Graphics/*.inl")
sfml_add_test(test-sfml-graphics "${GRAPHICS_SRC}" SFML::Graphics)

if(SFML_RUN_DISPLAY_TESTS)
    target_compile_definitions(test-sfml-graphics PRIVATE SFML_RUN_DISPLAY_TESTS)
endif()
if(SFML_OS_EMSCRIPTEN)
    set(GRAPHICS_TEST_RESOURCES
        Graphics/invalid_shader.vert
        Graphics/sfml-logo-big.bmp
        Graphics/sfml-logo-big.gif
        Graphics/sfml-logo-big.jpg
        Graphics/sfml-logo-big.png
        Graphics/sfml-logo-big.psd
        Graphics/shader.frag
        Graphics/shader.geom
        Graphics/shader.vert
        Graphics/tuffy.ttf)

    foreach(RESOURCE ${GRAPHICS_TEST_RESOURCES})
        target_link_options(test-sfml-graphics PRIVATE "SHELL:--embed-file ${RESOURCE}")
    endforeach()
endif()

file(GLOB NETWORK_SRC CONFIGURE_DEPENDS "${TEST_SRCROOT}/Network/*.cpp" "${TEST_SRCROOT}/Network/*.hpp" "${TEST_SRCROOT}/Network/*.inl")
sfml_add_test(test-sfml-network "${NETWORK_SRC}" SFML::Network)

file(GLOB AUDIO_SRC CONFIGURE_DEPENDS "${TEST_SRCROOT}/Audio/*.cpp" "${TEST_SRCROOT}/Audio/*.hpp" "${TEST_SRCROOT}/Audio/*.inl")
sfml_add_test(test-sfml-audio "${AUDIO_SRC}" SFML::Audio)

if(SFML_OS_ANDROID AND DEFINED ENV{LIBCXX_SHARED_SO})
    # Because we can only write to the tmp directory on the Android virtual device we will need to build our directory tree under it
    set(TARGET_DIR "/data/local/tmp/$<TARGET_FILE_DIR:test-sfml-system>")

    # Generate script that copies necessary files over to the Android virtual device
    file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/prepare-android-files.sh" CONTENT
        "#!/bin/bash\n\
        adb shell \"mkdir -p ${TARGET_DIR}\"\n\
        adb push $<TARGET_FILE:test-sfml-audio>    ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:test-sfml-base>     ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:test-sfml-graphics> ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:test-sfml-network>  ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:test-sfml-system>   ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:test-sfml-window>   ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:SFML::Audio>        ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:SFML::Graphics>     ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:SFML::ImGui>        ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:SFML::Network>      ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:SFML::System>       ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:SFML::Window>       ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:doctest>            ${TARGET_DIR}\n\
        adb push $<TARGET_FILE:doctest_with_main>  ${TARGET_DIR}\n\
        adb push $ENV{LIBCXX_SHARED_SO}            ${TARGET_DIR}\n\
        adb push ${CMAKE_CURRENT_LIST_DIR}         ${TARGET_DIR}\n\
        adb shell \"chmod -R 775 ${TARGET_DIR} && ls -la ${TARGET_DIR}\"\n"
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

    # Add the target to invoke the file copy script
    add_custom_target(prepare-android-files COMMAND "${PROJECT_BINARY_DIR}/prepare-android-files.sh")

    # Generate proxy script that translates CTest commands into adb shell commands
    file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/run-in-adb-shell.sh" CONTENT
        "#!/bin/bash\n\
        adb shell \"cd ${TARGET_DIR}/test; LD_LIBRARY_PATH=${TARGET_DIR} /data/local/tmp/$1 \\\"$2\\\" \\\"$3\\\" \\\"$4\\\" \\\"$5\\\" \\\"$6\\\" \\\"$7\\\" \\\"$8\\\" \\\"$9\\\"\"\n"
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
endif()

if(SFML_ENABLE_COVERAGE AND SFML_OS_WINDOWS AND NOT SFML_COMPILER_GCC)
    # Try to find and use OpenCppCoverage for coverage reporting when building with MSVC
    find_program(OpenCppCoverage_BINARY "OpenCppCoverage.exe")

    if(OpenCppCoverage_BINARY)
        execute_process(COMMAND "${OpenCppCoverage_BINARY}" --help ERROR_VARIABLE OpenCppCoverage_HELP_OUTPUT OUTPUT_QUIET)

        if(OpenCppCoverage_HELP_OUTPUT MATCHES "OpenCppCoverage Version: ([.0-9]+)")
            set(OpenCppCoverage_VERSION "${CMAKE_MATCH_1}")
        endif()
    endif()

    include(FindPackageHandleStandardArgs)

    find_package_handle_standard_args(OpenCppCoverage
        REQUIRED_VARS OpenCppCoverage_BINARY
        VERSION_VAR OpenCppCoverage_VERSION
    )
endif()

if(SFML_ENABLE_COVERAGE AND OpenCppCoverage_FOUND)
    # Use OpenCppCoverage
    message(STATUS "Using OpenCppCoverage to generate coverage report")

    string(REPLACE "/" "\\" COVERAGE_EXCLUDE "${CMAKE_CTEST_COMMAND}")
    string(REPLACE "/" "\\" COVERAGE_SRC "${PROJECT_SOURCE_DIR}/src")
    string(REPLACE "/" "\\" COVERAGE_INCLUDE "${PROJECT_SOURCE_DIR}/include")

    # We need to patch the OpenCppCoverage output to remove path prefixes so Coveralls doesn't get confused
    cmake_path(GET PROJECT_SOURCE_DIR ROOT_NAME COVERAGE_ROOT_NAME)
    string(REPLACE "/" "\\\\" COVERAGE_PATH_PREFIX "${PROJECT_SOURCE_DIR}/")
    string(REPLACE "${COVERAGE_ROOT_NAME}\\\\" "" COVERAGE_PATH_PREFIX "${COVERAGE_PATH_PREFIX}")

    file(WRITE "${PROJECT_BINARY_DIR}/patch_coverage.cmake"
         "file(READ \"${PROJECT_BINARY_DIR}/coverage.out\" COVERAGE_OUT)\n\
         string(REPLACE \"${COVERAGE_PATH_PREFIX}\" \"\" COVERAGE_OUT \"\${COVERAGE_OUT}\")\n\
         string(REPLACE \"${COVERAGE_ROOT_NAME}\" \".\" COVERAGE_OUT \"\${COVERAGE_OUT}\")\n\
         file(WRITE \"${PROJECT_BINARY_DIR}/coverage.out\" \"\${COVERAGE_OUT}\")\n")

    set(COVERAGE_PREFIX ${OpenCppCoverage_BINARY} --quiet --export_type cobertura:${PROJECT_BINARY_DIR}/coverage.out --cover_children --excluded_modules "${COVERAGE_EXCLUDE}" --sources "${COVERAGE_SRC}" --sources "${COVERAGE_INCLUDE}" --)
else()
    # On all other systems, we just run an empty script
    file(WRITE "${PROJECT_BINARY_DIR}/patch_coverage.cmake" "")
endif()

# Convenience for building and running tests in a single command
add_custom_target(runtests DEPENDS test-sfml-base test-sfml-system test-sfml-window test-sfml-graphics test-sfml-network test-sfml-audio)
add_custom_command(TARGET runtests
                   COMMENT "Run tests"
                   POST_BUILD
                   COMMAND ${COVERAGE_PREFIX} ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
                   COMMAND ${CMAKE_COMMAND} -P "${PROJECT_BINARY_DIR}/patch_coverage.cmake"
                   VERBATIM)
