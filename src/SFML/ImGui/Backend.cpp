#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/ImGui/Backend.hpp"

#include "SFML/Window/GLCheck.hpp"
#include "SFML/Window/Glad.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#if defined(__APPLE__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

// NOLINTBEGIN

#define IMGUI_IMPL_OPENGL_ES3
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#define IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef IMGUI_DISABLE
#include <stdint.h> // intptr_t
#include <stdio.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option" // warning: ignore unknown flags
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning: use of old-style cast
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wunused-macros"          // warning: macro is not used
#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#pragma clang diagnostic ignored "-Wcast-function-type" // warning: cast between incompatible function types (for loader)
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"                // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunknown-warning-option" // warning: unknown warning group 'xxx'
#pragma GCC diagnostic ignored "-Wcast-function-type" // warning: cast between incompatible function types (for loader)
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

// Vertex arrays are not supported on ES2/WebGL1 unless Emscripten which uses an extension
#ifndef IMGUI_IMPL_OPENGL_ES2
#define IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
#elif defined(__EMSCRIPTEN__)
#define IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
#define glBindVertexArray       glBindVertexArrayOES
#define glGenVertexArrays       glGenVertexArraysOES
#define glDeleteVertexArrays    glDeleteVertexArraysOES
#define GL_VERTEX_ARRAY_BINDING GL_VERTEX_ARRAY_BINDING_OES
#endif

// Desktop GL 2.0+ has extension and glPolygonMode() which GL ES and WebGL don't have..
// A desktop ES context can technically compile fine with our loader, so we also perform a runtime checks
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3)
#define IMGUI_IMPL_OPENGL_HAS_EXTENSIONS        // has glGetIntegerv(GL_NUM_EXTENSIONS)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_POLYGON_MODE // may have glPolygonMode()
#endif

// Desktop GL 2.1+ and GL ES 3.0+ have glBindBuffer() with GL_PIXEL_UNPACK_BUFFER target.
#if !defined(IMGUI_IMPL_OPENGL_ES2)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_BUFFER_PIXEL_UNPACK
#endif

// Desktop GL 3.1+ has GL_PRIMITIVE_RESTART state
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && defined(GL_VERSION_3_1)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
#endif

// Desktop GL 3.2+ has glDrawElementsBaseVertex() which GL ES and WebGL don't have.
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && defined(GL_VERSION_3_2)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
#endif

// Desktop GL 3.3+ and GL ES 3.0+ have glBindSampler()
#if !defined(IMGUI_IMPL_OPENGL_ES2) && (defined(IMGUI_IMPL_OPENGL_ES3) || defined(GL_VERSION_3_3))
#define IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
#endif

namespace sf::ImGui::priv
{

// OpenGL Data
struct ImGui_ImplOpenGL3_Data
{
    GLuint GlVersion; // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g. 320 for GL 3.2)
    char   GlslVersionString[32]; // Specified by user or detected based on compile time GL settings.
    bool   GlProfileIsES2;
    bool   GlProfileIsES3;
    bool   GlProfileIsCompat;
    GLint  GlProfileMask;
    GLuint FontTexture;
    GLuint ShaderHandle;
    GLint  AttribLocationTex; // Uniforms location
    GLint  AttribLocationProjMtx;
    GLuint AttribLocationVtxPos; // Vertex attributes location
    GLuint AttribLocationVtxUV;
    GLuint AttribLocationVtxColor;
    unsigned int VboHandle, ElementsHandle;
    GLsizeiptr   VertexBufferSize;
    GLsizeiptr   IndexBufferSize;
    bool         HasPolygonMode;
    bool         HasClipOrigin;
    bool         UseBufferSubData;

    ImGui_ImplOpenGL3_Data()
    {
        memset((void*)this, 0, sizeof(*this));
    }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplOpenGL3_Data* ImGui_ImplOpenGL3_GetBackendData()
{
    return ::ImGui::GetCurrentContext() ? (ImGui_ImplOpenGL3_Data*)::ImGui::GetIO().BackendRendererUserData : nullptr;
}

// OpenGL vertex attribute state (for ES 1.0 and ES 2.0 only)
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
struct ImGui_ImplOpenGL3_VtxAttribState
{
    GLint   Enabled, Size, Type, Normalized, Stride;
    GLvoid* Ptr;

