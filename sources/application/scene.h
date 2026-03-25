#pragma once

#include "engine/render/direction_light.h"
#include "engine/import/model.h"
#include "user_camera.h"
#include "character.h"

struct Scene
{
  std::vector<ModelAsset> models;
  DirectionLight light;

  UserCamera userCamera;

  std::vector<Character> characters;
};

// Глобальный флаг для режима визуализации весов костей
extern bool g_visualizeBoneWeights;