#include "scene.h"
#include "engine/render/shader.h"
#include "engine/render/material.h"
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <immintrin.h>

bool g_visualizeBoneWeights = false;
bool g_visualizeSkeleton = false;
bool g_visualizeNodeTransforms = false;

// Helper function to extract position from ozz::math::Float4x4
inline glm::vec3 extract_position(const ozz::math::Float4x4& matrix) {
	alignas(16) float values[4];
	_mm_storeu_ps(values, matrix.cols[3]);
	return glm::vec3(values[0], values[1], values[2]);
}

// Helper function to extract axis from ozz::math::Float4x4
inline glm::vec3 extract_axis(const ozz::math::Float4x4& matrix, int col_index) {
	alignas(16) float values[4];
	_mm_storeu_ps(values, matrix.cols[col_index]);
	return glm::vec3(values[0], values[1], values[2]);
}

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
	if (!character.ozz_skeleton) return;

	static ShaderPtr line_shader = compile_shader("line", "sources/shaders/line_vs.glsl", "sources/shaders/line_ps.glsl");
	if (!line_shader) return;
	line_shader->use();
	line_shader->set_mat4x4("view_projection", cameraProjView);
	line_shader->set_vec3("color", glm::vec3(1.f, 1.f, 0.f));

	glDisable(GL_DEPTH_TEST);

	// Use animated model_space_matrices if available, otherwise use rest poses
	const ozz::vector<ozz::math::Float4x4>* models = nullptr;
	if (character.ozz_animation)
	{
		// Use current animation pose
		models = &character.model_space_matrices;
	}
	else
	{
		// Fall back to rest pose
		static ozz::vector<ozz::math::Float4x4> rest_models;
		rest_models.clear();
	
		ozz::animation::LocalToModelJob job;
		job.skeleton = character.ozz_skeleton;
		job.input = character.ozz_skeleton->joint_rest_poses();
		rest_models.resize(character.ozz_skeleton->num_joints());
		job.output = ozz::make_span(rest_models);
		
		if (!job.Run())
			return;
		
		models = &rest_models;
	}

	// Get joint parent indices
	const ozz::span<const int16_t> parents = character.ozz_skeleton->joint_parents();

	std::vector<glm::vec3> points;
	for (int i = 0; i < character.ozz_skeleton->num_joints(); ++i)
	{
		int16_t parent_idx = parents[i];
		if (parent_idx != -1)
		{
			// Extract positions from Float4x4 matrices
			glm::vec3 parent_pos = extract_position((*models)[parent_idx]);
			glm::vec3 child_pos = extract_position((*models)[i]);

			points.push_back(parent_pos);
			points.push_back(child_pos);
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
	if (!character.ozz_skeleton) return;

	static ShaderPtr shader = compile_shader("colored_line", "sources/shaders/colored_line_vs.glsl", "sources/shaders/colored_line_ps.glsl");
	if (!shader) return;
	shader->use();
	shader->set_mat4x4("view_projection", cameraProjView);

	glDisable(GL_DEPTH_TEST);

	// Use animated model_space_matrices if available, otherwise use rest poses
	const ozz::vector<ozz::math::Float4x4>* models = nullptr;
	if (character.ozz_animation)
	{
		// Use current animation pose
		models = &character.model_space_matrices;
	}
	else
	{
		// Fall back to rest pose
		static ozz::vector<ozz::math::Float4x4> rest_models;
		rest_models.clear();
	
		ozz::animation::LocalToModelJob job;
		job.skeleton = character.ozz_skeleton;
		job.input = character.ozz_skeleton->joint_rest_poses();
		rest_models.resize(character.ozz_skeleton->num_joints());
		job.output = ozz::make_span(rest_models);
		
		if (!job.Run())
			return;
		
		models = &rest_models;
	}

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
	};
	std::vector<Vertex> points;

	float axis_length = 0.1f;
	for (int i = 0; i < character.ozz_skeleton->num_joints(); ++i)
	{
		// Extract position from Float4x4 matrix
		glm::vec3 origin = extract_position((*models)[i]);

		// Extract and normalize axes from matrix columns
		glm::vec3 x_axis = glm::normalize(extract_axis((*models)[i], 0)) * axis_length;
		glm::vec3 y_axis = glm::normalize(extract_axis((*models)[i], 1)) * axis_length;
		glm::vec3 z_axis = glm::normalize(extract_axis((*models)[i], 2)) * axis_length;

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

  if (character.ozz_skeleton)
  {
    ozz::animation::LocalToModelJob job;
    job.skeleton = character.ozz_skeleton;
    job.input = character.ozz_skeleton->joint_rest_poses();
    
    ozz::vector<ozz::math::Float4x4> models;
    models.resize(character.ozz_skeleton->num_joints());
    job.output = ozz::make_span(models);

    if (job.Run())
    {
        std::vector<glm::mat4> bone_matrices;
        bone_matrices.resize(character.ozz_skeleton->num_joints());

        for (int i = 0; i < character.ozz_skeleton->num_joints(); ++i)
        {
          // For T-pose, use identity matrices (no skinning)
          bone_matrices[i] = glm::identity<glm::mat4>();
        }
        shader.set_mat4x4_array("bone_matrices", bone_matrices.data(), bone_matrices.size());
    }
  }

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
