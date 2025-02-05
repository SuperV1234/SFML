#include <SFML/Copyright.hpp> // LICENSE AND COPYRIGHT (C) INFORMATION


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "SFML/Config.hpp"

#include "SFML/Graphics/GraphicsContext.hpp"
#include "SFML/Graphics/Shader.hpp"
#include "SFML/Graphics/Texture.hpp"

#include "SFML/Window/GLCheck.hpp"
#include "SFML/Window/GLUtils.hpp"
#include "SFML/Window/Glad.hpp"

#include "SFML/System/Err.hpp"
#include "SFML/System/InputStream.hpp"
#include "SFML/System/Path.hpp"
#include "SFML/System/PathUtils.hpp"
#include "SFML/System/Vector2.hpp"
#include "SFML/System/Vector3.hpp"

#include "SFML/Base/Algorithm.hpp"
#include "SFML/Base/Assert.hpp"
#include "SFML/Base/Macros.hpp"
#include "SFML/Base/Optional.hpp"
#include "SFML/Base/StringView.hpp"
#include "SFML/Base/TrivialVector.hpp"

#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>


using GLhandle = GLuint;

#if defined(SFML_SYSTEM_MACOS) || defined(SFML_SYSTEM_IOS)

#include "SFML/Base/PtrDiffT.hpp"

#define castToGlHandle(x)   reinterpret_cast<GLEXT_GLhandle>(::sf::base::PtrDiffT{x})
#define castFromGlHandle(x) static_cast<unsigned int>(reinterpret_cast<::sf::base::PtrDiffT>(x))

#else

#define castToGlHandle(x)   (x)
#define castFromGlHandle(x) (x)

#endif


namespace
{
////////////////////////////////////////////////////////////
// Retrieve the maximum number of texture units available
[[nodiscard]] sf::base::SizeT getMaxTextureUnits()
{
    static const auto maxUnits = static_cast<sf::base::SizeT>(sf::priv::getGLInteger(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS));
    return maxUnits;
}


////////////////////////////////////////////////////////////
// Pair of indices into thread-local buffer
struct [[nodiscard]] BufferSlice
{
    const sf::base::SizeT beginIdx;
    const sf::base::SizeT count;

    [[nodiscard]] explicit BufferSlice(sf::base::SizeT b, sf::base::SizeT c) : beginIdx(b), count(c)
    {
    }

