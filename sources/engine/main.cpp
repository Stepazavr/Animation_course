#include <glad/glad.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>
#include <map>
#include "engine/event.h"
#include "engine/log_history.h"

// forward declarations for game's entry points
extern void game_init();
extern void game_update();
extern void game_render();
extern void game_imgui_render();
extern void game_terminate();


typedef void *SDL_GLContext;

struct SDLContext
{
  SDL_Window *window = nullptr;
  SDL_GLContext gl_context = nullptr;
};

SDLContext context;

static void init_application()
{
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  // enable msaa antialiasing
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  const char *PROJECT_NAME = "animations";
  int window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;
  context.window = SDL_CreateWindow(PROJECT_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, (SDL_WindowFlags)(window_flags));
  context.gl_context = SDL_GL_CreateContext(context.window);
  SDL_GL_MakeCurrent(context.window, context.gl_context);
  SDL_GL_SetSwapInterval(0);

  if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
  {
    throw std::runtime_error{"Glad error"};
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(context.window, context.gl_context);
  const char *glsl_version = "#version 450";
  ImGui_ImplOpenGL3_Init(glsl_version);
  glEnable(GL_DEBUG_OUTPUT);
  // enable msaa antialiasing
  glEnable(GL_MULTISAMPLE);
}

static void close_application()
{
  game_terminate();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_Quit();
}

namespace engine
{
  Event<std::pair<int, int>> onWindowResizedEvent;
  float get_aspect_ratio()
  {
    int width, height;
    SDL_GL_GetDrawableSize(context.window, &width, &height);
    return (float)width / height;
  }

  std::pair<int, int> get_screen_size()
  {
    int width, height;
    SDL_GL_GetDrawableSize(context.window, &width, &height);
    return {width, height};
  }

  Event<SDL_KeyboardEvent> onKeyboardEvent;
  Event<SDL_MouseButtonEvent> onMouseButtonEvent;
  Event<SDL_MouseMotionEvent> onMouseMotionEvent;
  Event<SDL_MouseWheelEvent> onMouseWheelEvent;

  std::map<SDL_Keycode, bool> keyMap;
  float get_key(SDL_Keycode keycode)
  {
    auto it = keyMap.find(keycode);
    return (it != keyMap.end() ? it->second : false) ? 1.f : 0.f;
  }
  extern void start_time();
  extern void update_time();

}

static bool sdl_event_handler()
{
  SDL_Event event;
  bool running = true;
  while (SDL_PollEvent(&event))
  {
    ImGui_ImplSDL2_ProcessEvent(&event);
    switch (event.type)
    {
    case SDL_QUIT:
      running = false;
      break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
      if (ImGui::GetIO().WantCaptureKeyboard)
        break;
      if (!event.key.repeat)
      {
        if (event.key.state == SDL_PRESSED)
          engine::keyMap[event.key.keysym.sym] = true;
        if (event.key.state == SDL_RELEASED)
          engine::keyMap[event.key.keysym.sym] = false;
      }
      engine::onKeyboardEvent(event.key);

      if (event.key.keysym.sym == SDLK_ESCAPE)
        running = false;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      if (ImGui::GetIO().WantCaptureMouse)
        break;
      engine::onMouseButtonEvent(event.button);
      break;

    case SDL_MOUSEMOTION:
      if (ImGui::GetIO().WantCaptureMouse)
        break;
      engine::onMouseMotionEvent(event.motion);
      break;

    case SDL_MOUSEWHEEL:
      if (ImGui::GetIO().WantCaptureMouse)
        break;
      engine::onMouseWheelEvent(event.wheel);
      break;

    case SDL_WINDOWEVENT:
      break;
    }
  }
  return running;
}

void main_loop()
{
  engine::start_time();
  game_init();

  static std::pair<int, int> lastWindowSize = engine::get_screen_size();

  bool running = true;
  while (running)
  {
    engine::update_time();

    running = sdl_event_handler();

    std::pair<int, int> windowSize = engine::get_screen_size();
    if (windowSize != lastWindowSize)
    {
      lastWindowSize = windowSize;
      engine::onWindowResizedEvent(windowSize);
    }

    if (running)
    {
      game_update();
      SDL_GL_SwapWindow(context.window);

      game_render();

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame(context.window);
      ImGui::NewFrame();
      {
        if (ImGui::Begin("Log History"))
        {
          std::unique_lock read_write_lock(logMutex);
          for (const LogItem &m : logHistory)
            ImGui::TextColored(m.LogType == LogType::Log ? ImVec4(1, 1, 1, 1) : ImVec4(1, 0.1f, 0.1f, 1), "%s", m.message.c_str());
        }
        ImGui::End();

        game_imgui_render();
      }

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
  }
}

int main(int, char **)
{
  init_application();

  main_loop();

  close_application();

  return 0;
}