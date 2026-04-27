#pragma once
#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"
#include <vector>
#include <string>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/allocator.h>

struct Mesh;
struct Material;

struct Bone
{
	std::string name = "";
	int parent_idx = -1;
	glm::mat4 offset_matrix = glm::mat4(1.f);
	glm::mat4 local_transform = glm::mat4(1.f);
	std::vector<int> children_indices;
};

struct Skeleton
{
	std::vector<Bone> bones;
	std::vector<glm::mat4> bone_matrices;
};

// State for a single animation within a character
struct AnimationState
{
	ozz::animation::Animation* animation = nullptr;
	std::string name;
	
	float animation_time = 0.f;
	ozz::vector<ozz::math::SoaTransform> local_transforms;
	ozz::vector<ozz::math::Float4x4> model_space_matrices;
	ozz::animation::SamplingJob::Context* sampling_context = nullptr;
};

struct Character
{
	std::string name;
	glm::mat4 transform;
	std::vector<MeshPtr> meshes;
	MaterialPtr material;
	Skeleton skeleton;
	ozz::animation::Skeleton* ozz_skeleton = nullptr;
	
	// Animation states (each animation has its own state)
	std::vector<AnimationState> animation_states;
	int current_animation_index = 1;  // Start with Idle (index 0 is Rest Pose)
	
	std::vector<glm::mat4> inverse_bind_matrices;
	
	// Movement speed constants
	static constexpr float speed_idle = 0.f;
	static constexpr float speed_wasd = 2.f;
	static constexpr float speed_wasd_shift = 3.f;

	static constexpr float MIN_SPEED = speed_idle;
	static constexpr float DIVIDE_SPEED = speed_wasd;
	static constexpr float MAX_SPEED = speed_wasd_shift;

	// Current speed (interpolated)
	float current_speed = 0.f;

	bool is_blending = false;

	// Blending buffer for animation blending results
	ozz::vector<ozz::math::SoaTransform> blended_locals;
	
	// Helper to get current animation state
	AnimationState* get_current_animation_state() {
		if (current_animation_index >= 0 && current_animation_index < animation_states.size()) {
			return &animation_states[current_animation_index];
		}
		return nullptr;
	}
	
	const AnimationState* get_current_animation_state() const {
		if (current_animation_index >= 0 && current_animation_index < animation_states.size()) {
			return &animation_states[current_animation_index];
		}
		return nullptr;
	}
};
