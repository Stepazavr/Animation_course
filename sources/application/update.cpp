#include "scene.h"

void application_update(Scene &scene)
{
  arcball_camera_update(
    scene.userCamera.arcballCamera,
    scene.userCamera.transform,
    engine::get_delta_time());
}