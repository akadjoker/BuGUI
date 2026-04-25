#include "Device.hpp"
#include "RenderState.hpp"
#include "Input.hpp"
#include "Opengl.hpp"
#include "msf_gif.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>

static double sGetTime() { return static_cast<double>(SDL_GetTicks()) / 1000.0; }

// ─── Singleton ────────────────────────────────────────────────────────────────

Device& Device::Instance()
{
    static Device instance;
    return instance;
}

Device::Device()
{
    SDL_Log("[Device] Initialized.");
}

Device::~Device()
{
    SDL_Log("[Device] Destroyed.");
    Close();
}

// ─── Create ───────────────────────────────────────────────────────────────────

bool Device::Create(int width, int height, const char* title, bool vsync, int monitor, unsigned int sdlExtraFlags)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return false;
    }

    m_width  = width;
    m_height = height;
    SetTargetFPS(vsync ? 0 : 1000);
    m_closekey = 256;

    m_current  = sGetTime();
    m_previous = m_current;

    // Choose monitor
    int numDisplays = SDL_GetNumVideoDisplays();
    SDL_Log("[Device] Displays: %d", numDisplays);
    for (int i = 0; i < numDisplays; ++i)
    {
        SDL_Rect b;
        SDL_GetDisplayBounds(i, &b);
        SDL_Log("[Device]   Display %d: %dx%d @ (%d,%d)", i, b.w, b.h, b.x, b.y);
    }

    int posX = SDL_WINDOWPOS_CENTERED;
    int posY = SDL_WINDOWPOS_CENTERED;
    if (monitor >= 0 && monitor < numDisplays)
    {
        SDL_Rect bounds;
        SDL_GetDisplayBounds(monitor, &bounds);
        posX = bounds.x + (bounds.w - width)  / 2;
        posY = bounds.y + (bounds.h - height) / 2;
        SDL_Log("[Device] Using display %d", monitor);
    }

    // OpenGL core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | sdlExtraFlags;
    m_window = SDL_CreateWindow(
        title,
        posX, posY,
        width, height,
        winFlags);

    if (!m_window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[Device] Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[Device] GL context creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        SDL_Quit();
        return false;
    }

    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
    m_vsyncEnabled = vsync;

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[Device] Failed to load GL with glad");
        SDL_GL_DeleteContext(m_context);
        SDL_DestroyWindow(m_window);
        m_context = nullptr;
        m_window = nullptr;
        SDL_Quit();
        return false;
    }
#endif

    SDL_Log("[Device] Vendor  : %s", (const char*)glGetString(GL_VENDOR));
    SDL_Log("[Device] Renderer: %s", (const char*)glGetString(GL_RENDERER));
    SDL_Log("[Device] Version : %s", (const char*)glGetString(GL_VERSION));

    RenderState::instance();

    m_ready = true;
    return true;
}

// ─── Run (event pump) ────────────────────────────────────────────────────────

bool Device::Run()
{
    if (!m_ready)
        return false;

    m_current  = sGetTime();
    m_update   = m_current - m_previous;
    m_previous = m_current;
    m_is_resize = false;

    SDL_Event event;
    Input::Update();

    while (SDL_PollEvent(&event) != 0)
    {
        switch (event.type)
        {
        case SDL_QUIT:
            m_shouldclose = true;
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                m_width  = event.window.data1;
                m_height = event.window.data2;
                m_is_resize = true;
            }
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                SetShouldClose(true);
                break;
            }
            Input::OnKeyDown(event.key);
            break;

        case SDL_KEYUP:
            Input::OnKeyUp(event.key);
            break;

        case SDL_TEXTINPUT:
            Input::OnTextInput(event.text);
            break;

        case SDL_MOUSEBUTTONDOWN:
            Input::OnMouseDown(event.button);
            break;

        case SDL_MOUSEBUTTONUP:
            Input::OnMouseUp(event.button);
            break;

        case SDL_MOUSEMOTION:
            Input::OnMouseMove(event.motion);
            break;

        case SDL_MOUSEWHEEL:
            Input::OnMouseWheel(event.wheel);
            break;
        }
    }

    return !m_shouldclose;
}

// ─── Close ───────────────────────────────────────────────────────────────────

void Device::Close()
{
    if (!m_ready)
        return;

    // Finalize GIF if still recording
    if (m_gifRecording)
        StopGifRecording();

    m_ready = false;
    RenderState::instance().shutdown();

    if (m_context) SDL_GL_DeleteContext(m_context);
    if (m_window)  SDL_DestroyWindow(m_window);

    m_context = nullptr;
    m_window  = nullptr;

    SDL_Log("[Device] Closed.");
    SDL_Quit();
}

// ─── Flip ────────────────────────────────────────────────────────────────────

