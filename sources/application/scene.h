#pragma once

#include "engine/render/direction_light.h"
#include "engine/import/model.h"
#include "user_camera.h"
#include "third_person_controller.h"
#include "character.h"

struct Scene
{
  std::vector<ModelAsset> models;
  DirectionLight light;

  UserCamera userCamera;
  ThirdPersonController thirdPersonController;
  
  // Camera mode: true = third person, false = arcball
  bool use_third_person_camera = false;

  std::vector<Character> characters;
  
  // Animation mode: true = t-pose, false = dynamic (keyboard controlled)
  bool use_t_pose = false;
  
  // Character rotation based on WASD input (only in third person mode)
  float characterRotationY = 0.f;  // Current Y rotation in degrees
};

extern bool g_visualizeBoneWeights;
extern bool g_visualizeSkeleton;
extern bool g_visualizeNodeTransforms;
extern bool g_samplingEnabled;