    [[nodiscard]] sf::base::StringView toView(const sf::base::TrivialVector<char>& buffer) const
    {
        return {buffer.data() + beginIdx, count};
    }
};


////////////////////////////////////////////////////////////
// Read the contents of a file into an array of char
[[nodiscard]] sf::base::Optional<BufferSlice> appendFileContentsToVector(const sf::Path&                filename,
                                                                         sf::base::TrivialVector<char>& buffer)
{
    std::ifstream file(filename.to<std::string>(), std::ios_base::binary);

    if (!file)
    {
        sf::priv::err() << "Failed to open shader file";
        return sf::base::nullOpt;
    }

    file.seekg(0, std::ios_base::end);
    const std::ifstream::pos_type size = file.tellg();

    const sf::base::SizeT bufferSizeBeforeRead = buffer.size();

    if (size > 0)
    {
        file.seekg(0, std::ios_base::beg);
        buffer.resize(static_cast<sf::base::SizeT>(size) + bufferSizeBeforeRead);
        file.read(buffer.data() + bufferSizeBeforeRead, static_cast<std::streamsize>(size));
    }

    buffer.pushBack('\0');
    return sf::base::makeOptional<BufferSlice>(bufferSizeBeforeRead, buffer.size() - bufferSizeBeforeRead);
}


////////////////////////////////////////////////////////////
// Read the contents of a stream into an array of char
[[nodiscard]] sf::base::Optional<BufferSlice> appendStreamContentsToVector(sf::InputStream&               stream,
                                                                           sf::base::TrivialVector<char>& buffer)
{
    const sf::base::Optional<sf::base::SizeT> size = stream.getSize();

    if (!size.hasValue() || size.value() == 0)
    {
        buffer.pushBack('\0');
        return sf::base::nullOpt;
    }

    const sf::base::SizeT bufferSizeBeforeRead = buffer.size();
    buffer.resize(*size + bufferSizeBeforeRead);

    if (!stream.seek(0).hasValue())
    {
        sf::priv::err() << "Failed to seek shader stream";
        return sf::base::nullOpt;
    }

    const sf::base::Optional<sf::base::SizeT> read = stream.read(buffer.data() + bufferSizeBeforeRead, *size);

    if (!read.hasValue() || *read != *size)
    {
        sf::priv::err() << "Failed to read stream contents into buffer";
        return sf::base::nullOpt;
    }

    buffer.pushBack('\0');
    return sf::base::makeOptional<BufferSlice>(bufferSizeBeforeRead, buffer.size() - bufferSizeBeforeRead);
}


////////////////////////////////////////////////////////////
// Return a thread-local vector for suitable use as a temporary buffer
// This function is non-reentrant
[[nodiscard]] sf::base::TrivialVector<char>& getThreadLocalCharBuffer()
{
    thread_local sf::base::TrivialVector<char> result;
    return result;
}


////////////////////////////////////////////////////////////
// Transforms an array of 2D vectors into a contiguous array of scalars
[[nodiscard]] sf::base::TrivialVector<float> flatten(const sf::Vector2f* vectorArray, sf::base::SizeT length)
{
    const sf::base::SizeT vectorSize = 2;

    sf::base::TrivialVector<float> contiguous(vectorSize * length);

    for (sf::base::SizeT i = 0; i < length; ++i)
    {
        contiguous[vectorSize * i]     = vectorArray[i].x;
        contiguous[vectorSize * i + 1] = vectorArray[i].y;
    }

    return contiguous;
}


////////////////////////////////////////////////////////////
// Transforms an array of 3D vectors into a contiguous array of scalars
[[nodiscard]] sf::base::TrivialVector<float> flatten(const sf::Vector3f* vectorArray, sf::base::SizeT length)
{
    const sf::base::SizeT vectorSize = 3;

    sf::base::TrivialVector<float> contiguous(vectorSize * length);

    for (sf::base::SizeT i = 0; i < length; ++i)
    {
        contiguous[vectorSize * i]     = vectorArray[i].x;
        contiguous[vectorSize * i + 1] = vectorArray[i].y;
        contiguous[vectorSize * i + 2] = vectorArray[i].z;
    }

    return contiguous;
}


////////////////////////////////////////////////////////////
// Transforms an array of 4D vectors into a contiguous array of scalars
[[nodiscard]] sf::base::TrivialVector<float> flatten(const sf::Glsl::Vec4* vectorArray, sf::base::SizeT length)
{
    const sf::base::SizeT vectorSize = 4;

    sf::base::TrivialVector<float> contiguous(vectorSize * length);

    for (sf::base::SizeT i = 0; i < length; ++i)
    {
        contiguous[vectorSize * i]     = vectorArray[i].x;
        contiguous[vectorSize * i + 1] = vectorArray[i].y;
        contiguous[vectorSize * i + 2] = vectorArray[i].z;
        contiguous[vectorSize * i + 3] = vectorArray[i].w;
    }

    return contiguous;
}


////////////////////////////////////////////////////////////
struct StringHash
{
    using is_transparent = void;

    [[nodiscard, gnu::always_inline, gnu::flatten]] sf::base::SizeT operator()(const char* txt) const
    {
        return std::hash<std::string_view>{}(txt);
    }

    [[nodiscard, gnu::always_inline, gnu::flatten]] sf::base::SizeT operator()(sf::base::StringView txt) const
    {
        return std::hash<std::string_view>{}({txt.data(), txt.size()});
    }

    [[nodiscard, gnu::always_inline, gnu::flatten]] sf::base::SizeT operator()(const std::string& txt) const
    {
        return std::hash<std::string>{}(txt);
    }
};


////////////////////////////////////////////////////////////
struct StringEq
{
    using is_transparent = void;

