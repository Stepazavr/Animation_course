#pragma once
#include "engine/3dmath.h"
#include "engine/render/material.h"
#include "engine/render/mesh.h"
#include <vector>
#include <string>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/animation.h>
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

struct Character
{
	std::string name;
	glm::mat4 transform;
	std::vector<MeshPtr> meshes;
	MaterialPtr material;
	Skeleton skeleton;
	ozz::animation::Skeleton* ozz_skeleton = nullptr;
	ozz::animation::Animation* ozz_animation;

	float animation_time = 0.f;
	ozz::vector<ozz::math::SoaTransform> local_transforms;
	ozz::vector<ozz::math::Float4x4> model_space_matrices;
};
