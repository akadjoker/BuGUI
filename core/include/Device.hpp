#pragma once

#include "Config.hpp"
#include "msf_gif.h"
#include <SDL2/SDL.h>
#include <string>

class Device final
{
public:
    bool Create(int width, int height, const char* title, bool vsync = false,
                int monitor = -1, unsigned int sdlExtraFlags = 0);
    bool Run();
    void Close();
    void Flip();

    int  GetWidth()  const { return m_width; }
    int  GetHeight() const { return m_height; }
    void SetSize(int w, int h) { m_width = w; m_height = h; }

    void Wait(float ms);
    void SetTargetFPS(int fps);
    int  GetFPS();
    float GetFrameTime() const;
    double GetTime() const;
    u32  GetTicks() const;

    void SetShouldClose(bool close) { m_shouldclose = close; }
    bool ShouldClose() const { return m_shouldclose; }
    bool IsReady() const { return m_ready && !m_shouldclose; }
    bool IsResize() const { return m_is_resize; }
    bool IsRunning() const;

    SDL_Window*   GetWindow()    const { return m_window; }
    SDL_GLContext  GetGLContext() const { return m_context; }

    // ── GIF recording ─────────────────────────────────────────────────
    bool StartGifRecording(const char* filename = nullptr); // null = auto timestamped
    bool StopGifRecording();                                // finalize + save
    bool IsRecording() const { return m_gifRecording; }
    float GifElapsed() const { return m_gifElapsed; }
    int   GifFrameCount() const { return m_gifFrames; }

    static Device& Instance();

private:
    int m_width  = 0;
    int m_height = 0;
    bool m_shouldclose = false;
    bool m_is_resize   = false;
    bool m_ready       = false;
    bool m_vsyncEnabled = false;
    Sint32 m_closekey  = 256; // SDLK_ESCAPE

    SDL_Window*   m_window  = nullptr;
    SDL_GLContext  m_context = nullptr;

    double m_current  = 0;
    double m_previous = 0;
    double m_update   = 0;
    double m_draw     = 0;
    double m_frame    = 0;
    double m_target   = 0;

    Device();
    ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // GIF recording
    void GifCaptureFrame();

    MsfGifState* m_gifState     = nullptr;
    std::string  m_gifFilename;
    bool         m_gifRecording = false;
    float        m_gifElapsed   = 0.0f;
    int          m_gifFrames    = 0;
    float        m_gifAccum     = 0.0f;   // time accumulator for frame pacing
    unsigned char* m_gifPixels  = nullptr; // reusable pixel buffer
};