    void GetState(GLint index)
    {
        glCheck(glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &Enabled));
        glCheck(glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &Size));
        glCheck(glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &Type));
        glCheck(glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &Normalized));
        glCheck(glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &Stride));
        glCheck(glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &Ptr));
    }
    void SetState(GLint index)
    {
        glCheck(glVertexAttribPointer(index, Size, Type, (GLboolean)Normalized, Stride, Ptr));
        if (Enabled)
            glCheck(glEnableVertexAttribArray(index));
        else
            glCheck(glDisableVertexAttribArray(index));
    }
};
#endif

// Functions
bool ImGui_ImplOpenGL3_Init(const char* glsl_version)
{
    ImGuiIO& io = ::ImGui::GetIO();
    ::IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Initialize our loader
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
    if (imgl3wInit() != 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return false;
    }
#endif

    // Setup backend capabilities flags
#define IM_NEW2(_TYPE) new (ImNewWrapper(), ::ImGui::MemAlloc(sizeof(_TYPE))) _TYPE
    ImGui_ImplOpenGL3_Data* bd = IM_NEW2(ImGui_ImplOpenGL3_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName     = "imgui_impl_opengl3";

    // Query for GL version (e.g. 320 for GL 3.2)
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GLES 2
    bd->GlVersion      = 200;
    bd->GlProfileIsES2 = true;
#else
    // Desktop or GLES 3
    const char* gl_version_str = (const char*)glGetString(GL_VERSION);
    GLint       major          = 0;
    GLint       minor          = 0;
    glCheckIgnoreWithFunc(glGetError, glGetIntegerv(GL_MAJOR_VERSION, &major));
    glCheckIgnoreWithFunc(glGetError, glGetIntegerv(GL_MINOR_VERSION, &minor));

    // Need to drain errors here or subsequent assertion will fail on Emscripten
    while (glGetError() != GL_NO_ERROR)
        ;

    if (major == 0 && minor == 0)
        sscanf(gl_version_str, "%d.%d", &major, &minor); // Query GL_VERSION in desktop GL 2.x, the string will start with "<major>.<minor>"
    bd->GlVersion = (GLuint)(major * 100 + minor * 10);
#if defined(GL_CONTEXT_PROFILE_MASK)
    if (bd->GlVersion >= 320)
        glCheck(glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &bd->GlProfileMask));
    bd->GlProfileIsCompat = (bd->GlProfileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0;
#endif

#if defined(IMGUI_IMPL_OPENGL_ES3)
    bd->GlProfileIsES3 = true;
#else
    if (strncmp(gl_version_str, "OpenGL ES 3", 11) == 0)
        bd->GlProfileIsES3 = true;
#endif

    bd->UseBufferSubData = false;
    /*
    // Query vendor to enable glBufferSubData kludge
#ifdef _WIN32
    if (const char* vendor = (const char*)glGetString(GL_VENDOR))
        if (strncmp(vendor, "Intel", 5) == 0)
            bd->UseBufferSubData = true;
#endif
    */
#endif

#ifdef IMGUI_IMPL_OPENGL_DEBUG
    printf(
        "GlVersion = %d, \"%s\"\nGlProfileIsCompat = %d\nGlProfileMask = 0x%X\nGlProfileIsES2 = %d, GlProfileIsES3 = "
        "%d\nGL_VENDOR = '%s'\nGL_RENDERER = '%s'\n",
        bd->GlVersion,
        gl_version_str,
        bd->GlProfileIsCompat,
        bd->GlProfileMask,
        bd->GlProfileIsES2,
        bd->GlProfileIsES3,
        (const char*)glCheck(glGetString(GL_VENDOR)),
        (const char*)glCheck(glGetString(GL_RENDERER))); // [DEBUG]
#endif

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
    if (bd->GlVersion >= 320)
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
#endif

    // Store GLSL version string so we can refer to it later in case we recreate shaders.
    // Note: GLSL version is NOT the same as GL version. Leave this to nullptr if unsure.
    if (glsl_version == nullptr)
    {
#if defined(IMGUI_IMPL_OPENGL_ES2)
        glsl_version = "#version 100";
#elif defined(IMGUI_IMPL_OPENGL_ES3)
        glsl_version = "#version 300 es";
#elif defined(__APPLE__)
        glsl_version = "#version 150";
#else
        glsl_version = "#version 130";
#endif
    }
    IM_ASSERT((int)strlen(glsl_version) + 2 < IM_ARRAYSIZE(bd->GlslVersionString));
    strcpy(bd->GlslVersionString, glsl_version);
    strcat(bd->GlslVersionString, "\n");

    // Make an arbitrary GL call (we don't actually need the result)
    // IF YOU GET A CRASH HERE: it probably means the OpenGL function loader didn't do its job. Let us know!
    GLint current_texture;
    glCheck(glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture));

    // Detect extensions we support
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_POLYGON_MODE
    bd->HasPolygonMode = (!bd->GlProfileIsES2 && !bd->GlProfileIsES3);
#endif
    bd->HasClipOrigin = (bd->GlVersion >= 450);
#ifdef IMGUI_IMPL_OPENGL_HAS_EXTENSIONS
    GLint num_extensions = 0;
    glCheck(glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions));
    for (GLint i = 0; i < num_extensions; i++)
    {
        const char* extension = (const char*)glCheck(glGetStringi(GL_EXTENSIONS, i));
        if (extension != nullptr && strcmp(extension, "GL_ARB_clip_control") == 0)
            bd->HasClipOrigin = true;
    }
