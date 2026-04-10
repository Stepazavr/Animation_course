#include "third_person_controller.h"
#include "engine/api.h"

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