    [[nodiscard, gnu::always_inline, gnu::flatten]] bool operator()(const sf::base::StringView& a, const std::string& b) const
    {
        return a == sf::base::StringView{b};
    }

    [[nodiscard, gnu::always_inline, gnu::flatten]] bool operator()(const std::string& a, const sf::base::StringView& b) const
    {
        return sf::base::StringView{a} == b;
    }

    [[nodiscard, gnu::always_inline]] bool operator()(const sf::base::StringView& a, const sf::base::StringView& b) const
    {
        return a == b;
    }

    [[nodiscard, gnu::always_inline]] bool operator()(const std::string& a, const std::string& b) const
    {
        return a == b;
    }
};


////////////////////////////////////////////////////////////
[[nodiscard]] sf::base::StringView adjustPreamble(sf::base::StringView src)
{
    constexpr sf::base::StringView preamble{
#if defined(SFML_SYSTEM_EMSCRIPTEN)

        // Emscripten/WebGL always requires `#version 300 es` and precision
        R"glsl(#version 300 es

precision mediump float;

)glsl"

#elif defined(SFML_OPENGL_ES)

        // Desktop/mobile GLES can use `#version 310 es` and precision
        R"glsl(#version 310 es

precision mediump float;

)glsl"

#else

        // Desktop GL can use `#version 430 core`
        R"glsl(#version 430 core

)glsl"

#endif
    };

    thread_local sf::base::TrivialVector<char> buffer; // Cannot reuse the other buffer here
    buffer.clear();

    buffer.emplaceRange(preamble.data(), preamble.size() - 1);
    buffer.emplaceRange(src.data(), src.size() - 1);

    return {buffer.data(), buffer.size()};
};

} // namespace


namespace sf
{
////////////////////////////////////////////////////////////
struct Shader::Impl
{
    using TextureTable = std::unordered_map<int, const Texture*>;
    using UniformTable = std::unordered_map<std::string, int, StringHash, StringEq>;

    unsigned int shaderProgram{};    //!< OpenGL identifier for the program
    int          currentTexture{-1}; //!< Location of the current texture in the shader

    // TODO P1: protect with mutex? Change API?
    mutable TextureTable textures; //!< Texture variables in the shader, mapped to their location
    mutable UniformTable uniforms; //!< Parameters location cache

    explicit Impl(unsigned int theShaderProgram) : shaderProgram(theShaderProgram)
    {
    }

    explicit Impl(Impl&& rhs) noexcept :
    shaderProgram(base::exchange(rhs.shaderProgram, 0u)),
    currentTexture(base::exchange(rhs.currentTexture, -1)),
    textures(SFML_BASE_MOVE(rhs.textures)),
    uniforms(SFML_BASE_MOVE(rhs.uniforms))
    {
    }
};


////////////////////////////////////////////////////////////
Shader::UniformLocation::UniformLocation(int location) : m_value(location)
{
    SFML_BASE_ASSERT(m_value != -1);
}


////////////////////////////////////////////////////////////
class [[nodiscard]] Shader::UniformBinder
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Constructor: set up state before uniform is set
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] explicit UniformBinder(unsigned int shaderProgram) :
    m_currentProgram(static_cast<GLhandle>(castToGlHandle(shaderProgram)))
    {
        SFML_BASE_ASSERT(m_currentProgram != 0);

        // Enable program object
        glCheck(glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&m_savedProgram)));

        if (m_currentProgram != m_savedProgram)
            glCheck(glUseProgram(m_currentProgram));
    }

    ////////////////////////////////////////////////////////////
    /// \brief Destructor: restore state after uniform is set
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] ~UniformBinder()
    {
        // Disable program object
        if (m_currentProgram && (m_currentProgram != m_savedProgram))
            glCheck(glUseProgram(m_savedProgram));
    }

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    UniformBinder(const UniformBinder&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    UniformBinder& operator=(const UniformBinder&) = delete;

private:
    GLhandle m_currentProgram; //!< Handle to the program object of the modified `sf::Shader` instance
    GLhandle m_savedProgram{}; //!< Handle to the previously active program object
};