#endif

    return true;
}

void ImGui_ImplOpenGL3_DestroyDeviceObjects();
bool ImGui_ImplOpenGL3_CreateDeviceObjects();
bool ImGui_ImplOpenGL3_CreateFontsTexture();

void ImGui_ImplOpenGL3_Shutdown()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ::ImGui::GetIO();

    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    io.BackendRendererName     = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    IM_DELETE(bd);
}

void ImGui_ImplOpenGL3_NewFrame()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplOpenGL3_Init()?");

    if (!bd->ShaderHandle)
        ImGui_ImplOpenGL3_CreateDeviceObjects();
    if (!bd->FontTexture)
        ImGui_ImplOpenGL3_CreateFontsTexture();
}

static void ImGui_ImplOpenGL3_SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height, GLuint vertex_array_object)
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    glCheck(glEnable(GL_BLEND));
    glCheck(glBlendEquation(GL_FUNC_ADD));
    glCheck(glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    glCheck(glDisable(GL_CULL_FACE));
    glCheck(glDisable(GL_DEPTH_TEST));
    glCheck(glDisable(GL_STENCIL_TEST));
    glCheck(glEnable(GL_SCISSOR_TEST));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    if (bd->GlVersion >= 310)
        glCheck(glDisable(GL_PRIMITIVE_RESTART));
#endif
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_POLYGON_MODE
    if (bd->HasPolygonMode)
        glCheck(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
#endif

        // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
#if defined(GL_CLIP_ORIGIN)
    bool clip_origin_lower_left = true;
    if (bd->HasClipOrigin)
    {
        GLenum current_clip_origin = 0;
        glCheck(glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&current_clip_origin));
        if (current_clip_origin == GL_UPPER_LEFT)
            clip_origin_lower_left = false;
    }
#endif

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    glCheck(glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height));
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
#if defined(GL_CLIP_ORIGIN)
    if (!clip_origin_lower_left)
    {
        float tmp = T;
        T         = B;
        B         = tmp;
    } // Swap top and bottom if origin is upper left
#endif
    const float ortho_projection[4][4] = {
        {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };
    glCheck(glUseProgram(bd->ShaderHandle));
    glCheck(glUniform1i(bd->AttribLocationTex, 0));
    glCheck(glUniformMatrix4fv(bd->AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]));

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    if (bd->GlVersion >= 330 || bd->GlProfileIsES3)
        glCheck(glBindSampler(0, 0)); // We use combined texture/sampler state. Applications using GL 3.3 and GL ES 3.0 may set that otherwise.
#endif

    (void)vertex_array_object;
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glCheck(glBindVertexArray(vertex_array_object));
#endif

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, bd->VboHandle));
    glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ElementsHandle));
    glCheck(glEnableVertexAttribArray(bd->AttribLocationVtxPos));
    glCheck(glEnableVertexAttribArray(bd->AttribLocationVtxUV));
    glCheck(glEnableVertexAttribArray(bd->AttribLocationVtxColor));
    glCheck(glVertexAttribPointer(bd->AttribLocationVtxPos,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(ImDrawVert),
                                  (GLvoid*)offsetof(ImDrawVert, pos)));
    glCheck(
        glVertexAttribPointer(bd->AttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv)));
    glCheck(glVertexAttribPointer(bd->AttribLocationVtxColor,
                                  4,
                                  GL_UNSIGNED_BYTE,
                                  GL_TRUE,
                                  sizeof(ImDrawVert),
                                  (GLvoid*)offsetof(ImDrawVert, col)));
}