void Device::Flip()
{
    // Capture GIF frame BEFORE swap (framebuffer still has current image)
    if (m_gifRecording)
        GifCaptureFrame();

    SDL_GL_SwapWindow(m_window);

    m_current  = sGetTime();
    m_draw     = m_current - m_previous;
    m_previous = m_current;
    m_frame    = m_update + m_draw;

    if (!m_vsyncEnabled && m_target > 0.0 && m_frame < m_target)
    {
        Wait(static_cast<float>(m_target - m_frame) * 1000.0f);
        m_current  = sGetTime();
        double wait = m_current - m_previous;
        m_previous = m_current;
        m_frame += wait;
    }

    m_is_resize = false;
}

// ─── Timing ──────────────────────────────────────────────────────────────────

void Device::Wait(float ms) { SDL_Delay(static_cast<Uint32>(ms)); }

void Device::SetTargetFPS(int fps)
{
    m_target = (fps < 1) ? 0.0 : (1.0 / static_cast<double>(fps));
}

int Device::GetFPS()
{
    static int index = 0;
    static float history[30] = {};
    static float average = 0;
    static float last = 0;
    float ft = GetFrameTime();
    if (ft == 0.0f) return 0;
    if ((sGetTime() - last) > (0.5f / 30.0f))
    {
        last = static_cast<float>(sGetTime());
        index = (index + 1) % 30;
        average -= history[index];
        history[index] = ft / 30.0f;
        average += history[index];
    }
    return static_cast<int>(roundf(1.0f / average));
}

float  Device::GetFrameTime() const { return static_cast<float>(m_frame); }
double Device::GetTime()      const { return static_cast<double>(SDL_GetTicks()) / 1000.0; }
u32    Device::GetTicks()     const { return SDL_GetTicks(); }
bool   Device::IsRunning()    const { return m_ready && !m_shouldclose; }

// ═════════════════════════════════════════════════════════════════════════════
//  GIF Recording
// ═════════════════════════════════════════════════════════════════════════════

bool Device::StartGifRecording(const char* filename)
{
    if (m_gifRecording) return false;

    // Auto-generate filename with timestamp
    if (!filename)
    {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char buf[64];
        snprintf(buf, sizeof(buf), "bugui_%04d%02d%02d_%02d%02d%02d.gif",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);
        m_gifFilename = buf;
    }
    else
    {
        m_gifFilename = filename;
    }

    m_gifState = new MsfGifState{};
    if (!msf_gif_begin(m_gifState, m_width, m_height))
    {
        delete m_gifState;
        m_gifState = nullptr;
        return false;
    }

    m_gifPixels = new unsigned char[m_width * m_height * 4];
    m_gifRecording = true;
    m_gifElapsed   = 0.0f;
    m_gifFrames    = 0;
    m_gifAccum     = 0.0f;

    printf("INFO: [GIF] Recording started → %s (%dx%d)\n",
           m_gifFilename.c_str(), m_width, m_height);
    return true;
}

bool Device::StopGifRecording()
{
    if (!m_gifRecording || !m_gifState) return false;

    m_gifRecording = false;

    MsfGifResult result = msf_gif_end(m_gifState);
    delete m_gifState;
    m_gifState = nullptr;

    delete[] m_gifPixels;
    m_gifPixels = nullptr;

    if (!result.data)
    {
        printf("ERROR: [GIF] Encoding failed!\n");
        return false;
    }

    FILE* fp = fopen(m_gifFilename.c_str(), "wb");
    if (!fp)
    {
        printf("ERROR: [GIF] Cannot open %s for writing\n", m_gifFilename.c_str());
        msf_gif_free(result);
        return false;
    }

    fwrite(result.data, result.dataSize, 1, fp);
    fclose(fp);
    msf_gif_free(result);

    printf("INFO: [GIF] Saved %s (%d frames, %.1fs, %.1f KB)\n",
           m_gifFilename.c_str(), m_gifFrames, m_gifElapsed,
           static_cast<float>(result.dataSize) / 1024.0f);
    return true;
}

void Device::GifCaptureFrame()
{
    if (!m_gifState || !m_gifPixels) return;

    // Target ~15 fps for GIF (every ~67ms)
    static constexpr float kGifInterval = 1.0f / 15.0f;
    float dt = GetFrameTime();
    m_gifElapsed += dt;
    m_gifAccum   += dt;

    if (m_gifAccum < kGifInterval)
        return;

    int centiseconds = static_cast<int>(m_gifAccum * 100.0f);
    if (centiseconds < 1) centiseconds = 1;
    m_gifAccum = 0.0f;

    // Read framebuffer (GL reads bottom-up, need to flip)
    glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, m_gifPixels);

    // Flip vertically (GL origin is bottom-left, GIF is top-left)
    int stride = m_width * 4;
    unsigned char* row = new unsigned char[stride];
    for (int y = 0; y < m_height / 2; y++)
    {
        unsigned char* top = m_gifPixels + y * stride;
        unsigned char* bot = m_gifPixels + (m_height - 1 - y) * stride;
        memcpy(row, top, stride);
        memcpy(top, bot, stride);
        memcpy(bot, row, stride);
    }
    delete[] row;

    msf_gif_frame(m_gifState, m_gifPixels, centiseconds, 16, stride);
    m_gifFrames++;
}
