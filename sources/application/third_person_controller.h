#pragma once
#include "engine/3dmath.h"
#include "engine/api.h"

struct ThirdPersonController
{
  // Camera positioning
  float distance = 2.f;      // Distance from character (behind)
  float height = 1.7f;       // Height above character's origin
  float pitch = 30.f;        // Initial pitch angle in degrees (rotation around X axis)
  
  // Smoothing
  float lerpSpeed = 5.f;     // Interpolation speed for smooth camera movement
  
  // Internal state for smooth interpolation
  glm::vec3 targetPosition = glm::vec3(0.f);
  glm::vec3 currentPosition = glm::vec3(0.f);
};

// Update third person camera - only position follows character
void third_person_camera_update(
  ThirdPersonController& controller,
  glm::mat4& transform,
  const glm::vec3& characterPosition,
  float characterRotationY,
  float dt);

