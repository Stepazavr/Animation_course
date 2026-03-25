#include "scene.h"
#include "engine/render/shader.h"
#include "engine/render/material.h"

bool g_visualizeBoneWeights = false;
bool g_visualizeSkeleton = false;
bool g_visualizeNodeTransforms = false;

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

void render_skeleton_transforms(const Character& character, const mat4& cameraProjView)
{
	static ShaderPtr shader = compile_shader("colored_line", "sources/shaders/colored_line_vs.glsl", "sources/shaders/colored_line_ps.glsl");
	if (!shader) return;
	shader->use();
	shader->set_mat4x4("view_projection", cameraProjView);

	glDisable(GL_DEPTH_TEST);

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
	};
	std::vector<Vertex> points;

	for (const auto& bone : character.skeleton.bones)
	{
		glm::mat4 global_transform = glm::mat4(1.0f);
		int current_idx = &bone - &character.skeleton.bones[0];
		while (current_idx != -1)
		{
			global_transform = character.skeleton.bones[current_idx].local_transform * global_transform;
			current_idx = character.skeleton.bones[current_idx].parent_idx;
		}

		glm::vec3 origin = glm::vec3(global_transform[3]);
		float axis_length = 0.1f;
		glm::vec3 x_axis = glm::normalize(glm::vec3(global_transform[0])) * axis_length;
		glm::vec3 y_axis = glm::normalize(glm::vec3(global_transform[1])) * axis_length;
		glm::vec3 z_axis = glm::normalize(glm::vec3(global_transform[2])) * axis_length;

		points.push_back({ origin, glm::vec3(1, 0, 0) });
		points.push_back({ origin + x_axis, glm::vec3(1, 0, 0) });
		points.push_back({ origin, glm::vec3(0, 1, 0) });
		points.push_back({ origin + y_axis, glm::vec3(0, 1, 0) });
		points.push_back({ origin, glm::vec3(0, 0, 1) });
		points.push_back({ origin + z_axis, glm::vec3(0, 0, 1) });
	}

	if (points.empty())
		return;

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Vertex), points.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

	shader->set_mat4x4("model", character.transform);
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
  shader.set_int("visualizeBoneWeights", g_visualizeBoneWeights ? 1 : 0);

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

  auto get_view_projection = [&]()
  {
    return scene.userCamera.projection * inverse(scene.userCamera.transform);
  };

  for (const Character &character : scene.characters)
  {
    render_character(character, get_view_projection(), scene.userCamera.transform[3], scene.light);
	if (g_visualizeSkeleton)
		render_skeleton(character, get_view_projection());
	if (g_visualizeNodeTransforms)
		render_skeleton_transforms(character, get_view_projection());
  }
}
