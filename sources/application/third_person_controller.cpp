#include "third_person_controller.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void third_person_camera_update(
  ThirdPersonController& controller,
  glm::mat4& transform,
  const glm::vec3& characterPosition,
  float characterRotationY,
  float dt)
{
  // Get model's global position
  glm::vec3 modelPosition = characterPosition;
  
  // Fixed camera offset from model
  glm::vec3 cameraOffset = glm::vec3(0.f, controller.height, controller.distance);
  
  // Calculate target camera position
  controller.targetPosition = modelPosition + cameraOffset;
  
  // Smoothly interpolate camera to target position
  float lerpFactor = glm::min(1.f, controller.lerpSpeed * dt);
  controller.currentPosition = glm::mix(controller.currentPosition, controller.targetPosition, lerpFactor);
  
  // Calculate look-at target (slightly above character's center)
  glm::vec3 lookAtTarget = modelPosition + glm::vec3(0.f, controller.height + 0.2f, 0.f);
  
  // Create view matrix (camera looking at character)
  glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);
  glm::mat4 viewMatrix = glm::lookAt(controller.currentPosition, lookAtTarget, up);
  
  // Return the inverse of view matrix (camera's transform)
  transform = glm::inverse(viewMatrix);
}