////////////////////////////////////////////////////////////
Shader::~Shader()
{
    // Destroy effect program
    if (m_impl->shaderProgram)
    {
        SFML_BASE_ASSERT(GraphicsContext::hasActiveThreadLocalOrSharedGlContext());
        SFML_BASE_ASSERT(glCheck(glIsProgram(castToGlHandle(m_impl->shaderProgram))));
        glCheck(glDeleteProgram(castToGlHandle(m_impl->shaderProgram)));
    }
}


////////////////////////////////////////////////////////////
Shader::Shader(Shader&& source) noexcept = default;


////////////////////////////////////////////////////////////
Shader& Shader::operator=(Shader&& right) noexcept
{
    // Make sure we aren't moving ourselves.
    if (&right == this)
        return *this;

    // Destroy effect program
    if (m_impl->shaderProgram)
    {
        SFML_BASE_ASSERT(GraphicsContext::hasActiveThreadLocalOrSharedGlContext());
        SFML_BASE_ASSERT(glCheck(glIsProgram(castToGlHandle(m_impl->shaderProgram))));
        glCheck(glDeleteProgram(castToGlHandle(m_impl->shaderProgram)));
    }

    // Move the contents of right.
    m_impl->shaderProgram  = base::exchange(right.m_impl->shaderProgram, 0u);
    m_impl->currentTexture = base::exchange(right.m_impl->currentTexture, -1);
    m_impl->textures       = SFML_BASE_MOVE(right.m_impl->textures);
    m_impl->uniforms       = SFML_BASE_MOVE(right.m_impl->uniforms);

    return *this;
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromFile(const Path& filename, Type type)
{
    // Prepare thread-local buffer
    base::TrivialVector<char>& buffer = getThreadLocalCharBuffer();
    buffer.clear();

    // Read the file
    const base::Optional<BufferSlice> shaderSlice = appendFileContentsToVector(filename, buffer);
    if (!shaderSlice.hasValue())
    {
        priv::err() << "Failed to open shader file\n" << priv::PathDebugFormatter{filename};
        return base::nullOpt;
    }

    return loadFromMemory(shaderSlice->toView(buffer), type);
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromFile(const Path& vertexShaderFilename, const Path& fragmentShaderFilename)
{
    // Prepare thread-local buffer
    base::TrivialVector<char>& buffer = getThreadLocalCharBuffer();
    buffer.clear();

    // Read the vertex shader file
    const base::Optional<BufferSlice> vertexShaderSlice = appendFileContentsToVector(vertexShaderFilename, buffer);
    if (!vertexShaderSlice.hasValue())
    {
        priv::err() << "Failed to open vertex shader file\n" << priv::PathDebugFormatter{vertexShaderFilename};
        return base::nullOpt;
    }

    // Read the fragment shader file
    const base::Optional<BufferSlice> fragmentShaderSlice = appendFileContentsToVector(fragmentShaderFilename, buffer);
    if (!fragmentShaderSlice.hasValue())
    {
        priv::err() << "Failed to open fragment shader file\n" << priv::PathDebugFormatter{fragmentShaderFilename};
        return base::nullOpt;
    }

    return loadFromMemory(vertexShaderSlice->toView(buffer), fragmentShaderSlice->toView(buffer));
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromFile(const Path& vertexShaderFilename,
                                            const Path& geometryShaderFilename,
                                            const Path& fragmentShaderFilename)
{
    // Prepare thread-local buffer
    base::TrivialVector<char>& buffer = getThreadLocalCharBuffer();
    buffer.clear();

    // Read the vertex shader file
    const base::Optional<BufferSlice> vertexShaderSlice = appendFileContentsToVector(vertexShaderFilename, buffer);
    if (!vertexShaderSlice.hasValue())
    {
        priv::err() << "Failed to open vertex shader file\n" << priv::PathDebugFormatter{vertexShaderFilename};
        return base::nullOpt;
    }

    // Read the geometry shader file
    const base::Optional<BufferSlice> geometryShaderSlice = appendFileContentsToVector(geometryShaderFilename, buffer);
    if (!geometryShaderSlice.hasValue())
    {
        priv::err() << "Failed to open geometry shader file\n" << priv::PathDebugFormatter{geometryShaderFilename};
        return base::nullOpt;
    }

    // Read the fragment shader file
    const base::Optional<BufferSlice> fragmentShaderSlice = appendFileContentsToVector(fragmentShaderFilename, buffer);
    if (!fragmentShaderSlice.hasValue())
    {
        priv::err() << "Failed to open fragment shader file\n" << priv::PathDebugFormatter{fragmentShaderFilename};
        return base::nullOpt;
    }

    return loadFromMemory(vertexShaderSlice->toView(buffer),
                          geometryShaderSlice->toView(buffer),
                          fragmentShaderSlice->toView(buffer));
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromMemory(base::StringView shader, Type type)
{
    if (type == Type::Vertex)
        return compile(shader, {}, {});

    if (type == Type::Geometry)
        return compile({}, shader, {});

    SFML_BASE_ASSERT(type == Type::Fragment);
    return compile({}, {}, shader);
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromMemory(base::StringView vertexShader, base::StringView fragmentShader)
{
    return compile(vertexShader, {}, fragmentShader);
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromMemory(base::StringView vertexShader,
                                              base::StringView geometryShader,
                                              base::StringView fragmentShader)
{
    return compile(vertexShader, geometryShader, fragmentShader);
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromStream(InputStream& stream, Type type)
{
    // Prepare thread-local buffer
    base::TrivialVector<char>& buffer = getThreadLocalCharBuffer();
    buffer.clear();

    // Read the shader code from the stream
    const base::Optional<BufferSlice> shaderSlice = appendStreamContentsToVector(stream, buffer);
    if (!shaderSlice.hasValue())
    {
        priv::err() << "Failed to read vertex shader from stream";
        return base::nullOpt;
    }

    return loadFromMemory(shaderSlice->toView(buffer), type);
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromStream(InputStream& vertexShaderStream, InputStream& fragmentShaderStream)
{
    // Prepare thread-local buffer
    base::TrivialVector<char>& buffer = getThreadLocalCharBuffer();
    buffer.clear();

    // Read the vertex shader code from the stream
    const base::Optional<BufferSlice> vertexShaderSlice = appendStreamContentsToVector(vertexShaderStream, buffer);
    if (!vertexShaderSlice.hasValue())
    {
        priv::err() << "Failed to read vertex shader from stream";
        return base::nullOpt;
    }

    // Read the fragment shader code from the stream
    const base::Optional<BufferSlice> fragmentShaderSlice = appendStreamContentsToVector(fragmentShaderStream, buffer);
    if (!fragmentShaderSlice.hasValue())
    {
        priv::err() << "Failed to read fragment shader from stream";
        return base::nullOpt;
    }

    return loadFromMemory(vertexShaderSlice->toView(buffer), fragmentShaderSlice->toView(buffer));
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::loadFromStream(InputStream& vertexShaderStream,
                                              InputStream& geometryShaderStream,
                                              InputStream& fragmentShaderStream)
{
    // Prepare thread-local buffer
    base::TrivialVector<char>& buffer = getThreadLocalCharBuffer();
    buffer.clear();

    // Read the vertex shader code from the stream
    const base::Optional<BufferSlice> vertexShaderSlice = appendStreamContentsToVector(vertexShaderStream, buffer);
    if (!vertexShaderSlice.hasValue())
    {
        priv::err() << "Failed to read vertex shader from stream";
        return base::nullOpt;
    }

    // Read the geometry shader code from the stream
    const base::Optional<BufferSlice> geometryShaderSlice = appendStreamContentsToVector(geometryShaderStream, buffer);
    if (!geometryShaderSlice.hasValue())
    {
        priv::err() << "Failed to read geometry shader from stream";
        return base::nullOpt;
    }

    // Read the fragment shader code from the stream
    const base::Optional<BufferSlice> fragmentShaderSlice = appendStreamContentsToVector(fragmentShaderStream, buffer);
    if (!fragmentShaderSlice.hasValue())
    {
        priv::err() << "Failed to read fragment shader from stream";
        return base::nullOpt;
    }

    return loadFromMemory(vertexShaderSlice->toView(buffer),
                          geometryShaderSlice->toView(buffer),
                          fragmentShaderSlice->toView(buffer));
}


////////////////////////////////////////////////////////////
base::Optional<Shader::UniformLocation> Shader::getUniformLocation(base::StringView uniformName) const
{
    // Check the cache
    if (const auto it = m_impl->uniforms.find(uniformName); it != m_impl->uniforms.end())
    {
        // Already in cache, return it
        return it->second == -1 ? base::nullOpt : base::makeOptional(UniformLocation{it->second});
    }

    // Use thread-local string buffer to get a null-terminated uniform name
    static thread_local std::string uniformNameBuffer;
    uniformNameBuffer.clear();
    uniformNameBuffer.assign(uniformName.data(), uniformName.size());

    // Not in cache, request the location from OpenGL
    const int location = glGetUniformLocation(castToGlHandle(m_impl->shaderProgram), uniformNameBuffer.c_str());
    m_impl->uniforms.try_emplace(uniformNameBuffer, location);

    return location == -1 ? base::nullOpt : base::makeOptional(UniformLocation{location});
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, float x) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform1f(location.m_value, x));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, Glsl::Vec2 v) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform2f(location.m_value, v.x, v.y));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Vec3& v) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform3f(location.m_value, v.x, v.y, v.z));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Vec4& v) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform4f(location.m_value, v.x, v.y, v.z, v.w));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, int x) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform1i(location.m_value, x));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, Glsl::Ivec2 v) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform2i(location.m_value, v.x, v.y));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Ivec3& v) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform3i(location.m_value, v.x, v.y, v.z));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Ivec4& v) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform4i(location.m_value, v.x, v.y, v.z, v.w));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, bool x) const
{
    setUniform(location, static_cast<int>(x));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, Glsl::Bvec2 v) const
{
    setUniform(location, v.to<Glsl::Ivec2>());
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Bvec3& v) const
{
    setUniform(location, v.to<Glsl::Ivec3>());
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Bvec4& v) const
{
    setUniform(location, Glsl::Ivec4(v));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Mat3& matrix) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniformMatrix3fv(location.m_value, 1, GL_FALSE, matrix.array));
}


