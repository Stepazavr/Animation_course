#include "scene.h"
#include "engine/render/shader.h"
#include "engine/render/material.h"

bool g_visualizeBoneWeights = false;
bool g_visualizeSkeleton = false;

void update_bone_matrices(std::vector<glm::mat4>& bone_matrices, const Skeleton& skeleton, const Bone& bone, const glm::mat4& parent_transform)
{
	glm::mat4 global_transform = parent_transform * bone.local_transform;
	
	int bone_idx = &bone - &skeleton.bones[0];
	bone_matrices[bone_idx] = global_transform * bone.offset_matrix;

	for (int child_idx : bone.children_indices)
	{
		update_bone_matrices(bone_matrices, skeleton, skeleton.bones[child_idx], global_transform);
	}
}

void render_skeleton(const Character& character, const mat4& cameraProjView)
{
	static ShaderPtr line_shader = compile_shader("line", "sources/shaders/line_vs.glsl", "sources/shaders/line_ps.glsl");
	if (!line_shader) return;
	line_shader->use();
	line_shader->set_mat4x4("view_projection", cameraProjView);
	line_shader->set_vec3("color", glm::vec3(1.f, 1.f, 0.f));

	glDisable(GL_DEPTH_TEST);

	std::vector<glm::vec3> points;
	for (const auto& bone : character.skeleton.bones)
	{
		if (bone.parent_idx != -1)
		{
			glm::mat4 parent_transform = glm::mat4(1.0f);
			int parent_idx = bone.parent_idx;
			while (parent_idx != -1)
			{
				parent_transform = character.skeleton.bones[parent_idx].local_transform * parent_transform;
				parent_idx = character.skeleton.bones[parent_idx].parent_idx;
			}

			glm::mat4 bone_transform = parent_transform * bone.local_transform;

			points.push_back(glm::vec3(parent_transform[3]));
			points.push_back(glm::vec3(bone_transform[3]));
		}
	}

	if (points.empty())
		return;

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec3), points.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	line_shader->set_mat4x4("model", character.transform);
	glDrawArrays(GL_LINES, 0, points.size());

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glEnable(GL_DEPTH_TEST);
}


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

  for (Character &character : scene.characters)
    render_character(character, projView, glm::vec3(transform[3]), scene.light);

  if (g_visualizeSkeleton)
  {
	  for (const Character& character : scene.characters)
		  render_skeleton(character, projView);
  }
}