// OpenGL3 Render function.
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly.
// This is in order to be able to run within an OpenGL engine that doesn't do so.
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width  = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    SFML_BASE_ASSERT(bd != nullptr);

    // Backup GL state
    GLenum last_active_texture;
    glCheck(glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture));
    glCheck(glActiveTexture(GL_TEXTURE0));
    GLuint last_program;
    glCheck(glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&last_program));
    GLuint last_texture;
    glCheck(glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&last_texture));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    GLuint last_sampler;
    if (bd->GlVersion >= 330 || bd->GlProfileIsES3)
    {
        glCheck(glGetIntegerv(GL_SAMPLER_BINDING, (GLint*)&last_sampler));
    }
    else
    {
        last_sampler = 0;
    }
#endif
    GLuint last_array_buffer;
    glCheck(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&last_array_buffer));
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    // This is part of VAO on OpenGL 3.0+ and OpenGL ES 3.0+.
    GLint last_element_array_buffer;
    glCheck(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer));
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_pos;
    last_vtx_attrib_state_pos.GetState(bd->AttribLocationVtxPos);
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_uv;
    last_vtx_attrib_state_uv.GetState(bd->AttribLocationVtxUV);
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_color;
    last_vtx_attrib_state_color.GetState(bd->AttribLocationVtxColor);
#endif
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    GLuint last_vertex_array_object;
    glCheck(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&last_vertex_array_object));
#endif
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_POLYGON_MODE
    GLint last_polygon_mode[2];
    if (bd->HasPolygonMode)
    {
        glCheck(glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode));
    }
#endif
    GLint last_viewport[4];
    glCheck(glGetIntegerv(GL_VIEWPORT, last_viewport));
    GLint last_scissor_box[4];
    glCheck(glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box));
    GLenum last_blend_src_rgb;
    glCheck(glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb));
    GLenum last_blend_dst_rgb;
    glCheck(glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb));
    GLenum last_blend_src_alpha;
    glCheck(glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha));
    GLenum last_blend_dst_alpha;
    glCheck(glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha));
    GLenum last_blend_equation_rgb;
    glCheck(glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb));
    GLenum last_blend_equation_alpha;
    glCheck(glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha));
    GLboolean last_enable_blend        = glCheck(glIsEnabled(GL_BLEND));
    GLboolean last_enable_cull_face    = glCheck(glIsEnabled(GL_CULL_FACE));
    GLboolean last_enable_depth_test   = glCheck(glIsEnabled(GL_DEPTH_TEST));
    GLboolean last_enable_stencil_test = glCheck(glIsEnabled(GL_STENCIL_TEST));
    GLboolean last_enable_scissor_test = glCheck(glIsEnabled(GL_SCISSOR_TEST));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    GLboolean last_enable_primitive_restart = (bd->GlVersion >= 310) ? glCheck(glIsEnabled(GL_PRIMITIVE_RESTART)) : GL_FALSE;