////////////////////////////////////////////////////////////
void Shader::setMat4Uniform(UniformLocation location, const float* matrixPtr) const
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniformMatrix4fv(location.m_value, 1, GL_FALSE, matrixPtr));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Mat4& matrix) const
{
    setMat4Uniform(location, matrix.array);
}


////////////////////////////////////////////////////////////
bool Shader::setUniform(UniformLocation location, const Texture& texture) const
{
    SFML_BASE_ASSERT(m_impl->shaderProgram);
    SFML_BASE_ASSERT(GraphicsContext::hasActiveThreadLocalOrSharedGlContext());

    // Store the location -> texture mapping
    if (const auto it = m_impl->textures.find(location.m_value); it != m_impl->textures.end())
    {
        // Location already used, just replace the texture
        it->second = &texture;
        return true;
    }

    // New entry, make sure there are enough texture units
    if (m_impl->textures.size() + 1 >= getMaxTextureUnits())
    {
        priv::err() << "Impossible to use texture \"" << location.m_value << '"'
                    << " \"for shader: all available texture units are used";

        return false;
    }

    m_impl->textures[location.m_value] = &texture;
    return true;
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, CurrentTextureType)
{
    SFML_BASE_ASSERT(m_impl->shaderProgram);
    SFML_BASE_ASSERT(GraphicsContext::hasActiveThreadLocalOrSharedGlContext());

    // Find the location of the variable in the shader
    m_impl->currentTexture = location.m_value;
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const float* scalarArray, base::SizeT length)
{
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform1fv(location.m_value, static_cast<GLsizei>(length), scalarArray));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Vec2* vectorArray, base::SizeT length)
{
    base::TrivialVector<float> contiguous = flatten(vectorArray, length);
    const UniformBinder        binder{m_impl->shaderProgram};
    glCheck(glUniform2fv(location.m_value, static_cast<GLsizei>(length), contiguous.data()));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Vec3* vectorArray, base::SizeT length)
{
    base::TrivialVector<float> contiguous = flatten(vectorArray, length);
    const UniformBinder        binder{m_impl->shaderProgram};
    glCheck(glUniform3fv(location.m_value, static_cast<GLsizei>(length), contiguous.data()));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Vec4* vectorArray, base::SizeT length)
{
    base::TrivialVector<float> contiguous = flatten(vectorArray, length);
    const UniformBinder        binder{m_impl->shaderProgram};
    glCheck(glUniform4fv(location.m_value, static_cast<GLsizei>(length), contiguous.data()));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Mat3* matrixArray, base::SizeT length)
{
    const base::SizeT matrixSize = 3 * 3;

    base::TrivialVector<float> contiguous(matrixSize * length);

    for (base::SizeT i = 0; i < length; ++i)
        priv::copyMatrix(matrixArray[i].array, matrixSize, &contiguous[matrixSize * i]);

    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniformMatrix3fv(location.m_value, static_cast<GLsizei>(length), GL_FALSE, contiguous.data()));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Mat4* matrixArray, base::SizeT length)
{
    const base::SizeT matrixSize = 4 * 4;

    base::TrivialVector<float> contiguous(matrixSize * length);

    for (base::SizeT i = 0; i < length; ++i)
        priv::copyMatrix(matrixArray[i].array, matrixSize, &contiguous[matrixSize * i]);

    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniformMatrix4fv(location.m_value, static_cast<GLsizei>(length), GL_FALSE, contiguous.data()));
}


