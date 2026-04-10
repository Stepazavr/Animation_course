#pragma once
#include "engine/3dmath.h"

struct ThirdPersonController
{
  glm::vec3 targetPosition = glm::vec3(0.f);  // Position of the character (focus point)
  glm::vec3 cameraOffset = glm::vec3(0.f);  // Relative offset from target
  
  float distance = 4.f;  // Distance behind the character
  float height = -4.5f;    // Height offset from character center
  
  float yaw = 0.f;        // Rotation around Y axis (character facing direction)
  float pitch = 50.f;    // Rotation around X axis (look down angle)
  
  float lerpSpeed = 5.f;  // Lerp speed for smooth camera follow
  float mouseSensitivity = 0.5f;
  
  glm::vec3 currentPosition = glm::vec3(0.f);
  glm::vec3 currentLookAt = glm::vec3(0.f);
  
  bool rotationEnable = false;
};

#include "engine/api.h"

void third_person_controller_mouse_click_handler(
  const SDL_MouseButtonEvent& e,
  ThirdPersonController& controller);

void third_person_controller_mouse_move_handler(
  const SDL_MouseMotionEvent& e,
  ThirdPersonController& controller);

void third_person_controller_mouse_wheel_handler(
  const SDL_MouseWheelEvent& e,
  ThirdPersonController& controller);