#endif

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to. VAO are not shared among GL contexts)
    // The renderer would actually work without any VAO bound, but then our VertexAttrib calls would overwrite the default one currently bound.
    GLuint vertex_array_object = 0;
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glCheck(glGenVertexArrays(1, &vertex_array_object));
#endif
    ImGui_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off   = draw_data->DisplayPos;       // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        // - OpenGL drivers are in a very sorry state nowadays....
        //   During 2021 we attempted to switch from glBufferData() to orphaning+glBufferSubData() following reports
        //   of leaks on Intel GPU when using multi-viewports on Windows.
        // - After this we kept hearing of various display corruptions issues. We started disabling on non-Intel GPU, but issues still got reported on Intel.
        // - We are now back to using exclusively glBufferData(). So bd->UseBufferSubData IS ALWAYS FALSE in this code.
        //   We are keeping the old code path for a while in case people finding new issues may want to test the bd->UseBufferSubData path.
        // - See https://github.com/ocornut/imgui/issues/4468 and please report any corruption issues.
        const GLsizeiptr vtx_buffer_size = (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
        const GLsizeiptr idx_buffer_size = (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);
        if (bd->UseBufferSubData)
        {
            if (bd->VertexBufferSize < vtx_buffer_size)
            {
                bd->VertexBufferSize = vtx_buffer_size;
                glCheck(glBufferData(GL_ARRAY_BUFFER, bd->VertexBufferSize, nullptr, GL_STREAM_DRAW));
            }
            if (bd->IndexBufferSize < idx_buffer_size)
            {
                bd->IndexBufferSize = idx_buffer_size;
                glCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER, bd->IndexBufferSize, nullptr, GL_STREAM_DRAW));
            }
            glCheck(
                glBufferSubData(GL_ARRAY_BUFFER, 0, vtx_buffer_size, reinterpret_cast<const GLvoid*>(cmd_list->VtxBuffer.Data)));
            glCheck(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                                    0,
                                    idx_buffer_size,
                                    reinterpret_cast<const GLvoid*>(cmd_list->IdxBuffer.Data)));
        }
        else
        {
            glCheck(glBufferData(GL_ARRAY_BUFFER,
                                 vtx_buffer_size,
                                 reinterpret_cast<const GLvoid*>(cmd_list->VtxBuffer.Data),
                                 GL_STREAM_DRAW));
            glCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                 idx_buffer_size,
                                 reinterpret_cast<const GLvoid*>(cmd_list->IdxBuffer.Data),
                                 GL_STREAM_DRAW));
        }

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                glCheck(glScissor((int)clip_min.x,
                                  (int)((float)fb_height - clip_max.y),
                                  (int)(clip_max.x - clip_min.x),
                                  (int)(clip_max.y - clip_min.y)));

                // Bind texture, Draw
                glCheck(glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID()));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
                if (bd->GlVersion >= 320)
                    glCheck(glDrawElementsBaseVertex(GL_TRIANGLES,
                                                     (GLsizei)pcmd->ElemCount,
                                                     sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                                     (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)),
                                                     (GLint)pcmd->VtxOffset));
                else
#endif
                    glCheck(glDrawElements(GL_TRIANGLES,
                                           (GLsizei)pcmd->ElemCount,
                                           sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                           (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx))));
            }
        }
    }

    // Destroy the temporary VAO
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glCheck(glDeleteVertexArrays(1, &vertex_array_object));
#endif

    // Restore modified GL state
    // This "glIsProgram()" check is required because if the program is "pending deletion" at the time of binding backup, it will have been deleted by now and will cause an OpenGL error. See #6220.
    if (last_program == 0 || glCheck(glIsProgram(last_program)))
        glCheck(glUseProgram(last_program));
    glCheck(glBindTexture(GL_TEXTURE_2D, last_texture));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    if (bd->GlVersion >= 330 || bd->GlProfileIsES3)
        glCheck(glBindSampler(0, last_sampler));
#endif
    glCheck(glActiveTexture(last_active_texture));
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glCheck(glBindVertexArray(last_vertex_array_object));
#endif
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer));
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer));
    last_vtx_attrib_state_pos.SetState(bd->AttribLocationVtxPos);
    last_vtx_attrib_state_uv.SetState(bd->AttribLocationVtxUV);
    last_vtx_attrib_state_color.SetState(bd->AttribLocationVtxColor);
#endif
    glCheck(glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha));
    glCheck(glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha));
    if (last_enable_blend)
        glCheck(glEnable(GL_BLEND));
    else
        glCheck(glDisable(GL_BLEND));
    if (last_enable_cull_face)
        glCheck(glEnable(GL_CULL_FACE));
    else
        glCheck(glDisable(GL_CULL_FACE));
    if (last_enable_depth_test)
        glCheck(glEnable(GL_DEPTH_TEST));
    else
        glCheck(glDisable(GL_DEPTH_TEST));
    if (last_enable_stencil_test)
        glCheck(glEnable(GL_STENCIL_TEST));
    else
        glCheck(glDisable(GL_STENCIL_TEST));
    if (last_enable_scissor_test)
        glCheck(glEnable(GL_SCISSOR_TEST));
    else
        glCheck(glDisable(GL_SCISSOR_TEST));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    if (bd->GlVersion >= 310)
    {
        if (last_enable_primitive_restart)
            glCheck(glEnable(GL_PRIMITIVE_RESTART));
        else
            glCheck(glDisable(GL_PRIMITIVE_RESTART));
    }