////////////////////////////////////////////////////////////
unsigned int Shader::getNativeHandle() const
{
    return m_impl->shaderProgram;
}


////////////////////////////////////////////////////////////
void Shader::bind() const
{
    SFML_BASE_ASSERT(GraphicsContext::hasActiveThreadLocalOrSharedGlContext());
    SFML_BASE_ASSERT(m_impl->shaderProgram != 0u);

    // Enable the program
    SFML_BASE_ASSERT(glCheck(glIsProgram(castToGlHandle(m_impl->shaderProgram))));
    glCheck(glUseProgram(castToGlHandle(m_impl->shaderProgram)));

    // Bind the textures
    bindTextures();

    // Bind the current texture
    if (m_impl->currentTexture != -1)
        glCheck(glUniform1i(m_impl->currentTexture, 0));
}


////////////////////////////////////////////////////////////
void Shader::unbind()
{
    SFML_BASE_ASSERT(GraphicsContext::hasActiveThreadLocalOrSharedGlContext());
    glCheck(glUseProgram({}));
}


////////////////////////////////////////////////////////////
bool Shader::isGeometryAvailable()
{
#ifdef SFML_OPENGL_ES
    return false;
#else
    SFML_BASE_ASSERT(GraphicsContext::hasActiveThreadLocalOrSharedGlContext());
    return GL_VERSION_3_2;
#endif
}


