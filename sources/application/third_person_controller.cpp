#include "third_person_controller.h"
#include "engine/api.h"

glm::mat4 third_person_controller_calculate_transform(const ThirdPersonController& controller)
{
  // Calculate camera position based on yaw, pitch, and distance
  float yawRad = glm::radians(controller.yaw);
  float pitchRad = glm::radians(controller.pitch);
  
  // Position offset relative to target
  glm::vec3 offset;
  offset.x = controller.distance * sinf(yawRad) * cosf(pitchRad);
  offset.y = controller.height + controller.distance * sinf(pitchRad);
  offset.z = controller.distance * cosf(yawRad) * cosf(pitchRad);
  
  glm::vec3 cameraPos = controller.targetPosition + offset;
  
  // Look at character (slightly above center)
  glm::vec3 lookAt = controller.targetPosition + glm::vec3(0.f, controller.height * 0.5f, 0.f);
  
  // Up vector
  glm::vec3 upVector = glm::vec3(0.f, 1.f, 0.f);
  
  return glm::lookAt(cameraPos, lookAt, upVector);
}

void third_person_controller_update(
  ThirdPersonController& controller,
  glm::mat4& transform,
  float dt)
{
  // Smooth interpolation of position
  float lerpFactor = controller.lerpSpeed * dt;
  
  glm::vec3 desiredPosition = controller.targetPosition;
  controller.currentPosition = glm::mix(controller.currentPosition, desiredPosition, lerpFactor);
  
  // Update transform
  transform = third_person_controller_calculate_transform(controller);
}

void third_person_controller_mouse_move_handler(
  const SDL_MouseMotionEvent& e,
  ThirdPersonController& controller)
{
  if (controller.rotationEnable)
  {
    float pixToRad = DegToRad * controller.mouseSensitivity;
    controller.yaw -= e.xrel * pixToRad;
    controller.pitch += e.yrel * pixToRad;
    
    // Clamp pitch to avoid gimbal lock
    controller.pitch = glm::clamp(controller.pitch, -89.f, 89.f);
  }
}

void third_person_controller_mouse_click_handler(
  const SDL_MouseButtonEvent& e,
  ThirdPersonController& controller)
{
  if (e.button == SDL_BUTTON_RIGHT)
  {
    controller.rotationEnable = e.type == SDL_MOUSEBUTTONDOWN;
  }
}

void third_person_controller_mouse_wheel_handler(
  const SDL_MouseWheelEvent& e,
  ThirdPersonController& controller)
{
  controller.distance = glm::clamp(controller.distance - float(e.y) * 0.5f, 1.f, 10.f);
}