#endif

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_POLYGON_MODE
    // Desktop OpenGL 3.0 and OpenGL 3.1 had separate polygon draw modes for front-facing and back-facing faces of polygons
    if (bd->HasPolygonMode)
    {
        if (bd->GlVersion <= 310 || bd->GlProfileIsCompat)
        {
            glCheck(glPolygonMode(GL_FRONT, (GLenum)last_polygon_mode[0]));
            glCheck(glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]));
        }
        else
        {
            glCheck(glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]));
        }
    }
#endif // IMGUI_IMPL_OPENGL_MAY_HAVE_POLYGON_MODE

    glCheck(glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]));
    glCheck(glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]));
    (void)bd; // Not all compilation paths use this
}

bool ImGui_ImplOpenGL3_CreateFontsTexture()
{
    ImGuiIO&                io = ::ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Build texture atlas
    unsigned char* pixels;
    int            width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height); // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    GLint last_texture;
    glCheck(glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture));
    glCheck(glGenTextures(1, &bd->FontTexture));
    glCheck(glBindTexture(GL_TEXTURE_2D, bd->FontTexture));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
#ifdef GL_UNPACK_ROW_LENGTH // Not on WebGL/ES
    glCheck(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
#endif
    glCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels));

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);

    // Restore state
    glCheck(glBindTexture(GL_TEXTURE_2D, last_texture));

    return true;
}

void ImGui_ImplOpenGL3_DestroyFontsTexture()
{
    ImGuiIO&                io = ::ImGui::GetIO();
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->FontTexture)
    {
        glCheck(glDeleteTextures(1, &bd->FontTexture));
        io.Fonts->SetTexID(0);
        bd->FontTexture = 0;
    }
}

// If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
[[nodiscard]] static bool CheckShader(GLuint handle, const char* desc)
{
    ImGui_ImplOpenGL3_Data* bd     = ImGui_ImplOpenGL3_GetBackendData();
    GLint                   status = 0, log_length = 0;
    glCheck(glGetShaderiv(handle, GL_COMPILE_STATUS, &status));
    glCheck(glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length));
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr,
                "ERROR: ImGui_ImplOpenGL3_CreateDeviceObjects: failed to compile %s! With GLSL: %s\n",
                desc,
                bd->GlslVersionString);
    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glCheck(glGetShaderInfoLog(handle, log_length, nullptr, (GLchar*)buf.begin()));
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
[[nodiscard]] static bool CheckProgram(GLuint handle, const char* desc)
{
    ImGui_ImplOpenGL3_Data* bd     = ImGui_ImplOpenGL3_GetBackendData();
    GLint                   status = 0, log_length = 0;
    glCheck(glGetProgramiv(handle, GL_LINK_STATUS, &status));
    glCheck(glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length));
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr,
                "ERROR: ImGui_ImplOpenGL3_CreateDeviceObjects: failed to link %s! With GLSL %s\n",
                desc,
                bd->GlslVersionString);
    if (log_length > 1)
    {
        ImVector<char> buf;
        buf.resize((int)(log_length + 1));
        glCheck(glGetProgramInfoLog(handle, log_length, nullptr, (GLchar*)buf.begin()));
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

[[nodiscard]] bool ImGui_ImplOpenGL3_CreateDeviceObjects()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLint last_texture, last_array_buffer;
    glCheck(glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture));
    glCheck(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_BUFFER_PIXEL_UNPACK
    GLint last_pixel_unpack_buffer = 0;
    if (bd->GlVersion >= 210)
    {
        glCheck(glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &last_pixel_unpack_buffer));
        glCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    }
#endif
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    GLint last_vertex_array;
    glCheck(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array));
