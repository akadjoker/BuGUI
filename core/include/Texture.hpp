#pragma once
#include "Types.hpp"
#include "Opengl.hpp"
#include <SDL2/SDL.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

// ═════════════════════════════════════════════════════════════════════════════
//  Texture — lightweight GPU texture handle
// ═════════════════════════════════════════════════════════════════════════════

struct Texture
{
    std::string name;
    std::string sourcePath;
    GLuint  id     = 0;
    int     width  = 0;
    int     height = 0;
    GLenum  target = GL_TEXTURE_2D;
    PixelType type = PixelType::RGBA;

    void release()
    {
        if (id) { glDeleteTextures(1, &id); id = 0; }
    }

    ~Texture() { release(); }

    Texture()                            = default;
    Texture(const Texture &)             = delete;
    Texture &operator=(const Texture &)  = delete;
    Texture(Texture &&)                  = default;
    Texture &operator=(Texture &&)       = default;
};

class Pixmap; // forward

// Helper: create a GL texture from a Pixmap (components 1-4)
Texture* CreateTextureFromPixmap(const char* name, const Pixmap& pixmap);

// Helper: create a GL texture from raw RGBA pixels
Texture* CreateTextureFromRGBA(const char* name, int width, int height, const void* pixels);

// Helper: load a texture from an image file (stbi)
Texture* LoadTextureFromFile(const char* name, const char* path, bool flipVertical = false);

// ═════════════════════════════════════════════════════════════════════════════
//  Shader — GLSL program wrapper
// ═════════════════════════════════════════════════════════════════════════════

class Shader
{
public:
    Shader(const std::string& n) : name(n) {}

    bool load(const std::string& vertPath, const std::string& fragPath);
    bool loadFromSource(const std::string& vertSrc, const std::string& fragSrc);

    void setInt  (const std::string& u, int v)               const;
    void setFloat(const std::string& u, float v)             const;
    void setVec2 (const std::string& u, const glm::vec2& v)  const;
    void setVec3 (const std::string& u, const glm::vec3& v)  const;
    void setVec4 (const std::string& u, const glm::vec4& v)  const;
    void setMat3 (const std::string& u, const glm::mat3& v)  const;
    void setMat4 (const std::string& u, const glm::mat4& v)  const;

    GLuint   getId()      const { return id; }
    uint32_t getAttribs() const { return attribs; }

    ~Shader();

private:
    std::string name;
    uint32_t    attribs = 0;
    GLuint      id      = 0;

    mutable std::unordered_map<std::string, GLint> uniformCache;

    static bool readFile(const std::string& path, std::string& out);
    GLuint      compileSource(const std::string& src, GLenum type) const;
    void        resolveAttribs();
    GLint       getLoc(const std::string& u) const;
};
