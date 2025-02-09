set(INCROOT ${PROJECT_SOURCE_DIR}/include/SFML/ImGui)
set(SRCROOT ${PROJECT_SOURCE_DIR}/src/SFML/ImGui)

file(GLOB SRC CONFIGURE_DEPENDS "${INCROOT}/*.hpp" "${INCROOT}/*.inl"  "${SRCROOT}/*.hpp" "${SRCROOT}/*.inl" "${SRCROOT}/*.cpp")

# define the sfml-imgui target
sfml_add_library(ImGui
                 SOURCES ${SRC})

# glad sources
target_include_directories(sfml-imgui SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/extlibs/headers/glad/include")

# setup dependencies
target_link_libraries(sfml-imgui PUBLIC SFML::Graphics)

if(SFML_OS_IOS)
    target_link_libraries(sfml-imgui PRIVATE "-framework OpenGLES")
elseif(SFML_OS_ANDROID OR SFML_OS_EMSCRIPTEN OR SFML_OPENGL_ES)
    find_package(EGL REQUIRED)
    find_package(GLES REQUIRED)
    target_link_libraries(sfml-imgui PRIVATE EGL::EGL GLES::GLES)
else()
    find_package(OpenGL REQUIRED COMPONENTS OpenGL)
    target_link_libraries(sfml-imgui PRIVATE OpenGL::GL)
endif()


if(SFML_USE_SYSTEM_DEPS)
    find_package(ImGui 1.91.0 REQUIRED)
else()
    # use an immediately invoked function to scope option variables we have to set
    function(sfml_add_imgui_dependencies)

        # remember whether we are building SFML as a shared library
        if(BUILD_SHARED_LIBS)
            set(SFML_BUILD_SHARED_LIBS ON)
        endif()

        set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
        set(BUILD_SHARED_LIBS OFF)

        CPMAddPackage(
            NAME imgui
            GITHUB_REPOSITORY ocornut/imgui
            GIT_TAG v1.91.8
            SYSTEM TRUE
        )

        # imgui has no CMake support, so we create our own target

        file(GLOB imgui_sources "${imgui_SOURCE_DIR}/*.cpp")
        file(GLOB imgui_sources_cpp "${imgui_SOURCE_DIR}/misc/cpp/*.cpp")

        set(IMGUI_FOUND TRUE)
        set(IMGUI_INCLUDE_DIRS ${imgui_SOURCE_DIR})
        set(IMGUI_SOURCES ${imgui_sources} ${imgui_sources_cpp})
        set(IMGUI_VERSION "1.91.0")

        if(BUILD_SHARED_LIBS)
            add_library(imgui STATIC ${IMGUI_SOURCES})
        else()
            add_library(imgui ${IMGUI_SOURCES})
        endif()

        target_compile_features(imgui PUBLIC cxx_std_20)

        target_sources(imgui PUBLIC
                       FILE_SET public_headers
                       TYPE HEADERS
                       BASE_DIRS ${IMGUI_INCLUDE_DIRS}
                       FILES ${IMGUI_INCLUDE_DIRS}/imgui.h)

        set_target_properties(imgui PROPERTIES FOLDER "Dependencies")

        target_compile_options(imgui PRIVATE "-w")

        target_include_directories(imgui
                                   SYSTEM PUBLIC $<BUILD_INTERFACE:${IMGUI_INCLUDE_DIRS}>
                                                 $<INSTALL_INTERFACE:include>)

        # add flags required for emscripten support (e.g. pthread)
        if(SFML_OS_EMSCRIPTEN)
            target_compile_options(imgui PRIVATE ${SFML_EMSCRIPTEN_TARGET_COMPILE_OPTIONS})
            target_link_options(imgui PRIVATE ${SFML_EMSCRIPTEN_TARGET_LINK_OPTIONS})
        endif()

        # if building SFML as a shared library and linking our dependencies in
        # as static libraries we need to build them with -fPIC
        if(SFML_BUILD_SHARED_LIBS)
            set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
        endif()

        sfml_set_stdlib(imgui)
        add_library(ImGui::ImGui ALIAS imgui)

        # TODO P0: this is hardcoded, should follow same pattern as Macros.cmake
        install(TARGETS imgui EXPORT SFMLImGuiStaticTargets FILE_SET public_headers)
    endfunction()

    sfml_add_imgui_dependencies()
endif()

target_link_libraries(sfml-imgui PRIVATE ImGui::ImGui)
