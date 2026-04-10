
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
  if (scene)
  {
    for (auto& character : scene->characters)
    {
      if (character.ozz_skeleton)
      {
        ozz::Delete(character.ozz_skeleton);
        character.ozz_skeleton = nullptr;
      }
      // Clean up all animation states
      for (auto& anim_state : character.animation_states)
      {
        if (anim_state.animation)
        {
          ozz::Delete(anim_state.animation);
          anim_state.animation = nullptr;
        }
        if (anim_state.sampling_context)
        {
          ozz::Delete(anim_state.sampling_context);
          anim_state.sampling_context = nullptr;
        }
      }
      character.animation_states.clear();
    }
  }
  scene.reset();
}