////////////////////////////////////////////////////////////
Shader::Shader(base::PassKey<Shader>&&, unsigned int shaderProgram) : m_impl(shaderProgram)
{
}


////////////////////////////////////////////////////////////
base::Optional<Shader> Shader::compile(base::StringView vertexShaderCode,
                                       base::StringView geometryShaderCode,
                                       base::StringView fragmentShaderCode)
{
    SFML_BASE_ASSERT(GraphicsContext::hasActiveThreadLocalOrSharedGlContext());

    // Make sure we can use geometry shaders
    if (geometryShaderCode.data() != nullptr && !isGeometryAvailable())
    {
        priv::err() << "Failed to create a shader: your system doesn't support geometry shaders "
                    << "(you should test Shader::isGeometryAvailable() before trying to use geometry shaders)";

        return base::nullOpt;
    }

    // Create the program
    const GLhandle shaderProgram = glCheck(glCreateProgram());
    SFML_BASE_ASSERT(glCheck(glIsProgram(shaderProgram)));

    const auto makeShader = [&](GLenum type, const char* typeStr, base::StringView shaderCode)
    {
        // Add `#version` (and float precision if required)
        const auto adjustedShaderCode = adjustPreamble(shaderCode);

        const GLhandle shader = glCheck(glCreateShader(type));

        const GLcharARB* sourceCode       = adjustedShaderCode.data();
        const auto       sourceCodeLength = static_cast<GLint>(adjustedShaderCode.size());

        glCheck(glShaderSource(shader, 1, &sourceCode, &sourceCodeLength));
        glCheck(glCompileShader(shader));
        SFML_BASE_ASSERT(glCheck(glIsShader(shader)));

        // Check the compile log
        GLint success = 0;
        glCheck(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
        if (success == GL_FALSE)
        {
            char log[1024]{};
            glCheck(glGetShaderInfoLog(shader, sizeof(log), nullptr, log));

            priv::err() << "Failed to compile " << typeStr << " shader:" << '\n'
                        << static_cast<const char*>(log) << "\n\nSource code:\n"
                        << adjustedShaderCode;

            glCheck(glDeleteShader(shader));
            glCheck(glDeleteProgram(shaderProgram));

            return false;
        }

        // Attach the shader to the program, and delete it (not needed anymore)
        glCheck(glAttachShader(shaderProgram, shader));
        glCheck(glDeleteShader(shader));

        return true;
    };

    if (vertexShaderCode.data() == nullptr)
        vertexShaderCode = GraphicsContext::getBuiltInShaderVertexSrc();

    if (!makeShader(GL_VERTEX_SHADER, "vertex", vertexShaderCode))
        return base::nullOpt;


    // Create the geometry shader if needed
    if (geometryShaderCode.data())
    {
        if (!makeShader(GL_GEOMETRY_SHADER, "geometry", geometryShaderCode))
            return base::nullOpt;
    }

    if (fragmentShaderCode.data() == nullptr)
        fragmentShaderCode = GraphicsContext::getBuiltInShaderFragmentSrc();

    // Create the fragment shader
    if (!makeShader(GL_FRAGMENT_SHADER, "fragment", fragmentShaderCode))
        return base::nullOpt;

    // Link the program
    glCheck(glLinkProgram(shaderProgram));

    // Check the link log
    GLint success = 0;
    glCheck(glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success));
    if (success == GL_FALSE)
    {
        char log[1024]{};
        glCheck(glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log));
        priv::err() << "Failed to link shader:" << '\n'
                    << static_cast<const char*>(log) << "VERTEX SOURCE:\n"
                    << vertexShaderCode << "\n\nFRAGMENT SOURCE:\n"
                    << fragmentShaderCode << "\n\nGEOMETRY SOURCE:\n"
                    << geometryShaderCode;
        glCheck(glDeleteProgram(shaderProgram));
        return base::nullOpt;
    }

    // Force an OpenGL flush, so that the shader will appear updated
    // in all contexts immediately (solves problems in multi-threaded apps)
    glCheck(glFlush());

    return base::makeOptional<Shader>(base::PassKey<Shader>{}, castFromGlHandle(shaderProgram));
}


////////////////////////////////////////////////////////////
void Shader::bindTextures() const
{
    auto it = m_impl->textures.begin();
    for (base::SizeT i = 0; i < m_impl->textures.size(); ++i)
    {
        const auto index = static_cast<GLsizei>(i + 1);
        glCheck(glUniform1i(it->first, index));
        glCheck(glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(index)));
        it->second->bind();
        ++it;
    }

    // Make sure that the texture unit which is left active is the number 0
    glCheck(glActiveTexture(GL_TEXTURE0));
}

} // namespace sf