#endif

    // Parse GLSL version string
    int glsl_version = 130;
    sscanf(bd->GlslVersionString, "#version %d", &glsl_version);

    const GLchar* vertex_shader_glsl_120 =
        "uniform mat4 ProjMtx;\n"
        "attribute vec2 Position;\n"
        "attribute vec2 UV;\n"
        "attribute vec4 Color;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* vertex_shader_glsl_130 =
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 UV;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* vertex_shader_glsl_300_es =
        "precision highp float;\n"
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* vertex_shader_glsl_410_core =
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "uniform mat4 ProjMtx;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const GLchar* fragment_shader_glsl_120 =
        "#ifdef GL_ES\n"
        "    precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar* fragment_shader_glsl_130 =
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar* fragment_shader_glsl_300_es =
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    const GLchar* fragment_shader_glsl_410_core =
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D Texture;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    // Select shaders matching our GLSL versions
    const GLchar* vertex_shader   = nullptr;
    const GLchar* fragment_shader = nullptr;
    if (glsl_version < 130)
    {
        vertex_shader   = vertex_shader_glsl_120;
        fragment_shader = fragment_shader_glsl_120;
    }
    else if (glsl_version >= 410)
    {
        vertex_shader   = vertex_shader_glsl_410_core;
        fragment_shader = fragment_shader_glsl_410_core;
    }
    else if (glsl_version == 300)
    {
        vertex_shader   = vertex_shader_glsl_300_es;
        fragment_shader = fragment_shader_glsl_300_es;
    }
    else
    {
        vertex_shader   = vertex_shader_glsl_130;
        fragment_shader = fragment_shader_glsl_130;
    }

    // Create shaders
    const GLchar* vertex_shader_with_version[2] = {bd->GlslVersionString, vertex_shader};
    GLuint        vert_handle                   = glCheck(glCreateShader(GL_VERTEX_SHADER));
    glCheck(glShaderSource(vert_handle, 2, vertex_shader_with_version, nullptr));
    glCheck(glCompileShader(vert_handle));
    if (!CheckShader(vert_handle, "vertex shader"))
        return false;

    const GLchar* fragment_shader_with_version[2] = {bd->GlslVersionString, fragment_shader};
    GLuint        frag_handle                     = glCheck(glCreateShader(GL_FRAGMENT_SHADER));
    glCheck(glShaderSource(frag_handle, 2, fragment_shader_with_version, nullptr));
    glCheck(glCompileShader(frag_handle));
    if (!CheckShader(frag_handle, "fragment shader"))
        return false;

    // Link
    bd->ShaderHandle = glCheck(glCreateProgram());
    glCheck(glAttachShader(bd->ShaderHandle, vert_handle));
    glCheck(glAttachShader(bd->ShaderHandle, frag_handle));
    glCheck(glLinkProgram(bd->ShaderHandle));
    if (!CheckProgram(bd->ShaderHandle, "shader program"))
        return false;

    glCheck(glDetachShader(bd->ShaderHandle, vert_handle));
    glCheck(glDetachShader(bd->ShaderHandle, frag_handle));
    glCheck(glDeleteShader(vert_handle));
    glCheck(glDeleteShader(frag_handle));

    bd->AttribLocationTex      = glCheck(glGetUniformLocation(bd->ShaderHandle, "Texture"));
    bd->AttribLocationProjMtx  = glCheck(glGetUniformLocation(bd->ShaderHandle, "ProjMtx"));
    bd->AttribLocationVtxPos   = glCheck((GLuint)glGetAttribLocation(bd->ShaderHandle, "Position"));
    bd->AttribLocationVtxUV    = glCheck((GLuint)glGetAttribLocation(bd->ShaderHandle, "UV"));
    bd->AttribLocationVtxColor = glCheck((GLuint)glGetAttribLocation(bd->ShaderHandle, "Color"));

    // Create buffers
    glCheck(glGenBuffers(1, &bd->VboHandle));
    glCheck(glGenBuffers(1, &bd->ElementsHandle));

    ImGui_ImplOpenGL3_CreateFontsTexture();

    // Restore modified GL state
    glCheck(glBindTexture(GL_TEXTURE_2D, last_texture));
    glCheck(glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_BUFFER_PIXEL_UNPACK
    if (bd->GlVersion >= 210)
    {
        glCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, last_pixel_unpack_buffer));
    }
#endif
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glCheck(glBindVertexArray(last_vertex_array));
#endif

    return true;
}

void ImGui_ImplOpenGL3_DestroyDeviceObjects()
{
    ImGui_ImplOpenGL3_Data* bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->VboHandle)
    {
        glCheck(glDeleteBuffers(1, &bd->VboHandle));
        bd->VboHandle = 0;
    }
    if (bd->ElementsHandle)
    {
        glCheck(glDeleteBuffers(1, &bd->ElementsHandle));
        bd->ElementsHandle = 0;
    }
    if (bd->ShaderHandle)
    {
        glCheck(glDeleteProgram(bd->ShaderHandle));
        bd->ShaderHandle = 0;
    }
    ImGui_ImplOpenGL3_DestroyFontsTexture();
}

//-----------------------------------------------------------------------------

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif // #ifndef IMGUI_DISABLE
}

// NOLINTEND
