#pragma once
#include "engine/3dmath.h"
#include "arcball_camera.h"

struct UserCamera
{
  glm::mat4 transform;
  mat4x4 projection;
  ArcballCamera arcballCamera;
};
