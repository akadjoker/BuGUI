#include "Texture.hpp"
#include "Pixmap.hpp"
#include "Opengl.hpp"
#include <stb_image.h>

// ─── Helpers ──────────────────────────────────────────────────────────────────

static GLenum glFormatFromComponents(int c)
{
    switch (c)
    {
    case 1: return GL_RED;
    case 2: return GL_RG;
    case 3: return GL_RGB;
    case 4: default: return GL_RGBA;
    }
}

static GLenum glInternalFormatFromComponents(int c)
{
    switch (c)
    {
    case 1: return GL_R8;
    case 2: return GL_RG8;
    case 3: return GL_RGB8;
    case 4: default: return GL_RGBA8;
    }
}

static Texture* uploadPixels(const char* name, int width, int height,
                             int components, const void* pixels)
{
    if (width <= 0 || height <= 0 || !pixels)
        return nullptr;

    Texture* t = new Texture();
    t->width  = width;
    t->height = height;
    t->name   = name ? name : "";

    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);

    GLint prevAlign = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlign);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0,
                 glInternalFormatFromComponents(components),
                 width, height, 0,
                 glFormatFromComponents(components),
                 GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlign);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Swizzle grayscale so RGB all read from RED channel
    if (components == 1 || components == 2)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A,
                        components == 2 ? GL_GREEN : GL_ONE);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

// ─── Public API ───────────────────────────────────────────────────────────────

Texture* CreateTextureFromPixmap(const char* name, const Pixmap& pixmap)
{
    if (!pixmap.IsValid() || pixmap.components < 1 || pixmap.components > 4)
        return nullptr;
    return uploadPixels(name, pixmap.width, pixmap.height,
                        pixmap.components, pixmap.pixels);
}

Texture* CreateTextureFromRGBA(const char* name, int width, int height, const void* pixels)
{
    return uploadPixels(name, width, height, 4, pixels);
}

Texture* LoadTextureFromFile(const char* name, const char* path, bool flipVertical)
{
    stbi_set_flip_vertically_on_load(flipVertical ? 1 : 0);
    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = stbi_load(path, &w, &h, &ch, 0);
    stbi_set_flip_vertically_on_load(0);

    if (!pixels)
    {
        SDL_Log("[Texture] Failed to load '%s': %s", path, stbi_failure_reason());
        return nullptr;
    }

    Texture* t = uploadPixels(name, w, h, ch, pixels);
    stbi_image_free(pixels);

    if (t)
        t->sourcePath = path;

    return t;
}
