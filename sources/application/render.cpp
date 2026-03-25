
#include "scene.h"

void render_character(const Character &character, const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light)
{
  const Material &material = *character.material;
  const Shader &shader = material.get_shader();

  shader.use();
  material.bind_uniforms_to_shader();
  shader.set_mat4x4("Transform", character.transform);
  shader.set_mat4x4("ViewProjection", cameraProjView);
  shader.set_vec3("CameraPosition", cameraPosition);
  shader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
  shader.set_vec3("AmbientLight", light.ambient);
  shader.set_vec3("SunLight", light.lightColor);

  for (const MeshPtr &mesh : character.meshes)
    render(mesh);
}

void application_render(Scene &scene)
{
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  const float grayColor = 0.3f;
  glClearColor(grayColor, grayColor, grayColor, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  const mat4 &projection = scene.userCamera.projection;
  const glm::mat4 &transform = scene.userCamera.transform;
  mat4 projView = projection * inverse(transform);

  for (const Character &character : scene.characters)
    render_character(character, projView, glm::vec3(transform[3]), scene.light);
}
