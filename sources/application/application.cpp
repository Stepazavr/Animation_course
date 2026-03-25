
#include "scene.h"

void application_init(Scene &scene);
void application_update(Scene &scene);
void application_render(Scene &scene);
void application_imgui_render(Scene &scene);

static std::unique_ptr<Scene> scene;

// entry points for engine/main.cpp
void game_init()
{
  scene = std::make_unique<Scene>();
  application_init(*scene);
}

void game_update()
{
  application_update(*scene);
}

void game_render()
{
  application_render(*scene);
}

void game_imgui_render()
{
  application_imgui_render(*scene);
}

void game_terminate()
{
  scene.reset();
}